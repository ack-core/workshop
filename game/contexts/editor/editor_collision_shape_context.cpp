
#include "editor_collision_shape_context.h"

namespace {
    std::uint32_t getNextId() {
        static std::uint32_t id = 20000000;
        return id++;
    }
}
namespace game {
    void EditorNodeCollisionShape::update(float dtSec) {
        globalPosition = localPosition + (parent ? parent->globalPosition : math::vector3f(0, 0, 0));
    }
    void EditorNodeCollisionShape::setResourcePath(const API &api, const std::string &path) {
        if (path.empty()) {
            resourcePath = EDITOR_EMPTY_RESOURCE_PATH;
            description = {};
        }
        else {
            resourcePath = path;
            api.resources->getOrLoadDescription(path.c_str(), [this, &api](const util::Description& desc) {
                description = desc;
                shapeType = static_cast<voxel::SimulationInterface::ShapeType>(desc.getInteger("type", 1));
                                
                if (shapeType == voxel::SimulationInterface::ShapeType::CircleXZ) {
                    mass = desc.getNumber("mass", 0.0f);
                    radius = desc.getNumber("radius", 1.0f);
                }
                else if (shapeType == voxel::SimulationInterface::ShapeType::ObstaclePolygonXZ) {
                    obstaclePoints.clear();
                    coordMask[0] = coordMask[2] = true;
                    coordMask[1] = false;
                    const std::vector<math::vector3f> points = desc.getVector3fs("points");
                    for (std::size_t i = 0; i < points.size(); i++) {
                        obstaclePoints.emplace(getNextId(), points[i]);
                    }
                }
                else {
                    api.platform->logError("[EditorCollisionShapeContext::_startEditing] Unknown collision shape type");
                }
                updateVisual(api);
            });
        }
    }
    void EditorNodeCollisionShape::updateVisual(const API &api) {
        if (visual == nullptr) {
            visual = api.scene->addLineSet();
        }

        if (shapeType == voxel::SimulationInterface::ShapeType::CircleXZ) {
            for (std::uint32_t i = 0; i < 24; i++) {
                const float koeff0 = 2.0f * M_PI * float(i) / 24.0f;
                const float koeff1 = 2.0f * M_PI * float(i + 1) / 24.0f;
                const math::vector3f p0 = math::vector3f(radius * std::cosf(koeff0), 0.0f, radius * std::sinf(koeff0));
                const math::vector3f p1 = math::vector3f(radius * std::cosf(koeff1), 0.0f, radius * std::sinf(koeff1));
                visual->setLine(2 * i + 0, globalPosition + p0, globalPosition + p1, {0.0f, 1.0f, 1.0f, 0.7f});
                visual->setLine(2 * i + 1, globalPosition + p0, globalPosition + p0 + math::vector3f(0, 0.5f, 0), {0.0f, 1.0f, 1.0f, 0.7f});
            }
            visual->capLineCount(48);
        }
        else if (shapeType == voxel::SimulationInterface::ShapeType::ObstaclePolygonXZ) {
            std::uint32_t lineIndex = 0;
            for (auto index = obstaclePoints.begin(); index != obstaclePoints.end(); ++index) {
                auto next = index;
                if (++next == obstaclePoints.end()) {
                    next = obstaclePoints.begin();
                }
                
                visual->setLine(lineIndex++, globalPosition + index->second, globalPosition + next->second, {0.0f, 1.0f, 1.0f, 0.7f});
                const float distance = index->second.distanceTo(next->second);
                const std::uint32_t vcount = std::uint32_t(std::ceil(distance));
                for (std::uint32_t i = 0; i < vcount; i++) {
                    const math::vector3f vpos = index->second + (next->second - index->second) * float(i) / distance;
                    visual->setLine(lineIndex++, globalPosition + vpos, globalPosition + vpos + math::vector3f(0, 0.5f, 0), {0.0f, 1.0f, 1.0f, 0.7f});
                }
            }
            visual->capLineCount(lineIndex);
        }
        api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
    }

