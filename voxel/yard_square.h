
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "voxel/scene.h"

namespace voxel {
    class YardSquare {
    public:
        YardSquare(const SceneInterfacePtr &scene, int x, int z, int w, int h, std::unique_ptr<std::uint8_t[]> &&hm, const foundation::RenderTexturePtr &tx);
        ~YardSquare();
        
    public:
        const foundation::RenderTexturePtr _texture;
        const std::unique_ptr<std::uint8_t[]> _heightmap;
        SceneInterface::TexturedModelPtr _model;
        std::uint32_t _width;
        std::uint32_t _height;
    };
}

