
#define _CRT_SECURE_NO_WARNINGS

#include "interfaces.h"
#include "environment.h"

#include "foundation/parsing.h"

#include "thirdparty/upng/upng.h"

#include <sstream>
#include <iomanip>

namespace {
    const char *g_staticMeshShader = R"(
        fixed {
            cube[12] : float4 = 
                [-0.5, 0.5, 0.5, 1.0]
                [-0.5, -0.5, 0.5, 1.0]
                [0.5, 0.5, 0.5, 1.0]
                [0.5, -0.5, 0.5, 1.0]
                [0.5, -0.5, 0.5, 1.0]
                [0.5, 0.5, 0.5, 1.0]
                [0.5, -0.5, -0.5, 1.0]
                [0.5, 0.5, -0.5, 1.0]
                [0.5, 0.5, -0.5, 1.0]
                [0.5, 0.5, 0.5, 1.0]
                [-0.5, 0.5, -0.5, 1.0]
                [-0.5, 0.5, 0.5, 1.0]
        }
        inout {
            texcoord : float2
        }
        vssrc {
            float4 center = float4(instance_position.xyz, 1.0);
            float3 camSign = _sign(_cameraPosition.xyz - instance_position.xyz);
            float4 cube_position = float4(camSign, 0.0) * cube[vertex_ID] + center; //
            output_position = _transform(cube_position, _viewProjMatrix);
            output_texcoord = float2(instance_scale_color.w / 255.0, 0);
        }
        fssrc {
            output_color = _tex2d(0, input_texcoord);
        }
    )";
}

namespace voxel {
    TiledWorldImpl::TiledWorldImpl(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering, const std::shared_ptr<voxel::MeshFactory> &factory)
        : _platform(platform)
        , _rendering(rendering)
        , _factory(factory)
    {
        _shader = rendering->createShader("static_voxel_mesh", g_staticMeshShader,
            // vertex
            {
                {"ID", foundation::RenderingShaderInputFormat::VERTEX_ID}
            },
            // instance
            {
                {"position", foundation::RenderingShaderInputFormat::SHORT4},
                {"scale_color", foundation::RenderingShaderInputFormat::BYTE4}
            }
        );
    }

    TiledWorldImpl::~TiledWorldImpl() {

    }

    void TiledWorldImpl::clear() {

    }

