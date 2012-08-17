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

#include "partioVisualizer.h"

static MGLFunctionTable *gGLFT = NULL;

#define ID_PARTIOVISUALIZER  0x00116ECF // id is registered with autodesk no need to change
#define LEAD_COLOR 18 // green
#define ACTIVE_COLOR 15 // white
#define ACTIVE_AFFECTED_COLOR 8 // purple
#define DORMANT_COLOR 4 // blue
#define HILITE_COLOR 17 // pale blue

#define DRAW_STYLE_POINTS 0
#define DRAW_STYLE_RADIUS 1
#define DRAW_STYLE_DISK	 2
#define DRAW_STYLE_BOUNDING_BOX 3

using namespace Partio;
using namespace std;

MTypeId partioVisualizer::id( ID_PARTIOVISUALIZER );

MObject partioVisualizer::time;
MObject partioVisualizer::aDrawSkip;
MObject partioVisualizer::aUpdateCache;
MObject partioVisualizer::aSize; // The size of the logo
MObject partioVisualizer::aFlipYZ;
MObject partioVisualizer::aCacheDir;
MObject partioVisualizer::aCacheFile;
MObject partioVisualizer::aUseTransform;
MObject partioVisualizer::aCacheActive;
MObject partioVisualizer::aCacheOffset;
MObject partioVisualizer::aCacheStatic;
MObject partioVisualizer::aCacheFormat;
MObject partioVisualizer::aJitterPos;
MObject partioVisualizer::aJitterFreq;
MObject partioVisualizer::aPartioAttributes;
MObject partioVisualizer::aColorFrom;
MObject partioVisualizer::aRadiusFrom;
MObject partioVisualizer::aAlphaFrom;
MObject partioVisualizer::aPointSize;
MObject partioVisualizer::aDefaultPointColor;
MObject partioVisualizer::aDefaultAlpha;
MObject partioVisualizer::aDefaultRadius;
MObject partioVisualizer::aInvertAlpha;
MObject partioVisualizer::aDrawStyle;
MObject partioVisualizer::aForceReload;
MObject partioVisualizer::aRenderCachePath;

partioVizReaderCache::partioVizReaderCache():
		token(0),
		bbox(MBoundingBox(MPoint(0,0,0,0),MPoint(0,0,0,0))),
		dList(0),
		particles(NULL),
		rgb(NULL),
		rgba(NULL),
		flipPos(NULL) {}

/// CREATOR
partioVisualizer::partioVisualizer() : multiplier(1.0) {
	mContext.cacheChanged = false;
	mContext.frameChanged =false;
	mContext.flipped = false;
	mContext.drawError = false;
	mLast.fileLoaded = "";
	mLast.path = "";
	mLast.file = "";
	mLast.ext = "";
	mLast.isStatic = false;
	mLast.colorFromIndex = -1;
	mLast.alphaFromIndex = -1;
	mLast.radiusFromIndex = -1;
	mLast.opacityFromIndex = -1;
	mLast.color = MFloatVector(1,0,0);
	mLast.alpha = 0.0;
	mLast.invertAlpha = false;
	mLast.radius = 1.0;
	mLast.flipStatus = false;
	pvCache.particles = NULL;
	pvCache.flipPos = (float *) malloc(sizeof(float));
	pvCache.rgb = (float *) malloc (sizeof(float));
	pvCache.rgba = (float *) malloc(sizeof(float));
	pvCache.radius = MFloatArray();
}

/// DESTRUCTOR
partioVisualizer::~partioVisualizer() {
	if (pvCache.particles){
		pvCache.particles->release();
	}
	// free memory
	free(pvCache.flipPos);
	free(pvCache.rgb);
	free(pvCache.rgba);
	pvCache.radius.clear();
	// remove callbacks
	MSceneMessage::removeCallback( partioVisualizerOpenCallback);
	MSceneMessage::removeCallback( partioVisualizerImportCallback);
	MSceneMessage::removeCallback( partioVisualizerReferenceCallback);
}

void* partioVisualizer::creator() {
	return new partioVisualizer;
}

/// POST CONSTRUCTOR
void partioVisualizer::postConstructor() {
	setRenderable(true);
	// set callbacks
	partioVisualizerOpenCallback = MSceneMessage::addCallback(MSceneMessage::kAfterOpen, partioVisualizer::reInit, this);
	partioVisualizerImportCallback = MSceneMessage::addCallback(MSceneMessage::kAfterImport, partioVisualizer::reInit, this);
	partioVisualizerReferenceCallback = MSceneMessage::addCallback(MSceneMessage::kAfterReference, partioVisualizer::reInit, this);
}

/// INIT
void partioVisualizer::initCallback() {
	MObject tmo = thisMObject();
	short extENum;
	mLast.ext = partio4Maya::setExt(extENum);
	mContext.cacheChanged = false;
	
	MPlug(tmo,aCacheFormat).getValue(extENum);
	MPlug(tmo,aCacheDir).getValue(mLast.path);
	MPlug(tmo,aCacheFile).getValue(mLast.file);
	MPlug(tmo,aDefaultAlpha).getValue(mLast.alpha);
	MPlug(tmo,aDefaultRadius).getValue(mLast.radius);
	MPlug(tmo,aColorFrom).getValue(mLast.colorFromIndex);
	MPlug(tmo,aAlphaFrom).getValue(mLast.alphaFromIndex);
	MPlug(tmo,aRadiusFrom).getValue(mLast.radiusFromIndex);
	MPlug(tmo,aSize).getValue(multiplier);
	MPlug(tmo,aInvertAlpha).getValue(mLast.invertAlpha);
	MPlug(tmo,aCacheStatic).getValue(mLast.isStatic);
}

