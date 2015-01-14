#ifndef YU_MATRIX_H
#define YU_MATRIX_H

#include "yu_math.h"
#include "vector.h"
#include "../core/string.h"

namespace yu
{

class Matrix2x2
{
public:
	Matrix2x2() {}
	Matrix2x2(float m00, float m01, float m10, float m11);
	explicit Matrix2x2(const float* floatPtr);
	
	Vector2& operator [] (int row);
	const Vector2& operator [] (int row) const;

//	Vector2 column(int column) const;

	const Matrix2x2& operator*= (float a);
	const Matrix2x2& operator*= (const Matrix2x2& mat);

	Matrix2x2 operator * (float a) const;
	Vector2   operator * (const Vector2& vec) const;
	Matrix2x2 operator * (const Matrix2x2& mat) const;

	Vector2 row[2];
};

inline Matrix2x2::Matrix2x2(float m00, float m01, float m10, float m11)
{
	row[0].x = m00; row[0].y = m01;
	row[1].x = m10; row[1].y = m11;
}

inline Matrix2x2::Matrix2x2(const float* floatPtr)
{
	row[0].x = floatPtr[0]; row[0].y = floatPtr[1];
	row[1].x = floatPtr[2]; row[1].y = floatPtr[3];
}

inline Vector2& Matrix2x2::operator [] (int _row)
{
	return row[_row];
}

inline const Vector2& Matrix2x2::operator [] (int _row) const 
{
	return row[_row];
}

inline const Matrix2x2& Matrix2x2::operator*= (float a)
{
	row[0].x *= a; row[0].y *= a;
	row[1].x *= a; row[1].y *= a;
	return *this;
}

inline const Matrix2x2& Matrix2x2::operator*= (const Matrix2x2& mat)
{
	float tmp[2];

	tmp[0] = row[0].x * mat[0][0] + row[0].y * mat[1][0];
	tmp[1] = row[0].x * mat[0][1] + row[0].y * mat[1][1];
 
	row[0].x = tmp[0]; row[1].y = tmp[1];

	tmp[0] = row[1].x * mat[0][0] + row[1].y * mat[1][0];
	tmp[1] = row[1].x * mat[0][1] + row[1].y * mat[1][1];
	
	row[1].x = tmp[0]; row[1].y = tmp[1];

	return *this;
}

inline Matrix2x2 Matrix2x2::operator * (float a) const
{
	Matrix2x2 dst = (*this);
	return dst *= a;
} 

inline Vector2 Matrix2x2::operator * (const Vector2& vec) const
{
	return _Vector2(row[0].x * vec.x + row[0].y * vec.y, row[1].x * vec.x + row[1].y * vec.y);
}

inline Matrix2x2 Matrix2x2::operator * (const Matrix2x2& mat) const
{
	Matrix2x2 dst  = (*this);
	return dst *= mat;
}

class Matrix3x3
{
public: 
	Matrix3x3(){};
	Matrix3x3(float m00, float m01, float m02,
		float m10, float m11, float m12,
		float m20, float m21, float m22);	
	explicit Matrix3x3(const float* floatPtr);

	Vector3& operator [] (int row);
	const Vector3& operator [] (int row) const;

	const Matrix3x3& operator*= (float a);
	const Matrix3x3& operator*= (const Matrix3x3& mat);

	Matrix3x3 operator * (float a) const;
	Vector3   operator * (const Vector3& vec) const;
	Matrix3x3 operator * (const Matrix3x3& mat) const;

	void MakeIdentity();
	Matrix3x3  Transpose();
	Matrix3x3& TransposeSelf();
	YU_CLASS_FUNCTION Matrix3x3 RotateAxis(const Vector3& vec, float rad);

