#include "board/Components.h"
#include "board/Component.h"
#include "datastructures/UndoableObjects.h"
#include "logger/FRLogger.h"

#include <sstream>

namespace freerouting {
namespace board {

Components::Components()
    : flip_style_rotate_first_(false)
{
    undo_list_ = std::make_unique<UndoableObjects>();
    logger::FRLogger::instance().info("Creating Components container");
}

std::shared_ptr<Component> Components::add(const std::string& p_name,
                                           const geometry::planar::Point& p_location,
                                           double p_rotation_in_degree,
                                           bool p_on_front,
                                           const std::shared_ptr<core::Package>& p_package_front,
                                           const std::shared_ptr<core::Package>& p_package_back,
                                           bool p_position_fixed)
{
    // TODO: Implement Component constructor properly
    // For now, return nullptr as placeholder
    logger::FRLogger::instance().info("Adding component: " + p_name);
    
    // Create component (placeholder implementation)
    auto new_component = std::make_shared<Component>();
    component_arr_.push_back(new_component);
    
    // TODO: Implement undo list insertion
    // undo_list_->insert(new_component);
    
    return new_component;
}

std::shared_ptr<Component> Components::add(const geometry::planar::Point& p_location,
                                         double p_rotation,
                                         bool p_on_front,
                                         const std::shared_ptr<core::Package>& p_package)
{
    std::ostringstream oss;
    oss << "Component#" << (component_arr_.size() + 1);
    std::string component_name = oss.str();
    
    return add(component_name, p_location, p_rotation, p_on_front, 
               p_package, p_package, false);
}

std::shared_ptr<Component> Components::get(const std::string& p_name) const
{
    for (const auto& curr : component_arr_) {
        // TODO: Implement Component::get_name() method
        // if (curr->get_name() == p_name) {
        //     return curr;
        // }
    }
    return nullptr;
}

std::shared_ptr<Component> Components::get(int p_component_no) const
{
    if (p_component_no >= 0 && p_component_no < static_cast<int>(component_arr_.size())) {
        return component_arr_[p_component_no];
    }
    return nullptr;
}

bool Components::undo(const BoardObservers* p_observers)
{
    // TODO: Implement undo functionality
    logger::FRLogger::instance().info("Components::undo not yet implemented");
    return false;
}

bool Components::redo(const BoardObservers* p_observers)
{
    // TODO: Implement redo functionality
    logger::FRLogger::instance().info("Components::redo not yet implemented");
    return false;
}

void Components::move(int p_component_no, const geometry::planar::Vector& p_vector)
{
    if (auto component = get(p_component_no)) {
        // TODO: Implement Component::move() method
        logger::FRLogger::instance().info("Moving component " + std::to_string(p_component_no));
    }
}

void Components::turn_90_degree(int p_component_no, int p_factor, const geometry::planar::IntPoint& p_pole)
{
    if (auto component = get(p_component_no)) {
        // TODO: Implement Component::turn_90_degree() method
        logger::FRLogger::instance().info("Turning component " + std::to_string(p_component_no) + 
                                         " 90 degrees");
    }
}

void Components::rotate(int p_component_no, double p_rotation_in_degree, const geometry::planar::IntPoint& p_pole)
{
    if (auto component = get(p_component_no)) {
        // TODO: Implement Component::rotate() method
        logger::FRLogger::instance().info("Rotating component " + std::to_string(p_component_no) + 
                                         " by " + std::to_string(p_rotation_in_degree) + " degrees");
    }
}

void Components::change_side(int p_component_no, const geometry::planar::IntPoint& p_pole)
{
    if (auto component = get(p_component_no)) {
        // TODO: Implement Component::change_side() method
        logger::FRLogger::instance().info("Changing side of component " + std::to_string(p_component_no));
    }
}

void Components::restore_from_undo_list(const BoardObservers* p_observers)
{
    // TODO: Implement restore from undo list
    logger::FRLogger::instance().info("Components::restore_from_undo_list not yet implemented");
}

} // namespace board
} // namespace freerouting