void partioVisualizer::reInit(void *data) {
	partioVisualizer* vizNode = (partioVisualizer*) data;
	vizNode->initCallback();
}

MStatus partioVisualizer::initialize() {
	MFnEnumAttribute eAttr;
	MFnUnitAttribute uAttr;
	MFnNumericAttribute nAttr;
	MFnTypedAttribute tAttr;
	MStatus stat;
	
	//create attributes
	time = nAttr.create( "time", "tm", MFnNumericData::kLong ,0);
	uAttr.setKeyable( true );
	
	aSize = uAttr.create( "iconSize", "isz", MFnUnitAttribute::kDistance );
	uAttr.setDefault( 0.25 );
	
	aFlipYZ = nAttr.create( "flipYZ", "fyz", MFnNumericData::kBoolean);
	nAttr.setDefault ( false );
	nAttr.setKeyable ( true );
	
	aDrawSkip = nAttr.create( "drawSkip", "dsk", MFnNumericData::kLong ,0);
	nAttr.setKeyable( true );
	nAttr.setReadable( true );
	nAttr.setWritable( true );
	nAttr.setConnectable( true );
	nAttr.setStorable( true );
	nAttr.setMin(0);
	nAttr.setMax(1000);
	
	aCacheDir = tAttr.create ( "cacheDir", "cachD", MFnStringData::kString );
	tAttr.setReadable ( true );
	tAttr.setWritable ( true );
	tAttr.setKeyable ( true );
	tAttr.setConnectable ( true );
	tAttr.setStorable ( true );
	
	aCacheFile = tAttr.create ( "cachePrefix", "cachP", MFnStringData::kString );
	tAttr.setReadable ( true );
	tAttr.setWritable ( true );
	tAttr.setKeyable ( true );
	tAttr.setConnectable ( true );
	tAttr.setStorable ( true );
	
	aCacheOffset = nAttr.create("cacheOffset", "coff", MFnNumericData::kInt, 0, &stat );
	nAttr.setKeyable(true);
	
	aCacheStatic = nAttr.create("staticCache", "statC", MFnNumericData::kBoolean, false, &stat);
	nAttr.setKeyable(true);
	
	aCacheActive = nAttr.create("cacheActive", "cAct", MFnNumericData::kBoolean, 1, &stat);
	nAttr.setKeyable(true);
	
	aCacheFormat = eAttr.create( "cacheFormat", "cachFmt");
	std::map<short,MString> formatExtMap;
	partio4Maya::buildSupportedExtensionList(formatExtMap,false);
	for (unsigned short i = 0; i< formatExtMap.size(); i++) {
		eAttr.addField(formatExtMap[i].toUpperCase(), i);
	}
	
	eAttr.setDefault(4);  // PDC
	eAttr.setChannelBox(true);
	
	aDrawStyle = eAttr.create( "drawStyle", "drwStyl");
	eAttr.addField("points", 0);
	eAttr.addField("radius", 1);
	eAttr.addField("disk", 2);
	eAttr.addField("boundingBox", 3);
	//eAttr.addField("sphere", 4);
	//eAttr.addField("velocity", 5);
	
	eAttr.setDefault(0);
	eAttr.setChannelBox(true);
	
	aUseTransform = nAttr.create("useTransform", "utxfm", MFnNumericData::kBoolean, false, &stat);
	nAttr.setKeyable(true);
	
	aPartioAttributes = tAttr.create ("partioCacheAttributes", "pioCAts", MFnStringData::kString);
	tAttr.setArray(true);
	tAttr.setDisconnectBehavior(MFnAttribute::kDelete);
	
	aColorFrom = nAttr.create("colorFrom", "cfrm", MFnNumericData::kInt, -1, &stat);
	nAttr.setDefault(-1);
	nAttr.setKeyable(true);
	
	aAlphaFrom = nAttr.create("opacityFrom", "ofrm", MFnNumericData::kInt, -1, &stat);
	nAttr.setDefault(-1);
	nAttr.setKeyable(true);
	
	aRadiusFrom = nAttr.create("radiusFrom", "rfrm", MFnNumericData::kInt, -1, &stat);
	nAttr.setDefault(-1);
	nAttr.setKeyable(true);
	
	aPointSize = nAttr.create("pointSize", "ptsz", MFnNumericData::kInt, 2, &stat);
	nAttr.setDefault(2);
	nAttr.setKeyable(true);
	
	aDefaultPointColor = nAttr.createColor("defaultPointColor", "dpc", &stat);
	nAttr.setDefault(1.0f, 0.0f, 0.0f);
	nAttr.setKeyable(true);
	
	aDefaultAlpha = nAttr.create("defaultAlpha", "dalf", MFnNumericData::kFloat, 1.0, &stat);
	nAttr.setDefault(1.0);
	nAttr.setMin(0.0);
	nAttr.setMax(1.0);
	nAttr.setKeyable(true);
	
	aInvertAlpha= nAttr.create("invertAlpha", "ialph", MFnNumericData::kBoolean, false, &stat);
	nAttr.setDefault(false);
	nAttr.setKeyable(true);
	
	aDefaultRadius = nAttr.create("defaultRadius", "drad", MFnNumericData::kFloat, 1.0, &stat);
	nAttr.setDefault(1.0);
	nAttr.setMin(0.0);
	nAttr.setKeyable(true);
	
	aForceReload = nAttr.create("forceReload", "frel", MFnNumericData::kBoolean, false, &stat);
	nAttr.setDefault(false);
	nAttr.setKeyable(false);
	
	aUpdateCache = nAttr.create("updateCache", "upc", MFnNumericData::kInt, 0);
	nAttr.setHidden(true);
	
	aRenderCachePath = tAttr.create ( "renderCachePath", "rcp", MFnStringData::kString );
	nAttr.setHidden(true);
	
	// attaches our node attributes
	addAttribute ( aUpdateCache );
	addAttribute ( aSize );
	addAttribute ( aFlipYZ );
	addAttribute ( aDrawSkip );
	addAttribute ( aCacheDir );
	addAttribute ( aCacheFile );
	addAttribute ( aCacheOffset );
	addAttribute ( aCacheStatic );
	addAttribute ( aCacheActive );
	addAttribute ( aCacheFormat );
	addAttribute ( aPartioAttributes );
	addAttribute ( aColorFrom );
	addAttribute ( aAlphaFrom );
	addAttribute ( aRadiusFrom );
	addAttribute ( aPointSize );
	addAttribute ( aDefaultPointColor );
	addAttribute ( aDefaultAlpha );
	addAttribute ( aInvertAlpha );
	addAttribute ( aDefaultRadius );
	addAttribute ( aDrawStyle );
	addAttribute ( aForceReload );
	addAttribute ( aRenderCachePath );
	addAttribute ( time );
	
	// set up attribute relations
	attributeAffects ( aCacheDir, aUpdateCache );
	attributeAffects ( aSize, aUpdateCache );
	attributeAffects ( aFlipYZ, aUpdateCache );
	attributeAffects ( aCacheFile, aUpdateCache );
	attributeAffects ( aCacheOffset, aUpdateCache );
	attributeAffects ( aCacheStatic, aUpdateCache );
	attributeAffects ( aCacheFormat, aUpdateCache );
	attributeAffects ( aColorFrom, aUpdateCache );
	attributeAffects ( aAlphaFrom, aUpdateCache );
	attributeAffects ( aRadiusFrom, aUpdateCache );
	attributeAffects ( aPointSize, aUpdateCache );
	attributeAffects ( aDefaultPointColor, aUpdateCache );
	attributeAffects ( aDefaultAlpha, aUpdateCache );
	attributeAffects ( aInvertAlpha, aUpdateCache );
	attributeAffects ( aDefaultRadius, aUpdateCache );
	attributeAffects ( aDrawStyle, aUpdateCache );
	attributeAffects ( aForceReload, aUpdateCache );
	attributeAffects (time, aUpdateCache);
	attributeAffects (time,aRenderCachePath);
	
	return MS::kSuccess;
}

