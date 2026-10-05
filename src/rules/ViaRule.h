#pragma once

#include <memory>
#include <string>
#include <vector>

#include "rules/ViaInfo.h"

namespace freerouting {
namespace board {
class Padstack;
} // namespace board

namespace rules {

class ViaRule
{
public:
    static const ViaRule EMPTY;

    explicit ViaRule(const std::string& p_name);

    const std::string& get_name() const { return name_; }

    void append_via(const std::shared_ptr<ViaInfo>& p_via);
    bool remove_via(const std::shared_ptr<ViaInfo>& p_via);

    int via_count() const { return static_cast<int>(via_list_.size()); }
    std::shared_ptr<ViaInfo> get_via(int p_index) const;

    bool contains(const std::shared_ptr<ViaInfo>& p_via_info) const;
    bool contains_padstack(const std::shared_ptr<board::Padstack>& p_padstack) const;

    /**
     * Non owning overload of contains_padstack(), for callers which hold a
     * padstack pointer, such as a board item. Port of Java
     * ViaRule.containsPadstack(Padstack), which compares the padstack identity.
     */
    bool contains_padstack(const board::Padstack* p_padstack) const;

    bool swap(const std::shared_ptr<ViaInfo>& p_1, const std::shared_ptr<ViaInfo>& p_2);

    std::string to_string() const;

private:
    std::string name_;
    std::vector<std::shared_ptr<ViaInfo>> via_list_;
};

} // namespace rules
} // namespace freerouting