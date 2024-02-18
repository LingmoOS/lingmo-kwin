/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2006 Lubos Lunak <l.lunak@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KWIN_SCENE_H
#define KWIN_SCENE_H

#include "kwineffects.h"
#include "renderlayerdelegate.h"
#include "utils/common.h"
#include "window.h"

#include <optional>

#include <QElapsedTimer>
#include <QMatrix4x4>

namespace KWin
{

namespace Decoration
{
class DecoratedClientImpl;
}

class Output;
class DecorationRenderer;
class Deleted;
class EffectFrameImpl;
class EffectWindowImpl;
class GLTexture;
class Item;
class RenderLoop;
class Scene;
class SceneWindow;
class Shadow;
class ShadowItem;
class SurfaceItem;
class SurfacePixmapInternal;
class SurfacePixmapWayland;
class SurfacePixmapX11;
class SurfaceTexture;
class WindowItem;

class SceneDelegate : public RenderLayerDelegate
{
    Q_OBJECT

public:
    explicit SceneDelegate(Scene *scene, QObject *parent = nullptr);
    explicit SceneDelegate(Scene *scene, Output *output, QObject *parent = nullptr);
    ~SceneDelegate() override;

    QRect viewport() const;

    QRegion repaints() const override;
    SurfaceItem *scanoutCandidate() const override;
    void prePaint() override;
    void postPaint() override;
    void paint(RenderTarget *renderTarget, const QRegion &region) override;

private:
    Scene *m_scene;
    Output *m_output = nullptr;
};

class KWIN_EXPORT Scene : public QObject
{
    Q_OBJECT

public:
    explicit Scene(QObject *parent = nullptr);
    ~Scene() override;
    class EffectFrame;

    void initialize();

    /**
     * Schedules a repaint for the specified @a region.
     */
    void addRepaint(const QRegion &region);
    void addRepaint(int x, int y, int width, int height);
    void addRepaintFull();
    QRegion damage() const;

    QRect geometry() const;
    void setGeometry(const QRect &rect);

    QList<SceneDelegate *> delegates() const;
    void addDelegate(SceneDelegate *delegate);
    void removeDelegate(SceneDelegate *delegate);

    // Returns true if the ctor failed to properly initialize.
    virtual bool initFailed() const = 0;

    SurfaceItem *scanoutCandidate() const;
    void prePaint(Output *output);
    void postPaint();
    virtual void paint(RenderTarget *renderTarget, const QRegion &region) = 0;

    /**
     * Adds the Window to the Scene.
     *
     * If the window gets deleted, then the scene will try automatically
     * to re-bind an underlying scene window to the corresponding Deleted.
     *
     * @param window The window to be added.
     * @note You can add a window to scene only once.
     */
    virtual SceneWindow *createWindow(Window *window) = 0;

    /**
     * @brief Creates the Scene backend of an EffectFrame.
     *
     * @param frame The EffectFrame this Scene::EffectFrame belongs to.
     */
    virtual Scene::EffectFrame *createEffectFrame(EffectFrameImpl *frame) = 0;
    /**
     * @brief Creates the Scene specific Shadow subclass.
     *
     * An implementing class has to create a proper instance. It is not allowed to
     * return @c null.
     *
     * @param window The Window for which the Shadow needs to be created.
     */
    virtual Shadow *createShadow(Window *window) = 0;
    // Flags controlling how painting is done.
    enum {
        // SceneWindow (or at least part of it) will be painted opaque.
        PAINT_WINDOW_OPAQUE = 1 << 0,
        // SceneWindow (or at least part of it) will be painted translucent.
        PAINT_WINDOW_TRANSLUCENT = 1 << 1,
        // SceneWindow will be painted with transformed geometry.
        PAINT_WINDOW_TRANSFORMED = 1 << 2,
        // Paint only a region of the screen (can be optimized, cannot
        // be used together with TRANSFORMED flags).
        PAINT_SCREEN_REGION = 1 << 3,
        // Whole screen will be painted with transformed geometry.
        PAINT_SCREEN_TRANSFORMED = 1 << 4,
        // At least one window will be painted with transformed geometry.
        PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS = 1 << 5,
        // Clear whole background as the very first step, without optimizing it
        PAINT_SCREEN_BACKGROUND_FIRST = 1 << 6,

        PAINT_WINDOW_LANCZOS = 1 << 8
    };

    virtual bool makeOpenGLContextCurrent();
    virtual void doneOpenGLContextCurrent();
    virtual bool supportsNativeFence() const;

    virtual QMatrix4x4 screenProjectionMatrix() const;

    virtual DecorationRenderer *createDecorationRenderer(Decoration::DecoratedClientImpl *) = 0;

    /**
     * Whether the Scene is able to drive animations.
     * This is used as a hint to the effects system which effects can be supported.
     * If the Scene performs software rendering it is supposed to return @c false,
     * if rendering is hardware accelerated it should return @c true.
     */
    virtual bool animationsSupported() const = 0;

    /**
     * The QPainter used by a QPainter based compositor scene.
     * Default implementation returns @c nullptr;
     */
    virtual QPainter *scenePainter() const;

    /**
     * The backend specific extensions (e.g. EGL/GLX extensions).
     *
     * Not the OpenGL (ES) extension!
     *
     * Default implementation returns empty list
     */
    virtual QVector<QByteArray> openGLPlatformInterfaceExtensions() const;

    virtual QSharedPointer<GLTexture> textureForOutput(Output *output) const
    {
        Q_UNUSED(output);
        return {};
    }

