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

#define _DEBUG

#include "partioCache.h"
#include <maya/MAnimControl.h>

template <typename PT, int D, typename MT>
struct ArrayWriter
{
   static void Write(PT *data, const MT &mayaData)
   {
      for (unsigned int i=0; i<mayaData.length(); ++i, data+=D)
      {
         for (int j=0; j<D; ++j)
         {
            data[j] = PT(mayaData[i][j]);
         }
      }
   }
};

template <typename PT, typename MT>
struct ArrayWriter<PT, 1, MT>
{
   static void Write(PT *data, const MT &mayaData)
   {
      for (unsigned int i=0; i<mayaData.length(); ++i, ++data)
      {
         *data = PT(mayaData[i]);
      }
   }
};

template <typename PT, int D, typename MT>
struct ArrayReader
{
   static void Read(const PT *data, MT &mayaData)
   {
      for (unsigned long i=0; i<mayaData.length(); ++i, data+=D)
      {
         for (int j=0; j<D; ++j)
         {
            mayaData[i][j] = data[j];
         }
      }
   }
};

template <typename PT, typename MT>
struct ArrayReader<PT, 1, MT>
{
   static void Read(const PT *data, MT &mayaData)
   {
      for (unsigned long i=0; i<mayaData.length(); ++i, ++data)
      {
         mayaData[i] = *data;
      }
   }
};

template <Partio::ParticleAttributeType PT, int D, typename MT>
struct ArrayAccessor
{
   typedef typename Partio::ETYPE_TO_TYPE<PT>::TYPE PIOType;

   static MStatus Write(Partio::ParticlesDataMutable *partData, const MString &name, const MT &mayaData)
   {
      if (!partData)
      {
#ifdef _DEBUG
         MGlobal::displayInfo("ArrayAccessor::Write: No PartIO data");
#endif
         return MStatus::kFailure;
      }

      if (partData->numParticles() <= 0)
      {
         partData->addParticles(mayaData.length());
      }
      else if (partData->numParticles() != int(mayaData.length()))
      {
#ifdef _DEBUG
         MGlobal::displayInfo("ArrayAccessor::Write: Particle count mismatch");
#endif
         return MStatus::kFailure;
      }

      Partio::ParticleAttribute pattr;

      if (!partData->attributeInfo(name.asChar(), pattr))
      {
         pattr = partData->addAttribute(name.asChar(), PT, D);
      }

      PIOType *data = partData->dataWrite<PIOType>(pattr, 0);

      ArrayWriter<PIOType, D, MT>::Write(data, mayaData);

      return MStatus::kSuccess;
   }

   static MStatus Read(Partio::ParticlesData *partData, const Partio::ParticleAttribute &pattr, MT &mayaData)
   {
      const PIOType *data = partData->data<PIOType>(pattr, 0);

      mayaData.setLength(partData->numParticles());

      ArrayReader<PIOType, D, MT>::Read(data, mayaData);

      return MStatus::kSuccess;
   }
};

// ---

PartioCache::PartioCache(const MString &ext)
   : MPxCacheFormat()
   , mMode(MPxCacheFormat::kRead)
   , mExt(ext)
   , mRData(0)
   , mWData(0)
{
#ifdef _DEBUG
   MGlobal::displayInfo("Create PartioCache \"" + mExt + "\"");
#endif
   mCurSample = mCacheFiles.end();
   mLastSample = mCacheFiles.end();
}

PartioCache::~PartioCache()
{
#ifdef _DEBUG
   MGlobal::displayInfo("Destroy PartioCache \"" + mExt + "\"");
#endif
   clean();
}


MStatus PartioCache::isValid()
{
#ifdef _DEBUG
   MGlobal::displayInfo(MString("PartioCache isValid: ") + (mCacheFiles.size() > 0 ? "true" : "false"));
#endif
   return (mCacheFiles.size() > 0 ? MStatus::kSuccess : MStatus::kFailure);
}

MString PartioCache::extension()
{
   return mExt;
}

