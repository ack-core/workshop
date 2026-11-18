
#include "simulation_tests.h"

namespace test {
    bool simulationCircleMovesByVelocity(const API &api) {
        util::Description desc;
        desc.setInteger("type", int(core::SimulationInterface::ShapeType::CircleXZ));
        desc.setNumber("mass", 1.0);
        desc.setNumber("radius", 0.5);

        core::SimulationInterface::BodyPtr body = api.simulation->addBody(desc);
        EXPECT_TRUE(body != nullptr);

        body->setVelocity({1, 0, 0});
        api.simulation->update(0.033f);

        const math::vector3f position = body->getTransform().v3.xyz;
        EXPECT_EQ_F(position.x, 0.033f);
        EXPECT_EQ_F(position.y, 0.0f);
        EXPECT_EQ_F(position.z, 0.0f);
        return true;
    }
}

namespace test {
    void registerSimulationTests(TestRunner &runner) {
        REGISTER_UNIT_TEST(simulationCircleMovesByVelocity);
    }
}
