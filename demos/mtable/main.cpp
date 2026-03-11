#include <any>
#include <cmath>
#include <functional>
#include <iostream>
#include <meta>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

struct MethodEntry {
    std::string_view name;
    std::function<std::any(void*, std::span<const std::any>)> invoke;
    bool is_static = false;
    bool is_const = false;
    bool is_virtual = false;
    std::string_view return_type;
    size_t argcount = 0;
};

// ============================================================
// Base runtime interface
// ============================================================

class Invokable {
public:
    virtual ~Invokable() = default;

    virtual std::any invoke(std::string_view method_name,
                            std::span<const std::any> args) = 0;
};

// ============================================================
// Function traits for member functions
// ============================================================

template <typename T>
struct function_traits;

// non-const member function
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)> {
    using class_type  = C;
    using return_type = R;
    using args_tuple  = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr size_t arity = sizeof...(Args);
};

// const member function
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const> {
    using class_type  = C;
    using return_type = R;
    using args_tuple  = std::tuple<Args...>;
    static constexpr bool is_const = true;
    static constexpr size_t arity = sizeof...(Args);
};

// static/free function pointer
template <typename R, typename... Args>
struct function_traits<R (*)(Args...)> {
    using class_type  = void;
    using return_type = R;
    using args_tuple  = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr size_t arity = sizeof...(Args);
};

// ============================================================
// any conversion helpers
// ============================================================

template <typename T>
using any_arg_t = std::remove_cvref_t<T>;

template <typename T>
T any_cast_value(const std::any& a) {
    using U = any_arg_t<T>;

    if constexpr (std::is_lvalue_reference_v<T>) {
        using Base = std::remove_reference_t<T>;
        auto p = std::any_cast<std::remove_cv_t<Base>>(&a);
        if (!p) {
            throw std::bad_any_cast{};
        }
        return static_cast<T>(*p);
    } else {
        auto p = std::any_cast<U>(&a);
        if (!p) {
            throw std::bad_any_cast{};
        }
        return *p;
    }
}

// ============================================================
// Invoke helpers for member functions
// ============================================================

template <class C, class PMF, class Tuple, std::size_t... I>
std::any invoke_member_with_anys_impl(C& obj,
                                      PMF pmf,
                                      std::span<const std::any> args,
                                      std::index_sequence<I...>) {
    using traits = function_traits<PMF>;
    using R = typename traits::return_type;

    if constexpr (std::is_void_v<R>) {
        (obj.*pmf)(any_cast_value<std::tuple_element_t<I, Tuple>>(args[I])...);
        return std::any{};
    } else {
        return std::any(
            (obj.*pmf)(any_cast_value<std::tuple_element_t<I, Tuple>>(args[I])...)
        );
    }
}

template <class C, class PMF>
std::any invoke_member_with_anys(C& obj,
                                 PMF pmf,
                                 std::span<const std::any> args) {
    using traits = function_traits<PMF>;
    using Tuple = typename traits::args_tuple;
    constexpr std::size_t N = std::tuple_size_v<Tuple>;

    if (args.size() != N) {
        throw std::runtime_error("argument count mismatch");
    }

    return invoke_member_with_anys_impl<C, PMF, Tuple>(
        obj, pmf, args, std::make_index_sequence<N>{});
}

// Forward declarations for static/free functions
template <class FN, class Tuple, std::size_t... I>
std::any invoke_static_with_anys_impl(FN fn,
                                      std::span<const std::any> args,
                                      std::index_sequence<I...>);

template <class FN>
std::any invoke_static_with_anys(FN fn,
                                 std::span<const std::any> args);

// ============================================================
// Invoke helpers for static/free functions
// ============================================================

template <class FN, class Tuple, std::size_t... I>
std::any invoke_static_with_anys_impl(FN fn,
                                      std::span<const std::any> args,
                                      std::index_sequence<I...>) {
    using traits = function_traits<FN>;
    using R = typename traits::return_type;

    if constexpr (std::is_void_v<R>) {
        fn(any_cast_value<std::tuple_element_t<I, Tuple>>(args[I])...);
        return std::any{};
    } else {
        return std::any(
            fn(any_cast_value<std::tuple_element_t<I, Tuple>>(args[I])...)
        );
    }
}

template <class FN>
std::any invoke_static_with_anys(FN fn,
                                 std::span<const std::any> args) {
    using traits = function_traits<FN>;
    using Tuple = typename traits::args_tuple;
    constexpr std::size_t N = std::tuple_size_v<Tuple>;

    if (args.size() != N) {
        throw std::runtime_error("argument count mismatch");
    }

    return invoke_static_with_anys_impl<FN, Tuple>(
        fn, args, std::make_index_sequence<N>{});
}

template<class C, class PMF>
MethodEntry make_member_entry(std::string_view name, PMF pmf)
{
    return MethodEntry{
        name,
        [pmf](void* obj, std::span<const std::any> args) -> std::any {
            return invoke_member_with_anys(*static_cast<C*>(obj), pmf, args);
        }
    };
}

template<class C, class FN>
MethodEntry make_static_entry(std::string_view name, FN fn)
{
    return MethodEntry{
        name,
        [fn]([[maybe_unused]] void* obj, std::span<const std::any> args) -> std::any {
            return invoke_static_with_anys(fn, args);
        }
    };
}


// ============================================================
// Reflection-based dispatcher
// ============================================================

