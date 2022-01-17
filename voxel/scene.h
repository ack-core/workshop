
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/gears/math.h"

#include "mesh_factory.h"

namespace voxel {
    using SceneObjectToken = void *;
    
    class Scene {
    public:
        static std::shared_ptr<Scene> instance(const voxel::MeshFactoryPtr &factory, const foundation::RenderingInterfacePtr &rendering, const char *palette);

    public:
        SceneObjectToken addStaticModel(const char *voxPath, const int16_t(&offset)[3]);
        SceneObjectToken addLightSource(const math::vector3f &position, float intensivity, const math::color &rgba);

        void updateAndDraw(float dtSec);
        
    protected:
        virtual ~Scene() = default;
    };

    using ScenePtr = std::shared_ptr<Scene>;
}

