#pragma once

#include <QObject>
#include <QVariant>
#include <QVariantList>
#include <QString>
#include <QStringList>
#include <QMetaObject>

#include <any>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "method_table.hpp"

namespace reflected_qt {

namespace detail {

template <class T>
using bare_t = std::remove_cvref_t<T>;

template <typename T>
T variant_cast_argument(const QVariant& v) {
    using Bare = bare_t<T>;

    if constexpr (std::is_lvalue_reference_v<T>) {
        // The result is copied into std::any by the caller before method_table.hpp casts it.
        return v.value<Bare>();
    } else if constexpr (std::is_pointer_v<Bare>) {
        return v.value<Bare>();
    } else {
        return v.value<Bare>();
    }
}

template <typename... Args, std::size_t... I>
std::vector<std::any> pack_qvariants_impl(const QVariantList& values, std::index_sequence<I...>) {
    if (values.size() != static_cast<int>(sizeof...(Args))) {
        throw std::runtime_error("QVariant argument count mismatch");
    }

    std::vector<std::any> packed;
    packed.reserve(sizeof...(Args));
    (packed.emplace_back(variant_cast_argument<Args>(values.at(static_cast<int>(I)))), ...);
    return packed;
}

template <typename... Args>
std::vector<std::any> pack_qvariants(const QVariantList& values) {
    return pack_qvariants_impl<Args...>(values, std::make_index_sequence<sizeof...(Args)>{});
}

template <typename R>
QVariant any_to_qvariant(std::any value) {
    if constexpr (std::is_void_v<R>) {
        return {};
    } else {
        if (!value.has_value()) {
            return {};
        }
        return QVariant::fromValue(std::any_cast<R>(std::move(value)));
    }
}

template <typename... Args>
void invoke_direct(MethodEntry entry, void* object, Args&&... args) {
    std::tuple<std::remove_cvref_t<Args>...> forwarded(std::forward<Args>(args)...);
    entry.direct_invoke(object, static_cast<void*>(&forwarded));
}

template <typename R, typename... Args>
R invoke_any(MethodEntry entry, void* object, Args&&... args) {
    std::vector<std::any> packed;
    packed.reserve(sizeof...(Args));
    (packed.emplace_back(std::forward<Args>(args)), ...);

    std::any result = entry.invoke(object, std::span<const std::any>(packed.data(), packed.size()));
    if constexpr (std::is_void_v<R>) {
        return;
    } else {
        return std::any_cast<R>(std::move(result));
    }
}

} // namespace detail

// A small Qt-facing facade over your method_table.hpp reflection table.
// It does not require Q_OBJECT or moc for T.  It exposes reflected methods as
// functors that are valid Qt 6 connect() targets.
template <class T>
class QClassReflection {
public:
    explicit QClassReflection(T& object, bool allow_non_public = false)
        : object_(&object), allow_non_public_(allow_non_public), table_(get_method_table<T>()) {}

    [[nodiscard]] T& object() const { return *object_; }

    [[nodiscard]] QStringList methodNames() const {
        QStringList names;
        names.reserve(static_cast<int>(table_.size()));
        for (const auto& [name, entry] : table_) {
            if (allow_non_public_ || entry.is_accessible_from_current) {
                names.push_back(QString::fromUtf8(std::string(name)));
            }
        }
        names.removeDuplicates();
        names.sort();
        return names;
    }

    [[nodiscard]] std::vector<MethodEntry> overloads(QStringView methodName) const {
        std::vector<MethodEntry> out;
        const std::string key = methodName.toString().toStdString();
        auto range = table_.equal_range(std::string_view(key));
        for (auto it = range.first; it != range.second; ++it) {
            if (allow_non_public_ || it->second.is_accessible_from_current) {
                out.push_back(it->second);
            }
        }
        return out;
    }

    template <typename... Args>
    [[nodiscard]] MethodEntry find(QStringView methodName) const {
        return select_reflected_method_entry<T, Args...>(methodName.toString().toStdString(), allow_non_public_);
    }

