
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "foundation/platform.h"
#include "foundation/math.h"

namespace voxel {
    using YardObjectToken = std::uint64_t;

    class YardInterface {
    public:
        static std::shared_ptr<YardInterface> instance(const foundation::PlatformInterfacePtr &platform);
        
    public:
        // Yard source format:
        // s--------------------------------------
        // object <index> <frameCount> {
        //     <property> : <args>
        //     ...
        // }
        // object <index> <frameCount> {
        //     <property> : <args>
        //     ...
        // }
        // ...
        // s--------------------------------------
        // <index> - 
        virtual bool loadYard(const char *src) = 0;
        virtual void setObservingPosition(const math::vector3f &position) = 0;
        virtual void setObjectFrame(YardObjectToken obj, std::size_t frameIndex) = 0;
        
    public:
        virtual ~YardInterface() = default;
    };
    
    using YardInterfacePtr = std::shared_ptr<YardInterface>;
}

