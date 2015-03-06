#include "graphics_qt_offscreen.h"

static void
overlay_rect(struct graphics_priv* parent, struct graphics_priv* overlay, int clean, QRect* r)
{
    struct point p;
    int w, h;
    if (clean) {
        p = overlay->pclean;
    } else {
        p = overlay->p;
        ;
    }
//    w = overlay->widget->pixmap->width();
    w = 1024;
    h = 800;
//    h = overlay->widget->pixmap->height();
    if (overlay->wraparound) {
//        if (p.x < 0)
//            p.x += parent->widget->pixmap->width();
//        if (p.y < 0)
//            p.y += parent->widget->pixmap->height();
//        if (w < 0)
//            w += parent->widget->pixmap->width();
//        if (h < 0)
//            h += parent->widget->pixmap->height();
    }
    r->setRect(p.x, p.y, w, h);
}

void
qt_offscreen_draw(struct graphics_priv* gr, const QRect* r, int paintev)
{
    static int count = 0;
    qDebug() << Q_FUNC_INFO << count++;
    if (!paintev) {
        dbg(lvl_debug, "update %d,%d %d x %d\n", r->x(), r->y(), r->width(), r->height());
//        if (r->x() <= -r->width())
//            return;
//        if (r->y() <= -r->height())
//            return;
//        if (r->x() > gr->widget->pixmap->width())
//            return;
//        if (r->y() > gr->widget->pixmap->height())
//            return;
        dbg(lvl_debug, "update valid %d,%d %dx%d\n", r->x(), r->y(), r->width(), r->height());
//        gr->widget->update(*r);
        return;
    }
    QPixmap pixmap(r->width(), r->height());
    QPainter painter(&pixmap);
    struct graphics_priv* overlay = NULL;
    if (!gr->overlay_disable)
        overlay = gr->overlays;
    if ((gr->p.x || gr->p.y) && gr->background_gc) {
        painter.setPen(*gr->background_gc->pen);
//        painter.fillRect(0, 0, gr->widget->pixmap->width(), gr->widget->pixmap->height(), *gr->background_gc->brush);
    }
//    painter.drawPixmap(QPoint(gr->p.x, gr->p.y), *gr->widget->pixmap, *r);
    while (overlay) {
        QRect ovr;
        overlay_rect(gr, overlay, 0, &ovr);
        if (!overlay->overlay_disable && r->intersects(ovr)) {
            unsigned char* data;
            int i, size = ovr.width() * ovr.height();
//            QImage img = overlay->widget->pixmap->toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
//            data = img.bits();
//            for (i = 0; i < size; i++) {
//                if (data[0] == overlay->rgba[0] && data[1] == overlay->rgba[1] && data[2] == overlay->rgba[2])
//                    data[3] = overlay->rgba[3];
//                data += 4;
//            }
//            painter.drawImage(QPoint(ovr.x() - r->x(), ovr.y() - r->y()), img);
        }
        overlay = overlay->next;
    }
//    QByteArray bytes;
//    QBuffer buffer (&bytes);
//    buffer.open(QIODevice::WriteOnly);
//    pixmap.save(&buffer);

//    QPainter painterw(gr->widget);
//    painterw.drawPixmap(r->x(), r->y(), pixmap);
}

struct graphics_font_priv {
    QFont* font;
};

struct graphics_image_priv {
    QPixmap* pixmap;
};

static void graphics_destroy(struct graphics_priv* gr)
{
#ifdef QT_QPAINTER_USE_FREETYPE
    gr->freetype_methods.destroy();
#endif
    g_free(gr->window_title);
    g_free(gr);
}

static void font_destroy(struct graphics_font_priv* font)
{
}

static struct graphics_font_methods font_methods = {
    font_destroy
};

static struct graphics_font_priv* font_new(struct graphics_priv* gr, struct graphics_font_methods* meth, char* fontfamily, int size, int flags)
{
    struct graphics_font_priv* ret = g_new0(struct graphics_font_priv, 1);
    ret->font = new QFont("Arial", size / 20);
    *meth = font_methods;
    return ret;
}

