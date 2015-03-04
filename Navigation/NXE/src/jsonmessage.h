#ifndef JSONMESSAGE_H
#define JSONMESSAGE_H

#include <string>
#include <map>
#include <cstdint>
#include <boost/optional.hpp>
#include <boost/property_tree/ptree.hpp>

namespace NXE {

struct JSONMessage {
    // necessary fields
    const std::uint32_t id;
    const std::string call;
    // not necessary fields
    const std::string error;
    const boost::property_tree::ptree data;
};

namespace JSONUtils {
    JSONMessage deserialize(std::string buff);
    std::string serialize(JSONMessage json);
    std::string serialize(std::uint32_t id, const std::string& call, const std::string &err = "",
                          boost::property_tree::ptree data = boost::property_tree::ptree());
}
} // NXE

#endif // JSONMESSAGE_H
