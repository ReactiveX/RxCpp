// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once
#include "rx-includes.hpp"

#if !defined(CPPRX_RX_WINDOWS_HPP)
#define CPPRX_RX_WINDOWS_HPP
#pragma once

#if defined(WINDOWS) || defined(WIN32) || defined(_WIN32)

#pragma comment(lib, "user32.lib")

#define NOMINMAX
#include <Windows.h>

namespace rxcpp { namespace win32 {
 

#if !defined(OBSERVE_ON_DISPATCHER_OP)
#define OBSERVE_ON_DISPATCHER_OP rxcpp::win32::ObserveOnDispatcherOp
#endif

    struct ObserveOnDispatcherOp
    {
        HWND hwnd;

        ObserveOnDispatcherOp(): hwnd(WindowClass::Instance().CreateWindow_())
        {
            if (!hwnd)
                throw std::exception("error");
        }
        ~ObserveOnDispatcherOp()
        {
            // send one last message to ourselves to shutdown.
            post([=]{ CloseWindow(hwnd); });
        }

        struct WindowClass
        {
            static const wchar_t* const className(){ return L"ObserveOnDispatcherOp::WindowClass"; }
            WindowClass()
            {
                WNDCLASS wndclass = {};
                wndclass.style = 0;
                wndclass.lpfnWndProc = &WndProc;
                wndclass.cbClsExtra;
                wndclass.cbWndExtra = 0;
                wndclass.hInstance = NULL;
                wndclass.lpszClassName = className();

                if (!RegisterClass(&wndclass))
                    throw std::exception("error");
                
            }
            HWND CreateWindow_()
            {
                return CreateWindowEx(0, WindowClass::className(), L"MessageOnlyWindow", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, 0);
            }
            static const int WM_USER_DISPATCH = WM_USER + 1;

            static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
            {
                switch (message)
                {
                    // TODO: shatter attack surface. should validate the message, e.g. using a handle table.
                case WM_USER_DISPATCH:
                    ((void(*)(void*))wParam)((void*)lParam);
                    return 0;
                default:
                    return DefWindowProc(hwnd, message, wParam, lParam);
                }
            }
            static WindowClass& Instance() { 
                static WindowClass instance;
                return instance;
            }
        };

        template <class Fn>
        void post(Fn fn) const
        {
            std::unique_ptr<Fn> f(new Fn(fn));
            ::PostMessage(hwnd, WindowClass::WM_USER_DISPATCH, (WPARAM)(void(*)(void*))&run_proc<Fn>, (LPARAM)(void*)f.release());
        }
        template <class Fn>
        static void run_proc(
            void* pvfn
        )
        {
            std::unique_ptr<Fn> f((Fn*)(void*) pvfn);
            (*f.get())();
        }
    };

    class WindowScheduler : public rxcpp::LocalScheduler
    {

        struct compare_work
        {
            template <class T>
            bool operator()(const T& work1, const T& work2) const {
                return work1.first > work2.first;
            }
        };
        
        struct Queue;

        typedef std::pair<Scheduler::shared, Work> Item;

        typedef std::priority_queue<
            std::pair<clock::time_point, std::shared_ptr<Item>>,
            std::vector<std::pair<clock::time_point, std::shared_ptr<Item>>>,
            compare_work 
        > ScheduledWork;

        struct Queue 
        {
            Queue() : exit(false), window(NULL) {}
            bool exit;
            HWND window;
            ScheduledWork scheduledWork;
            mutable std::mutex lock;
        };

        struct WindowClass
        {
            std::shared_ptr<Queue> queue;

            static const wchar_t* const className(){ return L"rxcpp::win32::WindowScheduler::WindowClass"; }
            WindowClass()
            {
            }

