#pragma once

#include <memory>
#include <vector>
#include <string>

namespace freerouting {
namespace drc {

class UnconnectedItems
{
public:
    UnconnectedItems(const std::shared_ptr<class Item>& p_first,
                    const std::shared_ptr<class Item>& p_second);

    UnconnectedItems(const std::shared_ptr<class Item>& p_first,
                    const std::shared_ptr<class Item>& p_second,
                    const std::vector<std::shared_ptr<class Item>>& p_all_items);

    const std::shared_ptr<class Item>& get_first() const { return first_; }
    const std::shared_ptr<class Item>& get_second() const { return second_; }
    const std::vector<std::shared_ptr<class Item>>& get_all_items() const { return all_items_; }

    const std::string& get_type() const { return type_; }

private:
    std::shared_ptr<class Item> first_;
    std::shared_ptr<class Item> second_;
    std::vector<std::shared_ptr<class Item>> all_items_;
    std::string type_;
};

class DrcReport
{
public:
    DrcReport();

    void add_violation(const std::shared_ptr<UnconnectedItems>& p_violation);
    void add_violation(const std::shared_ptr<class DrcViolation>& p_violation);

    int violation_count() const { return static_cast<int>(violations_.size()); }
    const std::vector<std::shared_ptr<UnconnectedItems>>& get_violations() const { return violations_; }

    void clear() { violations_.clear(); }
    bool has_violations() const { return !violations_.empty(); }

    std::string to_string() const;

private:
    std::vector<std::shared_ptr<UnconnectedItems>> violations_;
    std::vector<std::shared_ptr<class DrcViolation>> drc_violations_;
};

} // namespace drc
} // namespace freerouting