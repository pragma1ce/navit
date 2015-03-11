#include <glib.h>
#include "graphics_qt_offscreen.h"
#include "navit/window.h"
#include "navit/event.h"
#include "navit/callback.h"
#include "config.h"
#include "plugin.h"

#include <QtCore/QtDebug>
#include <QtCore/QEventLoop>
#include <QtCore/QElapsedTimer>

#include <QtGui/QPen>
#include <QtGui/QBrush>
#include <QtGui/QPixmapCache>
#include <QtGui/QFont>
#include <QtGui/QPainter>
#include <QtGui/QPaintEngine>


#include <QtWidgets/QApplication>

void qt_offscreen_draw(graphics_priv* gr, const QRect* r, int paintev);
void event_qt_remove_timeout(event_timeout* ev);

namespace {
const std::uint16_t defaultWidth = 1900;
const std::uint16_t defaultHeight = 1080;

// For testing purposes only
struct Counter {
    int polygons = 0;
    int lines = 0;
    QElapsedTimer draw;
};

static Counter ctrs;
}

template <typename T>
QDebug operator<<(QDebug dbg, const std::unique_ptr<T>& ptr)
{
    dbg.space() << ptr.get();
    return dbg.space();
}

QDebug operator<<(QDebug dbg, const Counter& ctrs)
{
    dbg.nospace() << "Polygons =" << ctrs.polygons << "lines=" << ctrs.lines;
    return dbg.space();
}

static void
overlay_rect(struct graphics_priv* parent, struct graphics_priv* overlay, int clean, QRect* r)
{
    qDebug() << Q_FUNC_INFO;
    struct point p;
    int w, h;
    if (clean) {
        p = overlay->pclean;
    } else {
        p = overlay->p;
        ;
    }
    w = overlay->buffer->width();
    h = overlay->buffer->height();
    if (overlay->wraparound) {
        if (p.x < 0)
            p.x += parent->buffer->width();
        if (p.y < 0)
            p.y += parent->buffer->height();
        if (w < 0)
            w += parent->buffer->width();
        if (h < 0)
            h += parent->buffer->height();
    }
    r->setRect(p.x, p.y, w, h);
}

void
qt_offscreen_draw(struct graphics_priv* gr, const QRect* r, int paintev)
{
    static int count = 0;
    qDebug() << Q_FUNC_INFO << count++ << paintev;
    if (!paintev) {
        //        if (r->x() <= -r->width())
        //            return;
        //        if (r->y() <= -r->height())
        //            return;
        //        if (r->x() > gr->widget->pixmap->width())
        //            return;
        //        if (r->y() > gr->widget->pixmap->height())
        //            return;
        //        gr->widget->update(*r);
        //        qt_offscreen_draw(gr, r, 1);
        return;
    }
    qDebug() << "ASD";
    //    QPixmap pixmap(r->width(), r->height());
    //    QPainter painter(&pixmap);
    struct graphics_priv* overlay = nullptr;
    if (!gr->overlay_disable)
        overlay = gr->overlays;
    if ((gr->p.x || gr->p.y) && gr->background_gc) {
        //        painter.setPen(*gr->background_gc->pen);
        //        painter.fillRect(0, 0, gr->buffer->width(), gr->buffer->height(), *gr->background_gc->brush);
    }
    //    painter.drawPixmap(QPoint(gr->p.x, gr->p.y), *gr->buffer, *r);
    //    while (overlay) {
    //        QRect ovr;
    //        overlay_rect(gr, overlay, 0, &ovr);
    //        if (!overlay->overlay_disable && r->intersects(ovr)) {
    //            unsigned char* data;
    //            int i, size = ovr.width() * ovr.height();
    //            QImage img = overlay->buffer->toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    //            data = img.bits();
    //            for (i = 0; i < size; i++) {
    //                if (data[0] == overlay->rgba[0] && data[1] == overlay->rgba[1] && data[2] == overlay->rgba[2])
    //                    data[3] = overlay->rgba[3];
    //                data += 4;
    //            }
    //            painter.drawImage(QPoint(ovr.x() - r->x(), ovr.y() - r->y()), img);
    //        }
    //        overlay = overlay->next;
    //    }
//    const QString frame = QString("/tmp/frame%1.png").arg(count);
//    qDebug() << "Saving frame " << frame;
    qDebug() << ctrs;
//    gr->buffer->save(frame);
}

