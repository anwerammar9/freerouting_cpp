#pragma once

namespace freerouting {
namespace datastructures {

class Stoppable
{
public:
    virtual ~Stoppable() = default;

    virtual void request_stop();
    virtual bool is_stop_requested() const;

protected:
    bool stopped_ = false;
};

} // namespace datastructures
} // namespace freerouting