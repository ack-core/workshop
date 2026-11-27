
#include "particle_dev.h"

namespace game {
    ParticleDevContext::ParticleDevContext(API &&api) : _api(std::move(api)) {
        util::Description desc;
        desc.setInteger("emissionTimeMs", 1000);
        desc.setInteger("particleLifeTimeMs", 1000);
        //desc.setInteger("bakingFrameType", int(core::EmitterBaker::BakingTime::BAKE100));
        desc.setInteger("particlesToEmit", 20);
        desc.setBool("looped", true);
        //desc.setInteger("startShapeType", int(core::EmitterBaker::ShapeType::DISK));
        //desc.setInteger("endShapeType", int(core::EmitterBaker::ShapeType::DISK));
        desc.setVector3f("endShapeOffset", {0, 10, 0});
        //desc.setInteger("shapeDistributionType", int(core::EmitterBaker::ShapeDistribution::RANDOM));
        //desc.setString("emissionGraphData", "0 1 0 1 1 0");
        //desc.setString("widthGraphData", "0 2 0 1 2 0");
        //desc.setString("heightGraphData", "0 2 0 1 2 0");
        //desc.setString("speedGraphData", "0 10 0 1 10 0");
        desc.setString("alphaGraphData", "0 1 0 1 0 0");

        _baker = core::makeEmitterBaker();
        _baker->setParameters(desc);
        _baker->refresh();

        const float bakingTimeTable[] = {
            0.010f, 0.010f, 0.020f, 0.050f, 0.100f
        };
        _bakingFrameSec = bakingTimeTable[_baker->getDescription().getInteger("bakingFrameType", 0)];

        _api.resources->getOrLoadTexture("textures/particles/test", [this](const foundation::RenderTexturePtr &texture) {
            const foundation::RenderingInterfacePtr rendering = foundation::RenderingInterface::instance(foundation::PlatformInterface::instance());
            _emitter = _api.scene->addParticles(texture, _baker->getMap(rendering), _baker->getDescription());
        });

        _pointerToken = _api.platform->addPointerEventHandler([this](const foundation::PlatformPointerEventArgs &args) -> bool {
            if (args.type == foundation::PlatformPointerEventArgs::EventType::HOVER) {
                math::vector3f camPosition;
                const math::vector3f dir = _api.scene->getWorldDirection({args.coordinateX, args.coordinateY}, &camPosition);

                if (dir.y < -std::numeric_limits<float>::epsilon()) {
                    _position = camPosition - dir * (camPosition.y / dir.y);
                }
            }
            return false;
        }, true);

        _keyboardToken = _api.platform->addKeyboardEventHandler([this](const foundation::PlatformKeyboardEventArgs &args) -> bool {
            if (args.type == foundation::PlatformKeyboardEventArgs::EventType::PRESS && args.key == foundation::Key::SPACE) {
                if (_isPlaying) {
                    _stopTime = _t;
                }
                else {
                    _t = 0.0f;
                    _frameIndex = 0;
                }
                _isPlaying = !_isPlaying;
                return true;
            }
            return false;
        });
    }

    ParticleDevContext::~ParticleDevContext() {
        _api.platform->removeEventHandler(_pointerToken);
        _api.platform->removeEventHandler(_keyboardToken);
    }

    void ParticleDevContext::update(float dtSec) {
        if (_emitter) {
            const std::size_t frameIndex = std::size_t(_t / _bakingFrameSec);

            for (; _frameIndex < frameIndex; _frameIndex++) {
                _positionHistory.insert(_positionHistory.begin(), _framePosition);
                _framePosition = _position;
            }
            if (_positionHistory.size() >= core::MAX_PARTICLES_POSITION_HISTORY) {
                _positionHistory.resize(core::MAX_PARTICLES_POSITION_HISTORY - 1);
            }

            _emitter->setTransform(math::transform3f::identity().translated(_position));
            _emitter->setTimings(_t, _isPlaying ? 0.0f : _t - _stopTime, _positionHistory);
            _t += dtSec;
        }
    }
}
