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
#include "setup/IAppSetup.h"
#include <QApplication>
#include <QMouseEvent>
#include <QWheelEvent>
#include <vts-browser/log.hpp>
#include <vts-browser/map.hpp>
#include <vts-browser/mapOptions.hpp>
#include <vts-browser/camera.hpp>
#include <vts-browser/cameraCredits.hpp>
#include <vts-browser/navigation.hpp>
#include <vts-browser/navigationOptions.hpp>
#include <vts-browser/position.hpp>
#include <vts-browser/mapCallbacks.hpp>
#include <vts-renderer/renderer.hpp>
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

C3DMap::C3DMap()
{
    // make the window invisible until docked
    setFlag(Qt::BypassWindowManagerHint);

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
    mapopts.clientId = "vts-browser-qt";
    map = dataThread.map = std::make_shared<vts::Map>(mapopts);
    setupConfig();
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
    for (int i = mapConfig.size() - 1; i >= 0; --i) {
        filePath = "/3d/map_config/" + mapConfig.at(i) + ".json";
        QFile::copy(":" + filePath, cachePath + filePath);
    }
    filePath = "file://" + cachePath + filePath;
    map->setMapconfigPath(filePath.toStdString(), "");
}

void C3DMap::mouseMove(QMouseEvent *event)
{
    QPoint diff = event->globalPos() - lastMousePosition;
    lastMousePosition = event->globalPos();

    double n[3] = { (double)diff.x(), (double)diff.y(), 0 };

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
        view->render();

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

