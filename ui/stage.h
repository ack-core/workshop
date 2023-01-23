
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/math.h"

namespace ui {
    using ElementToken = std::uint64_t;
    static const ElementToken INVALID_TOKEN = std::uint64_t(-1);
    
    enum class ActionType {
        BEGIN,
        END,
    };
    
    struct RectOptions {
    
    };
    
    class StageInterface {
    public:
        static std::shared_ptr<StageInterface> instance(const foundation::RenderingInterfacePtr &rendering);
        
    public:
        virtual const std::shared_ptr<foundation::PlatformInterface> &getPlatformInterface() const = 0;
        virtual const std::shared_ptr<foundation::RenderingInterface> &getRenderingInterface() const = 0;
        
        virtual ElementToken addRect(ElementToken parent, const math::vector2f &lt, const math::vector2f &size) = 0;
        virtual void setActionHandler(ElementToken element, std::function<void(ActionType type)> &handler) = 0;
        virtual void removeElement(ElementToken token) = 0;
        
        virtual void updateAndDraw(float dtSec) = 0;
        
    public:
        virtual ~StageInterface() = default;
    };
    
    using StageInterfacePtr = std::shared_ptr<StageInterface>;
}

