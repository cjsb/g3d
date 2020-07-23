/**
  \file G3D-app.lib/source/Surface.cpp

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#include "G3D-base/Sphere.h"
#include "G3D-base/Box.h"
#include "G3D-app/Camera.h"
#include "G3D-base/debugPrintf.h"
#include "G3D-base/Any.h"
#include "G3D-base/Log.h"
#include "G3D-base/AABox.h"
#include "G3D-base/Sphere.h"
#include "G3D-base/typeutils.h"
#include "G3D-app/Surface.h"
#include "G3D-gfx/RenderDevice.h"
#include "G3D-app/UniversalSurface.h"
#include "G3D-gfx/GLCaps.h"
#include "G3D-app/ShadowMap.h"
#include "G3D-app/Light.h"
#include "G3D-app/Surface.h"
#include "G3D-gfx/Shader.h"
#include "G3D-app/LightingEnvironment.h"
#include "G3D-app/SVO.h"
#include "G3D-app/SkyboxSurface.h"

namespace G3D {

Surface::ExpressiveLightScatteringProperties::ExpressiveLightScatteringProperties(const Any& any) {
    *this = ExpressiveLightScatteringProperties();
    AnyTableReader r("ExpressiveLightScatteringProperties", any);
    r.getIfPresent("castsShadows", castsShadows);
    r.getIfPresent("receivesShadows", receivesShadows);
    r.getIfPresent("behaviorForPathsFromSource", behaviorForPathsFromSource);
    r.getIfPresent("visibleForPathsFromEye", visibleForPathsFromEye);

    r.verifyDone();
}


Any Surface::ExpressiveLightScatteringProperties::toAny() const {
    Any a(Any::TABLE, "ExpressiveLightScatteringProperties");
    a["castsShadows"] = castsShadows;
    a["receivesShadows"] = receivesShadows;
    a["behaviorForPathsFromSource"] = behaviorForPathsFromSource;
    a["visibleForPathsFromEye"] = visibleForPathsFromEye;
    return a;
}


bool ignoreBool;

void Surface::setStorage(const Array<shared_ptr<Surface>>& surfaceArray, ImageStorage newStorage) {
    for (int i = 0; i < surfaceArray.size(); ++i) {
        surfaceArray[i]->setStorage(newStorage);
    }
}


RealTime Surface::lastChangeTime() const {
    if (notNull(m_entity)) {
        return m_entity->lastChangeTime();
    } else {
        return System::time();
    }
}

String Surface::name() const {
    const shared_ptr<Entity>& e = entity();
    if (notNull(e)) {
        return e->name();
    } else {
        return "Surface";
    }
}

RenderMask Surface::renderMask() const {
	const shared_ptr<Entity>& e = entity();
	if (notNull(e)) {
		return e->canChange() ? RenderMask::DYNAMIC_GEOMETRY : RenderMask::STATIC_GEOMETRY;
	}
	else {
		return 0;
	}
}


void Surface::getCoordinateFrame(CoordinateFrame& cframe, bool previous) const {
    const shared_ptr<Entity>& e = entity();
    if (notNull(e)) {
        if (previous) {
            cframe = e->previousFrame();
        } else {
            cframe = e->frame();
        }
    } else {
        cframe = CFrame();
    }
}


void Surface::renderIntoGBuffer
   (RenderDevice*                   rd,
    const Array<shared_ptr<Surface> >& surfaceArray,
    const shared_ptr<GBuffer>&      gbuffer,
    const shared_ptr<Texture>&      depthPeelTexture,
    const float                     minZSeparation,
    const LightingEnvironment&      lightingEnvironment) {

    BEGIN_PROFILER_EVENT("Surface::renderIntoGBuffer");
    // Separate by type.  This preserves the sort order and ensures that the closest
    // object will still render first.
    Array< Array<shared_ptr<Surface> > > derivedTable;
    categorizeByDerivedType(surfaceArray, derivedTable);

    rd->pushState(gbuffer->framebuffer()); {
        rd->setProjectionAndCameraMatrix(gbuffer->camera()->projection(), gbuffer->camera()->frame());

        if (rd->depthWrite()) {
            // Render to the full image and let the shader cull color within the non-depth region
            rd->setClip2D(gbuffer->rect());
        } else {
            // Render within the color guard band only, since we're not writing depth anyway.
            rd->setClip2D(gbuffer->colorRect());
        }
        
        // Instead of sorting all batches by depth, simply force the skybox to the end

        const Array<shared_ptr<Surface>>* skyboxArray = nullptr;

        for (int t = 0; t < derivedTable.size(); ++t) {
            const Array<shared_ptr<Surface>>& derivedArray = derivedTable[t];
            debugAssertM(derivedArray.size() > 0, "categorizeByDerivedType produced an empty subarray");
            if ((derivedArray.size() > 0) && derivedArray[0]->isSkybox() && isNull(skyboxArray)) {
                skyboxArray = &derivedArray;
            } else if (gbuffer->isDepthAndStencilOnly()) {
                derivedArray[0]->renderDepthOnlyHomogeneous(rd, derivedArray, depthPeelTexture, minZSeparation, TransparencyTestMode::REJECT_TRANSPARENCY, Color3::white() / 3.0f);
            } else {
                derivedArray[0]->renderIntoGBufferHomogeneous(rd, derivedArray, gbuffer, depthPeelTexture, minZSeparation, lightingEnvironment);
            }
        }

        if (notNull(skyboxArray)) {
            if (gbuffer->isDepthAndStencilOnly()) {
                (*skyboxArray)[0]->renderDepthOnlyHomogeneous(rd, *skyboxArray, depthPeelTexture, minZSeparation, TransparencyTestMode::REJECT_TRANSPARENCY, Color3::white() / 3.0f);
            } else {
                (*skyboxArray)[0]->renderIntoGBufferHomogeneous(rd, *skyboxArray, gbuffer, depthPeelTexture, minZSeparation, lightingEnvironment);
            }
        }
        // Force the skybox to render last

    } rd->popState();
    END_PROFILER_EVENT();
}


void Surface::renderIntoSVO
   (RenderDevice*                   rd,
    Array< shared_ptr<Surface> >&   surfaceArray,
    const shared_ptr<SVO>&          svo,
    const CFrame&                   previousCameraFrame) {

    // Separate by type. 
    Array< Array<shared_ptr<Surface> > > derivedTable;
    categorizeByDerivedType(surfaceArray, derivedTable);

    rd->pushState(svo->framebuffer()); {
        rd->setProjectionAndCameraMatrix(svo->camera()->projection(), svo->camera()->frame());
        for (int t = 0; t < derivedTable.size(); ++t) {
            Array<shared_ptr<Surface> >& derivedArray = derivedTable[t];
            debugAssertM(derivedArray.size() > 0, "categorizeByDerivedType produced an empty subarray");
            derivedArray[0]->renderIntoSVOHomogeneous(rd, derivedArray, svo, previousCameraFrame);
        }
    } rd->popState();
}


void Surface::getBoxBounds(const Array<shared_ptr<Surface> >& models, AABox& bounds, bool previous, bool& anyInfinite, bool onlyShadowCasters) {
    bounds = AABox::empty();

    for (int i = 0; i < models.size(); ++i) {
        const shared_ptr<Surface>& surface = models[i];

        if (! onlyShadowCasters || surface->expressiveLightScatteringProperties.castsShadows) {
            AABox temp;
            CFrame cframe;
            surface->getCoordinateFrame(cframe, previous);
            surface->getObjectSpaceBoundingBox(temp, previous);
            debugAssert(! cframe.translation.isNaN());

            // Ignore infinite bounding boxes
            if (temp.isFinite()) {
                cframe.toWorldSpace(temp, temp);
                if (temp.isFinite()) {
                    bounds.merge(temp);
                } else {
                    anyInfinite = true;
                }
            } else {
                anyInfinite = true;
            }
        }
    }
}


void Surface::renderDepthOnlyHomogeneous
(RenderDevice*                      rd, 
 const Array<shared_ptr<Surface> >& surfaceArray,
 const shared_ptr<Texture>&         previousDepthBuffer,
 const float                        minZSeparation,
 TransparencyTestMode               transparencyTestMode,
 const Color3&                      transmissionWeight) const {
    rd->setColorWrite(false);
    renderHomogeneous(rd, surfaceArray, LightingEnvironment(), RenderPassType::OPAQUE_SAMPLES);
}


void Surface::renderWireframe(RenderDevice* rd, const Array<shared_ptr<Surface> >& surfaceArray, const Color4& color, bool previous) {
    BEGIN_PROFILER_EVENT("Surface::renderWireframe");
    // Separate by type.  This preserves the sort order and ensures that the closest
    // object will still render first.
    Array< Array<shared_ptr<Surface> > > derivedTable;
    categorizeByDerivedType(surfaceArray, derivedTable);

    for (int t = 0; t < derivedTable.size(); ++t) {
        Array<shared_ptr<Surface> >& derivedArray = derivedTable[t];
        debugAssertM(derivedArray.size() > 0, "categorizeByDerivedType produced an empty subarray");
        derivedArray[0]->renderWireframeHomogeneous(rd, derivedArray, color, previous);
    }
    END_PROFILER_EVENT();
}


void Surface::getSphereBounds(const Array<shared_ptr<Surface> >& models, Sphere& bounds, bool previous, bool& anyInfnite, bool onlyShadowCasters) {
    AABox temp;
    getBoxBounds(models, temp, previous, anyInfnite, onlyShadowCasters);
    temp.getBounds(bounds);
}


void Surface::cull
(const CFrame&              cameraFrame,
 const Projection&          cameraProjection,
 const Rect2D&              viewport, 
 Array<shared_ptr<Surface> >& allSurfaces,
 Array<shared_ptr<Surface> >& outSurfaces,
 bool                       previous,
 bool                       inPlace) {
    if (inPlace) {
        debugAssert(&allSurfaces != &outSurfaces);
        outSurfaces.fastClear();
    }
    Frustum fr;
    cameraProjection.frustum(viewport, fr);
    fr = cameraFrame.toWorldSpace(fr);
    
    static Array<Plane> clipPlanes;
    cameraProjection.getClipPlanes(viewport, clipPlanes);
    for (int i = 0; i < clipPlanes.size(); ++i) {
        clipPlanes[i] = cameraFrame.toWorldSpace(clipPlanes[i]);
    }

    for (int i = 0; i < allSurfaces.size(); ++i) {
        Sphere sphere;
        CFrame c;

        // We actually remove this shared_ptr from the array below,
        // at which point this reference will become invalid. However,
        // the code doesn't touch the reference again afterwards, so 
        // that dangling pointer is ok.
        const shared_ptr<Surface>& m = allSurfaces[i];
        m->getCoordinateFrame(c, previous);
        m->getObjectSpaceBoundingSphere(sphere, previous);
        sphere = c.toWorldSpace(sphere);
        
        // Not const because it is reevaluated in the branch immediately below
        bool culled = sphere.culledBy(clipPlanes);
        if (!culled) {
            AABox osBox;
            m->getObjectSpaceBoundingBox(osBox, previous);
            if (osBox.isEmpty()) {
                culled = true;
            } else {
                const Box& wsBox = c.toWorldSpace(osBox);
                culled = wsBox.culledBy(fr);
            }
        }

        if (! culled) {
            if (inPlace) {
                allSurfaces.fastRemove(i);
                --i;
            } else {
                outSurfaces.append(allSurfaces[i]);
            }
        }
    }
}


bool Surface::canChange() const {
    const shared_ptr<Entity>& e = entity();
    return isNull(e) || e->canChange();
}


void Surface::renderDepthOnly
(RenderDevice*                      rd, 
 const Array<shared_ptr<Surface> >& surfaceArray,
 CullFace                           cull,
 const shared_ptr<Texture>&         previousDepthBuffer,
 const float                        minZSeparation,
 TransparencyTestMode               transparencyTestMode,
 const Color3&                      transmissionWeight) {

    BEGIN_PROFILER_EVENT("Surface::renderDepthOnly");

    rd->pushState(); {

        rd->setCullFace(cull);
        rd->setDepthWrite(true);
        rd->setColorWrite(false);

        // Categorize by subclass (derived type)
        Array< Array<shared_ptr<Surface> > > derivedTable;
        categorizeByDerivedType(surfaceArray, derivedTable);

        for (int t = 0; t < derivedTable.size(); ++t) { 
            Array<shared_ptr<Surface> >& derivedArray = derivedTable[t];
            debugAssertM(derivedArray.size() > 0, "categorizeByDerivedType produced an empty subarray");
            // debugPrintf("Invoking on type %s\n", typeid(*derivedArray[0]).raw_name());
            derivedArray[0]->renderDepthOnlyHomogeneous(rd, derivedArray, previousDepthBuffer, minZSeparation, transparencyTestMode, transmissionWeight);
        }

    } rd->popState();

    END_PROFILER_EVENT();
}


void Surface2D::sortAndRender(RenderDevice* rd, Array<shared_ptr<Surface2D> >& posed2D) {
    if (posed2D.size() > 0) {
        BEGIN_PROFILER_EVENT("Surface2D::sortAndRender");
        rd->push2D();
            Surface2D::sort(posed2D);
            for (int i = 0; i < posed2D.size(); ++i) {
                posed2D[i]->render(rd);
            }
        rd->pop2D();
        END_PROFILER_EVENT();
    }
}


class ModelSorter {
public:
    float                  sortKey;
    shared_ptr<Surface>    model;

    ModelSorter() {}

    ModelSorter(const shared_ptr<Surface>& m, const Vector3& axis) : model(m) {
        Sphere s;
        CFrame c;
        m->getCoordinateFrame(c, false);
        m->getObjectSpaceBoundingSphere(s, false);
        sortKey = axis.dot(c.pointToWorldSpace(s.center));
    }

    inline bool operator>(const ModelSorter& s) const {
        return sortKey > s.sortKey;
    }

    inline bool operator<(const ModelSorter& s) const {
        return sortKey < s.sortKey;
    }
};


void Surface::sortFrontToBack
(Array<shared_ptr<Surface> >& surface, 
 const Vector3&       wsLook) {

    static Array<ModelSorter> sorter;
    
    for (int m = 0; m < surface.size(); ++m) {
        sorter.append(ModelSorter(surface[m], wsLook));
    }

    sorter.sort(SORT_INCREASING);

    for (int m = 0; m < sorter.size(); ++m) {
        surface[m] = sorter[m].model;
    }

    sorter.fastClear();
}


void Surface::renderHomogeneous
    (RenderDevice*                        rd, 
     const Array<shared_ptr<Surface> >&   surfaceArray, 
     const LightingEnvironment&           lightingEnvironment,
     RenderPassType                       passType) const {

    if ((passType == RenderPassType::OPAQUE_SAMPLES) || (passType == RenderPassType::UNBLENDED_SCREEN_SPACE_REFRACTION_SAMPLES)) {
        // Render front-to-back
        for (int i = surfaceArray.size() - 1; i >= 0; --i) {
            surfaceArray[i]->render(rd, lightingEnvironment, passType);
        }
    } else {
        // Render back-to-front
        for (int i = 0; i < surfaceArray.size(); ++i) {
            surfaceArray[i]->render(rd, lightingEnvironment, passType);
        }
    }
}
   

static bool depthGreaterThan(const shared_ptr<Surface2D>& a, const shared_ptr<Surface2D>& b) {
    return a->depth() > b->depth();
}


void Surface2D::sort(Array<shared_ptr<Surface2D> >& array) {
    array.sort(depthGreaterThan);
}


void Surface::getTris(const Array<shared_ptr<Surface> >& surfaceArray, CPUVertexArray& cpuVertexArray, Array<Tri>& triArray, bool computePrevPosition){

    Array< Array<shared_ptr<Surface> > > derivedTable;
    categorizeByDerivedType(surfaceArray, derivedTable);
    for (int t = 0; t < derivedTable.size(); ++t) {
        Array<shared_ptr<Surface> >& derivedArray = derivedTable[t];
        derivedArray[0]->getTrisHomogeneous(derivedArray, cpuVertexArray, triArray, computePrevPosition);
    }

}


}
