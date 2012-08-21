/*
PARTIO SOFTWARE
Copyright 2010 Disney Enterprises, Inc. All rights reserved

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

#include <Partio.h>
#include <iostream>
#include <cmath>
#include <stdexcept>
#include "partiotesting.h"
#include "testkdtree.h"

namespace PartioTests {
Partio::ParticlesDataMutable* makeParticlesSimpleData()
{
    Partio::ParticlesDataMutable& foo=*Partio::create();
    Partio::ParticleAttribute positionAttr=foo.addAttribute("position",Partio::VECTOR,3);
    Partio::ParticleAttribute idAttr=foo.addAttribute("id",Partio::INT,1);

    std::cout << "Inserting points ...\n";
    for (int i = 0; i < GRIDN; i++)
        for (int j = 0; j < GRIDN; ++j)
            for (int k = 0; k < GRIDN; ++k) {
                Partio::ParticleIndex pi = foo.addParticle();
                float* pos = foo.dataWrite<float>(positionAttr, pi);
                int* id = foo.dataWrite<int>(idAttr, pi);

                pos[0] = i * 1.0f / (GRIDN - 1);
                pos[1] = j * 1.0f / (GRIDN - 1);
                pos[2] = k * 1.0f / (GRIDN - 1);
                id[0]= i * 100 + j * 10 + k;
            }
    std::cout << "Building tree ...\n";
    foo.sort();
    std::cout << "Done\n";
    return &foo;
}

void test_ParticlesSimple()
{
    std::cout << "------- Executing test_ParticlesSimple() -------" << std::endl;
    Partio::ParticlesDataMutable* foo=makeParticlesSimpleData();

    Partio::ParticleAttribute posAttr;

    // check that position attribute exists

    TESTEXPECT (foo->attributeInfo("position", posAttr));
    std::cout << "Testing lookup with stl types ...\n";
    {
        std::vector<uint64_t> indices;
        std::vector<float> dists;
        float point[3] = {0.51, 0.52, 0.53};

        foo->findNPoints(point, 5, 0.15f, indices, dists);
        TESTEXPECT (indices.size() == 5);

        const float *pos = foo->data<float>(posAttr, indices[0]);
        TESTEXPECT (pos[0] == 0.375f && pos[1] == 0.5 && pos[2] == 0.5);
        pos = foo->data<float>(posAttr, indices[1]);
        TESTEXPECT (pos[0] == 0.625 && pos[1] == 0.5 && pos[2] == 0.5);
        pos = foo->data<float>(posAttr, indices[2]);
        TESTEXPECT (pos[0] == 0.5 && pos[1] == 0.5 && pos[2] == 0.625);
        pos = foo->data<float>(posAttr, indices[3]);
        // the following tests fail ( points are retrieved in the opposite order )
        TESTEXPECT (pos[0] == 0.5 && pos[1] == 0.625 && pos[2] == 0.5);
        pos = foo->data<float>(posAttr, indices[4]);
        TESTEXPECT (pos[0] == 0.5 && pos[1] == 0.5 && pos[2] == 0.5);

        std::cout << "Test finished\n";
    }
    std::cout << "Testing lookup with pod types ...\n";
    {
        uint64_t indices[10];
        float dists[10];
        float point[3] = {0.51, 0.52, 0.53};

        float finalDist;
        int returned=foo->findNPoints(point, 5, 0.15f, indices, dists,&finalDist);
        TESTEXPECT(returned == 5);

        const float *pos = foo->data<float>(posAttr, indices[0]);
        TESTEXPECT (pos[0] == 0.375f && pos[1] == 0.5 && pos[2] == 0.5);
        pos = foo->data<float>(posAttr, indices[1]);
        TESTEXPECT (pos[0] == 0.625 && pos[1] == 0.5 && pos[2] == 0.5);
        pos = foo->data<float>(posAttr, indices[2]);
        TESTEXPECT (pos[0] == 0.5 && pos[1] == 0.5 && pos[2] == 0.625);
        // the following tests fail ( points are retrieved in the opposite order )
        pos = foo->data<float>(posAttr, indices[3]);
        TESTEXPECT (pos[0] == 0.5 && pos[1] == 0.625 && pos[2] == 0.5);
        pos = foo->data<float>(posAttr, indices[4]);
        TESTEXPECT (pos[0] == 0.5 && pos[1] == 0.5 && pos[2] == 0.5);

        std::cout << "Test finished\n";
    }
    foo->release();

}

}
