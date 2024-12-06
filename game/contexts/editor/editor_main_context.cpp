
#include "editor_main_context.h"

namespace game {
    std::shared_ptr<EditorNode> (*EditorNode::makeByType[std::size_t(EditorNodeType::_count)])(std::size_t typeIndex) = {nullptr};
    
    namespace {
        const float MOVING_TOOL_SIZE = 5.0f;
        const math::vector3f lineDirs[] = {
            {1, 0, 0}, {0, 1, 0}, {0, 0, 1}
        };
        const math::vector4f lineColors[] = {
            {1, 0, 0, 1.0}, {0, 0.8, 0, 1.0}, {0, 0, 1, 1.0}
        };
        
        std::size_t g_unknownNodeIndex = 0;
        std::string getNextUnknownName() {
            return "Unnamed_" + std::to_string(g_unknownNodeIndex++);
        }
        
        std::string nodeTypeToPanelMapping[std::size_t(EditorNodeType::_count)] = {
            "inspect_static_mesh",
            "inspect_particles",
        };
    }
    
    MovingTool::MovingTool(const API &api, math::vector3f &target) : _api(api), _target(target) {
        _lineset = _api.scene->addLineSet();
        _lineset->setLine(0, {0, 0, 0}, MOVING_TOOL_SIZE * lineDirs[0], lineColors[0], true);
        _lineset->setLine(1, {0, 0, 0}, MOVING_TOOL_SIZE * lineDirs[1], lineColors[1], true);
        _lineset->setLine(2, {0, 0, 0}, MOVING_TOOL_SIZE * lineDirs[2], lineColors[2], true);
        _lineset->setPosition(target);
        
        _token = _api.platform->addPointerEventHandler([this](const foundation::PlatformPointerEventArgs &args) -> bool {
            _lineset->setLine(0, {0, 0, 0}, MOVING_TOOL_SIZE * lineDirs[0], lineColors[0], true);
            _lineset->setLine(1, {0, 0, 0}, MOVING_TOOL_SIZE * lineDirs[1], lineColors[1], true);
            _lineset->setLine(2, {0, 0, 0}, MOVING_TOOL_SIZE * lineDirs[2], lineColors[2], true);

            math::vector3f camPosition;
            math::vector3f cursorDir = _api.scene->getWorldDirection({args.coordinateX, args.coordinateY}, &camPosition);
            
            std::uint32_t index = _capturedLineIndex;
            float minDistance = 0.0f;
            
            if (_capturedPointerId == foundation::INVALID_POINTER_ID) {
                minDistance = std::numeric_limits<float>::max();

                for (std::uint32_t i = 0; i < 3; i++) {
                    const float dist = math::distanceRayLine(camPosition, cursorDir, _target, _target + MOVING_TOOL_SIZE * lineDirs[i]);
                    if (dist < minDistance) {
                        minDistance = dist;
                        index = i;
                    }
                }
            }
            if (minDistance < 0.5f) {
                _lineset->setLine(index, {0, 0, 0}, MOVING_TOOL_SIZE * lineDirs[index], lineColors[index] + math::vector4f(0.6, 0.6, 0.6, 0.0), true);

                if (args.type == foundation::PlatformPointerEventArgs::EventType::START) {
                    _capturedLineIndex = index;
                    _capturedPointerId = args.pointerID;
                    _capturedPosition = _target;
                    _capturedMovingKoeff = math::nearestKoeffsRayRay(camPosition, cursorDir, _target, lineDirs[_capturedLineIndex]).y;
                    return true;
                }
            }
            if (args.type == foundation::PlatformPointerEventArgs::EventType::MOVE) {
                if (_capturedPointerId == args.pointerID) {
                    const float lineMovingKoeff = math::nearestKoeffsRayRay(camPosition, cursorDir, _capturedPosition, lineDirs[_capturedLineIndex]).y;
                    _target = _capturedPosition + (lineMovingKoeff - _capturedMovingKoeff) * lineDirs[_capturedLineIndex];
                    _lineset->setPosition(_target);
                    _api.platform->sendEditorMsg("engine.nodeCoordinates", util::strstream::ftos(_target.x) + " " + util::strstream::ftos(_target.y) + " " + util::strstream::ftos(_target.z));
                    return true;
                }
            }
            if (args.type == foundation::PlatformPointerEventArgs::EventType::FINISH || args.type == foundation::PlatformPointerEventArgs::EventType::CANCEL) {
                _capturedPointerId = foundation::INVALID_POINTER_ID;
                _lineset->setLine(_capturedLineIndex, {0, 0, 0}, MOVING_TOOL_SIZE * lineDirs[_capturedLineIndex], lineColors[_capturedLineIndex], true);
            }

            return false;
        }, true);
        
        _api.platform->sendEditorMsg("engine.nodeCoordinates", util::strstream::ftos(_target.x) + " " + util::strstream::ftos(_target.y) + " " + util::strstream::ftos(_target.z));
    }
    MovingTool::~MovingTool() {
        _api.platform->removeEventHandler(_token);
    }
    
