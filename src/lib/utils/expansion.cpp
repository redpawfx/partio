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
#include "3rdParty/mtrand.h"
#include "partioMath.h"


namespace Partio {
using namespace std;

ParticlesDataMutable* expandSoft(ParticlesDataMutable* expandedPData,
								 bool sort, int numCopies,
								 bool doVelo, int expandType,
								 float jitterStren,float maxJitter,
								 float advectStrength,
								 float velocityStretch,
								 int unitFPS,
								 float searchDistance
								)
{
    if (sort)
    {
        expandedPData->sort();
    }
    ParticleAttribute posAttr;
    const float* masterPositions = NULL;
    const float* masterVelocities = NULL;
	const int*  masterIds = NULL;
    bool foundVelo = false;
	float veloStretch = 0;
	pioMTRand drand;
	float FPSmult = 1.0f/float(unitFPS);

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
    ParticleAttribute idAttr;
    if (expandedPData->attributeInfo("id",idAttr) ||expandedPData->attributeInfo("particleId",idAttr) )
    {
        masterIds = expandedPData->data<int>(idAttr,0);
    }

    if (expandType == EXPAND_VELO_ADVECT)
	{
		doVelo = true;
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
            veloStretch = velocityStretch;
        }
        else
		{
			doVelo = false;
		}
    }
    for (int partIndex = 0; partIndex< expandedPData->numParticles(); partIndex++)
    {
		int id = masterIds[partIndex];
		//cout << "ID-" << id << "-" <<  partIndex << endl;
        float pos[3] = {(float)masterPositions[partIndex*3],
                        (float)masterPositions[(partIndex*3)+1],
                        (float)masterPositions[(partIndex*3)+2]
                       };


        std::vector<std::pair<ParticleIndex,float> > idDistancePairs;
		int numSamples = 10;

        float avDist = expandedPData->findNPoints(pos,numSamples,searchDistance,idDistancePairs);
		avDist = sqrt(avDist);

		cout << avDist << endl;
		
        // to make sure that the nearest Neighbor thing is working for now..
		/*
        if (partIndex == 23)
        {
            //cout <<  "ID: " << partIndex << "->" ;
            for (int x = 0; x< idDistancePairs.size(); x++)
            {
                cout <<  idDistancePairs[x].first << "-";
            }
            cout << endl;
        }
        */

		//maxJitter = clamp(maxJitter, 0.f, jitterStren);
		Vector3D thisParticleVelo;
		if ( doVelo )
		{
			thisParticleVelo= Vector3D(masterVelocities[id*3],masterVelocities[(id*3)+1], masterVelocities[(id*3)+2]);
		}
		
		for (int expCount = 1; expCount <= numCopies; expCount++)
        {
			seed(id+expCount*123);
			drand.seed(id+expCount*123);
			Vector3D jit;
			switch (expandType)
			{
				case EXPAND_RAND_STATIC_OFFSET_FAST:
				{
					jit = randStaticOffset_fast(pos,idDistancePairs[0].second,id,jitterStren,maxJitter,expCount);
					jit += partioRand() * thisParticleVelo * FPSmult * veloStretch;
					break;
				}
				case EXPAND_RAND_STATIC_OFFSET_BETTER:
				{
					jit = randStaticOffset_better(pos,idDistancePairs[0].second,id,jitterStren,maxJitter,expCount);
					jit += drand() * thisParticleVelo * FPSmult * veloStretch;
					break;
				}
				case EXPAND_JITTERPOINT_FAST:
				{
					jit = jitterPoint_fast(pos,avDist,id,jitterStren,maxJitter,expCount);
					float stretch = veloStretch * maxJitter * smootherstep<float>(0, maxJitter, avDist);
					jit += partioRand() * thisParticleVelo * FPSmult * stretch;
					break;
				}
				case EXPAND_JITTERPOINT_BETTER:
				{
					jit = jitterPoint_better(pos,avDist,id,jitterStren,maxJitter,expCount);
					float stretch = veloStretch * maxJitter * smootherstep<float>(0, maxJitter, avDist);
					jit += drand() * thisParticleVelo * FPSmult * stretch;
					break;
				}
				case EXPAND_VELO_ADVECT:
				{
					if (doVelo)
					{						
						jit = veloAdvect(	expandedPData,
											masterVelocities,
											idDistancePairs,
											pos,
											avDist,
											id,
											jitterStren,
											maxJitter,
											veloStretch,
											advectStrength,
											expCount,
											FPSmult,
											searchDistance
										);
					}
					else
					{
						jit = jitterPoint_fast(pos,avDist,id,jitterStren,maxJitter,expCount);
						float stretch = veloStretch * maxJitter * smootherstep<float>(0, maxJitter, avDist);
						jit += partioRand() * thisParticleVelo * FPSmult * stretch;
					}
					break;
				}
				default:
				{
					jit = randStaticOffset_fast(pos,idDistancePairs[0].second,id,jitterStren,maxJitter,expCount);
					jit += partioRand() * thisParticleVelo * FPSmult * veloStretch;
					break;
				}
			}

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
        idDistancePairs.clear();
    }

    return expandedPData;
}

///////////////////////////////////////////////////////
/// Uses a std random offset for each particle
Vector3D randStaticOffset_fast (Vector3D pos, float neighborDist, int id, float jitterStren, float maxJitter, int current_pass)
{
	if ( !jitterStren )
    {
        return pos;
    }

	int z = (int)current_pass*76.2340;
	seed(id+z);
	float randomValX = partioRand() * jitterStren;
	seed(id+z+12);
	float randomValY = partioRand() * jitterStren;
	seed(id+z+123);
	float randomValZ = partioRand() * jitterStren;

	pos.x += randomValX;
    pos.y += randomValY;
    pos.z += randomValZ;

	return pos;
}

///////////////////////////////////////////////////////////////
/// Uses a mersenne twister random offset for each particle
Vector3D randStaticOffset_better (Vector3D pos, float neighborDist, int id, float jitterStren, float maxJitter, int current_pass)
{
	if ( !jitterStren )
    {
        return pos;
    }
	pioMTRand drand;

	int z = current_pass*76.2340;
	drand.seed(id+z);
	float randomValX = drand() * jitterStren;
	drand.seed(id+z+12);
	float randomValY = drand() * jitterStren;
	drand.seed(id+z+123);
	float randomValZ = drand() * jitterStren;

	pos.x += randomValX;
    pos.y += randomValY;
    pos.z += randomValZ;

	return pos;
}

////////////////////////////////////////////////////////////////////////////////////////
/// adapted from point jitter code by Michal Fratczak  in his  Partio43delight codebase
Vector3D jitterPoint_fast(Vector3D pos, float neighborDist, int id, float jitterStren, float maxJitter, int current_pass)
{
    if ( !jitterStren )
    {
        return pos;
    }

    Vector3D temp_p;
    Vector3D temp2_p;
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

    temp_p.x = ((partioRand()*.5)+.5); // 0-1
    temp_p.x = sqrt(temp_p.x);
    temp_p.x *= jitter;
    temp_p.y = 1 * M_PI * ( -.5f + partioRand() );
    temp_p.z = 2 * M_PI * ( -.5f + partioRand() );
    SphericalCoordToCartesianCoord(temp_p, temp2_p);

    pos.x += temp2_p.x;
    pos.y += temp2_p.y;
    pos.z += temp2_p.z;
    return pos;
}

/////////////////////////////////////////////////////////////////////////////////////////
/// adapted from point jitter code by Michal Fratczak  in his  Partio43delight codebase
Vector3D jitterPoint_better(Vector3D pos, float neighborDist, int id, float jitterStren, float maxJitter, int current_pass)
{
    if ( !jitterStren )
    {
        return pos;
    }

    Vector3D temp_p;
    Vector3D temp2_p;
    float jitter;
	pioMTRand drand;

    if (maxJitter > 0)
    {
        jitter = jitterStren * maxJitter * smootherstep<float>(0, maxJitter, neighborDist);
    }
    else
    {
        jitter = jitterStren * neighborDist;
    }

    unsigned int random_offset = id * 123 + current_pass;
    drand.seed(random_offset);

    temp_p.x = ((drand()*.5)+.5); // (0,1)
    temp_p.x = sqrt(temp_p.x);
    temp_p.x *= jitter;
    temp_p.y = 1 * M_PI * ( -.5f + ((drand()*.5)+.5) );
    temp_p.z = 2 * M_PI * ( -.5f + ((drand()*.5)+.5) );
    SphericalCoordToCartesianCoord(temp_p, temp2_p);

    pos.x += temp2_p.x;
    pos.y += temp2_p.y;
    pos.z += temp2_p.z;
    return pos;
}


//////////////////////////////////////////////////////////////////////////////
///  velocity advection of  expanded particles
Vector3D  veloAdvect(ParticlesDataMutable* pData, const float* masterVelocities,
					 std::vector<std::pair<ParticleIndex,float> > idDistancePairs,
					 Vector3D pos, float neighborDist,
					 int id, float jitterStren,
					 float maxJitter, float veloStretch, 
					 float advectStrength, int current_pass,
					 float FPSMult, float searchDistance)
{
	Vector3D thisParticleVelo = Vector3D(masterVelocities[id*3],masterVelocities[(id*3)+1], masterVelocities[(id*3)+2]);

	Vector3D jitterPoint = jitterPoint_fast(pos, neighborDist, id, jitterStren, maxJitter, current_pass);

	seed(id+current_pass*123);
	float stretch = veloStretch * maxJitter * smootherstep<float>(0, maxJitter, neighborDist);
	jitterPoint += partioRand() * thisParticleVelo * FPSMult * stretch;
	
	float temp[3];
	temp[0] = jitterPoint.x;
	temp[1] = jitterPoint.y;
	temp[2] = jitterPoint.z;

	std::vector<std::pair<ParticleIndex,float> > expIDdistancePair;
	int numSamples = 10;
    float avDist = pData->findNPoints(temp,numSamples,searchDistance,expIDdistancePair);

	Vector3D avVeloDir;
	float avSpeed = 0.f;

	//cout << "particle " << id << "->" ;
	
	// TODO  possibly work out some simple weight multing here, closest velo affects more 
	for (int x = 0; x< expIDdistancePair.size(); x++)
	{
		//cout << idDistancePair[x].first << "-";
		Vector3D surroundingVelo = Vector3D(masterVelocities[expIDdistancePair[x].first*3],
											masterVelocities[(expIDdistancePair[x].first*3)+1],
											masterVelocities[(expIDdistancePair[x].first*3)+2]);
		avSpeed += surroundingVelo.length();
		surroundingVelo.normalize();
		avVeloDir += surroundingVelo;
	}

	//cout << endl;
	avVeloDir.normalize();
	//cout << avVeloDir << endl;
	avSpeed /= expIDdistancePair.size();

	avVeloDir *= avSpeed * advectStrength ;

	Vector3D newSteeringVelo = avVeloDir - (thisParticleVelo * advectStrength);

	Vector3D newParticleVelo = thisParticleVelo + newSteeringVelo;

	//seed(id);
	//avVeloDir *= thisParticleVelo.length();// * ((partioRand()*.5)+.5);

	jitterPoint +=  (newParticleVelo - thisParticleVelo) * FPSMult;

	return jitterPoint;
}


} // end Partio namespace
