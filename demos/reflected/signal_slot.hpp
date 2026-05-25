#pragma once

#include <any>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "i_calling_queue.h"
#include "method_table.hpp"

struct ConnectionToken {
    std::size_t id = 0;
};

struct AsyncConnectionToken : ConnectionToken {
    std::function<void()> async_call;
};

template <typename... Args, std::size_t... I>
bool method_entry_matches_signal_args_impl(const MethodEntry& entry, std::index_sequence<I...>) {
    return ((entry.args[I].bare_type == std::string(type_name<std::remove_cvref_t<Args>>())) && ...);
}

template <typename... Args>
bool method_entry_matches_signal_args(const MethodEntry& entry) {
    if (entry.args.size() != sizeof...(Args)) {
        return false;
    }

    return method_entry_matches_signal_args_impl<Args...>(entry, std::make_index_sequence<sizeof...(Args)>{});
}

template <class T, typename... Args>
MethodEntry select_reflected_method_entry(std::string_view method_name,
                                         bool allow_non_public) {
    auto table = get_method_table<T>();
    auto range = table.equal_range(method_name);

    bool saw_name = false;
    bool saw_overload = false;
    bool matched = false;
    MethodEntry selected{};

    for (auto it = range.first; it != range.second; ++it) {
        saw_name = true;
        const MethodEntry& entry = it->second;

        if (!allow_non_public && !entry.is_accessible_from_current) {
            continue;
        }

        if (entry.arg_count != sizeof...(Args)) {
            saw_overload = true;
            continue;
        }

        saw_overload = true;

        if (!method_entry_matches_signal_args<Args...>(entry)) {
            continue;
        }

        if (matched) {
            throw std::runtime_error("ambiguous slot overload");
        }

        selected = entry;
        matched = true;
    }

    if (!matched) {
        if (!saw_name) {
            throw std::runtime_error("slot method not found");
        }
        if (saw_overload) {
            throw std::runtime_error("slot overload does not match signal arguments");
        }
        throw std::runtime_error("slot is not accessible from current context");
    }

    return selected;
}

template <typename... Args>
class Signal {
public:
    using Slot = std::function<void(Args...)>;

    ConnectionToken connect(Slot slot) {
        ConnectionToken token{next_id_++};
        slots_.push_back({token.id, std::move(slot)});
        return token;
    }

    template <class F>
    ConnectionToken connect(F&& slot) {
        return connect(Slot(std::forward<F>(slot)));
    }

    template <class T>
    ConnectionToken connect(T& obj, void (T::*method)(Args...)) {
        return connect([&obj, method](Args... args) {
            (obj.*method)(args...);
        });
    }

    template <class T>
    ConnectionToken connect(T& obj, void (T::*method)(Args...) const) {
        return connect([&obj, method](Args... args) {
            (obj.*method)(args...);
        });
    }

    template <class T>
    ConnectionToken connect_reflected(T& obj,
                                      std::string_view method_name,
                                      bool allow_non_public = false) {
        MethodEntry selected = select_reflected_method_entry<T, Args...>(method_name, allow_non_public);

        return connect([selected, &obj](Args... args) {
            std::vector<std::any> packed;
            packed.reserve(sizeof...(Args));
            (packed.emplace_back(args), ...);
            (void)selected.invoke(static_cast<void*>(&obj), packed);
        });
    }

    template <class T>
    ConnectionToken connect_reflected_async(ICallingQueue& queue, T& obj,std::string_view method_name, bool allow_non_public = false) {
        MethodEntry selected = select_reflected_method_entry<T, Args...>(method_name, allow_non_public);
        return connect([selected, &queue, &obj](Args... args) {
            std::vector<std::any> packed;
            packed.reserve(sizeof...(Args));
            (packed.emplace_back(args), ...);
            queue.put_callable([selected, &obj, &packed]() {
                (void)selected.invoke(static_cast<void*>(&obj), packed);
            });

        });

    }

    template <class T>
    ConnectionToken connect_reflected_direct(T& obj,
                                             std::string_view method_name,
                                             bool allow_non_public = false) {
        MethodEntry selected = select_reflected_method_entry<T, Args...>(method_name, allow_non_public);

        return connect([selected, &obj](Args... args) {
            std::tuple<Args...> forwarded(std::forward<Args>(args)...);
            selected.direct_invoke(static_cast<void*>(&obj), static_cast<void*>(&forwarded));
        });
    }

    bool disconnect(ConnectionToken token) {
        for (std::size_t i = 0; i < slots_.size(); ++i) {
            if (slots_[i].id == token.id) {
                slots_.erase(slots_.begin() + static_cast<std::ptrdiff_t>(i));
                return true;
            }
        }
        return false;
    }

    void emit_direct(Args... args) const {
        for (const auto& slot : slots_) {
            slot.fn(args...);
        }
    }

    void operator()(Args... args) const {
        emit_direct(args...);
    }

    [[nodiscard]] std::size_t size() const {
        return slots_.size();
    }

private:
    struct ConnectedSlot {
        std::size_t id;
        Slot fn;
    };

    std::size_t next_id_ = 1;
    std::vector<ConnectedSlot> slots_;
};


