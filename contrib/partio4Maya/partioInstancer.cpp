/* partio4Maya  5/02/2012, John Cassella  http://luma-pictures.com and  http://redpawfx.com
PARTIO Instancer
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

#include "partioInstancer.h"

static MGLFunctionTable *gGLFT = NULL;

#define ID_PARTIOINSTANCER  0x00116ED5 // id is registered with autodesk no need to change
#define LEAD_COLOR 18 // green
#define ACTIVE_COLOR 15 // white
#define ACTIVE_AFFECTED_COLOR 8 // purple
#define DORMANT_COLOR 4 // blue
#define HILITE_COLOR 17 // pale blue

#define DRAW_STYLE_POINTS 0
#define DRAW_STYLE_LABEL 1
#define DRAW_STYLE_BOUNDING_BOX 3

using namespace Partio;
using namespace std;

/// PARTIO INSTANCER
/*
Names and types of all array attributes  the maya instancer looks for
used by the geometry instancer nodes:  ( * = currently implemented)
position (vectorArray)  *
scale (vectorArray) *
shear (vectorArray)
visibility (doubleArray)
objectIndex (doubleArray) *
rotationType (doubleArray)
rotation (vectorArray) *
aimDirection (vectorArray)
aimPosition (vectorArray)
aimAxis (vectorArray)
aimUpAxis (vectorArray)
aimWorldUp (vectorArray)
age (doubleArray)
id (doubleArray)
*/

MTypeId partioInstancer::id( ID_PARTIOINSTANCER );

MObject partioInstancer::time;
MObject partioInstancer::aUpdateCache;
MObject partioInstancer::aSize;         // The size of the logo
MObject partioInstancer::aFlipYZ;
MObject partioInstancer::aCacheDir;
MObject partioInstancer::aCacheFile;
MObject partioInstancer::aUseTransform;
MObject partioInstancer::aCacheActive;
MObject partioInstancer::aCacheOffset;
MObject partioInstancer::aCacheStatic;
MObject partioInstancer::aCacheFormat;
MObject partioInstancer::aPartioAttributes;
MObject partioInstancer::aPointSize;
MObject partioInstancer::aDrawStyle;
MObject partioInstancer::aForceReload;
MObject partioInstancer::aRenderCachePath;
MObject partioInstancer::aRotationFrom;
MObject partioInstancer::aScaleFrom;
MObject partioInstancer::aIndexFrom;
MObject partioInstancer::aShaderIndexFrom;
MObject partioInstancer::aInMeshInstances;
MObject partioInstancer::aOutMesh;
MObject partioInstancer::aInstanceData;
MObject partioInstancer::aComputeVeloPos;

partioInstReaderCache::partioInstReaderCache():
		token(0),
		bbox(MBoundingBox(MPoint(0,0,0,0),MPoint(0,0,0,0))),
		dList(0),
		particles(NULL),
		flipPos(NULL) {}


partioInstancer::partioInstancer():  
		mLastFileLoaded(""),
		mLastPath(""),
		mLastFile(""),
		mLastExt(""),
		mLastRotationFromIndex(-1),
		mLastScaleFromIndex(-1),
		mLastIndexFromIndex(-1),
		mLastShaderIndexFromIndex(-1),
		cacheChanged(false),
		frameChanged(false),
		multiplier(1.0),
		canMotionBlur(false),
		mFlipped(false) 
{ /// Constructor
	pvCache.particles = NULL;
	pvCache.flipPos = (float *) malloc(sizeof(float));
	// create the instanceData object
	MStatus stat;
	pvCache.instanceDataObj = pvCache.instanceData.create ( &stat );
	CHECK_MSTATUS(stat);
}

partioInstancer::~partioInstancer() 
{ /// DESTRUCTOR
	//free memory
	if (pvCache.particles){
		pvCache.particles->release();
	}
	free(pvCache.flipPos);
	//remove callbacks
	MSceneMessage::removeCallback( partioInstancerOpenCallback );
	MSceneMessage::removeCallback( partioInstancerImportCallback );
	MSceneMessage::removeCallback( partioInstancerReferenceCallback );
}

