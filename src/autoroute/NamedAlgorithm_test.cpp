#include <gtest/gtest.h>
#include "autoroute/NamedAlgorithm.h"
#include "autoroute/NamedAlgorithmType.h"

namespace fr = freerouting;
using fr::autoroute::NamedAlgorithm;
using fr::autoroute::NamedAlgorithmType;

namespace {

class TestAlgorithm : public NamedAlgorithm {
public:
    std::string get_id() const override { return "test_algo"; }
    std::string get_name() const override { return "Test Algorithm"; }
    std::string get_version() const override { return "1.0"; }
    std::string get_description() const override { return "A test algorithm"; }
    NamedAlgorithmType get_type() const override { return NamedAlgorithmType::ROUTER; }
};

class TestListener : public fr::autoroute::events::BoardSnapshotEventListener {
public:
    void on_board_snapshot_event(const fr::autoroute::events::BoardSnapshotEvent& p_event) override {
        event_count_++;
    }
    int get_event_count() const { return event_count_; }
private:
    int event_count_ = 0;
};

} // anonymous namespace

TEST(NamedAlgorithmTest, GetId)
{
    TestAlgorithm algo;
    EXPECT_EQ(algo.get_id(), "test_algo");
}

TEST(NamedAlgorithmTest, GetName)
{
    TestAlgorithm algo;
    EXPECT_EQ(algo.get_name(), "Test Algorithm");
}

TEST(NamedAlgorithmTest, GetVersion)
{
    TestAlgorithm algo;
    EXPECT_EQ(algo.get_version(), "1.0");
}

TEST(NamedAlgorithmTest, GetDescription)
{
    TestAlgorithm algo;
    EXPECT_EQ(algo.get_description(), "A test algorithm");
}

TEST(NamedAlgorithmTest, GetType)
{
    TestAlgorithm algo;
    EXPECT_EQ(algo.get_type(), NamedAlgorithmType::ROUTER);
}

TEST(NamedAlgorithmTest, AddListener)
{
    TestAlgorithm algo;
    TestListener listener;
    algo.add_board_snapshot_event_listener(&listener);
    // No crash on add
}

TEST(NamedAlgorithmTest, MultipleListeners)
{
    TestAlgorithm algo;
    TestListener listener1;
    TestListener listener2;
    algo.add_board_snapshot_event_listener(&listener1);
    algo.add_board_snapshot_event_listener(&listener2);
    // No crash on add
}

TEST(NamedAlgorithmTest, AddNullListener)
{
    TestAlgorithm algo;
    algo.add_board_snapshot_event_listener(nullptr);
    // No crash on null add
}
