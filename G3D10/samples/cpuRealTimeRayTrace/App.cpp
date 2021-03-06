/**
  @file App.cpp
  */
#include "App.h"
#include "World.h"


Radiance3 App::rayTrace(const Ray& ray, World* world, Random& rng, int bounce) {
    Radiance3 L_o = Radiance3::zero();
    const float BUMP_DISTANCE = 0.001f;

    const shared_ptr<Surfel>& surfel = world->intersect(ray);

    if (notNull(surfel)) {
        if (m_debugNormals) {
            return Radiance3(surfel->shadingNormal) * 0.5f + Radiance3(0.5f);
        }

        // Shade this point (direct illumination)
        for (int L = 0; L < world->lightArray.size(); ++L) {
            const shared_ptr<Light>& light = world->lightArray[L];

            if (light->producesDirectIllumination()) {
                // Shadow rays
                if ((! light->shadowsEnabled()) || world->lineOfSight(light->position().xyz(), surfel->position + surfel->geometricNormal * BUMP_DISTANCE)) {
                    Vector3 w_i = light->position().xyz() - surfel->position;
                    const float distance2 = w_i.squaredLength();
                    w_i /= sqrt(distance2);

                    const Biradiance3& B_i = light->biradiance(surfel->position);

                    L_o +=
                        surfel->finiteScatteringDensity(w_i, -ray.direction()) *
                        B_i *
                        max(0.0f, w_i.dot(surfel->shadingNormal));

                    debugAssertM(L_o.isFinite(), "Non-finite radiance in L_direct");
                }
            }
        }

        // Indirect illumination
        // Ambient
        L_o += surfel->reflectivity(rng) * world->ambient;

        // Specular
        if (bounce < m_maxBounces) {
            // Perfect reflection and refraction
            Surfel::ImpulseArray impulseArray;
            surfel->getImpulses(PathDirection::EYE_TO_SOURCE, -ray.direction(), impulseArray);

            for (int i = 0; i < impulseArray.size(); ++i) {
                const Surfel::Impulse& impulse = impulseArray[i];
                // Bump along normal *in the outgoing ray's hemisphere*.
                const Vector3& offset = surfel->geometricNormal * sign(impulse.direction.dot(surfel->geometricNormal)) * BUMP_DISTANCE;
                const Ray& secondaryRay = Ray::fromOriginAndDirection(surfel->position + offset, impulse.direction);
                debugAssert(secondaryRay.direction().isFinite());
                if (m_debugMirrorDirection && (impulse.direction.dot(surfel->geometricNormal) >= 0.0)) {
                    L_o += Radiance3(impulse.direction * 0.4f) + Radiance3(0.5f);
                } else {
                    L_o += rayTrace(secondaryRay, world, rng, bounce + 1) * impulse.magnitude;
                }
                debugAssert(L_o.isFinite());
            }
        }
    } else {
        // Hit the sky
        if (m_debugColoredSky) {
            L_o = Radiance3(ray.direction()) * 0.5f + Color3(0.5f);
        } else {
            L_o = world->skyColor(ray.direction());
        }
    }

    return L_o;
}

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    GApp::Settings settings;
    settings.window.caption = "G3D CPU Real-Time Ray Tracing";
    settings.window.width = 800;
    settings.window.height = 600;

    return App(settings).run();
}


App::App(const GApp::Settings& settings) :
    GApp(settings),
    m_maxBounces(3),
    m_raysPerPixel(1),
    m_world(nullptr) {

    catchCommonExceptions = false;
}


void App::onInit() {
    GApp::onInit();
    message("Loading...");
    renderDevice->setSwapBuffersAutomatically(true);

    m_world = new World();

    showRenderingStats = false;
    developerWindow->setVisible(true);
    developerWindow->cameraControlWindow->setVisible(true);
    m_debugCamera->filmSettings().setAntialiasingEnabled(true);
    m_debugCamera->filmSettings().setContrastToneCurve();

    // Starting position
    m_debugCamera->setFrame(CFrame::fromXYZYPRDegrees(24.3f, 0.4f, 2.5f, 68.7f, 1.2f, 0.0f));
    m_debugCamera->frame();

    GApp::loadScene(
        //"G3D Transparency Test");
        // "Refraction Test");
        // "G3D Sports Car");
        //"G3D Simple Cornell Box");
        //"G3D Cubemap"
        //"G3D Simple Cornell Box (Water)"
        //"Real Time Ray Trace"
        //"G3D Sponza (Statue)"
        "G3D Sibenik (Statue)"
    );

    makeGUI();

    // Force re-render on first frame
    m_prevCFrame = CFrame(Matrix3::zero());
    setActiveCamera(debugCamera());
    scene()->setTime(1.0);
    setSimulationTimeScale(0.0f);
}


