
#include "editor_raycast_shape_context.h"

namespace {
    std::uint32_t getNextId() {
        static std::uint32_t id = 10000000;
        return id++;
    }
}
namespace game {
    void EditorNodeRaycastShape::update(float dtSec) {
        globalPosition = localPosition + (parent ? parent->globalPosition : math::vector3f(0, 0, 0));
        updateVisual();
    }
    void EditorNodeRaycastShape::setResourcePath(const API &api, const std::string &path) {
        if (path.empty()) {
            resourcePath = EDITOR_EMPTY_RESOURCE_PATH;
            description = {};
        }
        else {
            resourcePath = path;
            api.resources->getOrLoadDescription(path.c_str(), [this, &api](const util::Description& desc) {
                description = desc;
                points.clear();
                
                shapeType = static_cast<core::RaycastInterface::ShapeType>(description.getInteger("type", 1));
                const std::vector<math::vector3f> p = description.getVector3fs("points");
                
                if (shapeType == core::RaycastInterface::ShapeType::SPHERES) {
                    const std::vector<double> radiuses = description.getNumbers("radiuses");
                    for (std::size_t i = 0; i < p.size(); i++) {
                        points.emplace(getNextId(), Point {
                            p[i], math::vector3f(radiuses[i], 0, 0),
                            api.scene->addBoundingSphere(p[i], radiuses[i], math::color(1.0f, 0.0f, 1.0f, 0.7f)), nullptr
                        });
                    }
                }
                else if (shapeType == core::RaycastInterface::ShapeType::BOXES) {
                    const std::vector<math::vector3f> sizes = description.getVector3fs("sizes");
                    for (std::size_t i = 0; i < p.size(); i++) {
                        const math::vector3f bmin = math::vector3f(-0.5f);
                        const math::vector3f bmax = sizes[i] - math::vector3f(0.5f);
                        points.emplace(getNextId(), Point {
                            p[i], sizes[i],
                            nullptr, api.scene->addBoundingBox(p[i], {bmin.x, bmin.y, bmin.z, bmax.x, bmax.y, bmax.z}, math::color(1.0f, 0.0f, 1.0f, 0.7f))
                        });
                    }
                }
                else {
                    api.platform->logError("[EditorNodeRaycastShape::setResourcePath] Unknown raycast shape type");
                }
                
                updateVisual();
                api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
            });
        }
    }
    void EditorNodeRaycastShape::updateVisual() {
        for (auto &item : points) {
            const math::vector3f pos = globalPosition + item.second.position;
            if (item.second.box) {
                const math::bound3f bbox {
                    -0.5f, -0.5f, -0.5f,
                    item.second.args.x - 0.5f, item.second.args.y - 0.5f, item.second.args.z - 0.5f
                };
                item.second.box->setPosition(pos);
                item.second.box->setBBox(bbox);
            }
            else if (item.second.sphere) {
                item.second.sphere->setPosition(pos);
                item.second.sphere->setRadius(item.second.args.x);
            }
        }
    }

    EditorRaycastShapeContext::EditorRaycastShapeContext(API &&api, NodeAccessInterface &nodeAccess, CameraAccessInterface &cameraAccess)
    : _api(std::move(api))
    , _nodeAccess(nodeAccess)
    , _cameraAccess(cameraAccess)
    {
        EditorNode::makeByType[std::size_t(core::WorldInterface::NodeType::RAYCAST)] = [](std::size_t typeIndex) {
            return std::static_pointer_cast<EditorNode>(std::make_shared<EditorNodeRaycastShape>(typeIndex));
        };
        
        _handlers["editor.selectNode"] = &EditorRaycastShapeContext::_selectNode;
        _handlers["editor.setPath"] = &EditorRaycastShapeContext::_setResourcePath;
        _handlers["editor.startEditing"] = &EditorRaycastShapeContext::_startEditing;
        _handlers["editor.stopEditing"] = &EditorRaycastShapeContext::_stopEditing;
        _handlers["editor.reloadResources"] = &EditorRaycastShapeContext::_reload;
        _handlers["editor.resource.save"] = &EditorRaycastShapeContext::_save;
        _handlers["editor.raycast.pointType"] = &EditorRaycastShapeContext::_setPointType;
        _handlers["editor.raycast.pointAdd"] = &EditorRaycastShapeContext::_addPoint;
        _handlers["editor.raycast.pointRemove"] = &EditorRaycastShapeContext::_removePoint;
        _handlers["editor.raycast.pointSelected"] = &EditorRaycastShapeContext::_selectPoint;
        _handlers["editor.raycast.pointArgs"] = &EditorRaycastShapeContext::_setPointArgs;

        _editorEventsToken = _api.platform->addEditorEventHandler([this](const std::string &msg, const std::string &data) {
            auto handler = _handlers[msg];
            if (handler) {
                return (this->*handler)(data);
            }
            return false;
        });
    }
    