MStatus PartioCache::open(const MString &fileName, MPxCacheFormat::FileAccessMode mode)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache open \"" + fileName + MString("\" ") + (mode == MPxCacheFormat::kRead ? "R" : (mode == MPxCacheFormat::kWrite ? "W" : "RW")));
#endif

   bool success = true;

   if (fileName != mFilename)
   {
      if (mode == MPxCacheFormat::kRead)
      {
         success = (Partio::readFormatIndex(mExt.asChar()) != Partio::InvalidIndex && mCacheFiles.size() > 0);
      }
      else if (mode == MPxCacheFormat::kWrite)
      {
         success = (Partio::writeFormatIndex(mExt.asChar()) != Partio::InvalidIndex);
      }
      else
      {
         success = (Partio::writeFormatIndex(mExt.asChar()) != Partio::InvalidIndex && Partio::readFormatIndex(mExt.asChar()) != Partio::InvalidIndex);
      }
   }

   mFilename = fileName;
   mMode = mode;
   mCurSample = mCacheFiles.end();

#ifdef _DEBUG
   MGlobal::displayInfo(success ? "  Open succeeded" : "  Open failed");
#endif

   return (success ? MStatus::kSuccess : MStatus::kFailure);
}

MStatus PartioCache::rewind()
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache rewind");
#endif
   if (mCacheFiles.size() > 0)
   {
      mCurSample = mCacheFiles.begin();
      return MStatus::kSuccess;
   }
   else
   {
      return MStatus::kFailure;
   }
}

void PartioCache::close()
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache close");
#endif
   mCurSample = mCacheFiles.end();
}

void PartioCache::resetWriteAttr()
{
   mWChan = "";
   mWTime = 0.0;
}

void PartioCache::resetReadAttr()
{
   mRAttrIdx = -1;
   mRAttr.type = Partio::NONE;
   mRAttr.attributeIndex = -1;
   mRAttr.count = 0;
   mRAttr.name = "";
}

void PartioCache::resetPartioData()
{
   if (mRData)
   {
      mRData->release();
      mRData = 0;
   }
   if (mWData)
   {
      mWData->release();
      mWData = 0;
   }
   resetReadAttr();
   resetWriteAttr();
}

void PartioCache::clean()
{
   close();
   resetPartioData();
   mFilename = "";
   mDirname = "";
   mBasenameNoExt = "";
   mMode = MPxCacheFormat::kRead;
   mCacheFiles.clear();
   mLastSample = mCacheFiles.end();
   mChannels.clear();
}

MStatus PartioCache::readHeader()
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache readHeader");
#endif

   return MStatus::kSuccess;
}

MStatus PartioCache::beginReadChunk()
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache beginReadChunk");
#endif
   // Not always called by maya...

   if (mCurSample == mCacheFiles.end())
   {
#ifdef _DEBUG
      MGlobal::displayInfo("  Invalid sample");
#endif
      return MStatus::kFailure;
   }

   return MStatus::kSuccess;
}

MStatus PartioCache::readNextTime(MTime &foundTime)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache readNextTime");
#endif
   if (mCurSample != mCacheFiles.end())
   {
      foundTime = MTime(mCurSample->first.asUnits(MTime::k6000FPS), MTime::k6000FPS);
#ifdef _DEBUG
      MGlobal::displayInfo(MString("  -> ") + foundTime.asUnits(MTime::uiUnit()));
#endif
      ++mCurSample;
      return MStatus::kSuccess;
   }
   else
   {
      return MStatus::kFailure;
   }
}

MStatus PartioCache::findChannelName(const MString &name)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache findChannelName " + name);
#endif

   if (name == "count" || mChannels.find(name.asChar()) != mChannels.end())
   {
#ifdef _DEBUG
      MGlobal::displayInfo("  Found");
#endif
      if (mRData)
      {
         // If particles data exists, setup current attribute access
         Partio::ParticleAttribute pattr;
         if (name == "count")
         {
            mRAttrIdx = mRData->numAttributes();
         }
         else if (mRData->attributeInfo(name.asChar(), pattr))
         {
            mRAttrIdx = pattr.attributeIndex;
            mRAttr = pattr;
         }
         else
         {
            resetReadAttr();
            return MStatus::kFailure;
         }
      }
      return MStatus::kSuccess;
   }
   else
   {
      return MStatus::kFailure;
   }
}

void PartioCache::endReadChunk()
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache endReadChunk");
#endif
}

