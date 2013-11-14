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

#define _USE_MGL_FT_

#include "partio4MayaShared.h"

#define TABLE_SIZE 256


using namespace Partio;
using namespace std;

static inline int fround(double n, int d)
{
    return int(floor(n * pow(double(10), d) + .5));
}

//////////////////////////////////
MVector partio4Maya::jitterPoint(int id, float freq, float offset, float jitterMag)
///* generate a constant noise offset for this  ID
//  and return as a vector to add to the particle position
//
{
    MVector jitter(0,0,0);
    if (jitterMag > 0)
    {
        jitter.x = ((noiseAtValue(float((id+.124+offset))*freq))-.5)*2;
        jitter.y = ((noiseAtValue(float((id+1042321+offset))*freq))-.5)*2;
        jitter.z = ((noiseAtValue(float((id-2350212+offset))*freq))-.5)*2;

        jitter*= jitterMag;
    }

    return  jitter;
}

//////////////////////////////////////////////////
bool partio4Maya::partioCacheExists(const char* fileName)
{

    struct stat fileInfo;
    bool statReturn;
    int intStat;

    intStat = stat(fileName, &fileInfo);
    if (intStat == 0)
    {
        statReturn = true;
    }
    else
    {
        statReturn = false;
    }

    return(statReturn);

}

//////////////////////////////////
void partio4Maya::getFrameAndSubframe(double t, int &frame, int &subframe, int subFramePadding)
{
    frame = int(floor(t));
    if (subFramePadding > 0)
    {
        subframe = fround(t - frame, subFramePadding);
    }
    else
    {
        subframe = 0;
    }
}

/////////////////////////////////
bool partio4Maya::findCacheFile(partio4Maya::CacheFiles &files,
                                partio4Maya::FindMode mode,
                                MTime t,
                                partio4Maya::CacheFiles::iterator &fit)
{
    partio4Maya::CacheFiles::iterator fit1, fit2;

    fit = files.end();
    fit1 = files.find(t);

    if (fit1 != files.end())
    {
        fit = fit1;
    }
    else if (mode != FM_EXACT)
    {
        fit1 = files.upper_bound(t);
        if (mode == FM_NEXT)
        {
            fit = fit1;
        }
        else
        {
            fit2 = fit1;
            if (fit2 == files.begin())
            {
                fit2 = files.end();
            }
            else
            {
                fit2--;
            }
            if (mode == FM_PREV)
            {
                fit = fit2;
            }
            else
            {
                if (fit1 == files.end())
                {
                    fit = fit2;
                }
                else if (fit2 == files.end())
                {
                    fit = fit1;
                }
                else
                {
                    double dt1 = fabs(fit1->first.value() - t.value());
                    double dt2 = fabs(fit2->first.value() - t.value());
                    fit = (dt1 < dt2 ? fit1 : fit2);
                }
            }
        }
    }

    return (fit != files.end());
}

bool partio4Maya::findCacheFile(const partio4Maya::CacheFiles &files,
                                partio4Maya::FindMode mode,
                                MTime t,
                                partio4Maya::CacheFiles::const_iterator &fit)
{
    partio4Maya::CacheFiles::iterator it;
    if (findCacheFile((partio4Maya::CacheFiles&)files, mode, t, it))
    {
        fit = it;
        return true;
    }
    else
    {
        fit = files.end();
        return false;
    }
}

/////////////////////////////////
#include <regex.h>

class FramePatterns
{
public:

    static regex_t Pattern1;
    static regex_t Pattern2;
    static regex_t Pattern3;

    FramePatterns()
    {
        regcomp(&Pattern1, "[.]([0-9]+)([.]([0-9]+))?$", REG_EXTENDED);
        regcomp(&Pattern2, "_([0-9]+)(_([0-9]+))?$", REG_EXTENDED);
        regcomp(&Pattern3, "Frame([0-9]+)(Tick([0-9]+))?$", REG_EXTENDED);
    }

    ~FramePatterns()
    {
        regfree(&Pattern1);
        regfree(&Pattern2);
        regfree(&Pattern3);
    }
};

