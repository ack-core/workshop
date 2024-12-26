
#include "editor_particles_context.h"

namespace game {
    EditorParticlesContext::EditorParticlesContext(API &&api, NodeAccessInterface &nodeAccess, CameraAccessInterface &cameraAccess)
    : _api(std::move(api))
    , _nodeAccess(nodeAccess)
    , _cameraAccess(cameraAccess)
    {
        EditorNode::makeByType[std::size_t(EditorNodeType::PARTICLES)] = [](std::size_t typeIndex) {
            return std::static_pointer_cast<EditorNode>(std::make_shared<EditorNodeParticles>(typeIndex));
        };
        
        _handlers["editor.selectNode"] = &EditorParticlesContext::_selectNode;
        _handlers["editor.static.set_texture_path"] = &EditorParticlesContext::_setTexturePath;
        _handlers["editor.clearNodeSelection"] = &EditorParticlesContext::_clearNodeSelection;
        
        _editorEventsToken = _api.platform->addEditorEventHandler([this](const std::string &msg, const std::string &data) {
            auto handler = _handlers[msg];
            if (handler) {
                _api.platform->logMsg("[Static Mesh Context] msg = '%s' arg = '%s'", msg.data(), data.c_str());
                return (this->*handler)(data);
            }
            return false;
        });
    }
    
    EditorParticlesContext::~EditorParticlesContext() {
        _api.platform->removeEventHandler(_editorEventsToken);
    }
    
    void EditorParticlesContext::update(float dtSec) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            _movingTool->setBase(node->position);
            _lineset->setPosition(node->position);
            _lineset->setLine(0, {0, 0, 0}, _movingTool->getPosition(), {0.3f, 0.3f, 0.3f, 0.3f}, false);
        }
    }

    bool EditorParticlesContext::_selectNode(const std::string &data) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            _movingTool = std::make_unique<MovingTool>(_api, _cameraAccess, node->endShapeOffset, 0.4f);
            _movingTool->setEditorMsg("engine.endShapeOffset");
            _lineset = _api.scene->addLineSet();
            _api.platform->sendEditorMsg("engine.nodeSelected", data + " inspect_particles " + node->texturePath);
            return true;
        }
        else {
            _movingTool = nullptr;
            _lineset = nullptr;
        }
        return false;
    }
    
    bool EditorParticlesContext::_setTexturePath(const std::string &data) {
        return true;
    }
    
    bool EditorParticlesContext::_clearNodeSelection(const std::string &data) {
        _movingTool = nullptr;
        _lineset = nullptr;
        return false;
    }
}

