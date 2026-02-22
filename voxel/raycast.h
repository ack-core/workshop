
#pragma once
#include <memory>
#include "foundation/math.h"
#include "foundation/platform.h"
#include "foundation/util.h"

namespace voxel {
    class RaycastInterface {
    public:
        enum class ShapeType {
            SPHERES = 1,
            BOXES,
            TRIANGLES
        };
        
    public:
        static std::shared_ptr<RaycastInterface> instance(
          const foundation::PlatformInterfacePtr &platform
      );
        
    public:
        struct Mesh {
            virtual void setTranform(const math::transform3f &trfm) = 0;
            virtual ~Mesh() = default;
        };
        
        using MeshPtr = std::shared_ptr<Mesh>;
        
    public:
        virtual auto addMesh(const util::Description &desc) -> MeshPtr = 0;
        virtual bool rayCast(const math::vector3f &start, const math::vector3f &target, float length) const = 0;
        virtual bool sphereCast(const math::vector3f &start, const math::vector3f &target, float radius, float length) const = 0;
        
    public:
        virtual ~RaycastInterface() = default;
    };
    
    using RaycastInterfacePtr = std::shared_ptr<RaycastInterface>;
}

