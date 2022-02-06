
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/gears/math.h"

#include "mesh_factory.h"

namespace voxel {
    using SceneObjectToken = std::uint64_t;
    static const SceneObjectToken INVALID_TOKEN = std::uint64_t(-1);
    
    class Scene {
    public:
        static std::shared_ptr<Scene> instance(const voxel::MeshFactoryPtr &factory, const foundation::RenderingInterfacePtr &rendering, const char *palette);

    public:
        virtual SceneObjectToken addStaticModel(const char *voxPath, const int16_t(&offset)[3]) = 0;
        virtual SceneObjectToken addLightSource(const math::vector3f &position, float radius, const math::color &rgba) = 0;
        
        virtual void removeStaticModel(SceneObjectToken token) = 0;
        virtual void removeLightSource(SceneObjectToken token) = 0;
        
        virtual void updateAndDraw(float dtSec) = 0;
        
    protected:
        virtual ~Scene() = default;
    };

    using ScenePtr = std::shared_ptr<Scene>;
}