    EditorRaycastShapeContext::~EditorRaycastShapeContext() {
        _api.platform->removeEventHandler(_editorEventsToken);
    }
    
    void EditorRaycastShapeContext::update(float dtSec) {
        if (std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock())) {
            if (_movePointTool) {
                _movePointTool->setBase(node->globalPosition);
                node->updateVisual();
            }
        }
    }
    
    void EditorRaycastShapeContext::_endPointDragFinished() {
        if (std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock())) {
            _sendCurrentPointParameters(*node, false);
        }
    }
    void EditorRaycastShapeContext::_sendCurrentPointParameters(EditorNodeRaycastShape &node, bool isNew) {
        auto index = node.points.find(_currentPoint);
        if (index != node.points.end()) {
            std::string args = std::to_string(_currentPoint) + " ";
            args += util::strstream::ftos(index->second.position.x) + " " + util::strstream::ftos(index->second.position.y) + " " + util::strstream::ftos(index->second.position.z) + " ";
            args += util::strstream::ftos(index->second.args.x) + " " + util::strstream::ftos(index->second.args.y) + " " + util::strstream::ftos(index->second.args.z) + (isNew ? " new" : "");

            if (_movePointTool == nullptr) {
                _movePointTool = std::make_unique<MovingTool>(_api, _cameraAccess, index->second.position, 0.4f);
                _movePointTool->setOnDragEnd([this] {
                    this->_endPointDragFinished();
                });
            }
            else {
                _movePointTool->setTarget(index->second.position);
            }
            _api.platform->sendEditorMsg("engine.raycast.pointUpdate", args);
        }
    }
    bool EditorRaycastShapeContext::_selectNode(const std::string &data) {
        if (std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock())) {
            _api.platform->sendEditorMsg("engine.nodeSelected", data + " inspect_raycast " + node->resourcePath);
        }
        return false;
    }
    
    bool EditorRaycastShapeContext::_setResourcePath(const std::string &data) {
        if (std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock())) {
            node->setResourcePath(_api, data);
            return true;
        }
        return false;
    }
        
    bool EditorRaycastShapeContext::_startEditing(const std::string &data) {
        std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock());
        if (node && node->description.empty() == false) {
            _api.platform->sendEditorMsg("engine.raycast.editing", std::to_string(int(node->shapeType)));
            for (auto &item : node->points) {
                _currentPoint = item.first;
                _sendCurrentPointParameters(*node, true);
            }
            return true;
        }
        
        return false;
    }
    
    bool EditorRaycastShapeContext::_stopEditing(const std::string &data) {
        std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock());
        if (node && node->description.empty() == false) {
            _currentPoint = 0;
            _movePointTool = nullptr;
            return true;
        }
        return false;
    }
        
    bool EditorRaycastShapeContext::_reload(const std::string &data) {
        std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock());
        if (node) {
            _api.resources->removeDescription(data.c_str());
            _api.platform->sendEditorMsg("engine.resourcesReloaded", "");
            return true;
        }
        return false;
    }

    bool EditorRaycastShapeContext::_save(const std::string &data) {
        std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock());
        if (node) {
            node->description.clear();
            node->description.setInteger("type", int(node->shapeType));
            for (const auto &item : node->points) {
                node->description.setVector3f("points", item.second.position, false);
            }
            if (node->shapeType == core::RaycastInterface::ShapeType::SPHERES) {
                for (const auto &item : node->points) {
                    node->description.setNumber("radiuses", item.second.args.x, false);
                }
            }
            else if (node->shapeType == core::RaycastInterface::ShapeType::BOXES) {
                for (const auto &item : node->points) {
                    node->description.setVector3f("sizes", item.second.args, false);
                }
            }
            const std::string extPath = node->resourcePath + ".txt";
            const std::string serialized = util::Description::serialize(node->description);
            _api.platform->saveFile(extPath.c_str(), reinterpret_cast<const std::uint8_t *>(serialized.data()), serialized.length(), [this, path = node->resourcePath](bool result) {
                _api.platform->sendEditorMsg("engine.saved", path);
            });
            
            _currentPoint = 0;
            _movePointTool = nullptr;
            return true;
        }
        return false;
    }
    bool EditorRaycastShapeContext::_setPointType(const std::string &data) {
        std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock());
        if (node) {
            util::strstream input(data.c_str(), data.length());
            int type = 0;
            if (input >> type) {
                node->shapeType = core::RaycastInterface::ShapeType(type);
                node->points.clear();
                _movePointTool = nullptr;
                _currentPoint = 0;
                return true;
            }
        }
        return false;
    }
    bool EditorRaycastShapeContext::_addPoint(const std::string &data) {
        std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock());
        if (node) {
            if (node->shapeType == core::RaycastInterface::ShapeType::SPHERES) {
                _currentPoint = getNextId();
                node->points.emplace(_currentPoint, EditorNodeRaycastShape::Point {
                    {0, 0, 0}, {1, 1, 1}, _api.scene->addBoundingSphere({0, 0, 0}, 1.0f, math::color(1.0f, 0.0f, 1.0f, 0.7f)), nullptr
                });
            }
            else if (node->shapeType == core::RaycastInterface::ShapeType::BOXES) {
                _currentPoint = getNextId();
                const math::vector3f bmin = {-0.5f, -0.5f, -0.5f};
                const math::vector3f bmax = {+0.5f, +0.5f, +0.5f};
                node->points.emplace(_currentPoint, EditorNodeRaycastShape::Point {
                    {0, 0, 0}, {1, 1, 1}, nullptr, _api.scene->addBoundingBox({0, 0, 0}, {bmin.x, bmin.y, bmin.z, bmax.x, bmax.y, bmax.z}, math::color(1.0f, 0.0f, 1.0f, 0.7f))
                });
            }
            _sendCurrentPointParameters(*node, true);
            return true;
        }
        return false;
    }
    
    bool EditorRaycastShapeContext::_removePoint(const std::string &data) {
        std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock());
        if (node && _movePointTool) {
            util::strstream input(data.c_str(), data.length());
            if (input >> _currentPoint) {
                node->points.erase(_currentPoint);
                _currentPoint = 0;
                if (node->points.empty()) {
                    _movePointTool = nullptr;
                }
            }
            return true;
        }
        return false;
    }
    
    bool EditorRaycastShapeContext::_selectPoint(const std::string &data) {
        std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock());
        if (node && _movePointTool) {
            util::strstream input(data.c_str(), data.length());
            if (input >> _currentPoint) {
                _sendCurrentPointParameters(*node, false);
            }
            return true;
        }
        return false;
    }
    
    bool EditorRaycastShapeContext::_setPointArgs(const std::string &data) {
        std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock());
        if (node && _movePointTool) {
            util::strstream input(data.c_str(), data.length());
            math::vector3f pos, args;
            bool success = false;
            
            success |= input >> pos.x >> pos.y >> pos.z;
            success |= input >> args.x >> args.y >> args.z;
            
            if (success) {
                auto index = node->points.find(_currentPoint);
                if (index != node->points.end()) {
                    index->second.position = pos;
                    index->second.args = args;
                    _sendCurrentPointParameters(*node, false);
                }
            }
            return true;
        }
        return false;
    }
}

