
// TODO: + default transform enum constant
//       + +=, *=, etc
//       + xOz

// Basic math for games
// key features:
//

#pragma once
#include <limits>
#include <cmath>
#include <algorithm>

namespace math {
    using scalar = float;
    
    struct vector2f;
    struct vector3f;
    struct vector4f;
    
    struct transform2f;
    struct transform3f;
    
    struct bound2f;
    struct bound3f;
    
    template<int Ix, int Iy> struct swizzle2f {
        vector2f &operator=(const swizzle2f &other);
        template<int Ox, int Oy> vector2f &operator=(const swizzle2f<Ox, Oy> &other);
        operator vector2f() const;
        
        scalar (&asFlat())[4] {
            return reinterpret_cast<scalar (&)[4]>(*this);
        }
        const scalar (&asFlat() const)[4] {
            return reinterpret_cast<const scalar (&)[4]>(*this);
        }
        scalar operator[](int index) const {
            return reinterpret_cast<const scalar (&)[4]>(*this)[index];
        }

        auto length() const -> scalar;
        auto lengthSq() const -> scalar;
        auto normalized(scalar length = 1.0) const -> vector2f;
        auto atv4start(scalar z, scalar w) const -> vector4f;
        auto atv4middle(scalar x, scalar w) const -> vector4f;
        auto atv4end(scalar x, scalar y) const -> vector4f;
        auto dot(const vector2f &other) const -> scalar;
        auto cross(const vector2f &other) const -> scalar;
        auto angleTo(const vector2f &other) const -> scalar;
        auto rotated(scalar radians) const -> vector2f;
        auto distanceTo(const vector2f &other) const -> scalar;
        auto transformed(const transform2f &trfm, bool likePosition = false) const -> vector2f;
    };
    template<int Ix, int Iy, int Iz> struct swizzle3f {
        vector3f &operator=(const swizzle3f &other);
        template<int Ox, int Oy, int Oz> vector3f &operator=(const swizzle3f<Ox, Oy, Oz> &other);
        operator vector3f() const;

        scalar (&asFlat())[4] {
            return reinterpret_cast<scalar (&)[4]>(*this);
        }
        const scalar (&asFlat() const)[4] {
            return reinterpret_cast<const scalar (&)[4]>(*this);
        }
        scalar operator[](int index) const {
            return reinterpret_cast<const scalar (&)[4]>(*this)[index];
        }
        
        auto length() const -> scalar;
        auto lengthSq() const -> scalar;
        auto normalized(scalar length = 1.0) const -> vector3f;
        auto atv4start(scalar w) const -> vector4f;
        auto atv4end(scalar x) const -> vector4f;
        auto dot(const vector3f &other) const -> scalar;
        auto cross(const vector3f &other) const -> vector3f;
        auto angleTo(const vector3f &other) const -> scalar;
        auto rotated(const vector3f &axis, scalar radians) const -> vector3f;
        auto distanceTo(const vector3f &other) const -> scalar;
        auto transformed(const transform3f &trfm, bool likePosition = false) const -> vector3f;
    };
    template<int Ix, int Iy, int Iz, int Iw> struct swizzle4f {
        vector4f &operator=(const swizzle4f &other);
        template<int Ox, int Oy, int Oz, int Ow> vector4f &operator=(const swizzle4f<Ox, Oy, Oz, Ow> &other);
        operator vector4f() const;

        scalar (&asFlat())[4] {
            return reinterpret_cast<scalar (&)[4]>(*this);
        }
        const scalar (&asFlat() const)[4] {
            return reinterpret_cast<const scalar (&)[4]>(*this);
        }
        scalar operator[](int index) const {
            return reinterpret_cast<const scalar (&)[4]>(*this)[index];
        }

        auto dot(const vector4f &other) const -> scalar;
        auto transformed(const transform3f &trfm) const -> vector4f;
        auto quaternionMultiply(const vector4f &other) const -> vector4f;
    };

    struct vector2f : swizzle2f<0, 1> {
        union {
            struct {
                scalar x, y;
            };
            struct {
                scalar data[2];
            }
            block;
            scalar flat[2];
            
            swizzle2f<0, 0> xx;
            /*------ <0, 1> */;
            swizzle2f<1, 0> yx;
            swizzle2f<1, 1> yy;
        };
        
        vector2f() = default;
        vector2f(const vector2f &) = default;
        vector2f(const scalar s) : x(s), y(s) {}
        vector2f(const scalar (&xy)[2]) : x(xy[0]), y(xy[1]) {}
        vector2f(scalar x, scalar y) : x(x), y(y) {}
        vector2f& operator =(const vector2f &);
    };
    struct vector3f : swizzle3f<0, 1, 2> {
        union {
            struct {
                scalar x, y, z;
            };
            struct {
                scalar data[3];
            }
            block;
            scalar flat[3];
            
            swizzle2f<0, 0> xx; swizzle2f<1, 0> yx; swizzle2f<2, 0> zx;
            swizzle2f<0, 1> xy; swizzle2f<1, 1> yy; swizzle2f<2, 1> zy;
            swizzle2f<0, 2> xz; swizzle2f<1, 2> yz; swizzle2f<2, 2> zz;

            swizzle3f<0, 0, 0> xxx; swizzle3f<1, 0, 0> yxx; swizzle3f<2, 0, 0> zxx;
            swizzle3f<0, 0, 1> xxy; swizzle3f<1, 0, 1> yxy; swizzle3f<2, 0, 1> zxy;
            swizzle3f<0, 0, 2> xxz; swizzle3f<1, 0, 2> yxz; swizzle3f<2, 0, 2> zxz;
            swizzle3f<0, 1, 0> xyx; swizzle3f<1, 1, 0> yyx; swizzle3f<2, 1, 0> zyx;
            swizzle3f<0, 1, 1> xyy; swizzle3f<1, 1, 1> yyy; swizzle3f<2, 1, 1> zyy;
            /*------ <0, 1, 2> -*/; swizzle3f<1, 1, 2> yyz; swizzle3f<2, 1, 2> zyz;
            swizzle3f<0, 2, 0> xzx; swizzle3f<1, 2, 0> yzx; swizzle3f<2, 2, 0> zzx;
            swizzle3f<0, 2, 1> xzy; swizzle3f<1, 2, 1> yzy; swizzle3f<2, 2, 1> zzy;
            swizzle3f<0, 2, 2> xzz; swizzle3f<1, 2, 2> yzz; swizzle3f<2, 2, 2> zzz;
        };
        
        vector3f() = default;
        vector3f(const vector3f &) = default;
        vector3f(const scalar s) : x(s), y(s), z(s) {}
        vector3f(const scalar (&xyz)[3]) : x(xyz[0]), y(xyz[1]), z(xyz[2]) {}
        vector3f(scalar x, scalar y, scalar z) : x(x), y(y), z(z) {}
        vector3f(const vector2f &xy, scalar z) : x(xy.x), y(xy.y), z(z) {}
        vector3f(scalar x, const vector2f &yz) : x(x), y(yz.x), z(yz.y) {}
        vector3f& operator =(const vector3f &);
    };
    struct vector4f : swizzle4f<0, 1, 2, 3> {
        union {
            struct {
                scalar x, y, z, w;
            };
            struct {
                scalar data[4];
            }
            block;
            scalar flat[4];
            
            swizzle2f<0, 0> xx; swizzle2f<1, 0> yx; swizzle2f<2, 0> zx; swizzle2f<3, 0> wx;
            swizzle2f<0, 1> xy; swizzle2f<1, 1> yy; swizzle2f<2, 1> zy; swizzle2f<3, 1> wy;
            swizzle2f<0, 2> xz; swizzle2f<1, 2> yz; swizzle2f<2, 2> zz; swizzle2f<3, 2> wz;
            swizzle2f<0, 3> xw; swizzle2f<1, 3> yw; swizzle2f<2, 3> zw; swizzle2f<3, 3> ww;
            
