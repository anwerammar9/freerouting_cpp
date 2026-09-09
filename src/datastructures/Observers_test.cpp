#include "datastructures/Observers.h"

#include <gtest/gtest.h>

#include <vector>

namespace freerouting::datastructures {
namespace {

class TestObservers : public Observers<int>
{
public:
    void notify_deleted(int p_object) override { deleted.push_back(p_object); }
    void notify_changed(int p_object) override { changed.push_back(p_object); }
    void notify_new(int p_object) override { created.push_back(p_object); }

    void activate() override { active = true; }
    void deactivate() override { active = false; }
    bool is_active() const override { return active; }

    bool active = false;
    std::vector<int> deleted;
    std::vector<int> changed;
    std::vector<int> created;
};

} // namespace

TEST(ObserversTest, SupportsNotificationsAndActivation)
{
    TestObservers observer;
    Observers<int>& interface = observer;

    EXPECT_FALSE(interface.is_active());
    interface.activate();
    EXPECT_TRUE(interface.is_active());

    interface.notify_new(1);
    interface.notify_changed(2);
    interface.notify_deleted(3);

    EXPECT_EQ(observer.created, std::vector<int>({1}));
    EXPECT_EQ(observer.changed, std::vector<int>({2}));
    EXPECT_EQ(observer.deleted, std::vector<int>({3}));

    interface.deactivate();
    EXPECT_FALSE(interface.is_active());
}

} // namespace freerouting::datastructures
