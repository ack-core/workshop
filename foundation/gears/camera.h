
#pragma once
#include "math.h"

namespace gears {
    class Camera {
    public:
        Camera(float viewWidth, float viewHeight) : _viewWidth(viewWidth), _viewHeight(viewHeight) {
            setLookAtByRight({ 10.0f, 10.0f, 10.0f }, { 0.0f, 0, 0.0f }, { 1, 0, -1 });
        }
        
        void setViewSize(float w, float h) {
            _viewWidth = w;
            _viewHeight = h;
            _outdated = true;
        }

        void setLookAtByRight(const math::vector3f &position, const math::vector3f &target, const math::vector3f &right) {
            _position = position;
            _target = target;

            math::vector3f nrmlook = (target - position).normalized();
            math::vector3f nrmright = right.normalized();

            _up = nrmright.cross(nrmlook);
            _forward = nrmlook;
            _right = nrmright;
            _outdated = true;
        }

        void setPerspectiveProj(float fovY, float zNear, float zFar) {
            _fov = fovY;
            _zNear = zNear;
            _zFar = zFar;
            _outdated = true;
        }

        const math::vector3f &getPosition() const {
            return _position;
        }

        const math::vector3f &getForwardDirection() const {
            return _forward;
        }

        const math::vector3f &getRightDirection() const {
            return _right;
        }

        const math::vector3f &getUpDirection() const {
            return _up;
        }

        float getZNear() const {
            return _zNear;
        }

        float getZFar() const {
            return _zFar;
        }

        math::transform3f getVPMatrix() {
            if (_outdated) {
                _outdated = false;
                _updateMatrices();
            }
            return _viewMatrix * _projMatrix;
        }
        
        math::transform3f getViewMatrix() {
            if (_outdated) {
                _outdated = false;
                _updateMatrices();
            }
            return _viewMatrix;
        }

        math::transform3f getProjMatrix() {
            if (_outdated) {
                _outdated = false;
                _updateMatrices();
            }
            return _projMatrix;
        }

        math::vector3f screenToWorld(const math::vector2f &screenCoord) const {
            math::transform3f tv = _viewMatrix;
            tv._41 = 0.0f;
            tv._42 = 0.0f;
            tv._43 = 0.0f;

            math::transform3f tvp = (tv * _projMatrix).inverted();
            math::vector3f tcoord = math::vector3f(screenCoord, 0.0f);
            tcoord.x = 2.0f * tcoord.x / _viewWidth - 1.0f;
            tcoord.y = 1.0f - 2.0f * tcoord.y / _viewHeight;
            
            return tcoord.transformed(tvp, true).normalized();
        }

        math::vector2f worldToScreen(const math::vector3f &pointInWorld) const {
            return {};
        }

    protected:
        float _viewWidth;
        float _viewHeight;

        math::transform3f _viewMatrix;
        math::transform3f _projMatrix;

        math::vector3f _position;
        math::vector3f _target;
        math::vector3f _up;
        math::vector3f _right;
        math::vector3f _forward;

        float _fov = 50.0f;
        float _zNear = 0.1f;
        float _zFar = 10000.0f;
        
        bool _outdated = true;
        void _updateMatrices() {
            float aspect = _viewWidth / _viewHeight;
            _viewMatrix = math::transform3f::lookAtRH(_position, _target, _up);
            _projMatrix = math::transform3f::perspectiveFovRH(_fov / 180.0f * float(3.14159f), aspect, _zNear, _zFar);
        }
    };
}
