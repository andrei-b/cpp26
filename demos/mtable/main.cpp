#include <any>
#include <cmath>
#include <iostream>
#include <string>

#include "invokable.hpp"
#include "method_table.hpp"

// ============================================================
// Example derived class
// ============================================================

class TestClass : public Invokable {
public:
    void hello() {
        std::cout << "hello()\n";
    }

    std::string concat(const std::string &a, const std::string &b) {
        return a + ":" + b;
    }

    int add(int a, int b) {
        return a + b;
    }

    int add(int a, int b, int c) {
        return a + b + c;
    }

    double distance(double x1, double y1, double x2, double y2) {
        return std::sqrt((x2 - x1) * (x2 - x1) +
                         (y2 - y1) * (y2 - y1));
    }

    std::string toString(double x, double y) {
        return std::to_string(x) + ", " + std::to_string(y);
    }

    static int mul(int a, int b) {
        return a * b;
    }

    std::any invoke(std::string_view method_name,
                    std::span<const std::any> args) override {
        return invoke_reflected(*this, method_name, args);
    }
};

// ============================================================
// Demo
// ============================================================

int main() {
    std::cout << "Program started\n";
    std::cout.flush();

    TestClass tc;
    Invokable* p = &tc;


    auto mt = get_method_table<TestClass>();
    for (const auto& entry : mt) {
        std::cout << "Method: \"" << entry.first << "\", args count: " << entry.second.argcount << ", return type: " << entry.second.pretty_name << "\n";
        for (const auto &t : entry.second.arg_types) {
            std::cout << " " << t;
        }
        std::cout << "\n";
    }

    auto d =  mt.find("distance");
    if (d != mt.end()) {
        std::cout << "Invoking distance via method table...\n";
        auto result = d->second.invoke(p, {0.0, 0.0, 3.0, 4.0});
        std::cout << "Result: " << std::any_cast<double>(result) << "\n";
    } else {
        std::cout << "Method 'distance' not found in method table\n";
    }

    std::cout.flush();

    try {
        std::cout << "add(5, 6) = "
                  << std::any_cast<int>(invoke_dyn(p, "add", 5, 6))
                  << "\n";

        std::cout << "add(1, 2, 3) = "
                  << std::any_cast<int>(invoke_dyn(p, "add", 1, 2, 3))
                  << "\n";

        std::cout << "distance(0,0,3,4) = "
                  << std::any_cast<double>(invoke_dyn(p, "distance",
                                                      0.0, 0.0, 3.0, 4.0))
                  << "\n";

        std::cout << "toString(1.5,2.5) = "
                  << std::any_cast<std::string>(invoke_dyn(p, "toString",
                                                           1.5, 2.5))
                  << "\n";

        std::cout << "mul(6,7) = "
                  << std::any_cast<int>(invoke_dyn(p, "mul", 6, 7))
                  << "\n";

        invoke_dyn(p, "hello");
    }
    catch (const std::exception& e) {
        std::cerr << "invoke error: " << e.what() << "\n";
    }
}
