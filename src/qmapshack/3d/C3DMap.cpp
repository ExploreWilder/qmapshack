/**********************************************************************************************
    Copyright (C) 2021 Clement <explorewilder.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

**********************************************************************************************/

#include "3d/C3DMap.h"
#include "CAuth.h"
#include "setup/IAppSetup.h"
#include <QApplication>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QJsonDocument>
#include <vts-browser/buffer.hpp>
#include <vts-browser/log.hpp>
#include <vts-browser/math.hpp>
#include <vts-browser/map.hpp>
#include <vts-browser/mapOptions.hpp>
#include <vts-browser/camera.hpp>
#include <vts-browser/cameraCredits.hpp>
#include <vts-browser/cameraDraws.hpp>
#include <vts-browser/navigation.hpp>
#include <vts-browser/navigationOptions.hpp>
#include <vts-browser/position.hpp>
#include <vts-browser/mapCallbacks.hpp>
#include <vts-renderer/renderer.hpp>
#include <cmath>
#include <stdexcept>
#include <iostream>

static Gl *openglFunctionPointerInstance;

void *openglFunctionPointerProc(const char *name)
{
    return (void*)openglFunctionPointerInstance->getProcAddress(name);
}

Gl::Gl(QSurface *surface) : surface(surface)
{}

void Gl::current(bool bind)
{
    if (bind)
        makeCurrent(surface);
    else
        doneCurrent();
}

void Gl::initialize()
{
    setFormat(surface->format());
    create();
    if (!isValid())
        throw std::runtime_error("unable to create opengl context");
}

void DataThread::run()
{
    vts::setLogThreadName("data");
    gl->current();
    vts::renderer::installGlDebugCallback();
    map->dataAllRun();
}

C3DMap::C3DMap(const CAuth &auth)
{
    // make the window invisible until docked
    setFlag(Qt::BypassWindowManagerHint);

    bingUrl = auth.getBingUrl();
    uuid = auth.getUuid();

    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
#ifndef NDEBUG
    format.setOption(QSurfaceFormat::DebugContext);
#endif
    format.setDepthBufferSize(0);
    format.setStencilBufferSize(0);
    setFormat(format);
    setSurfaceType(QWindow::OpenGLSurface);

    show();

    dataThread.gl = std::make_shared<Gl>(this);
    dataThread.gl->initialize();
    dataThread.gl->current(false);

    gl = std::make_shared<Gl>(this);
    gl->setShareContext(dataThread.gl.get());
    gl->initialize();
    gl->current();

    openglFunctionPointerInstance = gl.get();
    vts::renderer::loadGlFunctions(&openglFunctionPointerProc);

    vts::MapCreateOptions mapopts;

    // application ID that is sent with all resources:
    mapopts.clientId = "qmapshack-explorewilder";

    // path to all downloaded files:
    QString cachePath = IAppSetup::getPlatformInstance()->defaultCachePath();
    QString subPath = "3d/vts_cache";
    QDir mapConfigDir(cachePath);
    mapConfigDir.mkpath(subPath);
    cachePath += "/" + subPath;
    mapopts.cachePath = cachePath.toStdString();

    // font to be used: (broken link if defined in the mapConfig file)
    mapopts.geodataFontFallback = "https://cdn.melown.com/libs/vtsjs/fonts/noto-basic/1.0.0/noto.fnt";

    map = dataThread.map = std::make_shared<vts::Map>(mapopts);
    loadResources();
#ifdef NDEBUG
    try
    {
#endif

        setupConfig();

#ifdef NDEBUG
    }
    catch(...)
    {
        this->close();
        return;
    }
#endif
    map->callbacks().mapconfigReady = [&]() -> void
    {
        emit sigMapReady();
    };
    camera = map->createCamera();
    navigation = camera->createNavigation();

    // same config as the 2D canvas:
    navigation->options().inertiaPan = 0;
    navigation->options().inertiaZoom = 0;

    context = std::make_shared<vts::renderer::RenderContext>();
    context->bindLoadFunctions(map.get());
    view = context->createView(camera.get());

    dataThread.gl->moveToThread(&dataThread);
    dataThread.start();

    lastTime = std::chrono::high_resolution_clock::now();
    requestUpdate();
}

C3DMap::~C3DMap()
{
    view.reset();
    navigation.reset();
    camera.reset();
    if (map)
        map->renderFinalize();
    dataThread.wait();
}

