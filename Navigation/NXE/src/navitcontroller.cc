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

namespace bpt = boost::property_tree;

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
        boost::fusion::make_pair<MoveByMessage>([this](const bpt::ptree& data) {
            if (data.empty()) {
                // TODO: Change exception
                throw std::runtime_error("Unable to parse");
            }

            const int x = data.get<int>("x");
            const int y = data.get<int>("y");
            nDebug() << "IPC: Move by " << x << y;
            q->moveBy(x,y);

            // TODO: proper success signal
            successSignal("");
        }),

        boost::fusion::make_pair<ZoomByMessage>([this](const bpt::ptree& data) {
            int factor = data.get<int>("factor");
            q->zoomBy(factor);
        }),

        boost::fusion::make_pair<ZoomMessage>([this](const bpt::ptree& data) {
            int zoomValue = q->zoom();
            // TODO: proper success signal
            successSignal("");
        }),

        boost::fusion::make_pair<PositionMessage>([this](const bpt::ptree& data) {
            q->positon();
            // TODO: proper success signal
            successSignal("");
        }),
    };

    template <typename T>
    void handleMessage(const bpt::ptree & data)
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
    fun(NXE::NavitControllerPrivate* d, const bpt::ptree & data)

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
    const bpt::ptree & _data;
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
    bool bCalled = false;
    const boost::property_tree::ptree& val = msg.data.get_value_or(boost::property_tree::ptree());
    nDebug() << "Handling message " << msg.call;
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
        throw std::runtime_error("No parser found");
    }
}

void NavitController::addListener(const NavitController::Callback_type &cb)
{
    d->successSignal.connect(cb);
}

} // namespace NXE
