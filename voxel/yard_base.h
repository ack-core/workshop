
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/platform.h"
#include "foundation/math.h"

#include "voxel/mesh_provider.h"
#include "voxel/texture_provider.h"
#include "voxel/scene.h"

namespace voxel {
    class YardInterfaceProvider {
    public:
        virtual const foundation::LoggerInterfacePtr &getLogger() const = 0;
        virtual const MeshProviderPtr &getMeshProvider() const = 0;
        virtual const TextureProviderPtr &getTextureProvider() const = 0;
        virtual const SceneInterfacePtr &getScene() const = 0;
        
    public:
        virtual ~YardInterfaceProvider() = default;
    };
    
    class YardStatic {
    public:
        enum class State {
            NONE = 0,
            NEARBY,
            RENDERED,
        };
        
    public:
        virtual void setState(State state) = 0;
        virtual auto getState() const -> State = 0;
        virtual auto getBBox() const -> math::bound3f = 0;

    public:
        virtual ~YardStatic() = default;
    };
}