    void MovingTool::setPosition(const math::vector3f &position) {
        _target = position;
        _lineset->setPosition(_target);
    }
    const math::vector3f &MovingTool::getPosition() const {
        return _target;
    }
    
    EditorMainContext::EditorMainContext(API &&api) : _api(std::move(api)) {
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
        
//        _testButton1 = _api.ui->addImage(nullptr, ui::StageInterface::ImageParams {
//            .anchorOffset = math::vector2f(100.0f, 100.0f),
//            .anchorH = ui::HorizontalAnchor::LEFT,
//            .anchorV = ui::VerticalAnchor::TOP,
//            .textureBase = "textures/ui/btn_square_up",
//            .textureAction = "textures/ui/btn_square_down",
//        });
//        _testButton1->setActionHandler(ui::Action::PRESS, [this](float, float) {
//            _api.platform->editorLoopbackMsg("editor.createNode", "0");
//        });
//
//        _testButton2 = _api.ui->addImage(nullptr, ui::StageInterface::ImageParams {
//            .anchorTarget = _testButton1,
//            .anchorOffset = math::vector2f(10.0f, 0.0f),
//            .anchorH = ui::HorizontalAnchor::RIGHTSIDE,
//            .anchorV = ui::VerticalAnchor::TOP,
//            .textureBase = "textures/ui/btn_square_up",
//            .textureAction = "textures/ui/btn_square_down",
//        });
//        _testButton2->setActionHandler(ui::Action::PRESS, [this](float, float) {
//            _api.platform->editorLoopbackMsg("editor.createNode", "1");
//        });
        
//        _api.resources->getOrLoadVoxelStatic("statics/ruins", [this](const foundation::RenderDataPtr &mesh) {
//            if (mesh) {
//                _thing = _api.scene->addStaticMesh(mesh);
//            }
//        });
        
        
    }
    
    EditorMainContext::~EditorMainContext() {
        _api.platform->removeEventHandler(_editorEventsToken);
    }
    
    const std::weak_ptr<EditorNode> &EditorMainContext::getSelectedNode() const {
        return _currentNode;
    }
    
    void EditorMainContext::update(float dtSec) {
        
    }
    
    bool EditorMainContext::_createNode(const std::string &data) {
        util::strstream args(data.c_str(), data.length());
        std::size_t typeIndex;
        if (args >> typeIndex) {
            if (typeIndex < std::size_t(EditorNodeType::_count) && EditorNode::makeByType[typeIndex]) {
                const auto &value = _nodes.emplace(getNextUnknownName(), EditorNode::makeByType[typeIndex](typeIndex)).first;
                _movingTool = std::make_unique<MovingTool>(_api, value->second->position);
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
            _movingTool = std::make_unique<MovingTool>(_api, index->second->position);
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
        return true;
    }
    bool EditorMainContext::_moveNodeY(const std::string &data) {
        std::size_t len = 0;
        const float newPos = float(util::strstream::atof(data.c_str(), len));
        const math::vector3f currentPosition = _movingTool->getPosition();
        _movingTool->setPosition(math::vector3f(currentPosition.x, newPos, currentPosition.z));
        return true;
    }
    bool EditorMainContext::_moveNodeZ(const std::string &data) {
        std::size_t len = 0;
        const float newPos = float(util::strstream::atof(data.c_str(), len));
        const math::vector3f currentPosition = _movingTool->getPosition();
        _movingTool->setPosition(math::vector3f(currentPosition.x, currentPosition.y, newPos));
        return true;
    }
    
    bool EditorMainContext::_clearNodeSelection(const std::string &data) {
        _movingTool = nullptr;
        _currentNode.reset();
        return true;
    }
    
}

