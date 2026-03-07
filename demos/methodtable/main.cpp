#include <iostream>
#include <meta>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

struct TestClass {
    int intval;
    const double height;
    int add(int a, int b) { return a + b; }
    static  int foo() { return 42; }
    int test_call(int i) {
        std::cout << "test_call called with " << i << "\n";
        return i;
    }
};

struct MethodEntry {
    std::string_view name;
    bool is_static;
};

using MethodTable = std::vector<MethodEntry>;

template <typename T>
consteval auto reflected_members()
{
    constexpr auto ctx = std::meta::access_context::current();
    return std::define_static_array(std::meta::members_of(^^std::remove_cvref_t<T>, ctx));
}

template <typename T>
auto build_table(T&)
{
    MethodTable result;
    template for (constexpr auto m : reflected_members<T>())
    {
        if constexpr (std::meta::is_function(m) && std::meta::has_identifier(m))
        {
            if (std::string_view name = std::meta::identifier_of(m); !name.empty())
            {
                result.push_back(MethodEntry{name, std::meta::is_static_member(m)});
            }
        }
    }
    return result;
}


template <class C>
void* get_address(C *, std::string_view method_name) {
    template for (constexpr auto m : reflected_members<C>())
    {
        if constexpr (std::meta::is_function(m) && std::meta::has_identifier(m))
        {
            if (method_name == std::meta::identifier_of(m))
            {
                if constexpr (std::meta::is_static_member(m))
                {
                    auto fn = &[:m:];
                    return reinterpret_cast<void*>(fn);
                }
                else
                {
                    auto pmf = &[:m:];
                    union {
                        decltype(pmf) p;
                        void* v;
                    } u;
                    u.p = pmf;
                    return u.v;
                }
            }
        }
    }
    return nullptr;
}

template <class C, typename R, typename... Args>
std::function<R(Args... )> make_invoker(C * c, std::string_view method_name) {
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

template <typename T, typename... Args>
decltype(auto) call_method(T&& obj, std::string_view method_name, Args&&... args) {
    template for (constexpr auto m : reflected_members<T>())
    {
        if constexpr (std::meta::is_function(m) && std::meta::has_identifier(m))
        {
            if (method_name == std::meta::identifier_of(m))
            {
                if constexpr (std::meta::is_static_member(m))
                {
                    auto fn = &[:m:];
                    if constexpr (requires { fn(std::forward<Args>(args)...); }) {
                        return fn(std::forward<Args>(args)...);
                    }
                }
                else
                {
                    auto pmf = &[:m:];
                    if constexpr (requires { (std::forward<T>(obj).*pmf)(std::forward<Args>(args)...); }) {
                        return (std::forward<T>(obj).*pmf)(std::forward<Args>(args)...);
                    }
                }
            }
        }
    }

    throw std::runtime_error("method not found or argument mismatch");
}

template <class C, typename Sig>
struct function_traits;

template <class C, typename R, typename... Args>
struct function_traits<C, R(Args...)> {
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
};

template <class C, typename Sig>
auto make_invoker_sig(C* c, std::string_view method_name) {
    using traits = function_traits<C, Sig>;
    using R = typename traits::return_type;

    if constexpr (std::tuple_size_v<typename traits::args_tuple> == 2) {
        using Arg1 = std::tuple_element_t<0, typename traits::args_tuple>;
        using Arg2 = std::tuple_element_t<1, typename traits::args_tuple>;
        return make_invoker<C, R, Arg1, Arg2>(c, method_name);
    }
    throw std::runtime_error("Unsupported signature");
}

// ...existing code...

int main() {
    TestClass p{};
    auto table = build_table(p);
    for (const auto& entry : table) {
        std::cout << "Method: " << entry.name

                  << ", Static: " << std::boolalpha << entry.is_static
                  << "\n";
    }
    int i = call_method(p, "add", 123, 456);
    auto m = "test_call";
    int c = call_method(p, m, 789);
    std::cout << "Result of add: " << i << "\n";

    // Using make_invoker with explicit signature int(int, int)
    auto addfn = make_invoker_sig<TestClass, int(int, int)>(&p, "add");
    std::cout << "Result of addfn: " << addfn(10, 20) << "\n";

    auto addr = get_address(&p, "add");
    std::cout << "Address of add: " << addr << "\n";
}