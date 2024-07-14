
#include "editor_static_mesh_context.h"

namespace game {
    EditorStaticMeshContext::EditorStaticMeshContext(API &&api, NodeAccessInterface &nodeAccess) : _api(std::move(api)) {
        
    }
    
    EditorStaticMeshContext::~EditorStaticMeshContext() {

    }
    
    void EditorStaticMeshContext::update(float dtSec) {

    }
}

