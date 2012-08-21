/* partio4Maya  3/12/2012, John Cassella  http://luma-pictures.com and  http://redpawfx.com
PARTIO Visualizer
Copyright 2012 (c)  All rights reserved

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

#ifndef Partio4MayaVisualizer_H
#define Partio4MayaVisualizer_H

#define _USE_MGL_FT_

#include <stdlib.h>
#include <vector>
#include <math.h>
#include <set>

#include <maya/MBoundingBox.h>
#include <maya/MColor.h>
#include <maya/MDagPath.h>
#include <maya/MDataHandle.h>
#include <maya/MDataBlock.h>
#include <maya/MDistance.h>
#include <maya/MDrawData.h>
#include <maya/MDoubleArray.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatVector.h>
#include <maya/MGLFunctionTable.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MIOStream.h>
#include <maya/MMatrix.h>
#include <maya/MPointArray.h>
#include <maya/MStringArray.h>
#include <maya/MSceneMessage.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>
#include <maya/MStatus.h>
#include <maya/MTypeId.h>
#include <maya/MTypes.h>
#include <maya/MTime.h>
#include <maya/MVectorArray.h>
#include <maya/MVector.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/M3dView.h>

#include <maya/MPxSurfaceShape.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MPxNode.h>

#include <maya/MFnUnitAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MFnNumericData.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnNumericAttribute.h>

#include <Partio.h>
#include <PartioAttribute.h>
#include <PartioIterator.h>

#include "partio4MayaShared.h"
#include "iconArrays.h"

struct partVizState 
{ // struct to keep track of state changes between renders
	MString fileLoaded;
	MString path;
	MString file;
	MString ext;
	bool isStatic;
	int colorFromIndex;
	int alphaFromIndex;
	int radiusFromIndex;
	int opacityFromIndex;
	MFloatVector color;
	float alpha;
	bool invertAlpha;
	float radius;
	bool flipStatus;
};

struct partVizContext 
{ // struct to keep track of current context vars
	MStringArray attributeList;
	MString newCacheFile;
	MString renderCacheFile;
	MFloatVector defaultColor;
	float defaultAlpha;
	float defaultRadius;
	bool flipped;
	bool frameChanged;
	bool drawError;
	bool cacheActive;
	bool cacheChanged;
	bool forceReload;
};

class partioVizReaderCache
{
	public:
		partioVizReaderCache();
		int token;
		MBoundingBox bbox;
		int dList;
		Partio::ParticlesDataMutable* particles;
		Partio::ParticleAttribute positionAttr;
		Partio::ParticleAttribute colorAttr;
		Partio::ParticleAttribute opacityAttr;
		Partio::ParticleAttribute radiusAttr;
		float* rgb;
		float* rgba;
		MFloatArray radius;
		float* flipPos;
};

class partioVisualizerUI : public MPxSurfaceShapeUI 
{
	public:
		partioVisualizerUI();
		virtual ~partioVisualizerUI();
		virtual void draw(const MDrawRequest & request, M3dView & view) const;
		virtual void getDrawRequests(const MDrawInfo & info, bool objectAndActiveOnly, MDrawRequestQueue & requests);
		void 	drawBoundingBox() const;
		void    drawBillboardCircleAtPoint(MVector position, float radius, int num_segments, int drawType) const;
		void 	drawPartio(partioVizReaderCache* pvCache, int drawStyle) const;
		static void* creator();
		virtual bool	select( MSelectInfo &selectInfo, MSelectionList &selectionList, MPointArray &worldSpaceSelectPts ) const;
};

class partioVisualizer : public MPxSurfaceShape 
{
	public:
		partioVisualizer();
		virtual ~partioVisualizer();
		virtual MStatus compute( const MPlug& plug, MDataBlock& block );
		virtual bool isBounded() const;
		virtual MBoundingBox boundingBox() const;
		virtual void 	postConstructor();
		
		static  void* creator();
		static  MStatus initialize();
		static void reInit(void *data);
		void initCallback();
		bool GetPlugData();
		partioVizReaderCache* updateParticleCache();
		
		static MObject time;
		static MObject aSize; // The size of the logo
		static MObject aDrawSkip;
		static MObject aFlipYZ;
		static MObject aUpdateCache;
		static MObject aCacheDir;
		static MObject aCacheFile;
		static MObject aUseTransform;
		static MObject aCacheActive;
		static MObject aCacheOffset;
		static MObject aCacheStatic;
		static MObject aCacheFormat;
		static MObject aJitterPos;
		static MObject aJitterFreq;
		static MObject aPartioAttributes;
		static MObject aColorFrom;
		static MObject aAlphaFrom;
		static MObject aRadiusFrom;
		static MObject aPointSize;
		static MObject aDefaultPointColor;
		static MObject aDefaultAlpha;
		static MObject aDefaultRadius;
		static MObject aInvertAlpha;
		static MObject aDrawStyle;
		static MObject aForceReload;
		static MObject aRenderCachePath;
		
		MCallbackId partioVisualizerOpenCallback;
		MCallbackId partioVisualizerImportCallback;
		MCallbackId partioVisualizerReferenceCallback;
		
		static MTypeId id;
		float 	multiplier;
		partioVizReaderCache pvCache;
	
	private:
		MStatus getCurrentState( const MPlug& plug, MDataBlock& block);
		MStatus loadCache( const MPlug& plug, MDataBlock& block);
		bool testForCacheReload( const MPlug& plug, MDataBlock& block);
		MStatus reloadCache( const MPlug& plug, MDataBlock& block);
		MStatus doFlip(const MPlug& plug, MDataBlock& block);
		void updateColor( const MPlug& plug, MDataBlock& block);
		void updateAlpha( const MPlug& plug, MDataBlock& block);
		void updateRadius( const MPlug& plug, MDataBlock& block);
		void updateAEControls(const MPlug& plug, MDataBlock& block);
		
		// state tracking structs
		partVizState mLast;
		partVizState mCurrent;
		// shared compute vars
		partVizContext mContext;
		
	protected:
		int dUpdate;
		GLuint dList;
};

#endif
