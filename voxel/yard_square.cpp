
#include "yard_base.h"
#include "yard_square.h"

#include <list>
#include <unordered_set>

namespace {
    static const float HM_EDGE_EPS = 0.001f;
    static const float HM_INTERNAL_EPS = 0.7f;
    static const float HM_DIV = 4.0f;
}

namespace voxel {
    struct Edge {
        std::uint32_t a;
        std::uint32_t b;
        
        bool operator==(const Edge &other) const {
            return (a == other.a && b == other.b) || (a == other.b && b == other.a);
        }
    };
    
    bool isEdgeIntersect(const SceneInterface::VTXNRMUV &a1, const SceneInterface::VTXNRMUV &a2, const SceneInterface::VTXNRMUV &b1, const SceneInterface::VTXNRMUV &b2) {
            float d = (a2.x - a1.x) * (b2.z - b1.z) - (a2.z - a1.z) * (b2.x - b1.x);

            if (std::fabs(d) < std::numeric_limits<float>::epsilon()) {
                return false;
            }

            float u = ((b1.x - a1.x) * (b2.z - b1.z) - (b1.z - a1.z) * (b2.x - b1.x)) / d;
            float v = ((b1.x - a1.x) * (a2.z - a1.z) - (b1.z - a1.z) * (a2.x - a1.x)) / d;

            if (u <= 0.0f || u >= 1.0f || v <= 0.0f || v >= 1.0f)
            {
                return false;
            }

            return true;
    }

