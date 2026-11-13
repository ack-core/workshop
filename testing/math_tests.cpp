
#include "math_tests.h"

namespace {
    bool eq2(const math::vector2f &v, float x, float y) {
        return v.x == x && v.y == y;
    }
    bool eq3(const math::vector3f &v, float x, float y, float z) {
        return v.x == x && v.y == y && v.z == z;
    }
    bool eq4(const math::vector4f &v, float x, float y, float z, float w) {
        return v.x == x && v.y == y && v.z == z && v.w == w;
    }
    bool near4(const float *r, float x, float y, float z, float w) {
        return std::fabs(r[0] - x) < 1.0e-5f && std::fabs(r[1] - y) < 1.0e-5f && std::fabs(r[2] - z) < 1.0e-5f && std::fabs(r[3] - w) < 1.0e-5f;
    }
}

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
        // ctors
        math::vector4f a(1.0f, 2.0f, 3.0f, 4.0f);
        EXPECT_TRUE(eq4(a, 1.0f, 2.0f, 3.0f, 4.0f));
        EXPECT_EQ_F(a[3], 4.0f);
        EXPECT_EQ_F(a.flat[3], 4.0f);

        math::vector4f b(5.0f);
        EXPECT_TRUE(eq4(b, 5.0f, 5.0f, 5.0f, 5.0f));

        const float arr[4] = {6.0f, 7.0f, 8.0f, 9.0f};
        math::vector4f c(arr);
        EXPECT_TRUE(eq4(c, 6.0f, 7.0f, 8.0f, 9.0f));

        const math::vector3f v3(1.0f, 2.0f, 3.0f);
        EXPECT_TRUE(eq4(math::vector4f(v3, 4.0f), 1.0f, 2.0f, 3.0f, 4.0f));
        EXPECT_TRUE(eq4(math::vector4f(4.0f, v3), 4.0f, 1.0f, 2.0f, 3.0f));

        const math::vector2f v2a(1.0f, 2.0f);
        const math::vector2f v2b(3.0f, 4.0f);
        EXPECT_TRUE(eq4(math::vector4f(5.0f, 6.0f, v2a), 5.0f, 6.0f, 1.0f, 2.0f));
        EXPECT_TRUE(eq4(math::vector4f(5.0f, v2a, 6.0f), 5.0f, 1.0f, 2.0f, 6.0f));
        EXPECT_TRUE(eq4(math::vector4f(v2a, 5.0f, 6.0f), 1.0f, 2.0f, 5.0f, 6.0f));
        EXPECT_TRUE(eq4(math::vector4f(v2a, v2b), 1.0f, 2.0f, 3.0f, 4.0f));

        math::vector4f d(a);
        EXPECT_TRUE(eq4(d, 1.0f, 2.0f, 3.0f, 4.0f));

        // assignment
        math::vector4f e(0.0f);
        e = a;
        EXPECT_TRUE(eq4(e, 1.0f, 2.0f, 3.0f, 4.0f));

        e = b = c;
        EXPECT_TRUE(eq4(b, 6.0f, 7.0f, 8.0f, 9.0f));
        EXPECT_TRUE(eq4(e, 6.0f, 7.0f, 8.0f, 9.0f));

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

    bool math_vector_dot_cross_test(const API &api) {
        const math::vector2f a2(1.0f, 2.0f);
        const math::vector2f b2(3.0f, 4.0f);
        EXPECT_EQ_F(a2.dot(b2), 11.0f);
        EXPECT_EQ_F(a2.cross(b2), -2.0f);
        EXPECT_EQ_F(b2.cross(a2), 2.0f);
        EXPECT_EQ_F(a2.yx.dot(b2), 10.0f);
        EXPECT_EQ_F(a2.yx.cross(b2), 5.0f);

        const math::vector3f a3(1.0f, 2.0f, 3.0f);
        const math::vector3f b3(4.0f, 5.0f, 6.0f);
        EXPECT_EQ_F(a3.dot(b3), 32.0f);
        EXPECT_EQ_F(a3.zyx.dot(b3), 28.0f);

        const math::vector3f c = a3.cross(b3);
        EXPECT_EQ_F(c.x, -3.0f);
        EXPECT_EQ_F(c.y, 6.0f);
        EXPECT_EQ_F(c.z, -3.0f);
        EXPECT_EQ_F(c.dot(a3), 0.0f);
        EXPECT_EQ_F(c.dot(b3), 0.0f);

        const math::vector3f d = b3.cross(a3);
        EXPECT_EQ_F(d.x, 3.0f);
        EXPECT_EQ_F(d.y, -6.0f);
        EXPECT_EQ_F(d.z, 3.0f);

        const math::vector3f e = a3.zyx.cross(b3);
        EXPECT_EQ_F(e.x, 7.0f);
        EXPECT_EQ_F(e.y, -14.0f);
        EXPECT_EQ_F(e.z, 7.0f);

        const math::vector3f z = math::vector3f(1.0f, 0.0f, 0.0f).cross(math::vector3f(0.0f, 1.0f, 0.0f));
        EXPECT_EQ_F(z.x, 0.0f);
        EXPECT_EQ_F(z.y, 0.0f);
        EXPECT_EQ_F(z.z, 1.0f);

        const math::vector4f a4(1.0f, 2.0f, 3.0f, 4.0f);
        const math::vector4f b4(5.0f, 6.0f, 7.0f, 8.0f);
        EXPECT_EQ_F(a4.dot(b4), 70.0f);
        EXPECT_EQ_F(a4.wzyx.dot(b4), 60.0f);

        return true;
    }

    bool math_vector_length_normalize_test(const API &api) {
        const math::vector2f a2(3.0f, -4.0f);
        EXPECT_EQ_F(a2.min(), -4.0f);
        EXPECT_EQ_F(a2.max(), 3.0f);
        EXPECT_EQ_F(a2.length(), 5.0f);
        EXPECT_EQ_F(a2.lengthSq(), 25.0f);
        EXPECT_EQ_F(a2.yx.min(), -4.0f);
        EXPECT_EQ_F(a2.yy.max(), -4.0f);
        EXPECT_EQ_F(a2.xx.length(), std::sqrt(18.0f));
        EXPECT_EQ_F(a2.xx.lengthSq(), 18.0f);
        EXPECT_EQ_F(a2.distanceTo({0.0f, 0.0f}), 5.0f);
        EXPECT_EQ_F(a2.yx.distanceTo({-1.0f, 7.0f}), 5.0f);

        const math::vector2f n2 = a2.normalized();
        EXPECT_EQ_F(n2.x, 0.6f);
        EXPECT_EQ_F(n2.y, -0.8f);
        const math::vector2f s2 = a2.yx.normalized(10.0f);
        EXPECT_EQ_F(s2.x, -8.0f);
        EXPECT_EQ_F(s2.y, 6.0f);
        EXPECT_TRUE(eq2(math::vector2f(0.0f).normalized(), 0.0f, 0.0f));

        const math::vector3f a3(2.0f, -3.0f, 6.0f);
        EXPECT_EQ_F(a3.min(), -3.0f);
        EXPECT_EQ_F(a3.max(), 6.0f);
        EXPECT_EQ_F(a3.length(), 7.0f);
        EXPECT_EQ_F(a3.lengthSq(), 49.0f);
        EXPECT_EQ_F(a3.xxy.min(), -3.0f);
        EXPECT_EQ_F(a3.xxy.max(), 2.0f);
        EXPECT_EQ_F(a3.xxy.lengthSq(), 17.0f);
        EXPECT_EQ_F(a3.zx.length(), std::sqrt(40.0f));
        EXPECT_EQ_F(a3.distanceTo({0.0f, 0.0f, 0.0f}), 7.0f);
        EXPECT_EQ_F(a3.zyx.distanceTo({9.0f, 0.0f, 5.0f}), 3.0f * std::sqrt(3.0f));

        const math::vector3f n3 = a3.normalized();
        EXPECT_EQ_F(n3.x, 2.0f / 7.0f);
        EXPECT_EQ_F(n3.y, -3.0f / 7.0f);
        EXPECT_EQ_F(n3.z, 6.0f / 7.0f);
        EXPECT_EQ_F(n3.length(), 1.0f);
        EXPECT_EQ_F(a3.zyx.normalized(14.0f).x, 12.0f);
        EXPECT_EQ_F(a3.zyx.normalized(14.0f).z, 4.0f);
        EXPECT_TRUE(eq3(math::vector3f(0.0f).normalized(), 0.0f, 0.0f, 0.0f));

        EXPECT_TRUE(eq3(a3.signs(), 1.0f, -1.0f, 1.0f));
        EXPECT_TRUE(eq3(a3.yzx.signs(), -1.0f, 1.0f, 1.0f));
        EXPECT_TRUE(eq3(math::vector3f(0.0f, -0.0f, 0.0f).signs(), 1.0f, -1.0f, 1.0f));

        return true;
    }

    bool math_transform3f_identity_multiply_test(const API &api) {
        const math::transform3f id = math::transform3f::identity();
        EXPECT_TRUE(near4(id.row0, 1.0f, 0.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near4(id.row1, 0.0f, 1.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near4(id.row2, 0.0f, 0.0f, 1.0f, 0.0f));
        EXPECT_TRUE(near4(id.row3, 0.0f, 0.0f, 0.0f, 1.0f));

        const math::transform3f a({1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}, {13, 14, 15, 16});
        EXPECT_EQ_F(a.m12, 2.0f);
        EXPECT_EQ_F(a.m23, 7.0f);
        EXPECT_EQ_F(a.flat[8], 9.0f);
        EXPECT_EQ_F(a.v3.w, 16.0f);

        math::transform3f c;
        c = a;
        EXPECT_TRUE(near4(c.row2, 9.0f, 10.0f, 11.0f, 12.0f));

        const math::transform3f ai = a * id;
        EXPECT_TRUE(near4(ai.row0, 1.0f, 2.0f, 3.0f, 4.0f));
        EXPECT_TRUE(near4(ai.row3, 13.0f, 14.0f, 15.0f, 16.0f));
        const math::transform3f ia = id * a;
        EXPECT_TRUE(near4(ia.row1, 5.0f, 6.0f, 7.0f, 8.0f));
        EXPECT_TRUE(near4(ia.row2, 9.0f, 10.0f, 11.0f, 12.0f));

        const math::transform3f b({2, 0, 0, 0}, {0, 3, 0, 0}, {0, 0, 4, 0}, {1, 2, 3, 1});
        const math::transform3f ab = a * b;
        EXPECT_TRUE(near4(ab.row0, 6.0f, 14.0f, 24.0f, 4.0f));
        EXPECT_TRUE(near4(ab.row3, 42.0f, 74.0f, 108.0f, 16.0f));
        const math::transform3f ba = b * a;
        EXPECT_TRUE(near4(ba.row0, 2.0f, 4.0f, 6.0f, 8.0f));
        EXPECT_TRUE(near4(ba.row3, 51.0f, 58.0f, 65.0f, 72.0f));

        const math::transform3f r({0, 0, 1}, MATH_PI * 0.5f);
        EXPECT_TRUE(near4(r.row0, 0.0f, -1.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near4(r.row1, 1.0f, 0.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near4(r.row2, 0.0f, 0.0f, 1.0f, 0.0f));
        EXPECT_TRUE(near4(r.row3, 0.0f, 0.0f, 0.0f, 1.0f));

        const math::transform3f rr = r * r;
        EXPECT_TRUE(near4(rr.row0, -1.0f, 0.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near4(rr.row1, 0.0f, -1.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near4(rr.row2, 0.0f, 0.0f, 1.0f, 0.0f));

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
        REGISTER_UNIT_TEST(math_vector_dot_cross_test);
        REGISTER_UNIT_TEST(math_vector_length_normalize_test);
        REGISTER_UNIT_TEST(math_transform3f_identity_multiply_test);
    }
}
