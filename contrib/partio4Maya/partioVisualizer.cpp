/* partio4Maya  3/12/2012, John Cassella  http://luma-pictures.com and  http://redpawfx.com
PARTIO Visualizer
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

static MGLFunctionTable *gGLFT = NULL;


// id is registered with autodesk no need to change
#define ID_PARTIOVISUALIZER  0x00116ECF

#define LEAD_COLOR				18	// green
#define ACTIVE_COLOR			15	// white
#define ACTIVE_AFFECTED_COLOR	8	// purple
#define DORMANT_COLOR			4	// blue
#define HILITE_COLOR			17	// pale blue

// define this as an enum?
#define DRAW_STYLE_POINTS 0
#define DRAW_STYLE_RADIUS 1
#define DRAW_STYLE_DISK	 2
#define DRAW_STYLE_BOUNDING_BOX 3

using namespace Partio;
using namespace std;

/// ///////////////////////////////////////////////////
/// PARTIO VISUALIZER

MTypeId partioVisualizer::id( ID_PARTIOVISUALIZER );

MObject partioVisualizer::time;
MObject partioVisualizer::aByFrame;
MObject partioVisualizer::aDrawSkip;
MObject partioVisualizer::aUpdateCache;
MObject partioVisualizer::aSize;         // The size of the logo
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
MObject partioVisualizer::aPartioPosAttributes;
MObject partioVisualizer::aPartioVelAttributes;
MObject partioVisualizer::aColorFrom;
MObject partioVisualizer::aRadiusFrom;
MObject partioVisualizer::aAlphaFrom;
MObject partioVisualizer::aPartioDisplayPartitions;
MObject partioVisualizer::aPointSize;
MObject partioVisualizer::aDefaultPointColor;
MObject partioVisualizer::aDefaultAlpha;
MObject partioVisualizer::aDefaultRadius;
MObject partioVisualizer::aInvertAlpha;
MObject partioVisualizer::aDrawStyle;
MObject partioVisualizer::aForceReload;
MObject partioVisualizer::aRenderCachePath;
MObject partioVisualizer::aExpNumCopies;
MObject partioVisualizer::aExpandVelo;
MObject partioVisualizer::aExpandType;
MObject partioVisualizer::aJitterStrength;
MObject partioVisualizer::aMaxJitter;
MObject partioVisualizer::aAdvectStrength;
MObject partioVisualizer::aVelocityStretch;
MObject partioVisualizer::aSearchDistance;


partioVizReaderCache::partioVizReaderCache():
        bbox(MBoundingBox(MPoint(0,0,0,0),MPoint(0,0,0,0))),
        dList(0),
        particles(NULL),
        rgb(NULL),
        rgba(NULL),
        flipPos(NULL)
{
}


/// CREATOR
partioVisualizer::partioVisualizer()
        :   mLastFileLoaded(""),
        mLastAlpha(0.0),
        mLastInvertAlpha(false),
        mLastPath(""),
        mLastFile(""),
        mLastExt(""),
        mLastColorFromIndex(-1),
        mLastAlphaFromIndex(-1),
        mLastRadiusFromIndex(-1),
        mLastColor(1,0,0),
        mLastRadius(1.0),
        mLastNumCopies(0),
        mLastExpandType(0),
        mLastJitterStrength(0),
        mLastAdvectStrength(0),
        mLastVelocityStretch(0),
        mLastSearchDistance(999999),
        somethingChanged(false),
        frameChanged(false),
        multiplier(1.0),
        mFlipped(false),
        drawError(0)
{
    pvCache.particles = NULL;
    pvCache.flipPos = (float *) malloc(sizeof(float));
    pvCache.rgb = (float *) malloc (sizeof(float));
    pvCache.rgba = (float *) malloc(sizeof(float));
    pvCache.radius = MFloatArray();
}
/// DESTRUCTOR
partioVisualizer::~partioVisualizer()
{

    if (pvCache.particles)
    {
        pvCache.particles->release();
    }
    free(pvCache.flipPos);
    free(pvCache.rgb);
    free(pvCache.rgba);
    pvCache.radius.clear();
    MSceneMessage::removeCallback( partioVisualizerOpenCallback);
    MSceneMessage::removeCallback( partioVisualizerImportCallback);
    MSceneMessage::removeCallback( partioVisualizerReferenceCallback);

}

void* partioVisualizer::creator()
{
    return new partioVisualizer;
}

/// POST CONSTRUCTOR
void partioVisualizer::postConstructor()
{
    setRenderable(true);
    partioVisualizerOpenCallback = MSceneMessage::addCallback(MSceneMessage::kAfterOpen, partioVisualizer::reInit, this);
    partioVisualizerImportCallback = MSceneMessage::addCallback(MSceneMessage::kAfterImport, partioVisualizer::reInit, this);
    partioVisualizerReferenceCallback = MSceneMessage::addCallback(MSceneMessage::kAfterReference, partioVisualizer::reInit, this);
}

///////////////////////////////////
/// init after opening
///////////////////////////////////

void partioVisualizer::initCallback()
{

    MObject tmo = thisMObject();

    short extENum;
    MPlug(tmo, aCacheFormat).getValue(extENum);

    mLastExt = partio4Maya::setExt(extENum);

    MPlug(tmo,aCacheDir).getValue(mLastPath);
    MPlug(tmo,aCacheFile).getValue(mLastFile);
    MPlug(tmo,aDefaultAlpha).getValue(mLastAlpha);
    MPlug(tmo,aDefaultRadius).getValue(mLastRadius);
    MPlug(tmo,aColorFrom).getValue(mLastColorFromIndex);
    MPlug(tmo,aAlphaFrom).getValue(mLastAlphaFromIndex);
    MPlug(tmo,aRadiusFrom).getValue(mLastRadiusFromIndex);
    MPlug(tmo,aSize).getValue(multiplier);
    MPlug(tmo,aInvertAlpha).getValue(mLastInvertAlpha);
    MPlug(tmo,aCacheStatic).getValue(mLastStatic);
	MPlug(tmo,aExpNumCopies).getValue(mLastNumCopies);
	MPlug(tmo,aExpandType).getValue(mLastExpandType);
	MPlug(tmo,aJitterStrength).getValue(mLastJitterStrength);
	MPlug(tmo,aAdvectStrength).getValue(mLastAdvectStrength);
	MPlug(tmo,aVelocityStretch).getValue(mLastVelocityStretch);
	MPlug(tmo,aSearchDistance).getValue(mLastSearchDistance);
	
    somethingChanged = false;

}

void partioVisualizer::reInit(void *data)
{
    partioVisualizer  *vizNode = (partioVisualizer*) data;
    vizNode->initCallback();
}


MStatus partioVisualizer::initialize()
{

    MFnEnumAttribute 	eAttr;
    MFnUnitAttribute 	uAttr;
    MFnNumericAttribute nAttr;
    MFnTypedAttribute 	tAttr;
    MStatus				stat;

    time = nAttr.create( "time", "tm", MFnNumericData::kLong ,0);
    uAttr.setKeyable( true );

	aByFrame = nAttr.create( "byFrame", "byf", MFnNumericData::kInt ,1);
    nAttr.setKeyable( true );
    nAttr.setReadable( true );
    nAttr.setWritable( true );
    nAttr.setConnectable( true );
    nAttr.setStorable( true );
    nAttr.setMin(1);
    nAttr.setMax(100);

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
    for (unsigned short i = 0; i< formatExtMap.size(); i++)
    {
        eAttr.addField(formatExtMap[i].toUpperCase(),	i);
    }

    eAttr.setDefault(short(Partio::readFormatIndex("pdc")));  // PDC
    eAttr.setChannelBox(true);

    aDrawStyle = eAttr.create( "drawStyle", "drwStyl");
    eAttr.addField("points",	0);
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
    tAttr.setUsesArrayDataBuilder( true );

	aPartioPosAttributes = tAttr.create ("partioPosAttributes", "pioPAts", MFnStringData::kString);
    tAttr.setArray(true);
    tAttr.setUsesArrayDataBuilder( true );

	aPartioVelAttributes = tAttr.create ("partioVelAttributes", "pioVAts", MFnStringData::kString);
    tAttr.setArray(true);
    tAttr.setUsesArrayDataBuilder( true );

	aPartioDisplayPartitions = nAttr.create ("displayPartitionData", "dpd", MFnNumericData::kBoolean);
    nAttr.setArray(true);
    nAttr.setUsesArrayDataBuilder( true );

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

	aExpNumCopies = nAttr.create("expandNumCopies", "exnc", MFnNumericData::kInt,0);
	nAttr.setKeyable(false);

	aSearchDistance = nAttr.create("searchDistance", "srchd", MFnNumericData::kFloat, 1000.0, &stat);
    nAttr.setDefault(1000.0);
    nAttr.setMin(0.0);
    nAttr.setKeyable(true);
	
	aExpandVelo = nAttr.create("expandVelocity", "expV", MFnNumericData::kBoolean, false, &stat);
    nAttr.setKeyable(false);

	aExpandType = eAttr.create( "expandType", "exTyp");
    eAttr.addField("StaticRand-Fast", EXPAND_RAND_STATIC_OFFSET_FAST);
	eAttr.addField("StaticRand-Better", EXPAND_RAND_STATIC_OFFSET_BETTER);
    eAttr.addField("JitterRand-Fast", EXPAND_JITTERPOINT_FAST);
	eAttr.addField("JitterRand-Better", EXPAND_JITTERPOINT_BETTER);
    eAttr.addField("VeloAdvect", EXPAND_VELO_ADVECT);
	//eAttr.addField("Infilling", EXPAND_INFILLING);
	eAttr.setDefault(0);
    eAttr.setChannelBox(true);

    aJitterStrength = nAttr.create("jitterStrength", "jstr", MFnNumericData::kFloat, 0.0, &stat);
    nAttr.setDefault(0.1);
    nAttr.setMin(0.0);
    nAttr.setKeyable(true);

	aMaxJitter = nAttr.create("maxJitterStrength", "mjstr", MFnNumericData::kFloat, 1.0, &stat);
    nAttr.setDefault(1.0);
    nAttr.setMin(0.0);
    nAttr.setKeyable(true);

	aAdvectStrength = nAttr.create("advectStrength", "advs", MFnNumericData::kFloat, 1.0, &stat);
    nAttr.setDefault(1.0);
    nAttr.setMin(0.0);
    nAttr.setKeyable(true);
	
	aVelocityStretch = nAttr.create("velocityStretch", "vels", MFnNumericData::kFloat, 0.0, &stat);
    nAttr.setDefault(0.0);
    nAttr.setMin(0.0);
    nAttr.setKeyable(true);

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
	addAttribute ( aPartioPosAttributes );
	addAttribute ( aPartioVelAttributes );
    addAttribute ( aColorFrom );
    addAttribute ( aAlphaFrom );
    addAttribute ( aRadiusFrom );
	addAttribute ( aPartioDisplayPartitions );
    addAttribute ( aPointSize );
    addAttribute ( aDefaultPointColor );
    addAttribute ( aDefaultAlpha );
    addAttribute ( aInvertAlpha );
    addAttribute ( aDefaultRadius );
    addAttribute ( aDrawStyle );
    addAttribute ( aForceReload );
    addAttribute ( aRenderCachePath );
	addAttribute ( aExpNumCopies );
	addAttribute ( aExpandVelo );
	addAttribute ( aExpandType );
	addAttribute ( aSearchDistance );
	addAttribute ( aJitterStrength );
	addAttribute ( aMaxJitter );
	addAttribute ( aAdvectStrength );
	addAttribute ( aVelocityStretch );
	addAttribute ( aByFrame );
    addAttribute ( time );

	attributeAffects ( aCacheActive, aUpdateCache);
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
	attributeAffects ( aPartioDisplayPartitions, aUpdateCache );
    attributeAffects ( aPointSize, aUpdateCache );
    attributeAffects ( aDefaultPointColor, aUpdateCache );
    attributeAffects ( aDefaultAlpha, aUpdateCache );
    attributeAffects ( aInvertAlpha, aUpdateCache );
    attributeAffects ( aDefaultRadius, aUpdateCache );
    attributeAffects ( aDrawStyle, aUpdateCache );
    attributeAffects ( aForceReload, aUpdateCache );
	attributeAffects ( aByFrame, aUpdateCache );
	attributeAffects ( aByFrame, aRenderCachePath );
	attributeAffects ( aExpNumCopies, aUpdateCache );
	attributeAffects ( aExpandVelo, aUpdateCache );
	attributeAffects ( aExpandType, aUpdateCache );
	attributeAffects ( aSearchDistance, aUpdateCache );
	attributeAffects ( aJitterStrength, aUpdateCache );
	attributeAffects ( aMaxJitter, aUpdateCache );
	attributeAffects ( aAdvectStrength, aUpdateCache );
	attributeAffects ( aVelocityStretch, aUpdateCache );
	attributeAffects (time, aUpdateCache);
    attributeAffects (time,aRenderCachePath);


    return MS::kSuccess;
}


partioVizReaderCache* partioVisualizer::updateParticleCache()
{
    GetPlugData(); // force update to run compute function where we want to do all the work
    return &pvCache;
}

// COMPUTE FUNCTION

MStatus partioVisualizer::compute( const MPlug& plug, MDataBlock& block )
{

	//cout << "Compute: " <<  plug.name() << endl;
    int colorFromIndex  = block.inputValue( aColorFrom ).asInt();
    int opacityFromIndex= block.inputValue( aAlphaFrom ).asInt();
    int radiusFromIndex = block.inputValue( aRadiusFrom ).asInt();
    bool cacheActive = block.inputValue(aCacheActive).asBool();

    // Determine if we are requesting the output plug for this node.
    //
    if (plug != aUpdateCache)
    {
        return ( MS::kUnknownParameter );
    }

    else
    {
        MStatus stat;

        MString cacheDir	= block.inputValue(aCacheDir).asString();
        MString cacheFile	= block.inputValue(aCacheFile).asString();

		// draw error controls the draw color of the partio logo and indicates missing cache data
        drawError = 0;
        if (cacheDir  == "" || cacheFile == "" )
        {
			// too much printing  rather force draw of icon to red or something
            drawError = 1;
			return ( MS::kFailure );
        }

        bool cacheStatic			= block.inputValue( aCacheStatic ).asBool();
        int cacheOffset 			= block.inputValue( aCacheOffset ).asInt();
        short cacheFormat			= block.inputValue( aCacheFormat ).asShort();
        MFloatVector defaultColor 	= block.inputValue( aDefaultPointColor ).asFloatVector();
        float defaultAlpha 			= block.inputValue( aDefaultAlpha ).asFloat();
        bool invertAlpha 			= block.inputValue( aInvertAlpha ).asBool();
        float defaultRadius			= block.inputValue( aDefaultRadius).asFloat();
        bool forceReload 			= block.inputValue( aForceReload ).asBool();
        int integerTime				= block.inputValue(time).asInt();
		int byFrame 				= block.inputValue( aByFrame ).asInt();
        bool flipYZ 				= block.inputValue( aFlipYZ ).asBool();
        MString renderCachePath 	= block.inputValue( aRenderCachePath ).asString();
		int expNumCopies			= block.inputValue( aExpNumCopies ).asInt();
		bool expandVelocity			= block.inputValue( aExpandVelo ).asBool();
		int  expType				= block.inputValue( aExpandType ).asShort();
		float jitterStrength		= block.inputValue( aJitterStrength ).asFloat();
		float maxJitter				= block.inputValue( aMaxJitter ).asFloat();
		float advectStrength		= block.inputValue( aAdvectStrength ).asFloat();
		float veloctityStretch		= block.inputValue( aVelocityStretch ).asFloat();
		float searchDistance		= block.inputValue( aSearchDistance ).asFloat();

        MString formatExt = "";
        int cachePadding = 0;
        MString newCacheFile = "";
        MString renderCacheFile = "";

        partio4Maya::updateFileName( cacheFile,  cacheDir,
                                     cacheStatic,  cacheOffset,
                                     cacheFormat,  integerTime, byFrame,
                                     cachePadding, formatExt,
                                     newCacheFile, renderCacheFile
                                   );

        if (renderCachePath != renderCacheFile || forceReload )
        {
            block.outputValue(aRenderCachePath).setString(renderCacheFile);
        }

        somethingChanged = false;

//////////////////////////////////////////////
/// Cache can change manually by changing one of the parts of the cache input...
        if (mLastExt != formatExt ||
            mLastPath != cacheDir ||
            mLastFile != cacheFile ||
            mLastFlipStatus  != flipYZ ||
            mLastStatic !=  cacheStatic ||
            mLastNumCopies != expNumCopies ||
            mLastExpandType != expType ||
            mLastJitterStrength != jitterStrength ||
            mLastMaxJitterStrength != maxJitter ||
            mLastAdvectStrength != advectStrength ||
            mLastVelocityStretch != veloctityStretch ||
            mLastSearchDistance != searchDistance ||
            newCacheFile != mLastFileLoaded ||
            forceReload
			)
        {
            somethingChanged = true;
            mFlipped = false;
            mLastFlipStatus = flipYZ;
            mLastExt = formatExt;
            mLastPath = cacheDir;
            mLastFile = cacheFile;
            mLastStatic = cacheStatic;
			mLastNumCopies = expNumCopies;
			mLastExpandType = expType;
			mLastJitterStrength = jitterStrength;
			mLastMaxJitterStrength = maxJitter;
			mLastAdvectStrength = advectStrength;
			mLastVelocityStretch = veloctityStretch;
			mLastSearchDistance = searchDistance;
            block.outputValue(aForceReload).setBool(false);
        }

//////////////////////////////////////////////
/// or it can change from a time input change


        if (newCacheFile != "" && !partio4Maya::partioCacheExists(newCacheFile.asChar()) || !cacheActive)
        {
			if (pvCache.particles != NULL)
			{
				ParticlesDataMutable* newParticles;
				newParticles = pvCache.particles;
				pvCache.particles=NULL; // resets the particles

				if (newParticles != NULL)
				{
					newParticles->release();
				}
			}
            pvCache.bbox.clear();
			mLastFileLoaded = "";
			drawError = 1;
			if (!cacheActive)
			{
				drawError = 2;
			}
			return ( MS::kSuccess );

        }

        if (somethingChanged)
        {
            mFlipped = false;
            MGlobal::displayInfo(MString("PartioVisualizer->Loading: " + newCacheFile));

			if(pvCache.particles != NULL)
			{
				/////////////////////////////////////////////
				/// This seems to work to solve the mem leak
				ParticlesDataMutable* newParticles;
				newParticles = pvCache.particles;
				pvCache.particles=NULL; // resets the pointer

				if (newParticles != NULL)
				{
					newParticles->release(); // frees the mem
				}
			}


			// NOW read the new particle data
			pvCache.particles = read(newCacheFile.asChar());

			///TODO: test here to see if we've already got some partitions in the particle cache
			int partitionsFromFile = 0;

			// we only support one way for expansion you can't use both together
			// partitions from file overrides internal expansion
			if (expNumCopies > 0 && partitionsFromFile == 0)
			{
				// calc  FPS value for  correct velocity multing
				MTime oneSec ( 1.0, MTime::kSeconds );
				int fps = ( int ) oneSec.asUnits ( MTime::uiUnit() );
	
				pvCache.particles = expandSoft(pvCache.particles, true, expNumCopies, expandVelocity,
											   expType, jitterStrength, maxJitter, advectStrength,
												veloctityStretch,fps, searchDistance);
			}
			///////////////////////////////////////

            mLastFileLoaded = newCacheFile;
            if (pvCache.particles->numParticles() == 0)
            {
                return (MS::kSuccess);
            }

			// get the attr list from the cache
			mAttributeList.clear();
			std::string attrs = getAttrList(pvCache.particles);
			MString attrList = MString(attrs.c_str());
			attrList.split(',',mAttributeList);

			// DEBUG
			//cout << mAttributeList << endl;

			// now lets search thru the attr list and get a separate list of the position and velocity attrs
			mPosAttrs.clear();
			mVelAttrs.clear();

			pvCache.partitions.clear();

			partio4Maya::findPosAndVelAttrs(mAttributeList, mPosAttrs, mVelAttrs);
			// DEBUG
			//cout << mPosAttrs << endl;
			//cout << mVelAttrs << endl;

			// first we store the values from the AE / attributes into the global variable so we can use it elsewhere and so it sticks
			MPlug partitionPlug (thisMObject(), aPartioDisplayPartitions);
			uint partitionAttrCount = partitionPlug.numElements();
			if ( displayPartition.length() != 0 && displayPartition.length() <= partitionAttrCount)
			{
				for (uint i=0;i<displayPartition.length();i++)
				{
						partitionPlug.selectAncestorLogicalIndex(i,aPartioDisplayPartitions);
						displayPartition[i] = partitionPlug.asInt();
				}
			}

			// then we check to resize the displayPartition count if needed before filling the vector of which pos attrs are selected

			if (displayPartition.length() == 0) // first run!
			{
				cout << "first run" << endl;
				displayPartition.setLength(mPosAttrs.length());
				displayPartition[0] = 1;
			}

			else if (displayPartition.length() < mPosAttrs.length())  //add entries to match
			{
				cout << "appending" << endl;
				for ( uint i = displayPartition.length(); i < mPosAttrs.length(); i++)
				{
					displayPartition.append(0);
				}
			}
			else if (displayPartition.length() > mPosAttrs.length()) // truncate
			{
				cout << "truncating" << endl;
				displayPartition.setLength(mPosAttrs.length());
			}

			// Makes a vector of all the position attrs that we want to display
			// this gets passed to the draw function.
			for (uint i = 0; i < mPosAttrs.length(); i++)
			{
				if (displayPartition[i])
				{
					ParticleAttribute temp;
					if (pvCache.particles->attributeInfo(mPosAttrs[i].asChar(),temp))
					{
						// DEBUG
						//cout << "ATTRIBUTES: " <<  temp.name << endl;
						pvCache.partitions.push_back(temp);
					}
				}
			}

//  if we're not loading any position data we can exit early, this shoud never really happen
			if (!pvCache.partitions.size())
			{
				return ( MS::kSuccess );
			}


/// DISPLAY PARTICLE COUNT data to info line..
/// TODO: make this print to a custom HUD element instead
            char partCount[15];
            sprintf (partCount, "%d", pvCache.particles->numParticles());
			char partitionString[50];
			sprintf (partitionString, " in %i partitions" , pvCache.partitions.size());
            MGlobal::displayInfo(MString ("PartioVisualizer-> LOADED: ") + partCount + MString (" particles") + partitionString );


            float * floatToRGB = (float *) realloc(pvCache.rgb, pvCache.particles->numParticles()*sizeof(float)*3);
            if (floatToRGB != NULL)
            {
                pvCache.rgb =  floatToRGB;
            }
            else
            {
                free(pvCache.rgb);
                MGlobal::displayError("PartioVisualizer->unable to allocate new memory for particles");
                return (MS::kFailure);
            }

            float * newRGBA = (float *) realloc(pvCache.rgba,pvCache.particles->numParticles()*sizeof(float)*4);
            if (newRGBA != NULL)
            {
                pvCache.rgba =  newRGBA;
            }
            else
            {
                free(pvCache.rgba);
                MGlobal::displayError("PartioVisualizer->unable to allocate new memory for particles");
                return (MS::kFailure);
            }

            pvCache.radius.clear();
            pvCache.radius.setLength(pvCache.particles->numParticles());

            pvCache.bbox.clear();

            /// TODO: possibly move this down to take into account the radius as well for a true bounding volume
            // resize the bounding box including partitions
            for (int i=0;i<pvCache.particles->numParticles();i++)
			{
				for (uint j = 0; j< pvCache.partitions.size(); j++)
				{
					const float * partioPositions = pvCache.particles->data<float>(pvCache.partitions[j],i);
					MPoint pos (partioPositions[0], partioPositions[1], partioPositions[2]);
					pvCache.bbox.expand(pos);
				}
			}

            block.outputValue(aForceReload).setBool(false);
            block.setClean(aForceReload);

        }

////////////////////////////////////////////////////////////////
// now we sort out the settings for color, opacity, and radius
        if (pvCache.particles)
        {
			// something changed..
            if  (somethingChanged || mLastColorFromIndex != colorFromIndex || mLastColor != defaultColor)
            {
                int numAttrs = pvCache.particles->numAttributes();
                if (colorFromIndex+1 > numAttrs || opacityFromIndex+1 > numAttrs || radiusFromIndex+1 > numAttrs)
                {
                    // reset the attrs
                    block.outputValue(aColorFrom).setInt(-1);
                    block.setClean(aColorFrom);
                    block.outputValue(aAlphaFrom).setInt(-1);
                    block.setClean(aAlphaFrom);
                    block.outputValue(aRadiusFrom).setInt(-1);
                    block.setClean(aRadiusFrom);

                    colorFromIndex  = block.inputValue( aColorFrom ).asInt();
                    opacityFromIndex= block.inputValue( aAlphaFrom ).asInt();
                    radiusFromIndex = block.inputValue( aRadiusFrom ).asInt();
                }

                if (colorFromIndex >=0)
                {
                    pvCache.particles->attributeInfo(colorFromIndex,pvCache.colorAttr);
                    // VECTOR or  4+ element float attrs
                    if (pvCache.colorAttr.type == VECTOR || pvCache.colorAttr.count > 3) // assuming first 3 elements are rgb
                    {
                        for (int i=0;i<pvCache.particles->numParticles();i++)
                        {
                            const float * attrVal = pvCache.particles->data<float>(pvCache.colorAttr,i);
                            pvCache.rgb[(i*3)] 		= attrVal[0];
                            pvCache.rgb[((i*3)+1)] 	= attrVal[1];
                            pvCache.rgb[((i*3)+2)] 	= attrVal[2];
                            pvCache.rgba[i*4] 		= attrVal[0];
                            pvCache.rgba[(i*4)+1] 	= attrVal[1];
                            pvCache.rgba[(i*4)+2] 	= attrVal[2];
                        }
                    }
                    else // single FLOAT
                    {
                        for (int i=0;i<pvCache.particles->numParticles();i++)
                        {
                            const float * attrVal = pvCache.particles->data<float>(pvCache.colorAttr,i);

                            pvCache.rgb[(i*3)] = pvCache.rgb[((i*3)+1)] = pvCache.rgb[((i*3)+2)] 	= attrVal[0];
                            pvCache.rgba[i*4]  = pvCache.rgba[(i*4)+1] 	= pvCache.rgba[(i*4)+2] 	= attrVal[0];
                        }
                    }
                }

                else
                {
                    for (int i=0;i<pvCache.particles->numParticles();i++)
                    {
                        pvCache.rgb[(i*3)] 		= defaultColor[0];
                        pvCache.rgb[((i*3)+1)] 	= defaultColor[1];
                        pvCache.rgb[((i*3)+2)] 	= defaultColor[2];
                        pvCache.rgba[i*4] 		= defaultColor[0];
                        pvCache.rgba[(i*4)+1] 	= defaultColor[1];
                        pvCache.rgba[(i*4)+2] 	= defaultColor[2];
                    }

                }
                mLastColorFromIndex = colorFromIndex;
                mLastColor = defaultColor;
            }

            if  (somethingChanged || opacityFromIndex != mLastAlphaFromIndex || defaultAlpha != mLastAlpha || invertAlpha != mLastInvertAlpha)
            {
                if (opacityFromIndex >=0)
                {
                    pvCache.particles->attributeInfo(opacityFromIndex,pvCache.opacityAttr);
                    if (pvCache.opacityAttr.count == 1)  // single float value for opacity
                    {
                        for (int i=0;i<pvCache.particles->numParticles();i++)
                        {
                            const float * attrVal = pvCache.particles->data<float>(pvCache.opacityAttr,i);
                            float temp = attrVal[0];
                            if (invertAlpha)
                            {
                                temp = float(1.0-temp);

                            }
                            pvCache.rgba[(i*4)+3] = temp;
                        }
                    }
                    else
                    {
                        if (pvCache.opacityAttr.count == 4)   // we have an  RGBA 4 float attribute ?
                        {
                            for (int i=0;i<pvCache.particles->numParticles();i++)
                            {
                                const float * attrVal = pvCache.particles->data<float>(pvCache.opacityAttr,i);
                                float temp = attrVal[3];
                                if (invertAlpha)
                                {
                                    temp = float(1.0-temp);
                                }
                                pvCache.rgba[(i*4)+3] = temp;
                            }
                        }
                        else
                        {
                            for (int i=0;i<pvCache.particles->numParticles();i++)
                            {
                                const float * attrVal = pvCache.particles->data<float>(pvCache.opacityAttr,i);
                                float lum = float((attrVal[0]*0.2126)+(attrVal[1]*0.7152)+(attrVal[2]*.0722));
                                if (invertAlpha)
                                {
                                    lum = float(1.0-lum);
                                }
                                pvCache.rgba[(i*4)+3] =  lum;
                            }
                        }
                    }
                }
                else
                {
                    mLastAlpha = defaultAlpha;
                    if (invertAlpha)
                    {
                        mLastAlpha= 1-defaultAlpha;
                    }
                    for (int i=0;i<pvCache.particles->numParticles();i++)
                    {
                        pvCache.rgba[(i*4)+3] = mLastAlpha;
                    }
                }
                mLastAlpha = defaultAlpha;
                mLastAlphaFromIndex =opacityFromIndex;
                mLastInvertAlpha = invertAlpha;
            }

            if  (somethingChanged || radiusFromIndex != mLastRadiusFromIndex || defaultRadius != mLastRadius )
            {
                if (radiusFromIndex >=0)
                {
                    pvCache.particles->attributeInfo(radiusFromIndex,pvCache.radiusAttr);
                    if (pvCache.radiusAttr.count == 1)  // single float value for radius
                    {
                        for (int i=0;i<pvCache.particles->numParticles();i++)
                        {
                            const float * attrVal = pvCache.particles->data<float>(pvCache.radiusAttr,i);
                            pvCache.radius[i] = attrVal[0] * defaultRadius;
                        }
                    }
                    else
                    {
                        for (int i=0;i<pvCache.particles->numParticles();i++)
                        {
                            const float * attrVal = pvCache.particles->data<float>(pvCache.radiusAttr,i);
                            float lum = float((attrVal[0]*0.2126)+(attrVal[1]*0.7152)+(attrVal[2]*.0722));
                            pvCache.radius[i] =  lum * defaultRadius;
                        }

                    }
                }
                else
                {
                    mLastRadius = defaultRadius;
                    for (int i=0;i<pvCache.particles->numParticles();i++)
                    {
                        pvCache.radius[i] = mLastRadius;
                    }
                }
                mLastRadius = defaultRadius;
                mLastRadiusFromIndex = radiusFromIndex;
            }
        }
    }  // end if plug = aUpdateCache


// we only update the AE Controls for attrs in the cache when we have particles

    if (pvCache.particles) // update the AE Controls for attrs in the cache
    {
        unsigned int numAttr=pvCache.particles->numAttributes();
        MPlug attrPlug (thisMObject(), aPartioAttributes);
		MPlug posPlug (thisMObject(), aPartioPosAttributes);
		MPlug velPlug (thisMObject(), aPartioVelAttributes);
		MPlug partPlug (thisMObject(), aPartioDisplayPartitions);

        if ((colorFromIndex+1) > int(attrPlug.numElements()))
        {
            block.outputValue(aColorFrom).setInt(-1);
        }
        if ((opacityFromIndex+1) > int(attrPlug.numElements()))
        {
            block.outputValue(aAlphaFrom).setInt(-1);
        }
        if ((radiusFromIndex+1) > int(attrPlug.numElements()))
        {
            block.outputValue(aRadiusFrom).setInt(-1);
        }

// update the AE Controls for attrs in the cache
        if (somethingChanged || attrPlug.numElements() != numAttr)
        {
			partio4Maya::updateMultiInput(mAttributeList, attrPlug, aPartioAttributes, numAttr, block);
        }
		if (somethingChanged || posPlug.numElements() != mPosAttrs.length())
		{
			partio4Maya::updateMultiInput(mPosAttrs, posPlug, aPartioPosAttributes, mPosAttrs.length(),block);
		}
		if (somethingChanged || velPlug.numElements() != mVelAttrs.length())
		{
			partio4Maya::updateMultiInput(mVelAttrs, velPlug, aPartioVelAttributes, mVelAttrs.length(),block);
		}
		if (somethingChanged || partPlug.numElements() != displayPartition.length())
		{
			for (unsigned int i=0;i<displayPartition.length();i++)
			{
				partPlug.selectAncestorLogicalIndex(i,aPartioDisplayPartitions);
				partPlug.setValue(displayPartition[i]);
			}

			MArrayDataHandle hPartioAttrs = block.inputArrayValue(aPartioDisplayPartitions);
			MArrayDataBuilder bPartioAttrs = hPartioAttrs.builder();
			// do we need to clean up some attributes from our array?
			if (bPartioAttrs.elementCount() > displayPartition.length())
			{
				unsigned int current = bPartioAttrs.elementCount();

				// remove excess elements from the end of our attribute array
				for (unsigned int x = displayPartition.length(); x < current; x++)
				{
					bPartioAttrs.removeElement(x);
				}
			}
		}
    }
    block.setClean(plug);
    return MS::kSuccess;
}


/////////////////////////////////////////////////////
/// procs to override bounding box mode...
bool partioVisualizer::isBounded() const
{
    return true;
}

MBoundingBox partioVisualizer::boundingBox() const
{
    // Returns the bounding box for the shape.
    partioVisualizer* nonConstThis = const_cast<partioVisualizer*>(this);
    partioVizReaderCache* geom = nonConstThis->updateParticleCache();
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
    MPoint corner1 = geom->bbox.min();
    MPoint corner2 = geom->bbox.max();
    return MBoundingBox( corner1, corner2 );
}

//
// Select function. Gets called when the bbox for the object is selected.
// This function just selects the object without doing any intersection tests.
//
bool partioVisualizerUI::select( MSelectInfo &selectInfo,
                                 MSelectionList &selectionList,
                                 MPointArray &worldSpaceSelectPts ) const
{
    MSelectionMask priorityMask( MSelectionMask::kSelectObjectsMask );
    MSelectionList item;
    item.add( selectInfo.selectPath() );
    MPoint xformedPt;
    selectInfo.addSelection( item, xformedPt, selectionList,
                             worldSpaceSelectPts, priorityMask, false );
    return true;
}

//////////////////////////////////////////////////////////////////
////  getPlugData is a util to update the drawing of the UI stuff

bool partioVisualizer::GetPlugData()
{
    MObject thisNode = thisMObject();
    int update = 0;
    MPlug updatePlug(thisNode, aUpdateCache );
    updatePlug.getValue( update );
    if (update != dUpdate)
    {
        dUpdate = update;
        return true;
    }
    else
    {
        return false;
    }
    return false;

}

// note the "const" at the end, its different than other draw calls
void partioVisualizerUI::draw( const MDrawRequest& request, M3dView& view ) const
{
    MDrawData data = request.drawData();

    partioVisualizer* shapeNode = (partioVisualizer*) surfaceShape();

    partioVizReaderCache* cache = (partioVizReaderCache*) data.geometry();

    MObject thisNode = shapeNode->thisMObject();
    MPlug sizePlug( thisNode, shapeNode->aSize );
    MDistance sizeVal;
    sizePlug.getValue( sizeVal );

    shapeNode->multiplier= (float) sizeVal.asCentimeters();

    int drawStyle = 0;
    MPlug drawStylePlug( thisNode, shapeNode->aDrawStyle );
    drawStylePlug.getValue( drawStyle );

    view.beginGL();

    if (drawStyle == 3 || view.displayStyle() == M3dView::kBoundingBox )
    {
        drawBoundingBox();
    }
    else
    {
        drawPartio(cache,drawStyle);
    }

	if (shapeNode->drawError == 1)
	{
		glColor3f(.75f,0.0f,0.0f);
	}
	else if (shapeNode->drawError == 2)
	{
		glColor3f(0.0f,0.0f,0.0f);
	}

	partio4Maya::drawPartioLogo(shapeNode->multiplier);

    view.endGL();
}

////////////////////////////////////////////
/// DRAW Bounding box
void  partioVisualizerUI::drawBoundingBox() const
{

    partioVisualizer* shapeNode = (partioVisualizer*) surfaceShape();

    MPoint  bboxMin = shapeNode->pvCache.bbox.min();
    MPoint  bboxMax = shapeNode->pvCache.bbox.max();

    float xMin = float(bboxMin.x);
    float yMin = float(bboxMin.y);
    float zMin = float(bboxMin.z);
    float xMax = float(bboxMax.x);
    float yMax = float(bboxMax.y);
    float zMax = float(bboxMax.z);

    /// draw the bounding box
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


////////////////////////////////////////////
/// DRAW PARTIO

void partioVisualizerUI::drawPartio(partioVizReaderCache* pvCache, int drawStyle) const
{
	partioVisualizer* shapeNode = (partioVisualizer*) surfaceShape();

    MObject thisNode = shapeNode->thisMObject();
    MPlug drawSkipPlug( thisNode, shapeNode->aDrawSkip );
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


    if (pvCache->particles)
    {
        glPushAttrib(GL_CURRENT_BIT);

        if (alphaFromVal >=0 || defaultAlphaVal < 1) //testing settings
        {

            // TESTING AROUND with dept/transparency  sorting issues..
            glDepthMask(true);
            //cout << "depthMask"<<  glGetError() << endl;
            glEnable(GL_DEPTH_TEST);
            //cout << "depth test" <<  glGetError() << endl;
            glEnable(GL_BLEND);
            //cout << "blend" <<  glGetError() << endl;
//				glBlendEquation(GL_FUNC_ADD);
            //cout << "blend eq" <<  glGetError() << endl;
            glEnable(GL_POINT_SMOOTH);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            //cout << "blend func" <<  glGetError() << endl;
            //glAlphaFunc(GL_GREATER, .01);
            //glEnable(GL_ALPHA_TEST);
        }

        if ( drawStyle == 0 )
        {
            glEnableClientState( GL_VERTEX_ARRAY );
            glEnableClientState( GL_COLOR_ARRAY );

            glPointSize(pointSizeVal);
            if (pvCache->particles->numParticles() > 0)
            {
                // now setup the position/color/alpha output pointers

				for (uint i = 0; i< pvCache->partitions.size(); i++)
				{
					const float * partioPositions = pvCache->particles->data<float>(pvCache->partitions[i],0);

					glVertexPointer( 3, GL_FLOAT, stride, partioPositions );


					if (defaultAlphaVal < 1 || alphaFromVal >=0)  // use transparency switch
					{
						glColorPointer(  4, GL_FLOAT, stride, pvCache->rgba );
					}
					else
					{
						glColorPointer(  3, GL_FLOAT, stride, pvCache->rgb );
					}
					glDrawArrays( GL_POINTS, 0, (pvCache->particles->numParticles()/(drawSkipVal+1)) );
				}
            }
            glDisableClientState( GL_VERTEX_ARRAY );
            glDisableClientState( GL_COLOR_ARRAY );

        }

        else
        {
            /// looping thru particles one by one...

            glPointSize(pointSizeVal);

            for (int i=0;i<pvCache->particles->numParticles();i+=(drawSkipVal+1))
            {
                if (defaultAlphaVal < 1 || alphaFromVal >=0)  // use transparency switch
                {
                    glColor4f(pvCache->rgb[i*3],pvCache->rgb[(i*3)+1],pvCache->rgb[(i*3)+2], pvCache->rgba[(i*4)+3] );
                }
                else
                {
                    glColor3f(pvCache->rgb[i*3],pvCache->rgb[(i*3)+1],pvCache->rgb[(i*3)+2]);
                }

				for (uint j = 0; j< pvCache->partitions.size(); j++)
				{
					const float * partioPositions = pvCache->particles->data<float>(pvCache->partitions[j],i);
					if (drawStyle == 1 || drawStyle == 2) // unfilled circles, disks, or spheres
					{
						MVector position(partioPositions[0], partioPositions[1], partioPositions[2]);
						float radius = pvCache->radius[i];
						drawBillboardCircleAtPoint(position,  radius, 10, drawStyle);
					}
					else // points
					{
						glVertex3f(partioPositions[0], partioPositions[1], partioPositions[2]);
					}
				}
            }
		}
        glDisable(GL_POINT_SMOOTH);
        glPopAttrib();
    } // if (particles)
}

partioVisualizerUI::partioVisualizerUI()
{
}
partioVisualizerUI::~partioVisualizerUI()
{
}

void* partioVisualizerUI::creator()
{
    return new partioVisualizerUI();
}


void partioVisualizerUI::getDrawRequests(const MDrawInfo & info,
        bool /*objectAndActiveOnly*/, MDrawRequestQueue & queue)
{

    MDrawData data;
    MDrawRequest request = info.getPrototype(*this);
    partioVisualizer* shapeNode = (partioVisualizer*) surfaceShape();
    partioVizReaderCache* geom = shapeNode->updateParticleCache();

    getDrawData(geom, data);
    request.setDrawData(data);

    // Are we displaying locators?
    if (!info.objectDisplayStatus(M3dView::kDisplayLocators))
        return;

    M3dView::DisplayStatus displayStatus = info.displayStatus();
    M3dView::ColorTable activeColorTable = M3dView::kActiveColors;
    M3dView::ColorTable dormantColorTable = M3dView::kDormantColors;
    switch (displayStatus)
    {
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

void partioVisualizerUI::drawBillboardCircleAtPoint(MVector position, float radius, int num_segments, int drawType) const
{
    float m[16];
    int j,k;
    glPushMatrix();
    glTranslatef( (GLfloat) position.x, (GLfloat) position.y, (GLfloat) position.z);
    glGetFloatv(GL_MODELVIEW_MATRIX, m);

    for (j = 0; j<3; j++)
    {
        for (k = 0; k<3; k++)
        {
            if (j==k)
            {
                m[j*4+k] = 1.0;
            }
            else
            {
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

    if (drawType == 1)
    {
        glBegin(GL_LINE_LOOP);
    }
    else if (drawType == 2)
    {
        glBegin(GL_POLYGON);
    }
    for (int ii = 0; ii < num_segments; ii++)
    {
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
    glTranslatef( (GLfloat)-position.x, (GLfloat)-position.y, (GLfloat)-position.z);
    glPopMatrix();
}

