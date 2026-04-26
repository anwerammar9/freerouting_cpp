#pragma once

#include <string>
#include <vector>
#include <memory>

namespace freerouting {
namespace rules {

class BoardRules;
struct Padstack;

class ViaInfo : public std::enable_shared_from_this<ViaInfo>
{
public:
    ViaInfo(const std::string& p_name,
          std::shared_ptr<Padstack> p_padstack,
          int p_clearance_class,
          bool p_attach_smd_allowed,
          BoardRules* p_board_rules);

    const std::string& get_name() const { return name_; }
    void set_name(const std::string& p_name) { name_ = p_name; }

    std::shared_ptr<Padstack> get_padstack() const { return padstack_; }
    void set_padstack(std::shared_ptr<Padstack> p_padstack) { padstack_ = p_padstack; }

    int get_clearance_class() const { return clearance_class_; }
    void set_clearance_class(int p_clearance_class) { clearance_class_ = p_clearance_class; }

    bool attach_smd_allowed() const { return attach_smd_allowed_; }
    void set_attach_smd_allowed(bool p_attach_smd_allowed) { attach_smd_allowed_ = p_attach_smd_allowed; }

    int compareTo(const ViaInfo& p_other) const;
    std::string to_string() const { return name_; }

private:
    std::string name_;
    std::shared_ptr<Padstack> padstack_;
    int clearance_class_;
    bool attach_smd_allowed_;
    BoardRules* board_rules_;
};

class ViaInfos
{
public:
    ViaInfos() = default;

    bool add(const std::shared_ptr<ViaInfo>& p_via_info);
    int count() const { return static_cast<int>(list_.size()); }
    std::shared_ptr<ViaInfo> get(int p_no) const;
    std::shared_ptr<ViaInfo> get(const std::string& p_name) const;
    bool name_exists(const std::string& p_name) const;
    bool remove(const std::shared_ptr<ViaInfo>& p_via_info);

private:
    std::vector<std::shared_ptr<ViaInfo>> list_;
};

} // namespace rules
} // namespace freerouting