partioVizReaderCache* partioVisualizer::updateParticleCache() {
	GetPlugData(); // force update to run compute function where we want to do all the work
	return &pvCache;
}

///  getPlugData is a util to update the drawing of the UI stuff
bool partioVisualizer::GetPlugData() {
	MObject thisNode = thisMObject();
	int update = 0;
	MPlug updatePlug(thisNode, aUpdateCache );
	updatePlug.getValue( update );
	// make sure dUpdate is recent
	if(update != dUpdate) { 
		dUpdate = update;
		return true;
	} else {
		return false;
	}
	return false;
}

/// COMPUTE FUNCTION
MStatus partioVisualizer::compute( const MPlug& plug, MDataBlock& block ) {
	//retrieve current node state vars
	if( getCurrentState( plug, block ) == MS::kFailure) {
		return MS::kFailure;
	}
	// if cache is not active return
	if (!mContext.cacheActive) {
		return ( MS::kSuccess );
	}
	//if we are not requesting aUpdateCache return
	if (plug != aUpdateCache) {
		return ( MS::kUnknownParameter );
	} else { // update the node
		if( loadCache( plug, block ) == MS::kFailure){
			return MS::kFailure;
		}
		// do we need to reload from cache?
		if ( testForCacheReload( plug, block) ) {
			MStatus reloadStatus = reloadCache( plug, block );
			if ( reloadStatus == MS::kSuccess || reloadStatus == MS::kFailure ){
				return reloadStatus;
			}
		}
		// if we have particles
		if (pvCache.particles) {
			// cache or color changed..
			if  (mContext.cacheChanged || mLast.colorFromIndex != mCurrent.colorFromIndex || mLast.color != mContext.defaultColor) {
				updateColor( plug, block);
			}
			// cache or alpha changed
			if  (mContext.cacheChanged || mCurrent.opacityFromIndex != mLast.alphaFromIndex || mContext.defaultAlpha != mLast.alpha || mCurrent.invertAlpha != mLast.invertAlpha) {
				updateAlpha( plug, block);
			}
			// cache or radius changed
			if  (mContext.cacheChanged || mCurrent.radiusFromIndex != mLast.radiusFromIndex || mContext.defaultRadius != mLast.radius ) {
				updateRadius( plug, block);
			}
		}
	} // the plug data is updated
	// update the AE Controls for attrs in the cache if we have particles
	if (pvCache.particles) { 
		updateAEControls( plug, block);
	}
	block.setClean(plug);
	return MS::kSuccess;
}

