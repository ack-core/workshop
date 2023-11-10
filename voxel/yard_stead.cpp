
#include "yard.h"
#include "yard_base.h"
#include "yard_stead.h"

#include <list>
#include <unordered_set>

/*
namespace {
    static const float HM_EDGE_EPS = 0.001f;
    static const float HM_INTERNAL_EPS = 0.7f;
    static const float HM_DIV = 4.0f;
}
*/

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

    void YardSteadImpl::_makeIndices(const std::vector<voxel::SceneInterface::VTXNRMUV> &points, std::vector<std::uint32_t> &indices) {
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
    
    /*
    void YardSteadImpl::_makeGeometry(const std::unique_ptr<std::uint8_t[]> &hm, const math::bound3f &bbox, std::vector<SceneInterface::VTXNRMUV> &ov, std::vector<std::uint32_t> &oi) {
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
    */
}

namespace voxel {
    YardSteadImpl::YardSteadImpl(const YardFacility &facility, std::uint64_t id, const math::vector3f &position, const math::bound3f &bbox, std::string &&source)
    : YardStatic(facility, id, bbox)
    , _sourcePath(std::move(source))
    , _position(position)
    {
    }
    
    YardSteadImpl::~YardSteadImpl() {
        _texture = nullptr;
        _model = nullptr;
        _bboxmdl = nullptr;
    }
    
    void YardSteadImpl::setPosition(const math::vector3f &position) {
        _position = position;
        if (_bboxmdl) {
            _bboxmdl->setPosition(position);
        }
        if (_model) {
            _model->setPosition(position);
        }
    }
    
    const math::vector3f &YardSteadImpl::getPosition() const {
        return _position;
    }
    
    std::uint64_t YardSteadImpl::getId() const {
        return _id;
    }
    
    void YardSteadImpl::updateState(YardLoadingState targetState) {
        if (_currentState != targetState) {
            if (targetState == YardLoadingState::NONE) { // unload
                _vertices = {};
                _indices = {};
                _currentState = targetState;
                _texture = nullptr;
                _model = nullptr;
                _bboxmdl = nullptr;
            }
            else {
                if (_bboxmdl == nullptr) {
                    _bboxmdl = _facility.getScene()->addBoundingBox(_bbox);
                    _bboxmdl->setPosition(_position);
                }
                if (_currentState == YardLoadingState::NONE) { // load resources
                    _currentState = YardLoadingState::LOADING;
                    
                    std::weak_ptr<YardSteadImpl> weak = weak_from_this();
                    _facility.getTextureProvider()->getOrLoadTexture(_sourcePath.data(), [weak](const std::unique_ptr<std::uint8_t[]> &data, const resource::TextureInfo &info) {
                        if (std::shared_ptr<YardSteadImpl> self = weak.lock()) {
                            if (data) {
                                struct AsyncContext {
                                    std::unique_ptr<std::uint8_t[]> txdata;
                                    std::vector<SceneInterface::VTXNRMUV> vertices;
                                    std::vector<std::uint32_t> indices;
                                };

                                const std::uint32_t w = info.width - 1;
                                const std::uint32_t h = info.height - 1;
                                
                                self->_facility.getPlatform()->executeAsync(std::make_unique<foundation::CommonAsyncTask<AsyncContext>>([weak, &data, &info, w, h](AsyncContext &ctx) {
                                    if (std::shared_ptr<YardSteadImpl> self = weak.lock()) {
                                        const std::uint32_t textureSize = w * h;
                                        ctx.txdata = std::make_unique<std::uint8_t[]>(textureSize);
                                        
                                        for (std::uint32_t c = 0; c < info.height; c++) {
                                            for (std::uint32_t i = 0; i < info.width; i++) {
                                                const std::uint32_t dstoff = c * w + i;
                                                const std::uint32_t srcoff = c * info.width + i;
                                                const float u = float(i) / float(w), v = float(c) / float(h);
                                                
                                                if (dstoff < textureSize) {
                                                    ctx.txdata[c * w + i] = data[4 * srcoff];
                                                }
                                                if (data[4 * srcoff + 1] & 128) {
                                                    ctx.vertices.emplace_back(SceneInterface::VTXNRMUV{-0.5f + float(i), float(data[4 * srcoff + 2]), -0.5f + float(c), u, 0, 1, 0, v});
                                                }
                                            }
                                        }
                                        
                                        _makeIndices(ctx.vertices, ctx.indices);
                                    }
                                },
                                [weak, &info, w, h](AsyncContext &ctx) {
                                    if (std::shared_ptr<YardSteadImpl> self = weak.lock()) {
                                        if (ctx.vertices.size() && ctx.indices.size()) {
                                            self->_texture = self->_facility.getRendering()->createTexture(foundation::RenderTextureFormat::R8UN, w, h, {ctx.txdata.get()});
                                            self->_vertices = std::move(ctx.vertices);
                                            self->_indices = std::move(ctx.indices);
                                            self->_currentState = YardLoadingState::LOADED;
                                        }
                                    }
                                }));
                            }
                            else {
                                self->_facility.getPlatform()->logError("[YardStead::updateState] unable to load stead source '%s'\n", self->_sourcePath.data());
                            }
                        }
                    });
                }
                if (targetState == YardLoadingState::RENDERING) { // add to scene
                    if (_currentState == YardLoadingState::LOADED) {
                        _currentState = targetState;
                        _model = _facility.getScene()->addTexturedModel(_vertices, _indices, _texture);
                        _model->setPosition(_position);
                    }
                }
                if (targetState == YardLoadingState::LOADED) { // remove from scene
                    if (_currentState == YardLoadingState::RENDERING) {
                        _currentState = targetState;
                        _model = nullptr;
                    }
                }
            }
        }
    }
    
}
