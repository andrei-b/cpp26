#include <any>
#include <cmath>
#include <functional>
#include <iostream>
#include <meta>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

class Invokable {
public:
    template <class C, typename Sig>
    struct function_traits;

    template <class C, typename R, typename... Args>
    struct function_traits<C, R(Args...)> {
        using return_type = R;
        using args_tuple = std::tuple<Args...>;
    };

    virtual ~Invokable() = default;

protected:
    template <class C, typename Sig>
    auto make_invoker(C* c, std::string_view method_name) {
        using traits = function_traits<C, Sig>;
        using R = typename traits::return_type;
        using args_tuple = typename traits::args_tuple;
        return make_invoker_from_tuple<C, R>(c, method_name, args_tuple{});
    }

    template <class C>
    auto make_invoker_reflected(C* c, std::string_view method_name) {
        return [this, c, method_name](auto&&... args) -> std::any {
            return call_method_any_with_args<C>(c, method_name, std::forward<decltype(args)>(args)...);
        };
    }

    template <typename T, typename Sig>
    decltype(auto) call_method(T&& obj, std::string_view method_name) {
        using traits = function_traits<T, Sig>;
        using R = typename traits::return_type;
        using args_tuple = typename traits::args_tuple;
        return call_method_impl<T, R>(std::forward<T>(obj), method_name, args_tuple{});
    }

    template <typename T, typename R, typename... Args>
    R call_method_impl_with_args(T& obj, std::string_view method_name, Args&&... args) {
        constexpr auto ctx = std::meta::access_context::current();
        template for (constexpr auto m : std::define_static_array(std::meta::members_of(^^T, ctx))) {
            if constexpr (std::meta::is_function(m) && std::meta::has_identifier(m)) {
                if (method_name == std::meta::identifier_of(m)) {
                    if constexpr (std::meta::is_static_member(m)) {
                        auto fn = &[:m:];
                        if constexpr (std::is_invocable_r_v<R, decltype(fn), Args...>) {
                            return fn(std::forward<Args>(args)...);
                        }
                    } else {
                        auto pmf = &[:m:];
                        if constexpr (std::is_invocable_r_v<R, decltype(pmf), T&, Args...>) {
                            return (obj.*pmf)(std::forward<Args>(args)...);
                        }
                    }
                }
            }
        }
        throw std::runtime_error("method not found or argument mismatch");
    }

    template <typename T, typename... Args>
    std::any call_method_any_with_args(T* obj, std::string_view method_name, Args&&... args) {
        constexpr auto ctx = std::meta::access_context::current();
        bool matched = false;
        std::any result;

        template for (constexpr auto m : std::define_static_array(std::meta::members_of(^^T, ctx))) {
            if constexpr (std::meta::is_function(m) && std::meta::has_identifier(m)) {
                if (method_name == std::meta::identifier_of(m)) {
                    if constexpr (std::meta::is_static_member(m)) {
                        auto fn = &[:m:];
                        if constexpr (requires { fn(std::forward<Args>(args)...); }) {
                            if (matched) {
                                throw std::runtime_error("ambiguous overload for provided arguments");
                            }
                            matched = true;
                            if constexpr (std::is_void_v<std::invoke_result_t<decltype(fn), Args...>>) {
                                fn(std::forward<Args>(args)...);
                                result = std::any{};
                            } else {
                                result = std::any(fn(std::forward<Args>(args)...));
                            }
                        }
                    } else {
                        auto pmf = &[:m:];
                        if constexpr (requires { (obj->*pmf)(std::forward<Args>(args)...); }) {
                            if (matched) {
                                throw std::runtime_error("ambiguous overload for provided arguments");
                            }
                            matched = true;
                            if constexpr (std::is_void_v<std::invoke_result_t<decltype(pmf), T&, Args...>>) {
                                (obj->*pmf)(std::forward<Args>(args)...);
                                result = std::any{};
                            } else {
                                result = std::any((obj->*pmf)(std::forward<Args>(args)...));
                            }
                        }
                    }
                }
            }
        }

        if (!matched) {
            throw std::runtime_error("method not found or argument mismatch");
        }
        return result;
    }

private:
    template <class C, typename R, typename... Args>
    std::function<R(Args...)> make_invoker_impl(C* c, std::string_view method_name) {
        constexpr auto ctx = std::meta::access_context::current();
        template for (constexpr auto m : std::define_static_array(std::meta::members_of(^^C, ctx))) {
            if constexpr (std::meta::is_function(m) && std::meta::has_identifier(m)) {
                if (method_name == std::meta::identifier_of(m)) {
                    if constexpr (std::meta::is_static_member(m)) {
                        auto fn = &[:m:];
                        if constexpr (std::is_invocable_r_v<R, decltype(fn), Args...>) {
                            return [fn](Args... args) -> R {
                                return fn(std::forward<Args>(args)...);
                            };
                        }
                    } else {
                        auto pmf = &[:m:];
                        if constexpr (std::is_invocable_r_v<R, decltype(pmf), C*, Args...>) {
                            return [c, pmf](Args... args) -> R {
                                return (c->*pmf)(std::forward<Args>(args)...);
                            };
                        }
                    }
                }
            }
        }
        throw std::runtime_error("method not found or signature mismatch");
    }

