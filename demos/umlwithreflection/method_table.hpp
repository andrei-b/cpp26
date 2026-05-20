#pragma once

#include <meta>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <type_traits>

enum class AccessSpecifier {
    Public,
    Protected,
    Private,
    Unknown
};

struct MethodEntry {
    std::string pretty_name;
    bool is_static = false;
    bool is_virtual = false;
    bool is_const = false;
    bool is_noexcept = false;
    std::size_t argcount = 0;
    std::vector<std::string> arg_types;
    AccessSpecifier access = AccessSpecifier::Unknown;
    std::string return_type;
};

template <typename T>
consteval std::string_view type_name()
{
    return std::meta::display_string_of(^^T);
}

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

template <class C>
auto get_method_table()
{
    constexpr auto ctx = std::meta::access_context::current();
    std::unordered_multimap<std::string_view, MethodEntry> table;

    template for (constexpr auto m : std::define_static_array(std::meta::members_of(^^C, ctx))) {
        if constexpr (std::meta::is_function(m) && std::meta::has_identifier(m)) {
            auto ptr = &[:m:];
            using PMF = decltype(ptr);

            if constexpr (requires { typename function_traits<PMF>::args_tuple; }) {
                using traits = function_traits<PMF>;

                MethodEntry entry;
                entry.pretty_name = std::string(std::meta::display_string_of(m));
                entry.is_static = std::meta::is_static_member(m);
                entry.is_virtual = !entry.is_static && std::meta::is_virtual(m);
                entry.is_const = std::meta::is_const(m);
                entry.is_noexcept = traits::is_noexcept;
                entry.return_type = std::meta::display_string_of(std::meta::return_type_of(m));
                entry.argcount = []() consteval -> std::size_t {
                    return std::meta::parameters_of(m).size();
                }();
                entry.arg_types = arg_type_names<typename traits::args_tuple>();

                if constexpr (std::meta::is_public(m)) {
                    entry.access = AccessSpecifier::Public;
                } else if constexpr (std::meta::is_protected(m)) {
                    entry.access = AccessSpecifier::Protected;
                } else if constexpr (std::meta::is_private(m)) {
                    entry.access = AccessSpecifier::Private;
                } else {
                    entry.access = AccessSpecifier::Unknown;
                }

                table.emplace(std::meta::identifier_of(m), std::move(entry));
            }
        }
    }

    return table;
}