    void YardSquare::_makeIndices(std::vector<voxel::SceneInterface::VTXNRMUV> &points, std::vector<std::uint32_t> &indices) {
        std::vector<Edge> completeEdges;
        std::list<Edge> computingEdges;
    
        indices.clear();
        computingEdges.emplace_back(Edge{1, 0});

        auto tryAddCompleteEdge = [&points, &computingEdges, &completeEdges](const Edge &newEdge) {
            for (auto index = completeEdges.rbegin(); index != completeEdges.rend(); ++index) {
                if (*index == newEdge || isEdgeIntersect(points[newEdge.a], points[newEdge.b], points[index->a], points[index->b])) {
                    return false;
                }
            }
            for (auto index = computingEdges.begin(); index != computingEdges.end(); ++index) {
                if (isEdgeIntersect(points[newEdge.a], points[newEdge.b], points[index->a], points[index->b])) {
                    return false;
                }
            }
            
            completeEdges.emplace_back(newEdge);
            return true;
        };

        auto tryAddComputingEdge = [&points, &computingEdges, &completeEdges](const Edge &newEdge) {
            bool doNotAdd = false;
            
            for (auto index = completeEdges.rbegin(); index != completeEdges.rend(); ++index) {
                if (*index == newEdge) {
                    doNotAdd = true;
                    break;
                }
                if (isEdgeIntersect(points[newEdge.a], points[newEdge.b], points[index->a], points[index->b])) {
                    return false;
                }
            }
            
            for (auto index = computingEdges.begin(); index != computingEdges.end(); ) {
                if (*index == newEdge) {
                    completeEdges.emplace_back(newEdge);
                    index = computingEdges.erase(index);
                    doNotAdd = true;
                    break;
                }
                else {
                    if (isEdgeIntersect(points[newEdge.a], points[newEdge.b], points[index->a], points[index->b])) {
                        return false;
                    }
                    
                    ++index;
                }
            }
            
            if (doNotAdd == false) {
                computingEdges.emplace_back(newEdge);
            }
            
            return true;
        };
        
        while (computingEdges.empty() == false) {
            const Edge edge = computingEdges.front();
            const voxel::SceneInterface::VTXNRMUV &ea = points[edge.a];
            const voxel::SceneInterface::VTXNRMUV &eb = points[edge.b];
            const math::vector2f ab {eb.x - ea.x, eb.z - ea.z};
            const math::vector2f edgen = ab.normalized();
            
            computingEdges.pop_front();
            completeEdges.emplace_back(edge);
            points[edge.a].nx -= 1.0f; points[edge.b].nx -= 1.0f;
            points[edge.a].ny += 1.0f; points[edge.b].ny += 1.0f;

            float minDot = 1.0f;
            
            struct {
                std::uint32_t index;
                float cosa;
            }
            minIndeces[16];
            int minIndexCount = 0;
            
            for (std::uint32_t i = 0; i < points.size(); i++) {
                const voxel::SceneInterface::VTXNRMUV &pp = points[i];
                const math::vector2f ap {pp.x - ea.x, pp.z - ea.z};
                
                if (ab.cross(ap) < 0.0f) { // is point at left of the edge
                    const math::vector2f dirA = ap.normalized();
                    const math::vector2f dirB = math::vector2f(pp.x - eb.x, pp.z - eb.z).normalized();
                    const float currentDot = dirA.dot(dirB);

                    if (currentDot < minDot - std::numeric_limits<float>::epsilon()) {
                        minDot = currentDot;
                        minIndexCount = 0;
                        minIndeces[minIndexCount].index = i;
                        minIndeces[minIndexCount].cosa = dirA.dot(edgen);
                        minIndexCount++;
                    }
                    else if (std::fabs(currentDot - minDot) < std::numeric_limits<float>::epsilon() && minIndexCount < 16) {
                        float cosa = dirA.dot(edgen);
                        
                        if (cosa >= 0.0f) {
                            minIndeces[minIndexCount].index = i;
                            minIndeces[minIndexCount].cosa = cosa;
                            minIndexCount++;
                        }
                    }
                }
            }
            
            for (int i = 0; i < minIndexCount - 1; ) {
                if (minIndeces[i].cosa > minIndeces[i + 1].cosa) {
                    std::swap(minIndeces[i], minIndeces[i + 1]);
                    i = 0;
                }
                else i++;
            }
            
            minIndeces[minIndexCount].index = edge.b;
            minIndeces[minIndexCount].cosa = 1.0f;
            
            for (int i = 1; i < minIndexCount + 1; i++) {
                const std::uint32_t pre = minIndeces[i - 1].index;
                const std::uint32_t cur = minIndeces[i].index;

                if (cur != edge.b) {
                    if (tryAddCompleteEdge(Edge{edge.a, cur})) {
                        indices.emplace_back(pre);
                        indices.emplace_back(cur);
                        indices.emplace_back(edge.a);
                    }
                }
                
                tryAddComputingEdge(Edge{pre, cur});
            }
            
            if (minIndexCount) {
                if (tryAddComputingEdge(Edge{edge.a, minIndeces[0].index})) {
                    indices.emplace_back(edge.a);
                    indices.emplace_back(edge.b);
                    indices.emplace_back(minIndeces[minIndexCount - 1].index);
                }
            }
            
        }
    }
    
    void YardSquare::_makeGeometry(const std::unique_ptr<std::uint8_t[]> &hm, const math::bound3f &bbox, std::vector<SceneInterface::VTXNRMUV> &ov, std::vector<std::uint32_t> &oi) {
        int w = int(bbox.xmax - bbox.xmin) + 1;
        int h = int(bbox.zmax - bbox.zmin) + 1;
        
        for (int c = 0; c < h; c++) {
            for (int i = 0; i < w; i++) {
                float yoffset = bbox.ymin + 1.0f;
                float height = float(hm[c * w + i]) / HM_DIV;
                float u = float(i) / float(w - 1), v = float(c) / float(h - 1);
                float fx = float(i) + bbox.xmin;
                float fz = float(c) + bbox.zmin;
                
                if (i == 0 || i == w - 1) {
                    if (c == 0 || c == h - 1) {
                        ov.emplace_back(SceneInterface::VTXNRMUV{fx, height + yoffset, fz, u, 0,0,0, v});
                    }
                    else if (fabs(float(hm[(c - 1) * w + i]) / HM_DIV - height) + fabs(float(hm[(c + 1) * w + i]) / HM_DIV - height) > HM_EDGE_EPS) {
                        ov.emplace_back(SceneInterface::VTXNRMUV{fx, height + yoffset, fz, u, 0,0,0, v});
                    }
                }
                else if (c == 0 || c == h - 1) {
                    if (i == 0 || i == w - 1) {
                        ov.emplace_back(SceneInterface::VTXNRMUV{fx, height + yoffset, fz, u, 0,0,0, v});
                    }
                    else if (fabs(float(hm[c * w + i - 1]) / HM_DIV - height) + fabs(float(hm[c * w + i + 1]) / HM_DIV - height) > HM_EDGE_EPS) {
                        ov.emplace_back(SceneInterface::VTXNRMUV{fx, height + yoffset, fz, u, 0,0,0, v});
                    }
                }
                else {
                    float d1 = (float(hm[(c - 1) * w + i]) / HM_DIV - height) - (height - float(hm[(c + 1) * w + i]) / HM_DIV);
                    float d2 = (float(hm[c * w + i - 1]) / HM_DIV - height) - (height - float(hm[c * w + i + 1]) / HM_DIV);

                    if (fabs(d1) + fabs(d2) > HM_INTERNAL_EPS) {
                        ov.emplace_back(SceneInterface::VTXNRMUV{fx, height + yoffset, fz, u, 0,0,0, v});
                    }
                }
            }
        }
        
        _makeIndices(ov, oi);
    }
}

