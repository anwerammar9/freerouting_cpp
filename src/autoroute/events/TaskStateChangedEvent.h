#pragma once

#include <string>

namespace freerouting::autoroute {

enum class TaskState;

}

namespace freerouting::autoroute {
class NamedAlgorithm;
}

namespace freerouting::autoroute::events {

class TaskStateChangedEvent {
public:
    TaskStateChangedEvent(NamedAlgorithm* p_source, autoroute::TaskState p_state,
                          int p_pass_number, const std::string& p_board_hash)
        : source_(p_source), state_(p_state), pass_number_(p_pass_number),
          board_hash_(p_board_hash) {}

    autoroute::TaskState get_state() const { return state_; }
    int get_pass_number() const { return pass_number_; }
    const std::string& get_board_hash() const { return board_hash_; }
    NamedAlgorithm* get_source() const { return source_; }

private:
    NamedAlgorithm* source_;
    autoroute::TaskState state_;
    int pass_number_;
    std::string board_hash_;
};

} // namespace freerouting::autoroute::events
