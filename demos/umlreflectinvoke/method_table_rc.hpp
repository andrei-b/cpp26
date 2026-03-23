#pragma once

#include <any>
#include <functional>
#include <meta>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <QString>
#include <QByteArray>

// ============================================================
// Access
// ============================================================

enum class AccessSpecifier {
    Public,
    Protected,
    Private,
    Unknown
};

// ============================================================
// Reflected method entry
// ============================================================

struct MethodEntry {
    std::string pretty_name;
    bool is_static = false;
    bool is_virtual = false;
    bool is_const = false;
    bool is_noexcept = false;
    std::size_t argcount = 0;
    std::vector<std::string> arg_types;
    AccessSpecifier access = AccessSpecifier::Unknown;

    std::function<std::any(void*, std::span<const std::any>)> invoke;
    bool has_invoke = false;
};

// ============================================================
// Type name helper
// ============================================================

template <typename T>
consteval std::string_view type_name()
{
    return std::meta::display_string_of(^^T);
}

// ============================================================
// function_traits
// ============================================================

template <typename T>
struct function_traits;

// member
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)> {
    using class_type = C;
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr bool is_noexcept = false;
    static constexpr std::size_t arity = sizeof...(Args);
};

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const> {
    using class_type = C;
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr bool is_const = true;
    static constexpr bool is_noexcept = false;
    static constexpr std::size_t arity = sizeof...(Args);
};

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) noexcept> {
    using class_type = C;
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr bool is_noexcept = true;
    static constexpr std::size_t arity = sizeof...(Args);
};

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const noexcept> {
    using class_type = C;
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr bool is_const = true;
    static constexpr bool is_noexcept = true;
    static constexpr std::size_t arity = sizeof...(Args);
};

// free/static
template <typename R, typename... Args>
struct function_traits<R (*)(Args...)> {
    using class_type = void;
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr bool is_noexcept = false;
    static constexpr std::size_t arity = sizeof...(Args);
};

template <typename R, typename... Args>
struct function_traits<R (*)(Args...) noexcept> {
    using class_type = void;
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr bool is_noexcept = true;
    static constexpr std::size_t arity = sizeof...(Args);
};

// ============================================================
// Safe invoker whitelist
// ============================================================

template <typename T>
struct is_safe_invoker_type : std::false_type {};

template <> struct is_safe_invoker_type<void> : std::true_type {};
template <> struct is_safe_invoker_type<bool> : std::true_type {};
template <> struct is_safe_invoker_type<char> : std::true_type {};
template <> struct is_safe_invoker_type<signed char> : std::true_type {};
template <> struct is_safe_invoker_type<unsigned char> : std::true_type {};
template <> struct is_safe_invoker_type<short> : std::true_type {};
template <> struct is_safe_invoker_type<unsigned short> : std::true_type {};
template <> struct is_safe_invoker_type<int> : std::true_type {};
template <> struct is_safe_invoker_type<unsigned int> : std::true_type {};
template <> struct is_safe_invoker_type<long> : std::true_type {};
template <> struct is_safe_invoker_type<unsigned long> : std::true_type {};
template <> struct is_safe_invoker_type<long long> : std::true_type {};
template <> struct is_safe_invoker_type<unsigned long long> : std::true_type {};
template <> struct is_safe_invoker_type<float> : std::true_type {};
template <> struct is_safe_invoker_type<double> : std::true_type {};
template <> struct is_safe_invoker_type<long double> : std::true_type {};
template <> struct is_safe_invoker_type<QString> : std::true_type {};
template <> struct is_safe_invoker_type<QByteArray> : std::true_type {};
template <> struct is_safe_invoker_type<std::string> : std::true_type {};
template <> struct is_safe_invoker_type<std::string_view> : std::true_type {};

template <typename T>
    requires std::is_enum_v<std::remove_cvref_t<T>>
struct is_safe_invoker_type<T> : std::true_type {};

template <typename T>
inline constexpr bool is_safe_invoker_type_v =
    is_safe_invoker_type<std::remove_cvref_t<T>>::value;