    template <class C, typename R, typename... Args>
    auto make_invoker_from_tuple(C* c, std::string_view method_name, std::tuple<Args...>) {
        return make_invoker_impl<C, R, Args...>(c, method_name);
    }

    template <typename T, typename R, typename ArgsTuple>
    decltype(auto) call_method_impl(T&& obj, std::string_view method_name, ArgsTuple&&) {
        constexpr auto ctx = std::meta::access_context::current();
        template for (constexpr auto m : std::define_static_array(std::meta::members_of(^^std::remove_cvref_t<T>, ctx))) {
            if constexpr (std::meta::is_function(m) && std::meta::has_identifier(m)) {
                if (method_name == std::meta::identifier_of(m)) {
                    if constexpr (std::meta::is_static_member(m)) {
                        auto fn = &[:m:];
                        if constexpr (std::is_invocable_v<decltype(fn)>) {
                            return fn();
                        }
                    } else {
                        auto pmf = &[:m:];
                        if constexpr (std::is_invocable_v<decltype(pmf), T>) {
                            return (std::forward<T>(obj).*pmf)();
                        }
                    }
                }
            }
        }
        throw std::runtime_error("method not found or argument mismatch");
    }
};

class TestClass : public Invokable {
public:
    void hello() {
        std::cout << "hello()\n";
    }

    int add(int a, int b) {
        return a + b;
    }

    int add(int a, int b, int c) {
        return a + b + c;
    }

    double distance(double x1, double y1, double x2, double y2) {
        return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
    }

    std::string toString(double x, double y) {
        return std::to_string(x) + ", " + std::to_string(y);
    }

    template <typename Sig>
    requires (std::tuple_size_v<typename function_traits<TestClass, Sig>::args_tuple> == 0)
    decltype(auto) call_method(std::string_view method_name) {
        using T = std::remove_reference_t<decltype(*this)>;
        using traits = function_traits<T, Sig>;
        using R = typename traits::return_type;
        return call_method_impl_with_args<T, R>(*this, method_name);
    }

    template <typename Sig, typename... Args>
    decltype(auto) call_method(std::string_view method_name, Args&&... args) {
        using T = std::remove_reference_t<decltype(*this)>;
        using traits = function_traits<T, Sig>;
        using R = typename traits::return_type;
        return call_method_impl_with_args<T, R, Args...>(*this, method_name, std::forward<Args>(args)...);
    }

    auto make_invoker(std::string_view method_name) {
        return make_invoker_reflected(this, method_name);
    }

    template <typename Sig>
    auto make_invoker(std::string_view method_name) {
        return Invokable::make_invoker<TestClass, Sig>(this, method_name);
    }
};

int main() {
    TestClass tc;

    auto typed_add = tc.make_invoker<int(int, int)>("add");
    std::cout << "Typed add(5, 6): " << typed_add(5, 6) << "\n";

    auto reflected = tc.make_invoker("add");
    std::cout << "Reflected add(8, 9): " << std::any_cast<int>(reflected(8, 9)) << "\n";
    std::cout << "Reflected add(1, 2, 3): " << std::any_cast<int>(reflected(1, 2, 3)) << "\n";
    auto distance = tc.make_invoker("distance");
    std::cout << "Reflected distance(0, 0, 3, 4): " << std::any_cast<double>(distance(0, 0, 3, 4)) << "\n";
    auto toString = tc.make_invoker("toString");
    std::cout << "Reflected toString(1.5, 2.5): " << std::any_cast<std::string>(toString(1.5, 2.5)) << "\n";
    return 0;
}