    EditorCollisionShapeContext::EditorCollisionShapeContext(API &&api, NodeAccessInterface &nodeAccess, CameraAccessInterface &cameraAccess)
    : _api(std::move(api))
    , _nodeAccess(nodeAccess)
    , _cameraAccess(cameraAccess)
    {
        EditorNode::makeByType[std::size_t(voxel::WorldInterface::NodeType::COLLISION)] = [](std::size_t typeIndex) {
            return std::static_pointer_cast<EditorNode>(std::make_shared<EditorNodeCollisionShape>(typeIndex));
        };
        
        _handlers["editor.selectNode"] = &EditorCollisionShapeContext::_selectNode;
        _handlers["editor.setPath"] = &EditorCollisionShapeContext::_setResourcePath;
        _handlers["editor.startEditing"] = &EditorCollisionShapeContext::_startEditing;
        _handlers["editor.stopEditing"] = &EditorCollisionShapeContext::_stopEditing;
        _handlers["editor.reloadResources"] = &EditorCollisionShapeContext::_reload;
        _handlers["editor.resource.save"] = &EditorCollisionShapeContext::_save;
        _handlers["editor.collision.shapeType"] = &EditorCollisionShapeContext::_setShapeType;
        _handlers["editor.collision.pointAdd"] = &EditorCollisionShapeContext::_addPoint;
        _handlers["editor.collision.pointRemove"] = &EditorCollisionShapeContext::_removePoint;
        _handlers["editor.collision.pointSelected"] = &EditorCollisionShapeContext::_selectPoint;
        _handlers["editor.collision.pointArgs"] = &EditorCollisionShapeContext::_setPointArgs;
        _handlers["editor.collision.actorArgs"] = &EditorCollisionShapeContext::_setActorArgs;

        _editorEventsToken = _api.platform->addEditorEventHandler([this](const std::string &msg, const std::string &data) {
            auto handler = _handlers[msg];
            if (handler) {
                return (this->*handler)(data);
            }
            return false;
        });
    }
    
    EditorCollisionShapeContext::~EditorCollisionShapeContext() {
        _api.platform->removeEventHandler(_editorEventsToken);
    }
    
    void EditorCollisionShapeContext::update(float dtSec) {
        if (std::shared_ptr<EditorNodeCollisionShape> node = std::dynamic_pointer_cast<EditorNodeCollisionShape>(_nodeAccess.getSelectedNode().lock())) {
            if (_movePointTool) {
                _movePointTool->setBase(node->globalPosition);
                node->updateVisual(_api);
            }
        }
    }
    
    void EditorCollisionShapeContext::_endPointDragFinished() {
        if (std::shared_ptr<EditorNodeCollisionShape> node = std::dynamic_pointer_cast<EditorNodeCollisionShape>(_nodeAccess.getSelectedNode().lock())) {
            _sendCurrentPointParameters(*node, false);
        }
    }
    
    void EditorCollisionShapeContext::_sendCurrentPointParameters(EditorNodeCollisionShape &node, bool isNew) {
        auto index = node.obstaclePoints.find(_currentPoint);
        if (index != node.obstaclePoints.end()) {
            std::string args = std::to_string(_currentPoint) + " ";
            args += node.coordMask[0] ? util::strstream::ftos(index->second.x) + " " : "";
            args += node.coordMask[1] ? util::strstream::ftos(index->second.y) + " " : "";
            args += node.coordMask[2] ? util::strstream::ftos(index->second.z) + " " : "";
            args += isNew ? "new" : "";

            if (_movePointTool == nullptr) {
                _movePointTool = std::make_unique<MovingTool>(_api, _cameraAccess, index->second, 0.4f);
                _movePointTool->setOnDragEnd([this] {
                    this->_endPointDragFinished();
                });
            }
            else {
                _movePointTool->setTarget(index->second);
            }
            _api.platform->sendEditorMsg("engine.collision.obstaclePoint", args);
        }
    }
    