void* partioInstancer::creator() 
{
	return new partioInstancer;
}

void partioInstancer::postConstructor() 
{ /// POST CONSTRUCTOR
	setRenderable(true);
	// add callbacks
	partioInstancerOpenCallback = MSceneMessage::addCallback(MSceneMessage::kAfterOpen, partioInstancer::reInit, this);
	partioInstancerImportCallback = MSceneMessage::addCallback(MSceneMessage::kAfterImport, partioInstancer::reInit, this);
	partioInstancerReferenceCallback = MSceneMessage::addCallback(MSceneMessage::kAfterReference, partioInstancer::reInit, this);
}

void partioInstancer::initCallback()
{ /// init after opening
	MObject tmo = thisMObject();
	short extENum;
	mLastExt = partio4Maya::setExt(extENum);
	cacheChanged = false;
	
	MPlug(tmo, aCacheFormat).getValue(extENum);
	MPlug(tmo,aCacheDir).getValue(mLastPath);
	MPlug(tmo,aCacheFile).getValue(mLastFile);
	MPlug(tmo,aSize).getValue(multiplier);
}

void partioInstancer::reInit(void *data) 
{
	partioInstancer  *instNode = (partioInstancer*) data;
	instNode->initCallback();
}


MStatus partioInstancer::initialize() 
{
	MFnEnumAttribute eAttr;
	MFnUnitAttribute uAttr;
	MFnNumericAttribute nAttr;
	MFnTypedAttribute tAttr;
	MStatus stat;
	
	// create attributes
	time = uAttr.create("time", "tm", MFnUnitAttribute::kTime, 0.0, &stat );
	uAttr.setKeyable( true );
	
	aSize = uAttr.create( "iconSize", "isz", MFnUnitAttribute::kDistance );
	uAttr.setDefault( 0.25 );
	
	aFlipYZ = nAttr.create( "flipYZ", "fyz", MFnNumericData::kBoolean);
	nAttr.setDefault ( false );
	nAttr.setKeyable ( true );
	
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
	eAttr.addField("points", DRAW_STYLE_POINTS);
	//eAttr.addField("index#",	1);
	//eAttr.addField("spheres",	2);
	eAttr.addField("boundingBox", DRAW_STYLE_BOUNDING_BOX);
	eAttr.setDefault(DRAW_STYLE_POINTS);
	eAttr.setChannelBox(true);
	
	aUseTransform = nAttr.create("useTransform", "utxfm", MFnNumericData::kBoolean, false, &stat);
	nAttr.setKeyable(true);
	
	aPartioAttributes = tAttr.create ("partioCacheAttributes", "pioCAts", MFnStringData::kString);
	tAttr.setArray(true);
	tAttr.setDisconnectBehavior(MFnAttribute::kDelete);
	
	aPointSize = nAttr.create("pointSize", "ptsz", MFnNumericData::kInt, 2, &stat);
	nAttr.setDefault(2);
	nAttr.setKeyable(true);
	
	aForceReload = nAttr.create("forceReload", "frel", MFnNumericData::kBoolean, false, &stat);
	nAttr.setDefault(false);
	nAttr.setKeyable(false);
	
	aUpdateCache = nAttr.create("updateCache", "upc", MFnNumericData::kInt, 0);
	nAttr.setHidden(true);
	
	aRenderCachePath = tAttr.create ( "renderCachePath", "rcp", MFnStringData::kString );
	tAttr.setHidden(true);
	
	aRotationFrom = nAttr.create("rotationFrom", "rfrm", MFnNumericData::kInt, -1, &stat);
	nAttr.setDefault(-1);
	nAttr.setKeyable(true);
	
	aScaleFrom = nAttr.create("scaleFrom", "sfrm", MFnNumericData::kInt, -1, &stat);
	nAttr.setDefault(-1);
	nAttr.setKeyable(true);
	
	aIndexFrom = nAttr.create("indexFrom", "ifrm", MFnNumericData::kInt, -1, &stat);
	nAttr.setDefault(-1);
	nAttr.setKeyable(true);

	aShaderIndexFrom = nAttr.create("shaderIndexFrom", "sifrm", MFnNumericData::kInt, -1, &stat);
	nAttr.setDefault(-1);
	nAttr.setKeyable(true);
	
	aInstanceData = tAttr.create( "instanceData", "instd", MFnArrayAttrsData::kDynArrayAttrs, &stat);
	tAttr.setKeyable(false);
	tAttr.setStorable(false);
	tAttr.setHidden(false);
	tAttr.setReadable(true);
	
	aComputeVeloPos = nAttr.create("computeVeloPos", "cvp", MFnNumericData::kBoolean, false, &stat);
	tAttr.setKeyable(false);
	
	// attach the attributes
	addAttribute ( aUpdateCache );
	addAttribute ( aSize );
	addAttribute ( aFlipYZ );
	addAttribute ( aCacheDir );
	addAttribute ( aCacheFile );
	addAttribute ( aCacheOffset );
	addAttribute ( aCacheStatic );
	addAttribute ( aCacheActive );
	addAttribute ( aCacheFormat );
	addAttribute ( aPartioAttributes );
	addAttribute ( aPointSize );
	addAttribute ( aDrawStyle );
	addAttribute ( aForceReload );
	addAttribute ( aRenderCachePath );
	addAttribute ( aRotationFrom );
	addAttribute ( aScaleFrom );
	addAttribute ( aIndexFrom );
	addAttribute ( aShaderIndexFrom );
	addAttribute ( aInstanceData );
	addAttribute ( aComputeVeloPos );
	addAttribute ( time );
	
	// setup attribute relations
	attributeAffects ( aCacheDir, aUpdateCache );
	attributeAffects ( aSize, aUpdateCache );
	attributeAffects ( aFlipYZ, aUpdateCache );
	attributeAffects ( aCacheFile, aUpdateCache );
	attributeAffects ( aCacheOffset, aUpdateCache );
	attributeAffects ( aCacheStatic, aUpdateCache );
	attributeAffects ( aCacheFormat, aUpdateCache );
	attributeAffects ( aPointSize, aUpdateCache );
	attributeAffects ( aDrawStyle, aUpdateCache );
	attributeAffects ( aForceReload, aUpdateCache );
	attributeAffects ( aInstanceData, aUpdateCache );
	attributeAffects ( aRotationFrom, aUpdateCache );
	attributeAffects ( aScaleFrom, aUpdateCache );
	attributeAffects ( aIndexFrom, aUpdateCache );
	attributeAffects ( aShaderIndexFrom, aUpdateCache );
	attributeAffects ( aComputeVeloPos, aUpdateCache );
	attributeAffects (time, aUpdateCache);
	attributeAffects (time, aInstanceData);
	
	return MS::kSuccess;
}

