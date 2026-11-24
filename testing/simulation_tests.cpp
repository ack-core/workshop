
#include "runner.h"

namespace {
    core::SimulationInterface::BodyPtr addTestCircle(const test::API &api, const math::vector3f &position, const math::vector3f &velocity) {
        util::Description desc;
        desc.setInteger("type", int(core::SimulationInterface::ShapeType::CircleXZ));
        desc.setNumber("mass", 1.0);
        desc.setNumber("radius", 0.5);

        core::SimulationInterface::BodyPtr body = api.simulation->addBody(desc);
        body->setTransform(math::transform3f::identity().translated(position));
        body->setVelocity(velocity);
        return body;
    }

    core::SimulationInterface::BodyPtr addTestWall(const test::API &api) {
        util::Description desc;
        desc.setInteger("type", int(core::SimulationInterface::ShapeType::ObstaclePolygonXZ));
        desc.setVector3f("points", math::vector3f(2.0f, 0.0f, -10.0f), false);
        desc.setVector3f("points", math::vector3f(2.0f, 0.0f, 10.0f), false);
        desc.setVector3f("points", math::vector3f(4.0f, 0.0f, 10.0f), false);
        desc.setVector3f("points", math::vector3f(4.0f, 0.0f, -10.0f), false);
        return api.simulation->addBody(desc);
    }
}

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

    bool simulationCircleObstacleStopAndSlide(const API &api) {
        core::SimulationInterface::BodyPtr wall = addTestWall(api);
        core::SimulationInterface::BodyPtr stopping = addTestCircle(api, {0, 0, 0}, {1, 0, 0});
        core::SimulationInterface::BodyPtr sliding = addTestCircle(api, {0, 0, -5}, {1, 0, 1});
        EXPECT_TRUE(wall != nullptr);

        for (int i = 0; i < 30; i++) {
            api.simulation->update(0.1f);
            EXPECT_LT(stopping->getTransform().v3.x, 1.5f + 1.0e-4f);
            EXPECT_LT(sliding->getTransform().v3.x, 1.5f + 1.0e-4f);
        }

        const math::vector3f stopPos = stopping->getTransform().v3.xyz;
        const math::vector3f slidePos = sliding->getTransform().v3.xyz;
        EXPECT_LT(std::fabs(stopPos.x - 1.5f), 1.0e-4f);
        EXPECT_LT(std::fabs(stopPos.z), 1.0e-4f);
        EXPECT_LT(std::fabs(slidePos.x - 1.5f), 1.0e-4f);
        EXPECT_LT(std::fabs(slidePos.z + 2.0f), 1.0e-4f);
        return true;
    }

    bool simulationCircleObstacleInactiveBodies(const API &api) {
        core::SimulationInterface::BodyPtr wall = addTestWall(api);
        core::SimulationInterface::BodyPtr disabled = addTestCircle(api, {1.8f, 0, 0}, {1, 0, 0});
        core::SimulationInterface::BodyPtr fixed = addTestCircle(api, {1.8f, 0, 3}, {1, 0, 0});
        disabled->setEnabled(false);
        fixed->setMovable(false);

        for (int i = 0; i < 5; i++) {
            api.simulation->update(0.1f);
        }

        const math::vector3f disabledPos = disabled->getTransform().v3.xyz;
        const math::vector3f fixedPos = fixed->getTransform().v3.xyz;
        EXPECT_EQ_F(disabledPos.x, 1.8f);
        EXPECT_EQ_F(disabledPos.z, 0.0f);
        EXPECT_EQ_F(fixedPos.x, 1.8f);
        EXPECT_EQ_F(fixedPos.z, 3.0f);
        return true;
    }

    bool simulationCircleImpulseFades(const API &api) {
        util::Description desc;
        desc.setInteger("type", int(core::SimulationInterface::ShapeType::CircleXZ));
        desc.setNumber("mass", 1.0);
        desc.setNumber("radius", 0.5);

        core::SimulationInterface::BodyPtr body = api.simulation->addBody(desc);
        EXPECT_TRUE(body != nullptr);

        body->addImpulse({1, 0, 0}, 1.0f);
        api.simulation->update(0.1f);
        const float step1 = body->getTransform().v3.x;
        EXPECT_EQ_F(step1, 0.1f);
        EXPECT_EQ_F(body->getVelocity().x, 0.0f);

        api.simulation->update(0.1f);
        const float step2 = body->getTransform().v3.x - step1;
        EXPECT_TRUE(step2 > 0.0f && step2 < step1);

        body->setVelocity({0, 0, 0});
        api.simulation->update(0.1f);
        const float step3 = body->getTransform().v3.x - step1 - step2;
        EXPECT_TRUE(step3 > 0.0f && step3 < step2);
        return true;
    }

    bool simulationCircleGravity(const API &api) {
        core::SimulationInterface::BodyPtr falling = addTestCircle(api, {0, 0, 0}, {0, 0, 0});
        core::SimulationInterface::BodyPtr disabled = addTestCircle(api, {5, 0, 0}, {0, 0, 0});
        core::SimulationInterface::BodyPtr fixed = addTestCircle(api, {-5, 0, 0}, {0, 0, 0});
        disabled->setEnabled(false);
        fixed->setMovable(false);

        api.simulation->setGravity({0, -5, 2});
        api.simulation->update(0.1f);
        EXPECT_EQ_F(falling->getTransform().v3.z, 0.02f);
        api.simulation->update(0.1f);
        api.simulation->setGravity({0, 0, 0});

        const math::vector3f position = falling->getTransform().v3.xyz;
        EXPECT_EQ_F(position.x, 0.0f);
        EXPECT_EQ_F(position.y, 0.0f);
        EXPECT_EQ_F(position.z, 0.06f);
        EXPECT_EQ_F(falling->getVelocity().z, 0.4f);
        EXPECT_EQ_F(disabled->getTransform().v3.z, 0.0f);
        EXPECT_EQ_F(fixed->getTransform().v3.z, 0.0f);
        return true;
    }

    bool simulationCircleGravityParabola(const API &api) {
        core::SimulationInterface::BodyPtr body = addTestCircle(api, {0, 0, 0}, {10, 0, -10});
        api.simulation->setGravity({0, 0, 10});

        const float expectedZ[] = {-2.5f, -2.5f, 0.0f, 5.0f};
        for (int i = 0; i < 4; i++) {
            api.simulation->update(0.5f);
            EXPECT_EQ_F(body->getTransform().v3.x, 5.0f * float(i + 1));
            EXPECT_EQ_F(body->getTransform().v3.z, expectedZ[i]);
        }

        api.simulation->setGravity({0, 0, 0});
        return true;
    }

    bool simulationCirclesCollision(const API &api) {
        core::SimulationInterface::BodyPtr movableA = addTestCircle(api, {0, 0, 0}, {0, 0, 0});
        core::SimulationInterface::BodyPtr movableB = addTestCircle(api, {0.6f, 0, 0}, {0, 0, 0});
        core::SimulationInterface::BodyPtr fixed = addTestCircle(api, {0, 0, 10}, {0, 0, 0});
        core::SimulationInterface::BodyPtr pushed = addTestCircle(api, {0.6f, 0, 10}, {0, 0, 0});
        core::SimulationInterface::BodyPtr disabled = addTestCircle(api, {0, 0, 20}, {0, 0, 0});
        core::SimulationInterface::BodyPtr untouched = addTestCircle(api, {0.6f, 0, 20}, {0, 0, 0});
        fixed->setMovable(false);
        disabled->setEnabled(false);

        api.simulation->update(0.1f);

        EXPECT_EQ_F(movableA->getTransform().v3.x, -0.2f);
        EXPECT_EQ_F(movableB->getTransform().v3.x, 0.8f);
        EXPECT_EQ_F(fixed->getTransform().v3.x, 0.0f);
        EXPECT_EQ_F(pushed->getTransform().v3.x, 1.0f);
        EXPECT_EQ_F(disabled->getTransform().v3.x, 0.0f);
        EXPECT_EQ_F(untouched->getTransform().v3.x, 0.6f);
        return true;
    }

    bool simulationCirclesDistanceConstraint(const API &api) {
        core::SimulationInterface::BodyPtr pushed = addTestCircle(api, {0, 0, 0}, {1, 0, 0});
        core::SimulationInterface::BodyPtr follower = addTestCircle(api, {2, 0, 0}, {0, 0, 0});
        core::SimulationInterface::ConstraintPtr constraint = api.simulation->addConstraint(pushed, follower, 2.0f);
        EXPECT_TRUE(constraint != nullptr);
        EXPECT_TRUE(api.simulation->addConstraint(pushed, pushed, 2.0f) == nullptr);

        for (int i = 0; i < 10; i++) {
            api.simulation->update(0.1f);
            const math::vector3f d = follower->getTransform().v3.xyz - pushed->getTransform().v3.xyz;
            EXPECT_LT(std::fabs(d.xz.length() - 2.0f), 1.0e-4f);
        }

        EXPECT_TRUE(follower->getTransform().v3.x > 2.0f);
        return true;
    }

    bool simulationCircleObstacleDistanceConstraint(const API &api) {
        core::SimulationInterface::BodyPtr wall = addTestWall(api);
        core::SimulationInterface::BodyPtr circle = addTestCircle(api, {-2, 0, 0}, {0, 0, 1});
        wall->setTransform(math::transform3f::identity());
        core::SimulationInterface::ConstraintPtr constraint = api.simulation->addConstraint(wall, circle, 2.0f);
        EXPECT_TRUE(constraint != nullptr);
        EXPECT_TRUE(api.simulation->addConstraint(wall, addTestWall(api), 2.0f) == nullptr);

        for (int i = 0; i < 10; i++) {
            api.simulation->update(0.1f);
            EXPECT_LT(std::fabs(circle->getTransform().v3.xz.length() - 2.0f), 1.0e-4f);
        }

        EXPECT_TRUE(circle->getTransform().v3.z > 0.0f);
        return true;
    }

    bool simulationBodyProperties(const API &api) {
        util::Description desc;
        desc.setInteger("type", int(core::SimulationInterface::ShapeType::CircleXZ));
        core::SimulationInterface::BodyPtr defaultCircle = api.simulation->addBody(desc);
        core::SimulationInterface::BodyPtr circle = addTestCircle(api, {0, 0, 0}, {2, 3, -1});
        core::SimulationInterface::BodyPtr wall = addTestWall(api);
        EXPECT_EQ_F(defaultCircle->getRadius(), 1.0f);
        EXPECT_EQ_F(circle->getRadius(), 0.5f);

        EXPECT_EQ_F(circle->getVelocity().x, 2.0f);
        EXPECT_EQ_F(circle->getVelocity().y, 0.0f);
        EXPECT_EQ_F(circle->getVelocity().z, -1.0f);

        defaultCircle->setEnabled(false);
        api.simulation->update(0.5f);
        EXPECT_EQ_F(circle->getVelocity().x, 2.0f);
        EXPECT_EQ_F(circle->getVelocity().y, 0.0f);
        EXPECT_EQ_F(circle->getVelocity().z, -1.0f);

        wall->setVelocity({1, 0, 1});
        EXPECT_EQ_F(wall->getVelocity().x, 0.0f);
        EXPECT_EQ_F(wall->getVelocity().y, 0.0f);
        EXPECT_EQ_F(wall->getVelocity().z, 0.0f);
        return true;
    }
}

namespace test {
    void registerSimulationTests(TestRunner &runner) {
        REGISTER_UNIT_TEST(simulationCircleMovesByVelocity);
        REGISTER_UNIT_TEST(simulationCircleObstacleStopAndSlide);
        REGISTER_UNIT_TEST(simulationCircleObstacleInactiveBodies);
        REGISTER_UNIT_TEST(simulationCircleImpulseFades);
        REGISTER_UNIT_TEST(simulationCircleGravity);
        REGISTER_UNIT_TEST(simulationCircleGravityParabola);
        REGISTER_UNIT_TEST(simulationCirclesCollision);
        REGISTER_UNIT_TEST(simulationCirclesDistanceConstraint);
        REGISTER_UNIT_TEST(simulationCircleObstacleDistanceConstraint);
        REGISTER_UNIT_TEST(simulationBodyProperties);
    }
}
