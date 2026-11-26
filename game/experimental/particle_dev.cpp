
#include "particle_dev.h"

namespace game {
    ParticleDevContext::ParticleDevContext(API &&api) : _api(std::move(api)) {
        util::Description desc;
        desc.setInteger("emissionTimeMs", 1000);
        desc.setInteger("particleLifeTimeMs", 1000);
        //desc.setInteger("bakingFrameType", int(core::EmitterBaker::BakingTime::BAKE100));
        desc.setInteger("particlesToEmit", 5);
        desc.setBool("looped", true);
        //desc.setInteger("startShapeType", int(core::EmitterBaker::ShapeType::DISK));
        //desc.setInteger("endShapeType", int(core::EmitterBaker::ShapeType::DISK));
        desc.setVector3f("endShapeOffset", {0, 10, 0});
        //desc.setInteger("shapeDistributionType", int(core::EmitterBaker::ShapeDistribution::RANDOM));
        //desc.setString("emissionGraphData", "0 1 0 1 1 0");
        //desc.setString("widthGraphData", "0 2 0 1 2 0");
        //desc.setString("heightGraphData", "0 2 0 1 2 0");
        //desc.setString("speedGraphData", "0 10 0 1 10 0");
        //desc.setString("alphaGraphData", "0 1 0 1 1 0");

        _emitter = core::makeEmitterBaker();
        //_emitter->setParameters(desc);
        _emitter->refresh();

        _api.resources->getOrLoadTexture("textures/particles/test", [this](const foundation::RenderTexturePtr &texture) {
            const foundation::RenderingInterfacePtr rendering = foundation::RenderingInterface::instance(foundation::PlatformInterface::instance());
            _particles = _api.scene->addParticles(texture, _emitter->getMap(rendering), _emitter->getDescription());
        });
    }

    void ParticleDevContext::update(float dtSec) {
        if (_particles) {
            _particles->setTransform(math::transform3f::identity());
            _particles->setTime(_t, 0.0f);
            _t += dtSec;
        }
    }
}
