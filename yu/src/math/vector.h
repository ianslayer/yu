#ifndef YU_VECTOR_H
#define YU_VECTOR_H

#include "yu_math.h"

namespace yu
{
struct Vector2
{
	float operator []  (int i) const;	
	float& operator [](int i);

	template <class T>
	Vector2& operator=(const T& e);
	
	Vector2& operator+=(const Vector2& rhs);	
	Vector2& operator+=(float s);	
	Vector2& operator-=(const Vector2& rhs);	
	Vector2& operator-=(const float s);
	Vector2& operator*=(const Vector2& rhs);
	Vector2& operator*=(const float s);
	Vector2& operator/=(const Vector2& rhs);
	Vector2& operator/=(const float s);

	bool operator == (const Vector2& rhs) const;
	
	template<class T>
	void	Evaluate(const T& e);

	float	SquaredLength();
	float	Length();
	void	Normalize();	

	float	x;
	float	y;
};

YU_INLINE Vector2 _Vector2(float _x, float _y)
{
	Vector2 vec;
	vec.x = _x;
	vec.y = _y;
	return vec;
}

YU_INLINE float Vector2::operator []  (int i)const
{
    return (&x)[i];
}	

YU_INLINE float& Vector2::operator [](int i)
{
    return (&x)[i];
}

template<class T>
YU_INLINE Vector2& Vector2::operator = (const T& e)
{
	Evalute(e);
	return *this;
}

YU_INLINE Vector2& Vector2::operator+=(const Vector2& rhs)
{
	x += rhs.x;
	y += rhs.y;
	return *this;
}

YU_INLINE Vector2& Vector2::operator+=(float s)
{
	x += s;
	y += s;
	return *this;
}

YU_INLINE Vector2& Vector2::operator-=(const Vector2& rhs)
{
	x -= rhs.x;
	y -= rhs.y;
	return *this;
}

YU_INLINE Vector2& Vector2::operator-=(float s)
{
	x -= s;
	y -= s;
	return *this;
}

YU_INLINE Vector2& Vector2::operator*=(const Vector2& rhs)
{
	x *= rhs.x;
	y *= rhs.y;
	return *this;
}

YU_INLINE Vector2& Vector2::operator*=(float s)
{
	x *= s;
	y *= s;
	return *this;
}

YU_INLINE Vector2& Vector2::operator/=(const Vector2& rhs)
{
	x /= rhs.x;
	y /= rhs.y;
	return *this;
}

YU_INLINE Vector2& Vector2::operator/=(float s)
{
	x /= s;
	y /= s;
	return *this;
}

YU_INLINE Vector2 operator-(const Vector2& v)
{
    return _Vector2(-v.x, -v.y);
}

template<class T> 
YU_INLINE void Vector2::Evaluate(const T& e){
		x = e[0];
		y = e[1];		
}

YU_INLINE Vector2 Max(const Vector2& a, const Vector2& b)
{
	return _Vector2(yu::max(a.x, b.x), yu::max(a.y, b.y) );
}

YU_INLINE Vector2 Min(const Vector2& a, const Vector2& b)
{
	return _Vector2(yu::min(a.x, b.x), yu::min(a.y, b.y));
}

YU_INLINE const Vector2 operator + (const Vector2& a, const Vector2& b)
{
	return _Vector2(a.x + b.x, a.y + b.y);	
}

YU_INLINE const Vector2 operator + (float a, const Vector2& b)
{
	return _Vector2(a + b.x, a + b.y);	
}	

YU_INLINE const Vector2 operator + (Vector2& a, float b)
{
	return _Vector2(a.x + b, a.y + b);	
}	

YU_INLINE const Vector2 operator - (const Vector2& a, const Vector2& b)
{
	return _Vector2(a.x - b.x, a.y - b.y);	
}

YU_INLINE const Vector2 operator*(const Vector2& a, const Vector2& b)
{
	return _Vector2(a.x * b.x, a.y * b.y);
}

YU_INLINE const Vector2 operator*(float a, const Vector2& b)
{
	return _Vector2(a * b.x, a * b.y);
}

YU_INLINE const Vector2 operator*(const Vector2& a, float b)
{
	return _Vector2(a.x * b, a.y * b);
}		

YU_INLINE const Vector2 operator/(const Vector2& a, const Vector2& b)
{
	//assert(b.x != 0 && b.y != 0 && b.z != 0);
	return _Vector2(a.x / b.x, a.y / b.y);
}

YU_INLINE const Vector2 operator / (const Vector2& v, float s)
{
	//assert(s != 0);
	return _Vector2(v.x / s, v.y / s);
}

YU_INLINE bool Vector2::operator == (const Vector2& rhs) const
{
    return ( (x == rhs.x) && (y == rhs.y));
}

YU_INLINE float cross(const Vector2& v1, const Vector2& v2)
{
	return v1.x * v2.y - v1.y * v2.x;
}

struct Vector3 {
public:

