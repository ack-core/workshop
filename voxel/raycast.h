
#pragma once
#include <memory>
#include "foundation/math.h"
#include "foundation/platform.h"
#include "voxel/scene.h"

namespace voxel {
    class RaycastInterface {
    public:
        static std::shared_ptr<RaycastInterface> instance(
          const foundation::PlatformInterfacePtr &platform,
          const voxel::SceneInterfacePtr &scene
      );
        
    public:
        struct Mesh {
            virtual void setTranform(const math::transform3f &trfm) = 0;
            virtual ~Mesh() = default;
        };
        
        using MeshPtr = std::shared_ptr<Mesh>;
        
    public:
        virtual auto addSphereMesh(const std::vector<math::vector4f> &data) -> MeshPtr = 0;
        virtual auto addTriangleMesh(const std::vector<math::vector3f> &vtx, const std::vector<std::uint16_t> &idx) -> MeshPtr = 0;
        virtual bool sphereCast(const math::vector3f &start, const math::vector3f &target, float radius, float length) const = 0;
        virtual void update(float dtSec) = 0;
        
    public:
        virtual ~RaycastInterface() = default;
    };
    
    using RaycastInterfacePtr = std::shared_ptr<RaycastInterface>;
}

