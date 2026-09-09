#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <memory>

#include "core/Session.h"

namespace freerouting {
namespace management {

class SessionManager
{
public:
    static SessionManager& instance();
    
    std::shared_ptr<core::Session> get_session(const std::string& session_id);
    std::shared_ptr<core::Session> get_session(const std::string& session_id, const std::string& user_id);
    std::shared_ptr<core::Session> create_session(const std::string& user_id, const std::string& host);
    void remove_session(const std::string& session_id);
    
private:
    SessionManager() = default;
    
    std::unordered_map<std::string, std::shared_ptr<core::Session>> sessions_;
};

} // namespace management
} // namespace freerouting