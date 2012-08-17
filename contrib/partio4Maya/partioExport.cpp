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

#include "partioExport.h"

#define  kAttributeFlagS	"-atr"
#define  kAttributeFlagL	"-attribute"
#define  kMinFrameFlagS	"-mnf"
#define  kMinFrameFlagL	"-minFrame"
#define  kMaxFrameFlagS	"-mxf"
#define  kMaxFrameFlagL	"-maxFrame"
#define  kHelpFlagS		"-h"
#define  kHelpFlagL		"-help"
#define  kPathFlagS		"-p"
#define  kPathFlagL		"-path"
#define  kFlipFlagS		"-flp"
#define  kFlipFlagL		"-flip"
#define  kFormatFlagS	"-f"
#define  kFormatFlagL	"-format"
#define  kFilePrefixFlagS	"-fp"
#define  kFilePrefixFlagL	"-filePrefix"

using namespace std;

bool PartioExport::hasSyntax() {
	return true;
}

// Has Syntax Similar to DynExport  to be a drop in replacement
MSyntax PartioExport::createSyntax() {
	MSyntax syntax;
	syntax.addFlag(kHelpFlagS, kHelpFlagL,  MSyntax::kNoArg);
	syntax.addFlag(kPathFlagS, kPathFlagL,  MSyntax::kString);
	syntax.addFlag(kAttributeFlagS, kAttributeFlagL, MSyntax::kString);
	syntax.makeFlagMultiUse( kAttributeFlagS );
	syntax.addFlag(kFlipFlagS, kFlipFlagL, MSyntax::kNoArg);
	syntax.addFlag(kFormatFlagS, kFormatFlagL, MSyntax::kString);
	syntax.addFlag(kMinFrameFlagS,kMinFrameFlagL, MSyntax::kLong);
	syntax.addFlag(kMaxFrameFlagS, kMaxFrameFlagL, MSyntax::kLong);
	syntax.addFlag(kFilePrefixFlagS,kFilePrefixFlagL, MSyntax::kString);
	syntax.addArg(MSyntax::kString);
	syntax.enableQuery(false);
	syntax.enableEdit(false);
	return syntax;
}

void* PartioExport::creator() {
	return new PartioExport;
}