MStatus PartioCache::readTime(MTime &time)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache readTime");
#endif
   // Never called by maya

   if (mCurSample != mCacheFiles.end())
   {
#ifdef _DEBUG
      MGlobal::displayInfo(MString("  -> ") + time.asUnits(MTime::uiUnit()));
#endif
      time = MTime(mCurSample->first.asUnits(MTime::k6000FPS), MTime::k6000FPS);
      return MStatus::kSuccess;
   }
   else
   {
      return MStatus::kFailure;
   }
}

MStatus PartioCache::findTime(MTime &time, MTime &foundTime)
{
#ifdef _DEBUG
   MGlobal::displayInfo(MString("PartioCache findTime: ") + time.asUnits(MTime::uiUnit()));
#endif
   // Called by maya when changing current frame
   // => this is where we want to read the data, not in beginReadChunk
   //    as it is called in loop to query all available samples and attributes (readNextTime, findChannelName)

   partio4Maya::CacheFiles::iterator fit;

   if (partio4Maya::findCacheFile(mCacheFiles, partio4Maya::FM_EXACT, time, fit))
   {
#ifdef _DEBUG
      MGlobal::displayInfo(MString("  Found: ") + fit->first.asUnits(MTime::uiUnit()));
#endif

      mCurSample = fit;

      if (mCurSample != mLastSample)
      {
         resetPartioData();
      }

      if (!mRData)
      {
#ifdef _DEBUG
         MGlobal::displayInfo("  Read sample from \"" + mCurSample->second + "\"");
#endif
         mRData = Partio::read(mCurSample->second.asChar());
      }

      resetReadAttr();

      mLastSample = mCurSample;

      foundTime = MTime(fit->first.asUnits(MTime::k6000FPS), MTime::k6000FPS);

      return (mRData != 0 ? MStatus::kSuccess : MStatus::kFailure);
   }
   else
   {
#ifdef _DEBUG
      MGlobal::displayInfo(MString("  Not found"));
#endif
      return MStatus::kFailure;
   }
}

MStatus PartioCache::readChannelName(MString &name)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache readChannelName");
#endif

   if (mRData)
   {
      int N = mRData->numAttributes();
      ++mRAttrIdx;
      if (mRAttrIdx >= 0 && mRAttrIdx < N)
      {
         Partio::ParticleAttribute pattr;
         if (mRData->attributeInfo(mRAttrIdx, mRAttr))
         {
            // skip particle id
            if (mRAttr.name == "id" || mRAttr.name == "particleId")
            {
#ifdef _DEBUG
               MGlobal::displayInfo(MString("  Skip ") + mRAttr.name.c_str());
#endif
               ++mRAttrIdx;
               if (mRAttrIdx < N)
               {
                  if (!mRData->attributeInfo(mRAttrIdx, mRAttr))
                  {
                     return MStatus::kFailure;
                  }
               }
               else if (mRAttrIdx == N)
               {
                  name = "count";
#ifdef _DEBUG
                  MGlobal::displayInfo("  " + name);
#endif
                  return MStatus::kSuccess;
               }
               else
               {
                  return MStatus::kFailure;
               }
            }
            name = mRAttr.name.c_str();
#ifdef _DEBUG
            MGlobal::displayInfo("  " + name);
#endif
            return MStatus::kSuccess;
         }
      }
      else if (mRAttrIdx == mRData->numAttributes())
      {
         // count!
         name = "count";
#ifdef _DEBUG
         MGlobal::displayInfo("  " + name);
#endif
         return MStatus::kSuccess;
      }
   }
   return MStatus::kFailure;
}

unsigned PartioCache::readArraySize()
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache readArraySize");
#endif

   if (mRData)
   {
      if (mRAttrIdx >= 0 && mRAttrIdx < mRData->numAttributes()) // or <=
      {
#ifdef _DEBUG
         MGlobal::displayInfo(MString("  ") + mRData->numParticles());
#endif
         return mRData->numParticles();
      }
      else if (mRAttrIdx == mRData->numAttributes())
      {
#ifdef _DEBUG
         MGlobal::displayInfo("-> \"count\"");
         MGlobal::displayInfo(MString("  ") + 1);
#endif
         // count attribute
         return 1;
      }
   }
   return 0;
}

