#include <iostream>
#include <meta>
#include <functional>
#include <stdexcept>
#include <string_view>
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

        if constexpr (std::tuple_size_v<typename traits::args_tuple> == 2) {
            using Arg1 = std::tuple_element_t<0, typename traits::args_tuple>;
            using Arg2 = std::tuple_element_t<1, typename traits::args_tuple>;
            return make_invoker<C, R, Arg1, Arg2>(c, method_name);
        }
        throw std::runtime_error("Unsupported signature");
    }

    template <typename T, typename Sig>
    decltype(auto) call_method(T&& obj, std::string_view method_name) {
        using traits = function_traits<T, Sig>;
        using R = typename traits::return_type;
        using args_tuple = typename traits::args_tuple;

        return call_method_impl<T, R>(std::forward<T>(obj), method_name, args_tuple{});
    }

    template <typename T, typename R, typename... Args>
        R call_method_impl_with_args(T& obj, std::string_view method_name, Args&&... args)
            requires true
    {
        constexpr auto ctx = std::meta::access_context::current();
        template for (constexpr auto m : std::define_static_array(std::meta::members_of(^^T, ctx)))
        {
            if constexpr (std::meta::is_function(m) && std::meta::has_identifier(m))
            {
                if (method_name == std::meta::identifier_of(m))
                {
                    if constexpr (std::meta::is_static_member(m))
                    {
                        auto fn = &[:m:];
                        if constexpr (std::is_invocable_r_v<R, decltype(fn), Args...>) {
                            return fn(std::forward<Args>(args)...);
                        }
                    }
                    else
                    {
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

private:

    template <typename T, typename R, typename ArgsTuple>
        decltype(auto) call_method_impl(T&& obj, std::string_view method_name, ArgsTuple&&) {
        constexpr auto ctx = std::meta::access_context::current();
        template for (constexpr auto m : std::define_static_array(std::meta::members_of(^^std::remove_cvref_t<T>, ctx)))
        {
            if constexpr (std::meta::is_function(m) && std::meta::has_identifier(m))
            {
                if (method_name == std::meta::identifier_of(m))
                {
                    if constexpr (std::meta::is_static_member(m))
                    {
                        auto fn = &[:m:];
                        if constexpr (std::is_invocable_v<decltype(fn)>) {
                            return fn();
                        }
                    }
                    else
                    {
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


    template <class C, typename R, typename... Args>
    std::function<R(Args... )> make_invoker_impl(C * c, std::string_view method_name) {
        template for (constexpr auto m : reflected_members<C>())
        {
            if constexpr (std::meta::is_function(m) && std::meta::has_identifier(m))
            {
                if (method_name == std::meta::identifier_of(m))
                {
                    if constexpr (std::meta::is_static_member(m))
                    {
                        auto fn = &[:m:];
                        if constexpr (requires { fn(std::declval<Args>()...); }) {
                            return [fn](Args... args) -> R {
                                return fn(std::forward<Args>(args)...);
                            };
                        }
                    }
                    else
                    {
                        auto pmf = &[:m:];
                        if constexpr (requires { (std::declval<C>().*pmf)(std::declval<Args>()...); }) {
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

};

class TestClass: public Invokable {
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
    ~TestClass() override = default;
};

// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.
int main() {
    TestClass tc;
    tc.call_method<void()>("hello");
    int sum = tc.call_method<int(int, int)>("add", 3, 4);
    auto d = tc.call_method<double(double, double, double, double)>("distance", 0.0, 0.0, 3.0, 4.0);
    std::cout << "Sum: " << sum << "\n";
    std::string str = tc.call_method<std::string(double, double)>("toString", 1.6, 2.5);
    std::cout << "String: " << str << "\n";
    std::cout << "Distance: " << d << "\n";
    auto s1 = tc.call_method<int(int, int, int)>("add", 1, 2, 3);
    std::cout << "Sum with 3 args: " << s1 << "\n";
    return 0;
    // TIP See CLion help at <a href="https://www.jetbrains.com/help/clion/">jetbrains.com/help/clion/</a>. Also, you can try interactive lessons for CLion by selecting 'Help | Learn IDE Features' from the main menu.
}