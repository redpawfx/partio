/* partio4Maya
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

#ifndef Partio4MayaCache_H
#define Partio4MayaCache_H

#include <maya/MPxCacheFormat.h>
#include <maya/MIntArray.h>
#include <maya/MFloatArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MVectorArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MTimeArray.h>
#include <set>
#if MAYA_API_VERSION >= 201200
#include <maya/MCacheFormatDescription.h>
#endif

#include "Partio.h"
#include "partio4MayaShared.h"

class PartioCache : public MPxCacheFormat
{
public:

   PartioCache(const MString &ext);
   virtual ~PartioCache();

   virtual MStatus open( const MString& fileName, FileAccessMode mode);
   virtual void close();

   virtual MStatus isValid();
   virtual MStatus rewind();

   virtual MString extension();

   virtual MStatus readHeader();
   virtual MStatus writeHeader(const MString& version, MTime& startTime, MTime& endTime);


   virtual void beginWriteChunk();
   virtual void endWriteChunk();

   virtual MStatus beginReadChunk();
   virtual void endReadChunk();

   virtual MStatus writeTime(MTime& time);
   virtual MStatus readTime(MTime& time);
   virtual  MStatus findTime(MTime& time, MTime& foundTime);
   virtual MStatus readNextTime(MTime& foundTime);

   virtual unsigned  readArraySize();

   // Write data to the cache.
   virtual MStatus writeDoubleArray(const MDoubleArray&);
   virtual MStatus writeFloatArray(const MFloatArray&);
   virtual MStatus writeIntArray(const MIntArray&);
   virtual MStatus writeDoubleVectorArray(const MVectorArray& array);
   virtual MStatus writeFloatVectorArray(const MFloatVectorArray& array);
   virtual MStatus writeInt32(int);

   // Read data from the cache.
   virtual MStatus readDoubleArray(MDoubleArray&, unsigned size);
   virtual MStatus readFloatArray(MFloatArray&, unsigned size);
   virtual MStatus readIntArray(MIntArray&, unsigned size);
   virtual MStatus readDoubleVectorArray(MVectorArray&, unsigned arraySize);
   virtual MStatus readFloatVectorArray(MFloatVectorArray& array, unsigned arraySize);
   virtual int readInt32();

   virtual MStatus writeChannelName(const MString & name);
   virtual MStatus findChannelName(const MString & name);
   virtual MStatus readChannelName(MString& name);

#if MAYA_API_VERSION >= 201200
   // Read and write the description file.
   virtual bool handlesDescription();
   virtual MStatus readDescription(MCacheFormatDescription& description,
                                   const MString& descriptionFileLocation,
                                   const MString& baseFileName );
   virtual MStatus writeDescription(const MCacheFormatDescription& description,
                                    const MString& descriptionFileLocation,
                                    const MString& baseFileName );
#endif

   // ---

   static size_t TotalNumFormats();
   static const char* FormatExtension(size_t n);

   template <size_t D>
   static void* Create()
   {
      const char *ext = FormatExtension(D);
      if (ext)
      {
         return new PartioCache(ext);
      }
      else
      {
         return 0;
      }
   }

private:

   void clean();
   void resetReadAttr();
   void resetWriteAttr();
   void resetPartioData();
   MString nodeName(const MString &channel);
   MString attrName(const MString &channel);

   MString mFilename;
   MString mDirname;
   MString mBasenameNoExt;
   FileAccessMode mMode;
   MString mExt;
   partio4Maya::CacheFiles mCacheFiles;
   partio4Maya::CacheFiles::iterator mCurSample;
   partio4Maya::CacheFiles::iterator mLastSample;
   Partio::ParticlesData *mRData;
   Partio::ParticlesDataMutable *mWData;
   MString mWChan;
   MTime mWTime;
   int mRAttrIdx;
   Partio::ParticleAttribute mRAttr;
   std::set<std::string> mChannels;

   static std::vector<const char*> msAllFormats;
};

#endif
