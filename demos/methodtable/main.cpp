#include <iostream>
#include <meta>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <variant>

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

using Value = std::variant<std::monostate, bool, int, long long, double, std::string>;

inline const char* value_type_name(const Value& v) {
    switch (v.index()) {
        case 0: return "void";
        case 1: return "bool";
        case 2: return "int";
        case 3: return "long long";
        case 4: return "double";
        case 5: return "string";
        default: return "?";
    }
}

template <class T>
std::optional<T> value_as(const Value& v) {
    if (auto p = std::get_if<T>(&v)) return *p;
    return std::nullopt;
}

template <class T>
Value to_value(T&& x) {
    using U = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<U, void>) {
        return Value{std::monostate{}};
    } else if constexpr (std::is_same_v<U, bool>) {
        return Value{static_cast<bool>(x)};
    } else if constexpr (std::is_same_v<U, int>) {
        return Value{static_cast<int>(x)};
    } else if constexpr (std::is_same_v<U, long long>) {
        return Value{static_cast<long long>(x)};
    } else if constexpr (std::is_same_v<U, double>) {
        return Value{static_cast<double>(x)};
    } else if constexpr (std::is_same_v<U, std::string>) {
        return Value{std::forward<T>(x)};
    } else if constexpr (std::is_same_v<U, const char*>) {
        return Value{std::string(x)};
    } else {
        static_assert(!sizeof(U), "Type not supported in Value/to_value()");
    }
}



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

struct InvokeResult {
    Value value;          // monostate => void
    const char* error;    // nullptr if ok
};

template <fixed_string Name, class Obj, class R, class... Args>
struct VariadicInvoker {
    static InvokeResult invoke(void* obj, std::span<const Value> argv) {
        if (argv.size() != sizeof...(Args))
            return {Value{std::monostate{}}, "arity mismatch"};

        Obj& o = *static_cast<Obj*>(obj);
        return invoke_impl(o, argv, std::index_sequence_for<Args...>{});
    }
    template <typename... A>
        static InvokeResult invoke_2(Obj& o, A&&... a) {
            return invoke_impl(o, std::span<const Value>{to_value(std::forward<A>(a))...}, std::index_sequence_for<Args...>{});
        }


private:
    template <std::size_t... I>
    static InvokeResult invoke_impl(Obj& o,
                                    std::span<const Value> argv,
                                    std::index_sequence<I...>) {

        // Build a tuple<optional<Arg0>, optional<Arg1>, ...>
        auto conv = std::tuple<std::optional<std::remove_cvref_t<Args>>...>{
            value_as<std::remove_cvref_t<Args>>(argv[I])...
        };

        // Check all converted OK
        bool ok = ((std::get<I>(conv).has_value()) && ...);
        if (!ok) return {Value{std::monostate{}}, "type mismatch"};

        // Call
        if constexpr (std::is_void_v<R>) {
            call_reflected<Name>(o, (*std::get<I>(conv))...);
            return {Value{std::monostate{}}, nullptr};
        } else {
            R r = call_reflected<Name>(o, (*std::get<I>(conv))...);
            return {to_value(r), nullptr};
        }
    }
};

template <std::size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N>;

template <typename T> consteval std::meta::info find_member_function(std::u8string_view wanted) {
    for (std::meta::info m : std::meta::members_of(^^T, std::meta::access_context::current())) { // GCC branch uses ^^
        if (std::meta::is_function(m) && std::meta::u8identifier_of(m) == wanted) {
            return m;
        }
    }
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

struct MethodEntry {
    std::u8string_view name;
    InvokeResult (*invoke)(void* obj, std::span<const Value> args);

    template <typename... Args>
    static InvokeResult (*invoke_2)(Person person, Args... args);
};

// Register methods with their *real* signatures:
template <fixed_string Name, class R, class... Args>
consteval MethodEntry entry_for() {
    return MethodEntry{
        Name.view(),
        &VariadicInvoker<Name, Person, R, Args...>::invoke
    };
}

// Build a constexpr table (explicit list)
static constexpr auto person_methods = std::array{
    entry_for<"test_call", int, int>(),
    entry_for<"add", int, int, int>(),
    // entry_for<"something", void, double, std::string>(),
};

inline const MethodEntry* find_method(std::u8string_view name) {
    for (auto const& e : person_methods)
        if (e.name == name) return &e;
    return nullptr;
}

int main() {
    Person p{};

    if (auto m = find_method(u8"test_call")) {
        std::vector<Value> args = { 42 };
        auto r = m->invoke(&p, args);
        if (r.error) std::cout << "error: " << r.error << "\n";
        else std::cout << "returned type=" << value_type_name(r.value)
                       << " value=" << std::get<int>(r.value) << "\n";
    }

    if (auto m = find_method(u8"add")) {
        std::vector<Value> args = { 1, 2 };
        auto r = m->invoke(&p, args);
        if (!r.error) std::cout << "add -> " << std::get<int>(r.value) << "\n";
        auto r2 = m->invoke_2<int, int>(p, 3, 4);
    }
}