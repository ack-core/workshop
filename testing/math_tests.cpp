
#include "math_tests.h"

namespace test {
    bool math_vector2f_ctor_assignment_test(const API &api) {
        // ctors
        math::vector2f a(1.0f, 2.0f);
        EXPECT_EQ_F(a.x, 1.0f);
        EXPECT_EQ_F(a.y, 2.0f);
        EXPECT_EQ_F(a[0], 1.0f);
        EXPECT_EQ_F(a[1], 2.0f);
        EXPECT_EQ_F(a.flat[0], 1.0f);
        EXPECT_EQ_F(a.flat[1], 2.0f);

        math::vector2f b(3.0f);
        EXPECT_EQ_F(b.x, 3.0f);
        EXPECT_EQ_F(b.y, 3.0f);

        const float arr[2] = {4.0f, 5.0f};
        math::vector2f c(arr);
        EXPECT_EQ_F(c.x, 4.0f);
        EXPECT_EQ_F(c.y, 5.0f);

        math::vector2f d(a);
        EXPECT_EQ_F(d.x, 1.0f);
        EXPECT_EQ_F(d.y, 2.0f);

        // assignment
        math::vector2f e(0.0f);
        e = a;
        EXPECT_EQ_F(e.x, 1.0f);
        EXPECT_EQ_F(e.y, 2.0f);
        EXPECT_NOT_EQ_F(e.x, e.y);

        e = b = c;
        EXPECT_EQ_F(b.x, 4.0f);
        EXPECT_EQ_F(b.y, 5.0f);
        EXPECT_EQ_F(e.x, 4.0f);
        EXPECT_EQ_F(e.y, 5.0f);

        return true;
    }

    bool math_vector2f_swizzle_test(const API &api) {
        const math::vector2f a(1.0f, 2.0f);

        // swizzle read
        math::vector2f yx = a.yx;
        EXPECT_LT(yx.y, yx.x);
        EXPECT_EQ_F(yx.x, 2.0f);
        EXPECT_EQ_F(yx.y, 1.0f);
        EXPECT_EQ_F(math::vector2f(a.yx)[0], 2.0f);
        EXPECT_EQ_F(math::vector2f(a.yx)[1], 1.0f);

        math::vector2f xx = a.xx;
        EXPECT_EQ_F(xx.x, 1.0f);
        EXPECT_EQ_F(xx.y, 1.0f);

        math::vector2f yy = a.yy;
        EXPECT_EQ_F(yy.x, 2.0f);
        EXPECT_EQ_F(yy.y, 2.0f);

        // swizzle write from vector
        math::vector2f f(0.0f);
        f.yx = a;
        EXPECT_LT(f.y, f.x);
        EXPECT_EQ_F(f.x, 2.0f);
        EXPECT_EQ_F(f.y, 1.0f);

        // swizzle write from same swizzle
        math::vector2f g(0.0f);
        g.yx = a.yx;
        EXPECT_LT(g.x, g.y);
        EXPECT_EQ_F(g.x, 1.0f);
        EXPECT_EQ_F(g.y, 2.0f);

        // swizzle write from other swizzle
        math::vector2f h(0.0f);
        h.yx = a.yy;
        EXPECT_EQ_F(h.x, 2.0f);
        EXPECT_EQ_F(h.y, 2.0f);

        // chained swizzle assignment
        const math::vector2f p(1.0f, 2.0f);
        math::vector2f m(3.0f, 4.0f);
        math::vector2f n(0.0f);
        n.yx = m.xx = p.yy;
        EXPECT_EQ_F(m.x, 2.0f);
        EXPECT_EQ_F(m.y, 4.0f);
        EXPECT_EQ_F(n.x, 4.0f);
        EXPECT_EQ_F(n.y, 2.0f);

        // swizzle to vector assignment
        math::vector2f k(0.0f);
        k = a.yx;
        EXPECT_LT(k.y, k.x);
        EXPECT_EQ_F(k.x, 2.0f);
        EXPECT_EQ_F(k.y, 1.0f);

        return true;
    }

