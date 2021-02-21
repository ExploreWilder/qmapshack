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

#ifndef C3DMAP_H
#define C3DMAP_H

#include <memory>
#include <chrono>
#include <QWindow>
#include <QMainWindow>
#include <QThread>
#include <QOpenGLContext>
#include <QSurface>
#include <QStatusBar>

namespace vts
{
    class Map;
    class Camera;
    class Navigation;

    namespace renderer
    {
        class RenderContext;
        class RenderView;
    }
}

class Gl : public QOpenGLContext
{
public:
    explicit Gl(QSurface *surface);

    void initialize();
    void current(bool bind = true);

    QSurface *surface;
};

class DataThread : public QThread
{
public:
    std::shared_ptr<Gl> gl;
    std::shared_ptr<vts::Map> map;

    void run() override;
};

class C3DMap : public QWindow
{
    Q_OBJECT
public:
    C3DMap();
    ~C3DMap();

    bool event(QEvent *event);
    void tick();

    void mouseMove(class QMouseEvent *event);
    void mousePress(class QMouseEvent *event);
    void mouseRelease(class QMouseEvent *event);
    void mouseWheel(class QWheelEvent *event);

    std::shared_ptr<Gl> gl;
    std::shared_ptr<vts::renderer::RenderContext> context;
    std::shared_ptr<vts::Map> map;
    std::shared_ptr<vts::Camera> camera;
    std::shared_ptr<vts::Navigation> navigation;
    std::shared_ptr<vts::renderer::RenderView> view;

    QPoint lastMousePosition;
    std::chrono::high_resolution_clock::time_point lastTime;

    DataThread dataThread;

signals:
    void sigMapReady();
    void sigMoveMap(const QPointF& pos);
    void sigZoomMap(QWheelEvent *event);

public slots:
    /**
       @brief Move the map to a specified position.
       @param pos The position of focus (center) in degree.
     */
    void slotMoveMap(const QPointF& pos);

    /**
       @brief Move and zoom in/out the map to a specified position and view extent.
       @param pos The position of focus (center) in degree.
       @param h The view extent in meters.
     */
    void slotZoomMap(const QPointF& pos, qreal h);
};

#endif //C3DMAP_H

