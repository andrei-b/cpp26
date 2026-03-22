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

enum class AccessSpecifier {
    Private,
    Protected,
    Public
};

struct MethodEntry {
    std::string_view name;
    std::function<std::any(void*, std::span<const std::any>)> invoke;
    bool is_static = false;
    bool is_const = false;
    bool is_virtual = false;
    AccessSpecifier access = AccessSpecifier::Public;
    std::string_view pretty_name;
    size_t argcount = 0;
    std::vector<std::string> arg_types;
};

// Function traits for member/static function pointers.
template <typename T>
struct function_traits;

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)> {
    using class_type = C;
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr size_t arity = sizeof...(Args);
};

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const> {
    using class_type = C;
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr bool is_const = true;
    static constexpr size_t arity = sizeof...(Args);
};

template <typename R, typename... Args>
struct function_traits<R (*)(Args...)> {
    using class_type = void;
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr size_t arity = sizeof...(Args);
};

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
        return std::any((obj.*pmf)(any_cast_value<std::tuple_element_t<I, Tuple>>(args[I])...));
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

    return invoke_member_with_anys_impl<C, PMF, Tuple>(obj, pmf, args, std::make_index_sequence<N>{});
}

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
        return std::any(fn(any_cast_value<std::tuple_element_t<I, Tuple>>(args[I])...));
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

    return invoke_static_with_anys_impl<FN, Tuple>(fn, args, std::make_index_sequence<N>{});
}

template <class C, class PMF>
    requires std::is_member_function_pointer_v<PMF>
MethodEntry make_member_entry(std::string_view name, PMF pmf) {
    MethodEntry entry{};
    entry.name = name;
    entry.invoke = [pmf](void* obj, std::span<const std::any> args) -> std::any {
        return invoke_member_with_anys(*static_cast<C*>(obj), pmf, args);
    };
    return entry;
}

template <class C, class FN>
    requires (!std::is_member_function_pointer_v<FN>)
MethodEntry make_static_entry(std::string_view name, FN fn) {
    MethodEntry entry{};
    entry.name = name;
    entry.invoke = [fn]([[maybe_unused]] void* obj, std::span<const std::any> args) -> std::any {
        return invoke_static_with_anys(fn, args);
    };
    return entry;
}

template <typename T>
std::string_view type_name() {
#if defined(__clang__) || defined(__GNUC__)
    std::string_view name = __PRETTY_FUNCTION__;
    auto start = name.find("T = ") + 4;
    auto end = name.find(']', start);
    return name.substr(start, end - start);
#elif defined(_MSC_VER)
    std::string_view name = __FUNCSIG__;
    auto start = name.find("type_name<") + 10;
    auto end = name.find(">(void)");
    return name.substr(start, end - start);
#endif
}

template <class Tuple, std::size_t... I>
std::vector<std::string> arg_type_names_impl(std::index_sequence<I...>) {
    std::vector<std::string> result;
    result.reserve(sizeof...(I));
    (result.emplace_back(type_name<std::tuple_element_t<I, Tuple>>()), ...);
    return result;
}

template <class Tuple>
std::vector<std::string> arg_type_names() {
    return arg_type_names_impl<Tuple>(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

template <class C>
auto get_method_table() {
    constexpr auto ctx = std::meta::access_context::current();

    std::unordered_multimap<std::string_view, MethodEntry> table;

    template for (constexpr auto m : std::define_static_array(std::meta::members_of(^^C, ctx))) {
        if constexpr (std::meta::is_function(m) && std::meta::has_identifier(m)) {
            auto pmf = &[:m:];
            using PMF = decltype(pmf);
            using traits = function_traits<PMF>;
            constexpr auto argcount = traits::arity;
            auto return_type = type_name<PMF>();

            if constexpr (std::meta::is_static_member(m)) {
                MethodEntry entry = make_static_entry<C, PMF>(std::meta::identifier_of(m), pmf);
                entry.is_static = true;
                entry.is_virtual = false;
                entry.is_const = false;
                entry.argcount = argcount;
                entry.pretty_name = return_type;
                entry.arg_types = arg_type_names<typename traits::args_tuple>();
                entry.access = std::meta::is_private(m) ? AccessSpecifier::Private :
                               std::meta::is_protected(m) ? AccessSpecifier::Protected :
                               AccessSpecifier::Public;
                table.insert({entry.name, entry});
            } else {
                MethodEntry entry = make_member_entry<C, PMF>(std::meta::identifier_of(m), pmf);
                entry.is_static = false;
                entry.is_virtual = std::meta::is_virtual(m);
                entry.is_const = traits::is_const;
                entry.argcount = argcount;
                entry.pretty_name = return_type;
                entry.access = std::meta::is_private(m) ? AccessSpecifier::Private :
                               std::meta::is_protected(m) ? AccessSpecifier::Protected :
                               AccessSpecifier::Public;
                table.insert({entry.name, entry});
                entry.arg_types = arg_type_names<typename traits::args_tuple>();
                table.insert({entry.name, entry});
            }
        }
    }

    return table;
}

