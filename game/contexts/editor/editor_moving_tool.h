
#pragma once
#include <game/context.h>
#include <unordered_map>
#include "camera_access_interface.h"

namespace game {
    class MovingTool {
    public:
        MovingTool(const API &api, const CameraAccessInterface &cameraAccess, math::vector3f &target, float size);
        ~MovingTool();
        
        void setPosition(const math::vector3f &position);
        void setUseGrid(bool useGrid);
        void setEditorMsg(const char *editorMsg);
        void setBase(const math::vector3f &base);
        void setOnDragEnd(util::callback<void()> &&handler);

        auto getPosition() const -> const math::vector3f &;
        
    public:
        const API &_api;
        const CameraAccessInterface &_cameraAccess;
        voxel::SceneInterface::LineSetPtr _lineset;
        foundation::EventHandlerToken _token = foundation::INVALID_EVENT_TOKEN;
        util::callback<void()> _onDragEnd;
        math::vector3f &_target;
        math::vector3f _base = {0, 0, 0};
        std::string _editorMsg;
        std::size_t _capturedPointerId = foundation::INVALID_POINTER_ID;
        std::uint32_t _capturedLineIndex = 0;
        math::vector3f _capturedPosition;
        float _capturedMovingKoeff = 0.0f;
        float _toolSize = 1.0f;
        bool _useGrid = false;
    };

}
