#pragma once

#include <any>
#include <meta>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "method_table.hpp"

// Base runtime interface for dynamic invocation.
class Invokable {
public:
    virtual ~Invokable() = default;

    virtual std::any invoke(std::string_view method_name,
                            std::span<const std::any> args) = 0;
};

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

                if constexpr (std::meta::is_static_member(m)) {
                    auto fn = &[:m:];
                    try {
                        result = invoke_static_with_anys(fn, args);
                        if (matched_overload) {
                            throw std::runtime_error("ambiguous overload");
                        }
                        matched_overload = true;
                    } catch (const std::bad_any_cast&) {
                    } catch (const std::runtime_error&) {
                    }
                } else {
                    auto pmf = &[:m:];
                    try {
                        result = invoke_member_with_anys(obj, pmf, args);
                        if (matched_overload) {
                            throw std::runtime_error("ambiguous overload");
                        }
                        matched_overload = true;
                    } catch (const std::bad_any_cast&) {
                    } catch (const std::runtime_error&) {
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

template <typename... Args>
std::any invoke_dyn(Invokable* obj,
                    std::string_view method_name,
                    Args&&... args) {
    std::vector<std::any> packed;
    packed.reserve(sizeof...(Args));
    (packed.emplace_back(std::forward<Args>(args)), ...);
    return obj->invoke(method_name, packed);
}

