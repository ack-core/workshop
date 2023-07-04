
#pragma once

#include <cstddef>
#include <memory>

/*
TODO:
            template <class C> callback(const std::weak_ptr<C> &weak, R(C::*method)(Args...)) : callback([weak, method](Args... args) -> R {
                if (auto target = weak.lock()) {
                    return (target.get()->*method)(args...);
                }
                
                return {};
            }) {}

 */

namespace util {
    template <typename M> class callback final {};
    template <typename R, typename... Args> class callback <R(Args...)> final {
    public:
        callback() {}
        template <typename L, R(L::*)(Args...) const = &L::operator()> callback(L&& lambda) {
            _data.target = new L (std::move(lambda));
            _data.call = [](void *ptr, Args... args) {
                return (static_cast<L *>(ptr))->operator()(std::forward<Args>(args)...);
            };
            _data.clean = [](void *ptr) {
                delete static_cast<L *>(ptr);
            };
        }
        template <typename L, R(L::*)(Args...) = &L::operator()> callback(L&& lambda) {
            _data.target = new L (std::move(lambda));
            _data.call = [](void *ptr, Args... args) {
                return (static_cast<L *>(ptr))->operator()(std::forward<Args>(args)...);
            };
            _data.clean = [](void *ptr) {
                delete static_cast<L *>(ptr);
            };
        }
        
        callback(callback &&other) : _data(other._data) {
            other._data = {};
        }
        callback& operator =(callback &&other) {
            _data.clean(_data.target);
            _data = other._data;
            other._data = {};
            return *this;
        }
        
        ~callback() {
            _data.clean(_data.target);
        }
        
        operator bool () const {
            return _data.target != nullptr;
        }
        
        decltype(auto) operator ()(Args... args) const {
            return _data.call(_data.target, std::forward<Args>(args)...);
        }
        
    private:
        struct Data {
            R(*call)(void *ptr, Args...) = nullptr;
            void (*clean)(void *ptr) = [](void *){};
            void *target = nullptr;
        }
        _data;
        
    private:
        callback(const callback &) = delete;
        callback& operator =(const callback &) = delete;
    };
}