    virtual SurfaceTexture *createSurfaceTextureInternal(SurfacePixmapInternal *pixmap);
    virtual SurfaceTexture *createSurfaceTextureX11(SurfacePixmapX11 *pixmap);
    virtual SurfaceTexture *createSurfaceTextureWayland(SurfacePixmapWayland *pixmap);

    QMatrix4x4 renderTargetProjectionMatrix() const;
    QRect renderTargetRect() const;
    void setRenderTargetRect(const QRect &rect);
    qreal renderTargetScale() const;
    void setRenderTargetScale(qreal scale);

    QRegion mapToRenderTarget(const QRegion &region) const;

Q_SIGNALS:
    void frameRendered();

protected:
    void createStackingOrder();
    void clearStackingOrder();
    // shared implementation, starts painting the screen
    void paintScreen(const QRegion &region);
    friend class EffectsHandlerImpl;
    // called after all effects had their paintScreen() called
    void finalPaintScreen(int mask, const QRegion &region, ScreenPaintData &data);
    // shared implementation of painting the screen in the generic
    // (unoptimized) way
    void preparePaintGenericScreen();
    virtual void paintGenericScreen(int mask, const ScreenPaintData &data);
    // shared implementation of painting the screen in an optimized way
    void preparePaintSimpleScreen();
    virtual void paintSimpleScreen(int mask, const QRegion &region);
    // paint the background (not the desktop background - the whole background)
    virtual void paintBackground(const QRegion &region) = 0;
    // called after all effects had their paintWindow() called
    void finalPaintWindow(EffectWindowImpl *w, int mask, const QRegion &region, WindowPaintData &data);
    // shared implementation, starts painting the window
    virtual void paintWindow(SceneWindow *w, int mask, const QRegion &region);
    // called after all effects had their drawWindow() called
    void finalDrawWindow(EffectWindowImpl *w, int mask, const QRegion &region, WindowPaintData &data);

    virtual void paintOffscreenQuickView(OffscreenQuickView *w) = 0;

    // saved data for 2nd pass of optimized screen painting
    struct Phase2Data
    {
        SceneWindow *window = nullptr;
        QRegion region;
        QRegion opaque;
        int mask = 0;
    };

    struct PaintContext
    {
        QRegion damage;
        int mask = 0;
        QVector<Phase2Data> phase2Data;
    };

    // The screen that is being currently painted
    Output *painted_screen = nullptr;

    // windows in their stacking order
    QVector<SceneWindow *> stacking_order;

private:
    std::chrono::milliseconds m_expectedPresentTimestamp = std::chrono::milliseconds::zero();
    QList<SceneDelegate *> m_delegates;
    QRect m_geometry;
    QMatrix4x4 m_renderTargetProjectionMatrix;
    QRect m_renderTargetRect;
    qreal m_renderTargetScale = 1;
    // how many times finalPaintScreen() has been called
    int m_paintScreenCount = 0;
    PaintContext m_paintContext;
};

// The base class for windows representations in composite backends
class SceneWindow : public QObject
{
    Q_OBJECT

public:
    explicit SceneWindow(Window *client, QObject *parent = nullptr);
    ~SceneWindow() override;
    // perform the actual painting of the window
    virtual void performPaint(int mask, const QRegion &region, const WindowPaintData &data) = 0;
    int x() const;
    int y() const;
    int width() const;
    int height() const;
    QRect geometry() const;
    QPoint pos() const;
    QSize size() const;
    QRect rect() const;
    bool isPaintingEnabled() const;
    void resetPaintingEnabled();
    // access to the internal window class
    // TODO eventually get rid of this
    Window *window() const;
    QRegion decorationShape() const;
    void setWindow(Window *window);
    void referencePreviousPixmap();
    void unreferencePreviousPixmap();
    WindowItem *windowItem() const;
    SurfaceItem *surfaceItem() const;
    ShadowItem *shadowItem() const;

protected:
    Window *m_window;

private:
    void referencePreviousPixmap_helper(SurfaceItem *item);
    void unreferencePreviousPixmap_helper(SurfaceItem *item);

    void updateWindowPosition();

    QScopedPointer<WindowItem> m_windowItem;
    Q_DISABLE_COPY(SceneWindow)
};

class Scene::EffectFrame
{
public:
    EffectFrame(EffectFrameImpl *frame);
    virtual ~EffectFrame();
    virtual void render(const QRegion &region, double opacity, double frameOpacity) = 0;
    virtual void free() = 0;
    virtual void freeIconFrame() = 0;
    virtual void freeTextFrame() = 0;
    virtual void freeSelection() = 0;
    virtual void crossFadeIcon() = 0;
    virtual void crossFadeText() = 0;

protected:
    EffectFrameImpl *m_effectFrame;
};

inline int SceneWindow::x() const
{
    return m_window->x();
}

inline int SceneWindow::y() const
{
    return m_window->y();
}

inline int SceneWindow::width() const
{
    return m_window->width();
}

inline int SceneWindow::height() const
{
    return m_window->height();
}

inline QRect SceneWindow::geometry() const
{
    return m_window->frameGeometry();
}

inline QSize SceneWindow::size() const
{
    return m_window->size();
}

inline QPoint SceneWindow::pos() const
{
    return m_window->pos();
}

inline QRect SceneWindow::rect() const
{
    return m_window->rect();
}

inline Window *SceneWindow::window() const
{
    return m_window;
}

} // namespace

#endif
