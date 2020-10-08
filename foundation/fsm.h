#pragma once
#include <functional>

namespace gears {
    struct Dummy {};
    struct FSMState {
        FSMState() {

        }
        template<typename Target> FSMState(Target *obj, void(Target::*update)(float)) {
            this->update = std::bind(update, obj, std::placeholders::_1);
        }
        template<typename Target> FSMState(Target *obj, void(Target::*entered)(), void(Target::*update)(float), void(Target::*leaving)()) {
            this->entered = std::bind(entered, obj);
            this->update = std::bind(update, obj, std::placeholders::_1);
            this->leaving = std::bind(leaving, obj);
        }

        std::function<void()> entered;
        std::function<void(float)> update;
        std::function<void()> leaving;
    };

    //class FSMState {
    //    template<typename T> struct Typeless {
    //        static void call(void *target, void(Dummy::*mtd)(float), float dt) {
    //            if (mtd) {
    //                auto typedMethod = reinterpret_cast<void (T::*)(float)>(mtd);
    //                (reinterpret_cast<T *>(target)->*typedMethod)(dt);
    //            }
    //        }
    //    };

    //public:
    //    FSMState() {

    //    }
    //    template<typename Target> FSMState(Target *obj, void(Target::*update)(float)) {
    //        _target = obj;
    //        _update = reinterpret_cast<void(Dummy::*)(float)>(update);
    //        _call = Typeless<Target>::call;
    //    }

    //    void update(float dt) {
    //        _call(_target, _update, dt);
    //    }

    //private:
    //    void *_target = nullptr;
    //    void (Dummy::*_update)(float) = nullptr;

    //    void (*_call)(void *target, void(Dummy::*mtd)(float), float) = Typeless<Dummy>::call;
    //};

    template <typename NameType> struct FSM {
        FSM(const std::initializer_list<std::pair<NameType, FSMState>>& states) {
            for (auto &item : states) {
                _states[std::size_t(item.first)] = item.second;
            }
        }

        void changeState(const NameType &name) {
            if (&_states[std::size_t(name)] != _current) {
                if (_current && _current->leaving) {
                    _current->leaving();
                }

                _current = &_states[std::size_t(name)];

                if (_current->entered) {
                    _current->entered();
                }
            }
        }

        void update(float dt) {
            if (_current->update) {
                _current->update(dt);
            }
        }

        //bool isState(const NameType &name) const {
        //    return _current == _states + std::size_t(name);
        //}

        //FSMState &operator[](const NameType &name) {
        //    return _states[std::size_t(name)];
        //}

    private:
        FSMState _states[std::size_t(NameType::_count)];
        FSMState *_current = nullptr;

        FSM(FSM &&) = delete;
        FSM(const FSM &) = delete;
        FSM &operator =(FSM &&) = delete;
        FSM &operator =(const FSM &) = delete;
    };
}