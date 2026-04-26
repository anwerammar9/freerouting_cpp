#pragma once

#include <memory>
#include <vector>
#include <string>

#include "board/Component.h"

namespace freerouting {
namespace board {

class Components : public std::enable_shared_from_this<Components>
{
public:
    Components();

    std::shared_ptr<Component> add(const std::string& p_name,
                                 const geometry::planar::Point& p_location,
                                 double p_rotation_in_degree,
                                 bool p_on_front,
                                 const std::shared_ptr<core::Package>& p_package_front,
                                 const std::shared_ptr<core::Package>& p_package_back,
                                 bool p_position_fixed);

    std::shared_ptr<Component> add(const geometry::planar::Point& p_location,
                                  double p_rotation,
                                  bool p_on_front,
                                  const std::shared_ptr<core::Package>& p_package);

    std::shared_ptr<Component> get(const std::string& p_name) const;
    std::shared_ptr<Component> get(int p_component_no) const;

    int count() const { return static_cast<int>(component_arr_.size()); }

    bool undo(const class BoardObservers* p_observers);
    bool redo(const class BoardObservers* p_observers);

    void move(int p_component_no, const geometry::planar::Vector& p_vector);
    void turn_90_degree(int p_component_no, int p_factor, const geometry::planar::IntPoint& p_pole);
    void rotate(int p_component_no, double p_rotation_in_degree, const geometry::planar::IntPoint& p_pole);
    void change_side(int p_component_no, const geometry::planar::IntPoint& p_pole);

    bool get_flip_style_rotate_first() const { return flip_style_rotate_first_; }
    void set_flip_style_rotate_first(bool p_value) { flip_style_rotate_first_ = p_value; }

private:
    void restore_from_undo_list(const class BoardObservers* p_observers);

    std::vector<std::shared_ptr<Component>> component_arr_;
    std::unique_ptr<class UndoableObjects> undo_list_;
    bool flip_style_rotate_first_ = false;
};

} // namespace board
} // namespace freerouting