#include "self_reflected.h"

#include <iostream>
#include <string>

class Sensor final : public SelfReflected<Sensor>
{
public:
    Sensor(int temperature, std::string name)
        : temperature_{temperature},
          name_{std::move(name)}
    {
    }

    [[nodiscard]]
    int get_temperature() const
    {
        return temperature_;
    }

    [[nodiscard]]
    const std::string& get_name() const
    {
        return name_;
    }

    // Ignored: name doesn't start with get_.
    [[nodiscard]]
    bool is_active() const
    {
        return true;
    }

    // Ignored: requires an argument.
    [[nodiscard]]
    int get_value(int index) const
    {
        return index;
    }

private:
    int temperature_;
    std::string name_;
};

int main()
{
    Sensor sensor{23, "outdoor"};

    std::cout << sensor.to_json().dump(4) << '\n';
}