MStatus partioVisualizer::getCurrentState(const MPlug& plug, MDataBlock& block ) {
	mCurrent.colorFromIndex = block.inputValue( aColorFrom ).asInt();
	mCurrent.opacityFromIndex = block.inputValue( aAlphaFrom ).asInt();
	mCurrent.radiusFromIndex = block.inputValue( aRadiusFrom ).asInt();
	mContext.drawError = false;
	mContext.cacheActive = block.inputValue(aCacheActive).asBool();
	mCurrent.path = block.inputValue(aCacheDir).asString();
	mCurrent.file = block.inputValue(aCacheFile).asString();
	mContext.newCacheFile = "";
	mContext.renderCacheFile = "";
	mContext.defaultColor = block.inputValue( aDefaultPointColor ).asFloatVector();
	mCurrent.invertAlpha = block.inputValue( aInvertAlpha ).asBool();
	mContext.defaultAlpha = block.inputValue( aDefaultAlpha ).asFloat();
	mCurrent.isStatic = block.inputValue( aCacheStatic ).asBool();
	mCurrent.flipStatus = block.inputValue( aFlipYZ ).asBool();
	mContext.forceReload = block.inputValue( aForceReload ).asBool();
	mContext.defaultRadius = block.inputValue( aDefaultRadius).asFloat();
	mContext.cacheChanged = false;
}

/// load cache from dir
MStatus partioVisualizer::loadCache( const MPlug& plug, MDataBlock& block) {
	MStatus stat;
	if (mCurrent.path  == "" || mCurrent.file == "" ) {
		mContext.drawError = true;
		// TODO: force draw of icon to red or something for feedback
		return MS::kFailure;
	}
	int cacheOffset 	= block.inputValue( aCacheOffset ).asInt();
	short cacheFormat = block.inputValue( aCacheFormat ).asShort();
	int integerTime = block.inputValue( time ).asInt();
	MString renderCachePath = block.inputValue( aRenderCachePath ).asString();
	int cachePadding = 0;
	partio4Maya::updateFileName( mCurrent.file,  mCurrent.path,
							mCurrent.isStatic,  cacheOffset,
							cacheFormat,  integerTime,
							cachePadding, mCurrent.ext,
							mContext.newCacheFile, mContext.renderCacheFile );
	if (renderCachePath != mContext.renderCacheFile || mContext.forceReload ) {
		block.outputValue(aRenderCachePath).setString(mContext.renderCacheFile);
	}
	return MS::kSuccess;
}

/// detect if we need to reload cache
bool partioVisualizer::testForCacheReload( const MPlug& plug, MDataBlock& block) {
	// Cache can change manually by changing one of the parts of the cache input...
	if ( mLast.ext != mCurrent.ext ||
			mLast.path != mCurrent.path ||
			mLast.file != mCurrent.file ||
			mLast.flipStatus != mCurrent.flipStatus ||
			mLast.isStatic != mCurrent.isStatic ||
			mContext.forceReload ) {
		mContext.cacheChanged = true;
		mContext.flipped = false;
		mLast.flipStatus = mCurrent.flipStatus;
		mLast.ext = mCurrent.ext;
		mLast.path = mCurrent.path;
		mLast.file = mCurrent.file;
		mLast.isStatic = mCurrent.isStatic;
		block.outputValue(aForceReload).setBool(false);
	} // or it can change from a time input change
	if(!partio4Maya::partioCacheExists(mContext.newCacheFile.asChar())) {
		pvCache.particles=0; // resets the particles
		pvCache.bbox.clear();
	}
	
	return mContext.newCacheFile != "" && partio4Maya::partioCacheExists(mContext.newCacheFile.asChar()) && (mContext.newCacheFile != mLast.fileLoaded || mContext.forceReload);
}

