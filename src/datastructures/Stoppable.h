#pragma once

namespace freerouting {
namespace datastructures {

class Stoppable
{
public:
    virtual ~Stoppable() = default;

    virtual void request_stop() = 0;
    virtual bool is_stop_requested() const = 0;
};

} // namespace datastructures
} // namespace freerouting