            static void Create(const std::shared_ptr<Queue>& queue)
            {
                WNDCLASSW wndclass = {};
                wndclass.style = 0;
                wndclass.lpfnWndProc = &WndProc;
                wndclass.cbWndExtra = 0;
                wndclass.hInstance = NULL;
                wndclass.lpszClassName = className();

                RegisterClassW(&wndclass);
                
                std::unique_ptr<WindowClass> that(new WindowClass);
                that->queue = queue;

                queue->window = CreateWindowExW(0, WindowClass::className(), L"MessageOnlyWindow", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, reinterpret_cast<LPVOID>(that.get()));
                if (!queue->window)
                    throw std::exception("create window failed");

                that.release();
            }

            static const int WM_USER_DISPATCH = WM_USER + 1;

            static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
            {
                static WindowClass* windowClass; 
                switch (message)
                {
                case WM_NCCREATE:
                    {
                        windowClass = reinterpret_cast<WindowClass*>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
                    }
                    break;
                case WM_NCDESTROY:
                    {
                        delete windowClass;
                        windowClass = nullptr;
                    }
                    break;
                case WM_TIMER:
                case WM_USER_DISPATCH:
                    {
                        bool destroy = false;
                        HWND window = NULL;
                        {
                            std::unique_lock<std::mutex> guard(windowClass->queue->lock);

                            while (!windowClass->queue->scheduledWork.empty() && !windowClass->queue->scheduledWork.top().second.get()->second)
                            {
                                // discard the disposed items
                                windowClass->queue->scheduledWork.pop();
                            }

                            if (!windowClass->queue->scheduledWork.empty())
                            {                
                                auto& item = windowClass->queue->scheduledWork.top();
                                auto now = item.second.get()->first->Now();
                
                                // wait until the work is due
                                if(now < item.first)
                                {
                                    auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(item.first - now).count();
                                    if (remaining >= USER_TIMER_MINIMUM)
                                    {
                                        return SetTimer(hwnd, 0, static_cast<UINT>(remaining), nullptr);
                                    }
                                    std::this_thread::sleep_until(item.first);
                                }
                
                                // dispatch work
                                auto work = std::move(item.second.get()->second);
                                auto scheduler = std::move(item.second.get()->first);
                                windowClass->queue->scheduledWork.pop();

                                {
                                    RXCPP_UNWIND_AUTO([&]{guard.lock();});
                                    guard.unlock();
                                    LocalScheduler::Do(work, scheduler);
                                    work = nullptr;
                                    scheduler = nullptr;
                                }

                                if (!windowClass->queue->scheduledWork.empty())
                                {
                                    ::PostMessageW(windowClass->queue->window, WindowClass::WM_USER_DISPATCH, 0, 0);
                                }
                            }

                            destroy = windowClass->queue->exit && windowClass->queue->scheduledWork.empty();
                            window = windowClass->queue->window;
                        }

                        if (destroy)
                        {
                            DestroyWindow(window); 
                        }
                    }
                    return 0;
                default:
                    break;
                }
                return DefWindowProcW(hwnd, message, wParam, lParam);
            }
        };
        std::shared_ptr<Queue> queue;

    public:
        WindowScheduler()
            : queue(std::make_shared<Queue>())
        {
            WindowClass::Create(queue);
        }
        ~WindowScheduler()
        {
            // send one last message to ourselves to shutdown.
            {
                std::unique_lock<std::mutex> guard(queue->lock);
                queue->exit = true;
            }
            ::PostMessageW(queue->window, WindowClass::WM_USER_DISPATCH, 0, 0);
        }

        using LocalScheduler::Schedule;
        virtual Disposable Schedule(clock::time_point dueTime, Work work)
        {
            auto cancelable = std::make_shared<Item>(std::make_pair(get(), std::move(work)));
            {
                std::unique_lock<std::mutex> guard(queue->lock);
                queue->scheduledWork.push(std::make_pair(dueTime, cancelable));
            }
            ::PostMessageW(queue->window, WindowClass::WM_USER_DISPATCH, 0, 0);
            auto local = queue;
            return Disposable([local, cancelable]{
                std::unique_lock<std::mutex> guard(local->lock);
                cancelable.get()->second = nullptr;
                ::PostMessageW(local->window, WindowClass::WM_USER_DISPATCH, 0, 0);
            });
        }
    };
} }

#endif

#endif
