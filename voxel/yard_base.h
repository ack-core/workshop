
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
        YardStatic(const YardInterfaceProvider &interfaces, const math::bound3f &bbox);
        virtual ~YardStatic() = default;
    
    public:
        virtual void setState(State state) = 0;
        
        auto getState() const -> State;
        auto getBBox() const -> math::bound3f;
        auto getLinks() const -> const std::vector<YardStatic *> &;
        void linkTo(YardStatic *object);

    protected:
        const YardInterfaceProvider &_interfaces;
        const math::bound3f _bbox;

        std::vector<YardStatic *> _links;
        State _currentState;
    };
}