    bool EditorCollisionShapeContext::_selectNode(const std::string &data) {
        if (std::shared_ptr<EditorNodeCollisionShape> node = std::dynamic_pointer_cast<EditorNodeCollisionShape>(_nodeAccess.getSelectedNode().lock())) {
            _api.platform->sendEditorMsg("engine.nodeSelected", data + " inspect_collision " + node->resourcePath);
        }
        return false;
    }
    
    bool EditorCollisionShapeContext::_setResourcePath(const std::string &data) {
        if (std::shared_ptr<EditorNodeCollisionShape> node = std::dynamic_pointer_cast<EditorNodeCollisionShape>(_nodeAccess.getSelectedNode().lock())) {
            node->setResourcePath(_api, data);
            return true;
        }
        return false;
    }
    
    bool EditorCollisionShapeContext::_startEditing(const std::string &data) {
        std::shared_ptr<EditorNodeCollisionShape> node = std::dynamic_pointer_cast<EditorNodeCollisionShape>(_nodeAccess.getSelectedNode().lock());
        if (node && node->description.empty() == false) {
            if (node->shapeType == voxel::SimulationInterface::ShapeType::CircleXZ) {
                std::string args = util::strstream::ftos(node->mass) + " " + util::strstream::ftos(node->radius) + " 0.0";
                _api.platform->sendEditorMsg("engine.collision.editing", std::to_string(int(node->shapeType)) + " actor");
                _api.platform->sendEditorMsg("engine.collision.circleXZ", args);
            }
            else if (node->shapeType == voxel::SimulationInterface::ShapeType::ObstaclePolygonXZ) {
                _api.platform->sendEditorMsg("engine.collision.editing", std::to_string(int(node->shapeType)) + " obstacle");
                const std::vector<math::vector3f> points = node->description.getVector3fs("points");
                for (auto &item : node->obstaclePoints) {
                    _currentPoint = item.first;
                    _sendCurrentPointParameters(*node, true);
                }
            }
            return true;
        }
        
        return false;
    }
    
    bool EditorCollisionShapeContext::_stopEditing(const std::string &data) {
        std::shared_ptr<EditorNodeCollisionShape> node = std::dynamic_pointer_cast<EditorNodeCollisionShape>(_nodeAccess.getSelectedNode().lock());
        if (node && node->description.empty() == false) {
            _currentPoint = 0;
            _movePointTool = nullptr;
            return true;
        }
        return false;
    }
    
    bool EditorCollisionShapeContext::_reload(const std::string &data) {
        std::shared_ptr<EditorNodeCollisionShape> node = std::dynamic_pointer_cast<EditorNodeCollisionShape>(_nodeAccess.getSelectedNode().lock());
        if (node) {
            _api.resources->removeDescription(data.c_str());
            _api.platform->sendEditorMsg("engine.resourcesReloaded", "");
            return true;
        }
        return false;
    }
    