            swizzle3f<0, 0, 0> xxx; swizzle3f<1, 0, 0> yxx; swizzle3f<2, 0, 0> zxx; swizzle3f<3, 0, 0> wxx;
            swizzle3f<0, 0, 1> xxy; swizzle3f<1, 0, 1> yxy; swizzle3f<2, 0, 1> zxy; swizzle3f<3, 0, 1> wxy;
            swizzle3f<0, 0, 2> xxz; swizzle3f<1, 0, 2> yxz; swizzle3f<2, 0, 2> zxz; swizzle3f<3, 0, 2> wxz;
            swizzle3f<0, 0, 3> xxw; swizzle3f<1, 0, 3> yxw; swizzle3f<2, 0, 3> zxw; swizzle3f<3, 0, 3> wxw;
            swizzle3f<0, 1, 0> xyx; swizzle3f<1, 1, 0> yyx; swizzle3f<2, 1, 0> zyx; swizzle3f<3, 1, 0> wyx;
            swizzle3f<0, 1, 1> xyy; swizzle3f<1, 1, 1> yyy; swizzle3f<2, 1, 1> zyy; swizzle3f<3, 1, 1> wyy;
            swizzle3f<0, 1, 2> xyz; swizzle3f<1, 1, 2> yyz; swizzle3f<2, 1, 2> zyz; swizzle3f<3, 1, 2> wyz;
            swizzle3f<0, 1, 3> xyw; swizzle3f<1, 1, 3> yyw; swizzle3f<2, 1, 3> zyw; swizzle3f<3, 1, 3> wyw;
            swizzle3f<0, 2, 0> xzx; swizzle3f<1, 2, 0> yzx; swizzle3f<2, 2, 0> zzx; swizzle3f<3, 2, 0> wzx;
            swizzle3f<0, 2, 1> xzy; swizzle3f<1, 2, 1> yzy; swizzle3f<2, 2, 1> zzy; swizzle3f<3, 2, 1> wzy;
            swizzle3f<0, 2, 2> xzz; swizzle3f<1, 2, 2> yzz; swizzle3f<2, 2, 2> zzz; swizzle3f<3, 2, 2> wzz;
            swizzle3f<0, 2, 3> xzw; swizzle3f<1, 2, 3> yzw; swizzle3f<2, 2, 3> zzw; swizzle3f<3, 2, 3> wzw;
            swizzle3f<0, 3, 0> xwx; swizzle3f<1, 3, 0> ywx; swizzle3f<2, 3, 0> zwx; swizzle3f<3, 3, 0> wwx;
            swizzle3f<0, 3, 1> xwy; swizzle3f<1, 3, 1> ywy; swizzle3f<2, 3, 1> zwy; swizzle3f<3, 3, 1> wwy;
            swizzle3f<0, 3, 2> xwz; swizzle3f<1, 3, 2> ywz; swizzle3f<2, 3, 2> zwz; swizzle3f<3, 3, 2> wwz;
            swizzle3f<0, 3, 3> xww; swizzle3f<1, 3, 3> yww; swizzle3f<2, 3, 3> zww; swizzle3f<3, 3, 3> www;
            