    float* FloatPtr();
    const float* FloatPtr() const;

private:
	Vector3 row[3];
};

inline Matrix3x3::Matrix3x3(float m00, float m01, float m02,
                              float m10, float m11, float m12,
                              float m20, float m21, float m22)
{
	row[0].x = m00; row[0].y = m01; row[0].z = m02;
	row[1].x = m10; row[1].y = m11; row[1].z = m12; 
	row[2].x = m20; row[2].y = m21; row[2].z = m22;
}

inline Matrix3x3::Matrix3x3(const float *floatPtr)
{
	float* mPtr = reinterpret_cast<float*> (this);
	yu::memcpy(mPtr, floatPtr, sizeof(float) * 9);
}

inline Vector3& Matrix3x3::operator [] (int _row)
{
	return row[_row];
}

inline const Vector3& Matrix3x3::operator [] (int _row) const
{
	return row[_row];
}

inline const Matrix3x3& Matrix3x3::operator *= (float a) 
{
	row[0].x *= a;	row[0].y *= a; row[0].z *= a;
	row[1].x *= a;	row[1].y *= a; row[1].z *= a;
	row[2].x *= a;	row[2].y *= a; row[2].z *= a;

	return *this;
}

inline const Matrix3x3& Matrix3x3::operator *= (const Matrix3x3& mat)
{
	int i, j;
	const float *m2Ptr;
	float *m1Ptr, dst[3];

	m1Ptr = reinterpret_cast<float *>(this);
	m2Ptr = reinterpret_cast<const float *>(&mat);

	for ( i = 0; i < 3; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			dst[j]  = m1Ptr[0] * m2Ptr[ 0 * 3 + j ]
					+ m1Ptr[1] * m2Ptr[ 1 * 3 + j ]
					+ m1Ptr[2] * m2Ptr[ 2 * 3 + j ];
		}
		m1Ptr[0] = dst[0]; m1Ptr[1] = dst[1]; m1Ptr[2] = dst[2];
		m1Ptr += 3;
	}	

	return *this;
}

inline Matrix3x3 Matrix3x3::operator * (float a) const
{
	Matrix3x3 dst = (*this);
	return dst *= a;
}

inline Vector3 Matrix3x3::operator * (const Vector3& _vec) const
{
	return _Vector3(
		row[0].x * _vec.x + row[0].y * _vec.y + row[0].z * _vec.z,
		row[1].x * _vec.x + row[1].y * _vec.y + row[1].z * _vec.z,
		row[2].x * _vec.x + row[2].y * _vec.y + row[2].z * _vec.z
		);
}

inline Matrix3x3 Matrix3x3::operator *(const Matrix3x3 &mat) const
{
	Matrix3x3 dst = (*this);
	return dst *= mat;
}

inline void Matrix3x3::MakeIdentity()
{
	row[0].x = 1.f; row[0].y = 0.f; row[0].z = 0.f;
	row[1].x = 0.f; row[1].y = 1.f; row[1].z = 0.f;
	row[2].x = 0.f; row[2].y = 0.f; row[2].z = 1.f;
}

inline Matrix3x3 Matrix3x3::Transpose()
{
	return Matrix3x3(
		row[0].x, row[1].x, row[2].x,
		row[0].y, row[1].y, row[2].y,
		row[0].z, row[1].z, row[2].z
	);
}

inline Matrix3x3& Matrix3x3::TransposeSelf()
{
	float temp0, temp1, temp2;
	temp0 = row[0][1];
	row[0][1] = row[1][0];
	row[1][0] = temp0;

	temp1 = row[0][2];
	row[0][2] = row[2][0];
	row[2][0] = temp1;

	temp2 = row[1][2];
	row[1][2] = row[2][1];
	row[2][1] = temp2;

	return *this;
}


inline Matrix3x3 Matrix3x3::RotateAxis(const Vector3& r, float rad)
{
	float cosPhi = cosf(rad);
	float one_cosPhi = (1.f - cosPhi);
	float sinPhi = sinf(rad);
	
	return Matrix3x3(
		cosPhi + one_cosPhi * r.x * r.x,       one_cosPhi * r.x * r.y - r.z * sinPhi, one_cosPhi  * r.x * r.z + r.y * sinPhi,
		one_cosPhi * r.x * r.y + r.z * sinPhi, cosPhi + one_cosPhi * r.y * r.y,       one_cosPhi * r.y * r.z - r.x * sinPhi,
		one_cosPhi * r.x * r.z - r.y * sinPhi, one_cosPhi * r.y * r.z + r.x * sinPhi, cosPhi + one_cosPhi * r.z * r.z
	);
}

inline float* Matrix3x3::FloatPtr()
{
    return reinterpret_cast<float*> (this);
}

inline const float* Matrix3x3::FloatPtr() const
{
    return reinterpret_cast<const float*> (this);
}

class Matrix4x4
{
public:
   Matrix4x4(){}
   Matrix4x4(float m00, float m01, float m02, float m03,
                      float m10, float m11, float m12, float m13,
                      float m20, float m21, float m22, float m23,
                      float m30, float m31, float m32, float m33);

   explicit Matrix4x4(const float* floatPtr);

   explicit Matrix4x4(const float m[4][4]);

   void Set(float m00, float m01, float m02, float m03,
            float m10, float m11, float m12, float m13,
            float m20, float m21, float m22, float m23,
            float m30, float m31, float m32, float m33);