int regsearch(regex_t *re, const char *str, size_t nmatch, regmatch_t *pmatch, int flags)
{
    const char *c = str;
    int rv = -1;

    while (*c != '\0')
    {
        rv = regexec(re, c, nmatch, pmatch, flags);
        if (rv == 0)
        {
            break;
        }
        ++c;
    }

    return rv;
}

regex_t FramePatterns::Pattern1;
regex_t FramePatterns::Pattern2;
regex_t FramePatterns::Pattern3;

static FramePatterns _framePatterns;

// ---

bool partio4Maya::identifyPath(const MString &path, MString &dirname, MString &basename, MString &frame, MTime &t, MString &ext)
{
    int idx;

#ifdef _WIN32
    int idx0 = path.rindexW('\\');
    int idx1 = path.rindexW('/');
    idx = (idx0 > idx1 ? idx0 : idx1);
#else
    idx = path.rindexW('/');
#endif

    if (idx == -1)
    {
        dirname = "";
        basename = path;
    }
    else
    {
        dirname = path.substringW(0, idx-1);
        basename = path.substringW(idx+1, path.length()-1);
    }

    idx = basename.rindexW('.');
    if (idx == -1)
    {
        ext = "";
    }
    else
    {
        ext = basename.substringW(idx+1, basename.length()-1);
        basename = basename.substringW(0, idx-1);
    }

    // Now identify frame pattern in basename, and extract frame
    regmatch_t m[8];


    // .rm.so/.rm_eo (== -> empty string, -1 -> no match)
    if (regsearch(&FramePatterns::Pattern1, basename.asChar(), 8, m, 0) == 0 ||
        regsearch(&FramePatterns::Pattern2, basename.asChar(), 8, m, 0) == 0)
    {
        frame = basename.substringW(m[0].rm_so+1, basename.length()-1);
        basename = basename.substringW(0, m[0].rm_so);
#ifdef _DEBUG
        MGlobal::displayInfo("partio4Maya::identifyPath: Frame pattern 1 or 2 (" + frame + ")");
#endif
        int idx = frame.rindexW('_');
        if (idx != -1)
        {
            //frame[idx] = '.';
            MString tmp = frame;
            frame = tmp.substringW(0, idx-1);
            frame += ".";
            frame += tmp.substringW(idx+1, tmp.length()-1);
        }
        double tval = 0;
        sscanf(frame.asChar(), "%lf", &tval);
        if (ext == "pdc")
        {
            tval *= MTime(1.0, MTime::k6000FPS).asUnits(MTime::uiUnit());
        }
        t.setValue(tval);
    }
    else if (regsearch(&FramePatterns::Pattern3, basename.asChar(), 8, m, 0) == 0)
    {
        frame = basename.substringW(m[0].rm_so+1, basename.length()-1);
        basename = basename.substringW(0, m[0].rm_so);
#ifdef _DEBUG
        MGlobal::displayInfo("partio4Maya::identifyPath: Frame pattern 3 (" + frame + ")");
#endif
        double tval = 0;
        int iframe = 0, iticks = 0;
        sscanf(frame.asChar(), "Frame%dTick%d", &iframe, &iticks);
        tval = double(iframe) + iticks * MTime(1.0, MTime::k6000FPS).asUnits(MTime::uiUnit());
        t.setValue(tval);

    }
    else
    {
        frame = "";
    }

#ifdef _DEBUG
    MGlobal::displayInfo("partio4Maya::identifyPath: \"" + path + "\" -> dirname=" + dirname + ", basename=" + basename + ", frame=" + frame + ", ext=" + ext + ", time=" + t.value());
#endif

    return (frame.length() > 0);
}

unsigned long partio4Maya::getFileList(const MString &path, partio4Maya::CacheFiles &files)
{
    MString dirname, basename, frame, ext;
    MTime t;

    identifyPath(path, dirname, basename, frame, t, ext);
    return getFileList(dirname, basename, ext, files);
}

