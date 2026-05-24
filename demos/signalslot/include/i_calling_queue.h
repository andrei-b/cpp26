
#ifndef SIGNALSLOT_ICALLQUEUE_H
#define SIGNALSLOT_ICALLQUEUE_H
#include <functional>

using
    Callable = std::function<void()>;

class ICallingQueue {
    public:
        virtual void put_callable(Callable &&callable) = 0;
        virtual ~ICallingQueue() = default;
};

#endif //SIGNALSLOT_ICALLQUEUE_H
