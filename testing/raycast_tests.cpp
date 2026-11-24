
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

    bool raycast_mask_and_enabled_test(const API &api) {
        util::Description desc;
        desc.setInteger("type", std::int64_t(core::RaycastInterface::ShapeType::SPHERES));
        desc.setNumber("radiuses", 1.0);
        desc.setVector3f("points", math::vector3f(20.0f, 0.0f, 5.0f));
        const core::RaycastInterface::ShapePtr nearShape = api.raycast->addShape(desc, 11, 1);
        desc.setVector3f("points", math::vector3f(20.0f, 0.0f, 10.0f));
        const core::RaycastInterface::ShapePtr farShape = api.raycast->addShape(desc, 12, 2);

        const math::vector3f start = math::vector3f(20.0f, 0.0f, 0.0f);
        const math::vector3f dir = math::vector3f(0.0f, 0.0f, 1.0f);
        EXPECT_EQ(api.raycast->rayCast(start, dir, 20.0f).uniqueId, 11u);
        EXPECT_EQ(api.raycast->rayCast(start, dir, 20.0f, 2).uniqueId, 12u);
        EXPECT_EQ(api.raycast->rayCast(start, dir, 20.0f, 4).uniqueId, core::RaycastInterface::INVALID_UNIQUE_ID);

        nearShape->setEnabled(false);
        EXPECT_EQ(api.raycast->rayCast(start, dir, 20.0f).uniqueId, 12u);
        nearShape->setEnabled(true);

        nearShape->setTransform(math::transform3f::identity().translated(math::vector3f(0.0f, 5.0f, 0.0f)));
        EXPECT_EQ(api.raycast->rayCast(start, dir, 20.0f).uniqueId, 12u);
        const core::RaycastInterface::RaycastResult moved = api.raycast->rayCast(math::vector3f(20.0f, 5.0f, 0.0f), dir, 20.0f);
        EXPECT_EQ(moved.uniqueId, 11u);
        EXPECT_EQ_F(moved.point.z, 4.0f);
        return true;
    }

    bool raycast_compound_spheres_test(const API &api) {
        util::Description desc;
        desc.setInteger("type", std::int64_t(core::RaycastInterface::ShapeType::SPHERES));
        desc.setVector3f("points", math::vector3f(40.0f, 0.0f, 5.0f), false);
        desc.setVector3f("points", math::vector3f(40.0f, 0.0f, 10.0f), false);
        desc.setVector3f("points", math::vector3f(43.0f, 0.0f, 5.0f), false);
        desc.setNumber("radiuses", 1.0, false);
        desc.setNumber("radiuses", 2.0, false);
        desc.setNumber("radiuses", 0.5, false);
        const core::RaycastInterface::ShapePtr shape = api.raycast->addShape(desc, 21);
        EXPECT_NOT_EQ(shape, nullptr);

        const core::RaycastInterface::RaycastResult nearest = api.raycast->rayCast(math::vector3f(40.0f, 0.0f, 0.0f), math::vector3f(0.0f, 0.0f, 1.0f), 20.0f);
        EXPECT_EQ(nearest.uniqueId, 21u);
        EXPECT_EQ_F(nearest.point.z, 4.0f);
        EXPECT_EQ_F(nearest.normal.z, -1.0f);

        const core::RaycastInterface::RaycastResult back = api.raycast->rayCast(math::vector3f(40.0f, 0.0f, 20.0f), math::vector3f(0.0f, 0.0f, -1.0f), 20.0f);
        EXPECT_EQ(back.uniqueId, 21u);
        EXPECT_EQ_F(back.point.z, 12.0f);
        EXPECT_EQ_F(back.normal.z, 1.0f);

        const core::RaycastInterface::RaycastResult side = api.raycast->rayCast(math::vector3f(43.0f, 0.0f, 0.0f), math::vector3f(0.0f, 0.0f, 1.0f), 20.0f);
        EXPECT_EQ(side.uniqueId, 21u);
        EXPECT_EQ_F(side.point.x, 43.0f);
        EXPECT_EQ_F(side.point.z, 4.5f);
        EXPECT_EQ_F(side.normal.z, -1.0f);
        return true;
    }

    bool raycast_compound_boxes_test(const API &api) {
        util::Description desc;
        desc.setInteger("type", std::int64_t(core::RaycastInterface::ShapeType::BOXES));
        desc.setVector3f("points", math::vector3f(40.0f, 20.0f, 5.0f), false);
        desc.setVector3f("points", math::vector3f(40.0f, 20.0f, 10.0f), false);
        desc.setVector3f("points", math::vector3f(43.0f, 20.0f, 5.0f), false);
        desc.setVector3f("sizes", math::vector3f(1.0f, 1.0f, 1.0f), false);
        desc.setVector3f("sizes", math::vector3f(2.0f, 2.0f, 2.0f), false);
        desc.setVector3f("sizes", math::vector3f(1.0f, 1.0f, 1.0f), false);
        const core::RaycastInterface::ShapePtr shape = api.raycast->addShape(desc, 22);
        EXPECT_NOT_EQ(shape, nullptr);

        const core::RaycastInterface::RaycastResult nearest = api.raycast->rayCast(math::vector3f(40.0f, 20.0f, 0.0f), math::vector3f(0.0f, 0.0f, 1.0f), 20.0f);
        EXPECT_EQ(nearest.uniqueId, 22u);
        EXPECT_EQ_F(nearest.point.z, 4.5f);
        EXPECT_EQ_F(nearest.normal.z, -1.0f);

        const core::RaycastInterface::RaycastResult back = api.raycast->rayCast(math::vector3f(40.0f, 20.0f, 20.0f), math::vector3f(0.0f, 0.0f, -1.0f), 20.0f);
        EXPECT_EQ(back.uniqueId, 22u);
        EXPECT_EQ_F(back.point.z, 11.5f);
        EXPECT_EQ_F(back.normal.z, 1.0f);

        const core::RaycastInterface::RaycastResult left = api.raycast->rayCast(math::vector3f(30.0f, 20.0f, 5.0f), math::vector3f(1.0f, 0.0f, 0.0f), 20.0f);
        EXPECT_EQ(left.uniqueId, 22u);
        EXPECT_EQ_F(left.point.x, 39.5f);
        EXPECT_EQ_F(left.normal.x, -1.0f);

        const core::RaycastInterface::RaycastResult right = api.raycast->rayCast(math::vector3f(50.0f, 20.0f, 5.0f), math::vector3f(-1.0f, 0.0f, 0.0f), 20.0f);
        EXPECT_EQ(right.uniqueId, 22u);
        EXPECT_EQ_F(right.point.x, 43.5f);
        EXPECT_EQ_F(right.normal.x, 1.0f);
        return true;
    }
}

namespace test {
    void registerRaycastTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(raycast_sphere_hit_test);
        REGISTER_UNIT_TEST(raycast_rotated_box_hit_test);
        REGISTER_UNIT_TEST(raycast_mask_and_enabled_test);
        REGISTER_UNIT_TEST(raycast_compound_spheres_test);
        REGISTER_UNIT_TEST(raycast_compound_boxes_test);
    }
}
