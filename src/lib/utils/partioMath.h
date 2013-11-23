/*
PARTIO SOFTWARE
Copyright 2013 Disney Enterprises, Inc. All rights reserved

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.

* The names "Disney", "Walt Disney Pictures", "Walt Disney Animation
Studios" or the names of its contributors may NOT be used to
endorse or promote products derived from this software without
specific prior written permission from Walt Disney Pictures.

Disclaimer: THIS SOFTWARE IS PROVIDED BY WALT DISNEY PICTURES AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE, NONINFRINGEMENT AND TITLE ARE DISCLAIMED.
IN NO EVENT SHALL WALT DISNEY PICTURES, THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND BASED ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
*/

#ifndef PARTIOMATH_H
#define PARTIOMATH_H

#include <iostream>
#include <sstream>
#include <cmath>
#include <float.h>

#include "vector3d.h"

using namespace std;

namespace Partio
{

	void CubeToSphere(Vector3D& P);
	void CartesianCoordToSphericalCoord(Vector3D XYZ, Vector3D& RTP);
	void SphericalCoordToCartesianCoord(Vector3D RTP, Vector3D& XYZ);


template <typename T>
inline T clamp(const T& val, const T& min, const T& max)
{
    T res = val;
    if (val>max)	return max;
    if (val<min)	return min;
    return val;
}

template <typename T>
T smoothstep(T edge0, T edge1, T x)
{
    // Scale, bias and saturate x to 0..1 range
    x = clamp<T>( (x - edge0) / (edge1 - edge0), 0, 1);
    // Evaluate polynomial
    return x*x*(T(3)-T(2)*x);
}

template <typename T>
T smootherstep(T edge0, T edge1, T x)
{
    // Scale, and saturate x to 0..1 range
    x = clamp<T>( (x - edge0) / (edge1 - edge0), 0, 1);
    // Evaluate polynomial
    return x*x*x*(x*(x*6 - 15) + 10);
}




}
#endif
