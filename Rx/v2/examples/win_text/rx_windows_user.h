#pragma once

namespace rxcpp { namespace windows_user {

    struct rx_messages
    {
        struct Result
        {
            LRESULT lres = 0;
            bool handled = false;
        };

        struct Message
        {
            template<UINT WM>
            static auto is() { return [](Message m){ return m.message == WM; }; }

            HWND hWnd;
            UINT message;
            WPARAM wParam;
            LPARAM lParam;
            Result* result;

            void handled() { result->handled = true; }
            void lresult(LRESULT lres) {result->lres = lres; }

            template<class T>
            T wparam_cast(){
                return *reinterpret_cast<T*>(std::addressof(wParam));
            }

            template<class T>
            T lparam_cast(){
                return *reinterpret_cast<T*>(std::addressof(lParam));
            }
        };

        template<class WPARAM_t = WPARAM, class LPARAM_t = LPARAM>
        struct TypedMessage
        {
            static auto as() { return [](Message m){return TypedMessage{m}; }; }

            TypedMessage(Message m)
                : hWnd(m.hWnd)
                , message(m.message)
                , wParam(m.wparam_cast<WPARAM_t>())
                , lParam(m.lparam_cast<LPARAM_t>())
                , result(m.result)
            {}

            HWND hWnd;
            UINT message;
            WPARAM_t wParam;
            LPARAM_t lParam;
            Result* result;

            void handled() { result->handled = true; }
            void lresult(LRESULT lres) {result->lres = lres; }
        };

        subjects::subject<Message> subject;
        subscriber<Message> sub;

        ~rx_messages() {
            sub.on_completed();
        }
        rx_messages() : sub(subject.get_subscriber()) {}

        std::tuple<bool, LRESULT> message(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
            Result result;
            auto m = Message{hWnd, message, wParam, lParam, &result};
            try {
                sub.on_next(m);
            } catch(...) {
                sub.on_error(std::current_exception());
            }
            return std::make_tuple(result.handled, result.lres);
        }

        observable<Message> messages() {
            return subject.get_observable();
        }

        template<UINT WM>
        observable<Message> messages() {
            return messages().filter(Message::is<WM>());
        }

        template<UINT WM, class WPARAM_t, class LPARAM_t = LPARAM>
        observable<TypedMessage<WPARAM_t, LPARAM_t>> messages() {
            return messages<WM>().map(TypedMessage<WPARAM_t, LPARAM_t>::as());
        }

    };

    template<class Derived, UINT WM>
    struct enable_send_call
    {
        static LRESULT send_call(HWND w, std::function<LRESULT(Derived&)> f) {
            auto fp = reinterpret_cast<LPARAM>(std::addressof(f));
            return SendMessage(w, WM, 0, fp);
        }

        void OnSendCall() {
            auto derived = static_cast<Derived*>(this);
            derived->messages<WM, WPARAM, std::function<LRESULT(Derived&)>*>().
                subscribe([=](auto m) {
                    m.handled(); // skip DefWindowProc
                    m.lresult((*m.lParam)(*derived));
                });
        }

        enable_send_call() {
            OnSendCall();
        }
    };
} }