   Vector4& operator [] (int _row);
   const Vector4& operator [] (int _row) const;

   Matrix4x4 operator* (float a) const;
   Vector4 operator*(const Vector4& vec) const;
   Matrix4x4 operator* (const Matrix4x4& mat) const;

   const Matrix4x4& operator*=(float a);
   const Matrix4x4& operator*=(const Matrix4x4& mat);

   friend Matrix4x4 operator*(float a, const Matrix4x4& mat);
   friend Vector4 operator*(const Vector4& vec, const Matrix4x4& mat);

   const float* FloatPtr() const;
   float* FloatPtr();

   Matrix4x4  Transpose();
   Matrix4x4& TransposeSelf();

private:
   
   Vector4 row[4];
};

inline Matrix4x4::Matrix4x4(float m00, float m01, float m02, float m03, 
                              float m10, float m11, float m12, float m13, 
                              float m20, float m21, float m22, float m23, 
                              float m30, float m31, float m32, float m33)
{
	row[0][0] = m00;
	row[0][1] = m01;
	row[0][2] = m02;
	row[0][3] = m03;

	row[1][0] = m10;
	row[1][1] = m11;
	row[1][2] = m12;
	row[1][3] = m13;

	row[2][0] = m20;
	row[2][1] = m21;
	row[2][2] = m22;
	row[2][3] = m23;

	row[3][0] = m30;
	row[3][1] = m31;
	row[3][2] = m32;
	row[3][3] = m33;

}

inline Matrix4x4::Matrix4x4(const float* _mIn)
{	
	yu::memcpy(FloatPtr(), _mIn, sizeof(float) * 16 );
}

inline Matrix4x4::Matrix4x4(const float m[4][4])
{
	yu::memcpy(FloatPtr(), m, sizeof(float) * 16 );
}

inline Vector4& Matrix4x4::operator[](int _row)
{	
	return row[_row];
}
	
inline const  Vector4& Matrix4x4::operator[](int _row) const
{
	return row[_row];
}

inline float* Matrix4x4::FloatPtr()
{
	return reinterpret_cast<float*> (this);
}

inline const float* Matrix4x4::FloatPtr() const
{
    return reinterpret_cast<const float*> (this);
}

inline void Matrix4x4::Set(float m00, float m01, float m02, float m03, 
							 float m10, float m11, float m12, float m13, 
							 float m20, float m21, float m22, float m23, 
							 float m30, float m31, float m32, float m33)
{
	row[0][0] = m00; row[0][1] = m01; row[0][2] = m02; row[0][3] = m03;
	row[1][0] = m10; row[1][1] = m11; row[1][2] = m12; row[1][3] = m13;
	row[2][0] = m20; row[2][1] = m21; row[2][2] = m22; row[2][3] = m23;
	row[3][0] = m30; row[3][1] = m31; row[3][2] = m32; row[3][3] = m33;
}

inline Matrix4x4 Matrix4x4::operator *(float a) const
{
	Matrix4x4 dst = (*this);
	return dst *= a;
}

inline Vector4 Matrix4x4::operator *(const Vector4 &vec) const
{
	return _Vector4(
		row[0][0] * vec.x + row[0][1] * vec.y + row[0][2] * vec.z + row[0][3] * vec.w,
		row[1][0] * vec.x + row[1][1] * vec.y + row[1][2] * vec.z + row[1][3] * vec.w,
		row[2][0] * vec.x + row[2][1] * vec.y + row[2][2] * vec.z + row[2][3] * vec.w,
		row[3][0] * vec.x + row[3][1] * vec.y + row[3][2] * vec.z + row[3][3] * vec.w
		);	
}

inline Matrix4x4 Matrix4x4::operator *(const Matrix4x4& mat) const
{
	Matrix4x4 dst = (*this);
	return dst *= mat;
}

inline const Matrix4x4& Matrix4x4::operator *=(float a)
{
	row[0][0] *= a; row[0][1] *= a; row[0][2] *= a; row[0][3] *= a;
	row[1][0] *= a; row[1][1] *= a; row[1][2] *= a; row[1][3] *= a;
	row[2][0] *= a; row[2][1] *= a; row[2][2] *= a; row[2][3] *= a;
	row[3][0] *= a; row[3][1] *= a; row[3][2] *= a; row[3][3] *= a;

	return *this;
}

