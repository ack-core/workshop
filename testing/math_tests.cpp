
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
}


namespace test {
    void registerMathTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(math_vector2f_ctor_assignment_test);
        REGISTER_UNIT_TEST(math_vector2f_swizzle_test);
        REGISTER_UNIT_TEST(math_vector3f_ctor_assignment_test);
    }
}