template <typename T>
inline constexpr bool is_safe_invoker_arg_v =
    !std::is_reference_v<T> &&
    (std::is_pointer_v<std::remove_cvref_t<T>> || is_safe_invoker_type_v<T>);

template <typename T>
inline constexpr bool is_safe_invoker_return_v =
    std::is_void_v<std::remove_cvref_t<T>> ||
    (!std::is_reference_v<T> &&
     (std::is_pointer_v<std::remove_cvref_t<T>> || is_safe_invoker_type_v<T>));

// ============================================================
// Argument type names
// ============================================================

template <typename Tuple, std::size_t... I>
std::vector<std::string> arg_type_names_impl(std::index_sequence<I...>)
{
    return { std::string(type_name<std::tuple_element_t<I, Tuple>>())... };
}

template <typename Tuple>
std::vector<std::string> arg_type_names()
{
    return arg_type_names_impl<Tuple>(
        std::make_index_sequence<std::tuple_size_v<Tuple>>{}
    );
}

// ============================================================
// Safe tuple check
// ============================================================

template <typename Tuple, std::size_t... I>
consteval bool tuple_args_safe_impl(std::index_sequence<I...>)
{
    return (is_safe_invoker_arg_v<std::tuple_element_t<I, Tuple>> && ...);
}

template <typename Tuple>
consteval bool tuple_args_safe()
{
    return tuple_args_safe_impl<Tuple>(
        std::make_index_sequence<std::tuple_size_v<Tuple>>{}
    );
}

// ============================================================
// Invoker eligibility
// ============================================================

template <typename PMF, typename = void>
struct can_build_invoker : std::false_type {};

template <typename PMF>
struct can_build_invoker<PMF, std::void_t<typename function_traits<PMF>::args_tuple>>
{
private:
    using traits = function_traits<PMF>;
    using R = typename traits::return_type;
    using ArgsTuple = typename traits::args_tuple;

public:
    static constexpr bool value =
        is_safe_invoker_return_v<R> &&
        tuple_args_safe<ArgsTuple>();
};

template <typename PMF>
inline constexpr bool can_build_invoker_v = can_build_invoker<PMF>::value;

// ============================================================
// any_cast helper
// ============================================================

template <typename T>
decltype(auto) any_cast_value(const std::any& a)
{
    static_assert(!std::is_reference_v<T>,
                  "reference arguments are not supported by dynamic invoker");

    using U = std::remove_cvref_t<T>;
    return std::any_cast<U>(a);
}

// ============================================================
// Invocation helpers
// ============================================================

template <class C, class PMF, class Tuple, std::size_t... I>
std::any invoke_member_with_anys_impl(
    C& obj,
    PMF pmf,
    std::span<const std::any> args,
    std::index_sequence<I...>)
{
    using traits = function_traits<PMF>;
    using R = typename traits::return_type;

    if constexpr (std::is_void_v<R>) {
        (obj.*pmf)(any_cast_value<std::tuple_element_t<I, Tuple>>(args[I])...);
        return std::any{};
    } else {
        using StoredR = std::remove_cvref_t<R>;
        return std::any(
            StoredR((obj.*pmf)(any_cast_value<std::tuple_element_t<I, Tuple>>(args[I])...))
        );
    }
}

template <class C, class PMF>
std::any invoke_member_with_anys(C& obj, PMF pmf, std::span<const std::any> args)
{
    using traits = function_traits<PMF>;
    using Tuple = typename traits::args_tuple;
    constexpr std::size_t N = std::tuple_size_v<Tuple>;

    if (args.size() != N) {
        throw std::runtime_error("argument count mismatch");
    }

    return invoke_member_with_anys_impl<C, PMF, Tuple>(
        obj, pmf, args, std::make_index_sequence<N>{});
}