inline const Matrix4x4& Matrix4x4::operator *= (const Matrix4x4& mat)
{
	int i, j;
	float* m1Ptr;
	const float* m2Ptr;
	float dst[4];

	m1Ptr = reinterpret_cast<float*> (this);
	m2Ptr = reinterpret_cast<const float*>(&mat);

	for ( i = 0; i < 4; i++ ) {
		for ( j = 0; j < 4; j++ ) {
			dst[j] = m1Ptr[0] * m2Ptr[ 0 * 4 + j ]
					+ m1Ptr[1] * m2Ptr[ 1 * 4 + j ]
					+ m1Ptr[2] * m2Ptr[ 2 * 4 + j ]
					+ m1Ptr[3] * m2Ptr[ 3 * 4 + j ];
		}
		m1Ptr[0] = dst[0]; m1Ptr[1] = dst[1]; m1Ptr[2] = dst[2]; m1Ptr[3] = dst[3];
		m1Ptr += 4;
	}
	return *this;
}



//test __restrict
inline void Matrix4x4Mult(const float* m1Ptr, const float* m2Ptr, float* __restrict dstPtr)
{
	int i, j;
	for ( i = 0; i < 4; i++ ) {
		for ( j = 0; j < 4; j++ ) {
			*dstPtr = m1Ptr[0] * m2Ptr[ 0 * 4 + j ]
			+ m1Ptr[1] * m2Ptr[ 1 * 4 + j ]
			+ m1Ptr[2] * m2Ptr[ 2 * 4 + j ]
			+ m1Ptr[3] * m2Ptr[ 3 * 4 + j ];
			dstPtr++;
		}
		m1Ptr += 4;
	}
}

inline Matrix4x4 operator*(float a, const Matrix4x4& mat)
{
	return Matrix4x4(
		a * mat[0][0], a * mat[0][1], a * mat[0][2], a * mat[0][3],
		a * mat[1][0], a * mat[1][1], a * mat[1][2], a * mat[1][3],
		a * mat[2][0], a * mat[2][1], a * mat[2][2], a * mat[2][3],
		a * mat[3][0], a * mat[3][1], a * mat[2][2], a * mat[3][3]
	);
}

inline Matrix4x4 Matrix4x4::Transpose()
{
	return Matrix4x4(
			row[0][0], row[1][0], row[2][0], row[3][0],
			row[0][1], row[1][1], row[2][1], row[3][1],
			row[0][2], row[1][2], row[2][2], row[3][2],
			row[0][3], row[1][3], row[2][3], row[3][3]
	);
}

inline Matrix4x4& Matrix4x4::TransposeSelf()
{
	float temp0, temp1, temp2, temp3, temp4, temp5;
	temp0 = row[0][1];
	row[0][1] = row[1][0];
	row[1][0] = temp0;

	temp1 = row[0][2];
	row[0][2] = row[2][0];
	row[2][0] = temp1;

	temp2 = row[0][3];
	row[0][3] = row[3][0];
	row[3][0] = temp2;

	temp3 = row[1][2];
	row[1][2] = row[2][1];
	row[2][1] = temp3;

	temp4 = row[1][3];
	row[1][3] = row[3][1];
	row[3][1] = temp4;

	temp5 = row[2][3];
	row[2][3] = row[3][2];
	row[3][2] = temp5;

	return *this;

}

inline void MakeIdentity(Matrix4x4& mat)
{
	mat[0][0] = 1.0f;
	mat[0][1] = 0.0f;
	mat[0][2] = 0.0f;
	mat[0][3] = 0.0f;

	mat[1][0] = 0.0f;
	mat[1][1] = 1.0f;
	mat[1][2] = 0.0f;
	mat[1][3] = 0.0f;

	mat[2][0] = 0.0f;
	mat[2][1] = 0.0f;
	mat[2][2] = 1.0f;
	mat[2][3] = 0.0f;

	mat[3][0] = 0.0f;
	mat[3][1] = 0.0f;
	mat[3][2] = 0.0f;
	mat[3][3] = 1.0f;
}

inline Matrix4x4 Identity4x4()
{
    Matrix4x4 identity;
    MakeIdentity(identity);

    return identity;
}

inline Matrix4x4 Translate(const Vector3& _offset)
{
    return Matrix4x4(1, 0, 0, _offset.x,
                     0, 1, 0, _offset.y,
                     0, 0, 1, _offset.z,
                     0, 0, 0, 1);
}

inline Matrix4x4 Scale(const Vector3& _scale)
{
    return Matrix4x4(_scale.x, 0, 0, 0,
                     0, _scale.y, 0, 0,
                     0, 0, _scale.z, 0,
                     0, 0, 0, 1);
}

