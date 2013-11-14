/* partio4Maya  3/12/2012, John Cassella  http://luma-pictures.com and  http://redpawfx.com
PARTIO Export
Copyright 2013 (c)  All rights reserved

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.

Disclaimer: THIS SOFTWARE IS PROVIDED BY  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE, NONINFRINGEMENT AND TITLE ARE DISCLAIMED.
IN NO EVENT SHALL  THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND BASED ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
*/

#ifndef PARTIO4MAYASHARED
#define PARTIO4MAYASHARED
#pragma once


#ifdef WIN32
#define _WINSOCKAPI_
#include <windows.h>
#endif

#ifdef OSMac_MachO_
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#elif  WIN32
#include <gl/GLU.h>
#include <gl/GL.h>
#include <GL/glext.h>
#else
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#endif

#include <sys/stat.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <map>
#include <sstream>

#include <maya/MString.h>
#include <maya/MDataBlock.h>
#include <maya/MVector.h>
#include <maya/MGlobal.h>
#include <maya/MStringArray.h>
#include <maya/MTimeArray.h>
#include <maya/MTime.h>

#include "iconArrays.h"
#include "Partio.h"

extern const int kTableMask;
#define MODPERM(x) permtable[(x)&kTableMask]

// Indices in filename breakdown array
#define BD_FILENAME 0
#define BD_PREDELIM 1
#define BD_POSTDELIM 2
#define BD_EXT 3
#define BD_FRAMEPAD 4
#define BD_SFRAMEPAD 5
#define BD_ORGFRAME 6

class partio4Maya
{
public:

    typedef std::map<MTime, MString> CacheFiles;

    static bool 	partioCacheExists(const char* fileName);
    static MStringArray partioGetBaseFileName(MString inFileName);
    static void 	updateFileName (MString cacheFile, MString cacheDir,
                                 bool cacheStatic, int cacheOffset,
                                 short cacheFormat, int integerTime, int byFrame,
                                 int &cachePadding, MString &formatExt,
                                 MString &outputFramePath, MString &outputRenderPath);

    static MString 	setExt(short extNum,bool write=false);
    static void 	buildSupportedExtensionList(std::map<short,MString> &formatExtMap,bool write=false);
    static void 	drawPartioLogo(float multiplier);
    static MVector 	jitterPoint(int id, float freq, float offset, float jitterMag);
    static float  	noiseAtValue( float x);
    static void   	initTable( long seed );

    static bool identifyPath(const MString &path, MString &dirname, MString &basename, MString &frame, MTime &t, MString &ext);
    static unsigned long getFileList(const MString &path, CacheFiles &files);
    static unsigned long getFileList(const MString &dirname, const MString &basename, const MString &ext, CacheFiles &files);
    static void getFrameAndSubframe(double t, int &frame, int &subframe, int subFramePadding=3);

    enum FindMode
    {
        FM_EXACT = 0,
        FM_CLOSEST,
        FM_PREV,
        FM_NEXT
    };
    static bool findCacheFile(const CacheFiles &files, FindMode mode, MTime t,
                              CacheFiles::const_iterator &it);
    static bool findCacheFile(CacheFiles &files, FindMode mode, MTime t,
                              CacheFiles::iterator &it);

private:

    static int    	permtable   [256];
    static float  	valueTable1 [256];
    static float  	valueTable2 [256];
    static float  	valueTable3 [256];
    static int    	isInitialized;
    static float  	spline( float x, float knot0, float knot1, float knot2, float knot3 );
    static float  	value( int x, float table[] = valueTable1 );
};

/// INLINES
/// gets the value of the permtable at x and &'s it with the provided table mask
inline float partio4Maya::value( int x, float table[] )
{
    return table[MODPERM( x )];
}

#endif
