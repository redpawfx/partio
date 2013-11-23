/*----------------------------------------------------------------------------*
 | headerfile Vector3D.h                                                      |
 |                                                                            |
 | version 1.00                                                               |
 |                                                                            |
 |                                                                            |
 | This file is part of the 3D Geometry Primer, found at:                     |
 | http://www.flipcode.com/geometry/                                          |
 |                                                                            |
 | It contains an example implementation of a 3D vector.                      |
 | You can find more info in the 3D Geometry Primer on www.flipcode.com.      |
 |                                                                            |
 |                                                                            |
 | Copyright (C) 2002  Bram de Greve                                          |
 |                                                                            |
 | This library is free software; you can redistribute it and/or              |
 | modify it under the terms of the GNU Lesser General Public                 |
 | License as published by the Free Software Foundation; either               |
 | version 2.1 of the License, or (at your option) any later version.         |
 |                                                                            |
 | This library is distributed in the hope that it will be useful,            |
 | but WITHOUT ANY WARRANTY; without even the implied warranty of             |
 | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU          |
 | Lesser General Public License for more details.                            |
 |                                                                            |
 | You should have received a copy of the GNU Lesser General Public           |
 | License along with this library; if not, write to the Free Software        |
 | Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA  |
 |                                                                            |
 | You can contact me by e-mail on bdegreve@mail.be                           |
 *----------------------------------------------------------------------------*/

// MODIFIED to be more versatile for the Partio Project by  John Cassella (redpawfx)

/// TODO: eventually we'll possibly swap this out with SeExpr library functions
///       but for now this is a pretty good and simple vector struct to use


#ifndef VECTOR3D_H
#define VECTOR3D_H

#include <cmath>
#include <iostream>

// using color output
#include "colorOutput.h"


using namespace std;

/////////////////////////////////////
// Vector3D structure

struct Vector3D
{
public:

	float x;
    float y;
    float z;

    // --- PUBLIC STRUCTORS ---

// default constructor
    Vector3D():
			x(0.0f),
            y(0.0f),
            z(0.0f)
    {
    }

    // constructor taking three component values (x, y, z)
    Vector3D(float a_x, float a_y, float a_z):
            x(a_x),
            y(a_y),
            z(a_z)
    {
    }

    // constructor taking three component values an array
    Vector3D(float in[3]):
            x(in[0]),
            y(in[1]),
            z(in[2])
    {
    }


    // --- PUBLIC OPERATORS ---

	// return the negative of this vector
	Vector3D operator-() const
    {
        return Vector3D(-x, -y, -z);
    }

	// get the Nth entry of the array
    double operator[](uint i) const
    {
		if(i == 0)
			return x;
		else if(i == 1)
			return y;
		else if(i == 2)
			return z;
		else
			return NULL;
    }

    // add v to this vector, and return *this
    Vector3D& operator+=(const Vector3D& a_v)
    {
        x += a_v.x;
        y += a_v.y;
        z += a_v.z;
        return *this;
    }
    // add f to this vector, and return *this
    Vector3D& operator+=(const float& a_f)
    {
        x += a_f;
        y += a_f;
        z += a_f;
        return *this;
    }

    // subtract v from this vector, and return *this
    Vector3D& operator-=(const Vector3D& a_v)
    {
        x -= a_v.x;
        y -= a_v.y;
        z -= a_v.z;
        return *this;
    }
    // subtract f from this vector, and return *this
    Vector3D& operator-=(const float& a_f)
    {
        x -= a_f;
        y -= a_f;
        z -= a_f;
        return *this;
    }

    // scale this vector with r, and return *this
    Vector3D& operator*=(float a_r)
    {
        x *= a_r;
        y *= a_r;
        z *= a_r;
        return *this;
    }

    // scale this vector with 1/r, and return *this
    Vector3D& operator/=(float a_r)
    {
        *this *= (1.0f / a_r);
        return *this;
    }


    // --- PUBLIC ACCESSORS ---

    // set the three component values (x, y, z) of this vector
    void set(float a_x, float a_y, float a_z)
    {
        x = a_x;
        y = a_y;
        z = a_z;
    }
    // set the three component values (x, y, z) of this vector
    void set(float in[3])
    {
        x = in[0];
        y = in[1];
        z = in[2];
    }

    // set this vector to the null vector (0, 0, 0)
    void zero()
    {
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
    }


    // --- PUBLIC METHODS ---

    // return true if this is the null vector (0, 0, 0)
    bool isZero() const
    {
        return (x == 0.0f) && (y == 0.0f) && (z == 0.0f);
    }