            swizzle4f<0, 0, 0, 0> xxxx; swizzle4f<1, 0, 0, 0> yxxx; swizzle4f<2, 0, 0, 0> zxxx; swizzle4f<3, 0, 0, 0> wxxx;
            swizzle4f<0, 0, 0, 1> xxxy; swizzle4f<1, 0, 0, 1> yxxy; swizzle4f<2, 0, 0, 1> zxxy; swizzle4f<3, 0, 0, 1> wxxy;
            swizzle4f<0, 0, 0, 2> xxxz; swizzle4f<1, 0, 0, 2> yxxz; swizzle4f<2, 0, 0, 2> zxxz; swizzle4f<3, 0, 0, 2> wxxz;
            swizzle4f<0, 0, 0, 3> xxxw; swizzle4f<1, 0, 0, 3> yxxw; swizzle4f<2, 0, 0, 3> zxxw; swizzle4f<3, 0, 0, 3> wxxw;
            swizzle4f<0, 0, 1, 0> xxyx; swizzle4f<1, 0, 1, 0> yxyx; swizzle4f<2, 0, 1, 0> zxyx; swizzle4f<3, 0, 1, 0> wxyx;
            swizzle4f<0, 0, 1, 1> xxyy; swizzle4f<1, 0, 1, 1> yxyy; swizzle4f<2, 0, 1, 1> zxyy; swizzle4f<3, 0, 1, 1> wxyy;
            swizzle4f<0, 0, 1, 2> xxyz; swizzle4f<1, 0, 1, 2> yxyz; swizzle4f<2, 0, 1, 2> zxyz; swizzle4f<3, 0, 1, 2> wxyz;
            swizzle4f<0, 0, 1, 3> xxyw; swizzle4f<1, 0, 1, 3> yxyw; swizzle4f<2, 0, 1, 3> zxyw; swizzle4f<3, 0, 1, 3> wxyw;
            swizzle4f<0, 0, 2, 0> xxzx; swizzle4f<1, 0, 2, 0> yxzx; swizzle4f<2, 0, 2, 0> zxzx; swizzle4f<3, 0, 2, 0> wxzx;
            swizzle4f<0, 0, 2, 1> xxzy; swizzle4f<1, 0, 2, 1> yxzy; swizzle4f<2, 0, 2, 1> zxzy; swizzle4f<3, 0, 2, 1> wxzy;
            swizzle4f<0, 0, 2, 2> xxzz; swizzle4f<1, 0, 2, 2> yxzz; swizzle4f<2, 0, 2, 2> zxzz; swizzle4f<3, 0, 2, 2> wxzz;
            swizzle4f<0, 0, 2, 3> xxzw; swizzle4f<1, 0, 2, 3> yxzw; swizzle4f<2, 0, 2, 3> zxzw; swizzle4f<3, 0, 2, 3> wxzw;
            swizzle4f<0, 0, 3, 0> xxwx; swizzle4f<1, 0, 3, 0> yxwx; swizzle4f<2, 0, 3, 0> zxwx; swizzle4f<3, 0, 3, 0> wxwx;
            swizzle4f<0, 0, 3, 1> xxwy; swizzle4f<1, 0, 3, 1> yxwy; swizzle4f<2, 0, 3, 1> zxwy; swizzle4f<3, 0, 3, 1> wxwy;
            swizzle4f<0, 0, 3, 2> xxwz; swizzle4f<1, 0, 3, 2> yxwz; swizzle4f<2, 0, 3, 2> zxwz; swizzle4f<3, 0, 3, 2> wxwz;
            swizzle4f<0, 0, 3, 3> xxww; swizzle4f<1, 0, 3, 3> yxww; swizzle4f<2, 0, 3, 3> zxww; swizzle4f<3, 0, 3, 3> wxww;
            swizzle4f<0, 1, 0, 0> xyxx; swizzle4f<1, 1, 0, 0> yyxx; swizzle4f<2, 1, 0, 0> zyxx; swizzle4f<3, 1, 0, 0> wyxx;
            swizzle4f<0, 1, 0, 1> xyxy; swizzle4f<1, 1, 0, 1> yyxy; swizzle4f<2, 1, 0, 1> zyxy; swizzle4f<3, 1, 0, 1> wyxy;
            swizzle4f<0, 1, 0, 2> xyxz; swizzle4f<1, 1, 0, 2> yyxz; swizzle4f<2, 1, 0, 2> zyxz; swizzle4f<3, 1, 0, 2> wyxz;
            swizzle4f<0, 1, 0, 3> xyxw; swizzle4f<1, 1, 0, 3> yyxw; swizzle4f<2, 1, 0, 3> zyxw; swizzle4f<3, 1, 0, 3> wyxw;
            swizzle4f<0, 1, 1, 0> xyyx; swizzle4f<1, 1, 1, 0> yyyx; swizzle4f<2, 1, 1, 0> zyyx; swizzle4f<3, 1, 1, 0> wyyx;
            swizzle4f<0, 1, 1, 1> xyyy; swizzle4f<1, 1, 1, 1> yyyy; swizzle4f<2, 1, 1, 1> zyyy; swizzle4f<3, 1, 1, 1> wyyy;
            swizzle4f<0, 1, 1, 2> xyyz; swizzle4f<1, 1, 1, 2> yyyz; swizzle4f<2, 1, 1, 2> zyyz; swizzle4f<3, 1, 1, 2> wyyz;
            swizzle4f<0, 1, 1, 3> xyyw; swizzle4f<1, 1, 1, 3> yyyw; swizzle4f<2, 1, 1, 3> zyyw; swizzle4f<3, 1, 1, 3> wyyw;
            swizzle4f<0, 1, 2, 0> xyzx; swizzle4f<1, 1, 2, 0> yyzx; swizzle4f<2, 1, 2, 0> zyzx; swizzle4f<3, 1, 2, 0> wyzx;
            swizzle4f<0, 1, 2, 1> xyzy; swizzle4f<1, 1, 2, 1> yyzy; swizzle4f<2, 1, 2, 1> zyzy; swizzle4f<3, 1, 2, 1> wyzy;
            swizzle4f<0, 1, 2, 2> xyzz; swizzle4f<1, 1, 2, 2> yyzz; swizzle4f<2, 1, 2, 2> zyzz; swizzle4f<3, 1, 2, 2> wyzz;
            /*------ <0, 1, 2, 3> --*/; swizzle4f<1, 1, 2, 3> yyzw; swizzle4f<2, 1, 2, 3> zyzw; swizzle4f<3, 1, 2, 3> wyzw;
            swizzle4f<0, 1, 3, 0> xywx; swizzle4f<1, 1, 3, 0> yywx; swizzle4f<2, 1, 3, 0> zywx; swizzle4f<3, 1, 3, 0> wywx;
            swizzle4f<0, 1, 3, 1> xywy; swizzle4f<1, 1, 3, 1> yywy; swizzle4f<2, 1, 3, 1> zywy; swizzle4f<3, 1, 3, 1> wywy;
            swizzle4f<0, 1, 3, 2> xywz; swizzle4f<1, 1, 3, 2> yywz; swizzle4f<2, 1, 3, 2> zywz; swizzle4f<3, 1, 3, 2> wywz;
            swizzle4f<0, 1, 3, 3> xyww; swizzle4f<1, 1, 3, 3> yyww; swizzle4f<2, 1, 3, 3> zyww; swizzle4f<3, 1, 3, 3> wyww;
            swizzle4f<0, 2, 0, 0> xzxx; swizzle4f<1, 2, 0, 0> yzxx; swizzle4f<2, 2, 0, 0> zzxx; swizzle4f<3, 2, 0, 0> wzxx;
            swizzle4f<0, 2, 0, 1> xzxy; swizzle4f<1, 2, 0, 1> yzxy; swizzle4f<2, 2, 0, 1> zzxy; swizzle4f<3, 2, 0, 1> wzxy;
            swizzle4f<0, 2, 0, 2> xzxz; swizzle4f<1, 2, 0, 2> yzxz; swizzle4f<2, 2, 0, 2> zzxz; swizzle4f<3, 2, 0, 2> wzxz;
            swizzle4f<0, 2, 0, 3> xzxw; swizzle4f<1, 2, 0, 3> yzxw; swizzle4f<2, 2, 0, 3> zzxw; swizzle4f<3, 2, 0, 3> wzxw;
            swizzle4f<0, 2, 1, 0> xzyx; swizzle4f<1, 2, 1, 0> yzyx; swizzle4f<2, 2, 1, 0> zzyx; swizzle4f<3, 2, 1, 0> wzyx;
            swizzle4f<0, 2, 1, 1> xzyy; swizzle4f<1, 2, 1, 1> yzyy; swizzle4f<2, 2, 1, 1> zzyy; swizzle4f<3, 2, 1, 1> wzyy;
            swizzle4f<0, 2, 1, 2> xzyz; swizzle4f<1, 2, 1, 2> yzyz; swizzle4f<2, 2, 1, 2> zzyz; swizzle4f<3, 2, 1, 2> wzyz;
            swizzle4f<0, 2, 1, 3> xzyw; swizzle4f<1, 2, 1, 3> yzyw; swizzle4f<2, 2, 1, 3> zzyw; swizzle4f<3, 2, 1, 3> wzyw;
            swizzle4f<0, 2, 2, 0> xzzx; swizzle4f<1, 2, 2, 0> yzzx; swizzle4f<2, 2, 2, 0> zzzx; swizzle4f<3, 2, 2, 0> wzzx;
            swizzle4f<0, 2, 2, 1> xzzy; swizzle4f<1, 2, 2, 1> yzzy; swizzle4f<2, 2, 2, 1> zzzy; swizzle4f<3, 2, 2, 1> wzzy;
            swizzle4f<0, 2, 2, 2> xzzz; swizzle4f<1, 2, 2, 2> yzzz; swizzle4f<2, 2, 2, 2> zzzz; swizzle4f<3, 2, 2, 2> wzzz;
            swizzle4f<0, 2, 2, 3> xzzw; swizzle4f<1, 2, 2, 3> yzzw; swizzle4f<2, 2, 2, 3> zzzw; swizzle4f<3, 2, 2, 3> wzzw;
            swizzle4f<0, 2, 3, 0> xzwx; swizzle4f<1, 2, 3, 0> yzwx; swizzle4f<2, 2, 3, 0> zzwx; swizzle4f<3, 2, 3, 0> wzwx;
            swizzle4f<0, 2, 3, 1> xzwy; swizzle4f<1, 2, 3, 1> yzwy; swizzle4f<2, 2, 3, 1> zzwy; swizzle4f<3, 2, 3, 1> wzwy;
            swizzle4f<0, 2, 3, 2> xzwz; swizzle4f<1, 2, 3, 2> yzwz; swizzle4f<2, 2, 3, 2> zzwz; swizzle4f<3, 2, 3, 2> wzwz;
            swizzle4f<0, 2, 3, 3> xzww; swizzle4f<1, 2, 3, 3> yzww; swizzle4f<2, 2, 3, 3> zzww; swizzle4f<3, 2, 3, 3> wzww;
            swizzle4f<0, 3, 0, 0> xwxx; swizzle4f<1, 3, 0, 0> ywxx; swizzle4f<2, 3, 0, 0> zwxx; swizzle4f<3, 3, 0, 0> wwxx;
            swizzle4f<0, 3, 0, 1> xwxy; swizzle4f<1, 3, 0, 1> ywxy; swizzle4f<2, 3, 0, 1> zwxy; swizzle4f<3, 3, 0, 1> wwxy;
            swizzle4f<0, 3, 0, 2> xwxz; swizzle4f<1, 3, 0, 2> ywxz; swizzle4f<2, 3, 0, 2> zwxz; swizzle4f<3, 3, 0, 2> wwxz;
            swizzle4f<0, 3, 0, 3> xwxw; swizzle4f<1, 3, 0, 3> ywxw; swizzle4f<2, 3, 0, 3> zwxw; swizzle4f<3, 3, 0, 3> wwxw;
            swizzle4f<0, 3, 1, 0> xwyx; swizzle4f<1, 3, 1, 0> ywyx; swizzle4f<2, 3, 1, 0> zwyx; swizzle4f<3, 3, 1, 0> wwyx;
            swizzle4f<0, 3, 1, 1> xwyy; swizzle4f<1, 3, 1, 1> ywyy; swizzle4f<2, 3, 1, 1> zwyy; swizzle4f<3, 3, 1, 1> wwyy;
            swizzle4f<0, 3, 1, 2> xwyz; swizzle4f<1, 3, 1, 2> ywyz; swizzle4f<2, 3, 1, 2> zwyz; swizzle4f<3, 3, 1, 2> wwyz;
            swizzle4f<0, 3, 1, 3> xwyw; swizzle4f<1, 3, 1, 3> ywyw; swizzle4f<2, 3, 1, 3> zwyw; swizzle4f<3, 3, 1, 3> wwyw;
            swizzle4f<0, 3, 2, 0> xwzx; swizzle4f<1, 3, 2, 0> ywzx; swizzle4f<2, 3, 2, 0> zwzx; swizzle4f<3, 3, 2, 0> wwzx;
            swizzle4f<0, 3, 2, 1> xwzy; swizzle4f<1, 3, 2, 1> ywzy; swizzle4f<2, 3, 2, 1> zwzy; swizzle4f<3, 3, 2, 1> wwzy;
            swizzle4f<0, 3, 2, 2> xwzz; swizzle4f<1, 3, 2, 2> ywzz; swizzle4f<2, 3, 2, 2> zwzz; swizzle4f<3, 3, 2, 2> wwzz;
            swizzle4f<0, 3, 2, 3> xwzw; swizzle4f<1, 3, 2, 3> ywzw; swizzle4f<2, 3, 2, 3> zwzw; swizzle4f<3, 3, 2, 3> wwzw;
            swizzle4f<0, 3, 3, 0> xwwx; swizzle4f<1, 3, 3, 0> ywwx; swizzle4f<2, 3, 3, 0> zwwx; swizzle4f<3, 3, 3, 0> wwwx;
            swizzle4f<0, 3, 3, 1> xwwy; swizzle4f<1, 3, 3, 1> ywwy; swizzle4f<2, 3, 3, 1> zwwy; swizzle4f<3, 3, 3, 1> wwwy;
            swizzle4f<0, 3, 3, 2> xwwz; swizzle4f<1, 3, 3, 2> ywwz; swizzle4f<2, 3, 3, 2> zwwz; swizzle4f<3, 3, 3, 2> wwwz;
            swizzle4f<0, 3, 3, 3> xwww; swizzle4f<1, 3, 3, 3> ywww; swizzle4f<2, 3, 3, 3> zwww; swizzle4f<3, 3, 3, 3> wwww;
        };
        
