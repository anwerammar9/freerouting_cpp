#pragma once

#include <string>
#include <vector>
#include <memory>

#include "rules/NetClass.h"

namespace freerouting {
namespace rules {

class BoardRules;

class Net
{
public:
    Net(int p_net_no, const std::string& p_name, NetClass* p_net_class);
    Net(int p_net_no,
        const std::string& p_name,
        int p_subnet_number,
        NetClass* p_net_class,
        bool p_contains_plane);

    int get_net_no() const { return net_no_; }
    const std::string& get_name() const { return name_; }
    void set_name(const std::string& p_name);

    int get_subnet_number() const { return subnet_number_; }

    bool contains_plane() const { return contains_plane_; }
    void set_contains_plane(bool p_value);

    NetClass* get_net_class() const { return net_class_; }
    void set_net_class(NetClass* p_net_class);

    int get_clearance_class() const { return clearance_class_; }
    void set_clearance_class(int p_class);

    bool is_ghost() const { return is_ghost_; }
    void set_ghost(bool p_value);

    bool is_has_items() const { return has_items_; }
    void set_has_items(bool p_value);

    bool is_routing_ignored() const { return routing_ignored_; }
    void set_routing_ignored(bool p_value);

    std::string to_string() const;

private:
    int net_no_;
    std::string name_;
    int subnet_number_;
    NetClass* net_class_;
    int clearance_class_;
    bool is_ghost_;
    bool has_items_;
    bool routing_ignored_;
    bool contains_plane_;
};

class Nets
{
public:
    static constexpr int max_legal_net_no = 9999999;
    static constexpr int hidden_net_no = 10000001;

    Nets();

    static bool is_normal_net_no(int p_net_no)
    {
        return p_net_no > 0 && p_net_no <= max_legal_net_no;
    }

    int max_net_no() const { return static_cast<int>(net_arr_.size()); }

    void add(Net* p_net);

    Net* get(int p_net_no) const;
    Net* get(const std::string& p_name, int p_subnet_number) const;
    std::vector<Net*> get_all_by_name(const std::string& p_name) const;
    Net* get_by_name(const std::string& p_name) const;

    Net* new_net();
    Net* add(const std::string& p_name, int p_subnet_number, bool p_contains_plane);

    int net_count() const { return static_cast<int>(net_arr_.size()); }
    int signal_net_count() const;

    /** The rules owning this net list; new nets get its default net class (Java Net ctor). */
    void set_rules(BoardRules* p_rules) { rules_ = p_rules; }
    void* get_board() const { return board_; }
    void set_board(void* p_board);

    const std::vector<Net*>& get_nets() const { return net_arr_; }

    std::string to_string() const;

private:
    std::vector<Net*> net_arr_;
    void* board_;
    BoardRules* rules_ = nullptr;
};

} // namespace rules
} // namespace freerouting