
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
}

namespace test {
    void registerRaycastTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(raycast_sphere_hit_test);
    }
}
