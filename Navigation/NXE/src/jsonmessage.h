#ifndef JSONMESSAGE_H
#define JSONMESSAGE_H

#include <string>
#include <map>
#include <cstdint>
#include <boost/optional.hpp>
#include <boost/property_tree/ptree.hpp>

#include "calls.h"

namespace NXE {

struct JSONMessage {
    // necessary fields
    const std::uint32_t id;
    const std::string call;
    // not necessary fields
    const boost::optional<int> errorCode;
    const boost::optional<boost::property_tree::ptree> data;
};

namespace JSONUtils {
    JSONMessage deserialize(std::string buff);
    std::string serialize(JSONMessage json);
    std::string serialize(std::uint32_t id, const std::string& call, int ec = 0,
                          boost::property_tree::ptree data = boost::property_tree::ptree());
}
}

#endif // JSONMESSAGE_H
