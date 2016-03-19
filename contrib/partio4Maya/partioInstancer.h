/* partio4Maya  3/12/2012, John Cassella  http://luma-pictures.com and  http://redpawfx.com
PARTIO Instancer
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

#ifndef Partio4MayaInstancer_H
#define Partio4MayaInstancer_H

#define _USE_MGL_FT_

#include <math.h>
#include <stdlib.h>
#include <vector>
#include <set>

#include <maya/MBoundingBox.h>
#include <maya/MColor.h>
#include <maya/MDagPath.h>
#include <maya/MDrawData.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MArrayDataBuilder.h>
#include <maya/MDoubleArray.h>
#include <maya/MDistance.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatVector.h>
#include <maya/MGLFunctionTable.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MIOStream.h>
#include <maya/MMatrix.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPointArray.h>
#include <maya/MStringArray.h>
#include <maya/MString.h>
#include <maya/MStatus.h>
#include <maya/MSceneMessage.h>
#include <maya/MSelectionList.h>
#include <maya/MTypeId.h>
#include <maya/MTypes.h>
#include <maya/MTime.h>
#include <maya/MVectorArray.h>
#include <maya/MVector.h>
#include <maya/M3dView.h>

#include <maya/MFnUnitAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MFnNumericData.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnArrayAttrsData.h>
#include <maya/MFnNumericAttribute.h>

#include <maya/MPxSurfaceShape.h>
#include <maya/MPxNode.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MPxSurfaceShapeUI.h>

#include <Partio.h>
#include <PartioAttribute.h>
#include <PartioIterator.h>

#include "partio4MayaShared.h"
#include "iconArrays.h"

class partioInstReaderCache {
public:
    partioInstReaderCache();

    MBoundingBox bbox;
    int dList;
    PARTIO::ParticlesDataMutable* particles;
    PARTIO::ParticleAttribute positionAttr;
    PARTIO::ParticleAttribute idAttr;
    PARTIO::ParticleAttribute velocityAttr;
    PARTIO::ParticleAttribute angularVelocityAttr;
    PARTIO::ParticleAttribute rotationAttr;
    PARTIO::ParticleAttribute aimDirAttr;
    PARTIO::ParticleAttribute aimPosAttr;
    PARTIO::ParticleAttribute aimAxisAttr;
    PARTIO::ParticleAttribute aimUpAttr;
    PARTIO::ParticleAttribute aimWorldUpAttr;
    PARTIO::ParticleAttribute lastRotationAttr;
    PARTIO::ParticleAttribute scaleAttr;
    PARTIO::ParticleAttribute lastScaleAttr;
    PARTIO::ParticleAttribute lastAimDirAttr;
    PARTIO::ParticleAttribute lastAimPosAttr;
    PARTIO::ParticleAttribute lastPosAttr;
    PARTIO::ParticleAttribute indexAttr;
//  PARTIO::ParticleAttribute shaderIndexAttr;
    MFnArrayAttrsData instanceData;
    MObject instanceDataObj;
};


class partioInstancerUI : public MPxSurfaceShapeUI {
public:

    partioInstancerUI();

    virtual ~partioInstancerUI();

    virtual void draw(const MDrawRequest& request, M3dView& view) const;

    virtual void getDrawRequests(const MDrawInfo& info, bool objectAndActiveOnly, MDrawRequestQueue& requests);

    void drawBoundingBox() const;

    void drawPartio(partioInstReaderCache* pvCache, int drawStyle, M3dView& view) const;

    static void* creator();

    virtual bool select(MSelectInfo& selectInfo,
                        MSelectionList& selectionList,
                        MPointArray& worldSpaceSelectPts) const;

};

class partioInstancer : public MPxSurfaceShape {
public:
    partioInstancer();

    virtual ~partioInstancer();

    virtual MStatus compute(const MPlug& plug, MDataBlock& block);

    virtual bool isBounded() const;

    virtual MBoundingBox boundingBox() const;

    static void* creator();

    static MStatus initialize();

    static void reInit(void* data);

    void initCallback();

    virtual void postConstructor();

    bool GetPlugData();

    partioInstReaderCache* updateParticleCache();

    MCallbackId partioInstancerOpenCallback;
    MCallbackId partioInstancerImportCallback;
    MCallbackId partioInstancerReferenceCallback;
/// ATTRS
    static MObject time;
    static MObject aByFrame;
    static MObject aSize;         // The size of the logo
    static MObject aFlipYZ;
    static MObject aDrawStyle;
    static MObject aPointSize;

/// Cache file related stuff
    static MObject aUpdateCache;
    static MObject aCacheDir;
    static MObject aCacheFile;
    static MObject aCacheActive;
    static MObject aCacheOffset;
    static MObject aCacheStatic;
    static MObject aCacheFormat;
    static MObject aForceReload;
    static MObject aRenderCachePath;

/// point position / velocity
    static MObject aComputeVeloPos;
    static MObject aVelocityMult;

/// attributes
    static MObject aPartioAttributes;
    static MObject aScaleFrom;
    static MObject aRotationType;

    static MObject aRotationFrom;
    static MObject aAimDirectionFrom;
    static MObject aAimPositionFrom;
    static MObject aAimAxisFrom;
    static MObject aAimUpAxisFrom;
    static MObject aAimWorldUpFrom;

    static MObject aLastScaleFrom;
    static MObject aLastRotationFrom;
    static MObject aLastAimDirectionFrom;
    static MObject aLastAimPositionFrom;
    static MObject aLastPositionFrom;
    static MObject aVelocityFrom;
    static MObject aAngularVelocityFrom;
    static MObject aAngularVelocityMult;

    static MObject aIndexFrom;

/// not implemented yet
//  static MObject  aJitterPos;
//  static MObject  aJitterFreq;
//  static MObject  aShaderIndexFrom;
//  static MObject  aInMeshInstances;
//  static MObject  aOutMesh;

//  output data to instancer
    static MObject aInstanceData;
    static MObject aExportAttributes;
    static MObject aVelocitySource;
    static MObject aAngularVelocitySource;

    static MTypeId id;
    partioInstReaderCache pvCache;
private:
    MStringArray m_attributeList;
    MString m_lastFileLoaded;
    MString m_lastPath;
    MString m_lastFile;
    MString m_lastExt;
    MString m_lastExportAttributes;
    MString m_lastRotationType;
    MString m_lastRotationFrom;
    MString m_lastLastRotationFrom;
    MString m_lastAimDirectionFrom;
    MString m_lastLastAimDirectionFrom;
    MString m_lastAimPositionFrom;
    MString m_lastLastAimPositionFrom;
    MString m_lastAimAxisFrom;
    MString m_lastAimUpAxisFrom;
    MString m_lastAimWorldUpFrom;
    MString m_lastScaleFrom;
    MString m_lastLastScaleFrom;
    MString m_lastIndexFrom;
    MString m_lastAngularVelocityFrom;
    float m_lastAngularVelocityMult;
    short m_lastAngularVelocitySource;
    bool m_lastFlipStatus;
    bool m_flipped;
    bool m_frameChanged;
    bool m_canMotionBlur;

public:
    float m_multiplier;
    int m_drawError;
    bool m_cacheChanged;

protected:
    int m_dUpdate;
};

#endif

