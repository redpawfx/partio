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

#include "partioVisualizer.h"
#include "partioInstancer.h"
#include "partioEmitter.h"
#include "partioExport.h"
#include "partioImport.h"
#include "partio4MayaShared.h"
#include "partioCache.h"
#include <maya/MFnPlugin.h>

#define REGISTER_CACHE_FORMAT(n)\
    if (n >= 0 && n < PartioCache::TotalNumFormats())\
    {\
        status = plugin.registerCacheFormat(MString("partio-")+PartioCache::FormatExtension(n), PartioCache::Create<n>);\
        if (!status)\
        {\
            status.perror("registerCacheFormat " + MString(PartioCache::FormatExtension(n)) + " failed");\
        }\
    }

#define DEREGISTER_CACHE_FORMAT(n)\
    if (n >= 0 && n < PartioCache::TotalNumFormats())\
    {\
        status = plugin.deregisterCacheFormat(MString("partio-")+PartioCache::FormatExtension(n));\
        if (!status)\
        {\
            status.perror("deregisterCacheFormat " + MString(PartioCache::FormatExtension(n)) + " failed");\
        }\
    }


#ifdef _WIN32
__declspec(dllexport)
#else
__attribute__ ((visibility("default")))
#endif
MStatus initializePlugin ( MObject obj )
{

    // source  mel scripts this way if they're missing from the script path it will alert the user...
    MGlobal::executeCommand("source AEpartioEmitterTemplate.mel");
    MGlobal::executeCommand("source AEpartioVisualizerTemplate.mel");
    MGlobal::executeCommand("source AEpartioInstancerTemplate.mel");
    MGlobal::executeCommand("source partioExportGui.mel");
    MGlobal::executeCommand("source partioUtils.mel");

    MStatus status;
    MFnPlugin plugin ( obj, "RedpawFX,Luma Pictures,WDAS", "0.9.7a", "Any" );

    status = plugin.registerShape( "partioVisualizer", partioVisualizer::id,
                                   &partioVisualizer::creator,
                                   &partioVisualizer::initialize,
                                   &partioVisualizerUI::creator);


    if ( !status )
    {
        status.perror ( "registerNode partioVisualizer failed" );
        return status;
    }
    status = plugin.registerShape( "partioInstancer", partioInstancer::id,
                                   &partioInstancer::creator,
                                   &partioInstancer::initialize,
                                   &partioInstancerUI::creator);


    if ( !status )
    {
        status.perror ( "registerNode partioInstancer failed" );
        return status;
    }


    status = plugin.registerNode ( "partioEmitter", partioEmitter::id,
                                   &partioEmitter::creator, &partioEmitter::initialize,
                                   MPxNode::kEmitterNode );
    if ( !status )
    {
        status.perror ( "registerNode partioEmitter failed" );
        return status;
    }

    status = plugin.registerCommand("partioExport",PartioExport::creator, PartioExport::createSyntax );
    if (!status)
    {
        status.perror("registerCommand partioExport failed");
        return status;
    }

    status = plugin.registerCommand("partioImport",PartioImport::creator, PartioImport::createSyntax );
    if (!status)
    {
        status.perror("registerCommand partioImport failed");
    }
    
    // Register up to 20 cache formats
    REGISTER_CACHE_FORMAT(0);
    REGISTER_CACHE_FORMAT(1);
    REGISTER_CACHE_FORMAT(2);
    REGISTER_CACHE_FORMAT(3);
    REGISTER_CACHE_FORMAT(4);
    REGISTER_CACHE_FORMAT(5);
    REGISTER_CACHE_FORMAT(6);
    REGISTER_CACHE_FORMAT(7);
    REGISTER_CACHE_FORMAT(8);
    REGISTER_CACHE_FORMAT(9);
    REGISTER_CACHE_FORMAT(10);
    REGISTER_CACHE_FORMAT(11);
    REGISTER_CACHE_FORMAT(12);
    REGISTER_CACHE_FORMAT(13);
    REGISTER_CACHE_FORMAT(14);
    REGISTER_CACHE_FORMAT(15);
    REGISTER_CACHE_FORMAT(16);
    REGISTER_CACHE_FORMAT(17);
    REGISTER_CACHE_FORMAT(18);
    REGISTER_CACHE_FORMAT(19);
    REGISTER_CACHE_FORMAT(20);
    
    return status;
}

#ifdef _WIN32
__declspec(dllexport)
#else
__attribute__ ((visibility("default")))
#endif
MStatus uninitializePlugin ( MObject obj )
{
    MStatus status;
    MFnPlugin plugin ( obj );

    status = plugin.deregisterNode ( partioVisualizer::id );
    if ( !status )
    {
        status.perror ( "deregisterNode partioVisualizer failed" );
        return status;
    }
    status = plugin.deregisterNode ( partioInstancer::id );
    if ( !status )
    {
        status.perror ( "deregisterNode partioInstancer failed" );
        return status;
    }

    status = plugin.deregisterNode ( partioEmitter::id );
    if ( !status )
    {
        status.perror ( "deregisterNode partioEmitter failed" );
        return status;
    }

    status = plugin.deregisterCommand("partioExport");
    if (!status)
    {
        status.perror("deregisterCommand partioExport failed");
        return status;
    }
    status = plugin.deregisterCommand("partioImport");
    if (!status)
    {
        status.perror("deregisterCommand partioImport failed");
    }
    
    // Cache formats
    DEREGISTER_CACHE_FORMAT(0);
    DEREGISTER_CACHE_FORMAT(1);
    DEREGISTER_CACHE_FORMAT(2);
    DEREGISTER_CACHE_FORMAT(3);
    DEREGISTER_CACHE_FORMAT(4);
    DEREGISTER_CACHE_FORMAT(5);
    DEREGISTER_CACHE_FORMAT(6);
    DEREGISTER_CACHE_FORMAT(7);
    DEREGISTER_CACHE_FORMAT(8);
    DEREGISTER_CACHE_FORMAT(9);
    DEREGISTER_CACHE_FORMAT(10);
    DEREGISTER_CACHE_FORMAT(11);
    DEREGISTER_CACHE_FORMAT(12);
    DEREGISTER_CACHE_FORMAT(13);
    DEREGISTER_CACHE_FORMAT(14);
    DEREGISTER_CACHE_FORMAT(15);
    DEREGISTER_CACHE_FORMAT(16);
    DEREGISTER_CACHE_FORMAT(17);
    DEREGISTER_CACHE_FORMAT(18);
    DEREGISTER_CACHE_FORMAT(19);
    DEREGISTER_CACHE_FORMAT(20);
    
    return status;

}
