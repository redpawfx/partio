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
#include "expansion.h"
#include "random.h"
#include <io/pdb.h>

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


// taken from Bo Schwarzstein code:
static void CubeToSphere(float P[])
{
    float Theta = P[0] * M_PI * 2.0;
    float U = P[1] * 2.0f - 1.0f;

    P[0] = cos(Theta) * sqrt( 1.0f - U*U );
    P[1] = sin(Theta) * sqrt( 1.0f - U*U );
    P[2] = U;
}

static void CartesianCoordToSphericalCoord(float XYZ[], float RTP[])
{
    RTP[0] = sqrt( XYZ[0]*XYZ[0] + XYZ[1]*XYZ[1] + XYZ[2]*XYZ[2] );
    if ( XYZ[0] > 0 )
    {
		RTP[1] = atan( XYZ[1]/XYZ[0] ) + M_PI;
    }
    else if ( fabs(XYZ[0]) < FLT_EPSILON )
    {
        if ( XYZ[1] > 0.0f )
        {
            RTP[1] = M_PI_2;
        }
        else
        {
            RTP[1] = -M_PI_2;
        }
    }
    else
    {
        RTP[1] = atan( XYZ[1]/XYZ[0] );
    }
    RTP[2] = acos( XYZ[2]/RTP[0] );
}

static void SphericalCoordToCartesianCoord(float RTP[], float XYZ[])
{
    XYZ[0] = RTP[0] * cos(RTP[1]) * sin(RTP[2]);
    XYZ[1] = RTP[0] * sin(RTP[1]) * sin(RTP[2]);
    XYZ[2] = RTP[0] * cos(RTP[2]);
}

namespace Partio {
using namespace std;

ParticlesDataMutable* expandSoft(ParticlesDataMutable* expandedPData, bool sort, int numCopies, bool doVelo)
{

    if (sort)
    {
        expandedPData->sort();
    }
    ParticleAttribute posAttr;
    const float* masterPositions = NULL;
    const float* masterVelocities = NULL;
    bool foundVelo = false;
    if (expandedPData->attributeInfo("position",posAttr))
    {
        masterPositions = expandedPData->data<float>(posAttr,0);
    }
    ParticleAttribute velAttr;
    if (expandedPData->attributeInfo("velocity",velAttr))
    {
        masterVelocities = expandedPData->data<float>(velAttr,0);
        foundVelo = true;
    }

    std::vector<ParticleAttribute> posVec;
    std::vector<ParticleAttribute> velVec;
    for (int expCount = 0; expCount < numCopies; expCount++)
    {
        char posAttrName[75];
        ParticleAttribute partitionPos;
        sprintf(posAttrName, "position_p%d",expCount+1);
        if (!expandedPData->attributeInfo(posAttrName,partitionPos))
        {
            partitionPos = expandedPData->addAttribute(posAttrName,  VECTOR, 3);
            posVec.push_back(partitionPos);
        }
        if (doVelo && foundVelo)
        {
            char velAttrName[75];
            ParticleAttribute partitionVel;
            sprintf(velAttrName,"velocity_p%i",expCount+1);
            if (!expandedPData->attributeInfo(velAttrName,partitionVel))
            {
                partitionVel = expandedPData->addAttribute(velAttrName,  VECTOR, 3);
                velVec.push_back(partitionVel);
            }
        }
    }

    for (int partIndex = 0; partIndex< expandedPData->numParticles(); partIndex++)
    {
        float pos[3] = {(float)masterPositions[partIndex*3],
                        (float)masterPositions[(partIndex*3)+1],
                        (float)masterPositions[(partIndex*3)+2]
                       };

        std::vector<std::pair<ParticleIndex,float> > idDistancePairs;
        float maxDist = expandedPData->findNPoints(pos,2,1000.f,idDistancePairs);
        float neighborPos[3] = {(float)masterPositions[idDistancePairs[0].first*3],
                                (float)masterPositions[(idDistancePairs[0].first*3)+1],
                                (float)masterPositions[(idDistancePairs[0].first*3)+2]
                               };
        // to make sure that the nearest Neighbor thing is working for now..
        if (partIndex == 23)
        {
            //cout <<  "ID: " << partIndex << "->" ;
            for (int x = 0; x< idDistancePairs.size(); x++)
            {
                cout <<  idDistancePairs[x].first << "-";
            }
            //cout << endl;
            //cout << "testing: "<<  pos[0] <<  " "  << pos[1] << " "  << pos[2] << endl;
           // cout << "neighbor: "<<  neighborPos[0] <<  " "  << neighborPos[1] << " "  << neighborPos[2] << endl;
        }
        for (int expCount = 1; expCount <= numCopies; expCount++)
        {
            /*
            int z = expCount*100;
            seed (partIndex+z);
            float randomValX = Partio::partioRand(-1.0,1.0) * idDistancePairs[0].second;
            seed (partIndex+z+12);
            float randomValY = Partio::partioRand(-1.0,1.0) * idDistancePairs[0].second;
            seed (partIndex+z+123);
            float randomValZ = Partio::partioRand(-1.0,1.0) * idDistancePairs[0].second;
            */
            Vector3D jit = jitterPoint(pos,idDistancePairs[0].second,idDistancePairs[0].first,1,2,expCount);
            expandedPData->dataWrite<float>(posVec[expCount-1], partIndex)[0] = jit.x;
            expandedPData->dataWrite<float>(posVec[expCount-1], partIndex)[1] = jit.y;
            expandedPData->dataWrite<float>(posVec[expCount-1], partIndex)[2] = jit.z;

            if (velVec.size()> 0)
            {
                expandedPData->dataWrite<float>(velVec[expCount-1], partIndex)[0] = (float)masterVelocities[partIndex*3];
                expandedPData->dataWrite<float>(velVec[expCount-1], partIndex)[1] = (float)masterVelocities[(partIndex*3)+1]*-1;
                expandedPData->dataWrite<float>(velVec[expCount-1], partIndex)[2] = (float)masterVelocities[(partIndex*3)+2];
            }
        }
    }


    return expandedPData;
}


// adapted from point jitter code by Michal Fratczak  in his  Partio43delight codebase
Vector3D jitterPoint(Vector3D pos, float neighborDist, int id, float jitterStren, float maxJitter, int current_pass)
{
    if ( !jitterStren )
    {
        return pos;
    }

    float temp_p[3];
    float temp2_p[3];
    float jitter;

    if (maxJitter > 0)
    {
        jitter = jitterStren * maxJitter * smootherstep<float>(0, maxJitter, neighborDist);
    }
    else
    {
        jitter = jitterStren * neighborDist;
    }

    unsigned int random_offset = id * 123 + current_pass;
    seed(random_offset);

    temp_p[0] = (partioRand()); // 0-1
    temp_p[0] = sqrt(temp_p[0]);
    temp_p[0] *= jitter;
    temp_p[1] = 1 * M_PI * ( -.5f + partioRand() );
    temp_p[2] = 2 * M_PI * ( -.5f + partioRand() );
    SphericalCoordToCartesianCoord(temp_p, temp2_p);

    pos.x += temp2_p[0];
    pos.y += temp2_p[1];
    pos.z += temp2_p[2];
    return pos;
}
} // end Partio namespace
