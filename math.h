#pragma once

// TODO: +=, *=, etc
// TODO: xOz
// TODO: at(index, index) -> xy
// TODO: clamped length of vector

#include <limits>
#include <algorithm>
#include <cmath>

namespace math
{
    using scalar = float;

    struct vector2f;
    struct vector3f;
    struct vector4f;
    struct quaternion;
    struct transform2f;
    struct transform3f;
    struct bound2f;
    struct bound3f;
    struct color;
    
    namespace imp {
        //------------------------------------------------------------------------------------------------------------------------------------------------------
        // vector2base
        // TODO: chessboard distace
        // TODO: absMin/absMax

        template <std::size_t Tx, std::size_t Ty> struct vector2base {
            vector2base &operator =(const vector2f &v);
            template <std::size_t X, std::size_t Y> vector2base &operator =(const vector2base<X, Y> &v) {
                _init({v[X], v[Y]});
                return *this;
            }
            vector2base &operator =(scalar s) {
                _init({s, s});
                return *this;
            }

            operator vector2f();

            scalar distanceTo(const vector2f &v) const;
            scalar distanceSqTo(const vector2f &v) const;
            scalar cross(const vector2f &v) const;
            scalar dot(const vector2f &v) const;
            scalar angleTo(const vector2f &v) const;
            
            scalar length() const {
                return std::sqrt((*this)[Tx] * (*this)[Tx] + (*this)[Ty] * (*this)[Ty]);
            }

            scalar lengthSq() const {
                return (*this)[Tx] * (*this)[Tx] + (*this)[Ty] * (*this)[Ty];
            }

            scalar min() const {
                return std::min((*this)[Tx], (*this)[Ty]);
            }
            
            scalar max() const {
                return std::max((*this)[Tx], (*this)[Ty]);
            }
            
            vector2f sign() const;
            vector2f clamped(scalar min, scalar max) const;
            vector2f bounded(const bound2f &b) const;
            
            vector2f boundedToLeft(const vector2f &p0, const vector2f &p1) const;
            vector2f boundedToRight(const vector2f &p0, const vector2f &p1) const;

            vector2f normalized(scalar ln = scalar(1.0)) const;
            vector2f inverted() const;
            vector2f rotated(scalar radians) const;
            
            vector2f rotatedAround(const vector2f &point) const;
            vector2f reflectedBy(const vector2f &nrm) const;
            vector2f projectedTo(const vector2f &v) const;
            vector2f directionTo(const vector2f &point) const;
            vector2f lerpTo(const vector2f &v, scalar koeff) const;

            vector2f nearestAxis() const;
            vector2f farthestAxis() const;
            vector2f orthogonalLeft() const;
            vector2f orthogonalRight() const;
            
            vector2f transformed(const transform2f &trfm, bool likePosition = false) const;

            // TODO: to all
            vector2f negatedX() const;
            
            bool isNaN() const {
                return std::isnan((*this)[Tx]) || std::isnan((*this)[Ty]);
            }

            bool isNearZero() const {
                return std::abs((*this)[Tx]) < std::numeric_limits<scalar>::epsilon() && std::abs((*this)[Ty]) < std::numeric_limits<scalar>::epsilon();
            }
            
            scalar &operator [](std::size_t index) {
                return *(reinterpret_cast<scalar *>(this) + index);
            }
            const scalar &operator [](std::size_t index) const {
                return *(reinterpret_cast<const scalar *>(this) + index);
            }
            
        protected:
            vector2base() = default;

        private:
            void _init(const scalar (&r)[2]) {
                *(reinterpret_cast<scalar *>(this) + Tx) = r[0];
                *(reinterpret_cast<scalar *>(this) + Ty) = r[1];
            }
        };

        //------------------------------------------------------------------------------------------------------------------------------------------------------
        // vector3base
        // TODO: create from polar coordinates
        // TODO: create from normal, azimuth and elevation
        
        template <std::size_t Tx, std::size_t Ty, std::size_t Tz> struct vector3base {
            vector3base &operator =(const vector3f &v);
            template <std::size_t X, std::size_t Y, std::size_t Z> vector3base &operator =(const vector3base<X, Y, Z> &v) {
                _init({v[X], v[Y], v[Z]});
                return *this;
            }
            vector3base &operator =(scalar s) {
                _init({s, s, s});
                return *this;
            }
            // TODO: same for vector2base
            vector3base &operator =(const scalar (&v)[3]) {
                _init(v);
                return *this;
            }
            operator vector3f();
            
            scalar distanceTo(const vector3f &v) const;
            scalar distanceSqTo(const vector3f &v) const;
            scalar dot(const vector3f &v) const;
            scalar angleTo(const vector3f &v) const;
            
            scalar length() const {
                return std::sqrt((*this)[Tx] * (*this)[Tx] + (*this)[Ty] * (*this)[Ty] + (*this)[Tz] * (*this)[Tz]);
            }

            scalar lengthSq() const {
                return (*this)[Tx] * (*this)[Tx] + (*this)[Ty] * (*this)[Ty] + (*this)[Tz] * (*this)[Tz];
            }

            scalar min() const {
                return std::min(std::min((*this)[Tx], (*this)[Ty]), (*this)[Tz]);
            }
            
            scalar max() const {
                return std::max(std::max((*this)[Tx], (*this)[Ty]), (*this)[Tz]);
            }

            vector3f cross(const vector3f &v) const;

            vector3f sign() const;
            vector3f clamped(scalar min, scalar max) const;
            vector3f bounded(const bound3f &b) const;

            vector3f normalized(scalar ln = scalar(1.0)) const;
            
            vector3f reflectedBy(const vector3f &nrm) const;
            vector3f projectedTo(const vector3f &v) const;
            vector3f directionTo(const vector3f &point) const;
            vector3f lerpTo(const vector3f &v, scalar koeff) const;

            vector3f nearestAxis() const;
            vector3f farthestAxis() const;
            vector3f randomOrthogonal() const;
            vector3f randomAberrant(scalar radians) const;
            
            vector3f rotated(const vector3f &axis, scalar radians) const;
            vector3f transformed(const transform3f &trfm, bool likePosition = false) const;
            vector3f transformed(const quaternion &q) const;

            // TODO: to all
            vector3f negatedX() const;
            
            quaternion rotationTo(const vector3f &v);

            bool isNaN() const {
                return std::isnan((*this)[Tx]) || std::isnan((*this)[Ty]) || std::isnan((*this)[Tz]);
            }

            bool isNearZero() const {
                return 
                    std::abs((*this)[Tx]) < std::numeric_limits<scalar>::epsilon() && 
                    std::abs((*this)[Ty]) < std::numeric_limits<scalar>::epsilon() &&
                    std::abs((*this)[Tz]) < std::numeric_limits<scalar>::epsilon();
            }
            
            scalar &operator [](std::size_t index) {
                return *(reinterpret_cast<scalar *>(this) + index);
            }
            const scalar &operator [](std::size_t index) const {
                return *(reinterpret_cast<const scalar *>(this) + index);
            }
            
        protected:
            vector3base() = default;

            void _init(const scalar (&r)[3]) {
                *(reinterpret_cast<scalar *>(this) + Tx) = r[0];
                *(reinterpret_cast<scalar *>(this) + Ty) = r[1];
                *(reinterpret_cast<scalar *>(this) + Tz) = r[2];
            }
        };
    }
    
    struct vector2f : imp::vector2base<0, 1> {
        union {
            struct {
                scalar x, y;
            };
            struct {
                scalar flat2[2];
            };
            struct : imp::vector2base<1, 0> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } yx;
        };

        static constexpr vector2f positiveX() {
            return {1, 0};
        }

        static constexpr vector2f negativeX() {
            return {-1, 0};
        }

        static constexpr vector2f positiveY() {
            return {0, 1};
        }

        static constexpr vector2f negativeY() {
            return {0, -1};
        }

        using vector2base::vector2base;
        using vector2base::operator =;

        vector2f() = default;
        explicit constexpr vector2f(scalar s) : x(s), y(s) {}
        constexpr vector2f(scalar x, scalar y) : x(x), y(y) {}
        
