#include <iostream>
#include <meta>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

struct Person {
    int age;
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

int main() {
    Person p{};
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
}