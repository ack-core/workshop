
#pragma once
#include <game/context.h>
#include <core/emitter_baker.h>

namespace game {
    class ParticleDevContext : public Context {
    public:
        ParticleDevContext(API &&api);
        ~ParticleDevContext() override;

        void init() override {}
        void update(float dtSec) override;

    private:
        const API _api;

        core::EmitterBakerPtr _baker;
        core::SceneInterface::ParticlesPtr _emitter;
        foundation::EventHandlerToken _pointerToken = foundation::INVALID_EVENT_TOKEN;
        foundation::EventHandlerToken _keyboardToken = foundation::INVALID_EVENT_TOKEN;

        bool _isPlaying = true;
        float _stopTime = 0.0f;
        float _t = 0.0f;
        float _bakingFrameSec = 0.0f;
        std::size_t _frameIndex = 0;
        math::vector3f _position = {0, 0, 0};
        math::vector3f _framePosition = {0, 0, 0};
        std::vector<math::vector3f> _positionHistory;
    };
}