    // Typed invocation by method name. Uses method_table.hpp's std::any invoker.
    template <typename R = void, typename... Args>
    R invoke(QStringView methodName, Args&&... args) const {
        MethodEntry entry = find<std::remove_cvref_t<Args>...>(methodName);
        return detail::invoke_any<R>(std::move(entry), static_cast<void*>(object_), std::forward<Args>(args)...);
    }

    // QVariant-based invocation when the caller provides the C++ argument types.
    template <typename R = void, typename... Args>
    QVariant invokeQt(QStringView methodName, const QVariantList& values) const {
        MethodEntry entry = select_reflected_method_entry<T, Args...>(methodName.toString().toStdString(), allow_non_public_);
        std::vector<std::any> packed = detail::pack_qvariants<Args...>(values);
        std::any result = entry.invoke(static_cast<void*>(object_), std::span<const std::any>(packed.data(), packed.size()));
        return detail::any_to_qvariant<R>(std::move(result));
    }

    // Return a functor that can be used as a Qt 6 callable/slot.
    // Example: QObject::connect(slider, &QSlider::valueChanged,
    //                          reflection.qtSlot<int>("setValue"));
    template <typename... Args>
    [[nodiscard]] auto qtSlot(QStringView methodName) const {
        MethodEntry entry = select_reflected_method_entry<T, Args...>(methodName.toString().toStdString(), allow_non_public_);
        T* object = object_;
        return [object, entry](Args... args) mutable {
            detail::invoke_direct<Args...>(entry, static_cast<void*>(object), std::forward<Args>(args)...);
        };
    }

    // Same as qtSlot(), but invokes through std::any. This is slower, but useful
    // when you want return values or want to avoid direct tuple forwarding.
    template <typename R = void, typename... Args>
    [[nodiscard]] auto qtCallable(QStringView methodName) const {
        MethodEntry entry = select_reflected_method_entry<T, Args...>(methodName.toString().toStdString(), allow_non_public_);
        T* object = object_;
        return [object, entry](Args... args) mutable -> R {
            return detail::invoke_any<R, Args...>(entry, static_cast<void*>(object), std::forward<Args>(args)...);
        };
    }

    // Convenience connection for Qt signals whose argument list exactly matches
    // the reflected method. Pass a context QObject if the target is owned by it.
    template <class Sender, typename... Args>
    QMetaObject::Connection connect(Sender* sender,
                                    void (Sender::*signal)(Args...),
                                    QStringView methodName,
                                    Qt::ConnectionType type = Qt::AutoConnection) const {
        return QObject::connect(sender, signal, qtSlot<Args...>(methodName), type);
    }

    template <class Sender, typename... Args>
    QMetaObject::Connection connect(Sender* sender,
                                    void (Sender::*signal)(Args...),
                                    const QObject* context,
                                    QStringView methodName,
                                    Qt::ConnectionType type = Qt::AutoConnection) const {
        return QObject::connect(sender, signal, context, qtSlot<Args...>(methodName), type);
    }

    template <class Sender, typename... Args>
    QMetaObject::Connection connect(Sender* sender,
                                    void (Sender::*signal)(Args...) const,
                                    QStringView methodName,
                                    Qt::ConnectionType type = Qt::AutoConnection) const {
        return QObject::connect(sender, signal, qtSlot<Args...>(methodName), type);
    }

    template <class Sender, typename... Args>
    QMetaObject::Connection connect(Sender* sender,
                                    void (Sender::*signal)(Args...) const,
                                    const QObject* context,
                                    QStringView methodName,
                                    Qt::ConnectionType type = Qt::AutoConnection) const {
        return QObject::connect(sender, signal, context, qtSlot<Args...>(methodName), type);
    }

private:
    T* object_ = nullptr;
    bool allow_non_public_ = false;
    std::unordered_multimap<std::string_view, MethodEntry> table_;
};

template <class T>
QClassReflection<T> reflectQt(T& object, bool allow_non_public = false) {
    return QClassReflection<T>(object, allow_non_public);
}

} // namespace reflected_qt
