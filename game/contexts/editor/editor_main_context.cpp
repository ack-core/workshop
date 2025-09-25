
#include "editor_main_context.h"

namespace game {
    std::shared_ptr<EditorNode> (*EditorNode::makeByType[std::size_t(EditorNodeType::_count)])(std::size_t typeIndex) = {nullptr};
    
    namespace {
        std::size_t g_unknownNodeIndex = 0;
        std::string getNextUnknownName() {
            return "Unnamed_" + std::to_string(g_unknownNodeIndex++);
        }
        
        std::string nodeTypeToPanelMapping[std::size_t(EditorNodeType::_count)] = {
            "inspect_static_mesh",
            "inspect_particles",
        };
    }
    
    EditorMainContext::EditorMainContext(API &&api, CameraAccessInterface &cameraAccess) : _api(std::move(api)), _cameraAccess(cameraAccess) {
        _handlers["editor.createNode"] = &EditorMainContext::_createNode;
        _handlers["editor.selectNode"] = &EditorMainContext::_selectNode;
        _handlers["editor.renameNode"] = &EditorMainContext::_renameNode;
        _handlers["editor.moveNodeX"] = &EditorMainContext::_moveNodeX;
        _handlers["editor.moveNodeY"] = &EditorMainContext::_moveNodeY;
        _handlers["editor.moveNodeZ"] = &EditorMainContext::_moveNodeZ;
        _handlers["editor.clearNodeSelection"] = &EditorMainContext::_clearNodeSelection;
        
        _editorEventsToken = _api.platform->addEditorEventHandler([this](const std::string &msg, const std::string &data) {
            auto handler = _handlers[msg];
            if (handler) {
                _api.platform->logMsg("[Editor] msg = '%s' arg = '%s'", msg.data(), data.c_str());
                return (this->*handler)(data);
            }
            return false;
        });
        
        _axis = _api.scene->addLineSet();
        _axis->setLine(0, {0, 0, 0}, {1000, 0, 0}, {1, 0, 0, 0.5});
        _axis->setLine(1, {0, 0, 0}, {0, 1000, 0}, {0, 1, 0, 0.5});
        _axis->setLine(2, {0, 0, 0}, {0, 0, 1000}, {0, 0, 1, 0.5});
        _axis->setLine(3, {-10, 0, -10}, {10, 0, -10}, {0.3, 0.3, 0.3, 0.5});
        _axis->setLine(4, {10, 0, -10}, {10, 0, 10}, {0.3, 0.3, 0.3, 0.5});
        _axis->setLine(5, {10, 0, 10}, {-10, 0, 10}, {0.3, 0.3, 0.3, 0.5});
        _axis->setLine(6, {-10, 0, 10}, {-10, 0, -10}, {0.3, 0.3, 0.3, 0.5});        
    }
    
    EditorMainContext::~EditorMainContext() {
        _api.platform->removeEventHandler(_editorEventsToken);
    }
    
    const std::weak_ptr<EditorNode> &EditorMainContext::getSelectedNode() const {
        return _currentNode;
    }
    
    void EditorMainContext::update(float dtSec) {

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
            if (typeIndex < std::size_t(EditorNodeType::_count) && EditorNode::makeByType[typeIndex]) {
                const auto &value = _nodes.emplace(getNextUnknownName(), EditorNode::makeByType[typeIndex](typeIndex)).first;
                value->second->position = _cameraAccess.getTarget();
                _movingTool = _makeMovingTool(value->second->position);
                _api.platform->sendEditorMsg("engine.nodeCreated", value->first + " " + nodeTypeToPanelMapping[typeIndex]);
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
    
    bool EditorMainContext::_selectNode(const std::string &data) {
        const auto &index = _nodes.find(data);
        if (index != _nodes.end()) {
            _currentNode = index->second;
            _movingTool = _makeMovingTool(index->second->position);
            _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
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
                const auto &index = _nodes.find(old);
                if (index != _nodes.end()) {
                    std::shared_ptr<EditorNode> tmp = index->second;
                    _nodes.erase(index);
                    _nodes.emplace(nv, tmp);
                    _api.platform->sendEditorMsg("engine.nodeRenamed", data);
                }
                else {
                    _api.platform->logError("[EditorMainContext::_renameNode] Node '%s' not found", old.data());
                }
            }
            else {
                _api.platform->logError("[EditorMainContext::_renameNode] Node '%s' already in collection", old.data());
            }
        }
        else {
            _api.platform->logError("[EditorMainContext::_renameNode] Invalid arguments");
        }
        return true;
    }
    
    bool EditorMainContext::_moveNodeX(const std::string &data) {
        std::size_t len = 0;
        const float newPos = float(util::strstream::atof(data.c_str(), len));
        const math::vector3f currentPosition = _movingTool->getPosition();
        _movingTool->setPosition(math::vector3f(newPos, currentPosition.y, currentPosition.z));
        _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
        return true;
    }
    bool EditorMainContext::_moveNodeY(const std::string &data) {
        std::size_t len = 0;
        const float newPos = float(util::strstream::atof(data.c_str(), len));
        const math::vector3f currentPosition = _movingTool->getPosition();
        _movingTool->setPosition(math::vector3f(currentPosition.x, newPos, currentPosition.z));
        _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
        return true;
    }
    bool EditorMainContext::_moveNodeZ(const std::string &data) {
        std::size_t len = 0;
        const float newPos = float(util::strstream::atof(data.c_str(), len));
        const math::vector3f currentPosition = _movingTool->getPosition();
        _movingTool->setPosition(math::vector3f(currentPosition.x, currentPosition.y, newPos));
        _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
        return true;
    }
    
    bool EditorMainContext::_clearNodeSelection(const std::string &data) {
        _movingTool = nullptr;
        _currentNode.reset();
        _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
        return false;
    }
    
}

