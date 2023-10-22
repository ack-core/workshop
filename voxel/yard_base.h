
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
    enum class YardLoadingState {
        NONE = 0,
        LOADING,
        LOADED,
        RENDERING,
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
    
    class YardStatic {
    public:
        YardStatic(const YardFacility &facility, std::uint64_t id, const math::bound3f &bbox);
        virtual ~YardStatic() = default;    
        virtual void updateState(YardLoadingState targetState) = 0;
        
        auto getBBox() const -> math::bound3f;
        auto getLinks() const -> const std::vector<YardStatic *> &;
        void linkTo(YardStatic *object);
        
    protected:
        const YardFacility &_facility;
        const std::uint64_t _id;
        const math::bound3f _bbox;

        std::vector<YardStatic *> _links;
        YardLoadingState _currentState;
    };
}

