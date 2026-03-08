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
#include <vector>
#include <map>

// Universal wrapper for reflected invokers
class UniversalInvoker {
private:
    std::function<std::any(std::vector<std::any>)> invoker_;
    std::string method_name_;

public:
    UniversalInvoker() = default;

    template <typename Fn>
    UniversalInvoker(Fn&& fn, std::string_view name)
        : method_name_(name) {
        invoker_ = [fn = std::forward<Fn>(fn)](std::vector<std::any> /*args*/) -> std::any {
            // This wraps the generic lambda into a vector-based interface
            // In practice, you'd unpack args based on runtime info
            // For now, we store the lambda as-is for direct call
            throw std::runtime_error("Use invoke_with template for typed calls");
        };
    }

    // Alternative: store the original lambda directly
    template <typename Fn>
    static auto wrap(Fn&& fn, std::string_view name) {
        return std::pair{std::string(name), std::function([fn = std::forward<Fn>(fn)](auto&&... args) -> std::any {
            return fn(std::forward<decltype(args)>(args)...);
        })};
    }

    const std::string& name() const { return method_name_; }
};

// Type-erased invoker storage using std::function with variadic template
using ReflectedInvoker = std::function<std::any(std::any const&, ...)>;

// Helper to convert reflected lambda to a storable form
template <typename Lambda>
auto to_universal_invoker(Lambda&& lambda) {
    return [fn = std::forward<Lambda>(lambda)](auto&&... args) -> std::any {
        return fn(std::forward<decltype(args)>(args)...);
    };
}

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
        return [c, method_name](auto&&... args) -> std::any {
            return Invokable::call_method_any_with_args<C>(c, method_name, std::forward<decltype(args)>(args)...);
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
    static std::any call_method_any_with_args(T* obj, std::string_view method_name, Args&&... args) {
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

    double distance_3d(double x1, double y1, double z1, double x2, double y2, double z2) {
        return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z2 - z1) * (z2 - z1));
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

    // Original usage still works
    auto typed_add = tc.make_invoker<int(int, int)>("add");
    std::cout << "Typed add(5, 6): " << typed_add(5, 6) << "\n";

    // Store reflected invokers in a map
    using InvokerType = decltype(tc.make_invoker("add"));
    std::map<std::string, InvokerType> invoker_map;
    invoker_map.emplace("add", tc.make_invoker("add"));
    invoker_map.emplace("distance", tc.make_invoker("distance"));
    invoker_map.emplace("toString", tc.make_invoker("toString"));
    invoker_map.emplace("distance_3d", tc.make_invoker("distance_3d"));

    // Use them from the map
    std::cout << "\n=== Using stored invokers ===\n";
    std::cout << "Map add(8, 9): " << std::any_cast<int>(invoker_map.at("add")(8, 9)) << "\n";
    std::cout << "Map distance(0, 0, 3, 4): " << std::any_cast<double>(invoker_map.at("distance")(0.0, 0.0, 3.0, 4.0)) << "\n";
    std::cout << "Map toString(1.5, 2.5): " << std::any_cast<std::string>(invoker_map.at("toString")(1.5, 2.5)) << "\n";
    std::cout << "Map distance_3d(0, 0, 0, 1, 2, 2): " << std::any_cast<double>(invoker_map.at("distance_3d")(0, 0, 0, 1, 2, 2)) << "\n";

    // Alternative: store in std::vector with a wrapper
    struct InvokerEntry {
        std::string name;
        std::function<std::any(int, int)> fn_2_int;
        std::function<std::any(double, double, double, double)> fn_4_double;
        int arg_count;

        InvokerEntry(std::string n, decltype(tc.make_invoker("add")) fn, int ac)
            : name(std::move(n)), arg_count(ac) {
            if (ac == 2) {
                fn_2_int = [fn](int a, int b) { return fn(a, b); };
            } else if (ac == 4) {
                fn_4_double = [fn](double a, double b, double c, double d) { return fn(a, b, c, d); };
            }
        }
    };

    std::vector<InvokerEntry> invokers;
    invokers.emplace_back("add", tc.make_invoker("add"), 2);
    invokers.emplace_back("distance", tc.make_invoker("distance"), 4);

    std::cout << "\n=== Using vector storage ===\n";
    std::cout << "Vector add(10, 20): " << std::any_cast<int>(invokers[0].fn_2_int(10, 20)) << "\n";

    // Best approach: Use the decltype directly in a map
    std::cout << "\n=== Best approach: decltype in container ===\n";
    std::unordered_map<std::string, InvokerType> best_map;

    best_map.emplace("add", tc.make_invoker("add"));
    best_map.emplace("distance", tc.make_invoker("distance"));
    best_map.emplace("toString", tc.make_invoker("toString"));
    best_map.emplace("distance_3d", tc.make_invoker("distance_3d"));

    std::cout << "Best map add(100, 200): " << std::any_cast<int>(best_map.at("add")(100, 200)) << "\n";
    std::cout << "Best map add(1, 2, 3): " << std::any_cast<int>(best_map.at("add")(1, 2, 3)) << "\n";

    return 0;
}