#pragma once

#include <atomic>

namespace freerouting {
namespace datastructures {

/**
 * Interface for creating unique identification numbers.
 */
class IdentificationNumberGenerator
{
public:
    virtual ~IdentificationNumberGenerator() = default;

    virtual int new_no() = 0;
    virtual int max_generated_no() const = 0;
};

/**
 * Implementation using atomic counter
 */
class IdentificationNumberGeneratorImpl : public IdentificationNumberGenerator
{
public:
    IdentificationNumberGeneratorImpl() = default;

    int new_no() override
    {
        return next_no_.fetch_add(1) + 1;
    }

    int max_generated_no() const override
    {
        return next_no_.load();
    }

private:
    std::atomic<int> next_no_{0};
};

} // namespace datastructures
} // namespace freerouting