/// returns fail or success if computation ends, returns invalid parameter if succeeds
MStatus partioVisualizer::reloadCache( const MPlug& plug, MDataBlock& block) {
	mContext.cacheChanged = true;
	mContext.flipped = false;
	MGlobal::displayWarning(MString("PartioVisualizer->Loading: " + mContext.newCacheFile));
	pvCache.particles=0; // resets the particles
	pvCache.particles=read(mContext.newCacheFile.asChar());
	mLast.fileLoaded = mContext.newCacheFile;
	char partCount[50];
	sprintf (partCount, "%d", pvCache.particles->numParticles());
	MGlobal::displayInfo(MString ("PartioVisualizer-> LOADED: ") + partCount + MString (" particles"));
	// if we have 0 particles return success
	if (pvCache.particles->numParticles() == 0) {
		return (MS::kSuccess);
	}
	//allocate memory for particles rgb
	float* floatToRGB = (float*) realloc(pvCache.rgb, pvCache.particles->numParticles()*sizeof(float)*3);
	if (floatToRGB != NULL) {
		pvCache.rgb =  floatToRGB;
	} else {
		free(pvCache.rgb);
		MGlobal::displayError("PartioVisualizer->unable to allocate new memory for particles");
		return (MS::kFailure);
	}
	//allocate memory for particles rgba
	float * newRGBA = (float *) realloc(pvCache.rgba,pvCache.particles->numParticles()*sizeof(float)*4);
	if (newRGBA != NULL) {
		pvCache.rgba =  newRGBA;
	} else {
		free(pvCache.rgba);
		MGlobal::displayError("PartioVisualizer->unable to allocate new memory for particles");
		return (MS::kFailure);
	}
	//clear old info out of our cache object
	pvCache.radius.clear();
	pvCache.radius.setLength(pvCache.particles->numParticles());
	pvCache.bbox.clear();
	//can we find position attribute
	if (!pvCache.particles->attributeInfo("position",pvCache.positionAttr) &&
			!pvCache.particles->attributeInfo("Position",pvCache.positionAttr)) {
		MGlobal::displayError("PartioVisualizer->Failed to find position attribute ");
		return ( MS::kFailure );
	} else {
		// TODO: possibly move this down to take into account the radius as well for a true bounding volume
		// resize the bounding box
		for (int i=0;i<pvCache.particles->numParticles();i++) {
			const float* partioPositions = pvCache.particles->data<float>(pvCache.positionAttr,i);
			MPoint pos (partioPositions[0], partioPositions[1], partioPositions[2]);
			pvCache.bbox.expand(pos);
		}
	}
	block.outputValue(aForceReload).setBool(false);
	block.setClean(aForceReload);
}

/// flip particles
MStatus partioVisualizer::doFlip(const MPlug& plug, MDataBlock& block){
	// TODO:  this does not work when scrubbing yet.. really need to put the  resort of channels into the  partio side as a filter
	// this is only a temporary hack until we start adding  filter functions to partio
	// only flip the axis stuff if we need to
	float* floatToPos = (float *) realloc(pvCache.flipPos, pvCache.particles->numParticles()*sizeof(float)*3);
	if (floatToPos != NULL){
		pvCache.flipPos =  floatToPos;
	}
	else{
		free(pvCache.flipPos);
		MGlobal::displayError("PartioVisualizer->unable to allocate new memory for flip particles");
		return (MS::kFailure);
	}
	for (int i=0;i<pvCache.particles->numParticles();i++){
		const float * attrVal = pvCache.particles->data<float>(pvCache.positionAttr,i);
		pvCache.flipPos[(i*3)] = attrVal[0];
		pvCache.flipPos[((i*3)+1)] 	= -attrVal[2];
		pvCache.flipPos[((i*3)+2)] 	= attrVal[1];
	}
	mContext.flipped = true;
}

/// update the particles color
void partioVisualizer::updateColor( const MPlug& plug, MDataBlock& block) {
	int numAttrs = pvCache.particles->numAttributes();
	// if our attributes are outdated
	if (mCurrent.colorFromIndex+1 > numAttrs || mCurrent.opacityFromIndex+1 > numAttrs || mCurrent.radiusFromIndex+1 > numAttrs) {
		// reset the attrs
		block.outputValue(aColorFrom).setInt(-1);
		block.setClean(aColorFrom);
		block.outputValue(aAlphaFrom).setInt(-1);
		block.setClean(aAlphaFrom);
		block.outputValue(aRadiusFrom).setInt(-1);
		block.setClean(aRadiusFrom);
		// reload the attrs
		mCurrent.colorFromIndex  = block.inputValue( aColorFrom ).asInt();
		mCurrent.opacityFromIndex= block.inputValue( aAlphaFrom ).asInt();
		mCurrent.radiusFromIndex = block.inputValue( aRadiusFrom ).asInt();
	}
	// is there color input?
	if(mCurrent.colorFromIndex >=0) {
		pvCache.particles->attributeInfo(mCurrent.colorFromIndex,pvCache.colorAttr);
		// VECTOR or  4+ element float attrs
		// assuming first 3 elements are rgb
		if (pvCache.colorAttr.type == VECTOR || pvCache.colorAttr.count > 3) {
			for (int i=0;i<pvCache.particles->numParticles();i++) {
				const float * attrVal = pvCache.particles->data<float>(pvCache.colorAttr,i);
				pvCache.rgb[(i*3)] = attrVal[0];
				pvCache.rgb[((i*3)+1)] = attrVal[1];
				pvCache.rgb[((i*3)+2)] = attrVal[2];
				pvCache.rgba[i*4] = attrVal[0];
				pvCache.rgba[(i*4)+1] = attrVal[1];
				pvCache.rgba[(i*4)+2] = attrVal[2];
			}
		} else { //single float
			for (int i=0;i<pvCache.particles->numParticles();i++) {
				const float * attrVal = pvCache.particles->data<float>(pvCache.colorAttr,i);
				pvCache.rgb[(i*3)] = pvCache.rgb[((i*3)+1)] = pvCache.rgb[((i*3)+2)] 	= attrVal[0];
				pvCache.rgba[i*4]  = pvCache.rgba[(i*4)+1] 	= pvCache.rgba[(i*4)+2] 	= attrVal[0];
			}
		}
	} else { //set default color
		for (int i=0;i<pvCache.particles->numParticles();i++) {
			pvCache.rgb[(i*3)] = mContext.defaultColor[0];
			pvCache.rgb[((i*3)+1)] = mContext.defaultColor[1];
			pvCache.rgb[((i*3)+2)] = mContext.defaultColor[2];
			pvCache.rgba[i*4] = mContext.defaultColor[0];
			pvCache.rgba[(i*4)+1] = mContext.defaultColor[1];
			pvCache.rgba[(i*4)+2] = mContext.defaultColor[2];
		}
	}
	//keep track for change detection
	mLast.colorFromIndex = mCurrent.colorFromIndex;
	mLast.color = mContext.defaultColor;
}