    bool math_vector3f_ctor_assignment_test(const API &api) {
        // ctors
        math::vector3f a(1.0f, 2.0f, 3.0f);
        EXPECT_EQ_F(a.x, 1.0f);
        EXPECT_EQ_F(a.y, 2.0f);
        EXPECT_EQ_F(a.z, 3.0f);
        EXPECT_EQ_F(a[2], 3.0f);
        EXPECT_EQ_F(a.flat[2], 3.0f);

        math::vector3f b(4.0f);
        EXPECT_EQ_F(b.x, 4.0f);
        EXPECT_EQ_F(b.y, 4.0f);
        EXPECT_EQ_F(b.z, 4.0f);

        const float arr[3] = {5.0f, 6.0f, 7.0f};
        math::vector3f c(arr);
        EXPECT_EQ_F(c.x, 5.0f);
        EXPECT_EQ_F(c.y, 6.0f);
        EXPECT_EQ_F(c.z, 7.0f);

        const math::vector2f v2(8.0f, 9.0f);
        math::vector3f d(v2, 10.0f);
        EXPECT_EQ_F(d.x, 8.0f);
        EXPECT_EQ_F(d.y, 9.0f);
        EXPECT_EQ_F(d.z, 10.0f);

        math::vector3f e(10.0f, v2);
        EXPECT_EQ_F(e.x, 10.0f);
        EXPECT_EQ_F(e.y, 8.0f);
        EXPECT_EQ_F(e.z, 9.0f);

        math::vector3f f(a);
        EXPECT_EQ_F(f.x, 1.0f);
        EXPECT_EQ_F(f.y, 2.0f);
        EXPECT_EQ_F(f.z, 3.0f);

        // assignment
        math::vector3f g(0.0f);
        g = a;
        EXPECT_EQ_F(g.x, 1.0f);
        EXPECT_EQ_F(g.y, 2.0f);
        EXPECT_EQ_F(g.z, 3.0f);

        g = b = c;
        EXPECT_EQ_F(b.z, 7.0f);
        EXPECT_EQ_F(g.x, 5.0f);
        EXPECT_EQ_F(g.y, 6.0f);
        EXPECT_EQ_F(g.z, 7.0f);

        return true;
    }

    bool math_vector4f_ctor_assignment_test(const API &api) {
        auto eq = [](const math::vector4f &v, float x, float y, float z, float w) {
            return v.x == x && v.y == y && v.z == z && v.w == w;
        };

        // ctors
        math::vector4f a(1.0f, 2.0f, 3.0f, 4.0f);
        EXPECT_TRUE(eq(a, 1.0f, 2.0f, 3.0f, 4.0f));
        EXPECT_EQ_F(a[3], 4.0f);
        EXPECT_EQ_F(a.flat[3], 4.0f);

        math::vector4f b(5.0f);
        EXPECT_TRUE(eq(b, 5.0f, 5.0f, 5.0f, 5.0f));

        const float arr[4] = {6.0f, 7.0f, 8.0f, 9.0f};
        math::vector4f c(arr);
        EXPECT_TRUE(eq(c, 6.0f, 7.0f, 8.0f, 9.0f));

        const math::vector3f v3(1.0f, 2.0f, 3.0f);
        EXPECT_TRUE(eq(math::vector4f(v3, 4.0f), 1.0f, 2.0f, 3.0f, 4.0f));
        EXPECT_TRUE(eq(math::vector4f(4.0f, v3), 4.0f, 1.0f, 2.0f, 3.0f));

        const math::vector2f v2a(1.0f, 2.0f);
        const math::vector2f v2b(3.0f, 4.0f);
        EXPECT_TRUE(eq(math::vector4f(5.0f, 6.0f, v2a), 5.0f, 6.0f, 1.0f, 2.0f));
        EXPECT_TRUE(eq(math::vector4f(5.0f, v2a, 6.0f), 5.0f, 1.0f, 2.0f, 6.0f));
        EXPECT_TRUE(eq(math::vector4f(v2a, 5.0f, 6.0f), 1.0f, 2.0f, 5.0f, 6.0f));
        EXPECT_TRUE(eq(math::vector4f(v2a, v2b), 1.0f, 2.0f, 3.0f, 4.0f));

        math::vector4f d(a);
        EXPECT_TRUE(eq(d, 1.0f, 2.0f, 3.0f, 4.0f));

        // assignment
        math::vector4f e(0.0f);
        e = a;
        EXPECT_TRUE(eq(e, 1.0f, 2.0f, 3.0f, 4.0f));

        e = b = c;
        EXPECT_TRUE(eq(b, 6.0f, 7.0f, 8.0f, 9.0f));
        EXPECT_TRUE(eq(e, 6.0f, 7.0f, 8.0f, 9.0f));

        math::vector2f f(0.0f);
        f = math::vector4f(1.0f, 2.0f, 3.0f, 4.0f).xy;
        EXPECT_EQ_F(f.x, 1.0f);
        EXPECT_EQ_F(f.y, 2.0f);

        math::vector3f g(0.0f);
        g = math::vector4f(1.0f, 2.0f, 3.0f, 4.0f).xyz;
        EXPECT_EQ_F(g.x, 1.0f);
        EXPECT_EQ_F(g.y, 2.0f);
        EXPECT_EQ_F(g.z, 3.0f);

        return true;
    }

