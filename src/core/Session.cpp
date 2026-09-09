#include "core/Session.h"
#include "core/RoutingJob.h"

#include <sstream>
#include <iomanip>

namespace freerouting {
namespace core {

Session::Session(const std::string& user_id, const std::string& host)
    : user_id_(user_id), host_(host)
{
    // Generate a simple UUID-like ID
    std::stringstream ss;
    ss << std::hex << std::time(nullptr);
    // Add a simple uniqueifier
    static int counter = 0;
    ss << std::setfill('0') << std::setw(4) << counter++;
    id_ = ss.str();
}

void Session::add_job(std::shared_ptr<RoutingJob> job)
{
    jobs_.push_back(job);
}

} // namespace core
} // namespace freerouting