partioInstReaderCache* partioInstancer::updateParticleCache() 
{
	GetPlugData(); // force update to run compute function where we want to do all the work
	return &pvCache;
}

MStatus partioInstancer::compute( const MPlug& plug, MDataBlock& block ) 
{ /// COMPUTE FUNCTION
	MStatus stat;
	int rotationFromIndex = block.inputValue( aRotationFrom ).asInt();
	int scaleFromIndex	 = block.inputValue( aScaleFrom ).asInt();
	int indexFromIndex = block.inputValue( aIndexFrom ).asInt();
	int shaderIndexFromIndex = block.inputValue( aShaderIndexFrom).asInt();
	bool cacheActive = block.inputValue(aCacheActive).asBool();

	if (!cacheActive) 
	{
		return ( MS::kSuccess );
	}
	
	if (plug != aUpdateCache && plug != aInstanceData) 
	{ // Determine if we are requesting the output plug for this node.
		return ( MS::kUnknownParameter );
	} else 
	{
		MString cacheDir = block.inputValue(aCacheDir).asString();
		MString cacheFile = block.inputValue(aCacheFile).asString();
		
		if (cacheDir  == "" || cacheFile == "" )
		{ // TODO: indicate failure visually
			return ( MS::kFailure );
		}

		bool cacheStatic 			= block.inputValue( aCacheStatic ).asBool();
		int cacheOffset 				= block.inputValue( aCacheOffset ).asInt();
		short cacheFormat 			= block.inputValue( aCacheFormat ).asShort();
		bool forceReload 			= block.inputValue( aForceReload ).asBool();
		MTime inputTime 			= block.inputValue(time).asTime();
		bool flipYZ 				= block.inputValue( aFlipYZ ).asBool();
		MString renderCachePath 		= block.inputValue( aRenderCachePath ).asString();
		bool computeMotionBlur 		= block.inputValue( aComputeVeloPos).asBool();
		
		int fps = (float)(MTime(1.0, MTime::kSeconds).asUnits(MTime::uiUnit()));
		int integerTime 	= (int)floor((inputTime.value())+.52);
		float deltaTime  = float(inputTime.value() - integerTime);
		bool motionBlurStep = false;
		
		// motion  blur rounding  frame logic
		if ((deltaTime < 1 || deltaTime > -1)&& deltaTime !=0) 
		{ // motion blur step?
			motionBlurStep = true;
		}
		
		MString formatExt = "";
		int cachePadding = 0;
		
		MString newCacheFile = "";
		MString renderCacheFile = "";
		
		partio4Maya::updateFileName( cacheFile,  cacheDir,
								cacheStatic,  cacheOffset,
								cacheFormat,  integerTime,
								cachePadding, formatExt,
								newCacheFile, renderCacheFile);
		
		if (renderCachePath != newCacheFile || renderCachePath != mLastFileLoaded ) 
		{
			block.outputValue(aRenderCachePath).setString(newCacheFile);
		}
		cacheChanged = false;
		if (mLastExt != formatExt || mLastPath != cacheDir || mLastFile != cacheFile ||  mLastFlipStatus  != flipYZ || forceReload ) 
		{ // Cache can change manually by changing one of the parts of the cache input...
			cacheChanged = true;
			mFlipped = false;
			mLastFlipStatus = flipYZ;
			mLastExt = formatExt;
			mLastPath = cacheDir;
			mLastFile = cacheFile;
			block.outputValue(aForceReload).setBool(false);
		}
		if(!partio4Maya::partioCacheExists(newCacheFile.asChar())) 
		{ // or it can change from a time input change
			pvCache.particles=0; // resets the particles
			pvCache.bbox.clear();
		}
		if ( newCacheFile != "" && partio4Maya::partioCacheExists(newCacheFile.asChar()) && (newCacheFile != mLastFileLoaded || forceReload) ) 
		{ // if the file changed or we force reload
			cacheChanged = true;
			mFlipped = false;
			MGlobal::displayWarning(MString("PartioInstancer->Loading: " + newCacheFile));
			pvCache.particles=0; // resets the particles
			pvCache.particles=read(newCacheFile.asChar());
			mLastFileLoaded = newCacheFile;
			char partCount[50];
			sprintf (partCount, "%d", pvCache.particles->numParticles());
			MGlobal::displayInfo(MString ("PartioInstancer-> LOADED: ") + partCount + MString (" particles"));
			
			if (pvCache.particles->numParticles() == 0){
				return (MS::kSuccess);
			}
			
			block.outputValue(aForceReload).setBool(false);
			block.setClean(aForceReload);
			pvCache.instanceData.clear();
		} // we had to reload cache
		if (pvCache.particles)
		{ // if there are particles
			MFnArrayAttrsData::Type vectorType(MFnArrayAttrsData::kVectorArray);
			MFnArrayAttrsData::Type doubleType(MFnArrayAttrsData::kDoubleArray);
			canMotionBlur = false;
			if(computeMotionBlur) 
			{ // if we want to moblur, can we?
				if ((pvCache.particles->attributeInfo("velocity",pvCache.velocityAttr) ||
						pvCache.particles->attributeInfo("Velocity",pvCache.velocityAttr)) ||
						pvCache.particles->attributeInfo("V", pvCache.velocityAttr))
				{ // if we have velocity enable
					canMotionBlur = true;
				} else 
				{
					MGlobal::displayWarning("PartioInstancer->Failed to find velocity attribute motion blur will be impaired ");
				}
			}//compute motion blur?
			pvCache.bbox.clear();
			if (!pvCache.particles->attributeInfo("position",pvCache.positionAttr) &&
					!pvCache.particles->attributeInfo("Position",pvCache.positionAttr)) 
			{ // if we have position
				MGlobal::displayError("PartioInstancer->Failed to find position attribute ");
				return ( MS::kFailure );
			} else 
			{
				// set up position array
				MVectorArray  positionArray;
				if(pvCache.instanceData.checkArrayExist("position",vectorType)) 
				{
					positionArray = pvCache.instanceData.getVectorData(MString("position"),&stat);
					CHECK_MSTATUS(stat);
				} else 
				{
					positionArray = pvCache.instanceData.vectorArray(MString("position"),&stat);
					CHECK_MSTATUS(stat);
				}
				positionArray.clear();
				for (int i=0;i<pvCache.particles->numParticles();i++) 
				{ // resize the bounding box
					const float * partioPositions = pvCache.particles->data<float>(pvCache.positionAttr,i);
					MPoint pos (partioPositions[0], partioPositions[1], partioPositions[2]);
					if(canMotionBlur)
					{ // if we can moblur and want to moblur
						const float * vel = pvCache.particles->data<float>(pvCache.velocityAttr,i);
						MVector velo(vel[0],vel[1],vel[2]);
						if (motionBlurStep)
						{
							int mFps = (float)(MTime(1.0, MTime::kSeconds).asUnits(MTime::uiUnit()));
							pos += (velo/mFps)*deltaTime;
						}
					}
					positionArray.append(pos);
					pvCache.bbox.expand(pos);
				}
			} // we had a position
			
			if (!pvCache.particles->attributeInfo("id",pvCache.idAttr) &&
					!pvCache.particles->attributeInfo("ID",pvCache.idAttr) &&
					!pvCache.particles->attributeInfo("particleId",pvCache.idAttr) &&
					!pvCache.particles->attributeInfo("ParticleId",pvCache.idAttr))
			{ // if we dont have an id attrib, fail
				MGlobal::displayError("PartioInstancer->Failed to find id attribute ");
				return ( MS::kFailure );
			}
			// set up id array
			MDoubleArray  idArray;
			if(pvCache.instanceData.checkArrayExist("id",doubleType))
			{
				idArray = pvCache.instanceData.getDoubleData(MString("id"),&stat);
				CHECK_MSTATUS(stat);
			}else 
			{
				idArray = pvCache.instanceData.doubleArray(MString("id"),&stat);
				CHECK_MSTATUS(stat);
			}
			idArray.clear();
			for (int i=0;i<pvCache.particles->numParticles();i++)
			{ // populate id array
				const int* attrVal = pvCache.particles->data<int>(pvCache.idAttr,i);
				idArray.append((double)attrVal[0]);
			}
			
			/*
			/// TODO:  this does not work when scrubbing yet.. really need to put the  resort of channels into the  partio side as a filter
			/// this is only a temporary hack until we start adding  filter functions to partio

			// only flip the axis stuff if we need to
			if ( cacheChanged && flipYZ  && !mFlipped )
			{
				float * floatToPos = (float *) realloc(pvCache.flipPos, pvCache.particles->numParticles()*sizeof(float)*3);
				if (floatToPos != NULL)
				{
					pvCache.flipPos =  floatToPos;
				}

				for (int i=0;i<pvCache.particles->numParticles();i++)
				{
					const float * attrVal = pvCache.particles->data<float>(pvCache.positionAttr,i);
					pvCache.flipPos[(i*3)] 		= attrVal[0];
					pvCache.flipPos[((i*3)+1)] 	= -attrVal[2];
					pvCache.flipPos[((i*3)+2)] 	= attrVal[1];
				}
				mFlipped = true;
			}
			*/
			
			if( cacheChanged || rotationFromIndex != mLastRotationFromIndex ||
					scaleFromIndex != mLastScaleFromIndex ||
					indexFromIndex != mLastIndexFromIndex ||
					shaderIndexFromIndex != mLastShaderIndexFromIndex)
			{ // cache or rotation changed
				if (rotationFromIndex >=0) 
				{ // if there was rotation input
					// set up rotation array
					MVectorArray  rotationArray;
					if(pvCache.instanceData.checkArrayExist("rotation",vectorType))
					{
						rotationArray = pvCache.instanceData.getVectorData(MString("rotation"),&stat);
						CHECK_MSTATUS(stat);
					}else
					{
						rotationArray = pvCache.instanceData.vectorArray(MString("rotation"),&stat);
						CHECK_MSTATUS(stat);
					}
					rotationArray.clear();
					// populate rotation array
					pvCache.particles->attributeInfo(rotationFromIndex,pvCache.rotationAttr);
					// single float value for rotation
					if (pvCache.rotationAttr.count == 1) 
					{  
						for (int i=0;i<pvCache.particles->numParticles();i++)
						{
							const float * attrVal = pvCache.particles->data<float>(pvCache.rotationAttr,i);
							rotationArray.append(MVector(attrVal[0],attrVal[0],attrVal[0]));
						}
					} else 
					{ // we have a 4 float attribute
						if (pvCache.rotationAttr.count >= 3) 
						{ 
							for (int i=0;i<pvCache.particles->numParticles();i++)
							{
								const float * attrVal = pvCache.particles->data<float>(pvCache.rotationAttr,i);
								rotationArray.append(MVector(attrVal[0],attrVal[1],attrVal[2]));
							}
						}
					}// rotation array population
				}// there was rotation input
				if (scaleFromIndex >=0) 
				{ // if there was scale input
					// set up scale array
					MVectorArray  scaleArray;
					if(pvCache.instanceData.checkArrayExist("scale",vectorType)) 
					{
						scaleArray = pvCache.instanceData.getVectorData(MString("scale"),&stat);
						CHECK_MSTATUS(stat);
					} else 
					{
						scaleArray = pvCache.instanceData.vectorArray(MString("scale"),&stat);
						CHECK_MSTATUS(stat);
					}
					scaleArray.clear();
					// populate scale array
					pvCache.particles->attributeInfo(scaleFromIndex,pvCache.scaleAttr);
					if (pvCache.scaleAttr.count == 1) 
					{ // single float value for scale
						for (int i=0;i<pvCache.particles->numParticles();i++)
						{
							const float * attrVal = pvCache.particles->data<float>(pvCache.scaleAttr,i);
							scaleArray.append(MVector(attrVal[0],attrVal[0],attrVal[0]));
						}
					} else 
					{ // we have a 4 float attribute ?
						if (pvCache.scaleAttr.count >= 3) 
						{
							for (int i=0;i<pvCache.particles->numParticles();i++)
							{
								const float * attrVal = pvCache.particles->data<float>(pvCache.scaleAttr,i);
								scaleArray.append(MVector(attrVal[0],attrVal[1],attrVal[2]));
							}
						}
					}// scale array population
				} // there was scale input
				if (indexFromIndex >=0)
				{ // there was id input
					// set up id array
					MDoubleArray  indexArray;
					if(pvCache.instanceData.checkArrayExist("objectIndex",doubleType))
					{
						indexArray = pvCache.instanceData.getDoubleData(MString("objectIndex"),&stat);
						CHECK_MSTATUS(stat);
					} else 
					{
						indexArray = pvCache.instanceData.doubleArray(MString("objectIndex"),&stat);
						CHECK_MSTATUS(stat);
					}
					indexArray.clear();
					//populate id array
					pvCache.particles->attributeInfo(indexFromIndex,pvCache.indexAttr);
					if (pvCache.indexAttr.count == 1) 
					{ // single float value for index
						for (int i=0;i<pvCache.particles->numParticles();i++)
						{
							const float * attrVal = pvCache.particles->data<float>(pvCache.indexAttr,i);
							indexArray.append((double)(int)attrVal[0]);
						}
					} else 
					{ // we have a 4 float attribute ?
						if (pvCache.indexAttr.count >= 3) 
						{
							for (int i=0;i<pvCache.particles->numParticles();i++)
							{
								const float * attrVal = pvCache.particles->data<float>(pvCache.indexAttr,i);
								indexArray.append((double)(int)attrVal[0]);
							}
						}
					}// populate id array
				}// there was id input
				// keep track of attribute changes
				mLastRotationFromIndex = rotationFromIndex;
				mLastScaleFromIndex = scaleFromIndex;
				mLastIndexFromIndex = indexFromIndex;
				mLastShaderIndexFromIndex = shaderIndexFromIndex;
			}//cache or rotation cahnged
		}// there were particles in the cache
		block.outputValue(aInstanceData).set(pvCache.instanceDataObj);
		block.setClean(aInstanceData);
	}// we were requesting the output plug for this node
	if (pvCache.particles) 
	{ // update the AE Controls for attrs in the cache
		unsigned int numAttr=pvCache.particles->numAttributes();
		MPlug zPlug (thisMObject(), aPartioAttributes);
		// clear indicies if they changed
		if ((rotationFromIndex+1) > zPlug.numElements())
		{
			block.outputValue(aRotationFrom).setInt(-1);
		}
		if ((scaleFromIndex+1) > zPlug.numElements())
		{
			block.outputValue(aScaleFrom).setInt(-1);
		}
		if ((indexFromIndex+1) > zPlug.numElements())
		{
			block.outputValue(aIndexFrom).setInt(-1);
		}
		if ((shaderIndexFromIndex+1) > zPlug.numElements())
		{
			block.outputValue(aShaderIndexFrom).setInt(-1);
		}
		if (cacheChanged || zPlug.numElements() != numAttr)
		{ // if the cache cchanged
			attributeList.clear();
			for (unsigned int i=0;i<numAttr;i++)
			{
				ParticleAttribute attr;
				pvCache.particles->attributeInfo(i,attr);
				
				// crazy casting string to  char
				char *temp;
				temp = new char[(attr.name).length()+1];
				strcpy (temp, attr.name.c_str());
				
				MString  mStringAttrName("");
				mStringAttrName += MString(temp);
				
				zPlug.selectAncestorLogicalIndex(i,aPartioAttributes);
				zPlug.setValue(MString(temp));
				attributeList.append(mStringAttrName);
				
				delete [] temp;
			}
		} // the cache changed
	} // update AE Controls for attrs in cache
	
	block.setClean(plug);
	return MS::kSuccess;
}