MStatus PartioCache::readDoubleArray(MDoubleArray &array, unsigned size)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache readDoubleArray");
#endif
   if (!mRData)
   {
      return MStatus::kFailure;
   }

   if (int(size) != mRData->numParticles() || mRAttrIdx < 0 || mRAttrIdx >= mRData->numAttributes())
   {
      // can be the count attribute
      if (mRAttrIdx == mRData->numAttributes() && size == 1)
      {
#ifdef _DEBUG
         MGlobal::displayInfo("PartioCache readDoubleArray \"count\"");
#endif
         array.setLength(1);
         array[0] = mRData->numParticles();
         return MStatus::kSuccess;
      }
#ifdef _DEBUG
      MGlobal::displayWarning("PartioCache readDoubleArray failed");
#endif
      return MStatus::kFailure;
   }
   return ArrayAccessor<Partio::FLOAT, 1, MDoubleArray>::Read(mRData, mRAttr, array);
}

MStatus PartioCache::readFloatArray(MFloatArray &array, unsigned size)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache readFloatArray");
#endif
   if (!mRData)
   {
      return MStatus::kFailure;
   }

   if (int(size) != mRData->numParticles() || mRAttrIdx < 0 || mRAttrIdx >= mRData->numAttributes())
   {
#ifdef _DEBUG
      MGlobal::displayWarning("PartioCache readFloatArray failed");
#endif
      return MStatus::kFailure;
   }
   return ArrayAccessor<Partio::FLOAT, 1, MFloatArray>::Read(mRData, mRAttr, array);
}

MStatus PartioCache::readIntArray(MIntArray &array, unsigned size)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache readIntArray");
#endif
   if (!mRData)
   {
      return MStatus::kFailure;
   }

   if (int(size) != mRData->numParticles() || mRAttrIdx < 0 || mRAttrIdx >= mRData->numAttributes())
   {
#ifdef _DEBUG
      MGlobal::displayWarning("PartioCache readIntArray failed");
#endif
      return MStatus::kFailure;
   }
   return ArrayAccessor<Partio::INT, 1, MIntArray>::Read(mRData, mRAttr, array);
}

MStatus PartioCache::readDoubleVectorArray(MVectorArray &array, unsigned size)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache readDoubleVectorArray");
#endif
   if (!mRData)
   {
      return MStatus::kFailure;
   }

   if (int(size) != mRData->numParticles() || mRAttrIdx < 0 || mRAttrIdx >= mRData->numAttributes())
   {
#ifdef _DEBUG
      MGlobal::displayWarning("PartioCache readDoubleVectorArray failed");
#endif
      return MStatus::kFailure;
   }
   return ArrayAccessor<Partio::VECTOR, 3, MVectorArray>::Read(mRData, mRAttr, array);
}

MStatus PartioCache::readFloatVectorArray(MFloatVectorArray &array, unsigned size)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache readFloatVectorArray");
#endif
   if (!mRData)
   {
      return MStatus::kFailure;
   }

   if (int(size) != mRData->numParticles() || mRAttrIdx < 0 || mRAttrIdx >= mRData->numAttributes())
   {
#ifdef _DEBUG
      MGlobal::displayWarning("PartioCache readFloatVectorArray failed");
#endif
      return MStatus::kFailure;
   }
   return ArrayAccessor<Partio::VECTOR, 3, MFloatVectorArray>::Read(mRData, mRAttr, array);
}

int PartioCache::readInt32()
{
#ifdef _DEBUG
   MGlobal::displayWarning("PartioCache readInt32, not implemeted");
#endif
   return 0;
}

MStatus PartioCache::writeHeader(const MString &version, MTime &startTime, MTime &endTime)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache writeHeader: " + version + " " + startTime.value() + " " + endTime.value());
#endif

   return MStatus::kSuccess;
}

void PartioCache::beginWriteChunk()
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache beginWriteChunk");
#endif
   // May need to review this
   if (mWData)
   {
      mWData->release();
   }
   mWData = Partio::create();
}

MString PartioCache::nodeName(const MString &channel)
{
   int p = channel.rindexW('_');
   if (p == -1)
   {
      return "";
   }
   else
   {
      return channel.substring(0, p-1);
   }
}

