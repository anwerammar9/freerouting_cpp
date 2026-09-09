#pragma once

#include <memory>
#include <vector>
#include <string>

namespace freerouting {
namespace core {

struct PartPin
{
    PartPin(int p_pin_no, const std::string& p_pin_name,
          const std::string& p_gate_name, int p_gate_swap_code,
          const std::string& p_gate_pin_name, int p_gate_pin_swap_code)
        : pin_no(p_pin_no)
        , pin_name(p_pin_name)
        , gate_name(p_gate_name)
        , gate_swap_code(p_gate_swap_code)
        , gate_pin_name(p_gate_pin_name)
        , gate_pin_swap_code(p_gate_pin_swap_code)
    {}

    int pin_no;
    std::string pin_name;
    std::string gate_name;
    int gate_swap_code;
    std::string gate_pin_name;
    int gate_pin_swap_code;
};

class LogicalPart
{
public:
    LogicalPart(const std::string& p_name, int p_no,
              const std::vector<std::shared_ptr<PartPin>>& p_pins);

    const std::string& get_name() const { return name_; }
    int get_no() const { return no_; }
    int pin_count() const { return static_cast<int>(pins_.size()); }

    std::shared_ptr<PartPin> get_pin(int p_no) const;
    const std::vector<std::shared_ptr<PartPin>>& get_pins() const { return pins_; }

    std::string to_string() const;

private:
    std::string name_;
    int no_;
    std::vector<std::shared_ptr<PartPin>> pins_;
};

class LogicalParts
{
public:
    LogicalParts() = default;

    void add(const std::shared_ptr<LogicalPart>& p_part);
    int count() const { return static_cast<int>(parts_.size()); }

    std::shared_ptr<LogicalPart> get(int p_no) const;
    std::shared_ptr<LogicalPart> get_by_name(const std::string& p_name) const;

    const std::vector<std::shared_ptr<LogicalPart>>& get_all() const { return parts_; }

    std::string to_string() const;

private:
    std::vector<std::shared_ptr<LogicalPart>> parts_;
};

} // namespace core
} // namespace freerouting