    bool math_vector_arithmetic_test(const API &api) {
        auto eq2 = [](const math::vector2f &v, float x, float y) { return v.x == x && v.y == y; };
        auto eq3 = [](const math::vector3f &v, float x, float y, float z) { return v.x == x && v.y == y && v.z == z; };
        auto eq4 = [](const math::vector4f &v, float x, float y, float z, float w) { return v.x == x && v.y == y && v.z == z && v.w == w; };

        // vector2f
        const math::vector2f a2(8.0f, 6.0f);
        const math::vector2f b2(2.0f, 3.0f);
        EXPECT_TRUE(eq2(a2 + b2, 10.0f, 9.0f));
        EXPECT_TRUE(eq2(a2 - b2, 6.0f, 3.0f));
        EXPECT_TRUE(eq2(a2 * b2, 16.0f, 18.0f));
        EXPECT_TRUE(eq2(a2 / b2, 4.0f, 2.0f));
        EXPECT_TRUE(eq2(a2 * 2.0f, 16.0f, 12.0f));
        EXPECT_TRUE(eq2(2.0f * a2.yx, 12.0f, 16.0f));
        EXPECT_TRUE(eq2(a2.yx / 2.0f, 3.0f, 4.0f));
        EXPECT_TRUE(eq2(24.0f / a2, 3.0f, 4.0f));
        EXPECT_TRUE(eq2(a2.yx + b2, 8.0f, 11.0f));
        EXPECT_TRUE(eq2(a2.yx - b2.yx, 3.0f, 6.0f));
        EXPECT_TRUE(eq2(a2.yy * b2.xx, 12.0f, 12.0f));
        EXPECT_TRUE(eq2(a2.yx / b2.yx, 2.0f, 4.0f));

        // vector3f
        const math::vector3f a3(8.0f, 6.0f, 4.0f);
        const math::vector3f b3(2.0f, 3.0f, 4.0f);
        EXPECT_TRUE(eq3(a3 + b3, 10.0f, 9.0f, 8.0f));
        EXPECT_TRUE(eq3(a3 - b3, 6.0f, 3.0f, 0.0f));
        EXPECT_TRUE(eq3(a3 * b3, 16.0f, 18.0f, 16.0f));
        EXPECT_TRUE(eq3(a3 / b3, 4.0f, 2.0f, 1.0f));
        EXPECT_TRUE(eq3(a3 * 2.0f, 16.0f, 12.0f, 8.0f));
        EXPECT_TRUE(eq3(2.0f * a3.zyx, 8.0f, 12.0f, 16.0f));
        EXPECT_TRUE(eq3(a3.zyx / 2.0f, 2.0f, 3.0f, 4.0f));
        EXPECT_TRUE(eq3(24.0f / a3, 3.0f, 4.0f, 6.0f));
        EXPECT_TRUE(eq3(a3.zyx + b3, 6.0f, 9.0f, 12.0f));
        EXPECT_TRUE(eq3(a3.zyx - b3.zxy, 0.0f, 4.0f, 5.0f));
        EXPECT_TRUE(eq3(a3.zyx * b3, 8.0f, 18.0f, 32.0f));
        EXPECT_TRUE(eq3(a3.zyx / b3.zyx, 1.0f, 2.0f, 4.0f));
        EXPECT_TRUE(eq2(a3.zx + b3.xy, 6.0f, 11.0f));

        // vector4f
        const math::vector4f a4(8.0f, 6.0f, 4.0f, 2.0f);
        const math::vector4f b4(2.0f, 3.0f, 4.0f, 1.0f);
        EXPECT_TRUE(eq4(a4 + b4, 10.0f, 9.0f, 8.0f, 3.0f));
        EXPECT_TRUE(eq4(a4 - b4, 6.0f, 3.0f, 0.0f, 1.0f));
        EXPECT_TRUE(eq4(a4 * b4, 16.0f, 18.0f, 16.0f, 2.0f));
        EXPECT_TRUE(eq4(a4 / b4, 4.0f, 2.0f, 1.0f, 2.0f));
        EXPECT_TRUE(eq4(a4 * 2.0f, 16.0f, 12.0f, 8.0f, 4.0f));
        EXPECT_TRUE(eq4(2.0f * a4.wzyx, 4.0f, 8.0f, 12.0f, 16.0f));
        EXPECT_TRUE(eq4(a4.wzyx / 2.0f, 1.0f, 2.0f, 3.0f, 4.0f));
        EXPECT_TRUE(eq4(24.0f / a4, 3.0f, 4.0f, 6.0f, 12.0f));
        EXPECT_TRUE(eq4(a4.wzyx + b4, 4.0f, 7.0f, 10.0f, 9.0f));
        EXPECT_TRUE(eq4(a4.wzyx - b4.wzyx, 1.0f, 0.0f, 3.0f, 6.0f));
        EXPECT_TRUE(eq4(a4.wzyx * b4, 4.0f, 12.0f, 24.0f, 8.0f));
        EXPECT_TRUE(eq4(a4.wzyx / b4.wzyx, 2.0f, 1.0f, 2.0f, 4.0f));
        EXPECT_TRUE(eq2(a4.wy * b4.zx, 8.0f, 12.0f));
        EXPECT_TRUE(eq3(a4.xyz - b4.zyx, 4.0f, 3.0f, 2.0f));

        return true;
    }
}


namespace test {
    void registerMathTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(math_vector2f_ctor_assignment_test);
        REGISTER_UNIT_TEST(math_vector2f_swizzle_test);
        REGISTER_UNIT_TEST(math_vector3f_ctor_assignment_test);
        REGISTER_UNIT_TEST(math_vector4f_ctor_assignment_test);
        REGISTER_UNIT_TEST(math_vector_arithmetic_test);
    }
}
