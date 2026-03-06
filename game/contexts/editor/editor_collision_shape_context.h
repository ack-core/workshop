
#pragma once
#include <game/context.h>
#include <map>
#include "node_access_interface.h"
#include "camera_access_interface.h"
#include "editor_moving_tool.h"

// BUGS:
//
namespace game {
    struct EditorNodeCollisionShape : public EditorNode {
        util::Description description;
        
        voxel::SimulationInterface::ShapeType shapeType;
        voxel::SceneInterface::LineSetPtr visual;

        // obstacle
        bool coordMask[3] = {false};
        std::map<std::uint32_t, math::vector3f> obstaclePoints;
        
        // circle/capsule
        float mass = 0;
        float radius = 0;
        
        EditorNodeCollisionShape(std::size_t typeIndex) : EditorNode(typeIndex) {}
        ~EditorNodeCollisionShape() override {}
        
        void update(float dtSec) override;
        void setResourcePath(const API &api, const std::string &path) override;
        void updateVisual(const API &api);
    };

    class EditorCollisionShapeContext : public Context {
    public:
        EditorCollisionShapeContext(API &&api, NodeAccessInterface &nodeAccess, CameraAccessInterface &cameraAccess);
        ~EditorCollisionShapeContext() override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        const CameraAccessInterface &_cameraAccess;
        NodeAccessInterface &_nodeAccess;
        foundation::EventHandlerToken _editorEventsToken;
        std::unordered_map<std::string, bool (EditorCollisionShapeContext::*)(const std::string &)> _handlers;
        
        std::unique_ptr<MovingTool> _movePointTool;
        std::uint32_t _currentPoint = 0;

    private:
        void _endPointDragFinished();
        void _sendCurrentPointParameters(EditorNodeCollisionShape &node, bool isNew);
        bool _selectNode(const std::string &data);
        bool _setResourcePath(const std::string &data);
        bool _startEditing(const std::string &data);
        bool _stopEditing(const std::string &data);
        bool _reload(const std::string &data);
        bool _save(const std::string &data);
        bool _setShapeType(const std::string &data);
        bool _addPoint(const std::string &data);
        bool _removePoint(const std::string &data);
        bool _selectPoint(const std::string &data);
        bool _setPointArgs(const std::string &data);
        bool _setActorArgs(const std::string &data);

    };
}
