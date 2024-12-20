
#pragma once

namespace game {
    class CameraAccessInterface : public Interface {
    public:
        virtual auto getOrbitSize() const -> float = 0;
        virtual auto getTarget() const -> math::vector3f = 0;


    public:
        ~CameraAccessInterface() override {}
    };
}
