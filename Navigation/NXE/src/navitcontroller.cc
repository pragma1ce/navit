#include "navitcontroller.h"
#include "log.h"

#include <functional>
#include <map>
#include <typeindex>
#include <thread>
#include <chrono>

#include <boost/lexical_cast.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/signals2/signal.hpp>

namespace NXE {
const std::uint16_t timeout = 2;

struct NavitControllerPrivate {
    NavitController* q;
    std::thread m_retriggerThread;
    bool m_isRunning = false;
    boost::signals2::signal<void (const std::string &) > successSignal;
    map_type m{ boost::fusion::make_pair<MoveByMessage>("moveBy"),
                boost::fusion::make_pair<ZoomByMessage>("zoomBy"),
                boost::fusion::make_pair<ZoomMessage>("zoom"),
                boost::fusion::make_pair<PositionMessage>("position") };

    map_cb_type cb{
        boost::fusion::make_pair<MoveByMessage>([this](const std::string& data) {
            q->moveBy(0,0);
        }),

        boost::fusion::make_pair<ZoomByMessage>([this](const std::string& data) {
            int factor = boost::lexical_cast<int>(data);
            q->zoomBy(factor);
        }),

        boost::fusion::make_pair<ZoomMessage>([this](const std::string& data) {
            int zoomValue = q->zoom();
        }),

        boost::fusion::make_pair<PositionMessage>([this](const std::string& data) {
            q->positon();
        }),
    };

    template <typename T>
    void handleMessage(const std::string& data)
    {
        auto fn = boost::fusion::at_key<T>(cb);
        fn(data);
    }
};

template <class Pred, class Fun>
struct filter {
    Pred pred_;
    const Fun& fun_;

    filter(Pred p, const Fun& f)
        : pred_(p)
        , fun_(f)
    {
    }

    template <class Pair>
    void operator()(Pair& pair) const
    {
        if (pred_(pair.second))
            fun_(pair);
    }
};

template <class Pred, class Fun>
filter<Pred, Fun> make_filter(Pred p, const Fun& f)
{
    return filter<Pred, Fun>(p, f);
}

struct fun {
    fun(NXE::NavitControllerPrivate* d, const std::string& data)
        : _d(d)
        , _data(data)
    {
    }
    template <class First, class Second>
    void operator()(boost::fusion::pair<First, Second>&) const
    {
        _d->handleMessage<First>(_data);
    }

    NXE::NavitControllerPrivate* _d;
    const std::string& _data;
};

NavitController::NavitController()
    : d(new NavitControllerPrivate)
{
    d->q = this;
}

NavitController::~NavitController()
{
}

void NavitController::positon()
{
    // TODO: Ask for LBS position
}

void NavitController::tryStart()
{
    nDebug() << "Trying to start IPC Navit controller";
    start();
}

void NavitController::handleMessage(JSONMessage msg)
{
    nDebug() << "Handling message " << msg.call;
    bool bCalled = false;
    try {
        const std::string& val = msg.data.get_value_or("");
        boost::fusion::for_each(d->m, make_filter([msg, &bCalled](const std::string& str) -> bool {
            if (str == msg.call)  {
                bCalled = true;
                return true;
            }
            return false;
                                                      },
                                                      fun(d.get(), val)));

        if (!bCalled) {
            nFatal() << "Unable to call " << msg.call;
        }
    }
    catch (const std::exception &ex)
    {
        nFatal() << "Unable to call IPC. Error= " << ex.what();
    }
}

void NavitController::addListener(const NavitController::Callback_type &cb)
{
    d->successSignal.connect(cb);
}

} // namespace NXE