template <class F, class Tuple, std::size_t... I>
std::any invoke_free_with_anys_impl(
    F f,
    std::span<const std::any> args,
    std::index_sequence<I...>)
{
    using traits = function_traits<F>;
    using R = typename traits::return_type;

    if constexpr (std::is_void_v<R>) {
        f(any_cast_value<std::tuple_element_t<I, Tuple>>(args[I])...);
        return std::any{};
    } else {
        using StoredR = std::remove_cvref_t<R>;
        return std::any(
            StoredR(f(any_cast_value<std::tuple_element_t<I, Tuple>>(args[I])...))
        );
    }
}

template <class F>
std::any invoke_free_with_anys(F f, std::span<const std::any> args)
{
    using traits = function_traits<F>;
    using Tuple = typename traits::args_tuple;
    constexpr std::size_t N = std::tuple_size_v<Tuple>;

    if (args.size() != N) {
        throw std::runtime_error("argument count mismatch");
    }

    return invoke_free_with_anys_impl<F, Tuple>(
        f, args, std::make_index_sequence<N>{});
}

// ============================================================
// Metadata builder
// ============================================================

template <class PMF>
MethodEntry make_method_metadata()
{
    using traits = function_traits<PMF>;

    MethodEntry entry;
    entry.is_const = traits::is_const;
    entry.is_noexcept = traits::is_noexcept;
    entry.argcount = traits::arity;
    entry.arg_types = arg_type_names<typename traits::args_tuple>();
    return entry;
}

// ============================================================
// Optional invoker attachment
// ============================================================

template <class C, class PMF>
void try_attach_invoker(MethodEntry& entry, PMF pmf)
{
    if constexpr (can_build_invoker_v<PMF>) {
        entry.invoke = [pmf](void* obj, std::span<const std::any> args) -> std::any {
            if constexpr (std::is_member_function_pointer_v<PMF>) {
                if (!obj) {
                    throw std::runtime_error("null object for member invocation");
                }
                return invoke_member_with_anys(*static_cast<C*>(obj), pmf, args);
            } else {
                return invoke_free_with_anys(pmf, args);
            }
        };
        entry.has_invoke = true;
    } else {
        entry.invoke = {};
        entry.has_invoke = false;
    }
}


// ============================================================
// Reflection core
// ============================================================

template <auto Ctx, class C>
auto get_method_table_impl()
{
    std::unordered_multimap<std::string_view, MethodEntry> table;

    template for (constexpr auto m : std::define_static_array(std::meta::members_of(^^C, Ctx))) {
        if constexpr (std::meta::is_function(m) && std::meta::has_identifier(m)) {
            auto ptr = &[:m:];
            using PMF = decltype(ptr);

            if constexpr (requires { typename function_traits<PMF>::args_tuple; }) {
                MethodEntry entry = make_method_metadata<PMF>();

                entry.pretty_name = std::string(std::meta::display_string_of(m));
                entry.is_static = std::meta::is_static_member(m);
                entry.is_virtual = !entry.is_static && std::meta::is_virtual(m);

                if constexpr (std::meta::is_public(m)) {
                    entry.access = AccessSpecifier::Public;
                } else if constexpr (std::meta::is_protected(m)) {
                    entry.access = AccessSpecifier::Protected;
                } else if constexpr (std::meta::is_private(m)) {
                    entry.access = AccessSpecifier::Private;
                } else {
                    entry.access = AccessSpecifier::Unknown;
                }

                try_attach_invoker<C>(entry, ptr);
                table.emplace(std::meta::identifier_of(m), std::move(entry));
            }
        }
    }

    return table;
}

// ============================================================
// Public view
// ============================================================

template <class C>
auto get_method_table()
{
    constexpr auto ctx = std::meta::access_context::current();
    return get_method_table_impl<ctx, C>();
}

// ============================================================
// Derived view: public + protected
// ============================================================

template <class C>
struct DerivedAccessReflector : C {
    static auto method_table()
    {
        constexpr auto ctx = std::meta::access_context::current();
        return get_method_table_impl<ctx, C>();
    }
};

template <class C>
auto get_method_table_from_derived()
{
    return DerivedAccessReflector<C>::method_table();
}

