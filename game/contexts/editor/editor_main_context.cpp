
#include "editor_main_context.h"
#include "utils.h"
#include <list>

namespace game {
    std::shared_ptr<EditorNode> (*EditorNode::makeByType[std::size_t(voxel::WorldInterface::NodeType::_count)])(std::size_t typeIndex) = {nullptr};
    
    namespace {
        std::size_t g_unknownPrefabIndex = 0;
        std::size_t g_unknownNodeIndex = 0;
    
        std::string getNextUnknownName(voxel::WorldInterface::NodeType type) {
            if (type == voxel::WorldInterface::NodeType::PREFAB) {
                return "p" + std::to_string(g_unknownPrefabIndex++);
            }
            else {
                return "new" + std::to_string(g_unknownNodeIndex++);
            }
        }
        
        std::string nodeTypeToPanelMapping[std::size_t(voxel::WorldInterface::NodeType::_count)] = {
            "inspect_prefab",
            "inspect_mesh",
            "inspect_particles",
        };
    }
    
    EditorMainContext::EditorMainContext(API &&api, CameraAccessInterface &cameraAccess) : _api(std::move(api)), _cameraAccess(cameraAccess) {
        _handlers["editor.createNode"] = &EditorMainContext::_createNode;
        _handlers["editor.deleteNode"] = &EditorMainContext::_deleteNode;
        _handlers["editor.selectNode"] = &EditorMainContext::_selectNode;
        _handlers["editor.renameNode"] = &EditorMainContext::_renameNode;
        _handlers["editor.moveNode"] = &EditorMainContext::_moveNode;
        _handlers["editor.clearNodeSelection"] = &EditorMainContext::_clearNodeSelection;
        _handlers["editor.startEditing"] = &EditorMainContext::_startEditing;
        _handlers["editor.stopEditing"] = &EditorMainContext::_stopEditing;
        _handlers["editor.resource.save"] = &EditorMainContext::_stopEditing;
        _handlers["editor.centerOnObject"] = &EditorMainContext::_centerOnObject;
        _handlers["editor.axisSwitch"] = &EditorMainContext::_axisSwitch;
        _handlers["editor.updateSwitch"] = &EditorMainContext::_updateSwitch;
        
        _editorEventsToken = _api.platform->addEditorEventHandler([this](const std::string &msg, const std::string &data) {
            auto handler = _handlers[msg];
            if (handler) {
                _api.platform->logMsg("[Editor] msg = '%s' arg = '%s'", msg.data(), data.c_str());
                const bool result = (this->*handler)(data);
                _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
                return result;
            }
            return false;
        });
        
        _axis = _api.scene->addLineSet();
        _axis->setLine(0, {0, 0, 0}, {1000, 0, 0}, {1, 0, 0, 0.6});
        _axis->setLine(1, {0, 0, 0}, {0, 1000, 0}, {0, 1, 0, 0.6});
        _axis->setLine(2, {0, 0, 0}, {0, 0, 1000}, {0, 0, 1, 0.6});
        _axis->setLine(3, {-10, 0, -10}, {10, 0, -10}, {0.3, 0.3, 0.3, 0.6});
        _axis->setLine(4, {10, 0, -10}, {10, 0, 10}, {0.3, 0.3, 0.3, 0.6});
        _axis->setLine(5, {10, 0, 10}, {-10, 0, 10}, {0.3, 0.3, 0.3, 0.6});
        _axis->setLine(6, {-10, 0, 10}, {-10, 0, -10}, {0.3, 0.3, 0.3, 0.6});
    }
    
    EditorMainContext::~EditorMainContext() {
        _api.platform->removeEventHandler(_editorEventsToken);
    }
    
    const std::weak_ptr<EditorNode> &EditorMainContext::getSelectedNode() const {
        return _currentNode;
    }

    bool EditorMainContext::hasNodeWithName(const std::string &name) const {
        return _nodes.find(name) != _nodes.end();
    }
    
    void EditorMainContext::forEachNode(util::callback<void(const std::shared_ptr<EditorNode> &)> &&handler) {
        for (auto &item : _nodes) {
            handler(item.second);
        }
    }
    
    void EditorMainContext::createNode(voxel::WorldInterface::NodeType type, const std::string &name, const math::vector3f &position, const std::string &resourcePath) {
        std::size_t typeIndex = std::size_t(type);
        const auto &value = _nodes.emplace(name, EditorNode::makeByType[typeIndex](typeIndex)).first;
        const std::string parentName = editor::getParentName(name);
        if (parentName.length()) {
            auto parentIndex = _nodes.find(parentName);
            value->second->parent = parentIndex->second;
            parentIndex->second->children.emplace(name, value->second);
        }
        value->second->name = name;
        value->second->localPosition = position;
        value->second->setResourcePath(_api, resourcePath);
    }
    
    void EditorMainContext::update(float dtSec) {
        if (_doUpdateEveryFrame || _isEditing) {
            _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
        }
        else {
            dtSec = 0.0f;
        }
        for (auto &item : _nodes) {
            item.second->update(dtSec);
        }
    }
    
    std::unique_ptr<MovingTool> EditorMainContext::_makeMovingTool(math::vector3f &target) {
        auto result = std::make_unique<MovingTool>(_api, _cameraAccess, target, 1.0f);
        result->setEditorMsg("engine.nodeCoordinates");
        result->setUseGrid(true);
        return result;
    }
    