/// update alpha vals in particles
void partioVisualizer::updateAlpha( const MPlug& plug, MDataBlock& block){
	// there is input opacity
	if (mCurrent.opacityFromIndex >=0) {
		pvCache.particles->attributeInfo(mCurrent.opacityFromIndex,pvCache.opacityAttr);
		// single float value for opacity
		if (pvCache.opacityAttr.count == 1) { 
			for (int i=0;i<pvCache.particles->numParticles();i++) {
				const float* attrVal = pvCache.particles->data<float>(pvCache.opacityAttr,i);
				float temp = attrVal[0];
				if (mCurrent.invertAlpha) {
					temp = float(1.0-temp);
				}
				pvCache.rgba[(i*4)+3] = temp;
			}
		} else { 
			// we have an  RGBA 4 float attribute ?
			if (pvCache.opacityAttr.count == 4) {  
				for (int i=0;i<pvCache.particles->numParticles();i++) {
					const float* attrVal = pvCache.particles->data<float>(pvCache.opacityAttr,i);
					float temp = attrVal[3];
					if (mCurrent.invertAlpha) {
						temp = float(1.0-temp);
					}
					pvCache.rgba[(i*4)+3] = temp;
				}
			} else {
				for (int i=0;i<pvCache.particles->numParticles();i++) {
					const float* attrVal = pvCache.particles->data<float>(pvCache.opacityAttr,i);
					float lum = float((attrVal[0]*0.2126)+(attrVal[1]*0.7152)+(attrVal[2]*.0722));
					if (mCurrent.invertAlpha) {
						lum = float(1.0-lum);
					}
					pvCache.rgba[(i*4)+3] =  lum;
				}
			}
		}
	} else { // there was no input opacity
		mLast.alpha = mContext.defaultAlpha;
		if (mCurrent.invertAlpha) {
			mLast.alpha= 1-mContext.defaultAlpha;
		}
		for (int i=0;i<pvCache.particles->numParticles();i++) {
			pvCache.rgba[(i*4)+3] = mLast.alpha;
		}
	}// input opacity check
	//keep track for change detection
	mLast.alpha = mContext.defaultAlpha;
	mLast.alphaFromIndex = mCurrent.opacityFromIndex;
	mLast.invertAlpha = mCurrent.invertAlpha;
}

/// update radius attribute on cache
void partioVisualizer::updateRadius( const MPlug& plug, MDataBlock& block){
	// if there was an input radius
	if (mCurrent.radiusFromIndex >=0) {
		pvCache.particles->attributeInfo(mCurrent.radiusFromIndex,pvCache.radiusAttr);
		// single float value for radius
		if (pvCache.radiusAttr.count == 1) { 
			for (int i=0;i<pvCache.particles->numParticles();i++) {
				const float * attrVal = pvCache.particles->data<float>(pvCache.radiusAttr,i);
				pvCache.radius[i] = attrVal[0] * mContext.defaultRadius;
			}
		} else {
			for (int i=0;i<pvCache.particles->numParticles();i++) {
				const float * attrVal = pvCache.particles->data<float>(pvCache.radiusAttr,i);
				float lum = float((attrVal[0]*0.2126)+(attrVal[1]*0.7152)+(attrVal[2]*.0722));
				pvCache.radius[i] =  lum * mContext.defaultRadius;
			}
		}
	} else { // there was no input radius
		mLast.radius = mContext.defaultRadius;
		for (int i=0;i<pvCache.particles->numParticles();i++) {
			pvCache.radius[i] = mLast.radius;
		}
	} // input radius check
	// keep track for change detection
	mLast.radius = mContext.defaultRadius;
	mLast.radiusFromIndex = mCurrent.radiusFromIndex;
}

/// Updates attribute editor controls
void partioVisualizer::updateAEControls(const MPlug& plug, MDataBlock& block){
	unsigned int numAttr=pvCache.particles->numAttributes();
	MPlug zPlug (thisMObject(), aPartioAttributes);
	if ((mCurrent.colorFromIndex+1) > zPlug.numElements()) {
		block.outputValue(aColorFrom).setInt(-1);
	}
	if ((mCurrent.opacityFromIndex+1) > zPlug.numElements()) {
		block.outputValue(aAlphaFrom).setInt(-1);
	}
	if ((mCurrent.radiusFromIndex+1) > zPlug.numElements()) {
		block.outputValue(aRadiusFrom).setInt(-1);
	}
	if (mContext.cacheChanged || zPlug.numElements() != numAttr) {
		mContext.attributeList.clear();
		for (unsigned int i=0;i<numAttr;i++) {
			ParticleAttribute attr;
			pvCache.particles->attributeInfo(i,attr);
			// note, casting string to char array
			char *temp;
			temp = new char[(attr.name).length()+1];
			strcpy (temp, attr.name.c_str());
			MString  mStringAttrName("");
			mStringAttrName += MString(temp);
			zPlug.selectAncestorLogicalIndex(i,aPartioAttributes);
			zPlug.setValue(MString(temp));
			mContext.attributeList.append(mStringAttrName);
			delete [] temp;
		}
	}
}

