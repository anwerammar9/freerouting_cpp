#include "management/SessionManager.h"

namespace freerouting {
namespace management {

SessionManager& SessionManager::instance()
{
    static SessionManager inst;
    return inst;
}

std::shared_ptr<core::Session> SessionManager::get_session(const std::string& session_id)
{
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<core::Session> SessionManager::get_session(const std::string& session_id, const std::string& user_id)
{
    auto session = get_session(session_id);
    if (!session) {
        return nullptr;
    }
    if (session->get_user_id() != user_id) {
        return nullptr;
    }
    return session;
}

std::shared_ptr<core::Session> SessionManager::create_session(const std::string& user_id, const std::string& host)
{
    auto session = std::make_shared<core::Session>(user_id, host);
    sessions_[session->get_id()] = session;
    // TODO: Increment statistics
    return session;
}

void SessionManager::remove_session(const std::string& session_id)
{
    sessions_.erase(session_id);
}

} // namespace management
} // namespace freerouting