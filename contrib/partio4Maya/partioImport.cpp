/* partio4Maya  3/12/2012, John Cassella  http://luma-pictures.com and  http://redpawfx.com
PARTIO Import
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

#include "partioImport.h"

static const char *kAttributeFlagS	= "-atr";
static const char *kAttributeFlagL      = "-attribute";
static const char *kFlipFlagS		= "-flp";
static const char *kFlipFlagL		= "-flip";
static const char *kHelpFlagS 		= "-h";
static const char *kHelpFlagL 		= "-help";
static const char *kParticleL		= "-particle";
static const char *kParticleS		= "-p";

using namespace std;
using namespace Partio;

bool PartioImport::hasSyntax(){
	return true;
}

MSyntax PartioImport::createSyntax(){
	MSyntax syntax;
	syntax.addFlag(kParticleS, kParticleL ,  MSyntax::kString);
	syntax.addFlag(kHelpFlagS, kHelpFlagL ,  MSyntax::kNoArg);
	syntax.addFlag(kAttributeFlagS, kAttributeFlagL, MSyntax::kString, MSyntax::kString);
	syntax.makeFlagMultiUse( kAttributeFlagS );
	syntax.addFlag(kFlipFlagS, kFlipFlagL, MSyntax::kNoArg);
	syntax.addArg(MSyntax::kString);
	syntax.enableQuery(false);
	syntax.enableEdit(false);
	return syntax;
}

void* PartioImport::creator(){
	return new PartioImport;
}

MStatus PartioImport::doIt(const MArgList& Args){
	bool makeParticle = false;
	MStatus status;
	MArgDatabase argData(syntax(), Args, &status);
	//failed to parse arguments
	if (status == MStatus::kFailure) {
		MGlobal::displayError("Error parsing arguments" );
	}
	// help was requested
	if( argData.isFlagSet(kHelpFlagL) ) {
		printUsage();
		return MStatus::kFailure;
	}
	// should we make a shape or load a specified shape
	MString particleShape;
	if (argData.isFlagSet(kParticleL) ) {
		argData.getFlagArgument(kParticleL, 0, particleShape );
		if (particleShape == "") {
			printUsage();
			MGlobal::displayError("Please supply particleShape argument" );
			return MStatus::kFailure;
		}
	} else { // they did not supply a shape
		makeParticle = true;
	}// shape initialization
	//do something if they needed to flip
	if( argData.isFlagSet(kFlipFlagL) || argData.isFlagSet(kFlipFlagS)){}
	// get rest of attributes
	unsigned int numUses = argData.numberOfFlagUses( kAttributeFlagL );
	MStringArray  attrNames;
	MStringArray  mayaAttrNames;
	bool worldVeloCheck = false;
	// loop through rest of attributes
	for( unsigned int i = 0; i < numUses; i++ ){
		MArgList argList;
		status = argData.getFlagArgumentList( kAttributeFlagL, i, argList );
		if( !status ) return status;
		MString AttrName = argList.asString( 0, &status );
		if( !status ) return status;
		// we have position or id in do something
		if( AttrName == "position" || AttrName == "worldPosition"  || AttrName == "id" || AttrName == "particleId") { 
		} else if( AttrName == "worldVelocity" || AttrName == "velocity" ) { // they passed in velocity
			if (!worldVeloCheck) {
				attrNames.append("velocity");
				worldVeloCheck = true;
			}
		} else { // this is not position, id, or velocity, add it!
			attrNames.append(AttrName);
			if (argList.length() > 1) {
				mayaAttrNames.append(argList.asString( 1 ));
			} else {
				mayaAttrNames.append(AttrName);
			}
		}// done wtih attribute attachment
	}// looped through the rest of attributes
	
	MString particleCache; // particleCache file
	argData.getCommandArgument(0, particleCache);
	// check if the cache file exists
	if(!partio4Maya::partioCacheExists(particleCache.asChar())){
		MGlobal::displayError("Particle Cache Does not exist");
		return MStatus::kFailure;
	}
	// if we are making a particle shape from scratch
	if (makeParticle){
		MStringArray foo;
		MGlobal::executeCommand("particle -n partioImport -p 0 0 0", foo);
		particleShape = foo[1];
	}
	// get shape node
	MSelectionList list;
	list.add(particleShape);
	MObject objNode;
	list.getDependNode(0, objNode);
	// if this isn't a  particle node error
	if( objNode.apiType() != MFn::kParticle && objNode.apiType() != MFn::kNParticle ){
		MGlobal::displayError("PartioImport-> can't find your PARTICLESHAPE.");
		return MStatus::kFailure;
	}
	// get particle system
	MStatus stat;
	MFnParticleSystem partSys(objNode, &stat);
	MString partName = partSys.particleName();
	// if we didn't make this node delete all particles in system
	if (!makeParticle) {
		MVectorArray clearOut;
		clearOut.clear();
		partSys.setCount(0);
		partSys.setPerParticleAttribute("position", clearOut);
		partSys.setPerParticleAttribute("velocity", clearOut);
	}
	// particle object was found and attached to
	if (stat == MStatus::kSuccess) {
		Partio::ParticlesDataMutable* particles;
		Partio::ParticleAttribute positionAttr;
		Partio::ParticleAttribute velocityAttr;
		// read cache
		MGlobal::displayInfo(MString ("PartioImport-> LOADING: ") + particleCache);
		particles=read(particleCache.asChar());
		bool hasVelo = true;
		char partCount[50];
		sprintf (partCount, "%d", particles->numParticles());
		MGlobal::displayInfo(MString ("PartioImport-> LOADED: ") + partCount + MString (" particles"));
		// if there were no particles in cache, fail
		if (!particles || particles->numParticles() <=0){
			MGlobal::displayError("Particle Cache cannot be read, or does not Contain any particles");
			return MStatus::kFailure;
		}
		// if there was no position attribute in cache, fail
		if (!particles->attributeInfo("position",positionAttr) && !particles->attributeInfo("Position",positionAttr)){
			MGlobal::displayError("PartioImport->Failed to find position attribute ");
			return ( MS::kFailure );
		}
		// check for velocity attribute
		if (!particles->attributeInfo("velocity",velocityAttr) &&
				!particles->attributeInfo("Velocity",velocityAttr) &&
				!particles->attributeInfo("vel",velocityAttr) &&
				!particles->attributeInfo("Vel",velocityAttr)){
			MGlobal::displayWarning("PartioImport->Failed to find Velocity attribute ");
			hasVelo = false;
		}
		// set up arrays and mappings for attributes
		MPointArray positions;
		MVectorArray velocities;
		std::map<std::string,  MVectorArray  > vectorAttrArrays;
		std::map<std::string,  MDoubleArray  > doubleAttrArrays;
		// we use this mapping to allow for direct writing of attrs to PP variables
		std::map<std::string, std::string > userPPMapping;
		// loop through each particle attribute
		for (unsigned int i=0;i<attrNames.length();i++){
			Partio::ParticleAttribute testAttr;
			// if the particles have this attribute
			if (particles->attributeInfo(attrNames[i].asChar(), testAttr)) {
				// if this is a vector attr
				if (testAttr.count == 3){
					// if this attribute is not already in the system add it
					if (!partSys.isPerParticleVectorAttribute(mayaAttrNames[i])) {
						MGlobal::displayInfo(MString("partioImport->adding ppAttr " + mayaAttrNames[i]) );
						MString command;
						command += "pioEmAddPPAttr ";
						command += mayaAttrNames[i];
						command += " vectorArray ";
						command += partName;
						command += ";";
						MGlobal::executeCommand(command);
					}
					MVectorArray vAttribute;
					vAttribute.setLength(particles->numParticles());
					vectorAttrArrays[attrNames[i].asChar()] = vAttribute;
					userPPMapping[attrNames[i].asChar()] = mayaAttrNames[i].asChar();
				} else if (testAttr.count == 1) { // this is a double attribute
					// if this attribute is not already in the system add it
					if (!partSys.isPerParticleDoubleAttribute(attrNames[i])) {
						MGlobal::displayInfo(MString("PartioEmiter->adding ppAttr " + mayaAttrNames[i]));
						MString command;
						command += "pioEmAddPPAttr ";
						command += mayaAttrNames[i];
						command += " doubleArray ";
						command += partName;
						command += ";";
						MGlobal::executeCommand(command);
					}
					MDoubleArray dAttribute;
					dAttribute.setLength(particles->numParticles());
					doubleAttrArrays[attrNames[i].asChar()] = dAttribute;
					userPPMapping[attrNames[i].asChar()] = mayaAttrNames[i].asChar();
				} else { // we don't know how to handle this attribute
					MGlobal::displayError(MString("PartioEmitter->skipping attr: " + MString(attrNames[i])));
				}
			} // if the particle system had the attribute
		} // looped through each particle attribute
		std::map <std::string, MVectorArray >::iterator vecIt;
		std::map <std::string, MDoubleArray >::iterator doubleIt;
		//loop through each particle and add attribute to partio object
		for (int i=0;i<particles->numParticles();i++){
			// add position
			const float * partioPositions = particles->data<float>(positionAttr,i);
			MPoint pos (partioPositions[0], partioPositions[1], partioPositions[2]);
			positions.append(pos);
			// add velocity if it is there
			if (hasVelo){
				const float * partioVelocities = particles->data<float>(velocityAttr,i);
				MPoint vel (partioVelocities[0], partioVelocities[1], partioVelocities[2]);
				velocities.append(vel);
			}
			// add vector attributes if they are present
			if (vectorAttrArrays.size() > 0){
				for (vecIt = vectorAttrArrays.begin(); vecIt != vectorAttrArrays.end(); vecIt++){
					ParticleAttribute vectorAttr;
					particles->attributeInfo(vecIt->first.c_str(), vectorAttr);
					const float* vecVal = particles->data<float>(vectorAttr, i);
					vectorAttrArrays[vecIt->first][i] = MVector(vecVal[0],vecVal[1],vecVal[2]);
				}
			}
			// add double attributes if they are present
			if (doubleAttrArrays.size() > 0){
				for (doubleIt = doubleAttrArrays.begin(); doubleIt != doubleAttrArrays.end(); doubleIt++) {
					ParticleAttribute doubleAttr;
					particles->attributeInfo(doubleIt->first.c_str(),doubleAttr);
					const float*  doubleVal = particles->data<float>(doubleAttr, i);
					doubleAttrArrays[doubleIt->first][i] = doubleVal[0];
				}
			}
		}// looped through and added attributes to particles
		// if the particles dont have velocity set the array length but dont fill in value
		// NOTE: this does not set data to NULL could be junk velocities in this array
		if (!hasVelo){
			velocities.setLength(particles->numParticles());
		}
		// if there are particles, release them
		if (particles){
			particles->release();
		}
		// emit the system
		partSys.setCount(positions.length());
		stat = partSys.emit(positions,velocities);
		// attach double attributes
		for (doubleIt = doubleAttrArrays.begin(); doubleIt != doubleAttrArrays.end(); doubleIt++) {
			partSys.setPerParticleAttribute(MString(userPPMapping[doubleIt->first].c_str()), doubleAttrArrays[doubleIt->first]);
		}
		// attach vector attributes
		for (vecIt = vectorAttrArrays.begin(); vecIt != vectorAttrArrays.end(); vecIt++) {
			partSys.setPerParticleAttribute(MString(userPPMapping[vecIt->first].c_str()), vectorAttrArrays[vecIt->first]);
		}
		// set this as initial state of particle system
		partSys.saveInitialState();
	} else { // we failed to initialize
		return stat;
	}// particle object was found and attached to
	
	return MStatus::kSuccess;
}

void PartioImport::printUsage(){
	// build string
	MString usage = "\n-----------------------------------------------------------------------------\n";
	usage += "\tpartioImport -p/particle <particleShapeName> [Options] </full/path/to/particleCacheFile> \n";
	usage += "\n";
	usage += "\t[Options]\n";
	usage += "\t\t-p/particle <particleShapeName> (if none defined, one will be created)";
	usage += "\t\t-atr/attribute (multi use) <cache Attr Name>  <PP attribute name>\n";
	usage += "\t\t     (position/velocity/id) are always imported \n";
	//usage += "\t\t-flp/flip  (flip y->z axis to go to Z up packages) \n";
	usage += "\n";
	usage += "\tExample:\n";
	usage += "\n";
	usage += "partioImport -p particleShape1 -atr \"pointColor\" \"rgbPP\" -atr \"opacity\" \"opacityPP\"  \"/file/path/to/fooBar.0001.prt\"  \n\n";
	//display
	MGlobal::displayInfo(usage);
}
