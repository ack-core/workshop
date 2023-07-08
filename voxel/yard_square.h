
#pragma once
#include "foundation/math.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace voxel {
    class YardSquare : public YardStatic, public std::enable_shared_from_this<YardSquare> {
    public:
        YardSquare(const YardFacility &facility, const math::bound3f &bbox, std::string &&texture, std::string &&heightmap);
        ~YardSquare() override;
        
    public:
        void updateState(YardStatic::State targetState) override;
        
    private:
        static void _makeIndices(std::vector<voxel::SceneInterface::VTXNRMUV> &points, std::vector<std::uint32_t> &indices);
        static void _makeGeometry(const std::unique_ptr<std::uint8_t[]> &hm, const math::bound3f &bbox, std::vector<SceneInterface::VTXNRMUV> &ov, std::vector<std::uint32_t> &oi);
            
    private:
        const std::string _texturePath;
        const std::string _hmPath;
        
        foundation::RenderTexturePtr _texture;
        std::unique_ptr<std::uint8_t[]> _heightmap;
        
        SceneInterface::TexturedModelPtr _model;
        SceneInterface::BoundingBoxPtr _bboxmdl;
    };
}

