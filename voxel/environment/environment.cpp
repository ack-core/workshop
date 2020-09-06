
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
            axis[3] : float4 = 
                [0.0, 0.0, 1.0, 0.0]
                [1.0, 0.0, 0.0, 0.0]
                [0.0, 1.0, 0.0, 0.0]
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
            normal : float3
        }
        vssrc {
            float4 center = float4(instance_position.xyz, 1.0);
            float3 camSign = _sign(_cameraPosition.xyz - instance_position.xyz);
            float4 cube_position = float4(camSign, 0.0) * cube[vertex_ID] + center; //
            output_position = _transform(cube_position, _viewProjMatrix);
            output_texcoord = float2(instance_scale_color.w / 255.0, 0);
            output_normal = camSign * axis[vertex_ID >> 2];
        }
        fssrc {
            float light = 0.7 + 0.3 * _dot(input_normal, _norm(float3(0.1, 2.0, 0.3)));
            output_color = float4(_tex2d(0, input_texcoord).xyz * light * light, 1.0);
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
        clear();
    }

    void TiledWorldImpl::clear() {
        if (_chunk.mapSource) {
            for (std::size_t i = 0; i < _chunk.mapSize; i++) {
                delete[] _chunk.mapSource[i];
            }

            delete[] _chunk.mapSource;
        }

        if (_chunk.geometry) {
            for (std::size_t i = 0; i < _chunk.drawSize; i++) {
                delete[] _chunk.geometry[i];
            }

            delete[] _chunk.geometry;
        }

        _chunk = {};
    }

    void TiledWorldImpl::loadSpace(const char *spaceDirectory) {
        _spaceDirectory = spaceDirectory;

        auto getIndexFromColor = [&](std::uint32_t color) {
            std::uint8_t index = NONEXIST_TILE;

            for (std::uint8_t i = 0; i < std::uint8_t(_tileTypes.size()); i++) {
                if (_tileTypes[i].color == color) {
                    index = i;
                }
            }

            return index;
        };

        std::string infoPath = _spaceDirectory + "/space.info";
        std::string palettePath = _spaceDirectory + "/palette.png";
        std::string mapPath = _spaceDirectory + "/map.png";

        std::unique_ptr<uint8_t[]> infoData;
        std::size_t infoSize;

        if (_platform->loadFile(infoPath.data(), infoData, infoSize)) {
            std::istringstream stream(std::string(reinterpret_cast<const char *>(infoData.get()), infoSize));
            std::string keyword;

            // info

            while (stream >> keyword) {
                if (keyword == "tileSize") {
                    if (bool(stream >> gears::expect<'='> >> _tileSize) != true) {
                        _platform->logError("[TiledWorld::loadSpace] tileSize has invalid value at '%s'", infoPath.data());
                        break;
                    }
                }
                else if (keyword == "tileTypeCount") {
                    unsigned count = 0;

                    if (stream >> gears::expect<'='> >> count) {
                        std::string name, options;
                        std::uint32_t r, g, b;

                        for (unsigned i = 0; i < count; i++) {
                            if (stream >> name >> gears::expect<'='> >> r >> g >> b >> options) {
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
                        _platform->logError("[TiledWorld::loadSpace] tileTypeCount has invalid value at '%s'", infoPath.data());
                        break;
                    }
                }
                else if (keyword == "helperTypeCount") {
                    unsigned typeCount = 0;

                    if (stream >> gears::expect<'='> >> typeCount) {
                        std::string name;
                        unsigned helperCount = 0;

                        for (unsigned i = 0; i < typeCount; i++) {
                            if (stream >> name >> gears::expect<'='> >> helperCount) {
                                std::vector<Helper> helpers;

                                for (unsigned c = 0; c < helperCount; c++) {
                                    std::string options;
                                    TileOffset offset;

                                    if (stream >> options >> offset.h >> offset.v) {
                                        math::vector3f helperPosition = getTileCenterPosition(offset);

                                        helpers.emplace_back(Helper{ offset, helperPosition });
                                    }
                                    else {
                                        _platform->logError("[TiledWorld::loadSpace] helper '%s' %u-th coordinates has invalid value at '%s'", name.data(), c, infoPath.data());
                                        i = unsigned(-1);
                                        break;
                                    }
                                }

                                _helpers.emplace(name, std::move(helpers));
                            }
                            else {
                                _platform->logError("[TiledWorld::loadSpace] helper type '%s' has invalid value at '%s'", name.data(), infoPath.data());
                                break;
                            }
                        }
                    }
                    else {
                        _platform->logError("[TiledWorld::loadSpace] helperTypeCount has invalid value at '%s'", infoPath.data());
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
        else {
            _platform->logError("[TiledWorld::loadSpace] '%s' not found", infoPath.data());
        }

        if (_chunk.mapSource) {
            // validation
            // TODO: 

            for (size_t i = 0; i < _chunk.mapSize; i++) {
                for (size_t c = 0; c < _chunk.mapSize; c++) {

                }
            }

            // geometry & baking

            _chunk.geometry = new std::shared_ptr<voxel::StaticMesh> * [_chunk.drawSize];

            for (int i = 0; i < _chunk.drawSize; i++) {
                _chunk.geometry[i] = new std::shared_ptr<voxel::StaticMesh> [_chunk.drawSize];

                for (int c = 0; c < _chunk.drawSize; c++) {
                    if (_chunk.mapSource[i + 1][c + 1] != NONEXIST_TILE) {
                        _updateTileGeometry(TileOffset{ c, i });
                    }
                }
            }
        }
    }

    bool TiledWorldImpl::isTileTypeSuitable(const TileOffset &offset, std::uint8_t typeIndex) {
        if (_isInChunk(_chunk, offset) == false) {
            _platform->logError("[TiledWorld::setTileTypeIndex] no tile at {%d;%d}", offset.h, offset.v);
            return false;
        }

        for (int i = offset.v; i <= offset.v + 2; i++) {
            for (int c = offset.h; c <= offset.h + 2; c++) {
                int diff = std::abs(int(_chunk.mapSource[i][c]) - int(typeIndex));

                if (diff > 1) {
                    _platform->logError("[TiledWorld::setTileTypeIndex] cannot change tile at {%d;%d}", offset.h, offset.v);
                    return false;
                }
            }
        }

        return true;
    }

    bool TiledWorldImpl::setTileTypeIndex(const TileOffset &offset, std::uint8_t typeIndex, const char *postfix) {
        if (isTileTypeSuitable(offset, typeIndex)) {
            _chunk.mapSource[offset.v + 1][offset.h + 1] = typeIndex;

            for (int v = offset.v - 1; v <= offset.v + 1; v++) {
                for (int h = offset.h - 1; h <= offset.h + 1; h++) {
                    if (_isInChunk(_chunk, TileOffset{h, v})) {
                        _updateTileGeometry(TileOffset{h, v});
                    }
                }
            }

            return true;
        }

        return false;
    }

    std::uint8_t TiledWorldImpl::getTileTypeIndex(const TileOffset &offset) const {
        std::uint8_t result = NONEXIST_TILE;

        if (_isInChunk(_chunk, offset)) {
            result = _chunk.mapSource[offset.v + 1][offset.h + 1];
        }

        return result;
    }

    math::vector3f TiledWorldImpl::getTileCenterPosition(const TileOffset &offset) const {
        float offx = float((offset.h - int(_chunk.drawSize / 2)) * _tileSize) + _tileSize * 0.5f;
        float offz = float((offset.v - int(_chunk.drawSize / 2)) * _tileSize) + _tileSize * 0.5f;
    
        return {offx, 0, offz};
    }

    std::size_t TiledWorldImpl::getHelperCount(const char *name) const {
        auto it = _helpers.find(name);
        if (it != _helpers.end()) {
            return it->second.size();
        }

        return 0;
    }

    bool TiledWorldImpl::getHelperPosition(const char *name, std::size_t index, math::vector3f &out) const {
        auto it = _helpers.find(name);
        if (it != _helpers.end()) {
            if (index < it->second.size()) {
                out = it->second[index].position;
                return true;
            }
        }

        return false;
    }

    bool TiledWorldImpl::getHelperTileOffset(const char *name, std::size_t index, TileOffset &out) const {
        auto it = _helpers.find(name);
        if (it != _helpers.end()) {
            if (index < it->second.size()) {
                out = it->second[index].offset;
                return true;
            }
        }

        return false;
    }

    void TiledWorldImpl::updateAndDraw(float dtSec) {
        _rendering->applyTextures({ _palette.get() });
        _rendering->applyShader(_shader);

        for (int i = 0; i < _chunk.drawSize; i++) {
            for (int c = 0; c < _chunk.drawSize; c++) {
                _rendering->drawGeometry(nullptr, _chunk.geometry[i][c]->getGeometry(), 0, 12, 0, _chunk.geometry[i][c]->getGeometry()->getCount(), foundation::RenderingTopology::TRIANGLESTRIP);
            }
        }
    }

    bool TiledWorldImpl::_isInChunk(const Chunk &chunk, const TileOffset &offset) const {
        if (offset.h >= 0 && offset.h < chunk.drawSize) {
            if (offset.v >= 0 && offset.v < chunk.drawSize) {
                return true;
            }
        }

        return false;
    }

    void TiledWorldImpl::_updateTileGeometry(const TileOffset &offset) {
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
        auto getNeighborMin = [&](Chunk &chunk, int i, int c) {
            static NeighborOffset offsets[] = {
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
            NeighborParams result{};
            static NeighborOffset majorOffsets[] = {
                { 0, -1},
                {+1,  0},
                { 0, +1},
                {-1,  0},
            };
            static NeighborOffset minorOffsets[] = {
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
                rotation = params.rotation;
            }
            else {
                ::sprintf(name, "%s.vox", selfName.data());
                rotation = voxel::MeshFactory::Rotation(rand() % 4);
            }

            return true;
        };

        char model[256];
        voxel::MeshFactory::Rotation rotation;

        int pathStart = ::sprintf(model, "%s/", _spaceDirectory.data());
        if (getModelNameAndRotation(_chunk, offset.v + 1, offset.h + 1, model + pathStart, rotation)) {
            int offx = (offset.h - int(_chunk.drawSize / 2)) * _tileSize;
            int offz = (offset.v - int(_chunk.drawSize / 2)) * _tileSize;

            std::vector<voxel::Voxel> voxels;

            if (_factory->loadVoxels(model, offx, 0, offz, rotation, voxels)) {
                _chunk.geometry[offset.v][offset.h] = _factory->createStaticMesh(voxels);
            }
            else {
                _platform->logError("[TiledWorld::loadSpace] voxels from '%s' aren't loaded", model);
            }
        }
    }
}

namespace voxel {
    std::shared_ptr<TiledWorld> TiledWorld::instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<foundation::RenderingInterface> &rendering, const std::shared_ptr<voxel::MeshFactory> &factory) {
        return std::make_shared<TiledWorldImpl>(platform, rendering, factory);
    }
}