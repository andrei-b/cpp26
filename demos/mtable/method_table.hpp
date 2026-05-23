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

struct MethodArgument {
    std::string name;
    std::string type;
    std::string bare_type;

    bool is_const = false;
    bool is_volatile = false;

    bool is_lvalue_reference = false;
    bool is_rvalue_reference = false;

    bool is_pointer = false;
    bool is_pointer_to_const = false;
    bool is_const_pointer = false;

    bool is_array = false;

    bool is_signed = false;
    bool is_unsigned = false;
    bool is_integral = false;
    bool is_floating_point = false;

    bool is_enum = false;
    bool is_class = false;
};

struct MethodEntry {
    std::string_view name;
    std::function<std::any(void*, std::span<const std::any>)> invoke;
    bool is_static = false;
    bool is_const = false;
    bool is_virtual = false;
    std::string_view pretty_name;
    size_t argcount = 0;
    std::vector<std::string> arg_types;
    std::vector<MethodArgument> args;
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
    const auto start = name.find("T = ") + 4;
    const auto end = name.find(']', start);
    return name.substr(start, end - start);
#elif defined(_MSC_VER)
    std::string_view name = __FUNCSIG__;
    auto start = name.find("type_name<") + 10;
    auto end = name.find(">(void)");
    return name.substr(start, end - start);
#endif
}

template <typename T>
MethodArgument make_argument_info(std::string name = {}) {
    using RawT = T;
    using NoRefT = std::remove_reference_t<RawT>;
    using NoCvRefT = std::remove_cvref_t<RawT>;

    MethodArgument arg{};
    arg.name = std::move(name);
    arg.type = std::string(type_name<RawT>());
    arg.bare_type = std::string(type_name<NoCvRefT>());

    arg.is_const = std::is_const_v<NoRefT>;
    arg.is_volatile = std::is_volatile_v<NoRefT>;

    arg.is_lvalue_reference = std::is_lvalue_reference_v<RawT>;
    arg.is_rvalue_reference = std::is_rvalue_reference_v<RawT>;

    arg.is_pointer = std::is_pointer_v<NoCvRefT>;

    if constexpr (std::is_pointer_v<NoCvRefT>) {
        using PointeeT = std::remove_pointer_t<NoCvRefT>;

        arg.is_pointer_to_const = std::is_const_v<PointeeT>;
        arg.is_const_pointer = std::is_const_v<NoRefT>;
    }

    arg.is_array = std::is_array_v<NoCvRefT>;

    arg.is_signed = std::is_signed_v<NoCvRefT>;
    arg.is_unsigned = std::is_unsigned_v<NoCvRefT>;
    arg.is_integral = std::is_integral_v<NoCvRefT>;
    arg.is_floating_point = std::is_floating_point_v<NoCvRefT>;

    arg.is_enum = std::is_enum_v<NoCvRefT>;
    arg.is_class = std::is_class_v<NoCvRefT>;

    return arg;
}

template <class Tuple, std::size_t... I>
std::vector<MethodArgument> arg_infos_impl(std::index_sequence<I...>) {
    std::vector<MethodArgument> result;
    result.reserve(sizeof...(I));
    (result.emplace_back(make_argument_info<std::tuple_element_t<I, Tuple>>()), ...);
    return result;
}

template <class Tuple>
std::vector<MethodArgument> arg_infos() {
    return arg_infos_impl<Tuple>(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

template <auto function_meta, class Entry>
void fill_argument_names_from_meta(Entry& entry) {
    std::size_t index = 0;

    template for (constexpr auto p : std::define_static_array(std::meta::parameters_of(function_meta))) {
        if constexpr (std::meta::has_identifier(p)) {
            if (index < entry.args.size()) {
                entry.args[index].name = std::string(std::meta::identifier_of(p));
            }
        }

        ++index;
    }
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

template <auto function_meta>
constexpr size_t get_parameter_count() {
    size_t count = 0;
    template for (constexpr auto p : std::define_static_array(std::meta::parameters_of(function_meta))) {
        ++count;
    }
    return count;
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
            constexpr auto argcount = get_parameter_count<m>();
            auto return_type = type_name<PMF>();

            if constexpr (std::meta::is_static_member(m)) {
                MethodEntry entry = make_static_entry<C, PMF>(std::meta::identifier_of(m), pmf);
                entry.is_static = true;
                entry.is_virtual = false;
                entry.is_const = false;
                entry.argcount = argcount;
                entry.pretty_name = return_type;
                entry.arg_types = arg_type_names<typename traits::args_tuple>();
                entry.args = arg_infos<typename traits::args_tuple>();
                fill_argument_names_from_meta<m>(entry);
                table.insert({entry.name, entry});
            } else {
                MethodEntry entry = make_member_entry<C, PMF>(std::meta::identifier_of(m), pmf);
                entry.is_static = false;
                entry.is_virtual = std::meta::is_virtual(m);
                entry.is_const = traits::is_const;
                entry.argcount = argcount;
                entry.pretty_name = return_type;
                entry.arg_types = arg_type_names<typename traits::args_tuple>();
                entry.args = arg_infos<typename traits::args_tuple>();
                fill_argument_names_from_meta<m>(entry);
                table.insert({entry.name, entry});
            }
        }
    }

    return table;
}

