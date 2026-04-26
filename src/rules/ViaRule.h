#pragma once

#include <string>
#include <vector>

namespace freerouting {
namespace rules {

class ViaInfo
{
public:
    ViaInfo(const std::string& p_name, int p_drill,
           const std::vector<int>& p_layer_indizes);

    std::string get_name() const { return name_; }
    int get_drill() const { return drill_; }
    int get_layer_count() const { return static_cast<int>(layer_indices_.size()); }
    int get_layer(int p_index) const { return layer_indices_[p_index]; }

    std::string to_string() const;

private:
    std::string name_;
    int drill_;
    std::vector<int> layer_indices_;
};

class ViaRule
{
public:
    static const ViaRule EMPTY;

    explicit ViaRule(const std::string& p_name);

    const std::string& get_name() const { return name_; }

    void append_via(const ViaInfo& p_via);
    bool remove_via(const ViaInfo& p_via);

    int via_count() const { return static_cast<int>(via_list_.size()); }
    const ViaInfo& get_via(int p_index) const;

    std::string to_string() const;

private:
    std::string name_;
    std::vector<ViaInfo> via_list_;
};

} // namespace rules
} // namespace freerouting