/// procs to override bounding box mode
bool partioVisualizer::isBounded() const {
    return true;
}

MBoundingBox partioVisualizer::boundingBox() const {
	// Returns the bounding box for the shape.
	partioVisualizer* nonConstThis = const_cast<partioVisualizer*>(this);
	partioVizReaderCache* geom = nonConstThis->updateParticleCache();
	
	MPoint corner1 = geom->bbox.min();
	MPoint corner2 = geom->bbox.max();
	return MBoundingBox( corner1, corner2 );
}

// Select function. Gets called when the bbox for the object is selected.
// This function just selects the object without doing any intersection tests.
bool partioVisualizerUI::select( MSelectInfo &selectInfo, MSelectionList &selectionList, MPointArray &worldSpaceSelectPts ) const {
	MSelectionMask priorityMask( MSelectionMask::kSelectObjectsMask );
	MSelectionList item;
	item.add( selectInfo.selectPath() );
	MPoint xformedPt;
	selectInfo.addSelection( item, xformedPt, selectionList, worldSpaceSelectPts, priorityMask, false );
	return true;
}

// note the "const" at the end, its different than other draw calls
void partioVisualizerUI::draw( const MDrawRequest& request, M3dView& view ) const {
	MDrawData data = request.drawData();
	partioVisualizer* shapeNode = (partioVisualizer*) surfaceShape();
	partioVizReaderCache* cache = (partioVizReaderCache*) data.geometry();
	MObject thisNode = shapeNode->thisMObject();
	MPlug sizePlug( thisNode, shapeNode->aSize );
	MDistance sizeVal;
	sizePlug.getValue( sizeVal );
	
	shapeNode->multiplier= (float) sizeVal.asCentimeters();
	int drawStyle = DRAW_STYLE_POINTS;
	MPlug drawStylePlug( thisNode, shapeNode->aDrawStyle );
	drawStylePlug.getValue( drawStyle );
	
	view.beginGL();
		if (drawStyle == DRAW_STYLE_BOUNDING_BOX || view.displayStyle() == M3dView::kBoundingBox ) {
			drawBoundingBox();
		} else {
			drawPartio(cache,drawStyle);
		}
		partio4Maya::drawPartioLogo(shapeNode->multiplier);
	view.endGL();
}

/// DRAW Bounding box
void  partioVisualizerUI::drawBoundingBox() const {
	partioVisualizer* shapeNode = (partioVisualizer*) surfaceShape();
	MPoint  bboxMin = shapeNode->pvCache.bbox.min();
	MPoint  bboxMax = shapeNode->pvCache.bbox.max();
	
	float xMin = float(bboxMin.x);
	float yMin = float(bboxMin.y);
	float zMin = float(bboxMin.z);
	float xMax = float(bboxMax.x);
	float yMax = float(bboxMax.y);
	float zMax = float(bboxMax.z);
	glBegin (GL_LINES);
		glColor3f(1.0f,0.5f,0.5f);
		
		glVertex3f (xMin,yMin,zMax);
		glVertex3f (xMax,yMin,zMax);
		
		glVertex3f (xMin,yMin,zMin);
		glVertex3f (xMax,yMin,zMin);
		
		glVertex3f (xMin,yMin,zMax);
		glVertex3f (xMin,yMin,zMin);
		
		glVertex3f (xMax,yMin,zMax);
		glVertex3f (xMax,yMin,zMin);
		
		glVertex3f (xMin,yMax,zMin);
		glVertex3f (xMin,yMax,zMax);
		
		glVertex3f (xMax,yMax,zMax);
		glVertex3f (xMax,yMax,zMin);
		
		glVertex3f (xMin,yMax,zMax);
		glVertex3f (xMax,yMax,zMax);
		
		glVertex3f (xMin,yMax,zMin);
		glVertex3f (xMax,yMax,zMin);
		
		glVertex3f (xMin,yMax,zMin);
		glVertex3f (xMin,yMin,zMin);
		
		glVertex3f (xMax,yMax,zMin);
		glVertex3f (xMax,yMin,zMin);
		
		glVertex3f (xMin,yMax,zMax);
		glVertex3f (xMin,yMin,zMax);
		
		glVertex3f (xMax,yMax,zMax);
		glVertex3f (xMax,yMin,zMax);
	glEnd();
}