struct graphics_font_priv {
    QFont* font;
};

struct graphics_image_priv {
    QPixmap* pixmap;
};

static void graphics_destroy(struct graphics_priv* gr)
{
    qDebug() << Q_FUNC_INFO;
    gr->freetype_methods.destroy();
    delete gr;
}

static void font_destroy(struct graphics_font_priv* font)
{
}

static struct graphics_font_methods font_methods = {
    font_destroy
};

static graphics_font_priv* font_new(struct graphics_priv* gr, struct graphics_font_methods* meth, char* fontfamily, int size, int flags)
{
    graphics_font_priv* ret = new graphics_font_priv;
    ret->font = new QFont("Arial", size / 20);
    *meth = font_methods;
    return ret;
}

static void gc_destroy(struct graphics_gc_priv* gc)
{
    delete gc->pen;
    delete gc->brush;
    delete gc;
}

static void gc_set_linewidth(struct graphics_gc_priv* gc, int w)
{
    gc->pen->setWidth(w);
}

static void gc_set_dashes(struct graphics_gc_priv* gc, int w, int offset, unsigned char* dash_list, int n)
{
}

static void gc_set_foreground(struct graphics_gc_priv* gc, struct color* c)
{
    QColor col(c->r >> 8, c->g >> 8, c->b >> 8 /* , c->a >> 8 */);
    gc->pen->setColor(col);
    gc->brush->setColor(col);
    gc->c = *c;
}

static void gc_set_background(struct graphics_gc_priv* gc, struct color* c)
{
}

static struct graphics_gc_methods gc_methods = {
    gc_destroy,
    gc_set_linewidth,
    gc_set_dashes,
    gc_set_foreground,
    gc_set_background
};

static struct graphics_gc_priv* gc_new(struct graphics_priv* gr, struct graphics_gc_methods* meth)
{
    *meth = gc_methods;
    graphics_gc_priv* ret = new graphics_gc_priv;
    ret->pen = new QPen();
    ret->brush = new QBrush(Qt::SolidPattern);
    return ret;
}

static struct graphics_image_priv* image_new(struct graphics_priv* gr, struct graphics_image_methods* meth, char* path, int* w, int* h, struct point* hot, int rotation)
{
    struct graphics_image_priv* ret;
    QPixmap* cachedPixmap;
    QString key(path);

    ret = new graphics_image_priv;

    cachedPixmap = QPixmapCache::find(key);
    if (!cachedPixmap) {
        ret->pixmap = new QPixmap(path);
        if (ret->pixmap->isNull()) {
            delete ret;
            return nullptr;
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
    ctrs.lines++;
    int i;
    static QPolygon polygon;
    polygon.resize(count);

    for (i = 0; i < count; i++)
        polygon.setPoint(i, p[i].x, p[i].y);
    gr->painter->setPen(*gc->pen);
    gr->painter->drawPolyline(polygon);
}

static void draw_polygon(struct graphics_priv* gr, struct graphics_gc_priv* gc, struct point* p, int count)
{
    int i;
    QPolygon polygon;
    ctrs.polygons++;

    for (i = 0; i < count; i++)
        polygon.putPoints(i, 1, p[i].x, p[i].y);
    gr->painter->setPen(*gc->pen);
    gr->painter->setBrush(*gc->brush);
    gr->painter->drawConvexPolygon(polygon);
}

static void draw_rectangle(struct graphics_priv* gr, struct graphics_gc_priv* gc, struct point* p, int w, int h)
{
    gr->painter->fillRect(p->x, p->y, w, h, *gc->brush);
}

static void draw_circle(struct graphics_priv* gr, struct graphics_gc_priv* gc, struct point* p, int r)
{
    gr->painter->setPen(*gc->pen);
    gr->painter->drawArc(p->x - r / 2, p->y - r / 2, r, r, 0, 360 * 16);
}

static void draw_text(struct graphics_priv* gr, struct graphics_gc_priv* fg, struct graphics_gc_priv* bg, struct graphics_font_priv* font, char* text, struct point* p, int dx, int dy)
{
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
                gr->painter->drawImage(((x + g->x) >> 6) - 1, ((y + g->y) >> 6) - 1, img);
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
            gr->painter->drawImage((x + g->x) >> 6, (y + g->y) >> 6, img);
        }
        x += g->dx;
        y += g->dy;
    }
    gr->freetype_methods.text_destroy(t);
}