unsigned long partio4Maya::getFileList(const MString &dirname, const MString &basename, const MString &ext, partio4Maya::CacheFiles &files)
{
    MStringArray tmp;

    std::string dn = dirname.asChar();
    if (dn.length() > 0)
    {
        char lc = dn[dn.length()-1];
#ifdef _WIN32
        if (lc != '\\' && lc != '/')
#else
        if (lc != '/')
#endif
        {
            dn.push_back('/');
        }
    }
    else
    {
        dn = "./";
    }

    MString mdn = dn.c_str();

    MGlobal::executeCommand("getFileList -folder \"" + mdn + "\" -filespec \"" + basename + "*." + ext + "\"", tmp);

    files.clear();

    MString _dn, _bn, _fn, _e;
    MTime t;

    for (unsigned long i=0; i<tmp.length(); ++i)
    {
        if (identifyPath(tmp[i], _dn, _bn, _fn, t, _e))
        {
            files[t] = mdn + tmp[i];
        }
    }

    return (unsigned long) files.size();
}


///////////////////////////////////////////
/// C++ version of the same mel procedure
MStringArray partio4Maya::partioGetBaseFileName(MString inFileName)
{
    MString preDelim = "";
    MString postDelim =  "";
    MString ext = "";
    MString padding = "";
    MString origFrameString = "";

    MStringArray outFileName;
    outFileName.append(inFileName);
    outFileName.append(preDelim);
    outFileName.append(postDelim);
    outFileName.append(ext);
    outFileName.append(padding);
    outFileName.append(origFrameString);

    ///////////////////////////////////////////////////////////
    /// first we strip off and determine the file extension
    int l = inFileName.length();
    bool foundExt = true;

    MString breakdownFileName = inFileName;

    const char* c = breakdownFileName.asChar();
    int end = breakdownFileName.length()-1;
    while (foundExt)
    {
        if (isalpha(c[end]))
        {
            end--;
        }
        else
        {
            foundExt = false;
            ext = breakdownFileName.substringW(end+1, (breakdownFileName.length()-1) );
            breakdownFileName = breakdownFileName.substringW(0,end);
        }
    }

    if (ext.length() > 0)
    {
        outFileName[3] = ext;
        //if (ext == "pts" || ext == "xyz") // special case for static lidar files
        //{
        //	return outFileName;
        //}
        outFileName[0] = breakdownFileName;
    }
    else
    {
        return outFileName;
    }

    //////////////////////////////////////////////////////////////
    /// then we  determine the postDelim character (only support  "." or  "_"

    l = breakdownFileName.length()-1;
    MString last =  breakdownFileName.substringW(l,l);

    if ( last == "_" || last == ".")
    {
        outFileName[2] = last;
        breakdownFileName = breakdownFileName.substringW(0,(l-1));
        outFileName[0] = breakdownFileName;
    }
    else
    {
        return outFileName;
    }

    /////////////////////////////////////////////////////////////
    /// now lets  get the frame numbers to determine padding

    bool foundNum = true;
    const char* f = breakdownFileName.asChar();
    end = breakdownFileName.length()-1;

    while (foundNum)
    {
        if (isdigit(f[end]))
        {
            end--;
            padding += "#";
        }
        else
        {
            foundNum = false;
            origFrameString = breakdownFileName.substringW(end+1,(breakdownFileName.length()-1));
            breakdownFileName = breakdownFileName.substringW(0,end);
        }
    }

    if (padding.length() > 0)
    {
        outFileName[4] = padding;
        outFileName[0] = breakdownFileName;
        outFileName[5] = origFrameString;
    }
    else
    {
        outFileName[4] = "-1";
        return outFileName;
    }

    /////////////////////////////////////////////////////////////
    ///  lastly  we  get the  preDelim character, again only supporting "." or "_"

    l = breakdownFileName.length()-1;
    last =  breakdownFileName.substringW(l,l);


    if ( last == "_" || last == ".")
    {
        outFileName[1] = last;
        breakdownFileName = breakdownFileName.substringW(0,(l-1));
        outFileName[0] = breakdownFileName;
    }
    else
    {
        return outFileName;
    }

    /////////////////////////////////////////////////////////////////////////
    /// if we've gotten here, we have a fully populated outputFileNameArray

    return outFileName;

}