MString PartioCache::attrName(const MString &channel)
{
   int p = channel.rindexW('_');
   if (p == -1)
   {
      return channel;
   }
   else
   {
      return channel.substring(p+1, channel.length()-1);
   }
}

MStatus PartioCache::writeTime(MTime &time)
{
#ifdef _DEBUG
   MGlobal::displayInfo(MString("PartioCache writeTime ") + time.value());
#endif
   mWTime = time;
   return MStatus::kSuccess;
}

MStatus PartioCache::writeChannelName(const MString &name)
{
#ifdef _DEBUG
   MGlobal::displayInfo(MString("PartioCache writeChannelName ") + name);
#endif
   mWChan = attrName(name);
   return MStatus::kSuccess;
}

MStatus PartioCache::writeDoubleArray(const MDoubleArray &array)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache writeDoubleArray");
#endif
   if (mWChan == "count")
   {
#ifdef _DEBUG
      MGlobal::displayInfo("  Ignore \"count\"");
#endif
      return MStatus::kSuccess;
   }
   return ArrayAccessor<Partio::FLOAT, 1, MDoubleArray>::Write(mWData, mWChan, array);
}

MStatus PartioCache::writeFloatArray(const MFloatArray &array)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache writeFloatArray");
#endif
   return ArrayAccessor<Partio::FLOAT, 1, MFloatArray>::Write(mWData, mWChan, array);
}

MStatus PartioCache::writeIntArray(const MIntArray &array)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache writeIntArray");
#endif
   return ArrayAccessor<Partio::INT, 1, MIntArray>::Write(mWData, mWChan, array);
}

MStatus PartioCache::writeDoubleVectorArray(const MVectorArray &array)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache writeDoubleVectorArray");
#endif
   return ArrayAccessor<Partio::VECTOR, 3, MVectorArray>::Write(mWData, mWChan, array);
}

MStatus PartioCache::writeFloatVectorArray(const MFloatVectorArray &array)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache writeFloatVectorArray");
#endif
   return ArrayAccessor<Partio::VECTOR, 3, MFloatVectorArray>::Write(mWData, mWChan, array);
}

MStatus PartioCache::writeInt32(int value)
{
#ifdef _DEBUG
   char tmp[256];
   sprintf(tmp, "PartioCache::writeInt32: %d, not implemented", value);
   MGlobal::displayWarning(tmp);
#endif

   return MStatus::kSuccess;
}

void PartioCache::endWriteChunk()
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache endWriteChunk");
#endif
   if (mWData)
   {
      if (mWData->numParticles() > 0)
      {
         char fext[256];

         MString filename;

         int frame = int(floor(mWTime.asUnits(MTime::uiUnit())));
         sprintf(fext, ".%04d.%s", frame, mExt.asChar());

         filename = mDirname + mBasenameNoExt + MString(fext);

#ifdef _DEBUG
         MGlobal::displayInfo("  Write sample \"" + filename + "\"");
#endif

         Partio::write(filename.asChar(), *mWData, false);

         mWData->release();
         mWData = 0;
      }
      else
      {
#ifdef _DEBUG
         MGlobal::displayInfo("  No particles to write");
#endif
         // remove current sample
         mCacheFiles.erase(mCurSample);
      }
   }
}

#if MAYA_API_VERSION >= 201200
bool PartioCache::handlesDescription()
{
   return true;
}

