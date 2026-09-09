#include "datastructures/IdentificationNumberGenerator.h"

namespace freerouting::datastructures {

int IdentificationNumberGeneratorImpl::new_no()
{
    return next_no_.fetch_add(1) + 1;
}

int IdentificationNumberGeneratorImpl::max_generated_no() const
{
    return next_no_.load();
}

} // namespace freerouting::datastructures