
#pragma once
#include <game/context.h>
#include <unordered_map>
#include "node_access_interface.h"

namespace game {
    struct EditorNodePrefab : public EditorNode {
        util::Description prefabDesc;
        
        EditorNodePrefab(std::size_t typeIndex) : EditorNode(typeIndex) {}
        ~EditorNodePrefab() override {}
        
        void update(float dtSec) override;
        void setResourcePath(const API &api, const std::string &path) override;
    };

    class EditorPrefabContext : public Context {
    public:
        EditorPrefabContext(API &&api, NodeAccessInterface &nodeAccess);
        ~EditorPrefabContext() override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        NodeAccessInterface &_nodeAccess;
        foundation::EventHandlerToken _editorEventsToken;
        std::unordered_map<std::string, bool (EditorPrefabContext::*)(const std::string &)> _handlers;
        
        
    private:
        bool _selectNode(const std::string &data);
        bool _setResourcePath(const std::string &data);
        bool _startEditing(const std::string &data);
        bool _validatePrefab(const EditorNode &rootNode);
        bool _savePrefab(const std::string &data);
        bool _reloadPrefabs(const std::string &data);
    };
}
