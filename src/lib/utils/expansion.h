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

#include <vector>
#include <stdlib.h>

#include "../Partio.h"
#include "vector3d.h"

#define  VFPS  .041666666667

enum expandType {
	EXPAND_RAND_STATIC_OFFSET_FAST = 0,
	EXPAND_RAND_STATIC_OFFSET_BETTER,
	EXPAND_JITTERPOINT_FAST,
	EXPAND_JITTERPOINT_BETTER,
	EXPAND_VELO_ADVECT,
	EXPAND_INFILLING
};

namespace Partio{

	// expandSoft adds n number of extra position and velo channels to existing cache
	// it does not increase the particle count
	ParticlesDataMutable*  expandSoft(ParticlesDataMutable* pData,
									  bool sort, int numCopies,
								   bool doVelo, int expandType,
								   float jitterStren, float maxJitter, float advectStrength);

	//! uses stdlib::rand
	//! creates N particles around the master particle with a random static offset
	Vector3D randStaticOffset_fast(Vector3D pos, float neighborDist, int id, float jitterStren, float maxJitter, int current_pass);

	//! uses stdlib::rand
	//! creates N particles around the master particle with a dynamic averaged  distance offset based on current neighbor distance
	Vector3D jitterPoint_fast(Vector3D pos, float neighborDist, int id, float jitterStren, float maxJitter, int current_pass);

	//! uses mersenne twister random (slower but better distrib)
	//! creates N particles around the master particle with a random static offset
	Vector3D randStaticOffset_better(Vector3D pos, float neighborDist, int id, float jitterStren, float maxJitter, int current_pass);

	//! uses mersenne twister random (slower but better distrib)
	//! creates N particles around the master particle with a dynamic averaged  distance offset based on current neighbor distance
	Vector3D jitterPoint_better(Vector3D pos, float neighborDist, int id, float jitterStren, float maxJitter, int current_pass);

	//! creates particles with a random static (stdlib:rand) offset around the last position of the master particles and then gets the average
	//! velocity from the surrounding N particles and repositions it by that value
	Vector3D veloAdvect(ParticlesDataMutable* pData, const float* masterVelocities,
						std::vector<std::pair<ParticleIndex,float> > idDistancePairs,
						Vector3D pos, float neighborDist,
						int id, float jitterStren,
						float maxJitter, float advectStrength, int current_pass);

} // end Partio namespace
