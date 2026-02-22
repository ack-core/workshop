
#pragma once
#include <game/context.h>
#include <unordered_map>
#include "node_access_interface.h"
#include "camera_access_interface.h"
#include "editor_moving_tool.h"

// BUGS:
//
namespace game {
    struct EditorNodeRaycastShape : public EditorNode {
        util::Description description;
        
        EditorNodeRaycastShape(std::size_t typeIndex) : EditorNode(typeIndex) {}
        ~EditorNodeRaycastShape() override {}
        
        void update(float dtSec) override;
        void setResourcePath(const API &api, const std::string &path) override;
    };

    class EditorRaycastShapeContext : public Context {
    public:
        EditorRaycastShapeContext(API &&api, NodeAccessInterface &nodeAccess, CameraAccessInterface &cameraAccess);
        ~EditorRaycastShapeContext() override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        const CameraAccessInterface &_cameraAccess;
        NodeAccessInterface &_nodeAccess;
        foundation::EventHandlerToken _editorEventsToken;
        std::unordered_map<std::string, bool (EditorRaycastShapeContext::*)(const std::string &)> _handlers;
        
        struct Point {
            math::vector3f position;
            math::vector3f args;
            voxel::SceneInterface::BoundingSpherePtr sphere;
            voxel::SceneInterface::OctahedronPtr point;
            voxel::SceneInterface::BoundingBoxPtr box;
        };
        std::unordered_map<std::uint32_t, Point> _points;
        std::uint32_t _currentPoint = 0;
        voxel::RaycastInterface::ShapeType _shapeType;
        std::unique_ptr<MovingTool> _movePointTool; // not null when editing is in progress

    private:
        void _endShapeDragFinished();
        void _sendCurrentPointParameters(bool isNew);
        bool _selectNode(const std::string &data);
        bool _setResourcePath(const std::string &data);
        bool _startEditing(const std::string &data);
        bool _stopEditing(const std::string &data);
        bool _reload(const std::string &data);
        bool _save(const std::string &data);
        bool _setPointType(const std::string &data);
        bool _addPoint(const std::string &data);
        bool _removePoint(const std::string &data);
        bool _selectPoint(const std::string &data);
        bool _setPointArgs(const std::string &data);
    };
}
