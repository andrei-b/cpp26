#ifndef DUMP_REFLECTABLE_H
#define DUMP_REFLECTABLE_H

#include <meta>
#include <string>
#include <string_view>
#include <type_traits>

#include <nlohmann/json.hpp>

template<typename Derived>
class SelfReflected
{
public:
    [[nodiscard]]
    nlohmann::json to_json() const
    {
        return make_json(static_cast<const Derived&>(*this));
    }

private:
    template<typename T>
    static nlohmann::json make_json(const T& object)
    {
        nlohmann::json result = nlohmann::json::object();

        // members_of() returns a transient compile-time vector.
        // define_static_array() makes it usable by template-for.
        template for (constexpr auto member :
            std::define_static_array(
                std::meta::members_of(
                    ^^T,
                    std::meta::access_context::current())))
        {
            if constexpr (
                std::meta::is_function(member) &&
                !std::meta::is_static_member(member) &&
                std::meta::has_identifier(member))
            {
                constexpr std::string_view name =
                    std::meta::identifier_of(member);

                if constexpr (
                    name.starts_with("get_") &&
                    name.size() > 4 &&
                    requires {
                        // Verifies that this is a const-callable
                        // zero-argument getter.
                        object.[:member:]();
                    })
                {
                    const std::string json_name{name.substr(4)};
                    result[json_name] = object.[:member:]();
                }
            }
        }

        return result;
    }
};

#endif //DUMP_REFLECTABLE_H
