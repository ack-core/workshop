
#pragma once
#include <variant>

namespace game {
    struct EditorNode {
        std::variant<
            voxel::SceneInterface::StaticMeshPtr,
            voxel::SceneInterface::ParticlesPtr
        > value;

        math::vector3f position;
        voxel::SceneInterface::BoundingBoxPtr _bbox;
        
        EditorNode(const foundation::LoggerInterfacePtr &logger, std::size_t typeIndex) : position(0, 0, 0) {
            switch (typeIndex) { // TODO: centralize mappings to dedicated header
                case 0:
                    value.emplace<voxel::SceneInterface::StaticMeshPtr>(nullptr);
                    break;
                case 1:
                    value.emplace<voxel::SceneInterface::ParticlesPtr>(nullptr);
                    break;
                default:
                    logger->logError("[EditorNode] Unknown type");
            }
        }
    };
    
    class NodeAccessInterface : public Interface {
    public:
        virtual EditorNode *getNode(const std::string &name) const = 0;

    public:
        ~NodeAccessInterface() override {}
    };
}