static void draw_image(struct graphics_priv* gr, struct graphics_gc_priv* fg, struct point* p, struct graphics_image_priv* img)
{
    gr->painter->drawPixmap(p->x, p->y, *img->pixmap);
}

static void background_gc(struct graphics_priv* gr, struct graphics_gc_priv* gc)
{
    qDebug() << Q_FUNC_INFO;
    gr->background_gc = gc;
    gr->rgba[2] = gc->c.r >> 8;
    gr->rgba[1] = gc->c.g >> 8;
    gr->rgba[0] = gc->c.b >> 8;
    gr->rgba[3] = gc->c.a >> 8;
}

static void draw_mode(struct graphics_priv* gr, enum draw_mode_num mode)
{
    qDebug() << gr << mode;
    QRect r;
    if (mode == draw_mode_begin) {
        ctrs.draw.start();
        if (gr->buffer->paintingActive()) {
            gr->buffer->paintEngine()->painter()->end();
        }
        gr->painter->begin(gr->buffer.get());
    }
    if (mode == draw_mode_end) {
        qDebug() << "Draw mode end" << gr->parent << gr->cleanup;
        qDebug() << "Took" << ctrs.draw.elapsed() << " ms";
        gr->painter->end();
        //        if (gr->parent) {
        //            if (gr->cleanup) {
        //                overlay_rect(gr->parent, gr, 1, &r);
        //                qt_offscreen_draw(gr->parent, &r, 0);
        //                gr->cleanup = 0;
        //            }
        //            overlay_rect(gr->parent, gr, 0, &r);
        //            qt_offscreen_draw(gr, &r, 0);
        //        } else {
        //            r.setRect(0, 0, gr->buffer->width(), gr->buffer->height());
        //            qt_offscreen_draw(gr, &r, 0);
        //        }
        overlay_rect(gr, gr, 0, &r);
        qt_offscreen_draw(gr, &r, 1);

        if (!gr->parent)
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers | QEventLoop::X11ExcludeTimers);
    }
    gr->mode = mode;
}


static int argc = 1;
static char* argv[] = { nullptr, nullptr, nullptr };

static int
fullscreen(struct window* win, int on)
{
    return 1;
}

static void
disable_suspend(struct window* win)
{
}

static void* get_data(struct graphics_priv* this_, const char* type)
{
    qDebug() << Q_FUNC_INFO << type;
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
    if (!strcmp(type, "window")) {
        win = new window;
        //        this_->buffer->paintEngine()->painter()->end();
        callback_list_call_attr_2(this_->cbl, attr_resize, defaultWidth, defaultHeight);
        win->priv = this_;
        win->fullscreen = fullscreen;
        win->disable_suspend = disable_suspend;
        return win;
    }
    return nullptr;
}

static void
image_free(struct graphics_priv* gr, struct graphics_image_priv* priv)
{
    delete priv->pixmap;
    delete priv;
}

static void
get_text_bbox(struct graphics_priv* gr, struct graphics_font_priv* font, char* text, int dx, int dy, struct point* ret, int estimate)
{
    QString tmp = QString::fromUtf8(text);
    gr->painter->setFont(*font->font);
    QRect r = gr->painter->boundingRect(0, 0, gr->w, gr->h, 0, tmp);
    ret[0].x = 0;
    ret[0].y = -r.height();
    ret[1].x = 0;
    ret[1].y = 0;
    ret[2].x = r.width();
    ret[2].y = 0;
    ret[3].x = r.width();
    ret[3].y = -r.height();
}

static void overlay_disable(struct graphics_priv* gr, int disable)
{
    qDebug() << Q_FUNC_INFO;
    gr->overlay_disable = disable;
}