static void gc_destroy(struct graphics_gc_priv* gc)
{
    delete gc->pen;
    delete gc->brush;
    g_free(gc);
}

static void gc_set_linewidth(struct graphics_gc_priv* gc, int w)
{
    gc->pen->setWidth(w);
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void gc_set_dashes(struct graphics_gc_priv* gc, int w, int offset, unsigned char* dash_list, int n)
{
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void gc_set_foreground(struct graphics_gc_priv* gc, struct color* c)
{
    QColor col(c->r >> 8, c->g >> 8, c->b >> 8 /* , c->a >> 8 */);
    gc->pen->setColor(col);
    gc->brush->setColor(col);
    gc->c = *c;
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void gc_set_background(struct graphics_gc_priv* gc, struct color* c)
{
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct graphics_gc_methods gc_methods = {
    gc_destroy,
    gc_set_linewidth,
    gc_set_dashes,
    gc_set_foreground,
    gc_set_background
};

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct graphics_gc_priv* gc_new(struct graphics_priv* gr, struct graphics_gc_methods* meth)
{
    *meth = gc_methods;
    struct graphics_gc_priv* ret = g_new0(struct graphics_gc_priv, 1);
    ret->pen = new QPen();
    ret->brush = new QBrush(Qt::SolidPattern);
    return ret;
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct graphics_image_priv* image_new(struct graphics_priv* gr, struct graphics_image_methods* meth, char* path, int* w, int* h, struct point* hot, int rotation)
{
    struct graphics_image_priv* ret;
    QPixmap* cachedPixmap;
    QString key(path);

    ret = g_new0(struct graphics_image_priv, 1);

    cachedPixmap = QPixmapCache::find(key);
    if (!cachedPixmap) {
        ret->pixmap = new QPixmap(path);
        if (ret->pixmap->isNull()) {
            g_free(ret);
            return NULL;
        }

        QPixmapCache::insert(key, QPixmap(*ret->pixmap));
    } else {
        ret->pixmap = new QPixmap(*cachedPixmap);
    }

    *w = ret->pixmap->width();
    *h = ret->pixmap->height();
    if (hot) {
        hot->x = *w / 2;
        hot->y = *h / 2;
    }

    return ret;
}

static void draw_lines(struct graphics_priv* gr, struct graphics_gc_priv* gc, struct point* p, int count)
{
    int i;
    QPolygon polygon;

    for (i = 0; i < count; i++)
        polygon.putPoints(i, 1, p[i].x, p[i].y);
    gr->painter->setPen(*gc->pen);
    gr->painter->drawPolyline(polygon);
}

static void draw_polygon(struct graphics_priv* gr, struct graphics_gc_priv* gc, struct point* p, int count)
{
    int i;
    QPolygon polygon;

    for (i = 0; i < count; i++)
        polygon.putPoints(i, 1, p[i].x, p[i].y);
    gr->painter->setPen(*gc->pen);
    gr->painter->setBrush(*gc->brush);
    gr->painter->drawPolygon(polygon);
}

static void draw_rectangle(struct graphics_priv* gr, struct graphics_gc_priv* gc, struct point* p, int w, int h)
{
    dbg(lvl_debug, "gr=%p gc=%p %d,%d,%d,%d\n", gr, gc, p->x, p->y, w, h);
    gr->painter->fillRect(p->x, p->y, w, h, *gc->brush);
}

static void draw_circle(struct graphics_priv* gr, struct graphics_gc_priv* gc, struct point* p, int r)
{
    gr->painter->setPen(*gc->pen);
    gr->painter->drawArc(p->x - r / 2, p->y - r / 2, r, r, 0, 360 * 16);
}

static void draw_text(struct graphics_priv* gr, struct graphics_gc_priv* fg, struct graphics_gc_priv* bg, struct graphics_font_priv* font, char* text, struct point* p, int dx, int dy)
{
    QPainter* painter = gr->painter;
#ifndef QT_QPAINTER_USE_FREETYPE
    QString tmp = QString::fromUtf8(text);
#ifndef QT_NO_TRANSFORMATIONS
    QMatrix sav = gr->painter->worldMatrix();
    QMatrix m(dx / 65535.0, dy / 65535.0, -dy / 65535.0, dx / 65535.0, p->x, p->y);
    painter->setWorldMatrix(m, TRUE);
    painter->setPen(*fg->pen);
    painter->setFont(*font->font);
    painter->drawText(0, 0, tmp);
    painter->setWorldMatrix(sav);
#else
    painter->setPen(*fg->pen);
    painter->setFont(*font->font);
    painter->drawText(p->x, p->y, tmp);
#endif
#else
    struct font_freetype_text* t;
    struct font_freetype_glyph* g, **gp;
    struct color transparent = { 0x0000, 0x0000, 0x0000, 0x0000 };
    struct color* fgc = &fg->c, *bgc = &bg->c;

    int i, x, y;

    if (!font)
        return;
    t = gr->freetype_methods.text_new(text, (struct font_freetype_font*)font, dx, dy);
    x = p->x << 6;
    y = p->y << 6;
    gp = t->glyph;
    i = t->glyph_count;
    if (bg) {
        while (i-- > 0) {
            g = *gp++;
            if (g->w && g->h) {
                unsigned char* data;
                QImage img(g->w + 2, g->h + 2, QImage::Format_ARGB32_Premultiplied);
                data = img.bits();
                gr->freetype_methods.get_shadow(g, (unsigned char*)data, 32, img.bytesPerLine(), bgc, &transparent);

                painter->drawImage(((x + g->x) >> 6) - 1, ((y + g->y) >> 6) - 1, img);
            }
            x += g->dx;
            y += g->dy;
        }
    } else
        bgc = &transparent;
    x = p->x << 6;
    y = p->y << 6;
    gp = t->glyph;
    i = t->glyph_count;
    while (i-- > 0) {
        g = *gp++;
        if (g->w && g->h) {
            unsigned char* data;
            QImage img(g->w, g->h, QImage::Format_ARGB32_Premultiplied);
            data = img.bits();
            gr->freetype_methods.get_glyph(g, (unsigned char*)data, 32, img.bytesPerLine(), fgc, bgc, &transparent);
            painter->drawImage((x + g->x) >> 6, (y + g->y) >> 6, img);
        }
        x += g->dx;
        y += g->dy;
    }
    gr->freetype_methods.text_destroy(t);
#endif
}

static void draw_image(struct graphics_priv* gr, struct graphics_gc_priv* fg, struct point* p, struct graphics_image_priv* img)
{
    gr->painter->drawPixmap(p->x, p->y, *img->pixmap);
}

static void draw_restore(struct graphics_priv* gr, struct point* p, int w, int h)
{
}

static void
draw_drag(struct graphics_priv* gr, struct point* p)
{
    if (!gr->cleanup) {
        gr->pclean = gr->p;
        gr->cleanup = 1;
    }
    if (p)
        gr->p = *p;
    else {
        gr->p.x = 0;
        gr->p.y = 0;
    }
}

static void background_gc(struct graphics_priv* gr, struct graphics_gc_priv* gc)
{
    gr->background_gc = gc;
    gr->rgba[2] = gc->c.r >> 8;
    gr->rgba[1] = gc->c.g >> 8;
    gr->rgba[0] = gc->c.b >> 8;
    gr->rgba[3] = gc->c.a >> 8;
}

static void draw_mode(struct graphics_priv* gr, enum draw_mode_num mode)
{
    qDebug() << gr << mode;
    dbg(lvl_debug, "mode for %p %d\n", gr, mode);
    QRect r;
    if (mode == draw_mode_begin) {
//        if (gr->widget->pixmap->paintingActive()) {
//            gr->widget->pixmap->paintEngine()->painter()->end();
//        }
//        gr->painter->begin(gr->widget->pixmap);
    }
    if (mode == draw_mode_end) {
        gr->painter->end();
        if (gr->parent) {
            if (gr->cleanup) {
                overlay_rect(gr->parent, gr, 1, &r);
                qt_offscreen_draw(gr->parent, &r, 0);
                gr->cleanup = 0;
            }
            overlay_rect(gr->parent, gr, 0, &r);
            qt_offscreen_draw(gr->parent, &r, 0);
        } else {
//            r.setRect(0, 0, gr->widget->pixmap->width(), gr->widget->pixmap->height());
//            qt_qpainter_draw(gr, &r, 0);
        }

        if (!gr->parent)
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers | QEventLoop::X11ExcludeTimers);
    }
    gr->mode = mode;
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct graphics_priv* overlay_new(struct graphics_priv* gr, struct graphics_methods* meth, struct point* p, int w, int h, int alpha, int wraparound);

static int argc = 1;
static char* argv[] = { NULL, NULL, NULL };

static int
fullscreen(struct window* win, int on)
{
#ifndef QT_QPAINTER_NO_WIDGET
    struct graphics_priv* this_ = (struct graphics_priv*)win->priv;
    QWidget* _outerWidget;
#ifdef QT_QPAINTER_USE_EMBEDDING
    _outerWidget = (QWidget*)this_->widget->parent();
#else
//    _outerWidget = this_->widget;

#endif /* QT_QPAINTER_USE_EMBEDDING */
    if (on)
        _outerWidget->showFullScreen();
    else
        _outerWidget->showMaximized();
#endif
    return 1;
}

static void
disable_suspend(struct window* win)
{
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void* get_data(struct graphics_priv* this_, const char* type)
{
    struct window* win;
    QString xid;

//    if (!strcmp(type, "resize")) {
//        dbg(lvl_debug, "resize %d %d\n", this_->w, this_->h);
//        QSize size(this_->w, this_->h);
//        this_->widget->do_resize(size);
//    }

//    if (!strcmp(type, "qt_widget"))
//        return this_->widget;
//    if (!strcmp(type, "qt_pixmap"))
//        return this_->widget->pixmap;
//    if (!strcmp(type, "window")) {
//        win = g_new(struct window, 1);
//#ifndef QT_QPAINTER_NO_WIDGET
//        if (this_->w && this_->h)
//            this_->widget->show();
//        else
//            this_->widget->showMaximized();

//#endif /* QT_QPAINTER_NO_WIDGET */
//        win->priv = this_;
//        win->fullscreen = fullscreen;
//        win->disable_suspend = disable_suspend;
//        return win;
//    }
    return NULL;
}

static void
image_free(struct graphics_priv* gr, struct graphics_image_priv* priv)
{
    delete priv->pixmap;
    g_free(priv);
}

static void
get_text_bbox(struct graphics_priv* gr, struct graphics_font_priv* font, char* text, int dx, int dy, struct point* ret, int estimate)
{
    QPainter* painter = gr->painter;
    QString tmp = QString::fromUtf8(text);
    painter->setFont(*font->font);
    QRect r = painter->boundingRect(0, 0, gr->w, gr->h, 0, tmp);
    ret[0].x = 0;
    ret[0].y = -r.height();
    ret[1].x = 0;
    ret[1].y = 0;
    ret[2].x = r.width();
    ret[2].y = 0;
    ret[3].x = r.width();
    ret[3].y = -r.height();
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void overlay_disable(struct graphics_priv* gr, int disable)
{
    gr->overlay_disable = disable;
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static int set_attr(struct graphics_priv* gr, struct attr* attr)
{
    switch (attr->type) {
    case attr_w:
        gr->w = attr->u.num;
        if (gr->w != 0 && gr->h != 0) {
            QSize size(gr->w, gr->h);
//            gr->widget->do_resize(size);
        }
        break;
    case attr_h:
        gr->h = attr->u.num;
        if (gr->w != 0 && gr->h != 0) {
            QSize size(gr->w, gr->h);
//            gr->widget->do_resize(size);
        }
        break;
    default:
        return 0;
    }
    return 1;
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct graphics_methods graphics_methods = {
    graphics_destroy,
    draw_mode,
    draw_lines,
    draw_polygon,
    draw_rectangle,
    draw_circle,
    draw_text,
    draw_image,
    NULL,
    draw_restore,
    draw_drag,
    font_new,
    gc_new,
    background_gc,
    overlay_new,
    image_new,
    get_data,
    image_free,
    get_text_bbox,
    overlay_disable,
    NULL,
    set_attr,

};

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct graphics_priv* overlay_new(struct graphics_priv* gr, struct graphics_methods* meth, struct point* p, int w, int h, int alpha, int wraparound)
{
    *meth = graphics_methods;
    struct graphics_priv* ret = g_new0(struct graphics_priv, 1);
#ifdef QT_QPAINTER_USE_FREETYPE
    if (gr->font_freetype_new) {
        ret->font_freetype_new = gr->font_freetype_new;
        gr->font_freetype_new(&ret->freetype_methods);
        meth->font_new = (struct graphics_font_priv * (*)(struct graphics_priv*, struct graphics_font_methods*, char*, int, int))ret->freetype_methods.font_new;
        meth->get_text_bbox = (void (*)(struct graphics_priv*, struct graphics_font_priv*, char*, int, int, struct point*, int))ret->freetype_methods.get_text_bbox;
    }
#endif
    ret->wraparound = wraparound;
    ret->painter = new QPainter;
    ret->p = *p;
    ret->parent = gr;
    ret->next = gr->overlays;
    gr->overlays = ret;
#ifndef QT_QPAINTER_NO_WIDGET
//    ret->widget->hide();
#endif
    return ret;
}

#ifdef QT_QPAINTER_USE_EVENT_QT

static struct graphics_priv* event_gr;

static void
event_qt_main_loop_run(void)
{
    event_gr->app->exec();
}

static void event_qt_main_loop_quit(void)
{
    dbg(lvl_debug, "enter\n");
    exit(0);
}

static struct event_watch*
event_qt_add_watch(int fd, enum event_watch_cond cond, struct callback* cb)
{
    dbg(lvl_debug, "enter fd=%d\n", (int)(long) fd);
    struct event_watch* ret = g_new0(struct event_watch, 1);
    ret->fd = fd;
    ret->cb = cb;
//    g_hash_table_insert(event_gr->widget->watches, GINT_TO_POINTER(fd), ret);
//    ret->sn = new QSocketNotifier(fd, QSocketNotifier::Read, event_gr->widget);
//    QObject::connect(ret->sn, SIGNAL(activated(int)), event_gr->widget, SLOT(watchEvent(int)));
    return ret;
}

static void
event_qt_remove_watch(struct event_watch* ev)
{
//    g_hash_table_remove(event_gr->widget->watches, GINT_TO_POINTER(ev->fd));
    delete (ev->sn);
    g_free(ev);
}

static struct event_timeout*
event_qt_add_timeout(int timeout, int multi, struct callback* cb)
{
    int id;
//    id = event_gr->widget->startTimer(timeout);
//    g_hash_table_insert(event_gr->widget->timer_callback, (void*)id, cb);
//    g_hash_table_insert(event_gr->widget->timer_type, (void*)id, (void*)!!multi);
    return (struct event_timeout*)id;
}

void
event_qt_remove_timeout(struct event_timeout* ev)
{
//    event_gr->widget->killTimer((int)(long) ev);
//    g_hash_table_remove(event_gr->widget->timer_callback, ev);
//    g_hash_table_remove(event_gr->widget->timer_type, ev);
}

static struct event_idle*
event_qt_add_idle(int priority, struct callback* cb)
{
    return (struct event_idle*)event_qt_add_timeout(0, 1, cb);
}

static void
event_qt_remove_idle(struct event_idle* ev)
{
    dbg(lvl_debug, "enter\n");
    event_qt_remove_timeout((struct event_timeout*)ev);
}

static void
event_qt_call_callback(struct callback_list* cb)
{
    dbg(lvl_debug, "enter\n");
}

static struct event_methods event_qt_methods = {
    event_qt_main_loop_run,
    event_qt_main_loop_quit,
    event_qt_add_watch,
    event_qt_remove_watch,
    event_qt_add_timeout,
    event_qt_remove_timeout,
    event_qt_add_idle,
    event_qt_remove_idle,
    event_qt_call_callback,
};

struct event_priv {
};

struct event_priv*
event_qt_new(struct event_methods* meth)
{
    dbg(lvl_debug, "enter\n");
    *meth = event_qt_methods;
    return NULL;
}
#endif

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct graphics_priv* graphics_qt_offscreen_new(struct navit* nav, struct graphics_methods* meth, struct attr** attrs, struct callback_list* cbl)
{
    struct graphics_priv* ret;
    struct font_priv* (*font_freetype_new)(void* meth);
    struct attr* attr;

    dbg(lvl_debug, "enter\n");
    if (event_gr)
        return NULL;
    if (!event_request_system("qt", "graphics_qt_qpainter_new"))
        return NULL;
#ifdef QT_QPAINTER_USE_EVENT_GLIB
    if (!event_request_system("glib", "graphics_qt_qpainter_new"))
        return NULL;
#endif
#ifdef QT_QPAINTER_USE_FREETYPE
    font_freetype_new = (struct font_priv * (*)(void*))plugin_get_font_type("freetype");
    if (!font_freetype_new) {
        dbg(lvl_error, "no freetype\n");
        return NULL;
    }
#endif
    ret = g_new0(struct graphics_priv, 1);
    *meth = graphics_methods;
    ret->nav = nav;
#ifdef QT_QPAINTER_USE_FREETYPE
    ret->font_freetype_new = font_freetype_new;
    font_freetype_new(&ret->freetype_methods);
    meth->font_new = (struct graphics_font_priv * (*)(struct graphics_priv*, struct graphics_font_methods*, char*, int, int))ret->freetype_methods.font_new;
    meth->get_text_bbox = (void (*)(struct graphics_priv*, struct graphics_font_priv*, char*, int, int, struct point*, int))ret->freetype_methods.get_text_bbox;
#endif

    argv[0] = (char*)malloc(255);
    strcpy(argv[0], "navit");
    if ((attr = attr_search(attrs, NULL, attr_flags)))
        ret->flags = attr->u.num;
    if (ret->flags & 1) {
        argv[1] = (char*)malloc(255);
        strcpy(argv[1], "-qws");
        argc++;
    }
    ret->app = new QApplication(argc, argv);
//    ret->widget = new RenderArea(ret);
//    ret->widget->cbl = cbl;
    ret->painter = new QPainter;
    event_gr = ret;
    ret->w = 800;
    ret->h = 600;
    if ((attr = attr_search(attrs, NULL, attr_w)))
        ret->w = attr->u.num;
    if ((attr = attr_search(attrs, NULL, attr_h)))
        ret->h = attr->u.num;
    if ((attr = attr_search(attrs, NULL, attr_window_title)))
        ret->window_title = g_strdup(attr->u.str);
    else
        ret->window_title = g_strdup("Navit");

    dbg(lvl_debug, "return\n");
    return ret;
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
void plugin_init(void)
{
    plugin_register_graphics_type("qt_offscreen", graphics_qt_offscreen_new);
    plugin_register_event_type("qt", event_qt_new);
}

// *** EOF ***
