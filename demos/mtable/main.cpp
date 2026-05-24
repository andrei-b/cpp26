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
    void use_private() {
        some_private_method(0);
    }
    private:
    void some_private_method(int x) {
        std::cout << "some_private_method(" << x << ")\n";
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

    // Lightweight runtime tests for reflection table and pointer helpers.
    auto expect = [](bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string("test failed: ") + std::string(message));
        }
    };

    {
        expect(!mt.empty(), "method table is not empty");

        for (const auto& entry : mt) {
            expect(entry.second.function_pointer.has_value(), "function pointer is populated");
        }

        auto distance_it = mt.find("distance");
        expect(distance_it != mt.end(), "distance method exists");
        expect(distance_it->second.arg_count == 4, "distance has 4 arguments");
        expect(distance_it->second.return_type.is_floating_point, "distance returns floating-point");

        using DistancePtr = make_member_pointer_type_t<TestClass, double, double, double, double, double>;
        DistancePtr distance_ptr = get_member_function_pointer<TestClass, double, double, double, double, double>(distance_it->second);
        double distance_value = (tc.*distance_ptr)(0.0, 0.0, 3.0, 4.0);
        expect(std::abs(distance_value - 5.0) < 1e-12, "distance pointer invocation returns 5");

        auto mul_it = mt.find("mul");
        expect(mul_it != mt.end(), "mul method exists");
        auto mul_ptr = get_static_function_pointer<int, int, int>(mul_it->second);
        expect(mul_ptr(6, 7) == 42, "mul pointer invocation returns 42");

        auto add_range = mt.equal_range("add");
        bool saw_add2 = false;
        bool saw_add3 = false;
        for (auto it = add_range.first; it != add_range.second; ++it) {
            saw_add2 = saw_add2 || (it->second.arg_count == 2);
            saw_add3 = saw_add3 || (it->second.arg_count == 3);
        }
        expect(saw_add2 && saw_add3, "add overloads with 2 and 3 args are present");

        std::cout << "[tests] all runtime checks passed\n";
    }

    for (const auto& entry : mt) {
        std::cout << "Method: \"" << entry.first
                  << "\" [" << (entry.second.is_accessible_from_current ? "publicly accessible" : "non-public") << "]"
                  << ", args count: " << entry.second.arg_count
                  << ", return type: " << entry.second.pretty_name
                  << ", has pointer: " << std::boolalpha << entry.second.function_pointer.has_value()
                  << "\n";
        for (const auto &t : entry.second.arg_types) {
            std::cout << " " << t;
        }
        std::cout << "\n";

        // Display MethodArgument metadata
        if (!entry.second.args.empty()) {
            std::cout << "  Arguments:\n";
            for (size_t i = 0; i < entry.second.args.size(); ++i) {
                const auto& arg = entry.second.args[i];
                std::cout << "    [" << i << "] ";

                // Parameter name (from reflection)
                if (!arg.name.empty()) {
                    std::cout << "name=" << arg.name;
                } else {
                    std::cout << "name=<unnamed>";
                }
                std::cout << ", type=" << arg.type << ", bare_type=" << arg.bare_type;

                // Qualifiers
                std::string quals;
                if (arg.is_const) quals += "const ";
                if (arg.is_volatile) quals += "volatile ";
                if (arg.is_lvalue_reference) quals += "lvalue_ref ";
                if (arg.is_rvalue_reference) quals += "rvalue_ref ";
                if (arg.is_pointer) quals += "pointer ";
                if (arg.is_pointer_to_const) quals += "ptr_to_const ";
                if (arg.is_const_pointer) quals += "const_ptr ";
                if (arg.is_array) quals += "array ";
                if (!quals.empty()) {
                    std::cout << " [" << quals << "]";
                }

                // Type category
                std::string category;
                if (arg.is_enum) category += "enum ";
                if (arg.is_class) category += "class ";
                if (arg.is_integral) category += "integral ";
                if (arg.is_floating_point) category += "floating_point ";
                if (arg.is_signed) category += "signed ";
                if (arg.is_unsigned) category += "unsigned ";
                if (!category.empty()) {
                    std::cout << " <" << category << ">";
                }

                std::cout << "\n";
            }
        }

        // Display return type metadata
        {
            const auto& ret = entry.second.return_type;
            std::cout << "  Return Type:\n";
            std::cout << "    type=" << ret.type << ", bare_type=" << ret.bare_type;

            // Qualifiers
            std::string quals;
            if (ret.is_const) quals += "const ";
            if (ret.is_volatile) quals += "volatile ";
            if (ret.is_lvalue_reference) quals += "lvalue_ref ";
            if (ret.is_rvalue_reference) quals += "rvalue_ref ";
            if (ret.is_pointer) quals += "pointer ";
            if (ret.is_pointer_to_const) quals += "ptr_to_const ";
            if (ret.is_const_pointer) quals += "const_ptr ";
            if (ret.is_array) quals += "array ";
            if (!quals.empty()) {
                std::cout << " [" << quals << "]";
            }

            // Type category
            std::string category;
            if (ret.is_void) category += "void ";
            if (ret.is_enum) category += "enum ";
            if (ret.is_class) category += "class ";
            if (ret.is_integral) category += "integral ";
            if (ret.is_floating_point) category += "floating_point ";
            if (ret.is_signed) category += "signed ";
            if (ret.is_unsigned) category += "unsigned ";
            if (!category.empty()) {
                std::cout << " <" << category << ">";
            }

            std::cout << "\n";
        }
    }

    auto d =  mt.find("distance");
    if (d != mt.end()) {
        std::cout << "Invoking distance via method table...\n";
        auto result = d->second.invoke(p, {0.0, 0.0, 3.0, 4.0});
        std::cout << "Result: " << std::any_cast<double>(result) << "\n";

        // Demonstrate typed pointer extraction helper
        try {
            using DistancePtr = double (TestClass::*)(double, double, double, double);
            auto pmf = get_function_pointer<DistancePtr>(d->second);
            std::cout << "Successfully extracted typed pointer for distance method\n";
        } catch (const std::exception& e) {
            std::cerr << "Error extracting pointer: " << e.what() << "\n";
        }
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
