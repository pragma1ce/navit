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

    NXEInstancePrivate(std::weak_ptr<NavitProcess> p, std::weak_ptr<NavitIPCInterface> w, NXEInstance *qptr):
        navitProcess(p),
        q(qptr),
        controller(w.lock())
    {}
    std::weak_ptr<NavitProcess> navitProcess;
    NXEInstance *q;
    NavitController controller;
    Settings settings;
    std::vector<NXEInstance::MessageCb_type> callbacks;

    void postMessage(const std::string& message)
    {
        // This is xwalk posting mechanism
        q->PostMessage(message.c_str());

        // This is our internal post message
        std::for_each(callbacks.begin(), callbacks.end(), [&message](const NXEInstance::MessageCb_type& callback) {
            callback(message);
        });
    }

    void navitMsgCallback(const std::string &response) {
        nDebug() << "Callback received posting response " << response;
        postMessage(response);
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
            d->postMessage("error");
        }
    }

    // Eat all exceptions!
    try {
        d->controller.tryStart();
        d->controller.handleMessage(JSONUtils::deserialize(message));
    }
    catch (const std::exception& ex) {
        nFatal() << "Unable to parse message, posting error= " << ex.what();
        d->postMessage(ex.what());
    }
}

void NXEInstance::registerMessageCallback(const NXEInstance::MessageCb_type& cb)
{
    nDebug() << "registering cb";
    d->callbacks.push_back(cb);
}

} // namespace NXE
