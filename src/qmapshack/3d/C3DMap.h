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

#include "CAuth.h"
#include <memory>
#include <chrono>
#include <QWindow>
#include <QMainWindow>
#include <QThread>
#include <QOpenGLContext>
#include <QSurface>
#include <vts-browser/math.hpp>
#include <vts-renderer/classes.hpp>

namespace vts
{
    class Map;
    class MapView;
    class Camera;
    class CameraCredits;
    class Navigation;
    class ResourceInfo;
    class GpuMeshSpec;

    namespace renderer
    {
        class RenderContext;
        class RenderView;
        class Mesh;
    }
}

typedef enum CreditsType
{
    IMAGERY,
    GEODATA
} CreditsType;

typedef struct Mark
{
    vts::vec3 coord = {};
    vts::vec3f color = {};
    bool visible = false;
} Mark;

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
    C3DMap(const CAuth &auth);
    ~C3DMap();

    /// Load mesh sphere and line.
    void loadResources();

    /**
      @brief Render the cursor marker with its location and color already updated.

      The elevation of the marker must be defined because guessing the altitude
      from the rendered surface is tricky. Please read the GitHub issue in the
      Melown repo for further information. The marker is hidden if the altitude
      is not a number.
     */
    void renderCursorMark();

    /// Render a compass on the bottom left corner of the map.
    void renderCompass();

    /// Copy resources to cache if not already existing, and load the mapConfig.
    void setupConfig();

    // Returns a list of available bound layers.
    QVector<QString> getBoundLayers();

    /// Returns the imagery credits (imagery or geodata).
    QString credits(CreditsType creditsType);

    bool event(QEvent *event);
    void tick();

    void mouseMove(class QMouseEvent *event);
    void mousePress(class QMouseEvent *event);
    void mouseRelease(class QMouseEvent *event);
    void mouseWheel(class QWheelEvent *event);
    void mouseDoubleClick(class QMouseEvent *event);

    std::shared_ptr<Gl> gl;
    std::shared_ptr<vts::renderer::RenderContext> context;
    std::shared_ptr<vts::renderer::Mesh> meshSphere;
    std::shared_ptr<vts::renderer::Mesh> meshLine;
    std::shared_ptr<vts::Map> map;
    std::shared_ptr<vts::Camera> camera;
    std::shared_ptr<vts::Navigation> navigation;
    std::shared_ptr<vts::renderer::RenderView> view;

    QPoint lastMousePosition;
    std::chrono::high_resolution_clock::time_point lastTime;

    DataThread dataThread;
    Mark cursorMark;

signals:
    void sigMapReady();
    void sigMoveMap(const QPointF& pos);
    void sigZoomMap(QWheelEvent *event);
    void sigMouseMove(const QPointF& pos, qreal ele);

public slots:
    /**
      @brief Change the cursor visibility.
      @param visible True to be displayed, false to hide.
     */
    void slotCursorVisibility(const bool visible);

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

    /**
       @brief Update the pointer position based on the 2D map.
       @param pos The cursor position in degree.
       @param ele The altitude in meters.
     */
    void slotMouseMove(const QPointF& pos, qreal ele);

    /**
       @brief Change the current boundlayer to an other one available in the mapConfig.
       @param boundLayer Name of the boundlayer as defined in the mapConfig.
      */
    void slotSetBoundLayer(const QString& boundLayer);

private:
    QString bingUrl;
    QString mapboxToken;
    QString uuid;
};

#endif //C3DMAP_H