static int set_attr(struct graphics_priv* gr, struct attr* attr)
{
    qDebug() << Q_FUNC_INFO;
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

static struct graphics_methods graphics_methods = {
    graphics_destroy,
    draw_mode,
    draw_lines,
    draw_polygon,
    draw_rectangle,
    draw_circle,
    draw_text,
    draw_image,
    nullptr,
    nullptr,
    nullptr,
    font_new,
    gc_new,
    background_gc,
    overlay_new,
    image_new,
    get_data,
    image_free,
    get_text_bbox,
    overlay_disable,
    nullptr,
    set_attr,
};

static struct graphics_priv* overlay_new(struct graphics_priv* gr, struct graphics_methods* meth, struct point* p, int w, int h, int alpha, int wraparound)
{
    qDebug() << Q_FUNC_INFO;
    *meth = graphics_methods;
    struct graphics_priv* ret = new graphics_priv;
    if (gr->font_freetype_new) {
        ret->font_freetype_new = gr->font_freetype_new;
        gr->font_freetype_new(&ret->freetype_methods);
        meth->font_new = (struct graphics_font_priv * (*)(struct graphics_priv*, struct graphics_font_methods*, char*, int, int))ret->freetype_methods.font_new;
        meth->get_text_bbox = (void (*)(struct graphics_priv*, struct graphics_font_priv*, char*, int, int, struct point*, int))ret->freetype_methods.get_text_bbox;
    }
    ret->wraparound = wraparound;
    ret->painter.reset(new QPainter);
    ret->p = *p;
    ret->parent = gr;
    ret->next = gr->overlays;
    gr->overlays = ret;
    return ret;
}

static struct graphics_priv* event_gr;

static void
event_qt_main_loop_run(void)
{
    qDebug() << Q_FUNC_INFO;
    event_gr->app->exec();
}

static void event_qt_main_loop_quit(void)
{
    exit(0);
}

static struct event_watch*
event_qt_add_watch(int fd, enum event_watch_cond cond, struct callback* cb)
{
    qDebug() << Q_FUNC_INFO;
    struct event_watch* ret = new event_watch;
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
    delete ev->sn;
    delete ev;
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
    event_qt_remove_timeout((struct event_timeout*)ev);
}

static void
event_qt_call_callback(struct callback_list* cb)
{
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
    qDebug() << Q_FUNC_INFO;
    *meth = event_qt_methods;
    return nullptr;
}

static struct graphics_priv* graphics_qt_offscreen_new(struct navit* nav, struct graphics_methods* meth, struct attr** attrs, struct callback_list* cbl)
{
    qDebug() << Q_FUNC_INFO;
    struct graphics_priv* ret;
    struct font_priv* (*font_freetype_new)(void* meth);
    struct attr* attr;

    if (event_gr)
        return nullptr;
    if (!event_request_system("qt", "graphics_qt_qpainter_new"))
        return nullptr;
    font_freetype_new = (struct font_priv * (*)(void*))plugin_get_font_type("freetype");
    if (!font_freetype_new) {
        return nullptr;
    }
    ret = new graphics_priv;
    ret->cbl = cbl;
    *meth = graphics_methods;
    ret->nav = nav;
    ret->font_freetype_new = font_freetype_new;
    font_freetype_new(&ret->freetype_methods);
    meth->font_new = (struct graphics_font_priv * (*)(struct graphics_priv*, struct graphics_font_methods*, char*, int, int))ret->freetype_methods.font_new;
    meth->get_text_bbox = (void (*)(struct graphics_priv*, struct graphics_font_priv*, char*, int, int, struct point*, int))ret->freetype_methods.get_text_bbox;

    ret->app.reset(new QApplication(argc, argv));
    ret->buffer.reset(new QPixmap(defaultWidth, defaultHeight));
    ret->buffer->fill();
    ret->painter.reset(new QPainter(ret->buffer.get()));
    qDebug() << "Crated pixmap" << ret->buffer << "and painter" << ret->painter;
    ret->painter->fillRect(0, 0, ret->buffer->width(), ret->buffer->height(), QBrush());
    event_gr = ret;
    ret->w = ret->buffer->width();
    ret->h = ret->buffer->height();
    if ((attr = attr_search(attrs, nullptr, attr_w)))
        ret->w = attr->u.num;
    if ((attr = attr_search(attrs, nullptr, attr_h)))
        ret->h = attr->u.num;
    if ((attr = attr_search(attrs, nullptr, attr_window_title)))
        ret->window_title = std::string(attr->u.str);
    else
        ret->window_title = "Navit";

    qDebug() << "Proper return";
    return ret;
}

extern "C" {
void plugin_init(void)
{
    plugin_register_graphics_type("qt_offscreen", graphics_qt_offscreen_new);
    plugin_register_event_type("qt", event_qt_new);
}
}