    bool EditorCollisionShapeContext::_save(const std::string &data) {
        std::shared_ptr<EditorNodeCollisionShape> node = std::dynamic_pointer_cast<EditorNodeCollisionShape>(_nodeAccess.getSelectedNode().lock());
        if (node) {
            node->description.clear();
            node->description.setInteger("type", int(node->shapeType));
            if (node->shapeType == voxel::SimulationInterface::ShapeType::CircleXZ) {
                node->description.setNumber("radius", node->radius);
                node->description.setNumber("mass", node->mass);
            }
            else if (node->shapeType == voxel::SimulationInterface::ShapeType::ObstaclePolygonXZ) {
                for (const auto &item : node->obstaclePoints) {
                    node->description.setVector3f("points", item.second, false);
                }
            }
            else {
                _api.platform->logError("[EditorCollisionShapeContext::_save] Unknown collision shape type");
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

    bool EditorCollisionShapeContext::_setShapeType(const std::string &data) {
        std::shared_ptr<EditorNodeCollisionShape> node = std::dynamic_pointer_cast<EditorNodeCollisionShape>(_nodeAccess.getSelectedNode().lock());
        if (node) {
            util::strstream input(data.c_str(), data.length());
            int type = 0;
            if (input >> type) {
                node->shapeType = voxel::SimulationInterface::ShapeType(type);
                node->obstaclePoints.clear();
                _movePointTool = nullptr;
                _currentPoint = 0;
                
                if (node->shapeType == voxel::SimulationInterface::ShapeType::CircleXZ) {
                    node->mass = 100.0f;
                    node->radius = 1.0f;
                    _api.platform->sendEditorMsg("engine.collision.circleXZ", "100.0 1.0 0.0");
                }
                else if (node->shapeType == voxel::SimulationInterface::ShapeType::ObstaclePolygonXZ) {
                    node->coordMask[0] = node->coordMask[2] = true;
                    node->coordMask[1] = false;
                }
                else {
                    _api.platform->logError("[EditorCollisionShapeContext::_setShapeType] Unknown collision shape type");
                }
                
                return true;
            }
        }
        return false;
    }

    bool EditorCollisionShapeContext::_addPoint(const std::string &data) {
        std::shared_ptr<EditorNodeCollisionShape> node = std::dynamic_pointer_cast<EditorNodeCollisionShape>(_nodeAccess.getSelectedNode().lock());
        if (node) {
            auto index = node->obstaclePoints.find(_currentPoint);
            const math::vector3f v = index != node->obstaclePoints.end() ? index->second : math::vector3f(0, 0, 0);
            _currentPoint = getNextId();
            node->obstaclePoints.emplace(_currentPoint, v);
            _sendCurrentPointParameters(*node, true);
            return true;
        }
        return false;
    }

    bool EditorCollisionShapeContext::_removePoint(const std::string &data) {
        std::shared_ptr<EditorNodeCollisionShape> node = std::dynamic_pointer_cast<EditorNodeCollisionShape>(_nodeAccess.getSelectedNode().lock());
        if (node && _movePointTool) {
            util::strstream input(data.c_str(), data.length());
            if (input >> _currentPoint) {
                node->obstaclePoints.erase(_currentPoint);
                _currentPoint = 0;
                if (node->obstaclePoints.empty()) {
                    _movePointTool = nullptr;
                }
            }
            return true;
        }
        return false;
    }

    bool EditorCollisionShapeContext::_selectPoint(const std::string &data) {
        std::shared_ptr<EditorNodeCollisionShape> node = std::dynamic_pointer_cast<EditorNodeCollisionShape>(_nodeAccess.getSelectedNode().lock());
        if (node && _movePointTool) {
            util::strstream input(data.c_str(), data.length());
            if (input >> _currentPoint) {
                _sendCurrentPointParameters(*node, false);
            }
            return true;
        }
        return false;
    }

    bool EditorCollisionShapeContext::_setPointArgs(const std::string &data) {
        std::shared_ptr<EditorNodeCollisionShape> node = std::dynamic_pointer_cast<EditorNodeCollisionShape>(_nodeAccess.getSelectedNode().lock());
        if (node && _movePointTool) {
            util::strstream input(data.c_str(), data.length());
            math::vector3f pos;
            bool success = false;
            
            success |= node->coordMask[0] ? (input >> pos.x) : false;
            success |= node->coordMask[1] ? (input >> pos.y) : false;
            success |= node->coordMask[2] ? (input >> pos.z) : false;

            if (success) {
                auto index = node->obstaclePoints.find(_currentPoint);
                if (index != node->obstaclePoints.end()) {
                    index->second = pos;
                    _sendCurrentPointParameters(*node, false);
                }
            }
            return true;
        }
        return false;
    }

    bool EditorCollisionShapeContext::_setActorArgs(const std::string &data) {
        std::shared_ptr<EditorNodeCollisionShape> node = std::dynamic_pointer_cast<EditorNodeCollisionShape>(_nodeAccess.getSelectedNode().lock());
        if (node) {
            util::strstream input(data.c_str(), data.length());
            float mass, radius, height;
            bool success = false;
            success |= input >> mass >> radius >> height;

            if (success) {
                node->mass = mass;
                node->radius = radius;
            }
            return true;
        }
        return false;
    }

}