        template <std::size_t X, std::size_t Y> vector2f(const vector2base<X, Y> &v) {
            x = v[X];
            y = v[Y];
        }
    };
    
    struct vector3f : imp::vector3base<0, 1, 2> {
        union {
            struct {
                scalar x, y, z;
            };
            struct {
                scalar flat2[2];
            };
            struct {
                scalar flat3[3];
            };
            struct : imp::vector2base<0, 0> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } xx;
            struct : imp::vector2base<0, 1> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } xy;
            struct : imp::vector2base<0, 2> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } xz;
            struct : imp::vector2base<1, 0> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } yx;
            struct : imp::vector2base<1, 1> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } yy;
            struct : imp::vector2base<1, 2> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } yz;
            struct : imp::vector2base<2, 0> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } zx;
            struct : imp::vector2base<2, 1> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } zy;
            struct : imp::vector2base<2, 2> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } zz;
        };
        
        static constexpr vector3f positiveX() {
            return {1, 0, 0};
        }

        static constexpr vector3f negativeX() {
            return {-1, 0, 0};
        }

        static constexpr vector3f positiveY() {
            return {0, 1, 0};
        }

        static constexpr vector3f negativeY() {
            return {0, -1, 0};
        }
        
        static constexpr vector3f positiveZ() {
            return {0, 0, 1};
        }

        static constexpr vector3f negativeZ() {
            return {0, 0, -1};
        }
        
        using vector3base::vector3base;
        using vector3base::operator =;
        
        vector3f() = default;
        explicit constexpr vector3f(scalar s) : x(s), y(s), z(s) {}
        constexpr vector3f(scalar x, scalar y, scalar z) : x(x), y(y), z(z) {}
        constexpr vector3f(const scalar (&v)[3]) : x(v[0]), y(v[1]), z(v[2]) {}
        
        template <std::size_t X, std::size_t Y, std::size_t Z> vector3f(const imp::vector3base<X, Y, Z> &v) {
            _init({v[X], v[Y], v[Z]});
        }
        template <std::size_t X, std::size_t Y> vector3f(const imp::vector2base<X, Y> &v, scalar z) {
            _init({v[X], v[Y], z});
        }
        template <std::size_t X, std::size_t Y> vector3f(scalar x, const imp::vector2base<X, Y> &v) {
            _init({x, v[X], v[Y]});
        }
    };
    
    struct vector4f {
        union {
            struct {
                scalar x, y, z, w;
            };
            struct {
                scalar flat2[2];
            };
            struct {
                scalar flat3[3];
            };
            struct {
                scalar flat4[4];
            };
            struct : imp::vector2base<0, 0> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } xx;
            struct : imp::vector2base<0, 1> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } xy;
            struct : imp::vector2base<0, 2> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } xz;
            struct : imp::vector2base<0, 3> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } xw;
            struct : imp::vector2base<1, 0> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } yx;
            struct : imp::vector2base<1, 1> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } yy;
            struct : imp::vector2base<1, 2> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } yz;
            struct : imp::vector2base<1, 3> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } yw;
            struct : imp::vector2base<2, 0> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } zx;
            struct : imp::vector2base<2, 1> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } zy;
            struct : imp::vector2base<2, 2> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } zz;
            struct : imp::vector2base<2, 3> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } zw;
            struct : imp::vector2base<3, 0> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } wx;
            struct : imp::vector2base<3, 1> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } wy;
            struct : imp::vector2base<3, 2> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } wz;
            struct : imp::vector2base<3, 3> {
                using vector2base::vector2base;
                using vector2base::operator =;
            } ww;
            struct : imp::vector3base<0, 0, 0> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } xxx;
            struct : imp::vector3base<0, 0, 1> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } xxy;
            struct : imp::vector3base<0, 0, 2> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } xxz;
            struct : imp::vector3base<0, 0, 3> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } xxw;
            struct : imp::vector3base<0, 1, 0> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } xyx;
            struct : imp::vector3base<0, 1, 1> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } xyy;
            struct : imp::vector3base<0, 1, 2> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } xyz;
            struct : imp::vector3base<0, 1, 3> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } xyw;
            struct : imp::vector3base<0, 2, 0> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } xzx;
            struct : imp::vector3base<0, 2, 1> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } xzy;
            struct : imp::vector3base<0, 2, 2> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } xzz;
            struct : imp::vector3base<0, 2, 3> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } xzw;
            struct : imp::vector3base<0, 3, 0> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } xwx;
            struct : imp::vector3base<0, 3, 1> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } xwy;
            struct : imp::vector3base<0, 3, 2> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } xwz;
            struct : imp::vector3base<0, 3, 3> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } xww;
            struct : imp::vector3base<1, 0, 0> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } yxx;
            struct : imp::vector3base<1, 0, 1> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } yxy;
            struct : imp::vector3base<1, 0, 2> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } yxz;
            struct : imp::vector3base<1, 0, 3> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } yxw;
            struct : imp::vector3base<1, 1, 0> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } yyx;
            struct : imp::vector3base<1, 1, 1> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } yyy;
            struct : imp::vector3base<1, 1, 2> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } yyz;
            struct : imp::vector3base<1, 1, 3> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } yyw;
            struct : imp::vector3base<1, 2, 0> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } yzx;
            struct : imp::vector3base<1, 2, 1> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } yzy;
            struct : imp::vector3base<1, 2, 2> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } yzz;
            struct : imp::vector3base<1, 2, 3> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } yzw;
            struct : imp::vector3base<1, 3, 0> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } ywx;
            struct : imp::vector3base<1, 3, 1> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } ywy;
            struct : imp::vector3base<1, 3, 2> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } ywz;
            struct : imp::vector3base<1, 3, 3> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } yww;
            struct : imp::vector3base<2, 0, 0> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } zxx;
            struct : imp::vector3base<2, 0, 1> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } zxy;
            struct : imp::vector3base<2, 0, 2> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } zxz;
            struct : imp::vector3base<2, 0, 3> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } zxw;
            struct : imp::vector3base<2, 1, 0> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } zyx;
            struct : imp::vector3base<2, 1, 1> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } zyy;
            struct : imp::vector3base<2, 1, 2> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } zyz;
            struct : imp::vector3base<2, 1, 3> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } zyw;
            struct : imp::vector3base<2, 2, 0> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } zzx;
            struct : imp::vector3base<2, 2, 1> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } zzy;
            struct : imp::vector3base<2, 2, 2> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } zzz;
            struct : imp::vector3base<2, 2, 3> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } zzw;
            struct : imp::vector3base<2, 3, 0> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } zwx;
            struct : imp::vector3base<2, 3, 1> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } zwy;
            struct : imp::vector3base<2, 3, 2> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } zwz;
            struct : imp::vector3base<2, 3, 3> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } zww;
            struct : imp::vector3base<3, 0, 0> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } wxx;
            struct : imp::vector3base<3, 0, 1> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } wxy;
            struct : imp::vector3base<3, 0, 2> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } wxz;
            struct : imp::vector3base<3, 0, 3> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } wxw;
            struct : imp::vector3base<3, 1, 0> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } wyx;
            struct : imp::vector3base<3, 1, 1> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } wyy;
            struct : imp::vector3base<3, 1, 2> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } wyz;
            struct : imp::vector3base<3, 1, 3> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } wyw;
            struct : imp::vector3base<3, 2, 0> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } wzx;
            struct : imp::vector3base<3, 2, 1> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } wzy;
            struct : imp::vector3base<3, 2, 2> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } wzz;
            struct : imp::vector3base<3, 2, 3> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } wzw;
            struct : imp::vector3base<3, 3, 0> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } wwx;
            struct : imp::vector3base<3, 3, 1> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } wwy;
            struct : imp::vector3base<3, 3, 2> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } wwz;
            struct : imp::vector3base<3, 3, 3> {
                using vector3base::vector3base;
                using vector3base::operator =;
            } www;
        };
        
        vector4f() = default;
        explicit constexpr vector4f(scalar s) : x(s), y(s), z(s), w(s) {}
        constexpr vector4f(scalar x, scalar y, scalar z, scalar w) : x(x), y(y), z(z), w(w) {}
        constexpr vector4f(const vector4f &v) : x(v.x), y(v.y), z(v.z), w(v.w) {}
        constexpr vector4f(const scalar (&v)[4]) : x(v[0]), y(v[1]), z(v[2]), w(v[3]) {}
        
        template <std::size_t X, std::size_t Y, std::size_t Z> vector4f(const imp::vector3base<X, Y, Z> &v, scalar w) {
            _init({v[X], v[Y], v[Z], w});
        }
        template <std::size_t X, std::size_t Y, std::size_t Z> vector4f(scalar x, const imp::vector3base<X, Y, Z> &v) {
            _init({x, v[X], v[Y], v[Z]});
        }
        template <std::size_t X1, std::size_t Y1, std::size_t X2, std::size_t Y2> vector4f(const imp::vector2base<X1, Y1> &v1, const imp::vector2base<X2, Y2> &v2) {
            _init({v1[X1], v1[Y1], v2[X2], v2[Y2]});
        }
        template <std::size_t X, std::size_t Y> vector4f(const imp::vector2base<X, Y> &v, scalar z, scalar w) {
            _init({v[X], v[Y], z, w});
        }
        template <std::size_t X, std::size_t Y> vector4f(scalar x, const imp::vector2base<X, Y> &v, scalar w) {
            _init({x, v[X], v[Y], w});
        }
        template <std::size_t X, std::size_t Y> vector4f(scalar x, scalar y, const imp::vector2base<X, Y> &v) {
            _init({x, y, v[X], v[Y]});
        }

        vector4f &operator =(const scalar (&v)[4]) {
            _init(v);
            return *this;
        }
        vector4f &operator =(const vector4f &v) {
            _init(v.flat4);
            return *this;
        }
        vector4f &operator =(scalar s) {
            _init({s, s, s, s});
            return *this;
        }
        
        // TODO methods

        vector4f transformed(const transform3f &trfm) const;
        vector4f transformed(const quaternion &q) const;
    
        bool isNaN() const {
            return std::isnan(x) || std::isnan(y) || std::isnan(z) || std::isnan(w);
        }

        bool isNearZero() const {
            return
                std::abs(x) < std::numeric_limits<scalar>::epsilon() &&
                std::abs(y) < std::numeric_limits<scalar>::epsilon() &&
                std::abs(z) < std::numeric_limits<scalar>::epsilon() &&
                std::abs(w) < std::numeric_limits<scalar>::epsilon();
        }
    
        scalar &operator [](std::size_t index) {
            return *(reinterpret_cast<scalar *>(this) + index);
        }
        const scalar &operator [](std::size_t index) const {
            return *(reinterpret_cast<const scalar *>(this) + index);
        }

    private:
        void _init(const scalar (&r)[4]) {
            flat4[0] = r[0];
            flat4[1] = r[1];
            flat4[2] = r[2];
            flat4[3] = r[3];
        }
    };

    struct quaternion {
        scalar x, y, z, w;
        
        static constexpr quaternion identity() {
            return {0, 0, 0, 1};
        }
        
        quaternion() = default;
        quaternion(const quaternion &q) : x(q.x), y(q.y), z(q.z), w(q.w) {}
        quaternion(const transform3f &trfm);

        quaternion(const vector3f &axis, scalar radians) {
            scalar sina = std::sin(-radians * scalar(0.5));
            scalar cosa = std::cos(-radians * scalar(0.5));

            w = cosa;
            x = axis.x * sina;
            y = axis.y * sina;
            z = axis.z * sina;
        }
        
        constexpr quaternion(scalar qx, scalar qy, scalar qz, scalar qw) : x(qx), y(qy), z(qz), w(qw) {}

        quaternion &operator =(const quaternion &q) {
            x = q.x;
            y = q.y;
            z = q.z;
            w = q.w;
            return *this;
        }

        quaternion operator *(const quaternion &q) {
            return {
                q.y * z - q.z * y + q.w * x + q.x * w,
                q.z * x - q.x * z + q.w * y + q.y * w,
                q.x * y - q.y * x + q.w * z + q.z * w,
                q.w * w - q.x * x - q.y * y - q.z * z,
            };
        }
        
        // TODO other operators

        quaternion inverted() const {
            return {-x, -y, -z, w};
        }
        
        quaternion normalized() const {
            scalar lm = scalar(1.0) / std::sqrt(x * x + y * y + z * z + w * w);
            return {lm * x, lm * y, lm * z, lm * w};
        }

        quaternion slerpTo(const quaternion &q, scalar koeff) {
            quaternion tmpq = q;

            scalar cosT = x * tmpq.x + y * tmpq.y + z * tmpq.z + w * tmpq.w;
            scalar theta, sinT, k1, k2;

            if (cosT < scalar(0.0)) {
                tmpq = {-tmpq.x, -tmpq.y, -tmpq.z, -tmpq.w};
                cosT = -cosT;
            }
            
            cosT = std::min(cosT, scalar(1.0));
            theta = std::acosf(cosT);
            sinT = std::sqrt(scalar(1.0) - cosT * cosT);

            k1 = std::sin((scalar(1.0) - koeff) * theta) / sinT;
            k2 = std::sin(koeff * theta) / sinT;

            return {
                x * k1 + tmpq.x * k2,
                y * k1 + tmpq.y * k2,
                z * k1 + tmpq.z * k2,
                w * k1 + tmpq.w * k2,
            };
        }
        
        operator transform3f() const;
    };
    
    struct transform2f {
        union {
            struct {
                scalar _11, _12, _13;
                scalar _21, _22, _23;
                scalar _31, _32, _33;
            };
            struct {
                scalar flat9[9];
            };
            struct {
                vector3f &operator [](std::size_t index) {
                    return *(reinterpret_cast<vector3f *>(this) + index);
                }
                const vector3f &operator [](std::size_t index) const {
                    return *(reinterpret_cast<const vector3f *>(this) + index);
                }
            } rows;
        };
        
        static constexpr transform2f identity() {
            return {
                1, 0, 0,
                0, 1, 0,
                0, 0, 1,
            };
        }
        
        transform2f() = default;
        constexpr transform2f(
            scalar m11, scalar m12, scalar m13,
            scalar m21, scalar m22, scalar m23,
            scalar m31, scalar m32, scalar m33
        ) : _11(m11), _12(m12), _13(m13),
            _21(m21), _22(m22), _23(m23),
            _31(m31), _32(m32), _33(m33) {}
        
        transform2f(scalar rotationRadians) {
            _11 = _22 = std::cos(rotationRadians);
            _12 = std::sin(rotationRadians);
            _21 = -_12;
            _33 = scalar(1.0);
            _13 = _23 = 0.0f;
            _31 = _32 = 0.0f;
        }
        transform2f(const vector2f &translation) {
            *this = transform2f{1,0,0,0,1,0,0,0,1}.translated(translation);
        }
        transform2f(const vector2f &translation, scalar rotationRadians) {
            *this = transform2f(rotationRadians).translated(translation);
        }
        transform2f(const transform2f &trfm) {
            rows[0] = trfm.rows[0];
            rows[1] = trfm.rows[1];
            rows[2] = trfm.rows[2];
        }
        
        transform2f &operator =(const transform2f &trfm) {
            rows[0] = trfm.rows[0];
            rows[1] = trfm.rows[1];
            rows[2] = trfm.rows[2];
            return *this;
        }
        transform2f operator *(const transform2f &trfm) const {
            return {
                _11 * trfm._11 + _12 * trfm._21 + _13 * trfm._31,
                _11 * trfm._12 + _12 * trfm._22 + _13 * trfm._32,
                _11 * trfm._13 + _12 * trfm._23 + _13 * trfm._33,
                _21 * trfm._11 + _22 * trfm._21 + _23 * trfm._31,
                _21 * trfm._12 + _22 * trfm._22 + _23 * trfm._32,
                _21 * trfm._13 + _22 * trfm._23 + _23 * trfm._33,
                _31 * trfm._11 + _32 * trfm._21 + _33 * trfm._31,
                _31 * trfm._12 + _32 * trfm._22 + _33 * trfm._32,
                _31 * trfm._13 + _32 * trfm._23 + _33 * trfm._33,
            };
        }
        
        vector2f translation() const {
            return {_31, _32};
        }

        transform2f translated(const vector2f &v) const {
            return {
                _11, _12, _13,
                _21, _22, _23,
                _31 + v.x, _32 + v.y, _33,
            };
        }
        transform2f scaled(const vector2f &s) const {
            return {
                _11 * s.x, _12 * s.y, _13,
                _21 * s.x, _22 * s.y, _23,
                _31 * s.x, _32 * s.y, _33,
            };
        }
        
        transform2f rotated(scalar radians) const {
            scalar cosv = std::cos(radians);
            scalar sinv = std::sin(radians);
            
            return {
                _11 * cosv - _12 * sinv, _11 * sinv + _12 * cosv, _13,
                _21 * cosv - _22 * sinv, _21 * sinv + _22 * cosv, _23,
                _31 * cosv - _32 * sinv, _31 * sinv + _32 * cosv, _33,
            };
        }
        
        transform2f transposed() const {
            return {
                _11, _21, _31,
                _12, _22, _32,
                _13, _23, _33,
            };
        }
        
        transform2f inverted() const {
            scalar det = scalar(1.0) / (_11 * (_22 * _33 - _32 * _23) - _12 * (_21 * _33 - _31 * _23) + _13 * (_21 * _32 - _31 * _22));

            return {
                (_22 * _33 - _32 * _23) * det,
                -(_12 * _33 - _32 * _13) * det,
                (_12 * _23 - _22 * _13) * det,
                -(_21 * _33 - _31 * _23) * det,
                (_11 * _33 - _31 * _13) * det,
                -(_11 * _23 - _21 * _13) * det,
                (_21 * _32 - _31 * _22) * det,
                -(_11 * _32 - _31 * _12) * det,
                (_11 * _22 - _12 * _21) * det,
            };
        }
        
        const scalar (&operator [](std::size_t index) const)[3] {
            return reinterpret_cast<const scalar(&)[3]>(*(reinterpret_cast<const scalar *>(this) + 3 * index));
        }
    };
    
    struct transform3f {
        union {
            struct {
                scalar _11, _12, _13, _14;
                scalar _21, _22, _23, _24;
                scalar _31, _32, _33, _34;
                scalar _41, _42, _43, _44;
            };
            struct {
                scalar flat16[16];
            };
            struct {
                vector4f &operator [](std::size_t index) {
                    return *(reinterpret_cast<vector4f *>(this) + index);
                }
                const vector4f &operator [](std::size_t index) const {
                    return *(reinterpret_cast<const vector4f *>(this) + index);
                }
            } rows;
        };
        
        static constexpr transform3f identity() {
            return {
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1,
            };
        }
        
        transform3f() = default;
        constexpr transform3f(
            scalar m11, scalar m12, scalar m13, scalar m14,
            scalar m21, scalar m22, scalar m23, scalar m24,
            scalar m31, scalar m32, scalar m33, scalar m34,
            scalar m41, scalar m42, scalar m43, scalar m44
        ) : _11(m11), _12(m12), _13(m13), _14(m14),
            _21(m21), _22(m22), _23(m23), _24(m24),
            _31(m31), _32(m32), _33(m33), _34(m34),
            _41(m41), _42(m42), _43(m43), _44(m44) {}

        transform3f(const vector3f &translation) {
            *this = identity().translated(translation);
        }
        transform3f(const vector3f &translation, const quaternion &rotation) {
            *this = transform3f(rotation).translated(translation);
        }
        transform3f(const vector3f &axis, scalar radians) {
            *this = quaternion(axis, radians);
        }
        transform3f(const transform3f &trfm) {
            *this = trfm;
        }
        
        transform3f &operator =(const transform3f &trfm) {
            rows[0] = trfm.rows[0];
            rows[1] = trfm.rows[1];
            rows[2] = trfm.rows[2];
            rows[3] = trfm.rows[3];
            return *this;
        }
        transform3f operator *(const transform3f &trfm) const {
            return {
                _11 * trfm._11 + _12 * trfm._21 + _13 * trfm._31 + _14 * trfm._41,
                _11 * trfm._12 + _12 * trfm._22 + _13 * trfm._32 + _14 * trfm._42,
                _11 * trfm._13 + _12 * trfm._23 + _13 * trfm._33 + _14 * trfm._43,
                _11 * trfm._14 + _12 * trfm._24 + _13 * trfm._34 + _14 * trfm._44,
                _21 * trfm._11 + _22 * trfm._21 + _23 * trfm._31 + _24 * trfm._41,
                _21 * trfm._12 + _22 * trfm._22 + _23 * trfm._32 + _24 * trfm._42,
                _21 * trfm._13 + _22 * trfm._23 + _23 * trfm._33 + _24 * trfm._43,
                _21 * trfm._14 + _22 * trfm._24 + _23 * trfm._34 + _24 * trfm._44,
                _31 * trfm._11 + _32 * trfm._21 + _33 * trfm._31 + _34 * trfm._41,
                _31 * trfm._12 + _32 * trfm._22 + _33 * trfm._32 + _34 * trfm._42,
                _31 * trfm._13 + _32 * trfm._23 + _33 * trfm._33 + _34 * trfm._43,
                _31 * trfm._14 + _32 * trfm._24 + _33 * trfm._34 + _34 * trfm._44,
                _41 * trfm._11 + _42 * trfm._21 + _43 * trfm._31 + _44 * trfm._41,
                _41 * trfm._12 + _42 * trfm._22 + _43 * trfm._32 + _44 * trfm._42,
                _41 * trfm._13 + _42 * trfm._23 + _43 * trfm._33 + _44 * trfm._43,
                _41 * trfm._14 + _42 * trfm._24 + _43 * trfm._34 + _44 * trfm._44,
            };
        }
        
        vector3f translation() const {
            return {_41, _42, _43};
        }
        
        transform3f translated(const vector3f &v) const {
            return {
                _11, _12, _13, _14,
                _21, _22, _23, _24,
                _31, _32, _33, _34,
                _41 + v.x, _42 + v.y, _43 + v.z, _44,
            };
        }
        transform3f scaled(const vector3f &s) const {
            return {
                _11 * s.x, _12 * s.y, _13 * s.z, _14,
                _21 * s.x, _22 * s.y, _23 * s.z, _24,
                _31 * s.x, _32 * s.y, _33 * s.z, _34,
                _41 * s.x, _42 * s.y, _43 * s.z, _44,
            };
        }
        transform3f rotated(const vector3f &axis, scalar radians) const {
            return (*this) * transform3f(axis, radians);
        }
        transform3f rotated(const quaternion &q) const {
            return (*this) * transform3f(q);
        }
        transform3f transposed() const {
            return {
                _11, _21, _31, _41,
                _12, _22, _32, _42,
                _13, _23, _33, _43,
                _14, _24, _34, _44,
            };
        }
        transform3f inverted() const {
            scalar d12 = ((*this)[0][2] * (*this)[1][3] - (*this)[0][3] * (*this)[1][2]);
            scalar d13 = ((*this)[0][2] * (*this)[2][3] - (*this)[0][3] * (*this)[2][2]);
            scalar d23 = ((*this)[1][2] * (*this)[2][3] - (*this)[1][3] * (*this)[2][2]);
            scalar d24 = ((*this)[1][2] * (*this)[3][3] - (*this)[1][3] * (*this)[3][2]);
            scalar d34 = ((*this)[2][2] * (*this)[3][3] - (*this)[2][3] * (*this)[3][2]);
            scalar d41 = ((*this)[3][2] * (*this)[0][3] - (*this)[3][3] * (*this)[0][2]);
            scalar row0[4];
            scalar row1[4];

            row0[0] = ((*this)[1][1] * d34 - (*this)[2][1] * d24 + (*this)[3][1] * d23);
            row0[1] = -((*this)[0][1] * d34 + (*this)[2][1] * d41 + (*this)[3][1] * d13);
            row0[2] = ((*this)[0][1] * d24 + (*this)[1][1] * d41 + (*this)[3][1] * d12);
            row0[3] = -((*this)[0][1] * d23 - (*this)[1][1] * d13 + (*this)[2][1] * d12);

            scalar invd = scalar(1.0) / ((*this)[0][0] * row0[0] + (*this)[1][0] * row0[1] + (*this)[2][0] * row0[2] + (*this)[3][0] * row0[3]);

            row1[0] = -((*this)[1][0] * d34 - (*this)[2][0] * d24 + (*this)[3][0] * d23) * invd;
            row1[1] = ((*this)[0][0] * d34 + (*this)[2][0] * d41 + (*this)[3][0] * d13) * invd;
            row1[2] = -((*this)[0][0] * d24 + (*this)[1][0] * d41 + (*this)[3][0] * d12) * invd;
            row1[3] = ((*this)[0][0] * d23 - (*this)[1][0] * d13 + (*this)[2][0] * d12) * invd;

            d12 = (*this)[0][0] * (*this)[1][1] - (*this)[0][1] * (*this)[1][0];
            d13 = (*this)[0][0] * (*this)[2][1] - (*this)[0][1] * (*this)[2][0];
            d23 = (*this)[1][0] * (*this)[2][1] - (*this)[1][1] * (*this)[2][0];
            d24 = (*this)[1][0] * (*this)[3][1] - (*this)[1][1] * (*this)[3][0];
            d34 = (*this)[2][0] * (*this)[3][1] - (*this)[2][1] * (*this)[3][0];
            d41 = (*this)[3][0] * (*this)[0][1] - (*this)[3][1] * (*this)[0][0];

            return {
                row0[0] * invd, row0[1] * invd, row0[2] * invd, row0[3] * invd,
                row1[0], row1[1], row1[2], row1[3],

                ((*this)[1][3] * d34 - (*this)[2][3] * d24 + (*this)[3][3] * d23) * invd,
                -((*this)[0][3] * d34 + (*this)[2][3] * d41 + (*this)[3][3] * d13) * invd,
                ((*this)[0][3] * d24 + (*this)[1][3] * d41 + (*this)[3][3] * d12) * invd,
                -((*this)[0][3] * d23 - (*this)[1][3] * d13 + (*this)[2][3] * d12) * invd,
                
                -((*this)[1][2] * d34 - (*this)[2][2] * d24 + (*this)[3][2] * d23) * invd,
                ((*this)[0][2] * d34 + (*this)[2][2] * d41 + (*this)[3][2] * d13) * invd,
                -((*this)[0][2] * d24 + (*this)[1][2] * d41 + (*this)[3][2] * d12) * invd,
                ((*this)[0][2] * d23 - (*this)[1][2] * d13 + (*this)[2][2] * d12) * invd,
            };
        }

        // TODO: same methods for all
        transform3f withoutTranslation() const {
            return {
                _11, _12, _13, _14,
                _21, _22, _23, _24,
                _31, _32, _33, _34,
                0, 0, 0, _44,
            };
        }
        
        static transform3f lookAtLH(const vector3f &eye, const vector3f &at, const vector3f &up) {
            vector3f tmpz = vector3f(at.x - eye.x, at.y - eye.y, at.z - eye.z).normalized();
            vector3f tmpx = up.cross(tmpz);
            vector3f tmpy = tmpz.cross(tmpx);
            
            return {
                tmpx.x, tmpy.x, tmpz.x, 0.0f,
                tmpx.y, tmpy.y, tmpz.y, 0.0f,
                tmpx.z, tmpy.z, tmpz.z, 0.0f,
                -tmpx.dot(eye), -tmpy.dot(eye), -tmpz.dot(eye), 1.0f,
            };
        }

        static transform3f lookAtRH(const vector3f &eye, const vector3f &at, const vector3f &up) {
            vector3f tmpz = vector3f(eye.x - at.x, eye.y - at.y, eye.z - at.z).normalized();
            vector3f tmpx = up.cross(tmpz);
            vector3f tmpy = tmpz.cross(tmpx);
            
            return {
                tmpx.x, tmpy.x, tmpz.x, 0.0f,
                tmpx.y, tmpy.y, tmpz.y, 0.0f,
                tmpx.z, tmpy.z, tmpz.z, 0.0f,
                -tmpx.dot(eye), -tmpy.dot(eye), -tmpz.dot(eye), 1.0f,
            };
        }

        static transform3f perspectiveFovRH(scalar fovY, scalar aspect, scalar zNear, scalar zFar) {
            scalar yS = scalar(1.0f) / std::tan(fovY * scalar(0.5));
            scalar xS = yS / aspect;
            
            return {
                xS, 0, 0, 0,
                0, yS, 0, 0,
                0, 0, zNear / (zFar - zNear), -scalar(1.0),
                0, 0, (zFar * zNear) / (zFar - zNear), 0,
            };
        }

        static transform3f perspectiveFovLH(scalar fovY, scalar aspect, scalar zNear, scalar zFar) {
            scalar yS = scalar(1.0f) / std::tan(fovY * scalar(0.5));
            scalar xS = yS / aspect;
            
            return {
                xS, 0, 0, 0,
                0, yS, 0, 0,
                0, 0, zNear / (zNear - zFar), scalar(1.0),
                0, 0, -(zFar * zNear) / (zNear - zFar), 0,
            };
        }

        const scalar (&operator [](std::size_t index) const)[4] {
            return reinterpret_cast<const scalar(&)[4]>(*(reinterpret_cast<const scalar *>(this) + 4 * index));
        }
    };
    
    struct bound2f {
        union {
            struct {
                scalar xmin;
                scalar ymin;
                scalar xmax;
                scalar ymax;
            };
        };
    };
    
    struct bound3f {
        union {
            struct {
                scalar xmin;
                scalar ymin;
                scalar zmin;
                scalar xmax;
                scalar ymax;
                scalar zmax;
            };
        };
    };
    
    struct color {
        float r, g, b, a;
        
        color() = default;
        constexpr color(unsigned rgba) : r(float(rgba & 255) / 255.0f), g(float((rgba >> 8) & 255) / 255.0f), b(float((rgba >> 16) & 255) / 255.0f), a(float((rgba >> 24) & 255) / 255.0f) {}
        constexpr color(scalar r, scalar g, scalar b, scalar a) : r(r), g(g), b(b), a(a) {}
        
        static constexpr color identity() {
            return {1, 1, 1, 1};
        }
        
        operator unsigned() const {
            return (unsigned(r * 255.0f) & 255) << 0 |
                   (unsigned(g * 255.0f) & 255) << 8 |
                   (unsigned(b * 255.0f) & 255) << 16 |
                   (unsigned(a * 255.0f) & 255) << 24;
        }
        
        operator vector4f() const {
            return {r, g, b, a};
        }
    };

    namespace imp {
        //----------------------------------------------------------------------------------------------------------------------------------------------------------
        // vector2f methods

        template <std::size_t Tx, std::size_t Ty> inline vector2base<Tx, Ty> &vector2base<Tx, Ty>::operator =(const vector2f &v) {
            (*this)[Tx] = v.x;
            (*this)[Ty] = v.y;
            return *this;
        }

        template <std::size_t X, std::size_t Y> inline vector2base<X, Y>::operator vector2f() {
            return {(*this)[X], (*this)[Y]};
        }

        template <std::size_t X, std::size_t Y> inline vector2f operator +(const vector2base<X, Y> &v, scalar s) {
            return {v[X] + s, v[Y] + s};
        }
        template <std::size_t X, std::size_t Y> inline vector2f operator +(scalar s, const vector2base<X, Y> &v) {
            return {v[X] + s, v[Y] + s};
        }
        template <std::size_t X, std::size_t Y> inline vector2f operator -(const vector2base<X, Y> &v, scalar s) {
            return {v[X] - s, v[Y] - s};
        }
        template <std::size_t X, std::size_t Y> inline vector2f operator -(scalar s, const vector2base<X, Y> &v) {
            return {s - v[X], s - v[Y]};
        }
        template <std::size_t X, std::size_t Y> inline vector2f operator *(const vector2base<X, Y> &v, scalar s) {
            return {v[X] * s, v[Y] * s};
        }
        template <std::size_t X, std::size_t Y> inline vector2f operator *(scalar s, const vector2base<X, Y> &v) {
            return {v[X] * s, v[Y] * s};
        }
        template <std::size_t X, std::size_t Y> inline vector2f operator /(const vector2base<X, Y> &v, scalar s) {
            return {v[X] / s, v[Y] / s};
        }
        template <std::size_t X, std::size_t Y> inline vector2f operator /(scalar s, const vector2base<X, Y> &v) {
            return {s / v[X], s / v[Y]};
        }

        template <std::size_t X0, std::size_t Y0, std::size_t X1, std::size_t Y1>
        inline vector2f operator +(const vector2base<X0, Y0> &v0, const vector2base<X1, Y1> &v1) {
            return {v0[X0] + v1[X1], v0[Y0] + v1[Y1]};
        }

        template <std::size_t X0, std::size_t Y0, std::size_t X1, std::size_t Y1>
        inline vector2f operator -(const vector2base<X0, Y0> &v0, const vector2base<X1, Y1> &v1) {
            return {v0[X0] - v1[X1], v0[Y0] - v1[Y1]};
        }

        template <std::size_t X0, std::size_t Y0, std::size_t X1, std::size_t Y1>
        inline vector2f operator *(const vector2base<X0, Y0> &v0, const vector2base<X1, Y1> &v1) {
            return {v0[X0] * v1[X1], v0[Y0] * v1[Y1]};
        }
        
        template <std::size_t X0, std::size_t Y0, std::size_t X1, std::size_t Y1>
        inline vector2f operator /(const vector2base<X0, Y0> &v0, const vector2base<X1, Y1> &v1) {
            return {v0[X0] / v1[X1], v0[Y0] / v1[Y1]};
        }
        
        template <std::size_t X, std::size_t Y> inline vector2f operator -(const vector2base<X, Y> &v) {
            return {-v[X], -v[Y]};
        }
        
        template <std::size_t Tx, std::size_t Ty> inline scalar vector2base<Tx, Ty>::distanceTo(const vector2f &v) const {
            scalar dx = v.x - (*this)[Tx];
            scalar dy = v.y - (*this)[Ty];
            return std::sqrt(dx * dx + dy * dy);
        }

        template <std::size_t Tx, std::size_t Ty> inline scalar vector2base<Tx, Ty>::distanceSqTo(const vector2f &v) const {
            scalar dx = v.x - (*this)[Tx];
            scalar dy = v.y - (*this)[Ty];
            return dx * dx + dy * dy;
        }
    
        template <std::size_t Tx, std::size_t Ty> inline scalar vector2base<Tx, Ty>::cross(const vector2f &v) const {
            return (*this)[Tx] * v.y - (*this)[Ty] * v.x;
        }
    
        template <std::size_t Tx, std::size_t Ty> inline scalar vector2base<Tx, Ty>::dot(const vector2f &v) const {
            return v.x * (*this)[Tx] + v.y * (*this)[Ty];
        }
    
        template <std::size_t Tx, std::size_t Ty> inline scalar vector2base<Tx, Ty>::angleTo(const vector2f &v) const {
            return std::acos(std::min(scalar(1.0), std::max(scalar(-1.0), dot(v) / sqrtf(lengthSq() * v.lengthSq()))));
        }
        
        template <std::size_t Tx, std::size_t Ty> inline vector2f vector2base<Tx, Ty>::sign() const {
            return {std::copysign(scalar(1.0), (*this)[Tx]), std::copysign(scalar(1.0), (*this)[Ty])};
        }

        template <std::size_t Tx, std::size_t Ty> inline vector2f vector2base<Tx, Ty>::clamped(scalar min, scalar max) const {
            scalar cx = std::max(std::min((*this)[Tx], max), min);
            scalar cy = std::max(std::min((*this)[Ty], max), min);
            return {cx, cy};
        }

        template <std::size_t Tx, std::size_t Ty> inline vector2f vector2base<Tx, Ty>::bounded(const bound2f &b) const {
            scalar cx = std::max(std::min((*this)[Tx], b.xmax), b.xmin);
            scalar cy = std::max(std::min((*this)[Ty], b.ymax), b.ymin);
            return {cx, cy};
        }

        template <std::size_t Tx, std::size_t Ty> inline vector2f vector2base<Tx, Ty>::boundedToLeft(const vector2f &p0, const vector2f &p1) const {
            // TODO: implement
        }

        template <std::size_t Tx, std::size_t Ty> inline vector2f vector2base<Tx, Ty>::boundedToRight(const vector2f &p0, const vector2f &p1) const {
            // TODO: implement
        }

        template <std::size_t Tx, std::size_t Ty> inline vector2f vector2base<Tx, Ty>::normalized(scalar ln) const {
            vector2f result = *this;
            scalar lm = length();

            if (lm > std::numeric_limits<scalar>::epsilon()) {
                lm = ln / lm;
                result = result * lm;
            }

            return result;
        }

        template <std::size_t Tx, std::size_t Ty> inline vector2f vector2base<Tx, Ty>::inverted() const {
            return scalar(1.0) / (*this);
        }

        template <std::size_t Tx, std::size_t Ty> inline vector2f vector2base<Tx, Ty>::rotated(scalar radians) const {
            scalar rx = (*this)[Tx] * std::cos(radians) - (*this)[Ty] * std::sin(radians);
            scalar ry = (*this)[Tx] * std::sin(radians) + (*this)[Ty] * std::cos(radians);
            return {rx, ry};
        }

        template <std::size_t Tx, std::size_t Ty> inline vector2f vector2base<Tx, Ty>::rotatedAround(const vector2f &point) const {
            // TODO: implement
        }

        template <std::size_t Tx, std::size_t Ty> inline vector2f vector2base<Tx, Ty>::reflectedBy(const vector2f &nrm) const {
            // TODO: implement
        }

        template <std::size_t Tx, std::size_t Ty> inline vector2f vector2base<Tx, Ty>::projectedTo(const vector2f &v) const {
            // TODO: implement
        }

        template <std::size_t Tx, std::size_t Ty> inline vector2f vector2base<Tx, Ty>::directionTo(const vector2f &point) const {
            return (point - (*this)).normalized();
        }

        template <std::size_t Tx, std::size_t Ty> inline vector2f vector2base<Tx, Ty>::lerpTo(const vector2f &v, scalar koeff) const {
            return {(*this)[Tx] + (v.x - (*this)[Tx]) * koeff, (*this)[Ty] + (v.y - (*this)[Ty]) * koeff};
        }

        template <std::size_t Tx, std::size_t Ty> inline vector2f vector2base<Tx, Ty>::nearestAxis() const {
            vector2f result {0, 0};
            
            if (std::abs((*this)[Tx]) >= std::abs((*this)[Ty])) {
                result.x = std::copysign(scalar(1.0), (*this)[Tx]);
            }
            else {
                result.y = std::copysign(scalar(1.0), (*this)[Ty]);
            }
            
            return result;
        }

        template <std::size_t Tx, std::size_t Ty> inline vector2f vector2base<Tx, Ty>::farthestAxis() const {
            vector2f result {0, 0};
            
            if (std::abs((*this)[Tx]) <= std::abs((*this)[Ty])) {
                result.x = std::copysign(scalar(1.0), (*this)[Tx]);
            }
            else {
                result.y = std::copysign(scalar(1.0), (*this)[Ty]);
            }
            
            return result;
        }

        template <std::size_t Tx, std::size_t Ty> inline vector2f vector2base<Tx, Ty>::orthogonalLeft() const {
            return -orthogonalRight();
        }

        template <std::size_t Tx, std::size_t Ty> inline vector2f vector2base<Tx, Ty>::orthogonalRight() const {
            return {-(*this)[Ty], (*this)[Tx]};
        }
        
        template <std::size_t Tx, std::size_t Ty> inline vector2f vector2base<Tx, Ty>::transformed(const transform2f &trfm, bool likePosition) const {
            float w = likePosition ? 1.0f : 0.0f;
            return {
                (*this)[Tx] * trfm._11 + (*this)[Ty] * trfm._21 + w * trfm._31,
                (*this)[Tx] * trfm._12 + (*this)[Ty] * trfm._22 + w * trfm._32,
            };
        }

        template <std::size_t Tx, std::size_t Ty>
        inline vector2f vector2base<Tx, Ty>::negatedX() const {
            return {-(*this)[Tx], (*this)[Ty]};
        }
        
        //----------------------------------------------------------------------------------------------------------------------------------------------------------
        // vector3f methods

        template <std::size_t Tx, std::size_t Ty, std::size_t Tz> inline vector3base<Tx, Ty, Tz> &vector3base<Tx, Ty, Tz>::operator =(const vector3f &v) {
            (*this)[Tx] = v.x;
            (*this)[Ty] = v.y;
            (*this)[Tz] = v.z;
        }

        template <std::size_t X, std::size_t Y, std::size_t Z> inline vector3base<X, Y, Z>::operator vector3f() {
            return {(*this)[X], (*this)[Y], (*this)[Z]};
        }

        template <std::size_t X, std::size_t Y, std::size_t Z> inline vector3f operator +(const vector3base<X, Y, Z> &v, scalar s) {
            return {v[X] + s, v[Y] + s, v[Z] + s};
        }
        template <std::size_t X, std::size_t Y, std::size_t Z> inline vector3f operator +(scalar s, const vector3base<X, Y, Z> &v) {
            return {v[X] + s, v[Y] + s, v[Z] + s};
        }
        template <std::size_t X, std::size_t Y, std::size_t Z> inline vector3f operator -(const vector3base<X, Y, Z> &v, scalar s) {
            return {v[X] - s, v[Y] - s, v[Z] - s};
        }
        template <std::size_t X, std::size_t Y, std::size_t Z> inline vector3f operator -(scalar s, const vector3base<X, Y, Z> &v) {
            return {s - v[X], s - v[Y], s - v[Z]};
        }
        template <std::size_t X, std::size_t Y, std::size_t Z> inline vector3f operator *(const vector3base<X, Y, Z> &v, scalar s) {
            return {v[X] * s, v[Y] * s, v[Z] * s};
        }
        template <std::size_t X, std::size_t Y, std::size_t Z> inline vector3f operator *(scalar s, const vector3base<X, Y, Z> &v) {
            return {v[X] * s, v[Y] * s, v[Z] * s};
        }
        template <std::size_t X, std::size_t Y, std::size_t Z> inline vector3f operator /(const vector3base<X, Y, Z> &v, scalar s) {
            return {v[X] / s, v[Y] / s, v[Z] / s};
        }
        template <std::size_t X, std::size_t Y, std::size_t Z> inline vector3f operator /(scalar s, const vector3base<X, Y, Z> &v) {
            return {s / v[X], s / v[Y], s / v[Z]};
        }
        
        template <std::size_t X0, std::size_t Y0, std::size_t Z0, std::size_t X1, std::size_t Y1, std::size_t Z1>
        inline vector3f operator +(const vector3base<X0, Y0, Z0> &v0, const vector3base<X1, Y1, Z1> &v1) {
            return {v0[X0] + v1[X1], v0[Y0] + v1[Y1], v0[Z0] + v1[Z1]};
        }

        template <std::size_t X0, std::size_t Y0, std::size_t Z0, std::size_t X1, std::size_t Y1, std::size_t Z1>
        inline vector3f operator -(const vector3base<X0, Y0, Z0> &v0, const vector3base<X1, Y1, Z1> &v1) {
            return {v0[X0] - v1[X1], v0[Y0] - v1[Y1], v0[Z0] - v1[Z1]};
        }
        
        template <std::size_t X0, std::size_t Y0, std::size_t Z0, std::size_t X1, std::size_t Y1, std::size_t Z1>
        inline vector3f operator *(const vector3base<X0, Y0, Z0> &v0, const vector3base<X1, Y1, Z1> &v1) {
            return {v0[X0] * v1[X1], v0[Y0] * v1[Y1], v0[Z0] * v1[Z1]};
        }
        
        template <std::size_t X0, std::size_t Y0, std::size_t Z0, std::size_t X1, std::size_t Y1, std::size_t Z1>
        inline vector3f operator /(const vector3base<X0, Y0, Z0> &v0, const vector3base<X1, Y1, Z1> &v1) {
            return {v0[X0] / v1[X1], v0[Y0] / v1[Y1], v0[Z0] / v1[Z1]};
        }

        template <std::size_t Tx, std::size_t Ty, std::size_t Tz> inline scalar vector3base<Tx, Ty, Tz>::distanceTo(const vector3f &v) const {
            scalar dx = v.x - (*this)[Tx];
            scalar dy = v.y - (*this)[Ty];
            scalar dz = v.z - (*this)[Tz];
            return std::sqrt(dx * dx + dy * dy + dz * dz);
        }

        template <std::size_t Tx, std::size_t Ty, std::size_t Tz> inline scalar vector3base<Tx, Ty, Tz>::distanceSqTo(const vector3f &v) const {
            scalar dx = v.x - (*this)[Tx];
            scalar dy = v.y - (*this)[Ty];
            scalar dz = v.z - (*this)[Tz];
            return dx * dx + dy * dy + dz * dz;
        }
    
        template <std::size_t Tx, std::size_t Ty, std::size_t Tz> inline scalar vector3base<Tx, Ty, Tz>::dot(const vector3f &v) const {
            return v.x * (*this)[Tx] + v.y * (*this)[Ty] + v.z * (*this)[Tz];
        }
    
        template <std::size_t Tx, std::size_t Ty, std::size_t Tz> inline scalar vector3base<Tx, Ty, Tz>::angleTo(const vector3f &v) const {
            return std::acos(std::min(scalar(1.0), std::max(scalar(-1.0), dot(v) / sqrtf(lengthSq() * v.lengthSq()))));
        }

        template <std::size_t Tx, std::size_t Ty, std::size_t Tz> inline vector3f vector3base<Tx, Ty, Tz>::cross(const vector3f &v) const {
            return vector3f {
                (*this)[Ty] * v.z - (*this)[Tz] * v.y,
                (*this)[Tz] * v.x - (*this)[Tx] * v.z,
                (*this)[Tx] * v.y - (*this)[Ty] * v.x,
            }.normalized();
        }
        
        template <std::size_t Tx, std::size_t Ty, std::size_t Tz> inline vector3f vector3base<Tx, Ty, Tz>::sign() const {
            return {std::copysign(scalar(1.0), (*this)[Tx]), std::copysign(scalar(1.0), (*this)[Ty]), std::copysign(scalar(1.0), (*this)[Tz])};
        }

        template <std::size_t Tx, std::size_t Ty, std::size_t Tz> inline vector3f vector3base<Tx, Ty, Tz>::clamped(scalar min, scalar max) const {
            scalar cx = std::max(std::min((*this)[Tx], max), min);
            scalar cy = std::max(std::min((*this)[Ty], max), min);
            scalar cz = std::max(std::min((*this)[Tz], max), min);
            return {cx, cy, cz};
        }
        
        template <std::size_t Tx, std::size_t Ty, std::size_t Tz> inline vector3f vector3base<Tx, Ty, Tz>::bounded(const bound3f &b) const {
            scalar cx = std::max(std::min((*this)[Tx], b.xmax), b.xmin);
            scalar cy = std::max(std::min((*this)[Ty], b.ymax), b.ymin);
            scalar cz = std::max(std::min((*this)[Tz], b.zmax), b.zmin);
            return {cx, cy, cz};
        }

        template <std::size_t Tx, std::size_t Ty, std::size_t Tz> inline vector3f vector3base<Tx, Ty, Tz>::normalized(scalar ln) const {
            vector3f result = *this;
            scalar lm = length();

            if (lm > std::numeric_limits<scalar>::epsilon()) {
                lm = ln / lm;
                result = result * lm;
            }

            return result;
        }
    
        template <std::size_t Tx, std::size_t Ty, std::size_t Tz> inline vector3f vector3base<Tx, Ty, Tz>::reflectedBy(const vector3f &nrm) const {
            // TODO: implement
        }

        template <std::size_t Tx, std::size_t Ty, std::size_t Tz> inline vector3f vector3base<Tx, Ty, Tz>::projectedTo(const vector3f &v) const {
            // TODO: implement
        }

        template <std::size_t Tx, std::size_t Ty, std::size_t Tz> inline vector3f vector3base<Tx, Ty, Tz>::directionTo(const vector3f &point) const {
            return (point - (*this)).normalized();
        }

        template <std::size_t Tx, std::size_t Ty, std::size_t Tz> inline vector3f vector3base<Tx, Ty, Tz>::lerpTo(const vector3f &v, scalar koeff) const {
            return {
                (*this)[Tx] + (v.x - (*this)[Tx]) * koeff,
                (*this)[Ty] + (v.y - (*this)[Ty]) * koeff,
                (*this)[Tz] + (v.z - (*this)[Tz]) * koeff,
            };
        }

        template <std::size_t Tx, std::size_t Ty, std::size_t Tz> inline vector3f vector3base<Tx, Ty, Tz>::nearestAxis() const {
            vector3f result {0, 0, 0};
            
            if (std::abs((*this)[Tx]) >= std::abs((*this)[Ty])) {
                if (std::abs((*this)[Tz]) >= std::abs((*this)[Tx])) {
                    result.z = std::copysign(scalar(1.0), (*this)[Tz]);
                }
                else {
                    result.x = std::copysign(scalar(1.0), (*this)[Tx]);
                }
            }
            else {
                if (std::abs((*this)[Tz]) >= std::abs((*this)[Ty])) {
                    result.z = std::copysign(scalar(1.0), (*this)[Tz]);
                }
                else {
                    result.y = std::copysign(scalar(1.0), (*this)[Ty]);
                }
            }
            
            return result;
        }
        
        template <std::size_t Tx, std::size_t Ty, std::size_t Tz> inline vector3f vector3base<Tx, Ty, Tz>::farthestAxis() const {
            vector3f result {0, 0, 0};
            
            if (std::abs((*this)[Tx]) <= std::abs((*this)[Ty])) {
                if (std::abs((*this)[Tz]) <= std::abs((*this)[Tx])) {
                    result.z = std::copysign(scalar(1.0), (*this)[Tz]);
                }
                else {
                    result.x = std::copysign(scalar(1.0), (*this)[Tx]);
                }
            }
            else {
                if (std::abs((*this)[Tz]) <= std::abs((*this)[Ty])) {
                    result.z = std::copysign(scalar(1.0), (*this)[Tz]);
                }
                else {
                    result.y = std::copysign(scalar(1.0), (*this)[Ty]);
                }
            }
            
            return result;
        }
        
        template <std::size_t Tx, std::size_t Ty, std::size_t Tz> inline vector3f vector3base<Tx, Ty, Tz>::randomOrthogonal() const {
            // TODO: implement
        }
        
        template <std::size_t Tx, std::size_t Ty, std::size_t Tz> inline vector3f vector3base<Tx, Ty, Tz>::randomAberrant(scalar radians) const {
            // TODO: implement
        }
    
        template <std::size_t Tx, std::size_t Ty, std::size_t Tz>
        inline vector3f vector3base<Tx, Ty, Tz>::rotated(const vector3f &axis, scalar radians) const {
            quaternion rotation(axis, radians);
            return this->transformed(rotation);
        }

        template <std::size_t Tx, std::size_t Ty, std::size_t Tz>
        inline vector3f vector3base<Tx, Ty, Tz>::transformed(const transform3f &trfm, bool likePosition) const {
            scalar w = likePosition ? scalar(1.0) : scalar(0.0);
            scalar tx = (*this)[Tx];
            scalar ty = (*this)[Ty];
            scalar tz = (*this)[Tz];

            return {
                tx * trfm._11 + ty * trfm._21 + tz * trfm._31 + w * trfm._41,
                tx * trfm._12 + ty * trfm._22 + tz * trfm._32 + w * trfm._42,
                tx * trfm._13 + ty * trfm._23 + tz * trfm._33 + w * trfm._43
            };
        }
        
        template <std::size_t Tx, std::size_t Ty, std::size_t Tz>
        inline vector3f vector3base<Tx, Ty, Tz>::transformed(const quaternion &q) const {
            quaternion p ((*this)[Tx], (*this)[Ty], (*this)[Tz], 0.0f);
            quaternion invq (-q.x, -q.y, -q.z, q.w);

            p = invq * p * q;
            return {p.x, p.y, p.z};
        }

        template <std::size_t Tx, std::size_t Ty, std::size_t Tz>
        inline vector3f vector3base<Tx, Ty, Tz>::negatedX() const {
            return {-(*this)[Tx], (*this)[Ty], (*this)[Tz]};
        }
    
        template <std::size_t Tx, std::size_t Ty, std::size_t Tz> inline quaternion vector3base<Tx, Ty, Tz>::rotationTo(const vector3f &v) {
            // TODO: implement
        }
    }
    
    //----------------------------------------------------------------------------------------------------------------------------------------------------------
    // vector4f methods

    inline vector4f operator +(const vector4f &v, scalar s) {
        return {v.x + s, v.y + s, v.z + s, v.w + s};
    }
    inline vector4f operator +(scalar s, const vector4f &v) {
        return {v.x + s, v.y + s, v.z + s, v.w + s};
    }
    inline vector4f operator -(const vector4f &v, scalar s) {
        return {v.x - s, v.y - s, v.z - s, v.w - s};
    }
    inline vector4f operator -(scalar s, const vector4f &v) {
        return {s - v.x, s - v.y, s - v.z, s - v.w};
    }
    inline vector4f operator *(const vector4f &v, scalar s) {
        return {v.x * s, v.y * s, v.z * s, v.w * s};
    }
    inline vector4f operator *(scalar s, const vector4f &v) {
        return {v.x * s, v.y * s, v.z * s, v.w * s};
    }
    inline vector4f operator /(const vector4f &v, scalar s) {
        return {v.x / s, v.y / s, v.z / s, v.w / s};
    }
    inline vector4f operator /(scalar s, const vector4f &v) {
        return {s / v.x, s / v.y, s / v.z, s / v.w};
    }

    inline vector4f operator +(const vector4f &v0, const vector4f &v1) {
        return {v0[0] + v1[0], v0[1] + v1[1], v0[2] + v1[2], v0[3] + v1[3]};
    }
    inline vector4f operator -(const vector4f &v0, const vector4f &v1) {
        return {v0[0] - v1[0], v0[1] - v1[1], v0[2] - v1[2], v0[3] - v1[3]};
    }
    inline vector4f operator *(const vector4f &v0, const vector4f &v1) {
        return {v0[0] * v1[0], v0[1] * v1[1], v0[2] * v1[2], v0[3] * v1[3]};
    }
    inline vector4f operator /(const vector4f &v0, const vector4f &v1) {
        return {v0[0] / v1[0], v0[1] / v1[1], v0[2] / v1[2], v0[3] / v1[3]};
    }
    
    //----------------------------------------------------------------------------------------------------------------------------------------------------------
    // quaternion methods
    
    inline quaternion operator -(const quaternion &q) {
        return q.inverted();
    }

    inline quaternion::quaternion(const transform3f &trfm) {
        scalar s;
        scalar q[4];
        scalar trace = trfm[0][0] + trfm[1][1] + trfm[2][2];

        int i, j, k;
        int nxt[3] = {1, 2, 0};

        if (trace > scalar(0.0)){
            s = std::sqrt(trace + scalar(1.0));
            w = s / scalar(2.0);
            s = scalar(0.5) / s;
            x = (trfm[1][2] - trfm[2][1]) * s;
            y = (trfm[2][0] - trfm[0][2]) * s;
            z = (trfm[0][1] - trfm[1][0]) * s;
        }
        else {
            i = 0;

            if (trfm[1][1] > trfm[0][0]) i = 1;
            if (trfm[2][2] > trfm[i][i]) i = 2;

            j = nxt[i];
            k = nxt[j];

            s = std::sqrt((trfm[i][i] - (trfm[j][j] + trfm[k][k])) + scalar(1.0));

            q[i] = s * scalar(0.5);

            if (s != scalar(0.0)) s = scalar(0.5) / s;

            q[3] = (trfm[j][k] - trfm[k][j]) * s;
            q[j] = (trfm[i][j] + trfm[j][i]) * s;
            q[k] = (trfm[i][k] + trfm[k][i]) * s;

            x = q[0];
            y = q[1];
            z = q[2];
            w = q[3];
        }
    }

    inline quaternion::operator transform3f() const {
        scalar xx = scalar(2.0) * x * x;
        scalar xy = scalar(2.0) * x * y;
        scalar xz = scalar(2.0) * x * z;
        scalar yy = scalar(2.0) * y * y;
        scalar yz = scalar(2.0) * y * z;
        scalar zz = scalar(2.0) * z * z;
        scalar wx = scalar(2.0) * w * x;
        scalar wy = scalar(2.0) * w * y;
        scalar wz = scalar(2.0) * w * z;

        return {
            scalar(1.0) - (yy + zz),
            xy + wz,
            xz - wy,
            scalar(0.0),

            xy - wz,
            scalar(1.0) - (xx + zz),
            yz + wx,
            scalar(0.0),

            xz + wy,
            yz - wx,
            scalar(1.0) - (xx + yy),
            scalar(0.0),

            scalar(0.0),
            scalar(0.0),
            scalar(0.0),
            scalar(1.0),
        };
    }
    
    constexpr scalar PI = scalar(3.141592653589793);
    constexpr scalar PI_2 = PI / scalar(2.0);
    constexpr scalar PI_4 = PI / scalar(4.0);
    constexpr scalar PI_6 = PI / scalar(6.0);

    //---

    static_assert(sizeof(vector2f) == 2 * sizeof(scalar),  "layout error");
    static_assert(sizeof(vector3f) == 3 * sizeof(scalar), "layout error");
    static_assert(sizeof(vector4f) == 4 * sizeof(scalar), "layout error");
    static_assert(sizeof(quaternion) == 4 * sizeof(scalar), "layout error");
    static_assert(sizeof(transform2f) == 9 * sizeof(scalar), "layout error");
    static_assert(sizeof(transform3f) == 16 * sizeof(scalar), "layout error");
}