void partio4Maya::updateFileName (MString cacheFile, MString cacheDir,
                                  bool cacheStatic, int cacheOffset,
                                  short cacheFormat, int integerTime, int byFrame,
                                  int &cachePadding, MString &formatExt,
                                  MString &outputFramePath, MString &outputRenderPath
                                 )
{
    formatExt = setExt(cacheFormat);
	int fps = (float)(MTime(1.0, MTime::kSeconds).asUnits(MTime::uiUnit()));

    MStringArray fileParts = partioGetBaseFileName(cacheFile);

    MString cachePrefix = fileParts[0];
    MString preDelim = fileParts[1];
    MString postDelim = fileParts[2];

	// future proofing..  if number of formats changes, this  entry will change, and could fail and crash
	// so if the formats don't jive take the one from the filename.
	if (formatExt != fileParts[3])
	{
		formatExt = fileParts[3];
	}

    cachePadding = fileParts[4].length();
    MString origFrameString = fileParts[5];

    bool tempFix = false;

    int cacheFrame;
    MString  newCacheFile;
    MString  renderCacheFile;

///////////////////////////////////////////////
///  output path  as normal

	cacheFrame =  (integerTime  + cacheOffset) * byFrame;

    MString formatString =  "%s%s%s%0";
    // special case for PDCs and maya nCache files because of the funky naming convention  TODO: support substepped/retiming  caches
    if (formatExt == "pdc")
    {
        cacheFrame *= (int)(6000 / fps);
        cachePadding = 1;
    }

    else if (formatExt == "mc")
    {
        cachePadding = 1;
        formatString = "%s%sFrame%0";
        int idx = cacheFile.rindexW("Frame");

        if (idx != -1)
        {
            cacheFile = cacheFile.substringW(0, idx-1);
        }
    }

    char fileName[512] = "";

    formatString += cachePadding;
    formatString += "d%s%s";

    const char* fmt = formatString.asChar();

    if (cacheStatic)
    {
        stringstream s_str;
        s_str << origFrameString.asChar();
        s_str >> cacheFrame;
    }

    sprintf(fileName, fmt, cacheDir.asChar(), cachePrefix.asChar(), preDelim.asChar(), cacheFrame, postDelim.asChar(), formatExt.asChar());

    newCacheFile = fileName;


///////////////////////////////////////////
/// output path for render output path


    MString frameString = "<frame>";
    if (cacheStatic)
    {
        frameString = origFrameString;
    }


    formatString =  "%s%s%s%s%s%s";
    char rfileName[512] = "";
    const char* rfmt = formatString.asChar();
    sprintf(rfileName, rfmt, cacheDir.asChar(), cachePrefix.asChar(), preDelim.asChar(), frameString.asChar(), postDelim.asChar(), formatExt.asChar());
    renderCacheFile = rfileName;


    outputFramePath =  newCacheFile;
    outputRenderPath = renderCacheFile;
}
////////////////////////////////////////////////////////////////
MString partio4Maya::setExt(short extEnum,bool write)
{
    const char *ext = (write ? Partio::writeFormatExtension(size_t(extEnum)) : Partio::readFormatExtension(size_t(extEnum)));
    return MString(ext ? ext : "");
}

///////////////////////////////////////////////////////////
// eventually this will be replaced with something from partio directly
void partio4Maya::buildSupportedExtensionList(std::map<short,MString> &formatExtMap,bool write)
{
    if (write)
    {
        for (size_t i=0; i<Partio::numWriteFormats(); ++i)
        {
            formatExtMap[short(i)] = Partio::writeFormatExtension(i);
        }
    }
    else
    {
        for (size_t i=0; i<Partio::numReadFormats(); ++i)
        {
            formatExtMap[short(i)] = Partio::readFormatExtension(i);
        }
    }
}


