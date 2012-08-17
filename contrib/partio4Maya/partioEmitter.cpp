/* partio4Maya  3/12/2012, John Cassella  http://luma-pictures.com and  http://redpawfx.com
PARTIO Export
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

#include "partioEmitter.h"

#define ID_PARTIOEMITTER  0x00116ED0 // id is registered with autodesk no need to change

#define McheckErr(stat, msg)\
	if ( MS::kSuccess != stat )\
	{\
		cerr << msg;\
		return MS::kFailure;\
	}

#define ATTR_TYPE_INT 0
#define ATTR_TYPE_DOUBLE 1
#define ATTR_TYPE_VECTOR 2

using namespace Partio;
using namespace std;

MTypeId partioEmitter::id ( ID_PARTIOEMITTER );

MObject partioEmitter::aCacheDir;
MObject partioEmitter::aCacheFile;
MObject partioEmitter::aCacheOffset;
MObject partioEmitter::aCacheActive;
MObject partioEmitter::aCacheFormat;
MObject partioEmitter::aCacheStatic;
MObject partioEmitter::aUseEmitterTransform;
MObject partioEmitter::aSize;
MObject partioEmitter::aFlipYZ;
MObject partioEmitter::aJitterPos;
MObject partioEmitter::aJitterFreq;
MObject partioEmitter::aPartioAttributes;
MObject partioEmitter::aMayaPPAttributes;

partioEmitter::partioEmitter():
		lastWorldPoint ( 0, 0, 0, 1 ),
		mLastFileLoaded(""),
		mLastPath(""),
		mLastFile(""),
		mLastExt(""),
		cacheChanged(false) {}

partioEmitter::~partioEmitter(){
	MSceneMessage::removeCallback( partioEmitterOpenCallback);
	MSceneMessage::removeCallback( partioEmitterImportCallback);
	MSceneMessage::removeCallback( partioEmitterReferenceCallback);
	MDGMessage::removeCallback( partioEmitterConnectionMade );
}

void *partioEmitter::creator(){
	return new partioEmitter;
}

void partioEmitter::postConstructor(){
	MStatus stat;
	partioEmitterOpenCallback = MSceneMessage::addCallback(MSceneMessage::kAfterOpen, partioEmitter::reInit, this);
	partioEmitterImportCallback = MSceneMessage::addCallback(MSceneMessage::kAfterImport, partioEmitter::reInit, this);
	partioEmitterReferenceCallback = MSceneMessage::addCallback(MSceneMessage::kAfterReference, partioEmitter::reInit, this);
	partioEmitterConnectionMade = MDGMessage::addConnectionCallback ( partioEmitter::connectionMadeCallbk, NULL, &stat );
}

/// init after opening
void partioEmitter::initCallback() {
	MObject tmo = thisMObject();
	short extEnum;
	MPlug(tmo, aCacheFormat).getValue(extEnum);
	mLastExt = partio4Maya::setExt(extEnum);
	MPlug(tmo, aCacheDir).getValue(mLastPath);
	MPlug(tmo, aCacheFile).getValue(mLastFile);
	cacheChanged = false;
}

void partioEmitter::reInit(void *data){
	partioEmitter  *emitterNode = (partioEmitter*) data;
	emitterNode->initCallback();
}

/// Creates the tracking ID attribute on the  particle object
void partioEmitter::connectionMadeCallbk(MPlug &srcPlug, MPlug &destPlug, bool made, void *clientData){
	MStatus status;
	MFnDependencyNode srcNode(srcPlug.node());
	MFnDependencyNode destNode(destPlug.node());
	// if the source is partioEmitter and dest is a particle then were good to go
	if (srcNode.typeId() == partioEmitter::id && (destNode.typeName() =="particle" || destNode.typeName() =="nParticle" ) ) {
		MObject  particleShapeNode =  destPlug.node(&status);
		MFnParticleSystem part(particleShapeNode, &status);
		createPPAttr( part, "partioID",  "pioID", 1);
	}
}

/// Initialize the node, create user defined attributes.
MStatus partioEmitter::initialize() {
	MStatus status;
	MFnTypedAttribute tAttr;
	MFnUnitAttribute uAttr;
	MFnNumericAttribute nAttr;
	MFnEnumAttribute  eAttr;
	
	// create attributes
	aCacheDir = tAttr.create ( "cacheDir", "cachD", MFnStringData::kString );
	tAttr.setReadable ( true );
	tAttr.setWritable ( true );
	tAttr.setKeyable ( false );
	tAttr.setConnectable ( true );
	tAttr.setStorable ( true );
	
	aCacheFile = tAttr.create ( "cachePrefix", "cachP", MFnStringData::kString );
	tAttr.setReadable ( true );
	tAttr.setWritable ( true );
	tAttr.setKeyable ( false );
	tAttr.setConnectable ( true );
	tAttr.setStorable ( true );
	
	aCacheOffset = nAttr.create("cacheOffset", "coff", MFnNumericData::kInt, 0, &status );
	nAttr.setKeyable(true);
	
	aCacheActive = nAttr.create("cacheActive", "cAct", MFnNumericData::kBoolean, 1, &status);
	nAttr.setKeyable(true);
	
	aCacheStatic = nAttr.create("staticCache", "statC", MFnNumericData::kBoolean, 0, &status);
	nAttr.setKeyable(true);
	
	aCacheFormat = eAttr.create( "cacheFormat", "cachFmt");
	std::map<short,MString> formatExtMap;
	partio4Maya::buildSupportedExtensionList(formatExtMap,false);
	for (unsigned short i = 0; i< formatExtMap.size(); i++) {
		eAttr.addField(formatExtMap[i].toUpperCase(),	i);
	}
	
	eAttr.setDefault(4); // PDC
	eAttr.setChannelBox(true);
	eAttr.setKeyable(false);
	
	aUseEmitterTransform = nAttr.create("useEmitterTransform", "uet", MFnNumericData::kBoolean, false, &status);
	nAttr.setKeyable(true);
	
	aSize = uAttr.create( "iconSize", "isz", MFnUnitAttribute::kDistance );
	uAttr.setDefault( 0.25 );
	
	aFlipYZ = nAttr.create( "flipYZ", "fyz", MFnNumericData::kBoolean);
	nAttr.setDefault ( false );
	nAttr.setKeyable ( true );
	
	aJitterPos = nAttr.create("jitterPos", "jpos", MFnNumericData::kFloat,0.0, &status );
	nAttr.setDefault(0);
	nAttr.setMin(0);
	nAttr.setSoftMax(999);
	nAttr.setKeyable(true);
	
	aJitterFreq = nAttr.create("jitterFreq", "jfreq", MFnNumericData::kFloat, 1.0, &status);
	nAttr.setDefault(1.0);
	nAttr.setKeyable(true);
	
	aPartioAttributes = tAttr.create ("partioCacheAttributes", "pioCAts", MFnStringData::kString);
	tAttr.setArray(true);
	tAttr.setDisconnectBehavior(MFnAttribute::kDelete);
	
	aMayaPPAttributes = tAttr.create("mayaPPAttributes", "pioMPPAts" , MFnStringData::kString);
	tAttr.setArray(true);
	tAttr.setDisconnectBehavior(MFnAttribute::kDelete);
	
	// attach attributes
	status = addAttribute ( aCacheDir );
	status = addAttribute ( aCacheFile );
	status = addAttribute ( aCacheOffset );
	status = addAttribute ( aCacheActive );
	status = addAttribute ( aCacheStatic );
	status = addAttribute ( aCacheFormat );
	status = addAttribute ( aUseEmitterTransform );
	status = addAttribute ( aSize );
	status = addAttribute ( aFlipYZ );
	status = addAttribute ( aJitterPos );
	status = addAttribute ( aJitterFreq );
	status = addAttribute ( aPartioAttributes );
	status = addAttribute ( aMayaPPAttributes );
	
	// set up attribute deps
	status = attributeAffects ( aCacheDir, mOutput );
	status = attributeAffects ( aCacheFile, mOutput );
	status = attributeAffects ( aCacheOffset, mOutput );
	status = attributeAffects ( aCacheStatic, mOutput );
	status = attributeAffects ( aCacheFormat, mOutput );
	status = attributeAffects ( aUseEmitterTransform, mOutput );
	status = attributeAffects ( aJitterPos, mOutput );
	status = attributeAffects ( aJitterFreq, mOutput );
	
	return ( status );
}

MStatus partioEmitter::compute ( const MPlug& plug, MDataBlock& block ){
	MStatus status, stat;
	bool cacheActive = block.inputValue(aCacheActive).asBool();
	// if the cache is not active, skip
	if (!cacheActive){
		return ( MS::kSuccess );
	}
	int cacheOffset 	= block.inputValue( aCacheOffset ).asInt();
	short cacheFormat = block.inputValue( aCacheFormat ).asShort();
	float jitterPos = block.inputValue( aJitterPos ).asFloat();
	float jitterFreq = block.inputValue( aJitterFreq ).asFloat();
	bool useEmitterTxfm = block.inputValue( aUseEmitterTransform ).asBool();
	bool cacheStatic = block.inputValue( aCacheStatic ).asBool();
	MString cacheDir = block.inputValue(aCacheDir).asString();
	MString cacheFile = block.inputValue(aCacheFile).asString();
	// Determine if we are requesting the output plug for this emitter node.
	if ( !( plug == mOutput ) ){
		return ( MS::kUnknownParameter );
	}
	// get the relevant plugs to get to array and particle system
	MPlugArray  connectionArray;
	plug.connectedTo(connectionArray, false, true, &stat);
	MPlug particleShapeOutPlug = connectionArray[0];
	MObject  particleShapeNode = particleShapeOutPlug.node(&stat);
	MFnParticleSystem part(particleShapeNode, &stat);
	MString partName = part.particleName();
	MString emitterPlugName = plug.name();
	MString particleShapeOutPlugName = particleShapeOutPlug.name();
	// if we dont have the partioID, fail
	if (!part.isPerParticleDoubleAttribute("partioID")){
		MGlobal::displayWarning("PartioEmitter->error:  was unable to create/find partioID attr");
		return ( MS::kFailure );
	}
	// if we don' have a valid cache dir, fail
	if (cacheDir  == "" || cacheFile == "" ) {
		//TODO: provide visual feedback, printing was too intensive
		return ( MS::kFailure );
	}
	// Get the logical index of the element this plug refers to,
	// because the node can be emitting particles into more
	// than one particle shape.
	int multiIndex = plug.logicalIndex ( &status );
	McheckErr ( status, "ERROR in plug.logicalIndex.\n" );
	// Get output data arrays (position, velocity, or parentId)
	// that the particle shape is holding from the previous frame.
	MArrayDataHandle hOutArray = block.outputArrayValue ( mOutput, &status );
	McheckErr ( status, "ERROR in hOutArray = block.outputArrayValue.\n" );
	// Create a builder to aid in the array construction efficiently.
	MArrayDataBuilder bOutArray = hOutArray.builder ( &status );
	McheckErr ( status, "ERROR in bOutArray = hOutArray.builder.\n" );
	// Get the appropriate data array that is being currently evaluated.
	MDataHandle hOut = bOutArray.addElement ( multiIndex, &status );
	McheckErr ( status, "ERROR in hOut = bOutArray.addElement.\n" );
	// Create the data and apply the function set,
	// particle array initialized to length zero,
	// fnOutput.clear()
	MFnArrayAttrsData fnOutput;
	MObject dOutput = fnOutput.create ( &status );
	McheckErr ( status, "ERROR in fnOutput.create.\n" );
	// Check if the particle object has reached it's maximum,
	// hence is full. If it is full then just return with zero particles.
	bool beenFull = isFullValue ( multiIndex, block );
	if ( beenFull ){
		return ( MS::kSuccess );
	}
	// Get deltaTime, currentTime and startTime.
	// If deltaTime <= 0.0, or currentTime <= startTime,
	// do not emit new pariticles and return.
	MTime cT = currentTimeValue ( block );
	MTime sT = startTimeValue ( multiIndex, block );
	MTime dT = deltaTimeValue ( multiIndex, block );
	if ( ( cT <= sT )) {
		// We do not emit particles before the start time,
		// we do support emitting / killing of particles if we scroll backward in time.
		// This code is necessary primarily the first time to
		// establish the new data arrays allocated, and since we have
		// already set the data array to length zero it does
		// not generate any new particles.
		hOut.set ( dOutput );
		block.setClean ( plug );
		return ( MS::kSuccess );
	}
	bool motionBlurStep = false;
	int integerTime = (int)floor(cT.value()+.52);
	// parse and get the new file name
	MString formatExt = "";
	int cachePadding = 0;
	MString newCacheFile = "";
	MString renderCacheFile = "";
	partio4Maya::updateFileName( cacheFile,  cacheDir,
							cacheStatic,  cacheOffset,
							cacheFormat,  integerTime,
							cachePadding, formatExt,
							newCacheFile, renderCacheFile);
	float deltaTime  = float(cT.value() - integerTime);
	// motion  blur rounding  frame logic
	if ((deltaTime < 1 || deltaTime > -1)&& deltaTime !=0) {  // motion blur step?
		motionBlurStep = true;
	}
	long seed = seedValue( multiIndex, block );
	// get the emitter offset
	MPoint emitterOffset;
	getWorldPosition(emitterOffset);
	double inheritFactor = inheritFactorValue(multiIndex, block);
	// using std:map to give us a nice fast binary search
	map<int, int>  particleIDMap;
	cacheChanged = false;
	// check if cache has changed formate or fiel or dir
	if (mLastExt != formatExt || mLastPath != cacheDir || mLastFile != cacheFile){
		cacheChanged = true;
		mLastExt = formatExt;
		mLastPath = cacheDir;
		mLastFile = cacheFile;
	}
	// if the new cache is valid, load it!
	if ( newCacheFile != "" && partio4Maya::partioCacheExists(newCacheFile.asChar())) {
		MGlobal::displayInfo(MString("partioEmitter->Loading: " + newCacheFile));
		ParticlesDataMutable* particles=0;
		ParticleAttribute IdAttribute;
		ParticleAttribute posAttribute;
		ParticleAttribute velAttribute;
		particles=read(newCacheFile.asChar());
		// if the cache contained particles
		if (particles){
			//mLastFileLoaded = cacheFile; NOTE: why is this commented out, check to see if behavior was replaced or if this is a mistake.
			// take particle id's from cache and put them into id map
			for (int i=0;i<particles->numParticles();i++){
				int id = -1;
				// if the attribut is id use that
				if (particles->attributeInfo("id",IdAttribute) || particles->attributeInfo("Id",IdAttribute)){
					const int* partioID = particles->data<int>(IdAttribute,i);
					id = partioID[0];
				} else if (particles->attributeInfo("particleId",IdAttribute) || particles->attributeInfo("ParticleId",IdAttribute)){ // if its particle id use that
					const int* partioID = particles->data<int>(IdAttribute,i);
					id = partioID[0];
				} else { // else try to use the array index, this is a bad idea
					MGlobal::displayWarning("Loaded Partio cache has a non-standard or non-existant id attribute, this may render things unstable");
					id = i;
				}
				particleIDMap[id] = i;
			}
			// if we don't have position, fail
			if (!particles->attributeInfo("position",posAttribute) && !particles->attributeInfo("Position",posAttribute)){
				std::cerr<<"Failed to find position attribute "<<std::endl;
				return ( MS::kFailure );
			}
			// if we don't have velocity, fail
			if (!particles->attributeInfo("velocity",velAttribute) && !particles->attributeInfo("Velocity",velAttribute)){
				std::cerr<<"Failed to find velocity attribute "<<std::endl;
				return ( MS::kFailure );
			}
			// set up map iteratior and get plug data for partio and ppattribs
			map <int, int>::iterator it;
			it = particleIDMap.begin();
			it = particleIDMap.end();
			unsigned int numAttr=particles->numAttributes();
			MPlug zPlug (thisMObject(), aPartioAttributes);
			MPlug yPlug (thisMObject(), aMayaPPAttributes);
			//if the cache has changed or num elements has changed update the AE Controls for attrs in the cache
			if (cacheChanged || zPlug.numElements() != numAttr) {
				MGlobal::displayInfo("partioEmitter->refreshing AE controls");
				// set up plugs to take new attributes
				for (unsigned int i=0;i<numAttr;i++) {
					ParticleAttribute attr;
					particles->attributeInfo(i,attr);
					// note, casting string to a char* here
					char* temp;
					temp = new char[(attr.name).length()+1];
					strcpy (temp, attr.name.c_str());
					MString  mStringAttrName("");
					mStringAttrName += MString(temp);
					zPlug.selectAncestorLogicalIndex(i,aPartioAttributes);
					zPlug.setValue(MString(temp));
					yPlug.selectAncestorLogicalIndex(i,aMayaPPAttributes);
					yPlug.setValue(MString(""));
					delete [] temp;
					MGlobal::executeCommand("refreshAE;");
				}
			} // end cache changed
			std::map<std::string,  MVectorArray  > vectorAttrArrays;
			std::map<std::string,  MDoubleArray  > doubleAttrArrays;
			// we use this mapping to allow for direct writing of attrs to PP variables
			std::map<std::string, std::string > userPPMapping;
			// loop through attributes and add them to plugs
			for (unsigned int i=0;i<numAttr;i++) {
				// get attribut plugs, set up map to write to them
				ParticleAttribute attr;
				particles->attributeInfo(i,attr);
				yPlug.selectAncestorLogicalIndex(i,aMayaPPAttributes);
				userPPMapping[yPlug.asString().asChar()] = attr.name;
				yPlug.selectAncestorLogicalIndex(i,aMayaPPAttributes);
				// make sure plug is not invalid
				if (yPlug.asString() != "") {
					// get the attribute name
					MString ppAttrName = yPlug.asString();
					// is it an attribute of length 3
					if (attr.count == 3) {
						// is it a new attribute?, then add it.. via mel NOTE: this is a bad idea
						if (!part.isPerParticleVectorAttribute(ppAttrName)) {
							MGlobal::displayInfo(MString("partioEmitter->adding ppAttr " + ppAttrName) );
							MString command;
							command += "pioEmAddPPAttr ";
							command += ppAttrName;
							command += " vectorArray ";
							command += partName;
							command += ";";
							MGlobal::executeCommandOnIdle(command);
						}
						// is it an old attribute, then update it
						if (part.isPerParticleVectorAttribute(ppAttrName)) {
							MVectorArray vAttribute;
							part.getPerParticleAttribute(ppAttrName, vAttribute, &status);
							// if you couldn't get the attribut error out
							if ( !status ) {
								MGlobal::displayError("PartioEmitter->could not get vector PP array ");
							}
							// add attrib to map NOTE: if we errored out we should probably not do this
							vectorAttrArrays[ppAttrName.asChar()] = vAttribute;
						}
					} else if (attr.count == 1) { // its not a vec 3, is it a float  or double?
						// if it's a new double add it as a mel process. NOTE: this is a bad idea
						if (!part.isPerParticleDoubleAttribute(ppAttrName)) {
							MGlobal::displayInfo(MString("PartioEmiter->adding ppAttr " + ppAttrName));
							MString command;
							command += "pioEmAddPPAttr ";
							command += ppAttrName;
							command += " doubleArray ";
							command += partName;
							command += ";";
							MGlobal::executeCommandOnIdle(command);
						} if (part.isPerParticleDoubleAttribute(ppAttrName)) { // if its an existing attribute set up the mapping
							MDoubleArray dAttribute;
							part.getPerParticleAttribute(ppAttrName, dAttribute, &status);
							// if we couldn't get the attribute error
							if ( !status ) {
								MGlobal::displayError("PartioEmitter->could not get double PP array ");
							}
							// set up the mapping NOTE: if we errored out we should probably not do this
							doubleAttrArrays[ppAttrName.asChar()] = dAttribute;
						}
					} else { // this wasn't a one or three lenght attribute, note taht we are skipping it
						MGlobal::displayError(MString("PartioEmitter->skipping attr: " + MString(attr.name.c_str())));
					} // we handled the acceptable types of attributes
				} // attribute plug was valid
			} // end loop to add user PP attributes
			// load base values, id, position, lifespan, partioIds, set up map iterator for vec and doubles
			MPointArray inPosArray;
			MVectorArray inVelArray;
			MIntArray ids;
			part.particleIds(ids);
			MVectorArray positions;
			part.position(positions);
			MDoubleArray lifespans;
			part.lifespan(lifespans);
			MVectorArray velocities;
			part.velocity(velocities);
			MDoubleArray partioIDs;
			part.getPerParticleAttribute("partioID", partioIDs);
			MIntArray  deletePoints;
			std::map <std::string, MVectorArray >::iterator vecIt;
			std::map <std::string, MDoubleArray >::iterator doubleIt;
			// loop through particles and move ones that are still valid
			for (unsigned int x = 0; x<part.count(); x++) {
				it = particleIDMap.find((int)partioIDs[x]);
				// if this particle still exists update its attrubutes
				if (it != particleIDMap.end()) { 
					// setup position, velocity, jitter
					const float* pos=particles->data<float>(posAttribute,it->second);
					const float* vel=particles->data<float>(velAttribute,it->second);
					MVector jitter = partio4Maya::jitterPoint(it->second, jitterFreq, float(seed), jitterPos);
					// set position + jitter
					positions[x] = MVector(pos[0],pos[1],pos[2])+(jitter);
					// if we are emitting from the emitters location apply the offset
					if (useEmitterTxfm) {
						positions[x] += emitterOffset;
					}
					MVector velo(vel[0],vel[1],vel[2]);
					// if we are using motion blur do it in this step NOTE: we are assuming 24 frames per second here
					if (motionBlurStep) {
						positions[x] += (velo/24)*deltaTime;
					}
					// set velocity
					velocities[x] = velo;
					// loop through double attributes and set / update them
					for (doubleIt = doubleAttrArrays.begin(); doubleIt != doubleAttrArrays.end(); doubleIt++) {
						ParticleAttribute doubleAttr;
						particles->attributeInfo(userPPMapping[doubleIt->first].c_str(),doubleAttr);
						const float*  doubleVal = particles->data<float>(doubleAttr, it->second);
						doubleAttrArrays[doubleIt->first][x] = doubleVal[0];
					}
					// loop through vector attributes and set / update them
					for (vecIt = vectorAttrArrays.begin(); vecIt != vectorAttrArrays.end(); vecIt++) {
						ParticleAttribute vectorAttr;
						particles->attributeInfo(userPPMapping[vecIt->first].c_str(), vectorAttr);
						const float* vecVal = particles->data<float>(vectorAttr, it->second);
						vectorAttrArrays[vecIt->first][x] = MVector(vecVal[0],vecVal[1],vecVal[2]);
					}
					// cleanse the iterator
					particleIDMap.erase(it);
				} else { // particle is not valid, set up for deletion
					deletePoints.append(x);
				}
			} // we have looped through the particles and moved the valid ones
			// TODO: handle a "release" attribute list to allow expressions to force partio emitter to forget or skip over certain particles
			// loop over scheduled particles to remove and remove them
			for (unsigned int y = 0; y< deletePoints.length(); y++) {
				//remove the related fixed attributes
				positions.remove(deletePoints[y]-y);
				velocities.remove(deletePoints[y]-y);
				lifespans.remove(deletePoints[y]-y);
				// remove any double attributes
				for (doubleIt = doubleAttrArrays.begin(); doubleIt != doubleAttrArrays.end(); doubleIt++) {
					doubleAttrArrays[doubleIt->first].remove(deletePoints[y]-y);
				}
				// remove any vector attributes
				for (vecIt = vectorAttrArrays.begin(); vecIt != vectorAttrArrays.end(); vecIt++) {
					vectorAttrArrays[vecIt->first].remove(deletePoints[y]-y);
				}
				// update particle count
				part.setCount (particles->numParticles());
			}
			//  now loop over and deal with new particles, which are all that is left in the map
			for (it = particleIDMap.begin(); it != particleIDMap.end(); it++) {
				// set up position, velocity, and id array pointers
				const float* pos=particles->data<float>(posAttribute,it->second);
				const float* vel=particles->data<float>(velAttribute,it->second);
				// TODO: this will break if we're dealing with a format type that has no "id" attribute or one is not found using the standard  nomenclature "id" or  "particleId"
				const int* id=particles->data<int>(IdAttribute,it->second);
				MVector temp(pos[0], pos[1], pos[2]);
				MVector jitter = partio4Maya::jitterPoint(it->second, jitterFreq, float(seed), jitterPos);
				// if we are using the emmitters position apply it as an offset
				if (useEmitterTxfm) {
					temp += emitterOffset;
				}
				// append the standard elements
				MVector velo(vel[0],vel[1],vel[2]);
				inPosArray.append(temp+(jitter));
				inVelArray.append(velo*inheritFactor);
				partioIDs.append(id[0]);
				//loop through double value attributes and add those
				for (doubleIt = doubleAttrArrays.begin(); doubleIt != doubleAttrArrays.end(); doubleIt++) {
					ParticleAttribute doubleAttr;
					particles->attributeInfo(userPPMapping[doubleIt->first].c_str(),doubleAttr);
					const float*  doubleVal = particles->data<float>(doubleAttr, it->second);
					doubleAttrArrays[doubleIt->first].append(doubleVal[0]);
				}
				// loop through vector attributes and add those
				for (vecIt = vectorAttrArrays.begin(); vecIt != vectorAttrArrays.end(); vecIt++) {
					ParticleAttribute vectorAttr;
					particles->attributeInfo(userPPMapping[vecIt->first].c_str(), vectorAttr);
					const float* vecVal = particles->data<float>(vectorAttr, it->second);
					vectorAttrArrays[vecIt->first].append(MVector(vecVal[0],vecVal[1],vecVal[2]));
				}
			}// we have added any new particles to our arrays
			// set the position, velocity, and lifespan arrays to the particle shape
			part.setPerParticleAttribute("position", positions);
			part.setPerParticleAttribute("velocity", velocities);
			part.setPerParticleAttribute("lifespanPP", lifespans);
			MGlobal::displayInfo (MString ("PartioEmitter->Emitting  ") + inPosArray.length() + MString( " new particles"));
			// emit the new particles
			part.emit(inPosArray, inVelArray);
			// set the id's of said particles
			part.setPerParticleAttribute("partioID", partioIDs);
			// loop through particles and set double values to new value
			for (doubleIt = doubleAttrArrays.begin(); doubleIt != doubleAttrArrays.end(); doubleIt++) {
				part.setPerParticleAttribute(MString(doubleIt->first.c_str()), doubleAttrArrays[doubleIt->first]);
			}
			// loop through particles and set vector values to new value
			for (vecIt = vectorAttrArrays.begin(); vecIt != vectorAttrArrays.end(); vecIt++) {
				part.setPerParticleAttribute(MString(vecIt->first.c_str()), vectorAttrArrays[vecIt->first]);
			}
			// free up memory
			particles->release();
			//MArrayDataHandle outputArray = block.outputArrayValue(aPartioAttributes,&stat);
			//stat = outputArray.set(builder);
			//MArrayDataHandle outputArrayChx = block.outputArrayValue(aPartioAttrCheckbox, &stat);
			//stat = outputArrayChx.set(builderChx);
		} // we dealt with particle in the array
	} else { // the new cache is invalid, display error
		MGlobal::displayError("PartioEmitter->Error loading the Cache file, it does not exist on disk, check path/file.");
	} // if we got here everythign went smoothly and all particles are taken care of
	// Update the data block with new dOutput and set plug clean.
	hOut.set ( dOutput );
	block.setClean ( plug );
	
	return MS::kSuccess;
}

/// Gets emitter position in world space
MStatus partioEmitter::getWorldPosition ( MPoint &point ) {
	MStatus status;
	MObject thisNode = thisMObject();
	MFnDependencyNode fnThisNode ( thisNode );
	// get worldMatrix attribute.
	MObject worldMatrixAttr = fnThisNode.attribute ( "worldMatrix" );
	// build worldMatrix plug, and specify which element the plug refers to.
	// We use the first element(the first dagPath of this emitter).
	MPlug matrixPlug ( thisNode, worldMatrixAttr );
	matrixPlug = matrixPlug.elementByLogicalIndex ( 0 );
	// Get the value of the 'worldMatrix' attribute
	MObject matrixObject;
	status = matrixPlug.getValue ( matrixObject );
	// if we can't get matrix node error out
	if( !status ){
		status.perror ( "partioEmitter::getWorldPosition: get matrixObject" );
		return status;
	}
	// if we cant get world matrix data error out
	MFnMatrixData worldMatrixData ( matrixObject, &status );
	if( !status ) {
		status.perror ( "partioEmitter::getWorldPosition: get worldMatrixData" );
		return status;
	}
	// if we cant get world matrix error out
	MMatrix worldMatrix = worldMatrixData.matrix ( &status );
	if( !status ) {
		status.perror ( "partioEmitter::getWorldPosition: get worldMatrix" );
		return status;
	}
	// assign the world matrix translate to the given vector.
	point[0] = worldMatrix ( 3, 0 );
	point[1] = worldMatrix ( 3, 1 );
	point[2] = worldMatrix ( 3, 2 );
	return ( status );
}

/// DRAW  the  Partio Logo  helper
void partioEmitter::draw ( M3dView& view, const MDagPath& path, M3dView::DisplayStyle style, M3dView:: DisplayStatus ) {
	view.beginGL();
		// get node and size
		MObject thisNode = thisMObject();
		MPlug sizePlug( thisNode, aSize );
		MDistance sizeVal;
		sizePlug.getValue( sizeVal );
		float multiplier = (float) sizeVal.asCentimeters();
		// draw logo by multiplier
		partio4Maya::drawPartioLogo(multiplier);
	view.endGL();
}

/// creates a particle attribute in the system
MStatus partioEmitter::createPPAttr( MFnParticleSystem  &part, MString attrName, MString shortName, int type) {
	// create start state
	MFnTypedAttribute initialStateAttr;
	MFnTypedAttribute ppAttr;
	MStatus stat1,stat2 = MS::kFailure;
	MObject initialStateAttrObj;
	MObject attrObj;
	// check which type of attr it is and if it is not already part of the system get ready to add it
	switch (type) {
		case ATTR_TYPE_INT:
			if (!part.isPerParticleIntAttribute((attrName+"0")) && !part.isPerParticleIntAttribute(attrName)){
				initialStateAttrObj = initialStateAttr.create((attrName+"0"), (shortName+"0"), MFnData::kIntArray, &stat1);
				attrObj = ppAttr.create((attrName), (shortName), MFnData::kIntArray, &stat2);
			}
			break;
		case ATTR_TYPE_DOUBLE:
			if (!part.isPerParticleDoubleAttribute((attrName+"0")) && !part.isPerParticleDoubleAttribute(attrName)){
				initialStateAttrObj = initialStateAttr.create((attrName+"0"), (shortName+"0"), MFnData::kDoubleArray, &stat1);
				attrObj = ppAttr.create((attrName), (shortName), MFnData::kDoubleArray, &stat2);
			}
			break;
		case ATTR_TYPE_VECTOR:
			if (!part.isPerParticleVectorAttribute((attrName+"0")) && !part.isPerParticleVectorAttribute(attrName)){
				initialStateAttrObj = initialStateAttr.create((attrName+"0"), (shortName+"0"), MFnData::kVectorArray, &stat1);
				attrObj = ppAttr.create((attrName), (shortName), MFnData::kVectorArray, &stat2);
			}
			break;
		default:
			break;
	}
	// if creating the initial and attr succeeded add it to the particle system
	if (stat1 == MStatus::kSuccess && stat2 == MStatus::kSuccess) {
		initialStateAttr.setStorable (true);
		ppAttr.setStorable (true);
		ppAttr.setKeyable (true);
		stat1 = part.addAttribute (initialStateAttrObj, MFnDependencyNode::kLocalDynamicAttr);
		// if either state throw warning
		if (!stat1) {
			MGlobal::displayWarning("PartioEmitter->error:  was unable to create "+attrName+"0"+ " attr");
		}
		stat2 = part.addAttribute (attrObj, MFnDependencyNode::kLocalDynamicAttr);
		if (!stat2) {
			MGlobal::displayWarning("PartioEmitter->error:  was unable to create "+ (attrName)+ " attr");
		}
	}
	// if unsuccessfull error out
	if (stat1 != MStatus::kSuccess || stat2 != MStatus::kSuccess) {
		return MStatus::kFailure;
	}
	return MStatus::kSuccess;
}