void App::makeGUI() {
    shared_ptr<GuiWindow> window = GuiWindow::create("Controls", debugWindow->theme(), Rect2D::xywh(0, 0, 0, 0), GuiTheme::TOOL_WINDOW_STYLE);
    GuiPane* pane = window->pane();
    pane->addLabel("Use WASD keys + right mouse to move");

    pane->addButton("Render High Quality", [this]() { onRender(); })->setWidth(200);

    pane->addNumberBox("Rays per pixel", &m_raysPerPixel, "", GuiTheme::LINEAR_SLIDER, 1, 16, 1);
    pane->addNumberBox("Max bounces", &m_maxBounces, "", GuiTheme::LINEAR_SLIDER, 1, 16, 1);

    GuiPane* debugging = pane->addPane("Debug Controls");
    debugging->moveBy(0, 5);

    debugging->addLabel("(Useful with breakpoints)");
    debugging->addCheckBox("Show reticle", &m_showReticle);
    debugging->addCheckBox("Visualize normals", &m_debugNormals);    
    debugging->addCheckBox("Visualize mirror dir.", &m_debugMirrorDirection);    
    debugging->addCheckBox("Rainbow sky", &m_debugColoredSky);
    debugging->addButton("Cast Center Ray", [this](){
        trace(m_currentImage->width() / 2, m_currentImage->height() / 2, Random::threadCommon());
    })->setWidth(200);

    window->pack();

    window->setVisible(true);
    addWidget(window);
    developerWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(false);
}


void App::onGraphics(RenderDevice* rd, Array<shared_ptr<Surface> >& surface3D, Array<shared_ptr<Surface2D> >& surface2D) {
    // Show animations by forcing this always to true. Remove to reduce CPU load
    //m_forceRender = true;

    // Update the preview image only while moving
    if ((!m_prevCFrame.fuzzyEq(m_debugCamera->frame())) || m_forceRender) {
        rayTraceImage(0.33f, 1);
        m_prevCFrame = m_debugCamera->frame();
        m_forceRender = false;
    }

    if (m_result) {
        rd->push2D(); {
            Draw::rect2D(rd->viewport(), rd, Color3::white(), m_result);
        } rd->pop2D();
    }

    Surface2D::sortAndRender(rd, surface2D);
}


void App::onCleanup() {
    delete m_world;
    m_world = nullptr;
}


void App::message(const String& msg) const {
    renderDevice->clear();
    renderDevice->push2D();
    debugFont->draw2D(renderDevice, msg, renderDevice->viewport().center(), 12,
        Color3::white(), Color4::clear(), GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
    renderDevice->pop2D();

    // Force update so that we can see the message
    renderDevice->swapBuffers();
}


void App::onRender() {
    // Show message
    message("Rendering...");

    Stopwatch timer;
    rayTraceImage(1.0f, m_raysPerPixel);
    timer.printElapsedTime("Trace");
    debugPrintf("%f s\n", timer.elapsedTime());
    //    m_result->toImage3uint8()->save("result.png");
}


void App::trace(int x, int y, Random& rng) {
    Radiance3 sum = Radiance3::zero();

    if (m_currentRays == 1) {
        sum = rayTrace(m_debugCamera->worldRay(x + 0.5f, y + 0.5f, m_currentImage->rect2DBounds()), m_world, rng);
    } else {
        // Random jitter for antialiasing
        for (int i = 0; i < m_currentRays; ++i) {
            sum += rayTrace(m_debugCamera->worldRay(x + rng.uniform(), y + rng.uniform(), m_currentImage->rect2DBounds()), m_world, rng);
        }
    }
    m_currentImage->set(x, y, sum / float(m_currentRays));
}


void App::rayTraceImage(float scale, int numRays) {
    const int width = int(window()->width()  * scale);
    const int height = int(window()->height() * scale);

    if (isNull(m_currentImage) || (m_currentImage->width() != width) || (m_currentImage->height() != height)) {
        m_currentImage = Image3::createEmpty(width, height);
    }

    m_currentRays = numRays;
    runConcurrently(Point2int32(0, 0), Point2int32(width, height), [&](Point2int32 coord) {
        trace(coord.x, coord.y, Random::threadCommon());
    });

    if (m_showReticle) {
        // Draw a cross to identify the center pixel that is used for debug rays
        const int centerX = m_currentImage->width() / 2;
        const int centerY = m_currentImage->height() / 2;

        for (int d = -7; d <= +7; ++d) {
            if (abs(d) > 2) {
                m_currentImage->set(centerX + d, centerY - 1, Color3::white());
                m_currentImage->set(centerX + d, centerY, Color3::black());
                m_currentImage->set(centerX + d, centerY + 1, Color3::white());

                m_currentImage->set(centerX - 1, centerY + d, Color3::white());
                m_currentImage->set(centerX, centerY + d, Color3::black());
                m_currentImage->set(centerX + 1, centerY + d, Color3::white());
            }
        }
    }

    // Post-process
    const shared_ptr<PixelTransferBuffer>& ptb = CPUPixelTransferBuffer::fromData(m_currentImage->width(), m_currentImage->height(), ImageFormat::RGB32F(), m_currentImage->getCArray(), 1, 1);
    const shared_ptr<Texture>& src = Texture::fromPixelTransferBuffer("Source", ptb, ImageFormat::RGB32F(), Texture::DIM_2D, false);
    if (m_result) {
        m_result->resize(width, height);
    }

    m_film->exposeAndRender(renderDevice, m_debugCamera->filmSettings(), src,
        settings().hdrFramebuffer.trimBandThickness().x,
        settings().hdrFramebuffer.depthGuardBandThickness.x, m_result);
}


void App::onAfterLoadScene(const Any& any, const String& sceneName) {
    GApp::onAfterLoadScene(any, sceneName);

    m_world->clearScene();
    m_world->begin();

    Array<shared_ptr<Surface>> surfaceArray;
    scene()->onPose(surfaceArray);
    for (shared_ptr<Surface> surface : surfaceArray) {
        m_world->insert(surface);
    }

    scene()->getTypedEntityArray<Light>(m_world->lightArray);
    m_world->end();

    m_forceRender = true;
}
