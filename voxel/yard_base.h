
#pragma once
#include "foundation/platform.h"
#include "foundation/math.h"

#include "providers/mesh_provider.h"
#include "providers/texture_provider.h"

#include "voxel/scene.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace voxel {
    struct YardObjectType {
        std::string model;
        math::vector3f center;
    };
    
    class YardFacility {
    public:
        virtual const foundation::PlatformInterfacePtr &getPlatform() const = 0;
        virtual const foundation::RenderingInterfacePtr &getRendering() const = 0;
        virtual const resource::MeshProviderPtr &getMeshProvider() const = 0;
        virtual const resource::TextureProviderPtr &getTextureProvider() const = 0;
        virtual const SceneInterfacePtr &getScene() const = 0;
        virtual ~YardFacility() = default;
    };

    class YardCollision {
    public:
        virtual void correctMovement(const math::vector3f &position, math::vector3f &movement) const = 0;
        virtual ~YardCollision() = default;
    };
    
    class YardStatic {
    public:
        enum class State {
            NONE = 0,
            LOADING,
            LOADED,
            RENDERING,
        };
        
    public:
        YardStatic(const YardFacility &facility, const math::bound3f &bbox);
        virtual ~YardStatic() = default;    
        virtual void updateState(State targetState) = 0;
        
        auto getBBox() const -> math::bound3f;
        auto getLinks() const -> const std::vector<YardStatic *> &;
        void linkTo(YardStatic *object);

    protected:
        const YardFacility &_facility;
        const math::bound3f _bbox;

        std::vector<YardStatic *> _links;
        State _currentState;
    };
}

