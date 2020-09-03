#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>
#include <unordered_map>
#include <functional>

namespace datahub {
    typedef struct {} *Token;
    template <class> constexpr int FALSE = 0;

    template<typename T> class Event final {
        static_assert(FALSE<T>, "Invalid T for Event");
    };
    template<typename... Args> class Event<void(Args...)> final {
        template<typename> friend struct Array;
        template<typename> friend struct Value;

    public:
        template<typename L, void(L:: *)(Args...) const = &L::operator()> Token operator+=(L &&lambda) {
            Token token = reinterpret_cast<Token>(_uniqueid++);
            return _handlers.emplace_back(token, std::move(lambda)), token;
        }
        void operator-=(Token id) {
            _handlers.erase(std::remove_if(std::begin(_handlers), std::end(_handlers), [id](const auto &item) {
                return item.first == reinterpret_cast<std::size_t>(id);
            }), std::end(_handlers));
        }

    private:
        template <typename... CallArgs> void call(CallArgs &&... args) {
            for (const auto &handler : _handlers) {
                handler.second(std::forward<CallArgs>(args)...);
            }
        }

    private:
        std::vector<std::pair<std::size_t, std::function<void(Args...)>>> _handlers;
        std::size_t _uniqueid = 0x1;
    };

    struct Scope {

    };

    template<typename T> struct Array {
        static_assert(std::is_base_of<Scope, T>::value, "T must be derived from Scope");

        Event<void(Token, T &)> onElementAdded;
        Event<void(Token)> onElementRemoving;

        Array() = default;

        template<typename L, void(L:: *)(T &) const = &L::operator()> Token add(L &&initializer) {
            Token result = reinterpret_cast<Token>(_uniqueid++);
            typename std::unordered_map<Token, T>::iterator position = _data.emplace(result, T{}).first;
            initializer(position->second);
            onElementAdded.call(result, position->second);
            return result;
        }

        void remove(Token id) {
            onElementRemoving.call(id);
            _data.erase(id);
        }

        T &operator[](Token id) {
            return _data[id];
        }

        template<typename L, void(L:: *)(Token, T &) const = &L::operator()> void foreach(L &&functor) {
            for (auto it = _data.begin(); it != _data.end(); ++it) {
                functor(it->first, it->second);
            }
        }

    private:
        std::unordered_map<Token, T> _data;
        std::size_t _uniqueid = 0x1;
    };

    template<typename T> struct Value {
        static_assert(std::is_trivially_constructible<T>::value, "");
        static_assert(std::is_copy_assignable<T>::value, "");

        Event<void(const T &)> onValueChanged;

        Value() = default;
        Value &operator =(const T &value) {
            _data = value;
            onValueChanged.call(_data);
            return *this;
        }

        operator const T &() const {
            return _data;
        }

    private:
        T _data = {};
        std::size_t _uniqueid = 0x1;
    };

    template<typename... Scopes> class DataHub {};
    template<typename Scope, typename... Scopes> class DataHub<Scope, Scopes...> : public DataHub<Scopes...> {
        static_assert(std::is_base_of<::datahub::Scope, Scope>::value, "DataHub scopes must be derived from Scope");

    public:
        template<typename T, typename = std::enable_if_t<std::is_same<T, Scope>::value>> T &get() {
            return _scope;
        }

    protected:
        Scope _scope = {};
    };
}