template <class T>
std::any invoke_reflected(T& obj,
                          std::string_view method_name,
                          std::span<const std::any> args) {
    constexpr auto ctx = std::meta::access_context::current();

    bool found_name = false;
    bool matched_overload = false;
    std::any result;

    template for (constexpr auto m :
        std::define_static_array(std::meta::members_of(^^T, ctx))) {

        if constexpr (std::meta::is_function(m) && std::meta::has_identifier(m)) {
            if (method_name == std::meta::identifier_of(m)) {
                found_name = true;

                // static member function
                if constexpr (std::meta::is_static_member(m)) {
                    auto fn = &[:m:];
                    try {
                        result = invoke_static_with_anys(fn, args);
                        if (matched_overload) {
                            throw std::runtime_error("ambiguous overload");
                        }
                        matched_overload = true;
                    } catch (const std::bad_any_cast&) {
                        // wrong argument types, keep scanning overloads
                    } catch (const std::runtime_error&) {
                        // wrong arg count, keep scanning overloads
                    }
                }
                // non-static member function
                else {
                    auto pmf = &[:m:];
                    try {
                        result = invoke_member_with_anys(obj, pmf, args);
                        if (matched_overload) {
                            throw std::runtime_error("ambiguous overload");
                        }
                        matched_overload = true;
                    } catch (const std::bad_any_cast&) {
                        // wrong argument types, keep scanning overloads
                    } catch (const std::runtime_error&) {
                        // wrong arg count, keep scanning overloads
                    }
                }
            }
        }
    }

    if (matched_overload) {
        return result;
    }

    if (found_name) {
        throw std::runtime_error("method found, but no overload matches provided arguments");
    }

    throw std::runtime_error("method not found");
}

// ============================================================
// Example derived class
// ============================================================

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
        return std::sqrt((x2 - x1) * (x2 - x1) +
                         (y2 - y1) * (y2 - y1));
    }

    std::string toString(double x, double y) {
        return std::to_string(x) + ", " + std::to_string(y);
    }

    static int mul(int a, int b) {
        return a * b;
    }

    std::any invoke(std::string_view method_name,
                    std::span<const std::any> args) override {
        return invoke_reflected(*this, method_name, args);
    }
};

// ============================================================
// Small helper for nicer call sites
// ============================================================

template <typename... Args>
std::any invoke_dyn(Invokable* obj,
                    std::string_view method_name,
                    Args&&... args) {
    std::vector<std::any> packed;
    packed.reserve(sizeof...(Args));
    (packed.emplace_back(std::forward<Args>(args)), ...);
    return obj->invoke(method_name, packed);
}

// ============================================================
// Build method table at compile time
// ============================================================

template<class C>
auto get_method_table() {
    constexpr auto ctx = std::meta::access_context::current();

    std::unordered_multimap<std::string_view, MethodEntry> table;

    template for (constexpr auto m :
        std::define_static_array(std::meta::members_of(^^C, ctx))) {
        if constexpr (std::meta::is_function(m) && std::meta::has_identifier(m)) {
            // static member function
            auto pmf = &[:m:];
            using PMF = decltype(pmf);
            using traits = function_traits<PMF>;
            constexpr auto argcount = traits::arity;            
            if constexpr (std::meta::is_static_member(m)) {
                MethodEntry entry = make_static_entry<C, PMF>(std::meta::identifier_of(m), pmf);
                entry.is_static = true;
                entry.is_virtual = false;
                entry.is_const = false;
                entry.argcount = argcount;
                table.insert({entry.name, entry});
            }
            // non-static member function
            else {
                MethodEntry entry = make_member_entry<C, PMF>(std::meta::identifier_of(m), pmf);
                entry.is_static = false;
                if constexpr (std::meta::is_virtual(m)) {
                    entry.is_virtual = true;
                } else {
                    entry.is_virtual = false;
                }
                if constexpr (traits::is_const) {
                    entry.is_const = true;
                } else {
                    entry.is_const = false;
                }
                entry.argcount = argcount;
                table.insert({entry.name, entry});;
            }
            
        }
    }
    return table;
}

// ============================================================
// Demo
// ============================================================

int main() {
    std::cout << "Program started\n";
    std::cout.flush();

    TestClass tc;
    Invokable* p = &tc;


    auto mt = get_method_table<TestClass>();
    for (const auto& entry : mt) {
        std::cout << "Method: \"" << entry.first << "\", args count: " << entry.second.argcount << "\n";
    }

    auto d =  mt.find("distance");
    if (d != mt.end()) {
        std::cout << "Invoking distance via method table...\n";
        auto result = d->second.invoke(p, {0.0, 0.0, 3.0, 4.0});
        std::cout << "Result: " << std::any_cast<double>(result) << "\n";
    } else {
        std::cout << "Method 'distance' not found in method table\n";
    }

    std::cout.flush();

    try {
        std::cout << "add(5, 6) = "
                  << std::any_cast<int>(invoke_dyn(p, "add", 5, 6))
                  << "\n";

        std::cout << "add(1, 2, 3) = "
                  << std::any_cast<int>(invoke_dyn(p, "add", 1, 2, 3))
                  << "\n";

        std::cout << "distance(0,0,3,4) = "
                  << std::any_cast<double>(invoke_dyn(p, "distance",
                                                      0.0, 0.0, 3.0, 4.0))
                  << "\n";

        std::cout << "toString(1.5,2.5) = "
                  << std::any_cast<std::string>(invoke_dyn(p, "toString",
                                                           1.5, 2.5))
                  << "\n";

        std::cout << "mul(6,7) = "
                  << std::any_cast<int>(invoke_dyn(p, "mul", 6, 7))
                  << "\n";

        invoke_dyn(p, "hello");
    }
    catch (const std::exception& e) {
        std::cerr << "invoke error: " << e.what() << "\n";
    }
}