/////////////////////////////////////////
void partio4Maya::drawPartioLogo(float multiplier)
{
    glBegin ( GL_LINES );

    int i,d;

    int last = P1Count - 1;
    for ( i = 0; i < last; ++i )
    {
        glVertex3f ( P1[i][0] * multiplier,
                     P1[i][1] * multiplier,
                     P1[i][2] * multiplier );
        glVertex3f ( P1[i+1][0] * multiplier,
                     P1[i+1][1] * multiplier,
                     P1[i+1][2] * multiplier );
    }
    last = P2Count -1;
    for ( i = 0; i < last; ++i )
    {
        glVertex3f ( P2[i][0] * multiplier,
                     P2[i][1] * multiplier,
                     P2[i][2] * multiplier );
        glVertex3f ( P2[i+1][0] * multiplier,
                     P2[i+1][1] * multiplier,
                     P2[i+1][2] * multiplier );
    }
    last = a1Count -1;
    for ( i = 0; i < last; ++i )
    {
        glVertex3f ( a1[i][0] * multiplier,
                     a1[i][1] * multiplier,
                     a1[i][2] * multiplier );
        glVertex3f ( a1[i+1][0] * multiplier,
                     a1[i+1][1] * multiplier,
                     a1[i+1][2] * multiplier );
    }
    last = a2Count -1;
    for ( i = 0; i < last; ++i )
    {
        glVertex3f ( a2[i][0] * multiplier,
                     a2[i][1] * multiplier,
                     a2[i][2] * multiplier );
        glVertex3f ( a2[i+1][0] * multiplier,
                     a2[i+1][1] * multiplier,
                     a2[i+1][2] * multiplier );
    }
    last = rCount -1;
    for ( i = 0; i < last; ++i )
    {
        glVertex3f ( r[i][0] * multiplier,
                     r[i][1] * multiplier,
                     r[i][2] * multiplier );
        glVertex3f ( r[i+1][0] * multiplier,
                     r[i+1][1] * multiplier,
                     r[i+1][2] * multiplier );
    }
    last = tCount -1;
    for ( i = 0; i < last; ++i )
    {
        glVertex3f ( t[i][0] * multiplier,
                     t[i][1] * multiplier,
                     t[i][2] * multiplier );
        glVertex3f ( t[i+1][0] * multiplier,
                     t[i+1][1] * multiplier,
                     t[i+1][2] * multiplier );
    }
    last = i1Count -1;
    for ( i = 0; i < last; ++i )
    {
        glVertex3f ( i1[i][0] * multiplier,
                     i1[i][1] * multiplier,
                     i1[i][2] * multiplier );
        glVertex3f ( i1[i+1][0] * multiplier,
                     i1[i+1][1] * multiplier,
                     i1[i+1][2] * multiplier );
    }
    last = i2Count -1;
    for ( i = 0; i < last; ++i )
    {
        glVertex3f ( i2[i][0] * multiplier,
                     i2[i][1] * multiplier,
                     i2[i][2] * multiplier );
        glVertex3f ( i2[i+1][0] * multiplier,
                     i2[i+1][1] * multiplier,
                     i2[i+1][2] * multiplier );
    }
    last = o1Count -1;
    for ( i = 0; i < last; ++i )
    {
        glVertex3f ( o1[i][0] * multiplier,
                     o1[i][1] * multiplier,
                     o1[i][2] * multiplier );
        glVertex3f ( o1[i+1][0] * multiplier,
                     o1[i+1][1] * multiplier,
                     o1[i+1][2] * multiplier );
    }
    last = o2Count -1;
    for ( i = 0; i < last; ++i )
    {
        glVertex3f ( o2[i][0] * multiplier,
                     o2[i][1] * multiplier,
                     o2[i][2] * multiplier );
        glVertex3f ( o2[i+1][0] * multiplier,
                     o2[i+1][1] * multiplier,
                     o2[i+1][2] * multiplier );
    }

    for ( d = 0; d < debrisCount; d++ )
    {
        for ( i = 0; i < ( debrisPointCount-1 ); ++i )
        {
            glVertex3f ( circles[d][i][0] * multiplier,
                         circles[d][i][1] * multiplier,
                         circles[d][i][2] * multiplier );
            glVertex3f ( circles[d][i+1][0] * multiplier,
                         circles[d][i+1][1] * multiplier,
                         circles[d][i+1][2] * multiplier );
        }
    }
    glEnd();

}

////////////////////////////////////////////////////
/// NOISE FOR JITTER!

const int kTableMask = TABLE_SIZE - 1;

float partio4Maya::noiseAtValue( float x )
//
//  Description:
//      Get the Noise value at the given point in 1-space and time
//
//  Arguments:
//      x - the point at which to calculate the lxgNoise
//
//  Return Value:
//      the Noise value at the point
//
{
    int ix;
    float fx;

    if ( !isInitialized )
    {
        initTable( 23479015 );
        isInitialized = 1;
    }

    ix = (int)floorf( x );
    fx = x - (float)ix;

    return spline( fx, value( ix - 1 ), value( ix ), value( ix + 1 ), value( ix + 2 ) );
}