// ============================================================
// Typed invokers
// ============================================================

template <class C, typename R, typename... Args>
std::function<R(Args...)> make_invoker(C* obj, std::string_view method_name)
{
    const auto table = get_method_table<C>();
    const auto wanted_arg_types = arg_type_names<std::tuple<Args...>>();

    auto range = table.equal_range(method_name);
    for (auto it = range.first; it != range.second; ++it) {
        const auto& entry = it->second;

        if (!entry.has_invoke) continue;
        if (entry.is_static) continue;
        if (entry.argcount != sizeof...(Args)) continue;
        if (entry.arg_types != wanted_arg_types) continue;

        return [obj, entry](Args... args) -> R {
            if (!obj) {
                throw std::runtime_error("null object passed to make_invoker");
            }

            std::vector<std::any> packed;
            packed.reserve(sizeof...(Args));
            (packed.emplace_back(std::forward<Args>(args)), ...);

            std::any result = entry.invoke(
                obj, std::span<const std::any>(packed.data(), packed.size()));

            if constexpr (std::is_void_v<R>) {
                return;
            } else {
                using StoredR = std::remove_cvref_t<R>;
                auto p = std::any_cast<StoredR>(&result);
                if (!p) throw std::bad_any_cast{};
                return static_cast<R>(*p);
            }
        };
    }

    throw std::runtime_error("no matching callable reflected method");
}

template <class C, typename R, typename... Args>
std::function<R(Args...)> make_invoker_from_derived(C* obj, std::string_view method_name)
{
    const auto table = get_method_table_from_derived<C>();
    const auto wanted_arg_types = arg_type_names<std::tuple<Args...>>();

    auto range = table.equal_range(method_name);
    for (auto it = range.first; it != range.second; ++it) {
        const auto& entry = it->second;

        if (!entry.has_invoke) continue;
        if (entry.is_static) continue;
        if (entry.argcount != sizeof...(Args)) continue;
        if (entry.arg_types != wanted_arg_types) continue;

        return [obj, entry](Args... args) -> R {
            if (!obj) {
                throw std::runtime_error("null object passed to make_invoker_from_derived");
            }

            std::vector<std::any> packed;
            packed.reserve(sizeof...(Args));
            (packed.emplace_back(std::forward<Args>(args)), ...);

            std::any result = entry.invoke(
                obj, std::span<const std::any>(packed.data(), packed.size()));

            if constexpr (std::is_void_v<R>) {
                return;
            } else {
                using StoredR = std::remove_cvref_t<R>;
                auto p = std::any_cast<StoredR>(&result);
                if (!p) throw std::bad_any_cast{};
                return static_cast<R>(*p);
            }
        };
    }

    throw std::runtime_error("no matching callable reflected method");
}

template <class C, typename R, typename... Args>
std::function<R(Args...)> make_static_invoker(std::string_view method_name)
{
    const auto table = get_method_table<C>();
    const auto wanted_arg_types = arg_type_names<std::tuple<Args...>>();

    auto range = table.equal_range(method_name);
    for (auto it = range.first; it != range.second; ++it) {
        const auto& entry = it->second;

        if (!entry.has_invoke) continue;
        if (!entry.is_static) continue;
        if (entry.argcount != sizeof...(Args)) continue;
        if (entry.arg_types != wanted_arg_types) continue;

        return [entry](Args... args) -> R {
            std::vector<std::any> packed;
            packed.reserve(sizeof...(Args));
            (packed.emplace_back(std::forward<Args>(args)), ...);

            std::any result = entry.invoke(
                nullptr, std::span<const std::any>(packed.data(), packed.size()));

            if constexpr (std::is_void_v<R>) {
                return;
            } else {
                using StoredR = std::remove_cvref_t<R>;
                auto p = std::any_cast<StoredR>(&result);
                if (!p) throw std::bad_any_cast{};
                return static_cast<R>(*p);
            }
        };
    }

    throw std::runtime_error("no matching callable reflected static method");
}