inline Vector3 TransformVector(const Matrix4x4& m, const Vector3& v)
{
	return DiscardW(m * _Vector4(v, 0.f) );
}

inline Vector3 TransformPoint(const Matrix4x4& m, const Vector3& v)
{
	return DivideW(m * _Vector4(v, 1.f));
}

inline Matrix4x4 RotateAxis(const Vector3& r, float rad)
{

	float cosPhi = cosf(rad);
	float one_cosPhi = (1.f - cosPhi);
	float sinPhi = sinf(rad);

	Matrix3x3 rotMatrix = Matrix3x3(
		cosPhi + one_cosPhi * r.x * r.x,       one_cosPhi * r.x * r.y - r.z * sinPhi, one_cosPhi  * r.x * r.z + r.y * sinPhi,
		one_cosPhi * r.x * r.y + r.z * sinPhi, cosPhi + one_cosPhi * r.y * r.y,       one_cosPhi * r.y * r.z - r.x * sinPhi,
		one_cosPhi * r.x * r.z - r.y * sinPhi, one_cosPhi * r.y * r.z + r.x * sinPhi, cosPhi + one_cosPhi * r.z * r.z
		);

	return Matrix4x4(rotMatrix[0][0], rotMatrix[0][1], rotMatrix[0][2], 0,
					 rotMatrix[1][0], rotMatrix[1][1], rotMatrix[1][2], 0,
					 rotMatrix[2][0], rotMatrix[2][1], rotMatrix[2][2], 0,
					 0, 0, 0, 1.f);
}

inline Matrix4x4 InverseAffine(const Matrix4x4& m)
{
    Matrix3x3 orien = Matrix3x3(m[0][0], m[0][1], m[0][2],
              m[1][0], m[1][1], m[1][2],
              m[2][0], m[2][1], m[2][2]);
    
    Matrix3x3 invM = orien.Transpose();
    Vector3 invTranslation = - (invM * _Vector3(m[0][3], m[1][3], m[2][3]));
    
    return Matrix4x4(invM[0][0], invM[0][1], invM[0][2], invTranslation[0],
                     invM[1][0], invM[1][1], invM[1][2], invTranslation[1],
                     invM[2][0], invM[2][1], invM[2][2], invTranslation[2],
                     0, 0, 0, 1);
}

inline Matrix4x4 Inverse(const Matrix4x4 &m) {
	int indxc[4], indxr[4];
	int ipiv[4] = { 0, 0, 0, 0 };
	float minv[4][4];
	yu::memcpy(minv, &m, 4*4*sizeof(float));
	for (int i = 0; i < 4; i++) {
		int irow = -1, icol = -1;
		float big = 0.;
		// Choose pivot
		for (int j = 0; j < 4; j++) {
			if (ipiv[j] != 1) {
				for (int k = 0; k < 4; k++) {
					if (ipiv[k] == 0) {
						if (fabsf(minv[j][k]) >= big) {
							big = float(fabsf(minv[j][k]));
							irow = j;
							icol = k;
						}
					}
					else if (ipiv[k] > 1)
					{
						//printf("Singular matrix in MatrixInvert");
					}
				}
			}
		}
		++ipiv[icol];
		// Swap rows _irow_ and _icol_ for pivot
		if (irow != icol) {
			for (int k = 0; k < 4; ++k)
				yu::swap(minv[irow][k], minv[icol][k]);
		}
		indxr[i] = irow;
		indxc[i] = icol;
		if (minv[icol][icol] == 0.)
		{
			//printf("Singular matrix in MatrixInvert");
		}
		
		// Set $m[icol][icol]$ to one by scaling row _icol_ appropriately
		float pivinv = 1.f / minv[icol][icol];
		minv[icol][icol] = 1.f;
		for (int j = 0; j < 4; j++)
			minv[icol][j] *= pivinv;

		// Subtract this row from others to zero out their columns
		for (int j = 0; j < 4; j++) {
			if (j != icol) {
				float save = minv[j][icol];
				minv[j][icol] = 0;
				for (int k = 0; k < 4; k++)
					minv[j][k] -= minv[icol][k]*save;
			}
		}
	}
	// Swap columns to reflect permutation
	for (int j = 3; j >= 0; j--) {
		if (indxr[j] != indxc[j]) {
			for (int k = 0; k < 4; k++)
				yu::swap(minv[k][indxr[j]], minv[k][indxc[j]]);
		}
	}
	return Matrix4x4(minv);
}

}


#endif
