
#define _CRT_SECURE_NO_WARNINGS

#include "interfaces.h"
#include "environment.h"

#include "foundation/parsing.h"

#include "thirdparty/upng/upng.h"

#include <sstream>
#include <iomanip>
#include <queue>

namespace voxel {
    const TileLocation TiledWorld::NONEXIST_LOCATION = { std::numeric_limits<int>::min(), std::numeric_limits<int>::min() };
}

namespace voxel {
    TiledWorldImpl::TiledWorldImpl(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<voxel::MeshFactory> &factory, const std::shared_ptr<gears::Primitives> &primitives)
        : _platform(platform)
        , _factory(factory)
        , _primitives(primitives)
    {

    }

    TiledWorldImpl::~TiledWorldImpl() {
        clear();
    }

    void TiledWorldImpl::clear() {
        if (_mapSource) {
            for (std::size_t i = 0; i < _mapSize; i++) {
                delete[] _mapSource[i];
            }

            delete[] _mapSource;
        }

        if (_geometry) {
            for (std::size_t i = 0; i < _visibleSize; i++) {
                delete[] _geometry[i];
            }

            delete[] _geometry;
        }
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
                                    TileLocation location;

                                    if (stream >> options >> location.h >> location.v) {
                                        helpers.emplace_back(Helper{ location, {} }); // fill positions later
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

            // map

            std::unique_ptr<std::uint8_t[]> mapData;
            std::size_t mapSize;

            if (_platform->loadFile(mapPath.data(), mapData, mapSize)) {
                upng_t *upng = upng_new_from_bytes(mapData.get(), unsigned long(mapSize));

                if (upng != nullptr) {
                    if (*reinterpret_cast<const unsigned *>(mapData.get()) == UPNG_HEAD && upng_decode(upng) == UPNG_EOK) {
                        if (upng_get_format(upng) == UPNG_RGBA8) {
                            if (upng_get_width(upng) == upng_get_height(upng)) {
                                const std::uint32_t *pngmem = reinterpret_cast<const std::uint32_t *>(upng_get_buffer(upng));

                                _visibleSize = upng_get_width(upng);
                                _mapSize = _visibleSize + 2;
                                _mapSource = new MapCell * [_mapSize];

                                for (std::size_t i = 0; i < _mapSize; i++) {
                                    _mapSource[i] = new MapCell [_mapSize];
                                    ::memset(_mapSource[i], 0, _mapSize);
                                }

                                for (std::size_t i = 0; i < _visibleSize; i++) {
                                    for (std::size_t c = 0; c < _visibleSize; c++) {
                                        _mapSource[i + 1][c + 1].tileTypeIndex = getIndexFromColor(pngmem[i * _visibleSize + c]);
                                    }
                                }

                                _mapSource[0][0] = _mapSource[1][1];
                                _mapSource[_mapSize - 1][0] = _mapSource[_mapSize - 2][1];
                                _mapSource[0][_mapSize - 1] = _mapSource[1][_mapSize - 2];
                                _mapSource[_mapSize - 1][_mapSize - 1] = _mapSource[_mapSize - 2][_mapSize - 2];

                                for (std::size_t i = 1; i < _mapSize - 1; i++) {
                                    _mapSource[0][i] = _mapSource[1][i];
                                    _mapSource[_mapSize - 1][i] = _mapSource[_mapSize - 2][i];
                                    _mapSource[i][0] = _mapSource[i][1];
                                    _mapSource[i][_mapSize - 1] = _mapSource[i][_mapSize - 2];
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

        if (_mapSource) {
            // validation
            // TODO: 

            for (size_t i = 0; i < _mapSize; i++) {
                for (size_t c = 0; c < _mapSize; c++) {

                }
            }

            // tile info

            for (int i = 0; i < _visibleSize; i++) {
                for (int c = 0; c < _visibleSize; c++) {
                    if (_mapSource[i + 1][c + 1].tileTypeIndex != NONEXIST_TILE) {
                        _updateTileInfo(TileLocation{ c, i });
                    }
                }
            }

            // geometry & baking

            _geometry = new std::shared_ptr<voxel::StaticMesh> * [_visibleSize];

            for (int i = 0; i < _visibleSize; i++) {
                _geometry[i] = new std::shared_ptr<voxel::StaticMesh> [_visibleSize];

                for (int c = 0; c < _visibleSize; c++) {
                    if (_mapSource[i + 1][c + 1].tileTypeIndex != NONEXIST_TILE) {
                        _updateTileGeometry(TileLocation{ c, i });
                    }
                }
            }

            for (auto &group : _helpers) {
                for (auto &item : group.second) {
                    item.position = getTileCenterPosition(item.location);
                }
            }
        }
    }

    std::size_t TiledWorldImpl::getMapSize() const {
        return _visibleSize;
    }

    bool TiledWorldImpl::isTileTypeSuitable(const TileLocation &location, std::uint8_t typeIndex) {
        if (_isInMap(location) == false) {
            _platform->logError("[TiledWorld::setTileTypeIndex] no tile at {%d;%d}", location.h, location.v);
            return false;
        }

        for (int i = location.v; i <= location.v + 2; i++) {
            for (int c = location.h; c <= location.h + 2; c++) {
                int diff = std::abs(int(_mapSource[i][c].tileTypeIndex) - int(typeIndex));

                if (diff > 1) {
                    _platform->logError("[TiledWorld::setTileTypeIndex] cannot change tile at {%d;%d}", location.h, location.v);
                    return false;
                }
            }
        }

        return true;
    }

    bool TiledWorldImpl::setTileTypeIndex(const TileLocation &location, std::uint8_t typeIndex, const char *postfix) {
        if (isTileTypeSuitable(location, typeIndex)) {
            _mapSource[location.v + 1][location.h + 1].tileTypeIndex = typeIndex;

            for (int v = location.v - 1; v <= location.v + 1; v++) {
                for (int h = location.h - 1; h <= location.h + 1; h++) {
                    if (_isInMap(TileLocation{h, v})) {
                        _updateTileInfo(TileLocation{ h, v });
                        _updateTileGeometry(TileLocation{ h, v });
                    }
                }
            }

            return true;
        }

        return false;
    }

    std::size_t TiledWorldImpl::getTileNeighbors(const TileLocation &location, std::uint8_t minTypeIndex, std::uint8_t maxTypeIndex, TileLocation(&out)[8]) const {
        return _getNeighbors(location, minTypeIndex, maxTypeIndex, out);
    }

    std::uint8_t TiledWorldImpl::getTileTypeIndex(const TileLocation &location) const {
        std::uint8_t result = NONEXIST_TILE;

        if (_isInMap(location)) {
            result = _mapSource[location.v + 1][location.h + 1].tileTypeIndex;
        }

        return result;
    }

    math::vector3f TiledWorldImpl::getTileCenterPosition(const TileLocation &location) const {
        float offx = float((location.h - int(_visibleSize / 2)) * _tileSize) + _tileSize * 0.5f;
        float offz = float((location.v - int(_visibleSize / 2)) * _tileSize) + _tileSize * 0.5f;
    
        return {offx, 0, offz};
    }

    TileClass TiledWorldImpl::getTileClass(const TileLocation &location, MeshFactory::Rotation *rotation) const {
        TileClass result = TileClass::SOLID;

        if (_isInMap(location)) {
            result = _mapSource[location.v + 1][location.h + 1].tileClass;
        }

        return result;

    }

    TileLocation TiledWorldImpl::getTileLocation(const math::vector3f &position) const {
        TileLocation result;

        result.h = int(position.x - _tileSize * 0.5f) / _tileSize + int(_visibleSize / 2);
        result.v = int(position.z - _tileSize * 0.5f) / _tileSize + int(_visibleSize / 2);

        if (result.h < 0) result.h = 0;
        if (result.v < 0) result.v = 0;

        if (result.h >= _visibleSize) result.h = int(_visibleSize) - 1;
        if (result.v >= _visibleSize) result.v = int(_visibleSize) - 1;
    
        return result;
    }

    std::size_t TiledWorldImpl::getHelperCount(const char *name) const {
        auto it = _helpers.find(name);
        if (it != _helpers.end()) {
            return it->second.size();
        }

        return 0;
    }

    math::vector3f TiledWorldImpl::getHelperPosition(const char *name, std::size_t index) const {
        math::vector3f result = {};

        auto it = _helpers.find(name);
        if (it != _helpers.end()) {
            if (index < it->second.size()) {
                result = it->second[index].position;
            }
        }

        return result;
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

    bool TiledWorldImpl::getHelperTileLocation(const char *name, std::size_t index, TileLocation &out) const {
        auto it = _helpers.find(name);
        if (it != _helpers.end()) {
            if (index < it->second.size()) {
                out = it->second[index].location;
                return true;
            }
        }

        return false;
    }

    std::vector<TileLocation> TiledWorldImpl::findPath(const TileLocation &from, const TileLocation &to, std::uint8_t minTypeIndex, std::uint8_t maxTypeIndex) const {
        struct FrontlineElement {
            TileLocation location = {};
            float priority = 0;

            bool operator >(const FrontlineElement &other) const {
                return priority > other.priority;
            }
        };

        struct Info {
            TileLocation cameFrom = {};
            int totalCost = 0;
        };

        struct hasher {
            std::size_t operator()(const TileLocation &element) const
            {
                return std::hash<std::size_t>{}((std::size_t(element.v) << 32) | element.h);
            }
        };
                
        std::priority_queue<FrontlineElement, std::vector<FrontlineElement>, std::greater<FrontlineElement>> frontline; // lowest - first
        std::unordered_map<TileLocation, Info, hasher> transitions;
        TileLocation neighbors[8];

        frontline.emplace(FrontlineElement{ from });
        transitions.emplace(from, Info{ from, 0 });

        while (frontline.empty() == false) {
            auto current = frontline.top();
            frontline.pop();

            if (current.location == to) {
                break;
            }

            std::size_t neighborCount = _getNeighbors(current.location, minTypeIndex, maxTypeIndex, neighbors);

            for (std::size_t i = 0; i < neighborCount; i++) {
                int costToNeighbor = 1;
                int newCost = transitions[current.location].totalCost + 1;

                auto neighborInfo = transitions.find(neighbors[i]);
                if (neighborInfo == transitions.end() || newCost < neighborInfo->second.totalCost) {
                    transitions[neighbors[i]] = Info{ current.location, newCost };

                    float priority = float(newCost) + getTileCenterPosition(neighbors[i]).xz.distanceTo(getTileCenterPosition(to).xz);
                    frontline.push(FrontlineElement{ neighbors[i], priority });
                }
            }
        }
        
        std::vector<TileLocation> result{ to };
        TileLocation current = to;

        do {
            current = transitions[current].cameFrom;
            result.push_back(current);
        }
        while (current != from);
        std::reverse(result.begin(), result.end());

        return result;
    }

    void TiledWorldImpl::updateAndDraw(float dtSec) {
        for (int i = 0; i < _visibleSize; i++) {
            for (int c = 0; c < _visibleSize; c++) {
                _geometry[i][c]->updateAndDraw(dtSec);
            }
        }

        //auto path = findPath({ 24, 24 }, { 34, 34 }, 1, 1);

        //for (std::size_t i = 0; i < path.size(); i++) {
        //    auto center = getTileCenterPosition(path[i]);
        //    _primitives->drawLine(center, center + math::vector3f(0, 10, 0), {1.0f, 1.0f, 1.0f, 1.0f});
        //}
    }

    bool TiledWorldImpl::_isInMap(const TileLocation &location) const {
        if (location.h >= 0 && location.h < _visibleSize) {
            if (location.v >= 0 && location.v < _visibleSize) {
                return true;
            }
        }

        return false;
    }

    void TiledWorldImpl::_updateTileInfo(const TileLocation &location) {
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

            TileClass tileClass;
        };
        auto getNeighborMin = [&](int i, int c) {
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

            std::uint8_t result = _mapSource[i][c].tileTypeIndex;

            for (int k = 0; k < 8; k++) {
                std::uint8_t neighbor = _mapSource[i + offsets[k].i][c + offsets[k].c].tileTypeIndex;

                if (neighbor < result) {
                    result = neighbor;
                }
            }

            return result;
        };
        auto getNeighborParams = [&](int i, int c) {
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

            result.neighborType = _mapSource[i][c].tileTypeIndex;

            for (int k = 0; k < 4; k++) {
                std::uint8_t major = _mapSource[i + majorOffsets[k].i][c + majorOffsets[k].c].tileTypeIndex;
                std::uint8_t minor = _mapSource[i + minorOffsets[k].i][c + minorOffsets[k].c].tileTypeIndex;

                if (major == NONEXIST_TILE) {
                    major = getNeighborMin(i + majorOffsets[k].i, c + majorOffsets[k].c);
                }
                if (minor == NONEXIST_TILE) {
                    minor = getNeighborMin(i + minorOffsets[k].i, c + minorOffsets[k].c);
                }

                if (major > _mapSource[i][c].tileTypeIndex) {
                    result.neighborType = major;
                    result.majorMask.bytes[k] = '1';
                }
                if (minor > _mapSource[i][c].tileTypeIndex) {
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

            std::uint8_t tileClassIndex = 0;

            if (result.majorMask.dword == *(std::uint32_t *)"0000") {
                tileClassIndex = std::uint8_t(result.minorMask.bytes[1] - '0') | (std::uint8_t(result.minorMask.bytes[2] - '0') << 1);

                if (result.minorMask.bytes[3] == '1') {
                    tileClassIndex += 1 + result.minorMask.bytes[0] == '1' ? 1 : 0;
                }
            }
            else if (result.majorMask.dword == *(std::uint32_t *)"0001") {
                result.minorMask.bytes[3] = 'x';
                result.minorMask.bytes[2] = 'x';
                tileClassIndex = std::uint8_t(result.minorMask.bytes[0] - '0') | (std::uint8_t(result.minorMask.bytes[1] - '0') << 1);
                tileClassIndex += 6;
            }
            else if (result.majorMask.dword == *(std::uint32_t *)"0011") {
                result.minorMask.bytes[3] = 'x';
                result.minorMask.bytes[2] = 'x';
                result.minorMask.bytes[1] = 'x';
                tileClassIndex = std::uint8_t(result.minorMask.bytes[0] - '0');
                tileClassIndex += 10;
            }
            else if (result.majorMask.dword == *(std::uint32_t *)"1111") {
                result.minorMask.bytes[3] = 'x';
                result.minorMask.bytes[2] = 'x';
                result.minorMask.bytes[1] = 'x';
                result.minorMask.bytes[0] = 'x';
                tileClassIndex = 14;
            }
            else if (result.majorMask.dword == *(std::uint32_t *)"0111") {
                result.minorMask.bytes[3] = 'x';
                result.minorMask.bytes[2] = 'x';
                result.minorMask.bytes[1] = 'x';
                result.minorMask.bytes[0] = 'x';
                tileClassIndex = 13;
            }
            else if (result.majorMask.dword == *(std::uint32_t *)"0101") {
                result.minorMask.bytes[3] = 'x';
                result.minorMask.bytes[2] = 'x';
                result.minorMask.bytes[1] = 'x';
                result.minorMask.bytes[0] = 'x';
                tileClassIndex = 12;
            }

            result.tileClass = TileClass(tileClassIndex);
            result.majorMask.dword = (result.majorMask.dword << 24) | ((result.majorMask.dword & 0xff00) << 8) | ((result.majorMask.dword & 0xff0000) >> 8) | (result.majorMask.dword >> 24);
            result.minorMask.dword = (result.minorMask.dword << 24) | ((result.minorMask.dword & 0xff00) << 8) | ((result.minorMask.dword & 0xff0000) >> 8) | (result.minorMask.dword >> 24);

            return result;
        };

        NeighborParams params = getNeighborParams(location.v + 1, location.h + 1);

        _mapSource[location.v + 1][location.h + 1].tileClass = params.tileClass;
        _mapSource[location.v + 1][location.h + 1].rotation = params.rotation;
        _mapSource[location.v + 1][location.h + 1].neighborTypeIndex = params.neighborType;
    }

    void TiledWorldImpl::_updateTileGeometry(const TileLocation &location) {
        static const char *maskTable[] = {
            "",
            "_0000_1000",
            "_0000_1010",
            "_0000_1100",
            "_0000_1110",
            "_0000_1111",
            "_1000_xx00",
            "_1000_xx01",
            "_1000_xx10",
            "_1000_xx11",
            "_1100_xxx0",
            "_1100_xxx1",
            "_1010_xxxx",
            "_1110_xxxx",
            "_1111_xxxx",
        };

        char model[256];
        MapCell &cell = _mapSource[location.v + 1][location.h + 1];

        if (cell.tileClass == TileClass::SOLID) {
            ::sprintf(model, "%s/%s.vox", _spaceDirectory.data(), _tileTypes[cell.tileTypeIndex].name.data());
        }
        else {
            ::sprintf(model, "%s/%s_%s%s.vox", _spaceDirectory.data(), _tileTypes[cell.tileTypeIndex].name.data(), _tileTypes[cell.neighborTypeIndex].name.data(), maskTable[int(cell.tileClass)]);
        }

        int offx = (location.h - int(_visibleSize / 2)) * _tileSize;
        int offz = (location.v - int(_visibleSize / 2)) * _tileSize;

        std::vector<voxel::Voxel> voxels;

        if (_factory->loadVoxels(model, offx, -1, offz, cell.rotation, voxels)) {
            _geometry[location.v][location.h] = _factory->createStaticMesh(&voxels[0], voxels.size());
        }
        else {
            _platform->logError("[TiledWorld::loadSpace] voxels from '%s' aren't loaded", model);
        }
    }

    std::size_t TiledWorldImpl::_getNeighbors(const TileLocation &location, std::uint8_t minTypeIndex, std::uint8_t maxTypeIndex, TileLocation(&out)[8]) const {
        std::size_t resultCount = 0;
        
        for (int i = location.v; i <= location.v + 2; i++) {
            if (i > 0 && i < _mapSize - 1) {
                for (int c = location.h; c <= location.h + 2; c++) {
                    if (c > 0 && c < _mapSize - 1) {
                        if (_mapSource[i][c].tileTypeIndex >= minTypeIndex && _mapSource[i][c].tileTypeIndex <= maxTypeIndex) {
                            auto result = TileLocation{ c - 1, i - 1 };
                            
                            if (result != location) {
                                out[resultCount++] = result;
                            }
                        }
                    }
                }
            }
        }

        return resultCount;
    }
}

namespace voxel {
    std::shared_ptr<TiledWorld> TiledWorld::instance(const std::shared_ptr<foundation::PlatformInterface> &platform, const std::shared_ptr<voxel::MeshFactory> &factory, const std::shared_ptr<gears::Primitives> &primitives) {
        return std::make_shared<TiledWorldImpl>(platform, factory, primitives);
    }
}