        vector4f() = default;
        vector4f(const vector4f &) = default;
        vector4f(const scalar s) : x(s), y(s), z(s), w(s) {}
        vector4f(const scalar (&xyzw)[4]) : x(xyzw[0]), y(xyzw[1]), z(xyzw[2]), w(xyzw[3]) {}
        vector4f(scalar x, scalar y, scalar z, scalar w) : x(x), y(y), z(z), w(w) {}
        vector4f(const vector3f &xyz, scalar w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}
        vector4f(scalar x, const vector3f &yzw) : x(x), y(yzw.x), z(yzw.y), w(yzw.z) {}
        vector4f(scalar x, scalar y, const vector2f &zw) : x(x), y(y), z(zw.x), w(zw.y) {}
        vector4f(scalar x, const vector2f &yz, scalar w) : x(x), y(yz.x), z(yz.y), w(w) {}
        vector4f(const vector2f &xy, scalar z, scalar w) : x(xy.x), y(xy.y), z(z), w(w) {}
        vector4f(const vector2f &xy, const vector2f &zw) : x(xy.x), y(xy.y), z(zw.x), w(zw.y) {}
        vector4f& operator =(const vector4f &);
    };
    struct transform2f {
        union {
            struct {
                scalar m11, m12, m13;
                scalar m21, m22, m23;
                scalar m31, m32, m33;
            };
            struct {
                scalar flat[9];
            };
            struct {
                scalar row0[3];
                scalar row1[3];
                scalar row2[3];
            };
            struct {
                scalar data[9];
            }
            block;
        };
        
        transform2f() = default;
        transform2f(const vector3f &r0, const vector3f &r1, const vector3f &r2);
        transform2f(scalar radians);
        transform2f& operator =(const transform2f& other);
        
        transform2f translated(const vector2f &v) const;
        transform2f rotated(scalar radians) const;
        transform2f scaled(const vector2f &v) const;
        transform2f inverted() const;
    };
    struct transform3f {
        union {
            struct {
                scalar m11, m12, m13, m14;
                scalar m21, m22, m23, m24;
                scalar m31, m32, m33, m34;
                scalar m41, m42, m43, m44;
            };
            struct {
                scalar flat[16];
            };
            struct {
                scalar row0[4];
                scalar row1[4];
                scalar row2[4];
                scalar row3[4];
            };
            struct {
                decltype(vector4f::block) rv0;
                decltype(vector4f::block) rv1;
                decltype(vector4f::block) rv2;
                decltype(vector4f::block) rv3;
            };
            struct {
                scalar data[16];
            }
            block;
        };
        
        static transform3f lookAtRH(const vector3f &eye, const vector3f &at, const vector3f &up);
        static transform3f perspectiveFovRH(scalar fovY, scalar aspect, scalar zNear, scalar zFar);
        
        transform3f() = default;
        transform3f(const vector4f &r0, const vector4f &r1, const vector4f &r2, const vector4f &r3);
        transform3f(const vector3f &axis, scalar radians);
        transform3f& operator =(const transform3f& other);
        
        transform3f translated(const vector3f &v) const;
        transform3f rotated(const vector3f &axis, scalar radians) const;
        transform3f scaled(const vector3f &v) const;
        transform3f inverted() const;
    };
    
    static_assert(sizeof(vector2f) == 2 * sizeof(scalar), "");
    static_assert(sizeof(vector3f) == 3 * sizeof(scalar), "");
    static_assert(sizeof(vector4f) == 4 * sizeof(scalar), "");
    static_assert(sizeof(transform2f) == 9 * sizeof(scalar), "");
    static_assert(sizeof(transform3f) == 16 * sizeof(scalar), "");

    inline vector2f& vector2f::operator =(const vector2f &other) {
        block = other.block;
        return *this;
    }
    inline vector3f& vector3f::operator =(const vector3f &other) {
        block = other.block;
        return *this;
    }
    inline vector4f& vector4f::operator =(const vector4f &other) {
        block = other.block;
        return *this;
    }

    template<int Ix, int Iy>
    inline vector2f &swizzle2f<Ix, Iy>::operator=(const swizzle2f<Ix, Iy> &other) {
        scalar (&flat)[4] = asFlat();
        flat[Ix] = other[Ix]; flat[Iy] = other[Ix];
        return *reinterpret_cast<vector2f *>(this);
    }
    template<int Ix, int Iy, int Iz>
    inline vector3f &swizzle3f<Ix, Iy, Iz>::operator=(const swizzle3f<Ix, Iy, Iz> &other) {
        scalar (&flat)[4] = asFlat();
        flat[Ix] = other[Ix]; flat[Iy] = other[Iy]; flat[Iz] = other[Iz];
        return *reinterpret_cast<vector3f *>(this);
    }
    template<int Ix, int Iy, int Iz, int Iw>
    inline vector4f &swizzle4f<Ix, Iy, Iz, Iw>::operator=(const swizzle4f<Ix, Iy, Iz, Iw> &other) {
        scalar (&flat)[4] = asFlat();
        flat[Ix] = other[Ix]; flat[Iy] = other[Iy]; flat[Iz] = other[Iz]; flat[Iw] = other[Iw];
        return *reinterpret_cast<vector4f *>(this);
    }