namespace voxel {
    YardSquare::YardSquare(const YardFacility &facility, const math::bound3f &bbox, std::string &&texture, std::string &&heightmap)
    : YardStatic(facility, bbox)
    , _texturePath(std::move(texture))
    , _heightmapPath(std::move(heightmap))
    {}
    
    YardSquare::~YardSquare() {
        _model = nullptr;
    }
    
    void YardSquare::setState(YardStatic::State newState) {
        if (_currentState != newState) {
            if (newState == YardStatic::State::NONE) { // unload
                _heightmap = nullptr;
                _texture = nullptr;
                _model = nullptr;
                _bboxmdl = nullptr;
            }
            else {
                if (_currentState == YardStatic::State::NONE) { // load resources
                    if (const foundation::RenderTexturePtr &tx = _facility.getTextureProvider()->getOrLoad2DTexture(_texturePath.data())) {
                        std::uint32_t bbx = std::uint32_t(_bbox.xmax - _bbox.xmin);
                        std::uint32_t bbz = std::uint32_t(_bbox.zmax - _bbox.zmin);
                        
                        if (tx->getWidth() == bbx && tx->getHeight() == bbz) {
                            if (_heightmapPath[0]) {
                                std::uint32_t hmwidth, hmheight;
                                
                                if (_facility.getTextureProvider()->getOrLoadTextureData(_heightmapPath.data(), _heightmap, hmwidth, hmheight) == false) {
                                    _facility.getLogger()->logError("[YardSquare::setState] unable to load heightmap '%s'\n", _heightmapPath.data());
                                }
                                else if (hmwidth != bbx + 1 || hmheight != bbz + 1) {
                                    _facility.getLogger()->logError("[YardSquare::setState] heightmap '%s' doesnt fit square bbox\n", _heightmapPath.data());
                                    _heightmap = nullptr;
                                }
                            }
                            
                            if (_heightmap == nullptr) {
                                _heightmap = std::make_unique<std::uint8_t[]>((bbx + 1) * (bbz + 1));
                            }
                            
                            _texture = tx;
                            _bboxmdl = _facility.getScene()->addBoundingBox(_bbox);
                        }
                        else {
                            _facility.getLogger()->logError("[YardSquare::setState] texture '%s' doesnt fit square bbox\n", _texturePath.data());
                        }
                    }
                    else {
                        _facility.getLogger()->logError("[YardSquare::setState] unable to load texture '%s'\n", _texturePath.data());
                    }
                }
                if (newState == YardStatic::State::RENDERED) { // add to scene
                    std::vector<SceneInterface::VTXNRMUV> vertices;
                    std::vector<std::uint32_t> indices;
                    
                    _makeGeometry(_heightmap, _bbox, vertices, indices);
                    _model = _facility.getScene()->addTexturedModel(vertices, indices, _texture);
                }
                if (newState == YardStatic::State::NEARBY) { // remove from scene
                    _model = nullptr;
                }
            }
        }
        
        _currentState = newState;
    }
    
}
