#ifndef NXE_NAVITCONTROLLER_H
#define NXE_NAVITCONTROLLER_H

#include <memory>
#include <functional>
#include "jsonmessage.h"

namespace NXE {

struct NavitControllerPrivate;
class NavitController
{
public:
    typedef std::function<void (const std::string &) > Callback_type;

    // basic ctor
    NavitController();
    virtual ~NavitController();

    //! IPC start/stop
    virtual void start() = 0;
    virtual void stop() = 0;

    //! An IPC interface
    // NAVIT methods
    virtual void moveBy(double x, double y) = 0;

    virtual int zoom() = 0;
    virtual void zoomBy(int factor) = 0;

    // LBS functions
    virtual void positon();

    //! Common functions
    void tryStart() ;
    void handleMessage(JSONMessage msg);
    void addListener(const Callback_type &cb);

private:
    std::unique_ptr<NavitControllerPrivate> d;
};

} // namespace NXE

#endif // NXE_NAVITCONTROLLER_H
