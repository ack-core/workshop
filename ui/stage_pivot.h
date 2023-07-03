
#pragma once

namespace ui {
    class PivotImpl : public ElementImpl, public StageInterface::Pivot {
    public:
        PivotImpl(const StageFacility &facility, const std::shared_ptr<Element> &parent);
        ~PivotImpl() override;
        
    public:
        void updateCoordinates() override;
        void setScreenPosition(const math::vector2f &position) override;
        void setWorldPosition(const math::vector3f &position) override;
        
        bool onInteraction(ui::Action action, std::size_t id, float x, float y) override;
        void draw(ShaderConstants &constStorage) override;
        
    private:
        enum class CoordinateState {
            ANCHOR = 0,
            SCREEN,
            WORLD
        }
        _state;
        
        math::vector2f _screenPosition;
        math::vector3f _worldPosition;
    };
}

