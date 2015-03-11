#ifndef __GRAPHICS_QT_OFFSCREEN_H
#define __GRAPHICS_QT_OFFSCREEN_H

#include "navit/point.h"
#include "navit/item.h"
#include "navit/graphics.h"
#include "navit/color.h"

#include "navit/font/freetype/font_freetype.h"

// Give me modern c++ please
#include <string>
#include <memory>

class QSocketNotifier;
class QPen;
class QBrush;
class QApplication;
class QPainter;
class QPixmap;
class QRect;
struct callback_list;
struct event_timeout;
struct callback_list;
struct font_priv;

// Those structs have to be named that way
struct graphics_gc_priv {
    QPen* pen;
    QBrush* brush;
    color c;
};

struct graphics_priv {
    std::unique_ptr<QApplication> app;
    std::unique_ptr<QPainter> painter;
    std::unique_ptr<QPixmap> buffer = nullptr;
    callback_list *cbl;
    graphics_gc_priv* background_gc;
    unsigned char rgba[4];
    draw_mode_num mode;
    graphics_priv* parent, *overlays, *next;
    point p, pclean;
    int cleanup;
    int overlay_disable;
    int wraparound;
    font_priv* (*font_freetype_new)(void* meth);
    font_freetype_methods freetype_methods;
    int w, h, flags;
    navit* nav;
    std::string window_title;
};

struct event_watch {
    QSocketNotifier* sn;
    callback* cb;
    int fd;
};
static struct graphics_priv* overlay_new(struct graphics_priv* gr, struct graphics_methods* meth, struct point* p, int w, int h, int alpha, int wraparound);

#endif /* __GRAPHICS_QT_OFFSCREEN_H */