MStatus PartioCache::readDescription(MCacheFormatDescription &desc, const MString &descFileLoc, const MString &descFileName)
{
#ifdef _DEBUG
   MGlobal::displayInfo("Read description");
#endif

   // Be sure to start with a clean plate
   clean();

   unsigned long n = partio4Maya::getFileList(descFileLoc, descFileName, mExt, mCacheFiles); //files, times);

   if (n == 0)
   {
#ifdef _DEBUG
      MGlobal::displayWarning("No cache file matching: " + descFileLoc + "/" + descFileName + ".*." + mExt);
#endif
      return MStatus::kFailure;
   }

   mDirname = descFileLoc;
   mBasenameNoExt = descFileName;
   mCurSample = mCacheFiles.end();
   mLastSample = mCacheFiles.end();

   partio4Maya::CacheFiles::iterator fit = mCacheFiles.begin();
   MTime start = fit->first;
   MTime end = fit->first;
   ++fit;
   for (; fit!=mCacheFiles.end(); ++fit)
   {
      if (fit->first < start)
      {
         start = fit->first;
      }
      if (fit->first > end)
      {
         end = fit->first;
      }
   }

#ifdef _DEBUG
   MGlobal::displayInfo(MString("  Cache range: ") + start.value() + "-" + end.value());
#endif

   Partio::ParticlesInfo *pinfo = Partio::readHeaders(mCacheFiles.begin()->second.asChar());

   if (!pinfo)
   {
#ifdef _DEBUG
      MGlobal::displayWarning("Could not read particle header for " + mCacheFiles.begin()->second);
#endif
      return MStatus::kFailure;
   }

   // dummy value, as we use kOneFile and kIrregular maya doesn't take it into account
   MTime srate(1.0, MTime::uiUnit());

   desc.setDistribution(MCacheFormatDescription::kOneFile);
   desc.setTimePerFrame(MTime(1, MTime::uiUnit()));
   desc.addDescriptionInfo("PartIO cache ." + mExt);

   for (int i=0; i<pinfo->numAttributes(); ++i)
   {
      Partio::ParticleAttribute pattr;
      if (pinfo->attributeInfo(i, pattr))
      {
         // name should be prefixed by "<shapename>_" ?
         MString name = pattr.name.c_str();
         if (name == "id" || name == "particleId")
         {
#ifdef _DEBUG
            MGlobal::displayInfo("  Skip channel: " + name);
#endif
            continue;
         }
#ifdef _DEBUG
         MGlobal::displayInfo("  Found channel: " + name);
#endif
         switch (pattr.type)
         {
         case Partio::INT:
            desc.addChannel(name, name, MCacheFormatDescription::kInt32Array, MCacheFormatDescription::kIrregular, srate, start, end);
            mChannels.insert(pattr.name);
            break;
         case Partio::FLOAT:
            desc.addChannel(name, name, MCacheFormatDescription::kDoubleArray, MCacheFormatDescription::kIrregular, srate, start, end);
            mChannels.insert(pattr.name);
            break;
         case Partio::VECTOR:
            desc.addChannel(name, name, MCacheFormatDescription::kDoubleVectorArray, MCacheFormatDescription::kIrregular, srate, start, end);
            mChannels.insert(pattr.name);
            break;
         case Partio::INDEXEDSTR:
         default:
            //desc.addChannel(name, name, MCacheFormatDescription::kUnknownData, MCacheFormatDescription::kIrregular, srate, start, end);
            continue;
         }
      }
   }

   // At last, add count attribute
   desc.addChannel("count", "count", MCacheFormatDescription::kDoubleArray, MCacheFormatDescription::kIrregular, srate, start, end);

   pinfo->release();

   return MStatus::kSuccess;
}

MStatus PartioCache::writeDescription(const MCacheFormatDescription &desc, const MString &descFileLoc, const MString &descFileName)
{
#ifdef _DEBUG
   MGlobal::displayInfo("Write description " + descFileLoc + " " + descFileName);
#endif

   return MStatus::kSuccess;
}
#endif

// ---

std::vector<const char*> PartioCache::msAllFormats;

size_t PartioCache::TotalNumFormats()
{
   if (msAllFormats.size() == 0)
   {
      for (size_t i=0; i<Partio::numReadFormats(); ++i)
      {
         const char *ext = Partio::readFormatExtension(i);
         if (!strcmp(ext, "mc"))
         {
            // skip maya cache file
            continue;
         }
         msAllFormats.push_back(ext);
      }
      for (size_t i=0; i<Partio::numWriteFormats(); ++i)
      {
         const char *ext = Partio::writeFormatExtension(i);
         if (!strcmp(ext, "mc"))
         {
            // skip maya cache file
            continue;
         }
         if (Partio::readFormatIndex(ext) == Partio::InvalidIndex)
         {
            msAllFormats.push_back(ext);
         }
      }
   }

   return msAllFormats.size();
}

const char* PartioCache::FormatExtension(size_t n)
{
   return (n < TotalNumFormats() ? msAllFormats[n] : NULL);
}
