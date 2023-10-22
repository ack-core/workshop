
#pragma once
#include "foundation/math.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace voxel {
    class YardSteadImpl : public YardStatic, public std::enable_shared_from_this<YardSteadImpl>, public YardInterface::Stead {
    public:
        YardSteadImpl(const YardFacility &facility, std::uint64_t id, const math::bound3f &bbox, std::string &&texture, std::string &&heightmap);
        ~YardSteadImpl() override;
        
    public:
        void setPosition(const math::vector3f &position) override;
        auto getPosition() const -> const math::vector3f & override;
        auto getId() const -> std::uint64_t override;
    
        void updateState(YardLoadingState targetState) override;
        
    private:
        static void _makeIndices(std::vector<voxel::SceneInterface::VTXNRMUV> &points, std::vector<std::uint32_t> &indices);
        static void _makeGeometry(const std::unique_ptr<std::uint8_t[]> &hm, const math::bound3f &bbox, std::vector<SceneInterface::VTXNRMUV> &ov, std::vector<std::uint32_t> &oi);
            
    private:
        const std::string _texturePath;
        const std::string _hmPath;
        
        math::vector3f _position = {0, 0, 0};
        foundation::RenderTexturePtr _texture;
        std::unique_ptr<std::uint8_t[]> _heightmap;
        std::vector<SceneInterface::VTXNRMUV> _vertices;
        std::vector<std::uint32_t> _indices;
        
        SceneInterface::TexturedModelPtr _model;
        SceneInterface::BoundingBoxPtr _bboxmdl;
    };
}