    template<int Ix, int Iy>
    template<int Ox, int Oy>
    inline vector2f &swizzle2f<Ix, Iy>::operator=(const swizzle2f<Ox, Oy> &other) {
        scalar (&flat)[4] = asFlat();
        flat[Ix] = other[Ox]; flat[Iy] = other[Oy];
        return *reinterpret_cast<vector2f *>(this);
    }
    template<int Ix, int Iy, int Iz>
    template<int Ox, int Oy, int Oz>
    inline vector3f &swizzle3f<Ix, Iy, Iz>::operator=(const swizzle3f<Ox, Oy, Oz> &other) {
        scalar (&flat)[4] = asFlat();
        flat[Ix] = other[Ox]; flat[Iy] = other[Oy]; flat[Iz] = other[Oz];
        return *reinterpret_cast<vector3f *>(this);
    }
    template<int Ix, int Iy, int Iz, int Iw>
    template<int Ox, int Oy, int Oz, int Ow>
    inline vector4f &swizzle4f<Ix, Iy, Iz, Iw>::operator=(const swizzle4f<Ox, Oy, Oz, Ow> &other) {
        scalar (&flat)[4] = asFlat();
        flat[Ix] = other[Ox]; flat[Iy] = other[Oy]; flat[Iz] = other[Oz]; flat[Iw] = other[Ow];
        return *reinterpret_cast<vector4f *>(this);
    }
    
    template<int Ix, int Iy>
    inline swizzle2f<Ix, Iy>::operator vector2f() const {
        const scalar (&flat)[4] = asFlat();
        return {flat[Ix], flat[Iy]};
    }
    template<int Ix, int Iy, int Iz>
    inline swizzle3f<Ix, Iy, Iz>::operator vector3f() const {
        const scalar (&flat)[4] = asFlat();
        return {flat[Ix], flat[Iy], flat[Iz]};
    }
    template<int Ix, int Iy, int Iz, int Iw>
    inline swizzle4f<Ix, Iy, Iz, Iw>::operator vector4f() const {
        const scalar (&flat)[4] = asFlat();
        return {flat[Ix], flat[Iy], flat[Iz], flat[Iw]};
    }
    
    //---

    template<int X0, int Y0, int Z0, int W0, int X1, int Y1, int Z1, int W1>
    inline vector4f operator *(const swizzle4f<X0, Y0, Z0, W0> &v0, const swizzle4f<X1, Y1, Z1, W1> &v1) {
        return {v0[X0] * v1[X1], v0[Y0] * v1[Y1], v0[Z0] * v1[Z1], v0[W0] * v1[W1]};
    }
    template<int X0, int Y0, int Z0, int X1, int Y1, int Z1>
    inline vector3f operator *(const swizzle3f<X0, Y0, Z0> &v0, const swizzle3f<X1, Y1, Z1> &v1) {
        return {v0[X0] * v1[X1], v0[Y0] * v1[Y1], v0[Z0] * v1[Z1]};
    }
    template<int X0, int Y0, int X1, int Y1>
    inline vector2f operator *(const swizzle2f<X0, Y0> &v0, const swizzle2f<X1, Y1> &v1) {
        return {v0[X0] * v1[X1], v0[Y0] * v1[Y1]};
    }
    template<int X0, int Y0, int Z0, int W0>
    inline vector4f operator *(const swizzle4f<X0, Y0, Z0, W0> &v0, scalar s1) {
        return {v0[X0] * s1, v0[Y0] * s1, v0[Z0] * s1, v0[W0] * s1};
    }
    template<int X1, int Y1, int Z1, int W1>
    inline vector4f operator *(scalar s0, const swizzle4f<X1, Y1, Z1, W1> &v1) {
        return {s0 * v1[X1], s0 * v1[Y1], s0 * v1[Z1], s0 * v1[W1]};
    }
    template<int X0, int Y0, int Z0>
    inline vector3f operator *(const swizzle3f<X0, Y0, Z0> &v0, scalar s1) {
        return {v0[X0] * s1, v0[Y0] * s1, v0[Z0] * s1};
    }
    template<int X1, int Y1, int Z1>
    inline vector3f operator *(scalar s0, const swizzle3f<X1, Y1, Z1> &v1) {
        return {s0 * v1[X1], s0 * v1[Y1], s0 * v1[Z1]};
    }
    template<int X0, int Y0>
    inline vector2f operator *(const swizzle2f<X0, Y0> &v0, scalar s1) {
        return {v0[X0] * s1, v0[Y0] * s1};
    }
    template<int X1, int Y1>
    inline vector2f operator *(scalar s0, const swizzle2f<X1, Y1> &v1) {
        return {s0 * v1[X1], s0 * v1[Y1]};
    }

    template<int X0, int Y0, int Z0, int W0, int X1, int Y1, int Z1, int W1>
    inline vector4f operator /(const swizzle4f<X0, Y0, Z0, W0> &v0, const swizzle4f<X1, Y1, Z1, W1> &v1) {
        return {v0[X0] / v1[X1], v0[Y0] / v1[Y1], v0[Z0] / v1[Z1], v0[W0] / v1[W1]};
    }
    template<int X0, int Y0, int Z0, int X1, int Y1, int Z1>
    inline vector3f operator /(const swizzle3f<X0, Y0, Z0> &v0, const swizzle3f<X1, Y1, Z1> &v1) {
        return {v0[X0] / v1[X1], v0[Y0] / v1[Y1], v0[Z0] / v1[Z1]};
    }
    template<int X0, int Y0, int X1, int Y1>
    inline vector2f operator /(const swizzle2f<X0, Y0> &v0, const swizzle2f<X1, Y1> &v1) {
        return {v0[X0] / v1[X1], v0[Y0] / v1[Y1]};
    }
    template<int X0, int Y0, int Z0, int W0>
    inline vector4f operator /(const swizzle4f<X0, Y0, Z0, W0> &v0, scalar s1) {
        return {v0[X0] / s1, v0[Y0] / s1, v0[Z0] / s1, v0[W0] / s1};
    }
    template<int X1, int Y1, int Z1, int W1>
    inline vector4f operator /(scalar s0, const swizzle4f<X1, Y1, Z1, W1> &v1) {
        return {s0 / v1[X1], s0 / v1[Y1], s0 / v1[Z1], s0 / v1[W1]};
    }
    template<int X0, int Y0, int Z0>
    inline vector3f operator /(const swizzle3f<X0, Y0, Z0> &v0, scalar s1) {
        return {v0[X0] / s1, v0[Y0] / s1, v0[Z0] / s1};
    }
    template<int X1, int Y1, int Z1>
    inline vector3f operator /(scalar s0, const swizzle3f<X1, Y1, Z1> &v1) {
        return {s0 / v1[X1], s0 / v1[Y1], s0 / v1[Z1]};
    }
    template<int X0, int Y0>
    inline vector2f operator /(const swizzle2f<X0, Y0> &v0, scalar s1) {
        return {v0[X0] / s1, v0[Y0] / s1};
    }
    template<int X1, int Y1>
    inline vector2f operator /(scalar s0, const swizzle2f<X1, Y1> &v1) {
        return {s0 / v1[X1], s0 / v1[Y1]};
    }

    template<int X0, int Y0, int Z0, int W0, int X1, int Y1, int Z1, int W1>
    inline vector4f operator +(const swizzle4f<X0, Y0, Z0, W0> &v0, const swizzle4f<X1, Y1, Z1, W1> &v1) {
        return {v0[X0] + v1[X1], v0[Y0] + v1[Y1], v0[Z0] + v1[Z1], v0[W0] + v1[W1]};
    }
    template<int X0, int Y0, int Z0, int X1, int Y1, int Z1>
    inline vector3f operator +(const swizzle3f<X0, Y0, Z0> &v0, const swizzle3f<X1, Y1, Z1> &v1) {
        return {v0[X0] + v1[X1], v0[Y0] + v1[Y1], v0[Z0] + v1[Z1]};
    }
    template<int X0, int Y0, int X1, int Y1>
    inline vector2f operator +(const swizzle2f<X0, Y0> &v0, const swizzle2f<X1, Y1> &v1) {
        return {v0[X0] + v1[X1], v0[Y0] + v1[Y1]};
    }