void C3DMap::loadResources()
{
    // load mesh sphere
    {
        meshSphere = std::make_shared<vts::renderer::Mesh>();
        vts::GpuMeshSpec spec(vts::readInternalMemoryBuffer("data/meshes/sphere.obj"));
        assert(spec.faceMode == vts::GpuMeshSpec::FaceMode::Triangles);
        spec.attributes[0].enable = true;
        spec.attributes[0].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[0].components = 3;
        spec.attributes[1].enable = true;
        spec.attributes[1].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[1].components = 2;
        spec.attributes[1].offset = sizeof(vts::vec3f);
        vts::ResourceInfo info;
        meshSphere->load(info, spec, "data/meshes/sphere.obj");
    }

    // load mesh line
    {
        meshLine = std::make_shared<vts::renderer::Mesh>();
        vts::GpuMeshSpec spec(vts::readInternalMemoryBuffer("data/meshes/line.obj"));
        assert(spec.faceMode == vts::GpuMeshSpec::FaceMode::Lines);
        spec.attributes[0].enable = true;
        spec.attributes[0].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[0].components = 3;
        spec.attributes[1].enable = true;
        spec.attributes[1].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[1].components = 2;
        spec.attributes[1].offset = sizeof(vts::vec3f);
        vts::ResourceInfo info;
        meshLine->load(info, spec, "data/meshes/line.obj");
    }
}

QString C3DMap::credits(CreditsType creditsType)
{
    QString credits;
    bool first = true;
    const vts::CameraCredits::Scope *scope = (creditsType == IMAGERY) ? &camera->credits().imagery : &camera->credits().geodata;
    for (const auto &it : scope->credits)
    {
        if (first)
        {
            first = false;
        }
        else
        {
            credits += ", ";
        }
        credits += QString::fromUtf8(it.notice.c_str());
    }
    return credits;
}

void C3DMap::setupConfig()
{
    QString cachePath = IAppSetup::getPlatformInstance()->defaultCachePath();
    QDir mapConfigDir(cachePath);
    QString filePath;
    QVector<QString> mapConfig;

    mapConfig << "main_config"
              << "osm-maptiler"
              << "osm-maptiler.style"
              << "peaklist.geo"
              << "peaklist"
              << "peaklist.style"
              << "webtrack.style";

    mapConfigDir.mkpath("3d/map_config");

    // copy static config
    for (int i = 1; i < mapConfig.size(); ++i) {
        filePath = "/3d/map_config/" + mapConfig.at(i) + ".json";
        QFile::copy(":" + filePath, cachePath + filePath);
    }

    // open and load the main mapConfig
    filePath = "/3d/map_config/" + mapConfig.first() + ".json";
    QFile mapConfigFile(":" + filePath);
    if (!mapConfigFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        throw std::runtime_error("Failed to open mapConfig from resources");
    }

    QJsonDocument mapConfigJsonDoc = QJsonDocument::fromJson(mapConfigFile.readAll());
    if (!mapConfigJsonDoc.isObject())
    {
        throw std::runtime_error("Failed to load mapConfig");
    }

    // update the dynamic Bing URL
    QJsonObject root = mapConfigJsonDoc.object();
    QJsonObject res1 = root["boundLayers"].toObject();
    QJsonObject res2 = res1["world-satellite-bing"].toObject();
    res2["url"] = bingUrl;
    res1["world-satellite-bing"] = res2;
    root["boundLayers"] = res1;
    mapConfigJsonDoc.setObject(root);

    // save to a mapConfig that will be used for the session
    QFile dynamicMapConfigFile(cachePath + filePath);
    if (!dynamicMapConfigFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        throw std::runtime_error("Failed to open the temporary mapConfig");
    }
    if (dynamicMapConfigFile.write(mapConfigJsonDoc.toJson(QJsonDocument::Compact)) == -1)
    {
        throw std::runtime_error("Failed to write the temporary mapConfig");
    }
    
    filePath = "file://" + cachePath + filePath;
    map->setMapconfigPath(filePath.toStdString(), "");
}

void C3DMap::mouseMove(QMouseEvent *event)
{
    QPoint diff = event->globalPos() - lastMousePosition;
    lastMousePosition = event->globalPos();

    double n[3] = { (double)diff.x(), (double)diff.y(), 0 };

    if (map->getMapconfigReady())
    {
        double posPhy[3], posNav[3];
        const QPointF eventPx = event->localPos();
        const double posPx[2] = {eventPx.x(), eventPx.y()};
        view->getWorldPosition(posPx, posPhy);
        map->convert(posPhy, posNav, vts::Srs::Physical, vts::Srs::Navigation);
        if (!std::isnan(posNav[2]))
        {
            emit sigMouseMove(QPointF{posNav[0], posNav[1]}, (qreal) posNav[2]);
        }
    }

    if (event->buttons() & Qt::LeftButton)
    {
        navigation->pan(n);
        vts::Position currentPosition(navigation->getPosition());
        emit sigMoveMap(QPointF{currentPosition.point[0], currentPosition.point[1]});
    }
    if ((event->buttons() & Qt::RightButton)
        || (event->buttons() & Qt::MiddleButton))
    {
        navigation->rotate(n);
    }
}

void C3DMap::mousePress(QMouseEvent *)
{}

void C3DMap::mouseRelease(QMouseEvent *)
{}

