
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/math.h"

#include "mesh_factory.h"

namespace voxel {
    using SceneObjectToken = std::uint64_t;
    static const SceneObjectToken INVALID_TOKEN = std::uint64_t(-1);
    
    // Abilities:
    // + Static and dynamic models rendering
    // + Sun and shadows from it
    // + Local light source
    // + Camera management
    //
    class SceneInterface {
    public:
        static std::shared_ptr<SceneInterface> instance(const foundation::RenderingInterfacePtr &rendering, const voxel::MeshFactoryPtr &factory, const char *palette);
        
    public:
        virtual const std::shared_ptr<foundation::PlatformInterface> &getPlatformInterface() const = 0;
        virtual const std::shared_ptr<foundation::RenderingInterface> &getRenderingInterface() const = 0;
        
        virtual void setCameraLookAt(const math::vector3f &position, const math::vector3f &target) = 0;
        virtual void setObservingPoint(const math::vector3f &position) = 0;
        virtual void setSun(const math::vector3f &direction, const math::color &rgba) = 0;
        
        virtual SceneObjectToken addStaticModel(const char *voxPath, const int16_t(&offset)[3]) = 0;
        virtual SceneObjectToken addDynamicModel(const char *voxPath, const math::vector3f &position, float rotation) = 0;
        
        virtual void removeStaticModel(SceneObjectToken token) = 0;
        virtual void removeDynamicModel(SceneObjectToken token) = 0;
        
        virtual void updateAndDraw(float dtSec) = 0;
        
    public:
        virtual ~SceneInterface() = default;
    };
    
    using SceneInterfacePtr = std::shared_ptr<SceneInterface>;
}