    template<int X0, int Y0, int Z0, int W0, int X1, int Y1, int Z1, int W1>
    inline vector4f operator -(const swizzle4f<X0, Y0, Z0, W0> &v0, const swizzle4f<X1, Y1, Z1, W1> &v1) {
        return {v0[X0] - v1[X1], v0[Y0] - v1[Y1], v0[Z0] - v1[Z1], v0[W0] - v1[W1]};
    }
    template<int X0, int Y0, int Z0, int X1, int Y1, int Z1>
    inline vector3f operator -(const swizzle3f<X0, Y0, Z0> &v0, const swizzle3f<X1, Y1, Z1> &v1) {
        return {v0[X0] - v1[X1], v0[Y0] - v1[Y1], v0[Z0] - v1[Z1]};
    }
    template<int X0, int Y0, int X1, int Y1>
    inline vector2f operator -(const swizzle2f<X0, Y0> &v0, const swizzle2f<X1, Y1> &v1) {
        return {v0[X0] - v1[X1], v0[Y0] - v1[Y1]};
    }

    //---
    
    template<int Ix, int Iy>
    inline scalar swizzle2f<Ix, Iy>::length() const {
        const scalar (&flat)[4] = asFlat();
        return std::sqrt(flat[Ix] * flat[Ix] + flat[Iy] * flat[Iy]);
    }
    template<int Ix, int Iy>
    inline scalar swizzle2f<Ix, Iy>::lengthSq() const {
        const scalar (&flat)[4] = asFlat();
        return flat[Ix] * flat[Ix] + flat[Iy] * flat[Iy];
    }
    template<int Ix, int Iy>
    inline vector2f swizzle2f<Ix, Iy>::normalized(scalar length) const {
        const scalar (&flat)[4] = asFlat();
        const scalar lm = this->length();
        const scalar k = lm > std::numeric_limits<scalar>::epsilon() ? length / lm : 1.0;
        return vector2f(flat[Ix] * k, flat[Iy] * k);
    }
    template<int Ix, int Iy>
    inline vector4f swizzle2f<Ix, Iy>::atv4start(scalar z, scalar w) const {
        const scalar (&flat)[4] = asFlat();
        return vector4f(flat[Ix], flat[Iy], z, w);
    }
    template<int Ix, int Iy>
    inline vector4f swizzle2f<Ix, Iy>::atv4middle(scalar x, scalar w) const {
        const scalar (&flat)[4] = asFlat();
        return vector4f(x, flat[Ix], flat[Iy], w);
    }
    template<int Ix, int Iy>
    inline vector4f swizzle2f<Ix, Iy>::atv4end(scalar x, scalar y) const {
        const scalar (&flat)[4] = asFlat();
        return vector4f(x, y, flat[Ix], flat[Iy]);
    }
    template<int Ix, int Iy>
    inline scalar swizzle2f<Ix, Iy>::dot(const vector2f &other) const {
        const scalar (&flat)[4] = asFlat();
        return flat[Ix] * other.x + flat[Iy] * other.y;
    }
    template<int Ix, int Iy>
    inline scalar swizzle2f<Ix, Iy>::cross(const vector2f &other) const {
        const scalar (&flat)[4] = asFlat();
        return flat[Ix] * other.y - flat[Iy] * other.x;
    }
    template<int Ix, int Iy>
    inline vector2f swizzle2f<Ix, Iy>::rotated(scalar radians) const {
        const float rsin = std::sin(radians);
        const float rcos = std::cos(radians);
        const scalar (&flat)[4] = asFlat();
        const scalar rx = flat[Ix] * rcos - flat[Iy] * rsin;
        const scalar ry = flat[Ix] * rsin + flat[Iy] * rcos;
        return {rx, ry};
    }
    template<int Ix, int Iy>
    inline scalar swizzle2f<Ix, Iy>::angleTo(const vector2f &other) const {
        return std::acos(std::min(scalar(1.0), std::max(scalar(-1.0), dot(other) / sqrtf(lengthSq() * other.lengthSq()))));
    }
    template<int Ix, int Iy>
    inline scalar swizzle2f<Ix, Iy>::distanceTo(const vector2f &other) const {
        const vector2f diff = other - *this;
        return std::sqrt(diff.dot(diff));
    }
    template<int Ix, int Iy>
    inline vector2f swizzle2f<Ix, Iy>::transformed(const transform2f &trfm, bool likePosition) const {
        const scalar (&flat)[4] = asFlat();
        const float z = likePosition ? 1.0f : 0.0f;
        return {
            flat[Ix] * trfm.m11 + flat[Iy] * trfm.m21 + z * trfm.m31,
            flat[Ix] * trfm.m12 + flat[Iy] * trfm.m22 + z * trfm.m32,
        };
    }
    
    //---
    
    template<int Ix, int Iy, int Iz>
    inline scalar swizzle3f<Ix, Iy, Iz>::length() const {
        const scalar (&flat)[4] = asFlat();
        return std::sqrt(flat[Ix] * flat[Ix] + flat[Iy] * flat[Iy] + flat[Iz] * flat[Iz]);
    }
    template<int Ix, int Iy, int Iz>
    inline scalar swizzle3f<Ix, Iy, Iz>::lengthSq() const {
        const scalar (&flat)[4] = asFlat();
        return flat[Ix] * flat[Ix] + flat[Iy] * flat[Iy] + flat[Iz] * flat[Iz];
    }
    template<int Ix, int Iy, int Iz>
    inline vector3f swizzle3f<Ix, Iy, Iz>::normalized(scalar length) const {
        const scalar (&flat)[4] = asFlat();
        const scalar lm = this->length();
        const scalar k = lm > std::numeric_limits<scalar>::epsilon() ? length / lm : 1.0;
        return vector3f(flat[Ix] * k, flat[Iy] * k, flat[Iz] * k);
    }
    template<int Ix, int Iy, int Iz>
    inline vector4f swizzle3f<Ix, Iy, Iz>::atv4start(scalar w) const {
        const scalar (&flat)[4] = asFlat();
        return vector4f(flat[Ix], flat[Iy], flat[Iz], w);
    }
    template<int Ix, int Iy, int Iz>
    inline vector4f swizzle3f<Ix, Iy, Iz>::atv4end(scalar x) const {
        const scalar (&flat)[4] = asFlat();
        return vector4f(x, flat[Ix], flat[Iy], flat[Iz]);
    }
    template<int Ix, int Iy, int Iz>
    inline scalar swizzle3f<Ix, Iy, Iz>::dot(const vector3f &other) const {
        const scalar (&flat)[4] = asFlat();
        return flat[Ix] * other.flat[0] + flat[Iy] * other.flat[1] + flat[Iz] * other.flat[2];
    }
    template<int Ix, int Iy, int Iz>
    inline vector3f swizzle3f<Ix, Iy, Iz>::cross(const vector3f &other) const {
        const scalar (&flat)[4] = asFlat();
        return {
            flat[Iy] * other.z - flat[Iz] * other.y, flat[Iz] * other.x - flat[Ix] * other.z, flat[Ix] * other.y - flat[Iy] * other.x
        };
    }
    template<int Ix, int Iy, int Iz>
    inline scalar swizzle3f<Ix, Iy, Iz>::angleTo(const vector3f &other) const {
        return std::acos(std::min(scalar(1.0), std::max(scalar(-1.0), dot(other) / sqrtf(lengthSq() * other.lengthSq()))));
    }
    template<int Ix, int Iy, int Iz>
    inline vector3f swizzle3f<Ix, Iy, Iz>::rotated(const vector3f &axis, scalar radians) const {
        const scalar rsin = std::sin(-radians * 0.5);
        const scalar rcos = std::cos(-radians * 0.5);
        const vector4f q = vector4f(axis.x * rsin, axis.y * rsin, axis.z * rsin, rcos);
        const vector4f invq = vector4f(-q.x, -q.y, -q.z, q.w);
        const vector4f p = this->atv4start(0.0f);
        return invq.quaternionMultiply(p).quaternionMultiply(q).xyz;
    }
    template<int Ix, int Iy, int Iz>
    inline scalar swizzle3f<Ix, Iy, Iz>::distanceTo(const vector3f &other) const {
        const vector3f diff = other - *this;
        return std::sqrt(diff.dot(diff));
    }
    template<int Ix, int Iy, int Iz>
    inline vector3f swizzle3f<Ix, Iy, Iz>::transformed(const transform3f &trfm, bool likePosition) const {
        const scalar flat[4] = {(*this)[Ix], (*this)[Iy], (*this)[Iz], likePosition ? 1.0 : 0.0};
        return {
            flat[0] * trfm.row0[0] + flat[1] * trfm.row1[0] + flat[2] * trfm.row2[0] + flat[3] * trfm.row3[0],
            flat[0] * trfm.row0[1] + flat[1] * trfm.row1[1] + flat[2] * trfm.row2[1] + flat[3] * trfm.row3[1],
            flat[0] * trfm.row0[2] + flat[1] * trfm.row1[2] + flat[2] * trfm.row2[2] + flat[3] * trfm.row3[2]
        };
    }
    
