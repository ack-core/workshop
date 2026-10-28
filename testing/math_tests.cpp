
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

    bool math_vector_arithmetic_operators_test(const API &api) {
        // vector2f
        const math::vector2f a2(1.0f, 2.0f);
        const math::vector2f b2(4.0f, 8.0f);

        math::vector2f r2 = a2 + b2;
        EXPECT_EQ_F(r2.x, 5.0f);
        EXPECT_EQ_F(r2.y, 10.0f);

        r2 = b2 - a2;
        EXPECT_EQ_F(r2.x, 3.0f);
        EXPECT_EQ_F(r2.y, 6.0f);

        r2 = a2 * b2;
        EXPECT_EQ_F(r2.x, 4.0f);
        EXPECT_EQ_F(r2.y, 16.0f);

        r2 = b2 / a2;
        EXPECT_EQ_F(r2.x, 4.0f);
        EXPECT_EQ_F(r2.y, 4.0f);

        r2 = a2 * 3.0f;
        EXPECT_EQ_F(r2.x, 3.0f);
        EXPECT_EQ_F(r2.y, 6.0f);

        r2 = 3.0f * a2;
        EXPECT_EQ_F(r2.x, 3.0f);
        EXPECT_EQ_F(r2.y, 6.0f);

        r2 = b2 / 2.0f;
        EXPECT_EQ_F(r2.x, 2.0f);
        EXPECT_EQ_F(r2.y, 4.0f);

        r2 = 8.0f / b2;
        EXPECT_EQ_F(r2.x, 2.0f);
        EXPECT_EQ_F(r2.y, 1.0f);

        // vector2f swizzles
        r2 = a2.yx - a2;
        EXPECT_EQ_F(r2.x, 1.0f);
        EXPECT_EQ_F(r2.y, -1.0f);

        r2 = a2.yy + b2.xx;
        EXPECT_EQ_F(r2.x, 6.0f);
        EXPECT_EQ_F(r2.y, 6.0f);

        r2 = b2.yx / a2.yx;
        EXPECT_EQ_F(r2.x, 4.0f);
        EXPECT_EQ_F(r2.y, 4.0f);

        r2 = a2.yx * 2.0f;
        EXPECT_EQ_F(r2.x, 4.0f);
        EXPECT_EQ_F(r2.y, 2.0f);

        // vector3f
        const math::vector3f a3(1.0f, 2.0f, 4.0f);
        const math::vector3f b3(8.0f, 6.0f, 2.0f);

        math::vector3f r3 = a3 + b3;
        EXPECT_EQ_F(r3.x, 9.0f);
        EXPECT_EQ_F(r3.y, 8.0f);
        EXPECT_EQ_F(r3.z, 6.0f);

        r3 = a3 - b3;
        EXPECT_EQ_F(r3.x, -7.0f);
        EXPECT_EQ_F(r3.y, -4.0f);
        EXPECT_EQ_F(r3.z, 2.0f);

        r3 = a3 * b3;
        EXPECT_EQ_F(r3.x, 8.0f);
        EXPECT_EQ_F(r3.y, 12.0f);
        EXPECT_EQ_F(r3.z, 8.0f);

        r3 = b3 / a3;
        EXPECT_EQ_F(r3.x, 8.0f);
        EXPECT_EQ_F(r3.y, 3.0f);
        EXPECT_EQ_F(r3.z, 0.5f);

        r3 = a3 * 2.0f;
        EXPECT_EQ_F(r3.x, 2.0f);
        EXPECT_EQ_F(r3.y, 4.0f);
        EXPECT_EQ_F(r3.z, 8.0f);

        r3 = 2.0f * a3;
        EXPECT_EQ_F(r3.x, 2.0f);
        EXPECT_EQ_F(r3.y, 4.0f);
        EXPECT_EQ_F(r3.z, 8.0f);

        r3 = a3 / 2.0f;
        EXPECT_EQ_F(r3.x, 0.5f);
        EXPECT_EQ_F(r3.y, 1.0f);
        EXPECT_EQ_F(r3.z, 2.0f);

        r3 = 8.0f / a3;
        EXPECT_EQ_F(r3.x, 8.0f);
        EXPECT_EQ_F(r3.y, 4.0f);
        EXPECT_EQ_F(r3.z, 2.0f);

        // vector3f swizzles
        r3 = a3.zyx + a3;
        EXPECT_EQ_F(r3.x, 5.0f);
        EXPECT_EQ_F(r3.y, 4.0f);
        EXPECT_EQ_F(r3.z, 5.0f);

        r3 = b3.zxy - a3.xxx;
        EXPECT_EQ_F(r3.x, 1.0f);
        EXPECT_EQ_F(r3.y, 7.0f);
        EXPECT_EQ_F(r3.z, 5.0f);

        math::vector2f s2 = a3.xz * b3.zy;
        EXPECT_EQ_F(s2.x, 2.0f);
        EXPECT_EQ_F(s2.y, 24.0f);

        s2 = 4.0f / a3.zy;
        EXPECT_EQ_F(s2.x, 1.0f);
        EXPECT_EQ_F(s2.y, 2.0f);

        // vector4f
        const math::vector4f a4(1.0f, 2.0f, 4.0f, 8.0f);
        const math::vector4f b4(2.0f, 4.0f, 2.0f, 4.0f);

        math::vector4f r4 = a4 + b4;
        EXPECT_EQ_F(r4.x, 3.0f);
        EXPECT_EQ_F(r4.y, 6.0f);
        EXPECT_EQ_F(r4.z, 6.0f);
        EXPECT_EQ_F(r4.w, 12.0f);

        r4 = a4 - b4;
        EXPECT_EQ_F(r4.x, -1.0f);
        EXPECT_EQ_F(r4.y, -2.0f);
        EXPECT_EQ_F(r4.z, 2.0f);
        EXPECT_EQ_F(r4.w, 4.0f);

        r4 = a4 * b4;
        EXPECT_EQ_F(r4.x, 2.0f);
        EXPECT_EQ_F(r4.y, 8.0f);
        EXPECT_EQ_F(r4.z, 8.0f);
        EXPECT_EQ_F(r4.w, 32.0f);

        r4 = a4 / b4;
        EXPECT_EQ_F(r4.x, 0.5f);
        EXPECT_EQ_F(r4.y, 0.5f);
        EXPECT_EQ_F(r4.z, 2.0f);
        EXPECT_EQ_F(r4.w, 2.0f);

        r4 = a4 * 0.5f;
        EXPECT_EQ_F(r4.x, 0.5f);
        EXPECT_EQ_F(r4.y, 1.0f);
        EXPECT_EQ_F(r4.z, 2.0f);
        EXPECT_EQ_F(r4.w, 4.0f);

        r4 = 0.5f * a4;
        EXPECT_EQ_F(r4.x, 0.5f);
        EXPECT_EQ_F(r4.y, 1.0f);
        EXPECT_EQ_F(r4.z, 2.0f);
        EXPECT_EQ_F(r4.w, 4.0f);

        r4 = a4 / 4.0f;
        EXPECT_EQ_F(r4.x, 0.25f);
        EXPECT_EQ_F(r4.y, 0.5f);
        EXPECT_EQ_F(r4.z, 1.0f);
        EXPECT_EQ_F(r4.w, 2.0f);

        r4 = 8.0f / a4;
        EXPECT_EQ_F(r4.x, 8.0f);
        EXPECT_EQ_F(r4.y, 4.0f);
        EXPECT_EQ_F(r4.z, 2.0f);
        EXPECT_EQ_F(r4.w, 1.0f);

        // vector4f swizzles
        r4 = a4.wzyx + a4;
        EXPECT_EQ_F(r4.x, 9.0f);
        EXPECT_EQ_F(r4.y, 6.0f);
        EXPECT_EQ_F(r4.z, 6.0f);
        EXPECT_EQ_F(r4.w, 9.0f);

        r4 = a4.wzyx - b4.xxyy;
        EXPECT_EQ_F(r4.x, 6.0f);
        EXPECT_EQ_F(r4.y, 2.0f);
        EXPECT_EQ_F(r4.z, -2.0f);
        EXPECT_EQ_F(r4.w, -3.0f);

        r3 = a4.wyz / b4.xyx;
        EXPECT_EQ_F(r3.x, 4.0f);
        EXPECT_EQ_F(r3.y, 0.5f);
        EXPECT_EQ_F(r3.z, 2.0f);

        r3 = a4.xyw * 2.0f;
        EXPECT_EQ_F(r3.x, 2.0f);
        EXPECT_EQ_F(r3.y, 4.0f);
        EXPECT_EQ_F(r3.z, 16.0f);

        // chained expression
        r3 = (a3 + b3) * 2.0f - a3 / 0.5f;
        EXPECT_EQ_F(r3.x, 16.0f);
        EXPECT_EQ_F(r3.y, 12.0f);
        EXPECT_EQ_F(r3.z, 4.0f);

        return true;
    }
}


namespace test {
    void registerMathTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(math_vector2f_ctor_assignment_test);
        REGISTER_UNIT_TEST(math_vector2f_swizzle_test);
        REGISTER_UNIT_TEST(math_vector_arithmetic_operators_test);
    }
}
