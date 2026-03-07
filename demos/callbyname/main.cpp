#include <meta>
#include <iostream>
#include <string_view>

struct Service
{
    void hello() {
        std::cout << "hello()\n";
    }

    void ping() {
        std::cout << "pong\n";
    }

    void goodbye() {
        std::cout << "goodbye()\n";
    }
};

template <typename T>
bool call_method(T& obj, std::string_view name)
{
    constexpr auto type = ^^T;
    constexpr auto ctx  = std::meta::access_context::current();

    template for (constexpr auto m :
                  std::define_static_array(std::meta::members_of(type, ctx)))
    {
        if constexpr (std::meta::is_function(m) && std::meta::has_identifier(m))
        {
            if (name == std::meta::identifier_of(m))
            {
                auto pmf = &[:m:];
                (obj.*pmf)();
                return true;
            }
        }
    }

    return false;
}

int main()
{
    Service s;

    call_method(s, "hello");
    call_method(s, "ping");
    std::string str = "goodbye";
    call_method(s, str);
    if (!call_method(s, "missing")) {
        std::cout << "Unknown method\n";
    }
}