    //---
    
    template<int Ix, int Iy, int Iz, int Iw>
    inline scalar swizzle4f<Ix, Iy, Iz, Iw>::dot(const vector4f &other) const {
        const scalar (&flat)[4] = asFlat();
        return flat[Ix] * other.flat[0] + flat[Iy] * other.flat[1] + flat[Iz] * other.flat[2] + flat[Iw] * other.flat[3];
    }
    template<int Ix, int Iy, int Iz, int Iw>
    inline vector4f swizzle4f<Ix, Iy, Iz, Iw>::transformed(const transform3f &trfm) const {
        const scalar flat[4] = {(*this)[Ix], (*this)[Iy], (*this)[Iz], (*this)[Iw]};
        return {
            flat[0] * trfm.row0[0] + flat[1] * trfm.row1[0] + flat[2] * trfm.row2[0] + flat[3] * trfm.row3[0],
            flat[0] * trfm.row0[1] + flat[1] * trfm.row1[1] + flat[2] * trfm.row2[1] + flat[3] * trfm.row3[1],
            flat[0] * trfm.row0[2] + flat[1] * trfm.row1[2] + flat[2] * trfm.row2[2] + flat[3] * trfm.row3[2],
            flat[0] * trfm.row0[3] + flat[1] * trfm.row1[3] + flat[2] * trfm.row2[3] + flat[3] * trfm.row3[3]
        };
    }
    template<int Ix, int Iy, int Iz, int Iw>
    inline vector4f swizzle4f<Ix, Iy, Iz, Iw>::quaternionMultiply(const vector4f &other) const {
        const scalar (&flat)[4] = asFlat();
        return {
            other.flat[1] * flat[Iz] - other.flat[2] * flat[Iy] + other.flat[3] * flat[Ix] + other.flat[0] * flat[Iw],
            other.flat[2] * flat[Ix] - other.flat[0] * flat[Iz] + other.flat[3] * flat[Iy] + other.flat[1] * flat[Iw],
            other.flat[0] * flat[Iy] - other.flat[1] * flat[Ix] + other.flat[3] * flat[Iz] + other.flat[2] * flat[Iw],
            other.flat[3] * flat[Iw] - other.flat[0] * flat[Ix] - other.flat[1] * flat[Iy] - other.flat[2] * flat[Iz],
        };
    }
    
    //---
    
    inline transform3f transform3f::lookAtRH(const vector3f &eye, const vector3f &at, const vector3f &up) {
        vector3f tmpz = vector3f(eye.x - at.x, eye.y - at.y, eye.z - at.z).normalized(1.0);
        vector3f tmpx = up.cross(tmpz).normalized(1.0);
        vector3f tmpy = tmpz.cross(tmpx).normalized(1.0);
        
        return {
            vector4f(tmpx.x, tmpy.x, tmpz.x, 0.0f),
            vector4f(tmpx.y, tmpy.y, tmpz.y, 0.0f),
            vector4f(tmpx.z, tmpy.z, tmpz.z, 0.0f),
            vector4f(-tmpx.dot(eye), -tmpy.dot(eye), -tmpz.dot(eye), 1.0f),
        };
    }
    
    inline transform3f transform3f::perspectiveFovRH(scalar fovY, scalar aspect, scalar zNear, scalar zFar) {
        scalar yS = scalar(1.0f) / std::tan(fovY * scalar(0.5));
        scalar xS = yS / aspect;
        
        return {
            vector4f(xS, 0, 0, 0),
            vector4f(0, yS, 0, 0),
            vector4f(0, 0, zNear / (zFar - zNear), -scalar(1.0)),
            vector4f(0, 0, (zFar * zNear) / (zFar - zNear), 0),
        };
    }
    
    inline transform3f::transform3f(const vector4f &r0, const vector4f &r1, const vector4f &r2, const vector4f &r3) {
        rv0 = r0.block;
        rv1 = r1.block;
        rv2 = r2.block;
        rv3 = r3.block;
    }
    inline transform3f::transform3f(const vector3f &axis, scalar radians) {
        const scalar sina = std::sin(-radians * 0.5);
        const scalar cosa = std::cos(-radians * 0.5);
        
        const vector4f q = vector4f(axis.x * sina, axis.y * sina, axis.z * sina, cosa);
        const vector4f xq = vector4f(2.0 * q.x) * q;
        const vector4f wq = vector4f(2.0 * q.w) * q;
        
        scalar yy = 2.0 * q.y * q.y;
        scalar yz = 2.0 * q.y * q.z;
        scalar zz = 2.0 * q.z * q.z;

        rv0 = {scalar(1.0) - (yy + zz), xq.y + wq.z, xq.z - wq.y, 0.0};
        rv1 = {xq.y - wq.z, scalar(1.0) - (xq.x + zz), yz + wq.x, 0.0};
        rv2 = {xq.z + wq.y, yz - wq.x, scalar(1.0) - (xq.x + yy), 0.0};
        rv3 = {0, 0, 0, 1};
    }
    inline transform3f& transform3f::operator =(const transform3f& other) {
        this->block = other.block;
        return *this;
    }
    inline transform3f transform3f::translated(const vector3f &v) const {
        return {
            vector4f(row0),
            vector4f(row1),
            vector4f(row2),
            vector4f(m41 + v.x, m42 + v.y, m43 + v.z, m44)
        };
    }
    inline transform3f transform3f::inverted() const {
        scalar a2323 = row2[2] * row3[3] - row2[3] * row3[2];
        scalar a1323 = row2[1] * row3[3] - row2[3] * row3[1];
        scalar a1223 = row2[1] * row3[2] - row2[2] * row3[1];
        scalar a0323 = row2[0] * row3[3] - row2[3] * row3[0];
        scalar a0223 = row2[0] * row3[2] - row2[2] * row3[0];
        scalar a0123 = row2[0] * row3[1] - row2[1] * row3[0];
        scalar a2313 = row1[2] * row3[3] - row1[3] * row3[2];
        scalar a1313 = row1[1] * row3[3] - row1[3] * row3[1];
        scalar a1213 = row1[1] * row3[2] - row1[2] * row3[1];
        scalar a2312 = row1[2] * row2[3] - row1[3] * row2[2];
        scalar a1312 = row1[1] * row2[3] - row1[3] * row2[1];
        scalar a1212 = row1[1] * row2[2] - row1[2] * row2[1];
        scalar a0313 = row1[0] * row3[3] - row1[3] * row3[0];
        scalar a0213 = row1[0] * row3[2] - row1[2] * row3[0];
        scalar a0312 = row1[0] * row2[3] - row1[3] * row2[0];
        scalar a0212 = row1[0] * row2[2] - row1[2] * row2[0];
        scalar a0113 = row1[0] * row3[1] - row1[1] * row3[0];
        scalar a0112 = row1[0] * row2[1] - row1[1] * row2[0];

        scalar det = 1.0 / (
            row0[0] * (row1[1] * a2323 - row1[2] * a1323 + row1[3] * a1223) -
            row0[1] * (row1[0] * a2323 - row1[2] * a0323 + row1[3] * a0223) +
            row0[2] * (row1[0] * a1323 - row1[1] * a0323 + row1[3] * a0123) -
            row0[3] * (row1[0] * a1223 - row1[1] * a0223 + row1[2] * a0123)
        );

        return transform3f{
            vector4f(
                det *  (row1[1] * a2323 - row1[2] * a1323 + row1[3] * a1223),
                det * -(row0[1] * a2323 - row0[2] * a1323 + row0[3] * a1223),
                det *  (row0[1] * a2313 - row0[2] * a1313 + row0[3] * a1213),
                det * -(row0[1] * a2312 - row0[2] * a1312 + row0[3] * a1212)
            ),
            vector4f(
                det * -(row1[0] * a2323 - row1[2] * a0323 + row1[3] * a0223),
                det *  (row0[0] * a2323 - row0[2] * a0323 + row0[3] * a0223),
                det * -(row0[0] * a2313 - row0[2] * a0313 + row0[3] * a0213),
                det *  (row0[0] * a2312 - row0[2] * a0312 + row0[3] * a0212)
            ),
            vector4f(
                det *  (row1[0] * a1323 - row1[1] * a0323 + row1[3] * a0123),
                det * -(row0[0] * a1323 - row0[1] * a0323 + row0[3] * a0123),
                det *  (row0[0] * a1313 - row0[1] * a0313 + row0[3] * a0113),
                det * -(row0[0] * a1312 - row0[1] * a0312 + row0[3] * a0112)
            ),
            vector4f(
                det * -(row1[0] * a1223 - row1[1] * a0223 + row1[2] * a0123),
                det *  (row0[0] * a1223 - row0[1] * a0223 + row0[2] * a0123),
                det * -(row0[0] * a1213 - row0[1] * a0213 + row0[2] * a0113),
                det *  (row0[0] * a1212 - row0[1] * a0212 + row0[2] * a0112)
            )
        };
    }
    
