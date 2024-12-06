
#pragma once
#include <variant>

namespace game {
    enum class EditorNodeType : std::size_t {
        STATIC,
        PARTICLE,
        _count
    };
    
    struct EditorNode {
        static std::shared_ptr<EditorNode> (*makeByType[std::size_t(EditorNodeType::_count)])(std::size_t typeIndex);
        
        EditorNodeType type;
        math::vector3f position;
        voxel::SceneInterface::BoundingBoxPtr bbox;
        
        EditorNode(std::size_t typeIndex) : type(EditorNodeType(typeIndex)), position(0, 0, 0) {}
    };
        
    class NodeAccessInterface : public Interface {
    public:
        virtual const std::weak_ptr<EditorNode> &getSelectedNode() const = 0;

    public:
        ~NodeAccessInterface() override {}
    };
}
