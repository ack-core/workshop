
#include "stage.h"

#include <cfloat>
#include <memory>
#include <vector>
#include <unordered_map>

#include "thirdparty/upng/upng.h"

namespace ui {
    class StageInterfaceImpl : public StageInterface {
    public:
        StageInterfaceImpl(const foundation::RenderingInterfacePtr &rendering);
        ~StageInterfaceImpl() override;
        
    public:
        const std::shared_ptr<foundation::PlatformInterface> &getPlatformInterface() const override { return _platform; }
        const std::shared_ptr<foundation::RenderingInterface> &getRenderingInterface() const override { return _rendering; }
        
        ElementToken addRect(ElementToken parent, const math::vector2f &lt, const math::vector2f &size) override;
        void setActionHandler(ElementToken element, std::function<void(ActionType type)> &handler) override;
        void removeElement(ElementToken token) override;
        
        void updateAndDraw(float dtSec) override;
        
    private:
        const foundation::PlatformInterfacePtr _platform;
        const foundation::RenderingInterfacePtr _rendering;
    };

    std::shared_ptr<StageInterface> StageInterface::instance(const foundation::RenderingInterfacePtr &rendering) {
        return std::make_shared<StageInterfaceImpl>(rendering);
    }
}

namespace ui {
    StageInterfaceImpl::StageInterfaceImpl(const foundation::RenderingInterfacePtr &rendering) : _platform(rendering->getPlatformInterface()), _rendering(rendering) {
    
    }
    
    StageInterfaceImpl::~StageInterfaceImpl() {
    
    }
        
    ElementToken StageInterfaceImpl::addRect(ElementToken parent, const math::vector2f &lt, const math::vector2f &size) {
    
    }
    
    void StageInterfaceImpl::setActionHandler(ElementToken element, std::function<void(ActionType type)> &handler) {
    
    }

    void StageInterfaceImpl::removeElement(ElementToken token) {
    
    }
    
    void StageInterfaceImpl::updateAndDraw(float dtSec) {
    
    }
}