    inline transform3f operator *(const transform3f &t0, const transform3f &t1) {
        vector4f r0, r1, r2, r3;
        
        r0.flat[0] = t0.row0[0] * t1.row0[0] + t0.row0[1] * t1.row1[0] + t0.row0[2] * t1.row2[0] + t0.row0[3] * t1.row3[0];
        r0.flat[1] = t0.row0[0] * t1.row0[1] + t0.row0[1] * t1.row1[1] + t0.row0[2] * t1.row2[1] + t0.row0[3] * t1.row3[1];
        r0.flat[2] = t0.row0[0] * t1.row0[2] + t0.row0[1] * t1.row1[2] + t0.row0[2] * t1.row2[2] + t0.row0[3] * t1.row3[2];
        r0.flat[3] = t0.row0[0] * t1.row0[3] + t0.row0[1] * t1.row1[3] + t0.row0[2] * t1.row2[3] + t0.row0[3] * t1.row3[3];
        
        r1.flat[0] = t0.row1[0] * t1.row0[0] + t0.row1[1] * t1.row1[0] + t0.row1[2] * t1.row2[0] + t0.row1[3] * t1.row3[0];
        r1.flat[1] = t0.row1[0] * t1.row0[1] + t0.row1[1] * t1.row1[1] + t0.row1[2] * t1.row2[1] + t0.row1[3] * t1.row3[1];
        r1.flat[2] = t0.row1[0] * t1.row0[2] + t0.row1[1] * t1.row1[2] + t0.row1[2] * t1.row2[2] + t0.row1[3] * t1.row3[2];
        r1.flat[3] = t0.row1[0] * t1.row0[3] + t0.row1[1] * t1.row1[3] + t0.row1[2] * t1.row2[3] + t0.row1[3] * t1.row3[3];
        
        r2.flat[0] = t0.row2[0] * t1.row0[0] + t0.row2[1] * t1.row1[0] + t0.row2[2] * t1.row2[0] + t0.row2[3] * t1.row3[0];
        r2.flat[1] = t0.row2[0] * t1.row0[1] + t0.row2[1] * t1.row1[1] + t0.row2[2] * t1.row2[1] + t0.row2[3] * t1.row3[1];
        r2.flat[2] = t0.row2[0] * t1.row0[2] + t0.row2[1] * t1.row1[2] + t0.row2[2] * t1.row2[2] + t0.row2[3] * t1.row3[2];
        r2.flat[3] = t0.row2[0] * t1.row0[3] + t0.row2[1] * t1.row1[3] + t0.row2[2] * t1.row2[3] + t0.row2[3] * t1.row3[3];
        
        r3.flat[0] = t0.row3[0] * t1.row0[0] + t0.row3[1] * t1.row1[0] + t0.row3[2] * t1.row2[0] + t0.row3[3] * t1.row3[0];
        r3.flat[1] = t0.row3[0] * t1.row0[1] + t0.row3[1] * t1.row1[1] + t0.row3[2] * t1.row2[1] + t0.row3[3] * t1.row3[1];
        r3.flat[2] = t0.row3[0] * t1.row0[2] + t0.row3[1] * t1.row1[2] + t0.row3[2] * t1.row2[2] + t0.row3[3] * t1.row3[2];
        r3.flat[3] = t0.row3[0] * t1.row0[3] + t0.row3[1] * t1.row1[3] + t0.row3[2] * t1.row2[3] + t0.row3[3] * t1.row3[3];
        
        return transform3f(r0, r1, r2, r3);
    }
}

namespace math {
    struct bound2f {
        scalar xmin;
        scalar ymin;
        scalar xmax;
        scalar ymax;
    };
    struct bound3f {
        scalar xmin;
        scalar ymin;
        scalar zmin;
        scalar xmax;
        scalar ymax;
        scalar zmax;
    };
    
    //---
    
    struct color {
        float r, g, b, a;
        
        color() = default;
        color(unsigned rgba) : r(float(rgba & 255) / 255.0f), g(float((rgba >> 8) & 255) / 255.0f), b(float((rgba >> 16) & 255) / 255.0f), a(float((rgba >> 24) & 255) / 255.0f) {}
        color(scalar r, scalar g, scalar b, scalar a) : r(r), g(g), b(b), a(a) {}
        
        operator unsigned() const {
            return (unsigned(r * 255.0f) & 255) << 0 | (unsigned(g * 255.0f) & 255) << 8 | (unsigned(b * 255.0f) & 255) << 16 | (unsigned(a * 255.0f) & 255) << 24;
        }
        operator vector4f() const {
            return {r, g, b, a};
        }
    };

    // Find intersection point between Ray and Plane
    // Assuming @dir and @planeNormal are normalised
    //
    inline bool intersectRayPlane(const vector3f &origin, const vector3f &dir, const vector3f &planeRef, const vector3f &planeNormal, vector3f &out) {
        float d = planeRef.dot(planeNormal);
        float n = dir.dot(planeNormal);
        
        if (std::fabs(n) > std::numeric_limits<float>::epsilon()) {
            float k = (d - origin.dot(planeNormal)) / n;
            
            if (k > 0.0f) {
                out = origin + k * dir;
                return true;
            }
        }
        
        return false;
    }
}
