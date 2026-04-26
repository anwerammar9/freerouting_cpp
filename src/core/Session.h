#pragma once

#include <string>
#include <memory>
#include <vector>

namespace freerouting {
namespace core {

class RoutingJob;

class Session
{
public:
    Session(const std::string& user_id, const std::string& host);
    
    std::string get_id() const { return id_; }
    std::string get_user_id() const { return user_id_; }
    std::string get_host() const { return host_; }
    
    void add_job(std::shared_ptr<RoutingJob> job);
    std::vector<std::shared_ptr<RoutingJob>>& get_jobs() { return jobs_; }
    
private:
    std::string id_;
    std::string user_id_;
    std::string host_;
    std::vector<std::shared_ptr<RoutingJob>> jobs_;
};

} // namespace core
} // namespace freerouting