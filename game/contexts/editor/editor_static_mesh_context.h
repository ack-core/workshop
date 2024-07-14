
#pragma once
#include <game/context.h>
#include <unordered_map>

#include "node_access_interface.h"

namespace game {
    class EditorStaticMeshContext : public Context {
    public:
        EditorStaticMeshContext(API &&api, NodeAccessInterface &nodeAccess);
        ~EditorStaticMeshContext() override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
    };
}
