
#include "raycast.h"
#include <list>

namespace core {
    class BaseShapeImpl : public RaycastInterface::Shape {
    public:
        BaseShapeImpl();
        ~BaseShapeImpl() override {}
    };

    class SphereShapeImpl : public BaseShapeImpl {
    public:
        SphereShapeImpl();
        ~SphereShapeImpl();
        void setTransform(const math::transform3f &trfm) override;
        
    private:
        core::SceneInterface::LineSetPtr _visual;
    };
    
    class BoxShapeImpl : public BaseShapeImpl {
    public:
        BoxShapeImpl();
        ~BoxShapeImpl();
        void setTransform(const math::transform3f &trfm) override;
        
    private:
        core::SceneInterface::LineSetPtr _visual;
    };
}

namespace core {
    class RaycastInterfaceImpl : public RaycastInterface {
    public:
        RaycastInterfaceImpl(const foundation::PlatformInterfacePtr &platform, const core::SceneInterfacePtr &scene) {}
        ~RaycastInterfaceImpl() override {}
        
    public:
        auto addShape(const util::Description &desc) -> ShapePtr override { return {}; }
        bool sphereCast(const math::vector3f &start, const math::vector3f &target, float radius, float length) const override { return false; }
        bool rayCast(const math::vector3f &start, const math::vector3f &target, float length) const override { return false; }

    private:

    };
}

namespace core {
    SphereShapeImpl::SphereShapeImpl() {
        
    }
    SphereShapeImpl::~SphereShapeImpl() {
        
    }
    void SphereShapeImpl::setTransform(const math::transform3f &trfm) {
        
    }
}

namespace core {
    BoxShapeImpl::BoxShapeImpl() {
        
    }
    BoxShapeImpl::~BoxShapeImpl() {
        
    }
    void BoxShapeImpl::setTransform(const math::transform3f &trfm) {
        
    }
}

namespace core {
    std::shared_ptr<RaycastInterface> RaycastInterface::instance(const foundation::PlatformInterfacePtr &platform, const core::SceneInterfacePtr &scene) {
        return std::make_shared<RaycastInterfaceImpl>(platform, scene);
    }
}

