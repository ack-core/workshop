
#include "walkers.h"
#include "foundation/gears/math.h"

#include <vector>

namespace dbg {
    extern math::vector3f direction;
}

namespace game {
    float periodic(float koeff) {
        return 1.0f - 2.0f * fabs(fabs(fmod(koeff + math::PI_2, 2.0f * math::PI)) - math::PI) / math::PI; //
    }

    class Walker {
    public:
        Walker(const voxel::MeshFactoryPtr &factory) {
            _core = factory->createDynamicMesh("mech_core", {}, 0.0f);
            _lleg = factory->createDynamicMesh("mech_leg_l", {}, 0.0f);
            _rleg = factory->createDynamicMesh("mech_leg_r", {}, 0.0f);
        }

        void updateAndDraw(float dtSec) {
            math::vector2f loff = math::vector2f(1.0f * cosf(_koeff), 2.0f * periodic(_koeff)); //1.0f * sinf(_koeff)
            math::vector2f roff = -loff;

            float bottom = std::min(0.0f, std::min(loff.x, roff.x));

            _core->setTransform({ _position.x, 8.0f + _position.y - bottom, _position.z }, 0.0f);
            _core->updateAndDraw(dtSec);
            _lleg->setTransform({ _position.x + 5.0f, _position.y + loff.x - bottom, _position.z + loff.y }, 0.0f);
            _lleg->updateAndDraw(dtSec);
            _rleg->setTransform({ _position.x - 5.0f, _position.y + roff.x - bottom, _position.z + roff.y }, 0.0f);
            _rleg->updateAndDraw(dtSec);

            //_koeff += _speed * dtSec;

            //if (_koeff > 2.0f * math::PI) {
            //    _koeff -= 2.0f * math::PI;
            //}

            _koeff += _speed * dbg::direction.x * dtSec;
            _position.z += 1.25f * _speed * dbg::direction.x * dtSec;
        }

    private:
        float _speed = 4.0f;
        float _koeff = 0.0f;
        math::vector3f _position = {0, 0, 0};

        voxel::DynamicMeshPtr _core;
        voxel::DynamicMeshPtr _lleg;
        voxel::DynamicMeshPtr _rleg;
    };
}

namespace game {
    class WalkersImpl : public Walkers {
    public:
        WalkersImpl(const voxel::MeshFactoryPtr &factory);
        ~WalkersImpl() override;

        void updateAndDraw(float dtSec) override;
        void test() override;

    private:
        std::shared_ptr<foundation::RenderingInterface> _rendering;
        voxel::MeshFactoryPtr _factory;
        std::vector<Walker> _walkers;
    };
}

namespace game {
    WalkersImpl::WalkersImpl(const voxel::MeshFactoryPtr &factory) : _factory(factory) {

    }

    WalkersImpl::~WalkersImpl() {

    }

    void WalkersImpl::updateAndDraw(float dtSec) {
        for (Walker &item : _walkers) {
            item.updateAndDraw(dtSec);
        }
    }

    void WalkersImpl::test() {
        _walkers.emplace_back(_factory);
    }
}

namespace game {
    std::shared_ptr<Walkers> Walkers::instance(const voxel::MeshFactoryPtr &factory) {
        return std::make_shared<WalkersImpl>(factory);
    }
}