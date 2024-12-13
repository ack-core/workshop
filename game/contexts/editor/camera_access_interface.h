
#pragma once

namespace game {
    class CameraAccessInterface : public Interface {
    public:
        virtual float getOrbitSize() const = 0;

    public:
        ~CameraAccessInterface() override {}
    };
}