	void Set(float x, float y, float z);
	void Zero(void);

	float operator []  (int i) const;	
	float& operator [](int i);

	Vector3& operator+=(const Vector3& rhs);	
	Vector3& operator+=(float s);	
	Vector3& operator-=(const Vector3& rhs);	
	Vector3& operator-=(const float s);
	Vector3& operator*=(const Vector3& rhs);
	Vector3& operator*=(const float s);
	Vector3& operator/=(const Vector3& rhs);
	Vector3& operator/=(const float s);

	bool operator == (const Vector3& rhs) const;
	
	float SquaredLength() const;
	float Length() const;
	void Normalize();	

	float x;
	float y;
	float z;
};

YU_INLINE Vector3 _Vector3(float* _pVec)
{
	Vector3 vec;
	vec.x = _pVec[0];
	vec.y = _pVec[1];
	vec.z = _pVec[2];
	return vec;
}

YU_INLINE Vector3 _Vector3(float _xyz)
{
	Vector3 vec;
	vec.x = vec.y = vec.z = _xyz;
	return vec;
}

YU_INLINE Vector3 _Vector3(float _x, float _y, float _z)
{
	Vector3 vec;
	vec.x = _x;
	vec.y = _y;
	vec.z = _z;
	return vec;
}

YU_INLINE void Vector3::Set(float _x, float _y, float _z)
{
	x = _x;
	y = _y;
	z = _z;
}

YU_INLINE void Vector3::Zero(void)
{
	x = y = z = 0;
}

YU_INLINE float Vector3::operator []  (int i)const 
{
	return (&x)[i];
}	

YU_INLINE float& Vector3::operator [](int i) 
{
	return (&x)[i];
}

YU_INLINE Vector3 operator-(const Vector3& v)
{
	return _Vector3(-v.x, -v.y, -v.z);
}

YU_INLINE Vector3& Vector3::operator+=(const Vector3& rhs)
{
	x += rhs.x;
	y += rhs.y;
	z += rhs.z;
	return *this;
}

YU_INLINE Vector3& Vector3::operator+=(float s)
{
	x += s;
	y += s;
	z += s;		
	return *this;
}

YU_INLINE Vector3& Vector3::operator-=(const Vector3& rhs)
{
	x -= rhs.x;
	y -= rhs.y;
	z -= rhs.z;
	return *this;
}

YU_INLINE Vector3& Vector3::operator-=(const float s)
{
	x -= s;
	y -= s;
	z -= s;		
	return *this;
}

YU_INLINE Vector3& Vector3::operator*=(const Vector3& rhs)
{
	x *= rhs.x;
	y *= rhs.y;
	z *= rhs.z;
	return *this;
}

YU_INLINE Vector3& Vector3::operator*=(const float s)
{
	x *= s;
	y *= s;
	z *= s;		
	return *this;
}	

YU_INLINE Vector3& Vector3::operator/=(const Vector3& rhs)
{
	//assert(rhs.x != 0 && rhs.y != 0 && rhs.z != 0);
	x /= rhs.x;
	y /= rhs.y;
	z /= rhs.z;
	return *this;
}

YU_INLINE Vector3& Vector3::operator/=(const float s)
{
	//assert(s != 0);
	float inv_s = 1.f / s;
	x *= inv_s;
	y *= inv_s;
	z *= inv_s;		
	return *this;
}	

YU_INLINE bool Vector3::operator == (const Vector3& rhs) const
{
	return ( (x == rhs.x) && (y == rhs.y) && (z == rhs.z));
}

YU_INLINE float Vector3::SquaredLength() const
{
	return x * x + y * y + z * z;
}

YU_INLINE float Vector3::Length() const
{
	return sqrtf(x * x + y * y + z * z);
}

YU_INLINE void Vector3::Normalize()
{
	float length = Length();
	x /= length;
	y /= length;
	z /= length;
}

YU_INLINE const Vector3 operator + (const Vector3& a, const Vector3& b)
{
	return _Vector3(a.x + b.x, a.y + b.y, a.z + b.z);	
}

YU_INLINE const Vector3 operator + (float a, const Vector3& b)
{
	return _Vector3(a + b.x, a + b.y, a + b.z);	
}	

YU_INLINE const Vector3 operator + (Vector3& a, float b)
{
	return _Vector3(a.x + b, a.y + b, a.z + b);	
}	

YU_INLINE const Vector3 operator - (const Vector3& a, const Vector3& b)
{
	return _Vector3(a.x - b.x, a.y - b.y, a.z - b.z);	
}

YU_INLINE const Vector3 operator*(const Vector3& a, const Vector3& b)
{
	return _Vector3(a.x * b.x, a.y * b.y, a.z * b.z);
}

YU_INLINE const Vector3 operator*(float a, const Vector3& b)
{
	return _Vector3(a * b.x, a * b.y, a * b.z);
}

YU_INLINE const Vector3 operator*(const Vector3& a, float b)
{
	return _Vector3(a.x * b, a.y * b, a.z * b);
}		

YU_INLINE const Vector3 operator/(const Vector3& a, const Vector3& b)
{
	//assert(b.x != 0 && b.y != 0 && b.z != 0);
	return _Vector3(a.x / b.x, a.y / b.y, a.z / b.z);
}

YU_INLINE const Vector3 operator / (const Vector3& v, float s)
{
	//assert(s != 0);
	return _Vector3(v.x / s, v.y / s, v.z / s);
}

YU_INLINE Vector3 cross(const Vector3& a, const Vector3& b)
{
	return _Vector3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

YU_INLINE float dot(const Vector3& a, const Vector3& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

YU_INLINE float ScalarTriple(const Vector3& a, const Vector3& b, const Vector3& c)
{
	return dot(cross(a, b), c);
}

YU_INLINE Vector3 Max(const Vector3& a, const Vector3& b)
{
	return _Vector3(yu::max(a.x, b.x), yu::max(a.y, b.y), yu::max(a.z, b.z) );
}

YU_INLINE Vector3 Min(const Vector3& a, const Vector3& b)
{
	return _Vector3(yu::min(a.x, b.x), yu::min(a.y, b.y), yu::min(a.z, b.z));
}

YU_INLINE Vector3 Normalize(const Vector3& a)
{
	Vector3 n(a);
	n.Normalize();
	return n;
}

struct Vector4 {
	float operator []  (int index) const;	
	float& operator [](int index);

	const Vector4& operator+=(const Vector4& rhs);	
	const Vector4& operator+=(float s);	
	const Vector4& operator-=(const Vector4& rhs);	
	const Vector4& operator-=(const float s);
	const Vector4& operator*=(const Vector4& rhs);
	const Vector4& operator*=(const float s);
	const Vector4& operator/=(const Vector4& rhs);
	const Vector4& operator/=(const float s);
	bool operator == (const Vector4& rhs) const;

	bool  Compare( const Vector4 &a ) const;							// exact compare, no epsilon
	bool  Compare( const Vector4 &a, const float epsilon ) const;		// compare with epsilon

	float SquaredLength() const;
    float Length() const;
	void  Normalize();	

	float x;
	float y;
	float z;
	float w;
};

YU_INLINE Vector4 _Vector4(float* _pVec)
{
	Vector4 result;
	result.x = _pVec[0];
	result.y = _pVec[1];
	result.z = _pVec[2];
	result.w = _pVec[3];
	return result;
}

YU_INLINE Vector4 _Vector4(float _xyzw)
{
	Vector4 result;
	result.x = result.y = result.z = result.w = _xyzw;

	return result;
}

YU_INLINE Vector4 _Vector4(float _x, float _y, float _z, float _w)
{
	Vector4 result;
	result.x = _x;
	result.y = _y;
	result.z = _z;
	result.w = _w;

	return result;
}

YU_INLINE Vector4 _Vector4(const Vector3& _xyz, float _w)
{
	Vector4 result;
	result.x = _xyz.x;
	result.y = _xyz.y;
	result.z = _xyz.z;
	result.w = _w;
	return result;
}

YU_INLINE float Vector4::operator [](int index) const
{
	return (&x)[index];
}

YU_INLINE float& Vector4::operator [](int index)
{
	return (&x)[index];
}

YU_INLINE Vector4 operator-(const Vector4& v)
{
    return _Vector4(-v.x, -v.y, -v.z, -v.w);
}

YU_INLINE const Vector4& Vector4::operator+=(const Vector4& rhs)
{
	x += rhs.x;
	y += rhs.y;
	z += rhs.z;
	w += rhs.w;
	return *this;
}

YU_INLINE const Vector4& Vector4::operator+=(float c)
{
	x += c;
	y += c;
	z += c;
	w += c;
	return *this;
}

YU_INLINE const Vector4& Vector4::operator-=(const Vector4& rhs)
{
	x -= rhs.x;
	y -= rhs.y;
	z -= rhs.z;
	w -= rhs.w;
	return *this;
}

YU_INLINE const Vector4& Vector4::operator -=(float c)
{
	x -= c;
	y -= c;
	z -= c;
	w -= c;
	return *this;
}

YU_INLINE const Vector4& Vector4::operator*=(const Vector4& rhs)
{
	x *= rhs.x;
	y *= rhs.y;
	z *= rhs.z;
	w *= rhs.w;
	return *this;
}

YU_INLINE const Vector4& Vector4::operator *=(float c)
{
	x *= c;
	y *= c;
	z *= c;
	w *= c;
	return *this;
}

YU_INLINE const Vector4& Vector4::operator /=(const Vector4& rhs)
{
	//assert(rhs.x != 0 && rhs.y != 0 && rhs.z != 0 && rhs.w != 0);
	x /= rhs.x;
	y /= rhs.y;
	z /= rhs.z;
	w /= rhs.w;
	return *this;
}

YU_INLINE const Vector4& Vector4::operator /= (float c)
{
	//assert(c!= 0);
	x /= c;
	y /= c;
	z /= c;
	w /= c;
	return *this;
}

YU_INLINE float Vector4::SquaredLength() const
{
    return x * x + y * y + z * z + w * w;
}

YU_INLINE float Vector4::Length() const
{
    return sqrtf(SquaredLength());
}

YU_INLINE void Vector4::Normalize()
{
	float length = Length();
	x /= length;
	y /= length;
	z /= length;
	w /= length;
}

YU_INLINE Vector3 DivideW(const Vector4& _vec4)
{
	return _Vector3(_vec4.x/_vec4.w, _vec4.y/_vec4.w, _vec4.z/_vec4.w);
}

YU_INLINE Vector3 DiscardW(const Vector4& _vec4)
{
	return _Vector3(_vec4.x, _vec4.y, _vec4.z);
}

YU_INLINE const Vector4 operator + (const Vector4& a, const Vector4& b)
{
	return _Vector4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);	
}

YU_INLINE const Vector4 operator + (float a, const Vector4& b)
{
	return _Vector4(a + b.x, a + b.y, a + b.z, a + b.w);	
}	

YU_INLINE const Vector4 operator + (Vector4& a, float b)
{
	return _Vector4(a.x + b, a.y + b, a.z + b, a.w + b);	
}	

YU_INLINE const Vector4 operator - (const Vector4& a, const Vector4& b)
{
	return _Vector4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);	
}

YU_INLINE const Vector4 operator*(const Vector4& a, const Vector4& b)
{
	return _Vector4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}

YU_INLINE const Vector4 operator*(float a, const Vector4& b)
{
	return _Vector4(a * b.x, a * b.y, a * b.z, a * b.w);
}

YU_INLINE const Vector4 operator*(const Vector4& a, float b)
{
	return _Vector4(a.x * b, a.y * b, a.z * b, a.w * b);
}		

YU_INLINE const Vector4 operator/(const Vector4& a, const Vector4& b)
{
	//assert(b.x != 0 && b.y != 0 && b.z != 0);
	return _Vector4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
}

YU_INLINE const Vector4 operator / (const Vector4& v, float s)
{
	//assert(s != 0);
	return _Vector4(v.x / s, v.y / s, v.z / s, v.w / s);
}

YU_INLINE Vector4 Max(const Vector4& a, const Vector4& b)
{
	return _Vector4(yu::max(a.x, b.x), yu::max(a.y, b.y), yu::max(a.z, b.z), yu::max(a.w, b.w) );
}

YU_INLINE Vector4 Min(const Vector4& a, const Vector4& b)
{
	return _Vector4(yu::min(a.x, b.x), yu::min(a.y, b.y), yu::min(a.z, b.z), yu::min(a.w, b.w));
}

YU_INLINE bool Vector4::operator == (const Vector4& rhs) const
{
    return ( (x == rhs.x) && (y == rhs.y) && (z == rhs.z) && (w == rhs.w));
}

	
}

#endif
