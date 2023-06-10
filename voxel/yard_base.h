
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
    struct YardObjectType {
        std::string model;
        math::vector3f center;
    };
    
    class YardFacility {
    public:
        virtual const foundation::LoggerInterfacePtr &getLogger() const = 0;
        virtual const MeshProviderPtr &getMeshProvider() const = 0;
        virtual const TextureProviderPtr &getTextureProvider() const = 0;
        virtual const SceneInterfacePtr &getScene() const = 0;
        
    public:
        virtual ~YardFacility() = default;
    };

    class YardCollision {
    public:
        virtual void correctMovement(const math::vector3f &position, math::vector3f &movement) const = 0;
        
    public:
        virtual ~YardCollision() = default;
    };
    
    class YardStatic {
    public:
        enum class State {
            NONE = 0,
            NEARBY,
            RENDERED,
        };
        
    public:
        YardStatic(const YardFacility &facility, const math::bound3f &bbox);
        virtual ~YardStatic() = default;
    
    public:
        virtual void setState(State state) = 0;
        
        auto getState() const -> State;
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

