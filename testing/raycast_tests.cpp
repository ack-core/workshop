
#include "runner.h"

namespace test {
    bool raycast_sphere_hit_test(const API &api) {
        util::Description desc;
        desc.setInteger("type", std::int64_t(core::RaycastInterface::ShapeType::SPHERES));
        desc.setVector3f("points", math::vector3f(0.0f, 0.0f, 5.0f));
        desc.setNumber("radiuses", 1.0);
        const core::RaycastInterface::ShapePtr shape = api.raycast->addShape(desc, 7);
        EXPECT_NOT_EQ(shape, nullptr);

        const core::RaycastInterface::RaycastResult hit = api.raycast->rayCast(math::vector3f(0.0f, 0.0f, 0.0f), math::vector3f(0.0f, 0.0f, 1.0f), 10.0f);
        EXPECT_EQ(hit.uniqueId, 7u);
        EXPECT_EQ_F(hit.point.x, 0.0f);
        EXPECT_EQ_F(hit.point.y, 0.0f);
        EXPECT_EQ_F(hit.point.z, 4.0f);
        EXPECT_EQ_F(hit.normal.x, 0.0f);
        EXPECT_EQ_F(hit.normal.y, 0.0f);
        EXPECT_EQ_F(hit.normal.z, -1.0f);

        const core::RaycastInterface::RaycastResult miss = api.raycast->rayCast(math::vector3f(0.0f, 0.0f, 0.0f), math::vector3f(1.0f, 0.0f, 0.0f), 10.0f);
        EXPECT_EQ(miss.uniqueId, core::RaycastInterface::INVALID_UNIQUE_ID);
        return true;
    }

    bool raycast_rotated_box_hit_test(const API &api) {
        util::Description desc;
        desc.setInteger("type", std::int64_t(core::RaycastInterface::ShapeType::BOXES));
        desc.setVector3f("points", math::vector3f(0.0f, 0.0f, 0.0f));
        desc.setVector3f("sizes", math::vector3f(1.0f, 1.0f, 4.0f));
        const core::RaycastInterface::ShapePtr shape = api.raycast->addShape(desc, 4);
        EXPECT_NOT_EQ(shape, nullptr);

        const float c = std::cos(MATH_PI / 6.0f);
        const float s = std::sin(MATH_PI / 6.0f);
        shape->setTransform({
            math::vector4f(c, 0.0f, -s, 0.0f),
            math::vector4f(0.0f, 1.0f, 0.0f, 0.0f),
            math::vector4f(s, 0.0f, c, 0.0f),
            math::vector4f(0.0f, 0.0f, 5.0f, 1.0f)
        });

        const core::RaycastInterface::RaycastResult hit = api.raycast->rayCast(math::vector3f(1.5f, 0.0f, 0.0f), math::vector3f(0.0f, 0.0f, 1.0f), 10.0f);
        EXPECT_EQ(hit.uniqueId, 4u);
        EXPECT_EQ_F(hit.point.x, 1.5f);
        EXPECT_EQ_F(hit.point.y, 0.0f);
        EXPECT_EQ_F(hit.point.z, 4.0f + 3.0f * c);
        EXPECT_EQ_F(hit.normal.x, c);
        EXPECT_EQ_F(hit.normal.y, 0.0f);
        EXPECT_EQ_F(hit.normal.z, -s);

        const core::RaycastInterface::RaycastResult miss = api.raycast->rayCast(math::vector3f(-1.5f, 0.0f, 0.0f), math::vector3f(0.0f, 0.0f, 1.0f), 10.0f);
        EXPECT_EQ(miss.uniqueId, core::RaycastInterface::INVALID_UNIQUE_ID);

        const core::RaycastInterface::RaycastResult shortMiss = api.raycast->rayCast(math::vector3f(1.5f, 0.0f, 0.0f), math::vector3f(0.0f, 0.0f, 1.0f), 6.0f);
        EXPECT_EQ(shortMiss.uniqueId, core::RaycastInterface::INVALID_UNIQUE_ID);
        return true;
    }
}

namespace test {
    void registerRaycastTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(raycast_sphere_hit_test);
        REGISTER_UNIT_TEST(raycast_rotated_box_hit_test);
    }
}
