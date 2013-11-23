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

#include <iostream>
#include <sstream>
#include <cmath>
#include <float.h>

#include "vector3d.h"
#include "partioMath.h"

using namespace std;

namespace Partio {

// taken from Bo Schwarzstein code:
void CubeToSphere(Vector3D& P)
{
    float Theta = P.x * M_PI * 2.0;
    float U = P.y * 2.0f - 1.0f;

    P.x = cos(Theta) * sqrt( 1.0f - U*U );
    P.y = sin(Theta) * sqrt( 1.0f - U*U );
    P.z = U;
}

void CartesianCoordToSphericalCoord(Vector3D XYZ, Vector3D& RTP)
{
    RTP.x = sqrt( XYZ.x*XYZ.x + XYZ.y*XYZ.y + XYZ.z*XYZ.z );
    if ( XYZ.x > 0 )
    {
		RTP.y = atan( XYZ.y/XYZ.x ) + M_PI;
    }
    else if ( fabs(XYZ.x) < FLT_EPSILON )
    {
        if ( XYZ.y > 0.0f )
        {
            RTP.y = M_PI_2;
        }
        else
        {
            RTP.y = -M_PI_2;
        }
    }
    else
    {
        RTP.y = atan( XYZ.y/XYZ.x );
    }
    RTP.z = acos( XYZ.z/RTP.x );
}

void SphericalCoordToCartesianCoord(Vector3D RTP, Vector3D& XYZ)
{
    XYZ.x = RTP.x * cos(RTP.y) * sin(RTP.z);
    XYZ.y = RTP.x * sin(RTP.y) * sin(RTP.z);
    XYZ.z = RTP.x * cos(RTP.z);
}

} // end namespace Partio