void  partio4Maya::initTable( long seed )
//
//  Description:
//      Initialize the table of random values with the given seed.
//
//  Arguments:
//      seed - the new seed value
//
{

#ifndef drand48
#define drand48() (rand()*(1.0/RAND_MAX))
#endif

#ifdef WIN32
    srand( seed );
#else
    srand48( seed );
#endif

    for ( int i = 0; i < TABLE_SIZE; i++ )
    {
        valueTable1[i] = (float)drand48();
        valueTable2[i] = (float)drand48();
        valueTable3[i] = (float)drand48();
    }
    isInitialized = 1;
}


float partio4Maya::spline( float x, float knot0, float knot1, float knot2, float knot3 )
//
//  Description:
//      This is a simple version of a Catmull-Rom spline interpolation.
//
//  Assumptions:
//
//      0 < x < 1
//
//
{
    float c0, c1, c2, c3;

    // Evaluate span of cubic at x using Horner's rule
    //
    c3 = (-0.5F * knot0 ) + ( 1.5F * knot1 ) + (-1.5F * knot2 ) + ( 0.5F * knot3 );
    c2 = ( 1.0F * knot0 ) + (-2.5F * knot1 ) + ( 2.0F * knot2 ) + (-0.5F * knot3 );
    c1 = (-0.5F * knot0 ) + ( 0.0F * knot1 ) + ( 0.5F * knot2 ) + ( 0.0F * knot3 );
    c0 = ( 0.0F * knot0 ) + ( 1.0F * knot1 ) + ( 0.0F * knot2 ) + ( 0.0F * knot3 );

    return ( ( c3 * x + c2 ) * x + c1 ) * x + c0;;
}

int partio4Maya::isInitialized = 0;

int partio4Maya::permtable[256] =
{
    254,    91,     242,    186,    90,     204,    85,     133,    233,
    50,     187,    49,     182,    224,    144,    166,    7,      51,
    20,     179,    36,     203,    114,    156,    195,    40,     24,
    60,     162,    84,     126,    102,    63,     194,    220,    161,
    72,     94,     193,    229,    140,    57,     3,      189,    106,
    54,     164,    198,    199,    44,     245,    235,    100,    87,
    25,     41,     62,     111,    13,     70,     27,     82,     69,
    53,     66,     247,    124,    67,     163,    125,    155,    228,
    122,    19,     113,    143,    121,    9,      1,      241,    171,
    200,    83,     244,    185,    170,    141,    115,    190,    154,
    48,     32,     178,    127,    167,    56,     134,    15,     160,
    238,    64,     6,      11,     196,    232,    26,     89,     0,
    219,    112,    68,     30,     215,    227,    75,     132,    71,
    239,    251,    92,     14,     104,    231,    29,     180,    150,
    226,    191,    47,     73,     37,     183,    88,     105,    42,
    22,     2,      38,     5,      119,    74,     249,    184,    52,
    8,      55,     118,    255,    206,    173,    165,    78,     31,
    123,    98,     212,    80,     139,    61,     138,    77,     177,
    45,     137,    145,    28,     168,    128,    95,     223,    35,
    205,    76,     211,    175,    81,     33,     207,    21,     131,
    58,     152,    16,     240,    18,     96,     210,    109,    214,
    216,    202,    148,    34,     146,    117,    176,    93,     246,
    172,    97,     159,    197,    218,    65,     147,    253,    221,
    217,    79,     101,    142,    23,     149,    99,     39,     12,
    135,    110,    234,    108,    153,    129,    4,      169,    174,
    116,    243,    130,    107,    222,    10,     43,     188,    46,
    213,    252,    86,     157,    192,    236,    158,    120,    17,
    103,    248,    225,    230,    250,    208,    181,    151,    237,
    201,    59,     136,    209
};

float partio4Maya::valueTable1[256];
float partio4Maya::valueTable2[256];
float partio4Maya::valueTable3[256];