MStatus PartioExport::doIt(const MArgList& Args) {
	MStatus status;
	MArgParser argData(createSyntax(), Args, &status);
	// if they wanted help message print and exit
	if( argData.isFlagSet(kHelpFlagL)) {
		printUsage();
		return MStatus::kFailure;
	}
	// if there are les than 3 args print error info, usage and exit
	if( Args.length() < 3) {
		MGlobal::displayError("PartioExport-> need the EXPORT PATH and a PARTICLESHAPE's NAME, and at least one ATTRIBUTE's NAME you want to export." );
		printUsage();
		return MStatus::kFailure;
	}
	MString Path;   // directory path
	MString Format;
	MString fileNamePrefix;
	bool hasFilePrefix = false;
	// if they set a path
	if (argData.isFlagSet(kPathFlagL)) {
		argData.getFlagArgument(kPathFlagL, 0, Path);
	}
	// if they set the format
	if (argData.isFlagSet(kFormatFlagL)) {
		argData.getFlagArgument(kFormatFlagL, 0, Format);
	}
	// if they set the prefix
	if (argData.isFlagSet(kFilePrefixFlagL)) {
		argData.getFlagArgument(kFilePrefixFlagL, 0, fileNamePrefix);
		if (fileNamePrefix.length() > 0) {
			hasFilePrefix = true;
		}
	}
	// check for supported formats, error if not
	Format = Format.toLowerCase();
	if (Format != "pda" &&
			Format != "pdb" &&
			Format != "pdc" &&
			Format != "prt" &&
			Format != "bin" &&
			Format != "bgeo" &&
			Format != "geo" &&
			Format != "ptc" &&
			Format != "mc" &&
			Format != "rib" &&
			Format != "pts" &&
			Format != "xyz" &&
			Format != "pcd" &&
			Format != "rib" ) {
		MGlobal::displayError("PartioExport-> format is one of: pda,pdb,pdc,prt,bin,bgeo,geo,ptc,mc,rib,ass");
		return MStatus::kFailure;
	}
	bool startFrameSet = false;
	bool endFrameSet = false;
	int  startFrame, endFrame;
	// check if start frame is set
	if (argData.isFlagSet(kMinFrameFlagL)) {
		argData.getFlagArgument(kMinFrameFlagL, 0, startFrame);
		startFrameSet = true;
	} else {
		startFrame = -123456;
	}
	// check if end frame is set
	if (argData.isFlagSet(kMaxFrameFlagL)) {
		argData.getFlagArgument(kMaxFrameFlagL, 0, endFrame);
		endFrameSet = true;
	} else {
		endFrame = -123456;
	}
	// set if flip up vector is set
	bool swapUP = false;
	if (argData.isFlagSet(kFlipFlagL)){
		swapUP = true;
	}
	// get a link to the node to export
	MString PSName; // particle shape name
	argData.getCommandArgument(0, PSName);
	MSelectionList list;
	list.add(PSName);
	MObject objNode;
	list.getDependNode(0, objNode);
	MFnDependencyNode depNode(objNode, &status);
	// if this isn't a particle shape error
	if( objNode.apiType() != MFn::kParticle && objNode.apiType() != MFn::kNParticle ){
		MGlobal::displayError("PartioExport-> can't find your PARTICLESHAPE.");
		return MStatus::kFailure;
	}
	// parse attribute  flags
	unsigned int numUses = argData.numberOfFlagUses( kAttributeFlagL );
	MStringArray  attrNames;
	attrNames.append(MString("id"));
	attrNames.append(MString("position"));
	bool worldVeloCheck = false;
	// loop through remaining attribute flags
	for( unsigned int i = 0; i < numUses; i++ ){
		//get attr data and name
		MArgList argList;
		status = argData.getFlagArgumentList( kAttributeFlagL, i, argList );
		if( !status ) return status;
		MString AttrName = argList.asString( 0, &status );
		if( !status ) return status;
		// if it is position or id ignore
		if( AttrName == "position" || AttrName == "worldPosition"  || AttrName == "id" || AttrName == "particleId") {}
		else if( AttrName == "worldVelocity" || AttrName == "velocity" ){ // if it is velocity 
			// if we have not already recieved velocity
			if (!worldVeloCheck) {
				attrNames.append("velocity");
				worldVeloCheck = true;
			}
		} else { // this is a non standard attribute, append the name
			attrNames.append(AttrName);
		} // attributes attached to attr names
	}// set up attrNames array with attributes
	int outFrame= -123456;
	// loop through export frames
	for  (int frame = startFrame; frame<=endFrame; frame++) {
		// if we have start end and start is less than end, move the scene to this frame
		if (startFrameSet && endFrameSet && startFrame < endFrame) {
			MGlobal::viewFrame(frame);
			outFrame = frame;
		} else { // we are at the end frame, use this frame
			outFrame = (int)MAnimControl::currentTime().as(MTime::kFilm);
		}// we are at the current frame
		char padNum [10];
		// temp usage for this..  PDC's  are counted by 250s..  TODO:  implement  "substeps"  setting
		// set up step size or padding num
		if (Format == "pdc") {
			int substepFrame = outFrame;
			substepFrame = outFrame*250;
			sprintf(padNum, "%d", substepFrame);
		} else {
			sprintf(padNum, "%04d", outFrame);
		}
		// set out path
		MString  outputPath =  Path;
		outputPath += "/";
		// if we have supplied a fileName prefix, then use it instead of the particle shape name
		if (hasFilePrefix) {
			outputPath += fileNamePrefix;
		} else {
			outputPath += PSName;
		}
		outputPath += ".";
		outputPath += padNum;
		outputPath += ".";
		outputPath += Format;
		MGlobal::displayInfo("PartioExport-> exporting: "+ outputPath);
		// get the particle system of the node
		MFnParticleSystem PS(objNode);
		unsigned int particleCount = PS.count();
		// add particle data from node prep iterator over it
		Partio::ParticlesDataMutable* p = Partio::createInterleave();
		p->addParticles((const int)particleCount);
		Partio::ParticlesDataMutable::iterator it=p->begin();
		// if we have particles
		if (particleCount > 0) {
			// loop through all our attributes
			for (unsigned int i = 0; i< attrNames.length(); i++) {
				// reset our particle iterator
				it=p->begin();
				MString  attrName =  attrNames[i];
				//  INT Attribute found
				if (PS.isPerParticleIntAttribute(attrName) || attrName == "id" || attrName == "particleId") {
					MIntArray IA;
					// if it is the id load it appropriately
					if ( attrName == "id" || attrName == "particleId" ) {
						PS.particleIds( IA );
						attrName = "id";
						if (Format == "pdc") {
							attrName = "particleId";
						}
					} else { // load the int attribute
						PS.getPerParticleAttribute( attrName , IA, &status);
					}
					// add integer attribute
					Partio::ParticleAttribute  intAttribute = p->addAttribute(attrName.asChar(), Partio::INT, 1);
					Partio::ParticleAccessor intAccess(intAttribute);
					it.addAccessor(intAccess);
					int a = 0;
					// load the data into partio
					for(it=p->begin();it!=p->end();++it){
						Partio::Data<int,1>& intAttr=intAccess.data<Partio::Data<int,1> >(it);
						intAttr[0] = IA[a];
						a++;
					}
				}// add INT attribute
				// DOUBLE Attribute found
				else if  (PS.isPerParticleDoubleAttribute(attrName)){
					MDoubleArray DA;
					// check for radius, ag, opacity or handle generic case
					if( attrName == "radius" || attrName  == "radiusPP") {
						attrName = "radiusPP";
						PS.radius( DA );
					} else if( attrName == "age" ) {
						PS.age( DA );
					} else if ( attrName == "opacity" || attrName == "opacityPP") {
						attrName = "opacityPP";
						PS.opacity( DA );
					} else {
						PS.getPerParticleAttribute( attrName , DA, &status);
					}
					//Set up partio attribute and accessor
					Partio::ParticleAttribute  floatAttribute = p->addAttribute(attrName.asChar(), Partio::FLOAT, 1);
					Partio::ParticleAccessor floatAccess(floatAttribute);
					it.addAccessor(floatAccess);
					int a = 0;
					// loop through partio object and add attribute to particles
					for(it=p->begin();it!=p->end();++it) {
						Partio::Data<float,1>& floatAttr=floatAccess.data<Partio::Data<float,1> >(it);
						floatAttr[0] = float(DA[a]);
						a++;
					}
				} // add double attr
				// VECTOR Attribute found
				else if (PS.isPerParticleVectorAttribute(attrName)) {
					MVectorArray VA;
					// check for standard attributes or handle generic case
					if( attrName == "position" ) {
						PS.position( VA );
					} else if ( attrName == "velocity") {
						PS.velocity( VA );
					} else if (attrName == "acceleration" ) {
						PS.acceleration( VA );
					} else if (attrName == "rgbPP" ) {
						PS.rgb( VA );
					} else if (attrName == "incandescencePP") {
						PS.emission( VA );
					} else {
						PS.getPerParticleAttribute( attrName , VA, &status);
					}
					// set up partio attribute and accessor
					Partio::ParticleAttribute  vectorAttribute = p->addAttribute(attrName.asChar(), Partio::VECTOR, 3);
					Partio::ParticleAccessor vectorAccess(vectorAttribute);
					it.addAccessor(vectorAccess);
					int a = 0;
					// loop through partio object and attach vector attribute to particle data
					for(it=p->begin();it!=p->end();++it){
						Partio::Data<float,3>& vecAttr=vectorAccess.data<Partio::Data<float,3> >(it);
						MVector P = VA[a];
						vecAttr[0] = (float)P.x;
						// if we are swapping up axis and this is a swappable attribute, do it
						if (swapUP && (attrName == "position" || attrName == "velocity" || attrName == "acceleration" )){
							vecAttr[1] = -(float)P.z;
							vecAttr[2] = (float)P.y;
						} else { // keep up as is
							vecAttr[1] = (float)P.y;
							vecAttr[2] = (float)P.z;
						}
						a++;
					}// looped through partio object and attached vectors
				} // add vector attr
			} // loop attributes
			// check if directory exists, and if not, create it
			struct stat st;
			if (stat(Path.asChar(),&st) < 0)
			{
				#ifdef WIN32
					HWND hwnd = NULL;
					const SECURITY_ATTRIBUTES *psa = NULL;
					SHCreateDirectoryEx(hwnd, Path.asChar(), psa);
				#else
					mkdir (Path.asChar(),0755);
				#endif
			}
			// write to our cache path
			Partio::write(outputPath.asChar(), *p );
			// release memory from partio
			p->release();
		} // if particle count > 0
	} // loop frames
	return MStatus::kSuccess;
}

