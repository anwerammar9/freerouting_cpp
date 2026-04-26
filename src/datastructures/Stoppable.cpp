#include "datastructures/Stoppable.h"

namespace freerouting {
namespace datastructures {

void Stoppable::request_stop()
{
    stopped_ = true;
}

bool Stoppable::is_stop_requested() const
{
    return stopped_;
}

} // namespace datastructures
} // namespace freerouting