bool partioInstancer::isBounded() const 
{ /// procs to override bounding box mode...
	return true;
}

MBoundingBox partioInstancer::boundingBox() const
{
	partioInstancer* nonConstThis = const_cast<partioInstancer*>(this);
	partioInstReaderCache* geom = nonConstThis->updateParticleCache();
	
	MPoint corner1 = geom->bbox.min();
	MPoint corner2 = geom->bbox.max();
	return MBoundingBox( corner1, corner2 );
}

bool partioInstancerUI::select( MSelectInfo &selectInfo, MSelectionList &selectionList, MPointArray &worldSpaceSelectPts ) const
{ /// This function just selects the object without doing any intersection tests.
	MSelectionMask priorityMask( MSelectionMask::kSelectObjectsMask );
	MSelectionList item;
	item.add( selectInfo.selectPath() );
	MPoint xformedPt;
	selectInfo.addSelection( item, xformedPt, selectionList, worldSpaceSelectPts, priorityMask, false );
	return true;
}

bool partioInstancer::GetPlugData()
{ ///  getPlugData is a util to update the drawing of the UI stuff
	// force update by requesting the update var
	MObject thisNode = thisMObject();
	int update = 0;
	MPlug updatePlug(thisNode, aUpdateCache );
	updatePlug.getValue( update );
	if(update != dUpdate)
	{ // make shure dUpdate is current
		dUpdate = update;
		return true;
	} else 
	{
		return false;
	}
	return false;
}

