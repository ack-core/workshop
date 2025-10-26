
#pragma once
#include <map>

namespace game {
    static const char EDITOR_REFRESH_PARAM[] = "4";
    
    enum class EditorNodeType : std::size_t {
        PREFAB,
        MESH,
        PARTICLES,
        // DATA, ANIMATION, RAYCAST_SHAPE, COLLISION_SHAPE
        _count
    };
    
    struct EditorNode {
        static std::shared_ptr<EditorNode> (*makeByType[std::size_t(EditorNodeType::_count)])(std::size_t typeIndex);
        
        std::string name;
        EditorNodeType type;
        math::vector3f position;
        voxel::SceneInterface::BoundingBoxPtr bbox;
        
        std::shared_ptr<EditorNode> parent;
        std::map<std::string, std::shared_ptr<EditorNode>> children;
        
        EditorNode(std::size_t typeIndex) : type(EditorNodeType(typeIndex)), position(0, 0, 0) {}
        virtual ~EditorNode() {}
        
        virtual void update(float dtSec) {}
    };
    
    class NodeAccessInterface : public Interface {
    public:
        virtual const std::weak_ptr<EditorNode> &getSelectedNode() const = 0;
        virtual void forEachNode(util::callback<void(const std::shared_ptr<EditorNode> &)> &&handler) = 0;

    public:
        ~NodeAccessInterface() override {}
    };
}
