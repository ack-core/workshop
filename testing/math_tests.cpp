
#include "runner.h"

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
    bool near2(const math::vector2f &v, float x, float y) {
        return std::fabs(v.x - x) < 1.0e-5f && std::fabs(v.y - y) < 1.0e-5f;
    }
    bool near3(const math::vector3f &v, float x, float y, float z) {
        return std::fabs(v.x - x) < 1.0e-5f && std::fabs(v.y - y) < 1.0e-5f && std::fabs(v.z - z) < 1.0e-5f;
    }
    bool near4(const math::vector4f &v, float x, float y, float z, float w) {
        return std::fabs(v.x - x) < 1.0e-5f && std::fabs(v.y - y) < 1.0e-5f && std::fabs(v.z - z) < 1.0e-5f && std::fabs(v.w - w) < 1.0e-5f;
    }
    bool eq2i(const math::vector2i &v, int x, int y) {
        return v.x == x && v.y == y;
    }
    bool eq3i(const math::vector3i &v, int x, int y, int z) {
        return v.x == x && v.y == y && v.z == z;
    }
    bool eq4i(const math::vector4i &v, int x, int y, int z, int w) {
        return v.x == x && v.y == y && v.z == z && v.w == w;
    }
    bool nearBound(const math::bound3f &b, float xmin, float ymin, float zmin, float xmax, float ymax, float zmax) {
        return near3({b.xmin, b.ymin, b.zmin}, xmin, ymin, zmin) && near3({b.xmax, b.ymax, b.zmax}, xmax, ymax, zmax);
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

    bool math_vector_compound_assignment_test(const API &api) {
        // vector2f
        math::vector2f v2(8.0f, 6.0f);
        EXPECT_TRUE(eq2(v2 += {2.0f, 3.0f}, 10.0f, 9.0f));
        EXPECT_TRUE(eq2(v2 -= 1.0f, 9.0f, 8.0f));
        EXPECT_TRUE(eq2(v2 *= {2.0f, 0.5f}, 18.0f, 4.0f));
        EXPECT_TRUE(eq2(v2 /= 2.0f, 9.0f, 2.0f));
        v2.yx += {1.0f, 2.0f};
        EXPECT_TRUE(eq2(v2, 11.0f, 3.0f));
        v2.yx -= v2;
        EXPECT_TRUE(eq2(v2, 8.0f, -8.0f));
        v2.yx *= {2.0f, 3.0f};
        EXPECT_TRUE(eq2(v2, 24.0f, -16.0f));
        v2.yx /= {4.0f, 8.0f};
        EXPECT_TRUE(eq2(v2, 3.0f, -4.0f));

        // vector3f
        math::vector3f v3(8.0f, 6.0f, 4.0f);
        EXPECT_TRUE(eq3(v3 += {2.0f, 3.0f, 4.0f}, 10.0f, 9.0f, 8.0f));
        EXPECT_TRUE(eq3(v3 -= 1.0f, 9.0f, 8.0f, 7.0f));
        EXPECT_TRUE(eq3(v3 *= {2.0f, 0.5f, 2.0f}, 18.0f, 4.0f, 14.0f));
        EXPECT_TRUE(eq3(v3 /= 2.0f, 9.0f, 2.0f, 7.0f));
        v3.zx += {1.0f, 2.0f};
        EXPECT_TRUE(eq3(v3, 11.0f, 2.0f, 8.0f));
        v3.zyx -= 1.0f;
        EXPECT_TRUE(eq3(v3, 10.0f, 1.0f, 7.0f));
        v3.xz *= 2.0f;
        EXPECT_TRUE(eq3(v3, 20.0f, 1.0f, 14.0f));
        v3.zx /= {7.0f, 4.0f};
        EXPECT_TRUE(eq3(v3, 5.0f, 1.0f, 2.0f));

        // vector4f
        math::vector4f v4(8.0f, 6.0f, 4.0f, 2.0f);
        EXPECT_TRUE(eq4(v4 += {2.0f, 3.0f, 4.0f, 1.0f}, 10.0f, 9.0f, 8.0f, 3.0f));
        EXPECT_TRUE(eq4(v4 -= 1.0f, 9.0f, 8.0f, 7.0f, 2.0f));
        EXPECT_TRUE(eq4(v4 *= {2.0f, 0.5f, 2.0f, 3.0f}, 18.0f, 4.0f, 14.0f, 6.0f));
        EXPECT_TRUE(eq4(v4 /= 2.0f, 9.0f, 2.0f, 7.0f, 3.0f));
        v4.wzyx += {1.0f, 2.0f, 3.0f, 4.0f};
        EXPECT_TRUE(eq4(v4, 13.0f, 5.0f, 9.0f, 4.0f));
        v4.xyz -= v4.wzy;
        EXPECT_TRUE(eq4(v4, 9.0f, -4.0f, 4.0f, 4.0f));
        v4.yw *= -1.0f;
        EXPECT_TRUE(eq4(v4, 9.0f, 4.0f, 4.0f, -4.0f));
        v4.wx /= {2.0f, 3.0f};
        EXPECT_TRUE(eq4(v4, 3.0f, 4.0f, 4.0f, -2.0f));

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

    bool math_vector_transformed_test(const API &api) {
        math::transform2f t2;
        const float t2flat[9] = {1, 2, 0, 0, 1, 0, 5, 6, 1};
        std::copy(t2flat, t2flat + 9, t2.flat);
        EXPECT_TRUE(eq2(math::vector2f(1.0f, 1.0f).transformed(t2, true), 6.0f, 9.0f));
        EXPECT_TRUE(eq2(math::vector2f(1.0f, 1.0f).transformed(t2), 1.0f, 3.0f));
        EXPECT_TRUE(eq2(math::vector2f(1.0f, 2.0f).yx.transformed(t2, true), 7.0f, 11.0f));

        const math::transform3f t3({1, 2, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, -1}, {10, 20, 30, 1});
        EXPECT_TRUE(eq3(math::vector3f(1.0f, 1.0f, 1.0f).transformed(t3, true), 11.0f, 23.0f, 31.0f));
        EXPECT_TRUE(eq3(math::vector3f(1.0f, 1.0f, 1.0f).transformed(t3), 1.0f, 3.0f, 1.0f));
        EXPECT_TRUE(eq3(math::vector3f(1.0f, 2.0f, 3.0f).zyx.transformed(t3, true), 13.0f, 28.0f, 31.0f));

        EXPECT_TRUE(eq4(math::vector4f(1.0f, 1.0f, 1.0f, 1.0f).transformed(t3), 11.0f, 23.0f, 31.0f, 0.0f));
        EXPECT_TRUE(eq4(math::vector4f(1.0f, 1.0f, 1.0f, 0.0f).transformed(t3), 1.0f, 3.0f, 1.0f, -1.0f));
        EXPECT_TRUE(eq4(math::vector4f(1.0f, 2.0f, 3.0f, 0.0f).wzyx.transformed(t3), 10.0f, 23.0f, 32.0f, -1.0f));

        return true;
    }

    bool math_transform3f_translated_inverted_test(const API &api) {
        const math::transform3f t = math::transform3f::identity().translated({1.0f, 2.0f, 3.0f});
        EXPECT_TRUE(near4(t.row0, 1.0f, 0.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near4(t.row1, 0.0f, 1.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near4(t.row2, 0.0f, 0.0f, 1.0f, 0.0f));
        EXPECT_TRUE(near4(t.row3, 1.0f, 2.0f, 3.0f, 1.0f));

        const math::transform3f tt = t.translated({-4.0f, 0.5f, 1.0f});
        EXPECT_TRUE(near4(tt.row3, -3.0f, 2.5f, 4.0f, 1.0f));

        const math::transform3f ti = t.inverted();
        EXPECT_TRUE(near4(ti.row0, 1.0f, 0.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near4(ti.row1, 0.0f, 1.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near4(ti.row2, 0.0f, 0.0f, 1.0f, 0.0f));
        EXPECT_TRUE(near4(ti.row3, -1.0f, -2.0f, -3.0f, 1.0f));

        const math::transform3f m = math::transform3f({0, 2, 0, 0}, {0, 0, 4, 0}, {-0.5f, 0, 0, 0}, {1, 2, 3, 1}).translated({4.0f, 5.0f, 6.0f});
        const math::transform3f mi = m.inverted();
        EXPECT_TRUE(near4(mi.row0, 0.0f, 0.0f, -2.0f, 0.0f));
        EXPECT_TRUE(near4(mi.row1, 0.5f, 0.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near4(mi.row2, 0.0f, 0.25f, 0.0f, 0.0f));
        EXPECT_TRUE(near4(mi.row3, -3.5f, -2.25f, 10.0f, 1.0f));

        const math::transform3f mmi = m * mi;
        EXPECT_TRUE(near4(mmi.row0, 1.0f, 0.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near4(mmi.row1, 0.0f, 1.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near4(mmi.row2, 0.0f, 0.0f, 1.0f, 0.0f));
        EXPECT_TRUE(near4(mmi.row3, 0.0f, 0.0f, 0.0f, 1.0f));

        const math::transform3f mim = mi * m;
        EXPECT_TRUE(near4(mim.row0, 1.0f, 0.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near4(mim.row1, 0.0f, 1.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near4(mim.row2, 0.0f, 0.0f, 1.0f, 0.0f));
        EXPECT_TRUE(near4(mim.row3, 0.0f, 0.0f, 0.0f, 1.0f));

        return true;
    }

    bool math_transform2f_methods_test(const API &api) {
        const math::transform2f id(0.0f);
        EXPECT_TRUE(near3(id.row0, 1.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near3(id.row1, 0.0f, 1.0f, 0.0f));
        EXPECT_TRUE(near3(id.row2, 0.0f, 0.0f, 1.0f));

        const math::transform2f r(MATH_PI * 0.5f);
        EXPECT_TRUE(near3(r.row0, 0.0f, 1.0f, 0.0f));
        EXPECT_TRUE(near3(r.row1, -1.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near3(r.row2, 0.0f, 0.0f, 1.0f));

        const math::transform2f m({2, 0, 0}, {0, 3, 0}, {1, 2, 1});
        const math::transform2f t = m.translated({4.0f, 5.0f});
        EXPECT_TRUE(near3(t.row0, 2.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near3(t.row1, 0.0f, 3.0f, 0.0f));
        EXPECT_TRUE(near3(t.row2, 5.0f, 7.0f, 1.0f));

        const math::transform2f s = m.scaled({2.0f, 0.5f});
        EXPECT_TRUE(near3(s.row0, 4.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near3(s.row1, 0.0f, 1.5f, 0.0f));
        EXPECT_TRUE(near3(s.row2, 2.0f, 1.0f, 1.0f));

        const math::transform2f mr = m.rotated(MATH_PI * 0.5f);
        EXPECT_TRUE(near3(mr.row0, 0.0f, 2.0f, 0.0f));
        EXPECT_TRUE(near3(mr.row1, -3.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near3(mr.row2, -2.0f, 1.0f, 1.0f));

        const math::transform2f mi = m.inverted();
        EXPECT_TRUE(near3(mi.row0, 0.5f, 0.0f, 0.0f));
        EXPECT_TRUE(near3(mi.row1, 0.0f, 1.0f / 3.0f, 0.0f));
        EXPECT_TRUE(near3(mi.row2, -0.5f, -2.0f / 3.0f, 1.0f));

        const math::transform2f n = math::transform2f(0.3f).scaled({2.0f, 3.0f}).translated({4.0f, -1.0f});
        const math::transform2f nni = n * n.inverted();
        EXPECT_TRUE(near3(nni.row0, 1.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near3(nni.row1, 0.0f, 1.0f, 0.0f));
        EXPECT_TRUE(near3(nni.row2, 0.0f, 0.0f, 1.0f));

        return true;
    }

    bool math_vector_rotated_angle_test(const API &api) {
        const float hpi = MATH_PI * 0.5f;

        // vector2f
        EXPECT_TRUE(near2(math::vector2f(1.0f, 0.0f).rotated(hpi), 0.0f, 1.0f));
        EXPECT_TRUE(near2(math::vector2f(1.0f, 0.0f).rotated(-hpi), 0.0f, -1.0f));
        EXPECT_TRUE(near2(math::vector2f(3.0f, 4.0f).rotated(MATH_PI), -3.0f, -4.0f));
        EXPECT_TRUE(near2(math::vector2f(1.0f, 2.0f).yx.rotated(hpi), -1.0f, 2.0f));
        EXPECT_EQ_F(math::vector2f(3.0f, 4.0f).rotated(0.7f).length(), 5.0f);

        EXPECT_EQ_F(math::vector2f(1.0f, 0.0f).angleTo({0.0f, 1.0f}), hpi);
        EXPECT_EQ_F(math::vector2f(0.0f, 1.0f).angleTo({1.0f, 0.0f}), hpi);
        EXPECT_EQ_F(math::vector2f(1.0f, 0.0f).angleTo({1.0f, 1.0f}), MATH_PI * 0.25f);
        EXPECT_EQ_F(math::vector2f(2.0f, 0.0f).angleTo({3.0f, 0.0f}), 0.0f);
        EXPECT_EQ_F(math::vector2f(1.0f, 0.0f).angleTo({-1.0f, 0.0f}), MATH_PI);
        EXPECT_EQ_F(math::vector2f(0.0f, 3.0f).yx.angleTo({1.0f, 0.0f}), 0.0f);
        EXPECT_EQ_F(math::vector2f(2.0f, 1.0f).rotated(0.5f).angleTo({2.0f, 1.0f}), 0.5f);

        // vector3f
        EXPECT_TRUE(near3(math::vector3f(1.0f, 0.0f, 0.0f).rotated({0.0f, 0.0f, 1.0f}, hpi), 0.0f, -1.0f, 0.0f));
        EXPECT_TRUE(near3(math::vector3f(0.0f, 1.0f, 0.0f).rotated({1.0f, 0.0f, 0.0f}, hpi), 0.0f, 0.0f, -1.0f));
        EXPECT_TRUE(near3(math::vector3f(0.0f, 0.0f, 1.0f).rotated({0.0f, 1.0f, 0.0f}, hpi), -1.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near3(math::vector3f(1.0f, 0.0f, 0.0f).rotated({0.0f, 0.0f, 1.0f}, -hpi), 0.0f, 1.0f, 0.0f));
        EXPECT_TRUE(near3(math::vector3f(1.0f, 2.0f, 3.0f).rotated({0.0f, 0.0f, 1.0f}, MATH_PI), -1.0f, -2.0f, 3.0f));
        EXPECT_TRUE(near3(math::vector3f(0.0f, 0.0f, 5.0f).rotated({0.0f, 0.0f, 1.0f}, 1.0f), 0.0f, 0.0f, 5.0f));
        EXPECT_TRUE(near3(math::vector3f(1.0f, 2.0f, 3.0f).zyx.rotated({0.0f, 0.0f, 1.0f}, hpi), 2.0f, -3.0f, 1.0f));

        EXPECT_EQ_F(math::vector3f(1.0f, 0.0f, 0.0f).angleTo({0.0f, 0.0f, 1.0f}), hpi);
        EXPECT_EQ_F(math::vector3f(1.0f, 1.0f, 0.0f).angleTo({1.0f, 0.0f, 0.0f}), MATH_PI * 0.25f);
        EXPECT_EQ_F(math::vector3f(0.0f, 0.0f, 2.0f).angleTo({0.0f, 0.0f, -3.0f}), MATH_PI);
        EXPECT_EQ_F(math::vector3f(1.0f, 0.0f, 0.0f).zyx.angleTo({0.0f, 0.0f, 1.0f}), 0.0f);
        EXPECT_EQ_F(math::vector3f(1.0f, 0.0f, 0.0f).rotated({0.0f, 1.0f, 0.0f}, 0.8f).angleTo({1.0f, 0.0f, 0.0f}), 0.8f);

        return true;
    }

    bool math_color_conversion_test(const API &api) {
        const math::color packed(0x80FF3300u);
        EXPECT_EQ_F(packed.r, 0.0f);
        EXPECT_EQ_F(packed.g, 0.2f);
        EXPECT_EQ_F(packed.b, 1.0f);
        EXPECT_EQ_F(packed.a, 128.0f / 255.0f);

        const math::color scalars(0.2f, 0.4f, 0.6f, 1.0f);
        EXPECT_TRUE(eq4(scalars, 0.2f, 0.4f, 0.6f, 1.0f));
        EXPECT_EQ(unsigned(scalars), 0xFF996633u);

        const math::color vector(math::vector4f(1.0f, 0.0f, 0.2f, 0.4f));
        EXPECT_TRUE(eq4(vector, 1.0f, 0.0f, 0.2f, 0.4f));
        EXPECT_EQ(unsigned(vector), 0x663300FFu);

        EXPECT_EQ(unsigned(math::color(0x80FF3300u)), 0x80FF3300u);
        EXPECT_EQ(unsigned(math::color(0x01020304u)), 0x01020304u);
        EXPECT_EQ(unsigned(math::color(0xFFFFFFFFu)), 0xFFFFFFFFu);
        EXPECT_EQ(unsigned(math::color(0u)), 0u);

        for (unsigned i = 0; i < 256; i++) {
            EXPECT_EQ(unsigned(math::color(i * 0x01010101u)), i * 0x01010101u);
        }

        return true;
    }

    bool math_transform3f_camera_test(const API &api) {
        const math::transform3f view = math::transform3f::lookAtRH({5.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 3.0f, 0.0f});
        EXPECT_TRUE(near3(math::vector3f(5.0f, 0.0f, 0.0f).transformed(view, true), 0.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near3(math::vector3f(0.0f, 0.0f, 0.0f).transformed(view, true), 0.0f, 0.0f, -5.0f));
        EXPECT_TRUE(near3(math::vector3f(0.0f, 0.0f, -1.0f).transformed(view, true), 1.0f, 0.0f, -5.0f));
        EXPECT_TRUE(near3(math::vector3f(5.0f, 2.0f, 0.0f).transformed(view, true), 0.0f, 2.0f, 0.0f));
        EXPECT_TRUE(near3(math::vector3f(1.0f, 0.0f, 0.0f).transformed(view), 0.0f, 0.0f, 1.0f));

        const math::transform3f proj = math::transform3f::perspectiveFovRH(MATH_PI * 0.5f, 2.0f, 1.0f, 3.0f);
        EXPECT_TRUE(near4(math::vector4f(2.0f, 1.0f, -1.0f, 1.0f).transformed(proj), 1.0f, 1.0f, 1.0f, 1.0f));
        EXPECT_TRUE(near4(math::vector4f(-6.0f, -3.0f, -3.0f, 1.0f).transformed(proj), -3.0f, -3.0f, 0.0f, 3.0f));
        EXPECT_TRUE(near4(math::vector4f(0.0f, 0.0f, -2.0f, 1.0f).transformed(proj), 0.0f, 0.0f, 0.5f, 2.0f));

        const math::vector4f clip = math::vector4f(0.0f, 1.0f, 0.0f, 1.0f).transformed(view * proj);
        EXPECT_TRUE(near4(clip, 0.0f, 1.0f, -1.0f, 5.0f));

        const math::transform3f dz = math::transform3f::directionXZ({0.0f, 0.0f, 1.0f});
        EXPECT_TRUE(near3(math::vector3f(1.0f, 0.0f, 0.0f).transformed(dz), 0.0f, 0.0f, 1.0f));
        EXPECT_TRUE(near3(math::vector3f(0.0f, 1.0f, 0.0f).transformed(dz), 0.0f, 1.0f, 0.0f));

        const float h = std::sqrt(0.5f);
        EXPECT_TRUE(near3(math::vector3f(1.0f, 0.0f, 0.0f).transformed(math::transform3f::directionXZ({1.0f, 0.0f, 0.0f})), 1.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near3(math::vector3f(1.0f, 0.0f, 0.0f).transformed(math::transform3f::directionXZ({0.0f, 0.0f, -2.0f})), 0.0f, 0.0f, -1.0f));
        EXPECT_TRUE(near3(math::vector3f(1.0f, 0.0f, 0.0f).transformed(math::transform3f::directionXZ({2.0f, 0.0f, 2.0f})), h, 0.0f, h));
        EXPECT_TRUE(near3(math::vector3f(1.0f, 0.0f, 0.0f).transformed(math::transform3f::directionXZ({-1.0f, 0.0f, -1.0f})), -h, 0.0f, -h));

        return true;
    }

    bool math_bound3f_world_bounds_test(const API &api) {
        const math::transform3f id = math::transform3f::identity();
        const math::transform3f t = id.translated({1.0f, 2.0f, 3.0f});
        EXPECT_TRUE(nearBound(math::bound3f::getWorldBounds(t, 2.0f), -1.0f, 0.0f, 1.0f, 3.0f, 4.0f, 5.0f));
        EXPECT_TRUE(nearBound(math::bound3f::getWorldBounds(id, 0.5f), -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f));

        const math::bound3f bb = {1.0f, 3.0f, -2.0f, 2.0f, 5.0f, 4.0f};
        EXPECT_TRUE(nearBound(math::bound3f::getWorldBounds(id, bb), 1.0f, 3.0f, -2.0f, 2.0f, 5.0f, 4.0f));
        EXPECT_TRUE(nearBound(math::bound3f::getWorldBounds(t, bb), 2.0f, 5.0f, 1.0f, 3.0f, 7.0f, 7.0f));

        const math::transform3f r({0, 0, 1}, MATH_PI * 0.5f);
        EXPECT_TRUE(nearBound(math::bound3f::getWorldBounds(r, bb), 3.0f, -2.0f, -2.0f, 5.0f, -1.0f, 4.0f));

        const math::transform3f s({2, 0, 0, 0}, {0, -3, 0, 0}, {0, 0, 0.5f, 0}, {1, 1, 1, 1});
        EXPECT_TRUE(nearBound(math::bound3f::getWorldBounds(s, bb), 3.0f, -14.0f, 0.0f, 5.0f, -8.0f, 3.0f));

        return true;
    }

    bool math_ray_geometry_test(const API &api) {
        math::vector3f out;
        EXPECT_TRUE(math::intersectRayPlane({1, 5, 2}, {0, -1, 0}, {3, 1, -4}, {0, 1, 0}, out));
        EXPECT_TRUE(near3(out, 1.0f, 1.0f, 2.0f));
        EXPECT_TRUE(math::intersectRayPlane({0, 0, 0}, {0, 0, -1}, {3, 4, -5}, {0, 0, 1}, out));
        EXPECT_TRUE(near3(out, 0.0f, 0.0f, -5.0f));
        EXPECT_FALSE(math::intersectRayPlane({1, 5, 2}, {1, 0, 0}, {0, 1, 0}, {0, 1, 0}, out));
        EXPECT_FALSE(math::intersectRayPlane({0, 0, 0}, {0, 1, 0}, {0, -2, 0}, {0, 1, 0}, out));

        const math::vector3f o = {0, 0, 0};
        const math::vector3f x = {1, 0, 0};
        EXPECT_EQ_F(math::distanceRayLine(o, x, {2, -1, 0}, {2, 1, 0}), 0.0f);
        EXPECT_EQ_F(math::distanceRayLine(o, x, {3, -1, 2}, {3, 1, 2}), 2.0f);
        EXPECT_EQ_F(math::distanceRayLine(o, x, {2, 1, 0}, {2, 3, 0}), 1.0f);
        EXPECT_EQ_F(math::distanceRayLine(o, x, {-3, -1, 1}, {-3, 1, 1}), std::sqrt(10.0f));

        const float h = std::sqrt(0.5f);
        EXPECT_TRUE(near2(math::nearestKoeffsRayRay(o, x, {3, -2, 4}, {0, 1, 0}), 3.0f, 2.0f));
        EXPECT_TRUE(near2(math::nearestKoeffsRayRay(o, x, {5, -2, 0}, {-h, h, 0}), 3.0f, 2.0f / h));

        return true;
    }

    bool math_quaternion_axis_rotation_test(const API &api) {
        const float h = std::sqrt(0.5f);
        const float hpi = MATH_PI * 0.5f;
        const math::vector4f id = {0.0f, 0.0f, 0.0f, 1.0f};
        const math::vector4f q = {0.1f, 0.2f, 0.3f, 0.4f};
        EXPECT_TRUE(near4(q.quaternionMultiply(id), 0.1f, 0.2f, 0.3f, 0.4f));
        EXPECT_TRUE(near4(id.quaternionMultiply(q), 0.1f, 0.2f, 0.3f, 0.4f));

        const math::vector4f qz30 = {0.0f, 0.0f, std::sin(MATH_PI / 12.0f), std::cos(MATH_PI / 12.0f)};
        const math::vector4f qz60 = {0.0f, 0.0f, std::sin(MATH_PI / 6.0f), std::cos(MATH_PI / 6.0f)};
        EXPECT_TRUE(near4(qz30.quaternionMultiply(qz60), 0.0f, 0.0f, h, h));
        EXPECT_TRUE(near4(qz60.quaternionMultiply(qz30), 0.0f, 0.0f, h, h));

        const math::transform3f rx({1, 0, 0}, hpi);
        EXPECT_TRUE(near3(math::vector3f(1.0f, 0.0f, 0.0f).transformed(rx), 1.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near3(math::vector3f(0.0f, 1.0f, 0.0f).transformed(rx), 0.0f, 0.0f, -1.0f));
        EXPECT_TRUE(near3(math::vector3f(0.0f, 0.0f, 1.0f).transformed(rx), 0.0f, 1.0f, 0.0f));

        const math::transform3f ry({0, 1, 0}, hpi);
        EXPECT_TRUE(near3(math::vector3f(1.0f, 0.0f, 0.0f).transformed(ry), 0.0f, 0.0f, 1.0f));
        EXPECT_TRUE(near3(math::vector3f(0.0f, 1.0f, 0.0f).transformed(ry), 0.0f, 1.0f, 0.0f));
        EXPECT_TRUE(near3(math::vector3f(0.0f, 0.0f, 1.0f).transformed(ry), -1.0f, 0.0f, 0.0f));

        const math::transform3f rz({0, 0, 1}, hpi);
        EXPECT_TRUE(near3(math::vector3f(1.0f, 0.0f, 0.0f).transformed(rz), 0.0f, -1.0f, 0.0f));
        EXPECT_TRUE(near3(math::vector3f(0.0f, 1.0f, 0.0f).transformed(rz), 1.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near3(math::vector3f(2.0f, 3.0f, 4.0f).transformed(rz, true), 3.0f, -2.0f, 4.0f));

        const math::transform3f rd(math::vector3f(1.0f, 1.0f, 1.0f).normalized(1.0f), MATH_PI * 2.0f / 3.0f);
        EXPECT_TRUE(near3(math::vector3f(1.0f, 0.0f, 0.0f).transformed(rd), 0.0f, 0.0f, 1.0f));
        EXPECT_TRUE(near3(math::vector3f(0.0f, 1.0f, 0.0f).transformed(rd), 1.0f, 0.0f, 0.0f));
        EXPECT_TRUE(near3(math::vector3f(0.0f, 0.0f, 1.0f).transformed(rd), 0.0f, 1.0f, 0.0f));

        const math::vector3f axis = math::vector3f(1.0f, -2.0f, 0.5f).normalized(1.0f);
        const math::vector3f v = {1.0f, 2.0f, 3.0f};
        const math::vector3f expected = v.rotated(axis, 0.7f);
        EXPECT_TRUE(near3(v.transformed(math::transform3f(axis, 0.7f)), expected.x, expected.y, expected.z));
        EXPECT_TRUE(near3(axis.transformed(math::transform3f(axis, 0.7f)), axis.x, axis.y, axis.z));

        return true;
    }

    bool math_swizzle_atv4_test(const API &api) {
        const math::vector2f v2 = {1.0f, 2.0f};
        EXPECT_TRUE(eq4(v2.atv4start(3.0f, 4.0f), 1.0f, 2.0f, 3.0f, 4.0f));
        EXPECT_TRUE(eq4(v2.atv4middle(3.0f, 4.0f), 3.0f, 1.0f, 2.0f, 4.0f));
        EXPECT_TRUE(eq4(v2.atv4end(3.0f, 4.0f), 3.0f, 4.0f, 1.0f, 2.0f));
        EXPECT_TRUE(eq4(v2.yx.atv4start(3.0f, 4.0f), 2.0f, 1.0f, 3.0f, 4.0f));
        EXPECT_TRUE(eq4(v2.yx.atv4middle(3.0f, 4.0f), 3.0f, 2.0f, 1.0f, 4.0f));
        EXPECT_TRUE(eq4(v2.yx.atv4end(3.0f, 4.0f), 3.0f, 4.0f, 2.0f, 1.0f));
        EXPECT_TRUE(eq4(v2.yy.atv4start(3.0f, 4.0f), 2.0f, 2.0f, 3.0f, 4.0f));

        const math::vector3f v3 = {1.0f, 2.0f, 3.0f};
        EXPECT_TRUE(eq4(v3.atv4start(4.0f), 1.0f, 2.0f, 3.0f, 4.0f));
        EXPECT_TRUE(eq4(v3.atv4end(4.0f), 4.0f, 1.0f, 2.0f, 3.0f));
        EXPECT_TRUE(eq4(v3.zyx.atv4start(4.0f), 3.0f, 2.0f, 1.0f, 4.0f));
        EXPECT_TRUE(eq4(v3.zyx.atv4end(4.0f), 4.0f, 3.0f, 2.0f, 1.0f));
        EXPECT_TRUE(eq4(v3.zx.atv4middle(4.0f, 5.0f), 4.0f, 3.0f, 1.0f, 5.0f));
        EXPECT_TRUE(eq4(v3.yz.atv4end(4.0f, 5.0f), 4.0f, 5.0f, 2.0f, 3.0f));

        const math::vector4f v4 = {1.0f, 2.0f, 3.0f, 4.0f};
        EXPECT_TRUE(eq4(v4.wzy.atv4start(5.0f), 4.0f, 3.0f, 2.0f, 5.0f));
        EXPECT_TRUE(eq4(v4.xwz.atv4end(5.0f), 5.0f, 1.0f, 4.0f, 3.0f));
        EXPECT_TRUE(eq4(v4.wx.atv4middle(5.0f, 6.0f), 5.0f, 4.0f, 1.0f, 6.0f));
        EXPECT_TRUE(eq4(v4.zw.atv4start(5.0f, 6.0f), 3.0f, 4.0f, 5.0f, 6.0f));

        return true;
    }

    bool math_vector_int_ctors_test(const API &api) {
        EXPECT_TRUE(eq2i(math::vector2i(-3), -3, -3));
        EXPECT_TRUE(eq3i(math::vector3i(7), 7, 7, 7));
        EXPECT_TRUE(eq4i(math::vector4i(0), 0, 0, 0, 0));

        const int a2[2] = {1, -2};
        const int a3[3] = {1, -2, 3};
        const int a4[4] = {1, -2, 3, -4};
        EXPECT_TRUE(eq2i(math::vector2i(a2), 1, -2));
        EXPECT_TRUE(eq3i(math::vector3i(a3), 1, -2, 3));
        EXPECT_TRUE(eq4i(math::vector4i(a4), 1, -2, 3, -4));

        const math::vector2i v2(5, 6);
        const math::vector3i v3(5, 6, 7);
        const math::vector4i v4(5, 6, 7, 8);
        EXPECT_TRUE(eq2i(v2, 5, 6));
        EXPECT_TRUE(eq3i(v3, 5, 6, 7));
        EXPECT_TRUE(eq4i(v4, 5, 6, 7, 8));

        const math::vector2i c2(v2);
        const math::vector3i c3(v3);
        const math::vector4i c4(v4);
        EXPECT_TRUE(eq2i(c2, 5, 6));
        EXPECT_TRUE(eq3i(c3, 5, 6, 7));
        EXPECT_TRUE(eq4i(c4, 5, 6, 7, 8));

        return true;
    }

    bool math_vector_comparison_test(const API &api) {
        const float half = math::EPSILON * 0.5f;
        const float twice = math::EPSILON * 2.0f;

        const math::vector2f a2(1.0f, 2.0f);
        EXPECT_TRUE(a2 == math::vector2f(1.0f, 2.0f));
        EXPECT_TRUE(a2 == math::vector2f(1.0f, 2.0f) + math::vector2f(half, 0.0f));
        EXPECT_TRUE(math::vector2f(0.0f, half) == math::vector2f(0.0f));
        EXPECT_FALSE(math::vector2f(0.0f, twice) == math::vector2f(0.0f));
        EXPECT_TRUE(a2 != math::vector2f(2.0f, 1.0f));
        EXPECT_FALSE(a2 != math::vector2f(1.0f, 2.0f));
        EXPECT_TRUE(a2.yx == math::vector2f(2.0f, 1.0f));

        const math::vector3f a3(1.0f, 2.0f, 3.0f);
        EXPECT_TRUE(a3 == math::vector3f(1.0f, 2.0f, 3.0f));
        EXPECT_TRUE(math::vector3f(half, 0.0f, half) == math::vector3f(0.0f));
        EXPECT_TRUE(math::vector3f(0.0f, 0.0f, twice) != math::vector3f(0.0f));
        EXPECT_FALSE(a3 != math::vector3f(1.0f, 2.0f, 3.0f));
        EXPECT_TRUE(a3.zyx == math::vector3f(3.0f, 2.0f, 1.0f));
        EXPECT_TRUE(a3.xy != a3.yx);

        const math::vector4f a4(1.0f, 2.0f, 3.0f, 4.0f);
        const math::vector4f b4(3.0f, 4.0f, 1.0f, 2.0f);
        EXPECT_TRUE(a4 == math::vector4f(1.0f, 2.0f, 3.0f, 4.0f));
        EXPECT_TRUE(math::vector4f(0.0f, 0.0f, 0.0f, half) == math::vector4f(0.0f));
        EXPECT_TRUE(math::vector4f(twice, 0.0f, 0.0f, 0.0f) != math::vector4f(0.0f));
        EXPECT_TRUE(a4 != b4);
        EXPECT_TRUE(a4.xy == b4.zw);
        EXPECT_TRUE(a4.zw == b4.xy);
        EXPECT_FALSE(a4.xyz == b4.xyz);
        EXPECT_TRUE(a4.zwxy == b4);

        EXPECT_TRUE(math::vector2i(1, 2) == math::vector2i(1, 2));
        EXPECT_TRUE(math::vector2i(1, 2) != math::vector2i(2, 1));
        EXPECT_FALSE(math::vector2i(1, 2) != math::vector2i(1, 2));
        EXPECT_TRUE(math::vector3i(1, 2, 3) == math::vector3i(1, 2, 3));
        EXPECT_TRUE(math::vector3i(1, 2, 3) != math::vector3i(1, 2, 4));
        EXPECT_FALSE(math::vector3i(1, 2, 3) == math::vector3i(0, 2, 3));
        EXPECT_TRUE(math::vector4i(1, 2, 3, 4) == math::vector4i(1, 2, 3, 4));
        EXPECT_TRUE(math::vector4i(1, 2, 3, 4) != math::vector4i(1, 2, 3, 5));
        EXPECT_FALSE(math::vector4i(1, 2, 3, 4) != math::vector4i(1, 2, 3, 4));

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
        REGISTER_UNIT_TEST(math_vector_compound_assignment_test);
        REGISTER_UNIT_TEST(math_vector_dot_cross_test);
        REGISTER_UNIT_TEST(math_vector_length_normalize_test);
        REGISTER_UNIT_TEST(math_transform3f_identity_multiply_test);
        REGISTER_UNIT_TEST(math_vector_transformed_test);
        REGISTER_UNIT_TEST(math_transform3f_translated_inverted_test);
        REGISTER_UNIT_TEST(math_transform2f_methods_test);
        REGISTER_UNIT_TEST(math_vector_rotated_angle_test);
        REGISTER_UNIT_TEST(math_color_conversion_test);
        REGISTER_UNIT_TEST(math_transform3f_camera_test);
        REGISTER_UNIT_TEST(math_bound3f_world_bounds_test);
        REGISTER_UNIT_TEST(math_ray_geometry_test);
        REGISTER_UNIT_TEST(math_quaternion_axis_rotation_test);
        REGISTER_UNIT_TEST(math_swizzle_atv4_test);
        REGISTER_UNIT_TEST(math_vector_int_ctors_test);
        REGISTER_UNIT_TEST(math_vector_comparison_test);
    }
}
