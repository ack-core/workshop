
#pragma once

namespace game {
    struct EditorNode {
        math::vector3f position;
    };
    
    class NodeAccessInterface : public Interface {
    public:
        virtual EditorNode *getNode(const std::string &name) const = 0;

    public:
        ~NodeAccessInterface() override {}
    };
}