void PartioExport::printUsage(){
	MString usage = "\n-----------------------------------------------------------------------------\n";
	usage += "\tpartioExport [Options]  node \n";
	usage += "\n";
	usage += "\t[Options]\n";
	usage += "\t\t-mnf/minFrame <int> \n";
	usage += "\t\t-mxf/maxFrame <int> \n";
	usage += "\t\t-f/format <string> (format is one of: pda,pdb,pdc,prt,bin,bgeo,geo,ptc,mc,pts,xyz,pcd,rib,ass)\n";
	usage += "\t\t-atr/attribute (multi use)  <PP attribute name>\n";
	usage += "\t\t     (position/velocity/id) are always exported \n";
	usage += "\t\t-p/path	 <directory file path> \n";
	usage += "\t\t-fp/filePrefix <fileNamePrefix>  \n";
	usage += "\t\t-flp/flip  (flip y->z axis to go to Z up packages) \n";
	usage += "\n";
	usage += "\tExample:\n";
	usage += "\n";
	usage += "partioExport  -mnf 1 -mxf 10 -f prt -atr position -atr rgbPP -at opacityPP -fp \"SmokeShapepart1of20\" -p \"/file/path/to/output/directory\"  particleShape1 \n\n";

	MGlobal::displayInfo(usage);
}
