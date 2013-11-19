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
#include "expansion.h"

namespace Partio{
using namespace std;

ParticlesDataMutable* expand(ParticlesDataMutable* expandedPData, bool sort, int numCopies)
{

	if(sort)
	{
		expandedPData->sort();
	}
	ParticleAttribute posAttr;
	const float* masterPositions = NULL;
	const float* masterVelocities = NULL;
	if(expandedPData->attributeInfo("position",posAttr))
	{
		masterPositions = expandedPData->data<float>(posAttr,0);
	}
	ParticleAttribute velAttr;
	if (expandedPData->attributeInfo("velocity",velAttr))
	{
		masterVelocities = expandedPData->data<float>(velAttr,0);
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
		char velAttrName[75];
		ParticleAttribute partitionVel;
		sprintf(velAttrName,"velocity_p%d",expCount);
		if (!expandedPData->attributeInfo(velAttrName,partitionVel))
		{
			sprintf(velAttrName,"velocity_p:%i",expCount+1);
			partitionVel = expandedPData->addAttribute(velAttrName,  VECTOR, 3);
			velVec.push_back(partitionVel);
		}
	}

	for (int partIndex = 0; partIndex< expandedPData->numParticles(); partIndex++)
	{
		for (int expCount = 0; expCount < numCopies; expCount++)
		{
			expandedPData->dataWrite<float>(posVec[expCount], partIndex)[0] = (float)masterPositions[partIndex*3];
			expandedPData->dataWrite<float>(posVec[expCount], partIndex)[1] = (float)masterPositions[(partIndex*3)+1]-expCount;
			expandedPData->dataWrite<float>(posVec[expCount], partIndex)[2] = (float)masterPositions[(partIndex*3)+2];

			expandedPData->dataWrite<float>(velVec[expCount], partIndex)[0] = (float)masterVelocities[partIndex*3];
			expandedPData->dataWrite<float>(velVec[expCount], partIndex)[1] = (float)masterVelocities[(partIndex*3)+1]*-1;
			expandedPData->dataWrite<float>(velVec[expCount], partIndex)[2] = (float)masterVelocities[(partIndex*3)+2];
		}
	}

	return expandedPData;
}


} // end Partio namespace
