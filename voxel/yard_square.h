
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

namespace voxel {
    class YardSquare : public YardStatic {
    public:
        YardSquare(const YardFacility &facility, const math::bound3f &bbox, std::string &&texture, std::string &&heightmap);
        ~YardSquare() override;
        
    public:
        void setState(State newState) override;
        
    private:
        static void _makeIndices(std::vector<voxel::SceneInterface::VTXNRMUV> &points, std::vector<std::uint32_t> &indices);
        static void _makeGeometry(const std::unique_ptr<std::uint8_t[]> &hm, const math::bound3f &bbox, std::vector<SceneInterface::VTXNRMUV> &ov, std::vector<std::uint32_t> &oi);
            
    private:
        const std::string _texturePath;
        const std::string _heightmapPath;
        
        foundation::RenderTexturePtr _texture;
        std::unique_ptr<std::uint8_t[]> _heightmap;
        
        SceneInterface::TexturedModelPtr _model;
        SceneInterface::BoundingBoxPtr _bboxmdl;
    };
}

