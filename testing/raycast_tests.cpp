
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
        util::Description alignedDesc;
        alignedDesc.setInteger("type", std::int64_t(core::RaycastInterface::ShapeType::BOXES));
        alignedDesc.setVector3f("points", math::vector3f(0.0f, 0.0f, 5.0f));
        alignedDesc.setVector3f("sizes", math::vector3f(1.0f, 1.0f, 1.0f));
        EXPECT_NOT_EQ(api.raycast->addShape(alignedDesc, 3), nullptr);

        util::Description rotatedDesc;
        rotatedDesc.setInteger("type", std::int64_t(core::RaycastInterface::ShapeType::BOXES));
        rotatedDesc.setVector3f("points", math::vector3f(0.0f, 0.0f, 0.0f));
        rotatedDesc.setVector3f("sizes", math::vector3f(1.0f, 1.0f, 3.0f));
        const core::RaycastInterface::ShapePtr rotated = api.raycast->addShape(rotatedDesc, 4);
        EXPECT_NOT_EQ(rotated, nullptr);
        rotated->setTransform({
            math::vector4f(0.0f, 0.0f, 1.0f, 0.0f),
            math::vector4f(0.0f, 1.0f, 0.0f, 0.0f),
            math::vector4f(-1.0f, 0.0f, 0.0f, 0.0f),
            math::vector4f(10.0f, 0.0f, 0.0f, 1.0f)
        });

        const core::RaycastInterface::RaycastResult alignedHit = api.raycast->rayCast(math::vector3f(0.0f, 0.0f, 0.0f), math::vector3f(0.0f, 0.0f, 1.0f), 10.0f);
        EXPECT_EQ(alignedHit.uniqueId, 3u);
        EXPECT_EQ_F(alignedHit.point.x, 0.0f);
        EXPECT_EQ_F(alignedHit.point.y, 0.0f);
        EXPECT_EQ_F(alignedHit.point.z, 4.5f);
        EXPECT_EQ_F(alignedHit.normal.x, 0.0f);
        EXPECT_EQ_F(alignedHit.normal.y, 0.0f);
        EXPECT_EQ_F(alignedHit.normal.z, -1.0f);

        const core::RaycastInterface::RaycastResult rotatedHit = api.raycast->rayCast(math::vector3f(0.0f, 0.0f, 0.0f), math::vector3f(1.0f, 0.0f, 0.0f), 20.0f);
        EXPECT_EQ(rotatedHit.uniqueId, 4u);
        EXPECT_EQ_F(rotatedHit.point.x, 7.5f);
        EXPECT_EQ_F(rotatedHit.point.y, 0.0f);
        EXPECT_EQ_F(rotatedHit.point.z, 0.0f);
        EXPECT_EQ_F(rotatedHit.normal.x, -1.0f);
        EXPECT_EQ_F(rotatedHit.normal.y, 0.0f);
        EXPECT_EQ_F(rotatedHit.normal.z, 0.0f);

        const core::RaycastInterface::RaycastResult shortMiss = api.raycast->rayCast(math::vector3f(0.0f, 0.0f, 0.0f), math::vector3f(0.0f, 0.0f, 1.0f), 4.0f);
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
