
#pragma once
#include <map>
#include <game/context.h>

namespace game {
    static const char EDITOR_REFRESH_PARAM[] = "4";
    static const char EDITOR_EMPTY_RESOURCE_PATH[] = "<None>";
    
    struct EditorNode {
        static std::shared_ptr<EditorNode> (*makeByType[std::size_t(voxel::WorldInterface::NodeType::_count)])(std::size_t typeIndex);
        
        std::string name;
        std::string resourcePath = EDITOR_EMPTY_RESOURCE_PATH;
        voxel::WorldInterface::NodeType type;
        math::vector3f globalPosition;
        math::vector3f localPosition;
        
        std::shared_ptr<EditorNode> parent;
        std::map<std::string, std::shared_ptr<EditorNode>> children;
        
        EditorNode(std::size_t typeIndex) : type(voxel::WorldInterface::NodeType(typeIndex)), globalPosition(0, 0, 0), localPosition(0, 0, 0) {}
        virtual ~EditorNode() {}
        
        virtual void update(float dtSec) {}
        virtual void setResourcePath(const API &api, const std::string &path) {}
    };
    
    class NodeAccessInterface : public Interface {
    public:
        virtual const std::weak_ptr<EditorNode> &getSelectedNode() const = 0;
        virtual bool hasNodeWithName(const std::string &name) const = 0;
        virtual void forEachNode(util::callback<void(const std::shared_ptr<EditorNode> &)> &&handler) = 0;
        virtual void createNode(voxel::WorldInterface::NodeType type, const std::string &name, const math::vector3f &position, const std::string &resourcePath) = 0;

    public:
        ~NodeAccessInterface() override {}
    };
}