    // return true if this is the null vector (0, 0, 0) within a given tolerance
    bool isNearZero(float a_tolerance) const
    {
        return lengthSq() <= (a_tolerance * a_tolerance);
    }

    // return the norm (== length == magnitude) of this vector
    float length() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    // return the squared norm (== norm * norm) of this vector
    float lengthSq() const
    {
        return x * x + y * y + z * z;
    }

    // scale this vector so that it's norm has length 1
    void normalize()
    {
        const float squaredNorm = lengthSq();

        if (squaredNorm != 0.0f)
        {
            const float invNorm = 1.0f / sqrt(squaredNorm);
            x *= invNorm;
            y *= invNorm;
            z *= invNorm;
        }
    }

    // return true if this is a unit vector
    bool isNormalized() const
    {
        return lengthSq() == 1.0f;
    }

    // return true is this is a unit vector within a given tolerance
    bool isNearNormalized(float a_tolerance) const
    {
        return fabs(length() - 1.0f) <= a_tolerance;
    }


};

// --- NON-MEMBER BOOLEAN OPERATORS --------------------------------------------

// return true if v equals w
inline bool operator==(const Vector3D& a_v, const Vector3D& a_w)
{
    return (a_v.x == a_w.x) && (a_v.y == a_w.y) && (a_v.z == a_w.z);
}

// return true if v doesn't equal w
inline bool operator!=(const Vector3D& a_v, const Vector3D& a_w)
{
    return (a_v.x != a_w.x) || (a_v.y != a_w.y) || (a_v.z != a_w.z);
}



// --- NON-MEMBER ARITHMETICAL OPERATORS ---------------------------------------

// return w added to v
inline Vector3D operator+(const Vector3D& a_v, const Vector3D& a_w)
{
    return Vector3D(a_v.x + a_w.x, a_v.y + a_w.y, a_v.z + a_w.z);
}

// return w subtracted from v
inline Vector3D operator-(const Vector3D& a_v, const Vector3D& a_w)
{
    return Vector3D(a_v.x - a_w.x, a_v.y - a_w.y, a_v.z - a_w.z);
}

// return v scaled by r
inline Vector3D operator*(const Vector3D& a_v, const float a_r)
{
    return Vector3D(a_v.x * a_r, a_v.y * a_r, a_v.z * a_r);
}

// return v scaled by r
inline Vector3D operator*(float a_r, const Vector3D& a_v)
{
    return a_v * a_r;
}

// return v scaled by 1/r
inline Vector3D operator/(const Vector3D a_v, float a_r)
{
    return a_v * (1.0f / a_r);
}


// --- NON-MEMBER ARITHMETICAL FUNCTIONS ---------------------------------------

// return the dot product between a_v and a_w
inline float dot(const Vector3D& a_v, const Vector3D& a_w)
{
    return a_v.x * a_w.x + a_v.y * a_w.y + a_v.z * a_w.z;
}

// return the cross product between a_v and a_w
inline Vector3D cross(const Vector3D& a_v, const Vector3D& a_w)
{
    return Vector3D(a_v.y * a_w.z - a_v.z * a_w.y,
                    a_v.z * a_w.x - a_v.x * a_w.z,
                    a_v.x * a_w.y - a_v.y * a_w.x);
}

// return the triple product between u, v and w
inline float triple(const Vector3D& a_u, const Vector3D& a_v,
                    const Vector3D& a_w)
{
    return dot(a_u, cross(a_v, a_w));
}

// pointwise multiplication of two vectors v and w
inline Vector3D pointwiseMul(const Vector3D& a_v, const Vector3D& a_w)
{
    return Vector3D(a_v.x * a_w.x, a_v.y * a_w.y, a_v.z * a_w.z);
}

// pointwise division of two vectors v and w
inline Vector3D pointwiseDiv(const Vector3D& a_v, const Vector3D& a_w)
{
    return Vector3D(a_v.x / a_w.x, a_v.y / a_w.y, a_v.z / a_w.z);
}



// --- NON-MEMBER OUTPUT FUNCTIONS ---------------------------------------------


// write vector v to output stream theStream
inline std::ostream& operator<<(std::ostream& a_stream, const Vector3D& a_v)
{
	Color::Modifier fYellow(Color::FG_YELLOW);
	Color::Modifier fDefault(Color::FG_DEFAULT);
	char out[100];
	sprintf(out,"[%.4f, %.4f, %.4f]",a_v.x,a_v.y,a_v.z);
    a_stream << fYellow <<  out << fDefault;
    return a_stream;
}


#endif