    bool EditorMainContext::_createNode(const std::string &data) {
        util::strstream args(data.c_str(), data.length());
        std::size_t typeIndex;
        if (args >> typeIndex) {
            if (typeIndex < std::size_t(voxel::WorldInterface::NodeType::_count) && EditorNode::makeByType[typeIndex]) {
                std::string name = getNextUnknownName(voxel::WorldInterface::NodeType(typeIndex));
                while (_nodes.find(name) != _nodes.end()) {
                    name = getNextUnknownName(voxel::WorldInterface::NodeType(typeIndex));
                }
                
                const auto &value = _nodes.emplace(name, EditorNode::makeByType[typeIndex](typeIndex)).first;
                value->second->name = name;
                value->second->localPosition = _cameraAccess.getTarget();
                _movingTool = _makeMovingTool(value->second->localPosition);
                const char *isPrefab = value->second->type == voxel::WorldInterface::NodeType::PREFAB ? " 1" : " 0";
                _api.platform->sendEditorMsg("engine.nodeCreated", value->first + isPrefab);
            }
            else {
                _api.platform->logError("[EditorMainContext::_createNode] Unknown node type");
            }
        }
        else {
            _api.platform->logError("[EditorMainContext::_createNode] Invalid arguments");
        }
        return true;
    }
    
    bool EditorMainContext::_deleteNode(const std::string &data) {
        if (_currentNode.lock()) {
            std::list<std::string> toDeleteList;
            toDeleteList.emplace_back(data);
            
            while (toDeleteList.empty() == false) {
                auto index = _nodes.find(toDeleteList.front());
                if (index != _nodes.end()) {
                    if (index->second->parent) {
                        index->second->parent->children.erase(index->second->name);
                        index->second->parent = nullptr;
                    }

                    for (auto &item : index->second->children) {
                        toDeleteList.emplace_back(item.first);
                    }

                    _nodes.erase(index->first);
                }
                toDeleteList.pop_front();
            }
            
            _api.platform->sendEditorMsg("engine.nodeDeleted", data);
        }
        return true;
    }
    
    bool EditorMainContext::_selectNode(const std::string &data) {
        const auto &index = _nodes.find(data);
        if (index != _nodes.end()) {
            _currentNode = index->second;
            _movingTool = _makeMovingTool(index->second->localPosition);
            _movingTool->setBase(index->second->parent ? index->second->parent->globalPosition : math::vector3f(0, 0, 0));
            return false; // A typed context must handle this
        }
        else {
            _currentNode.reset();
            _api.platform->logError("[EditorMainContext::_selectNode] Node '%s' not found", data.data());
        }
        return true;
    }
    
    bool EditorMainContext::_renameNode(const std::string &data) {
        util::strstream args(data.c_str(), data.length());
        std::string old, nv;
        if (args >> old >> nv) {
            if (_nodes.find(nv) == _nodes.end()) {
                const auto &nodeIndex = _nodes.find(old);
                if (nodeIndex != _nodes.end()) {
                    std::shared_ptr<EditorNode> tmp = nodeIndex->second;
                    if (tmp->children.empty()) {
                        const std::string newParentName = editor::getParentName(nv);
                        if (newParentName.length()) {
                            auto newParentIndex = _nodes.find(newParentName);
                            bool prefabLimited = tmp->type != voxel::WorldInterface::NodeType::PREFAB && newParentIndex->second->type != voxel::WorldInterface::NodeType::PREFAB;
                            if (newParentIndex != _nodes.end() && newParentName != old && prefabLimited) {
                                newParentIndex->second->children.emplace(nv, tmp);
                                tmp->parent->children.erase(tmp->name);
                                tmp->parent = newParentIndex->second;
                            }
                            else {
                                _api.platform->sendEditorMsg("engine.nodeRenamed", "declined " + old + " " + nv);
                                return true;
                            }
                        }

                        tmp->name = nv;
                        _nodes.erase(nodeIndex);
                        _nodes.emplace(nv, tmp);
                        _api.platform->sendEditorMsg("engine.nodeRenamed", "ok " + data);
                    }
                    else {
                        _api.platform->sendEditorMsg("engine.nodeRenamed", "declined " + old + " " + nv);
                    }
                }
                else {
                    _api.platform->logError("[EditorMainContext::_renameNode] Node '%s' not found", old.data());
                }
            }
            else {
                _api.platform->sendEditorMsg("engine.nodeRenamed", "declined " + old + " " + nv);
            }
        }
        else {
            _api.platform->logError("[EditorMainContext::_renameNode] Invalid arguments");
        }
        return true;
    }

    bool EditorMainContext::_moveNode(const std::string &data) {
        util::strstream input(data.c_str(), data.length());
        math::vector3f newPos;
        
        if (input >> newPos.x >> newPos.y >> newPos.z) {
            _movingTool->setPosition(newPos);
        }

        return true;
    }
    
    bool EditorMainContext::_clearNodeSelection(const std::string &data) {
        _movingTool = nullptr;
        _currentNode.reset();
        return false;
    }
    
    bool EditorMainContext::_startEditing(const std::string &data) {
        _isEditing = true;
        return false;
    }
    
    bool EditorMainContext::_stopEditing(const std::string &data) {
        _isEditing = false;
        return false;
    }
    
    bool EditorMainContext::_centerOnObject(const std::string &data) {
        if (std::shared_ptr<EditorNode> node = _currentNode.lock()) {
            _cameraAccess.setTarget(node->globalPosition);
        }
        return true;
    }
    
    bool EditorMainContext::_axisSwitch(const std::string &data) {
        _api.scene->setLinesDrawingEnabled(data == "true");
        return true;
    }
    
    bool EditorMainContext::_updateSwitch(const std::string &data) {
        _doUpdateEveryFrame = (data == "true");
        return true;
    }
}