/// DRAW PARTIO
void partioVisualizerUI::drawPartio(partioVizReaderCache* pvCache, int drawStyle) const {
	partioVisualizer* shapeNode = (partioVisualizer*) surfaceShape();
	MObject thisNode = shapeNode->thisMObject();
	MPlug drawSkipPlug( thisNode, shapeNode->aDrawSkip );
	// get values from plug
	int drawSkipVal;
	drawSkipPlug.getValue( drawSkipVal );
	MPlug flipYZPlug( thisNode, shapeNode->aFlipYZ );
	bool flipYZVal;
	flipYZPlug.getValue( flipYZVal );
	int stride =  3*sizeof(float)*(drawSkipVal);
	MPlug pointSizePlug( thisNode, shapeNode->aPointSize );
	float pointSizeVal;
	pointSizePlug.getValue( pointSizeVal );
	MPlug colorFromPlug( thisNode, shapeNode->aColorFrom);
	int colorFromVal;
	colorFromPlug.getValue( colorFromVal );
	MPlug alphaFromPlug( thisNode, shapeNode->aAlphaFrom);
	int alphaFromVal;
	alphaFromPlug.getValue( alphaFromVal );
	MPlug defaultAlphaPlug( thisNode, shapeNode->aDefaultAlpha);
	float defaultAlphaVal;
	defaultAlphaPlug.getValue( defaultAlphaVal );
	//if we have particles draw them
	if (pvCache->particles) {
		struct Point { float p[3]; };
		glPushAttrib(GL_CURRENT_BIT);
		//testing settings
		if (alphaFromVal >=0 || defaultAlphaVal < 1) { 
			glDepthMask(true);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glEnable(GL_POINT_SMOOTH);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		// draw some point arrays!
		if(drawStyle == DRAW_STYLE_POINTS) {
			glEnableClientState( GL_VERTEX_ARRAY );
			glEnableClientState( GL_COLOR_ARRAY );
			
			glPointSize(pointSizeVal);
			if (pvCache->particles->numParticles() > 0){
				// now setup the position/color/alpha output pointers
				const float * partioPositions = pvCache->particles->data<float>(pvCache->positionAttr,0);
				glVertexPointer( 3, GL_FLOAT, stride, partioPositions );
				if (defaultAlphaVal < 1 || alphaFromVal >=0) {  // use transparency switch 
					glColorPointer(  4, GL_FLOAT, stride, pvCache->rgba );
				} else {
					glColorPointer(  3, GL_FLOAT, stride, pvCache->rgb );
				}
				glDrawArrays( GL_POINTS, 0, (pvCache->particles->numParticles()/(drawSkipVal+1)) );
			}
			glDisableClientState( GL_VERTEX_ARRAY );
			glDisableClientState( GL_COLOR_ARRAY );
		}
		glDisable(GL_POINT_SMOOTH);
		glPopAttrib();
	}
}

partioVisualizerUI::partioVisualizerUI() {}
partioVisualizerUI::~partioVisualizerUI() {}

void* partioVisualizerUI::creator() {
	return new partioVisualizerUI();
}

void partioVisualizerUI::getDrawRequests(const MDrawInfo & info, bool objectAndActiveOnly, MDrawRequestQueue & queue) {
	MDrawData data;
	MDrawRequest request = info.getPrototype(*this);
	partioVisualizer* shapeNode = (partioVisualizer*) surfaceShape();
	partioVizReaderCache* geom = shapeNode->updateParticleCache();
	getDrawData(geom, data);
	request.setDrawData(data);
	// Are we displaying locators?
	if (!info.objectDisplayStatus(M3dView::kDisplayLocators)) {
		return;
	}
	//set color based on draw type
	M3dView::DisplayStatus displayStatus = info.displayStatus();
	M3dView::ColorTable activeColorTable = M3dView::kActiveColors;
	M3dView::ColorTable dormantColorTable = M3dView::kDormantColors;
	switch (displayStatus) {
		case M3dView::kLead:
			request.setColor(LEAD_COLOR, activeColorTable);
			break;
		case M3dView::kActive:
			request.setColor(ACTIVE_COLOR, activeColorTable);
			break;
		case M3dView::kActiveAffected:
			request.setColor(ACTIVE_AFFECTED_COLOR, activeColorTable);
			break;
		case M3dView::kDormant:
			request.setColor(DORMANT_COLOR, dormantColorTable);
			break;
		case M3dView::kHilite:
			request.setColor(HILITE_COLOR, activeColorTable);
			break;
		default:
			break;
	}
	queue.add(request);
}

void partioVisualizerUI::drawBillboardCircleAtPoint(MVector position, float radius, int num_segments, int drawType) const {
	float m[16];
	int j,k;
	glPushMatrix();
	glTranslatef( (GLfloat) position.x, (GLfloat) position.y, (GLfloat) position.z);
	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	// set the identity matrix
	for (j = 0; j<3; j++) {
		for (k = 0; k<3; k++) {
			if (j==k) {
				m[j*4+k] = 1.0;
			} else {
				m[j*4+k] = 0.0;
			}
		}
	}
	glLoadMatrixf(m);
	float theta =(float)(  2 * 3.1415926 / float(num_segments)  );
	float tangetial_factor = tanf(theta);//calculate the tangential factor
	float radial_factor = cosf(theta);//calculate the radial factor
	float x = radius;//we start at angle = 0
	float y = 0;
	if (drawType == DRAW_STYLE_RADIUS) {
		glBegin(GL_LINE_LOOP);
	} else { // (drawType == DRAW_STYLE_DISK)
		glBegin(GL_POLYGON);
	}
		for(int ii = 0; ii < num_segments; ii++) {
			glVertex2f(x, y);//output vertex
			//calculate the tangential vector
			//remember, the radial vector is (x, y)
			//to get the tangential vector we flip those coordinates and negate one of them
			float tx = -y;
			float ty = x;
			//add the tangential vector
			x += tx * tangetial_factor;
			y += ty * tangetial_factor;
			//correct using the radial factor
			x *= radial_factor;
			y *= radial_factor;
		}
	glEnd();
	glPopMatrix();
}