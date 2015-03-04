#include "nxe_instance.h"
#include "navitprocess.h"
#include "navitcontroller.h"
#include "jsonmessage.h"
#include "settings.h"
#include "settingtags.h"
#include "log.h"
#include "calls.h"

#include <boost/algorithm/string.hpp>

namespace NXE {

struct NXEInstancePrivate {

    NXEInstancePrivate(std::weak_ptr<NavitProcess> p, std::weak_ptr<NavitIPCInterface> w, NXEInstance* qptr)
        : navitProcess(p)
        , q(qptr)
        , controller(w.lock())
    {
    }
    std::weak_ptr<NavitProcess> navitProcess;
    NXEInstance* q;
    NavitController controller;
    Settings settings;
    std::vector<NXEInstance::MessageCb_type> callbacks;
    std::map<std::string, std::chrono::time_point<std::chrono::high_resolution_clock> > timers;

    void postMessage(const JSONMessage& message)
    {
        const std::string rsp = JSONUtils::serialize(message);
        // This is xwalk posting mechanism
        q->PostMessage(rsp.c_str());

        // This is our internal post message
        std::for_each(callbacks.begin(), callbacks.end(), [&rsp](const NXEInstance::MessageCb_type& callback) {
            callback(rsp);
        });
    }

    void navitMsgCallback(const JSONMessage& response)
    {
        postMessage(response);
        auto it = timers.find(response.call);
        if (it != timers.end()) {
            auto now = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> diff = now - timers[response.call];
            nInfo() << "Parsing " << response.call << " took " << diff.count() << " ms";
            timers.erase(it);
        }
    }
};

NXEInstance::NXEInstance(std::weak_ptr<NavitProcess> process, std::weak_ptr<NavitIPCInterface> ipc)
    : d(new NXEInstancePrivate{ process, ipc, this })
{
    using SettingsTags::Navit::Path;

    auto navi = d->navitProcess.lock();
    assert(navi);

    nDebug() << "Creating NXE instance";
    if (navi) {
        std::string path{ d->settings.get<Path>() };
        nInfo() << "Setting navit path = [" << path << "]";
        navi->setProgramPath(path);
    }

    nDebug() << "Connecting to navitprocess signals";
    auto bound = std::bind(&NXEInstancePrivate::navitMsgCallback, d.get(), std::placeholders::_1);
    d->controller.addListener(bound);
}

NXEInstance::~NXEInstance()
{
    auto navit = d->navitProcess.lock();
    if (navit) {
        navit->stop();
    }
}

void NXEInstance::Initialize()
{
    nDebug() << "Initializing NXEInstance";
    using SettingsTags::Navit::AutoStart;
    bool bAutoRun = d->settings.get<AutoStart>();
    if (bAutoRun) {
        nInfo() << "Autorun is set, starting Navit";
        auto navi = d->navitProcess.lock();
        navi->start();
    }
}

void NXEInstance::HandleMessage(const char* msg)
{
    // lock shared ptr
    const auto naviProcess = d->navitProcess.lock();
    std::string message{ msg };

    boost::algorithm::erase_all(message, " ");
    boost::algorithm::erase_all(message, "\n");
    boost::algorithm::erase_all(message, "\t");

    nDebug() << "Handling message " << message;

    if (!naviProcess->isRunning()) {
        if (!naviProcess->start()) {
            d->postMessage(JSONMessage{ 0, "" });
        }
    }

    // Eat all exceptions!
    try {
        NXE::JSONMessage jsonMsg = JSONUtils::deserialize(message);

        try {
            d->controller.tryStart();
            d->timers[jsonMsg.call] = std::chrono::high_resolution_clock::now();
            d->controller.handleMessage(jsonMsg);
        }
        catch (const std::exception& ex) {
            nFatal() << "Unable to handle message " << jsonMsg.call << ", error=" << ex.what();
            NXE::JSONMessage error{ jsonMsg.id, jsonMsg.call, " some error" };
            auto it = d->timers.find(jsonMsg.call);
            if (it != d->timers.end()) {
                d->timers.erase(it);
            }
            d->postMessage(error);
        }
    }
    catch (const std::exception& ex) {
        NXE::JSONMessage error{ 0, "", 0 };
        nFatal() << "Unable to parse message, posting error= " << ex.what();
        d->postMessage(error);
    }
}

void NXEInstance::registerMessageCallback(const NXEInstance::MessageCb_type& cb)
{
    nTrace() << "registering cb";
    d->callbacks.push_back(cb);
}

} // namespace NXE
