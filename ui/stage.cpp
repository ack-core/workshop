
#include "stage.h"

#include <cfloat>
#include <memory>
#include <vector>
#include <unordered_map>

#include "thirdparty/upng/upng.h"

namespace ui {
    class StageInterfaceImpl : public StageInterface {
    public:
        StageInterfaceImpl(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering);
        ~StageInterfaceImpl() override;
        
    public:
        ElementToken addRect(ElementToken parent, const math::vector2f &lt, const math::vector2f &size) override;
        void setActionHandler(ElementToken element, std::function<void(ActionType type)> &handler) override;
        void removeElement(ElementToken token) override;
        
        void updateAndDraw(float dtSec) override;
        
    private:
        const foundation::PlatformInterfacePtr _platform;
        const foundation::RenderingInterfacePtr _rendering;
    };

    std::shared_ptr<StageInterface> StageInterface::instance(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering) {
        return std::make_shared<StageInterfaceImpl>(platform, rendering);
    }
}

namespace ui {
    StageInterfaceImpl::StageInterfaceImpl(const foundation::PlatformInterfacePtr &platform, const foundation::RenderingInterfacePtr &rendering)
    : _platform(platform)
    , _rendering(rendering)
    {
    
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