    void TiledWorldImpl::loadSpace(const char *spaceDirectory) {
        struct NeighborOffset {
            int i;
            int c;
        };

        struct NeighborParams {
            union Mask {
                char bytes[4];
                std::uint32_t dword;
            };

            std::uint8_t neighborType;
            voxel::MeshFactory::Rotation rotation;
            Mask majorMask = { '0', '0', '0', '0' };
            std::uint32_t majorZero = 0;
            Mask minorMask = { '0', '0', '0', '0' };
            std::uint32_t minorZero = 0;
        };

        auto getIndexFromColor = [&](std::uint32_t color) {
            std::uint8_t index = NONEXIST_TILE;

            for (std::uint8_t i = 0; i < std::uint8_t(_tileTypes.size()); i++) {
                if (_tileTypes[i].color == color) {
                    index = i;
                }
            }

            return index;
        };

        auto getNeighborMin = [&](Chunk &chunk, int i, int c) {
            NeighborOffset offsets[] = {
                {-1, +1},
                {-1,  0},
                {-1, -1},
                { 0, -1},
                { 0, +1},
                {+1,  0},
                {+1, -1},
                {+1, +1},
            };

            std::uint8_t result = chunk.mapSource[i][c];

            for (int k = 0; k < 8; k++) {
                std::uint8_t neighbor = chunk.mapSource[i + offsets[k].i][c + offsets[k].c];
                
                if (neighbor < result) {
                    result = neighbor;
                }
            }

            return result;
        };

        auto getNeighborParams = [&](Chunk &chunk, int i, int c) {
            NeighborParams result {};
            NeighborOffset majorOffsets[] = {
                { 0, -1},
                {+1,  0},
                { 0, +1},
                {-1,  0},
            };
            NeighborOffset minorOffsets[] = {
                {+1, -1},
                {+1, +1},
                {-1, +1},
                {-1, -1},
            };

            result.neighborType = chunk.mapSource[i][c];

            for (int k = 0; k < 4; k++) {
                std::uint8_t major = chunk.mapSource[i + majorOffsets[k].i][c + majorOffsets[k].c];
                std::uint8_t minor = chunk.mapSource[i + minorOffsets[k].i][c + minorOffsets[k].c];

                if (major == NONEXIST_TILE) {
                    major = getNeighborMin(chunk, i + majorOffsets[k].i, c + majorOffsets[k].c);
                }
                if (minor == NONEXIST_TILE) {
                    minor = getNeighborMin(chunk, i + minorOffsets[k].i, c + minorOffsets[k].c);
                }

                if (major > chunk.mapSource[i][c]) {
                    result.neighborType = major;
                    result.majorMask.bytes[k] = '1';
                }
                if (minor > chunk.mapSource[i][c]) {
                    result.neighborType = minor;
                    result.minorMask.bytes[k] = '1';
                }
            }

            std::uint32_t major = result.majorMask.dword;
            std::uint32_t minor = result.minorMask.dword;

            for (int k = 0; k < 3; k++) {
                std::uint32_t nextMajor = (major << 8) | (major >> 24);
                std::uint32_t nextMinor = (minor << 8) | (minor >> 24);

                bool isMajor = result.majorMask.dword != *(std::uint32_t *)"0000";

                if ((isMajor && nextMajor > result.majorMask.dword) || (!isMajor && nextMinor > result.minorMask.dword)) {
                    result.majorMask.dword = nextMajor;
                    result.minorMask.dword = nextMinor;
                    result.rotation = voxel::MeshFactory::Rotation(k + 1);
                }

                major = nextMajor;
                minor = nextMinor;
            }

            if (result.majorMask.dword == *(std::uint32_t *)"0001") {
                result.minorMask.bytes[3] = 'x';
                result.minorMask.bytes[2] = 'x';
            }
            if (result.majorMask.dword == *(std::uint32_t *)"0011") {
                result.minorMask.bytes[3] = 'x';
                result.minorMask.bytes[2] = 'x';
                result.minorMask.bytes[1] = 'x';
            }
            if (result.majorMask.dword == *(std::uint32_t *)"1111" || result.majorMask.dword == *(std::uint32_t *)"0111" || result.majorMask.dword == *(std::uint32_t *)"0101") {
                result.minorMask.bytes[3] = 'x';
                result.minorMask.bytes[2] = 'x';
                result.minorMask.bytes[1] = 'x';
                result.minorMask.bytes[0] = 'x';
            }

            result.majorMask.dword = (result.majorMask.dword << 24) | ((result.majorMask.dword & 0xff00) << 8) | ((result.majorMask.dword & 0xff0000) >> 8) | (result.majorMask.dword >> 24);
            result.minorMask.dword = (result.minorMask.dword << 24) | ((result.minorMask.dword & 0xff00) << 8) | ((result.minorMask.dword & 0xff0000) >> 8) | (result.minorMask.dword >> 24);

            return result;
        };

        auto getModelNameAndRotation = [&](Chunk &chunk, int i, int c, char *name, voxel::MeshFactory::Rotation &rotation) {
            std::uint8_t selfType = chunk.mapSource[i][c];
            std::string &selfName = _tileTypes[selfType].name;

            NeighborParams params = getNeighborParams(chunk, i, c);

            if (selfType != params.neighborType) {
                ::sprintf(name, "%s_%s_%s_%s.vox", _tileTypes[selfType].name.data(), _tileTypes[params.neighborType].name.data(), params.majorMask.bytes, params.minorMask.bytes);
            }
            else {
                ::sprintf(name, "%s.vox", selfName.data());
            }

            rotation = params.rotation;
            return true;
        };

        std::string infoPath = std::string(spaceDirectory) + "/space.info";
        std::string palettePath = std::string(spaceDirectory) + "/palette.png";
        std::string mapPath = std::string(spaceDirectory) + "/map.png";

        std::unique_ptr<uint8_t[]> infoData;
        std::size_t infoSize;

        if (_platform->loadFile(infoPath.data(), infoData, infoSize)) {
            std::istringstream stream(std::string(reinterpret_cast<const char *>(infoData.get()), infoSize));
            std::string keyword;

            // info

            while (stream >> keyword) {
                if (keyword == "tilesize") {
                    if (bool(stream >> gears::expect<'='> >> _tileSize) != true) {
                        _platform->logError("[TiledWorld::loadSpace] tilesize invalid value at '%s'", infoPath.data());
                        break;
                    }
                }
                else if (keyword == "tileset") {
                    unsigned count = 0;

                    if (stream >> gears::expect<'='> >> count) {
                        std::string name;
                        std::uint32_t r, g, b;

                        for (unsigned i = 0; i < count; i++) {
                            if (stream >> name >> gears::expect<'='> >> r >> g >> b) {
                                _tileTypes.emplace_back(TileType{ std::move(name), std::uint32_t(0xFF000000) | (b << 16) | (g << 8) | r });
                            }
                            else {
                                _platform->logError("[TiledWorld::loadSpace] tile type '%s' has invalid value at '%s'", name.data(), infoPath.data());
                                break;
                            }
                        }

                        if (count == 0) {
                            _platform->logError("[TiledWorld::loadSpace] no tile types loaded from '%s'", infoPath.data());
                            break;
                        }
                    }
                    else {
                        _platform->logError("[TiledWorld::loadSpace] tileset invalid value at '%s'", infoPath.data());
                        break;
                    }
                }
                else {
                    _platform->logError("[TiledWorld::loadSpace] Unreconized keyword '%s' in '%s'", keyword.data(), infoPath.data());
                    break;
                }
            }

            // palette

            std::unique_ptr<std::uint8_t[]> paletteData;
            std::size_t paletteSize;

            if (_platform->loadFile(palettePath.data(), paletteData, paletteSize)) {
                upng_t *upng = upng_new_from_bytes(paletteData.get(), unsigned long(paletteSize));

                if (upng != nullptr) {
                    if (*reinterpret_cast<const unsigned *>(paletteData.get()) == UPNG_HEAD && upng_decode(upng) == UPNG_EOK) {
                        if (upng_get_format(upng) == UPNG_RGBA8 && upng_get_width(upng) == 256 && upng_get_height(upng) == 1) {
                            _palette = _rendering->createTexture(foundation::RenderingTextureFormat::RGBA8UN, 256, 1, { upng_get_buffer(upng) });
                        }
                        else {
                            _platform->logError("[TiledWorld::loadSpace] '%s' is not 256x1 RGBA png file", palettePath.data());
                        }
                    }
                    else {
                        _platform->logError("[TiledWorld::loadSpace] '%s' is not a valid png file", palettePath.data());
                    }

                    upng_free(upng);
                }
            }
            else {
                _platform->logError("[TiledWorld::loadSpace] '%s' not found", palettePath.data());
            }

            // map

            std::unique_ptr<std::uint8_t[]> mapData;
            std::size_t mapSize;

            if (_platform->loadFile(mapPath.data(), mapData, mapSize)) {
                upng_t *upng = upng_new_from_bytes(mapData.get(), unsigned long(mapSize));

                if (upng != nullptr) {
                    if (*reinterpret_cast<const unsigned *>(paletteData.get()) == UPNG_HEAD && upng_decode(upng) == UPNG_EOK) {
                        if (upng_get_format(upng) == UPNG_RGBA8) {
                            if (upng_get_width(upng) == upng_get_height(upng)) {
                                const std::uint32_t *pngmem = reinterpret_cast<const std::uint32_t *>(upng_get_buffer(upng));

                                _chunk.drawSize = upng_get_width(upng);
                                _chunk.mapSize = _chunk.drawSize + 2;
                                _chunk.mapSource = new uint8_t * [_chunk.mapSize];

                                for (std::size_t i = 0; i < _chunk.mapSize; i++) {
                                    _chunk.mapSource[i] = new uint8_t [_chunk.mapSize];
                                    ::memset(_chunk.mapSource[i], 0, _chunk.mapSize);
                                }

                                for (std::size_t i = 0; i < _chunk.drawSize; i++) {
                                    for (std::size_t c = 0; c < _chunk.drawSize; c++) {
                                        _chunk.mapSource[i + 1][c + 1] = getIndexFromColor(pngmem[i * _chunk.drawSize + c]);
                                    }
                                }

                                _chunk.mapSource[0][0] = _chunk.mapSource[1][1];
                                _chunk.mapSource[_chunk.mapSize - 1][0] = _chunk.mapSource[_chunk.mapSize - 2][1];
                                _chunk.mapSource[0][_chunk.mapSize - 1] = _chunk.mapSource[1][_chunk.mapSize - 2];
                                _chunk.mapSource[_chunk.mapSize - 1][_chunk.mapSize - 1] = _chunk.mapSource[_chunk.mapSize - 2][_chunk.mapSize - 2];

                                for (std::size_t i = 1; i < _chunk.mapSize - 1; i++) {
                                    _chunk.mapSource[0][i] = _chunk.mapSource[1][i];
                                    _chunk.mapSource[_chunk.mapSize - 1][i] = _chunk.mapSource[_chunk.mapSize - 2][i];
                                    _chunk.mapSource[i][0] = _chunk.mapSource[i][1];
                                    _chunk.mapSource[i][_chunk.mapSize - 1] = _chunk.mapSource[i][_chunk.mapSize - 2];
                                }
                            }
                            else {
                                _platform->logError("[TiledWorld::loadSpace] '%s' has inconsistent size", mapPath.data());
                            }
                        }
                        else {
                            _platform->logError("[TiledWorld::loadSpace] '%s' has invalid format", mapPath.data());
                        }
                    }
                    else {
                        _platform->logError("[TiledWorld::loadSpace] '%s' is not a valid png file", mapPath.data());
                    }

                    upng_free(upng);
                }
            }
            else {
                _platform->logError("[TiledWorld::loadSpace] '%s' not found", mapPath.data());
            }
        }

        if (_chunk.mapSource) {
            // validation

            for (size_t i = 0; i < _chunk.mapSize; i++) {
                for (size_t c = 0; c < _chunk.mapSize; c++) {

                }
            }

            // baking

            char model[256];
            voxel::MeshFactory::Rotation rotation;
            _chunk.voxels.clear();

            int offset = ::sprintf(model, "%s/", spaceDirectory);

            for (int i = 0; i < _chunk.drawSize; i++) {
                for (int c = 0; c < _chunk.drawSize; c++) {
                    if (_chunk.mapSource[i + 1][c + 1] != NONEXIST_TILE) {
                        if (getModelNameAndRotation(_chunk, i + 1, c + 1, model + offset, rotation)) {
                            int offx = (c - int(_chunk.drawSize / 2)) * _tileSize;
                            int offz = (i - int(_chunk.drawSize / 2)) * _tileSize;

                            if (_factory->loadVoxels(model, offx, 0, offz, rotation, _chunk.voxels) == false) {
                                _platform->logError("[TiledWorld::loadSpace] voxels from '%s' aren't loaded", model);
                            }
                        }
                    }
                }
            }

            _chunk.geometry = _factory->createStaticMesh(_chunk.voxels);
        }
    }

    void TiledWorldImpl::updateAndDraw(float dtSec) {
        _rendering->applyTextures({ _palette.get() });
        _rendering->applyShader(_shader);
        _rendering->drawGeometry(nullptr, _chunk.geometry->getGeometry(), 0, 12, 0, _chunk.geometry->getGeometry()->getCount(), foundation::RenderingTopology::TRIANGLESTRIP);
    }
}

namespace voxel {
    std::shared_ptr<TiledWorld> TiledWorld::instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering, const std::shared_ptr<voxel::MeshFactory> &factory) {
        return std::make_shared<TiledWorldImpl>(platform, rendering, factory);
    }
}