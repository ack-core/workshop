
#pragma once
#include <game/context.h>
#include <core/emitter_baker.h>

namespace game {
    class ParticleDevContext : public Context {
    public:
        ParticleDevContext(API &&api);
        ~ParticleDevContext() override {}

        void init() override {}
        void update(float dtSec) override;

    private:
        const API _api;

        core::EmitterBakerPtr _emitter;
        core::SceneInterface::ParticlesPtr _particles;
        float _t = 0.0f;
    };
}
