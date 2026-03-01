#include <meta>
#include <print>
#include <utility>

namespace meta = std::meta;

// Turn a reflected entity into a callable.
// Fn must reflect a member function *or* a pointer-to-member function expression


struct Calc {
    int add(int a, int b) { return a + b; }
    int add(int a) { return a + 100; }
};

template <class T>
using PMF2 = int (T::*)(int, int);

using PMF1 = int (Calc::*)(int);

template <std::meta::info Fn, class C, class PMF>
constexpr auto make_callable_from() {
    constexpr PMF pmf = static_cast<PMF>([:Fn:]);
    return [pmf](C& obj, auto&&... args) -> decltype(auto) {
        return (obj.*pmf)(std::forward<decltype(args)>(args)...);
    };
}

int main() {
    using PMF2Calc = PMF2<Calc>;
    using PMF1Calc = PMF1;

    // disambiguate overload
    constexpr PMF2Calc p2 = static_cast<PMF2Calc>(&Calc::add);
    constexpr PMF1Calc p1 = static_cast<PMF1Calc>(&Calc::add);

    auto add2 = make_callable_from<^^p2, Calc, PMF2Calc>();
    auto add1 = make_callable_from<^^p1, Calc, PMF1Calc>();

    Calc c;
    std::println("Rsult {}, {}", add2(c, 3, 4), add1(c, 3));
}