void partioInstancerUI::draw( const MDrawRequest& request, M3dView& view ) const
{ /// note the "const" at the end, its different than other draw calls
	MDrawData data = request.drawData();
	partioInstancer* shapeNode = (partioInstancer*) surfaceShape();
	partioInstReaderCache* cache = (partioInstReaderCache*) data.geometry();
	MObject thisNode = shapeNode->thisMObject();
	MPlug sizePlug( thisNode, shapeNode->aSize );
	MDistance sizeVal;
	
	sizePlug.getValue( sizeVal );
	shapeNode->multiplier= (float) sizeVal.asCentimeters();
	
	int drawStyle = DRAW_STYLE_POINTS;
	MPlug drawStylePlug( thisNode, shapeNode->aDrawStyle );
	drawStylePlug.getValue( drawStyle );
	
	view.beginGL();
		if (drawStyle < DRAW_STYLE_BOUNDING_BOX && view.displayStyle() != M3dView::kBoundingBox )
		{ // are we drawing not bounding box?
			drawPartio(cache,drawStyle);
		} else 
		{
			drawBoundingBox();
		}
		partio4Maya::drawPartioLogo(shapeNode->multiplier);
	view.endGL();
}

void  partioInstancerUI::drawBoundingBox() const 
{ /// DRAW Bounding box
	partioInstancer* shapeNode = (partioInstancer*) surfaceShape();
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

void partioInstancerUI::drawPartio(partioInstReaderCache* pvCache, int drawStyle) const 
{ /// DRAW PARTIO
	partioInstancer* shapeNode = (partioInstancer*) surfaceShape();
	MObject thisNode = shapeNode->thisMObject();
	//MPlug flipYZPlug( thisNode, shapeNode->aFlipYZ );
	//bool flipYZVal;
	//flipYZPlug.getValue( flipYZVal );
	MPlug pointSizePlug( thisNode, shapeNode->aPointSize );
	float pointSizeVal;
	pointSizePlug.getValue( pointSizeVal );
	int stride =  3*sizeof(float);
	// if we have particles
	if (pvCache->particles) {
		struct Point { float p[3]; };
		glPushAttrib(GL_CURRENT_BIT);
		glPointSize(pointSizeVal);
		glColor3f(1.0,1.0,1.0);
		glBegin(GL_POINTS);
			// looping through particles one by one
			for (int i=0;i<pvCache->particles->numParticles();i++){
				const float * partioPositions = pvCache->particles->data<float>(pvCache->positionAttr,i);
				glVertex3f(partioPositions[0], partioPositions[1], partioPositions[2]);
				//if draw style is label, label
				if (drawStyle == DRAW_STYLE_LABEL){
					// TODO: draw text label per particle here
				}
			}// looped through particles
		glEnd( );
		
		glDisable(GL_POINT_SMOOTH);
		glPopAttrib();
	} // we had particles
}

partioInstancerUI::partioInstancerUI(){}
partioInstancerUI::~partioInstancerUI(){}

void* partioInstancerUI::creator()
{
	return new partioInstancerUI();
}

void partioInstancerUI::getDrawRequests(const MDrawInfo & info, bool objectAndActiveOnly, MDrawRequestQueue & queue)
{
	MDrawData data;
	MDrawRequest request = info.getPrototype(*this);
	partioInstancer* shapeNode = (partioInstancer*) surfaceShape();
	partioInstReaderCache* geom = shapeNode->updateParticleCache();
	getDrawData(geom, data);
	request.setDrawData(data);
	if (!info.objectDisplayStatus(M3dView::kDisplayLocators)) 
	{ // Are we displaying locators?
		return;
	}
	M3dView::DisplayStatus displayStatus = info.displayStatus();
	M3dView::ColorTable activeColorTable = M3dView::kActiveColors;
	M3dView::ColorTable dormantColorTable = M3dView::kDormantColors;
	switch (displayStatus)
	{ // set display color
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
