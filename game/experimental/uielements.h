
#pragma once
#include <game/context.h>

namespace game {
    class UIElementsContext : public Context {
    public:
        UIElementsContext(API &&api);
        ~UIElementsContext() override {}

        void init() override {}
        void update(float dtSec) override {}

    private:
        const API _api;

        ui::Img9SlicePtr  _img;
        ui::InteractorPtr _button;
        ui::InteractorPtr _joystick;
    };
}