void C3DMap::mouseWheel(QWheelEvent *event)
{
    // angleDelta() returns the eighths of a degree
    // of the mousewheel
    // -> zoom in/out every 15 degrees = every 120 eights
    navigation->zoom(event->angleDelta().y() / 120.0);

    emit sigZoomMap(event);
}

void C3DMap::mouseDoubleClick(QMouseEvent *event)
{
    vts::renderer::RenderOptions &ro = view->options();
    const double padding = std::min(ro.width, ro.height) * .15 * (1. + .5 / 2.);
    const QPointF eventPx = event->localPos();

    if (eventPx.x() < padding && (eventPx.y() + padding) > ro.height)
    {
        double rot[3];
        navigation->getRotation(rot);
        rot[0] = 0.; // north up
        navigation->setRotation(rot);
    }
}

bool C3DMap::event(QEvent *event)
{
    switch (event->type())
    {
    case QEvent::UpdateRequest:
        tick();
        return true;
    case QEvent::MouseMove:
        mouseMove(dynamic_cast<QMouseEvent*>(event));
        return true;
    case QEvent::MouseButtonDblClick:
        mouseDoubleClick(dynamic_cast<QMouseEvent*>(event));
        return true;
    case QEvent::MouseButtonPress:
        mousePress(dynamic_cast<QMouseEvent*>(event));
        return true;
    case QEvent::MouseButtonRelease:
        mouseRelease(dynamic_cast<QMouseEvent*>(event));
        return true;
    case QEvent::Wheel:
        mouseWheel(dynamic_cast<QWheelEvent*>(event));
        return true;
    default:
        return QWindow::event(event);
    }
}

void C3DMap::tick()
{
    auto currentTime = std::chrono::high_resolution_clock::now();
    double elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(
                currentTime - lastTime).count() * 1e-6;
    lastTime = currentTime;

    requestUpdate();
    if (!isExposed())
        return;

    if (!gl->isValid())
        throw std::runtime_error("invalid gl context");

    gl->makeCurrent(this);
    QSize size = QWindow::size();

    // release build -> catch exceptions and close the window
    // debug build -> let the debugger handle the exceptions
#ifdef NDEBUG
    try
    {
#endif

        map->renderUpdate(elapsedTime);
        camera->setViewportSize(size.width(), size.height());
        camera->renderUpdate();
        view->options().targetFrameBuffer = gl->defaultFramebufferObject();
        view->options().width = size.width();
        view->options().height = size.height();
        renderCursorMark();
        view->render();
        renderCompass();

#ifdef NDEBUG
    }
    catch(...)
    {
        this->close();
        return;
    }
#endif

    // finish the frame
    gl->swapBuffers(this);
}

void C3DMap::renderCompass()
{
    vts::renderer::RenderOptions &ro = view->options();
    const double size = std::min(ro.width, ro.height) * .15;
    const double offset = size * .5;
    const double posSize[3] = { offset, offset, size };
    double rot[3];
    navigation->getRotation(rot);
    view->renderCompass(posSize, rot);
}

void C3DMap::slotMoveMap(const QPointF& pos)
{
    if (!map->getMapconfigReady())
    {
        return; // do nothing if the map is not ready
    }
    vts::Position currentPosition(navigation->getPosition());
    currentPosition.point[0] = pos.x();
    currentPosition.point[1] = pos.y();
    navigation->setPosition(currentPosition);
}

void C3DMap::slotZoomMap(const QPointF& pos, qreal h)
{
    if (!map->getMapconfigReady())
    {
        return;
    }
    slotMoveMap(pos);
    navigation->setViewExtent(h);
}

void C3DMap::slotCursorVisibility(const bool visible)
{
    cursorMark.visible = visible;
}

void C3DMap::slotMouseMove(const QPointF& pos, qreal ele)
{
    if (!map->getMapconfigReady())
    {
        return;
    }
    double posNav[3] = {pos.x(), pos.y(), ele}, posPhy[3];
    map->convert(posNav, posPhy, vts::Srs::Navigation, vts::Srs::Physical);
    cursorMark.coord = vts::vec3(posPhy[0], posPhy[1], posPhy[2]);
    cursorMark.color = vts::vec3f(1., 1., 0.);
}

void C3DMap::renderCursorMark()
{
    if (std::isnan(cursorMark.coord(0)) || !cursorMark.visible)
    {
        return;
    }

    vts::mat4 view = vts::rawToMat4(camera->draws().camera.view);
    vts::mat4 mv = view * vts::translationMatrix(cursorMark.coord) * vts::scaleMatrix(navigation->getViewExtent() * 0.005);
    vts::mat4f mvf = mv.cast<float>();
    vts::DrawInfographicsTask t;
    vts::vec4f c = vts::vec3to4(cursorMark.color, 1);
    for (int i = 0; i < 4; i++)
    {
        t.color[i] = c(i);
    }
    t.mesh = meshSphere;
    vts::matToRaw(mvf, t.mv);
    camera->draws().infographics.push_back(t);
}

