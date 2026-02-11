#include <iostream>
#include <meta>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
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
template <std::size_t N>
struct fixed_string {
    char8_t value[N];
    consteval fixed_string(const char (&s)[N]) {
        for (std::size_t i = 0; i < N; ++i) value[i] = static_cast<char8_t>(s[i]);
    }
    consteval fixed_string(const char8_t (&s)[N]) {
        for (std::size_t i = 0; i < N; ++i) value[i] = s[i];
    }
    constexpr std::u8string_view view() const noexcept {
        return std::u8string_view(value, N - 1); // drop terminating '\0'
      }
    constexpr bool operator==(fixed_string const&) const = default;
};

template <std::size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N>;

template <typename T> consteval std::meta::info find_member_function(std::u8string_view wanted) {
    for (std::meta::info m : std::meta::members_of(^^T, std::meta::access_context::current())) { // GCC branch uses ^^
        if (std::meta::is_function(m) && std::meta::u8identifier_of(m) == wanted) {
            return m;
        }
    } // You can also std::terminate or throw; error handling varies by implementation
    throw "member function not found";
}


template <fixed_string Name, class T, class... Args>
decltype(auto) call_reflected(T&& obj, Args&&... args) {
    using U = std::remove_reference_t<T>;
    constexpr std::meta::info mem = find_member_function<U>(Name.view()); // splice the member function name into the call:
    return std::forward<T>(obj).[:mem:](std::forward<Args>(args)...);
}

template <fixed_string Name>
struct MethodThunk {
    static int call_i(Person& obj, int x) {
        // calls Person::test_call(int) etc
        return call_reflected<Name>(obj, x);
    }
};

struct MethodEntry_i {
    std::u8string_view name;
    int (*call)(Person&, int);
};

template <fixed_string... Names>
consteval auto make_method_table_i() {
    return std::array<MethodEntry_i, sizeof...(Names)>{
        MethodEntry_i{ Names.view(), &MethodThunk<Names>::call_i }...
    };
}

// --- Example: register methods with signature int(Person&, int) ---
static constexpr auto person_methods_i =
    make_method_table_i<"test_call">(); // add more: <"test_call","other">

const MethodEntry_i* find_method_i(std::u8string_view name) {
    for (auto const& e : person_methods_i)
        if (e.name == name) return &e;
    return nullptr;
}

int main() {
    Person p{};
    if (auto e = find_method_i(u8"test_call")) {
        std::cout << "-> " << e->call(p, 42) << "\n";
    }
}