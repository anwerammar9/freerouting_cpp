#pragma once

namespace freerouting::datastructures {

/**
 * Interface for synchronizing changes to observed objects.
 */
template <typename ObjectType>
class Observers
{
public:
    virtual ~Observers() = default;

    virtual void notify_deleted(ObjectType p_object) = 0;
    virtual void notify_changed(ObjectType p_object) = 0;
    virtual void notify_new(ObjectType p_object) = 0;
    virtual void activate() = 0;
    virtual void deactivate() = 0;
    virtual bool is_active() const = 0;
};

} // namespace freerouting::datastructures
