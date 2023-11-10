
#pragma once
#include "foundation/math.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace voxel {
    class YardSteadImpl : public YardStatic, public std::enable_shared_from_this<YardSteadImpl>, public YardInterface::Stead {
    public:
        // @source - stead encoded to 32-bit png:
        //     R : color index in palette
        //     G : flags [128 - is vertex, 64 - wall point, 32 - wall direction, 16..1 - reserved]
        //     B : heightmap
        //     A : reserved
        //
        YardSteadImpl(const YardFacility &facility, std::uint64_t id, const math::vector3f &position, const math::bound3f &bbox, std::string &&source);
        ~YardSteadImpl() override;
        
    public:
        void setPosition(const math::vector3f &position) override;
        auto getPosition() const -> const math::vector3f & override;
        auto getId() const -> std::uint64_t override;
    
        void updateState(YardLoadingState targetState) override;
        
    private:
        static void _makeIndices(const std::vector<voxel::SceneInterface::VTXNRMUV> &points, std::vector<std::uint32_t> &indices);
            
    private:
        const std::string _sourcePath;
        
        math::vector3f _position = {0, 0, 0};
        std::vector<SceneInterface::VTXNRMUV> _vertices;
        std::vector<std::uint32_t> _indices;
        
        foundation::RenderTexturePtr _texture;
        SceneInterface::TexturedModelPtr _model;
        SceneInterface::BoundingBoxPtr _bboxmdl;
    };
}

