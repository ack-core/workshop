
#include "editor_raycast_shape_context.h"

namespace game {
    std::uint32_t getNextId() {
        static std::uint32_t id = 10000000;
        return id++;
    }

    void EditorNodeRaycastShape::update(float dtSec) {
        globalPosition = localPosition + (parent ? parent->globalPosition : math::vector3f(0, 0, 0));
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
            });
        }
    }

    EditorRaycastShapeContext::EditorRaycastShapeContext(API &&api, NodeAccessInterface &nodeAccess, CameraAccessInterface &cameraAccess)
    : _api(std::move(api))
    , _nodeAccess(nodeAccess)
    , _cameraAccess(cameraAccess)
    {
        EditorNode::makeByType[std::size_t(voxel::WorldInterface::NodeType::RAYCAST)] = [](std::size_t typeIndex) {
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
            for (auto &item : _points) {
                const math::vector3f pos = node->globalPosition + item.second.position;
                if (item.second.box) {
                    const math::bound3f bbox {
                        pos.x - 0.5f, pos.y - 0.5f, pos.z - 0.5f,
                        pos.x + item.second.args.x - 0.5f, pos.y + item.second.args.y - 0.5f, pos.z + item.second.args.z - 0.5f
                    };
                    item.second.box->setBBox(bbox);
                }
                else if (item.second.point) {
                    item.second.point->setPositionAndRadius(math::vector4f(pos, 0.1f));
                }
                else if (item.second.sphere) {
                    item.second.sphere->setPositionAndRadius(math::vector4f(pos, item.second.args.x));
                }
            }
        }
    }
    
    void EditorRaycastShapeContext::_endShapeDragFinished() {
        if (std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock())) {
            _sendCurrentPointParameters(false);
        }
    }
    void EditorRaycastShapeContext::_sendCurrentPointParameters(bool isNew) {
        auto index = _points.find(_currentPoint);
        if (index != _points.end()) {
            std::string args = std::to_string(_currentPoint) + " ";
            args += util::strstream::ftos(index->second.position.x) + " " + util::strstream::ftos(index->second.position.y) + " " + util::strstream::ftos(index->second.position.z) + " ";
            args += util::strstream::ftos(index->second.args.x) + " " + util::strstream::ftos(index->second.args.y) + " " + util::strstream::ftos(index->second.args.z) + (isNew ? " new" : "");

            if (_movePointTool == nullptr) {
                _movePointTool = std::make_unique<MovingTool>(_api, _cameraAccess, index->second.position, 0.4f);
                _movePointTool->setOnDragEnd([this] {
                    this->_endShapeDragFinished();
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
            _shapeType = static_cast<voxel::RaycastInterface::ShapeType>(node->description.getInteger("type", 1));
            _api.platform->sendEditorMsg("engine.raycast.editing", std::to_string(int(_shapeType)));
            const std::vector<math::vector3f> points = node->description.getVector3fs("point");
                        
            if (_shapeType == voxel::RaycastInterface::ShapeType::SPHERES) {
                const std::vector<double> radiuses = node->description.getNumbers("radius");
                for (std::size_t i = 0; i < points.size(); i++) {
                    _currentPoint = getNextId();
                    _points.emplace(_currentPoint, Point {
                        points[i], math::vector3f(radiuses[i], 0, 0),
                        _api.scene->addBoundingSphere(math::vector4f(points[i], radiuses[i]), math::color(1.0f, 0.0f, 1.0f, 0.7f)), nullptr, nullptr
                    });
                    _sendCurrentPointParameters(true);
                }
            }
            else if (_shapeType == voxel::RaycastInterface::ShapeType::BOXES) {
                const std::vector<math::vector3f> sizes = node->description.getVector3fs("size");
                for (std::size_t i = 0; i < points.size(); i++) {
                    _currentPoint = getNextId();
                    const math::vector3f bmin = points[i] - math::vector3f(0.5f);
                    const math::vector3f bmax = points[i] + sizes[i] - math::vector3f(0.5f);
                    _points.emplace(_currentPoint, Point {
                        points[i], sizes[i],
                        nullptr, nullptr, _api.scene->addBoundingBox({bmin.x, bmin.y, bmin.z, bmax.x, bmax.y, bmax.z}, math::color(1.0f, 0.0f, 1.0f, 0.7f))
                    });
                    _sendCurrentPointParameters(true);
                }
            }
            else if (_shapeType == voxel::RaycastInterface::ShapeType::TRIANGLES) {
                for (std::size_t i = 0; i < points.size(); i++) {
                    _currentPoint = getNextId();
                    _points.emplace(_currentPoint, Point {
                        points[i], math::vector3f(0, 0, 0),
                        nullptr, _api.scene->addOctahedron(math::vector4f(points[i], 0.1f), math::color(1.0f, 0.0f, 1.0f, 0.7f)), nullptr
                    });
                    _sendCurrentPointParameters(true);
                }
            }
            return true;
        }
        
        return false;
    }
    
    bool EditorRaycastShapeContext::_stopEditing(const std::string &data) {
        std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock());
        if (node && node->description.empty() == false) {
            _points.clear();
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
            node->description.setInteger("type", int(_shapeType));
            for (const auto &item : _points) {
                node->description.setVector3f("point", item.second.position, false);
            }
            if (_shapeType == voxel::RaycastInterface::ShapeType::SPHERES) {
                for (const auto &item : _points) {
                    node->description.setNumber("radius", item.second.args.x, false);
                }
            }
            else if (_shapeType == voxel::RaycastInterface::ShapeType::BOXES) {
                for (const auto &item : _points) {
                    node->description.setVector3f("size", item.second.args, false);
                }
            }
            else if (_shapeType == voxel::RaycastInterface::ShapeType::TRIANGLES) {
                // TODO: indexes
            }
            const std::string extPath = node->resourcePath + ".txt";
            const std::string serialized = util::Description::serialize(node->description);
            _api.platform->saveFile(extPath.c_str(), reinterpret_cast<const std::uint8_t *>(serialized.data()), serialized.length(), [this, path = node->resourcePath](bool result) {
                _api.platform->sendEditorMsg("engine.saved", path);
            });
            
            _points.clear();
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
                _shapeType = voxel::RaycastInterface::ShapeType(type);
                _points.clear();
                _movePointTool = nullptr;
                return true;
            }
        }
        return false;
    }
    bool EditorRaycastShapeContext::_addPoint(const std::string &data) {
        std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock());
        if (node) {
            if (_shapeType == voxel::RaycastInterface::ShapeType::SPHERES) {
                _currentPoint = getNextId();
                _points.emplace(_currentPoint, Point {
                    {0, 0, 0}, {1, 1, 1}, _api.scene->addBoundingSphere(math::vector4f({0, 0, 0}, 1.0f), math::color(1.0f, 0.0f, 1.0f, 0.7f)), nullptr, nullptr
                });
            }
            else if (_shapeType == voxel::RaycastInterface::ShapeType::BOXES) {
                _currentPoint = getNextId();
                const math::vector3f bmin = {-0.5f, -0.5f, -0.5f};
                const math::vector3f bmax = {+0.5f, +0.5f, +0.5f};
                _points.emplace(_currentPoint, Point {
                    {0, 0, 0}, {1, 1, 1}, nullptr, nullptr, _api.scene->addBoundingBox({bmin.x, bmin.y, bmin.z, bmax.x, bmax.y, bmax.z}, math::color(1.0f, 0.0f, 1.0f, 0.7f))
                });
            }
            else if (_shapeType == voxel::RaycastInterface::ShapeType::TRIANGLES) {
                _currentPoint = getNextId();
                _points.emplace(_currentPoint, Point {
                    {0, 0, 0}, {1, 1, 1}, nullptr, _api.scene->addOctahedron({0, 0, 0, 0.1f}, math::color(1.0f, 0.0f, 1.0f, 0.7f)), nullptr
                });
            }
            _sendCurrentPointParameters(true);
            return true;
        }
        return false;
    }
    
    bool EditorRaycastShapeContext::_removePoint(const std::string &data) {
        std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock());
        if (node && _movePointTool) {
            util::strstream input(data.c_str(), data.length());
            if (input >> _currentPoint) {
                _points.erase(_currentPoint);
                _currentPoint = 0;
                if (_points.empty()) {
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
                _sendCurrentPointParameters(false);
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
                auto index = _points.find(_currentPoint);
                if (index != _points.end()) {
                    index->second.position = pos;
                    index->second.args = args;
                    _sendCurrentPointParameters(false);
                }
            }
            return true;
        }
        return false;
    }
}

