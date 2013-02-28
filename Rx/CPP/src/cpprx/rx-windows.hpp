// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#if !defined(CPPRX_RX_WINDOWS_HPP)
#define CPPRX_RX_WINDOWS_HPP
#pragma once

#if defined(WINDOWS) || defined(WIN32) || defined(_WIN32)

#define NOMINMAX
#include <Windows.h>

namespace rxcpp
{

#define OBSERVE_ON_DISPATCHER_OP

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
            auto p = new Fn(fn);
            ::PostMessage(hwnd, WindowClass::WM_USER_DISPATCH, (WPARAM)(void(*)(void*))&run_proc<Fn>, (LPARAM)(void*)p);
        }
        template <class Fn>
        static void run_proc(
            void* pvfn
        )
        {
            auto fn = (Fn*)(void*) pvfn;
            (*fn)();
            delete fn;
        }
    };

}

#endif

#endif
