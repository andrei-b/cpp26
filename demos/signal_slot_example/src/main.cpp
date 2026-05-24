#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "signal_slot.hpp"

struct Reading {
    std::string sensor;
    double value_celsius{};
    int sequence{};
};

std::ostream& operator<<(std::ostream& os, const Reading& r) {
    return os << r.sensor << " #" << r.sequence << " = "
              << std::fixed << std::setprecision(1) << r.value_celsius << " C";
}

class TemperatureSensor {
public:
    Signal<Reading> reading_ready;

    explicit TemperatureSensor(std::string name) : name_(std::move(name)) {}

    void sample(double value_celsius) {
        Reading reading{name_, value_celsius, ++sequence_};
        std::cout << "[sensor] " << reading << '\n';
        reading_ready.emit(reading);
    }

private:
    std::string name_;
    int sequence_ = 0;
};

class TelemetryLogger {
public:
    void store(Reading reading) {
        history_.push_back(reading);
        std::cout << "[logger] stored reading; total=" << history_.size() << '\n';
    }

    void print_summary() const {
        std::cout << "\n--- telemetry summary ---\n";
        for (const auto& r : history_) {
            std::cout << "  " << r << '\n';
        }
    }

private:
    std::vector<Reading> history_;
};

class CoolingFan {
public:
    void set_enabled(bool enabled) {
        if (enabled == enabled_) {
            return;
        }
        enabled_ = enabled;
        std::cout << "[fan] " << (enabled_ ? "ON" : "OFF") << '\n';
    }

private:
    bool enabled_ = false;
};

class MaintenancePager {
public:
    void page_technician(std::string message) {
        std::cout << "[pager] " << message << '\n';
    }

    void record_alarm(Reading reading) {
        std::cout << "[pager] alarm recorded for " << reading << '\n';
    }
};

class SafetyController {
public:
    Signal<bool> cooling_required;
    Signal<std::string> alert_text;
    Signal<Reading> alarm_reading;

    explicit SafetyController(double limit_celsius) : limit_(limit_celsius) {}

    void inspect(Reading reading) {
        const bool too_hot = reading.value_celsius >= limit_;

        std::cout << "[safety] " << (too_hot ? "limit exceeded" : "normal") << '\n';
        cooling_required.emit(too_hot);

        if (too_hot) {
            alert_text.emit("Temperature limit exceeded by " + reading.sensor);
            alarm_reading.emit(reading);
        }
    }

private:
    double limit_{};
};

int main() {
    TemperatureSensor oven{"oven-A"};
    TelemetryLogger logger;
    SafetyController safety{75.0};
    CoolingFan fan;
    MaintenancePager pager;

    // Direct member-function slots.
    oven.reading_ready.connect(logger, &TelemetryLogger::store);
    oven.reading_ready.connect(safety, &SafetyController::inspect);
    safety.cooling_required.connect(fan, &CoolingFan::set_enabled);

    // Reflected slots: connected by method name and checked against signal arguments.
    safety.alert_text.connect_reflected(pager, "page_technician");
    safety.alarm_reading.connect_reflected(pager, "record_alarm");

    // Lambda slot for lightweight inline behavior.
    auto console_token = oven.reading_ready.connect([](Reading reading) {
        std::cout << "[console] live dashboard saw " << reading << '\n';
    });

    for (double value : {63.5, 70.0, 78.2, 81.0, 69.4}) {
        oven.sample(value);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "\n[main] disconnecting live dashboard\n";
    oven.reading_ready.disconnect(console_token);
    oven.sample(76.8);

    logger.print_summary();
}
