#include <iostream>
#include <meta>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

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

static void print_u8(std::u8string_view s) {
    std::cout.write(reinterpret_cast<const char*>(s.data()),
                    static_cast<std::streamsize>(s.size()));
}

enum MemberFlags {
    MF_CONST       = 1u << 0,
    MF_VOLATILE    = 1u << 1,
    MF_POINTER     = 1u << 2,
    MF_LREF        = 1u << 3,
    MF_RREF        = 1u << 4,
    MF_FUNDAMENTAL = 1u << 5,
    MF_CLASS       = 1u << 6,
    MF_ENUM        = 1u << 7,
    MF_FUNCTION    = 1u << 8,
    MF_CONSTRUCTOR = 1u << 9,
    MF_DESTRUCTOR  = 1u << 10,
    MF_STATIC = 1u << 11,
};

struct MemberDesc {
    std::u8string_view name;          // identifier
    std::u8string_view type_display;  // printable type string (works for int/double/etc)
    std::uint32_t flags;
    std::u8string_view params[10]; // for functions: parameter type display strings
    std::u8string_view return_type;
};

consteval std::u8string_view safe_member_name(std::meta::info m) {
    // members typically have identifiers, but keep it safe
    return has_identifier(m) ? u8identifier_of(m) : u8"<no-id>";
}

consteval std::u8string_view safe_type_string(std::meta::info t) {
    // works for ANY reflection, including fundamental types
    return u8display_string_of(t);
}

template <typename T>
struct ReflectedMembers {
    static consteval auto make() {
        auto mems = members_of(^^T, std::meta::access_context::current());

        std::array<MemberDesc, 64> out{};
        std::size_t n = mems.size();

        for (std::size_t i = 0; i < n && i < out.size(); ++i) {
            auto m = mems[i];
            if (is_function(m)) {
                std::uint32_t f = 0;
                f = MF_FUNCTION;
                if (std::meta::is_static_member(m))
                    f |= MF_STATIC;
                auto params = parameters_of(m);
                if (is_constructor(m)) f |= MF_CONSTRUCTOR;
                else if (is_destructor(m)) f |= MF_DESTRUCTOR;
                out[i] = MemberDesc{
                    safe_member_name(m),
                    u8display_string_of(m),
                    f
                };
                for (std::size_t j = 0; j < params.size() && j < std::size(out[i].params); ++j) {
                    out[i].params[j] = safe_type_string(type_of(params[j]));
                }
                auto has_return_type = !(out[i].flags & MF_CONSTRUCTOR) && !(out[i].flags & MF_DESTRUCTOR);
                if (has_return_type) {
                    out[i].return_type = safe_type_string(return_type_of(m));
                }
            } else {
                auto t = type_of(m);

                std::uint32_t f = 0;
                if (is_const(t))    f |= MF_CONST;
                if (is_volatile(t)) f |= MF_VOLATILE;

                if (is_pointer_type(t))          f |= MF_POINTER;
                if (is_lvalue_reference_type(t)) f |= MF_LREF;
                if (is_rvalue_reference_type(t)) f |= MF_RREF;

                auto base = remove_cvref(t);
                if (is_fundamental_type(base)) f |= MF_FUNDAMENTAL;
                else if (is_class_type(base))  f |= MF_CLASS;
                else if (is_enum_type(base))   f |= MF_ENUM;

                out[i] = MemberDesc{
                    safe_member_name(m),
                    safe_type_string(t), // NOTE: use t (keeps const/&/* in display string if implementation prints them)
                    f
                };
            }
        }
        return std::pair{out, n};
    }

    static constexpr auto built = make();
    static constexpr auto descs = built.first;
    static constexpr std::size_t count = built.second;
};

static void print_flags(std::uint32_t f) {
    bool first = true;
    auto add = [&](const char* s){
        if (!first) std::cout << ", ";
        std::cout << s;
        first = false;
    };

    if (f & MF_CONST)    add("const");
    if (f & MF_VOLATILE) add("volatile");
    if (f & MF_POINTER)  add("ptr");
    if (f & MF_LREF)     add("lref");
    if (f & MF_RREF)     add("rref");

    if (f & MF_FUNDAMENTAL) add("fundamental");
    else if (f & MF_CLASS)  add("class");
    else if (f & MF_ENUM)   add("enum");
    if (f & MF_FUNCTION)    add("function");
    if (f & MF_CONSTRUCTOR) add ("constructor");
    else if (f & MF_DESTRUCTOR) add ("destructor");
    if (f & MF_STATIC) add ("static");
}

template <typename T>
consteval std::meta::info find_member_function(std::u8string_view wanted) {
    for (std::meta::info m : std::meta::members_of(^^T, std::meta::access_context::current())) {               // GCC branch uses ^^
        if (std::meta::is_function(m) && std::meta::u8identifier_of(m) == wanted) {
            return m;
        }
    }
    // You can also std::terminate or throw; error handling varies by implementation
    throw "member function not found";
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


template <std::size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N>;

template <fixed_string Name, class T, class... Args>
decltype(auto) call_reflected(T&& obj, Args&&... args) {
    using U = std::remove_reference_t<T>;
    constexpr std::meta::info mem = find_member_function<U>(Name.view());

    // splice the member function name into the call:
    return std::forward<T>(obj).[:mem:](std::forward<Args>(args)...);
}


int main() {
    std::cout << "Person members:\n";
    for (std::size_t i = 0; i < ReflectedMembers<Person>::count
                         && i < ReflectedMembers<Person>::descs.size(); ++i) {
        const auto& d = ReflectedMembers<Person>::descs[i];

        std::cout << "  - ";
        print_u8(d.name);
        std::cout << " : ";
        print_u8(d.type_display);
        std::cout << "  [";
        print_flags(d.flags);
        std::cout << "]";
        if (d.flags & MF_FUNCTION) {
            std::cout << "  (";
            bool first = true;
            for (const auto& p : d.params) {
                if (p.empty()) break;
                if (!first) std::cout << ", ";
                print_u8(p);
                first = false;
            }
            std::cout << ")";
            if (!d.return_type.empty()) {
                std::cout << " -> ";
                print_u8(d.return_type);
            }

        }
        std::cout << "\n";
    }
    Person p{};
    std::cout << "function returns " << call_reflected<"test_call">(p, 42);
}
