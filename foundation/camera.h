
#pragma once

#include "math.h"
#include "platform/interfaces.h"

class Camera {
public:
    Camera(const std::shared_ptr<foundation::PlatformInterface> &platform) : _platform(platform) {
        setLookAtByRight({100.0f, 100.0f, 100.0f}, {0, 0, 0}, {1, 0, -1});
    }
    
    void setLookAtByRight(const math::vector3f &position, const math::vector3f &target, const math::vector3f &right) {
        _position = position;
        _target = target;
        
        math::vector3f nrmlook = (target - position).normalized();
        math::vector3f nrmright = right.normalized();
        
        _up = nrmright.cross(nrmlook);
        _forward = nrmlook;
        _right = nrmright;
        _updateMatrix();
    }
    
    void setPerspectiveProj(float fovY, float zNear, float zFar) {
        _fov = fovY;
        _zNear = zNear;
        _zFar = zFar;
        _updateMatrix();
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
    
    math::transform3f getVPMatrix() const {
        return _viewMatrix * _projMatrix;// math::transform3f::identity().scaled({0.5, 0.5, 0.5});// ;
    }
    
    math::vector3f screenToWorld(const math::vector2f &screenCoord) const {
        return {};
    }
    
    math::vector2f worldToScreen(const math::vector3f &pointInWorld) const {
        return {};
    }
    
protected:
    std::shared_ptr<foundation::PlatformInterface> _platform;
    
    math::transform3f _viewMatrix;
    math::transform3f _projMatrix;
    
    math::vector3f _position;
    math::vector3f _target;
    math::vector3f _up;
    math::vector3f _right;
    math::vector3f _forward;
    
    float _fov = 50.0f;
    float _zNear = 0.1f;
    float _zFar = 1000.0f;
    
    void _updateMatrix() {
        float aspect = _platform->getNativeScreenWidth() / _platform->getNativeScreenHeight();
        
        _viewMatrix = math::transform3f::lookAtRH(_position, _target, _up);
        _projMatrix = math::transform3f::perspectiveFovRH(_fov / 180.0f * float(3.14159f), aspect, _zNear, _zFar);
    }
};

