#include "autoroute/ExpansionRoom.h"
#include "autoroute/ItemAutorouteInfo.h"
#include "geometry/planar/FloatLine.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Simplex.h"
#include "board/ConductionArea.h"
#include "board/Item.h"
#include "board/PolylineTrace.h"
#include "board/ShapeSearchTree.h"
#include <cmath>
#include <algorithm>

namespace freerouting::autoroute {

CompleteExpansionRoom::CompleteExpansionRoom(int p_layer, int p_room_id)
    : layer_(p_layer), room_id_(p_room_id)
{
}

void CompleteExpansionRoom::add_door(ExpansionDoor* p_door)
{
    if (p_door)
    {
        doors_.push_back(p_door);
    }
}

std::vector<ExpansionDoor*> CompleteExpansionRoom::get_doors() const
{
    return doors_;
}

void CompleteExpansionRoom::clear_doors()
{
    doors_.clear();
}

void CompleteExpansionRoom::reset_doors()
{
    for (auto* door : doors_)
    {
        if (door)
        {
            door->reset();
        }
    }
}

bool CompleteExpansionRoom::door_exists(ExpansionRoom* p_other) const
{
    for (auto* door : doors_)
    {
        if (door)
        {
            if (door->other_room(p_other) != nullptr)
            {
                return true;
            }
        }
    }
    return false;
}

bool CompleteExpansionRoom::remove_door(ExpandableObject* p_door)
{
    auto it = std::find(doors_.begin(), doors_.end(), static_cast<ExpansionDoor*>(p_door));
    if (it != doors_.end())
    {
        doors_.erase(it);
        return true;
    }
    return false;
}

geometry::planar::IntBox CompleteExpansionRoom::get_shape() const
{
    return geometry::planar::IntBox::INFINITE;
}

void CompleteExpansionRoom::add_target_door(ExpansionDoor* p_door)
{
    if (p_door)
    {
        target_doors_.push_back(p_door);
    }
}

CompleteFreeSpaceExpansionRoom::CompleteFreeSpaceExpansionRoom(int p_layer, int p_room_id, const geometry::planar::IntBox& p_shape)
    : CompleteExpansionRoom(p_layer, p_room_id), shape_tile_(p_shape.to_Simplex())
{
}

CompleteFreeSpaceExpansionRoom::CompleteFreeSpaceExpansionRoom(int p_layer, int p_room_id, const geometry::planar::Simplex& p_shape)
    : CompleteExpansionRoom(p_layer, p_room_id), shape_tile_(p_shape)
{
}

geometry::planar::IntBox CompleteFreeSpaceExpansionRoom::get_shape() const
{
    return shape_tile_.bounding_box();
}

int CompleteFreeSpaceExpansionRoom::tree_shape_count(const datastructures::ShapeTree& p_tree) const
{
    (void)p_tree;
    return 1;
}

const geometry::planar::TileShape* CompleteFreeSpaceExpansionRoom::get_tree_shape(
    const datastructures::ShapeTree& p_tree, int p_index) const
{
    (void)p_tree;
    if (p_index != 0)
    {
        return nullptr;
    }
    return &shape_tile_;
}

void CompleteFreeSpaceExpansionRoom::set_search_tree_entries(
    std::vector<std::unique_ptr<datastructures::ShapeTree::Leaf>> p_entries, datastructures::ShapeTree& p_tree)
{
    (void)p_tree;
    tree_entries_ = std::move(p_entries);
}

int CompleteFreeSpaceExpansionRoom::compare_to(const datastructures::ShapeTree::Storable& p_other) const
{
    // Mirrors Java CompleteFreeSpaceExpansionRoom.compareTo(): rooms are
    // ordered by reversed id before any non-room object.
    const CompleteFreeSpaceExpansionRoom* other =
        dynamic_cast<const CompleteFreeSpaceExpansionRoom*>(&p_other);
    if (other == nullptr)
    {
        return -1;
    }
    return other->room_id_ - room_id_;
}

void CompleteFreeSpaceExpansionRoom::remove_from_tree(board::ShapeSearchTree& p_tree)
{
    std::vector<datastructures::ShapeTree::Leaf*> raw_entries;
    raw_entries.reserve(tree_entries_.size());
    for (auto& entry : tree_entries_)
    {
        raw_entries.push_back(entry.get());
    }
    static_cast<datastructures::ShapeTree&>(p_tree).remove(raw_entries);
    tree_entries_.clear();
}

ObstacleExpansionRoom::ObstacleExpansionRoom(board::Item* p_item, int p_index_in_item, int p_layer, const geometry::planar::IntBox& p_shape)
    : CompleteExpansionRoom(p_layer, (p_item->get_id() << 10) | p_index_in_item),
      item_(p_item), index_in_item_(p_index_in_item), shape_(p_shape), shape_tile_(p_shape.to_Simplex())
{
}

ObstacleExpansionRoom::ObstacleExpansionRoom(board::Item* p_item, int p_index_in_item, board::ShapeSearchTree* p_shape_tree)
    : ObstacleExpansionRoom(p_item, p_index_in_item, p_item->shape_layer(p_index_in_item),
          p_item->get_tree_shape(*p_shape_tree, p_index_in_item) != nullptr
              ? p_item->get_tree_shape(*p_shape_tree, p_index_in_item)->bounding_box()
              : geometry::planar::IntBox::EMPTY)
{
    const geometry::planar::TileShape* exact_shape =
        p_item->get_tree_shape(*p_shape_tree, p_index_in_item);
    if (exact_shape != nullptr)
    {
        shape_tile_ = exact_shape->to_Simplex();
    }
}

bool ObstacleExpansionRoom::create_overlap_door(ObstacleExpansionRoom* p_other)
{
    if (p_other == nullptr || door_exists(p_other))
    {
        return false;
    }
    if (!(item_->is_routable() && p_other->item_->is_routable()))
    {
        return false;
    }
    if (!item_->shares_net(p_other->item_))
    {
        return false;
    }
    if (item_ == p_other->item_)
    {
        // Two expansion rooms of the same item are only connected if their
        // shapes are neighbouring along a trace polyline.
        if (dynamic_cast<board::PolylineTrace*>(item_) == nullptr)
        {
            return false;
        }
        if (index_in_item_ != p_other->index_in_item_ + 1
            && index_in_item_ != p_other->index_in_item_ - 1)
        {
            return false;
        }
    }
    auto* new_door = new ExpansionDoor(this, p_other, 2);
    add_door(new_door);
    p_other->add_door(new_door);
    return true;
}

void CompleteFreeSpaceExpansionRoom::calculate_target_doors(
    const datastructures::ShapeTree::TreeEntry& p_own_net_object, int p_net_no,
    board::ShapeSearchTree* p_autoroute_tree)
{
    set_net_dependent();

    // C++ analogue of the Java `instanceof Connectable` guard: the C++
    // Connectable interface is a dead draft implemented by no real item, so a
    // connectable object is a routeable item (trace, drill) or a conduction
    // area.
    auto* current_item = dynamic_cast<board::Item*>(p_own_net_object.object);
    if (current_item == nullptr)
    {
        return;
    }
    if (!(current_item->is_trace() || current_item->is_via() || current_item->is_pin()
          || dynamic_cast<board::ConductionArea*>(current_item) != nullptr))
    {
        return;
    }
    if (!current_item->contains_net(p_net_no))
    {
        return;
    }
    const geometry::planar::TileShape* connection_shape =
        current_item->get_tree_shape(*p_autoroute_tree, p_own_net_object.shape_index_in_object);
    if (connection_shape == nullptr)
    {
        return;
    }
    if (!get_shape().intersects(connection_shape->to_Simplex()))
    {
        return;
    }
    auto* new_target_door = new TargetItemExpansionDoor(
        this, current_item, p_own_net_object.shape_index_in_object, p_autoroute_tree);
    add_target_door(new_target_door);
}

geometry::planar::IntBox ObstacleExpansionRoom::get_shape() const
{
    return shape_;
}

ExpansionDoor::ExpansionDoor(ExpansionRoom* p_first_room, ExpansionRoom* p_second_room, int p_dimension)
    : first_room_(p_first_room), second_room_(p_second_room), dimension_(p_dimension)
{
}

ExpansionDoor::ExpansionDoor(ExpansionRoom* p_first_room, ExpansionRoom* p_second_room)
    : first_room_(p_first_room)
    , second_room_(p_second_room)
    , dimension_(p_first_room->get_shape().intersection(p_second_room->get_shape()).dimension())
{
}

ExpansionRoom* ExpansionDoor::other_room(ExpansionRoom* p_room)
{
    if (p_room == first_room_)
        return second_room_;
    if (p_room == second_room_)
        return first_room_;
    return nullptr;
}

CompleteExpansionRoom* ExpansionDoor::other_room(CompleteExpansionRoom* p_room)
{
    ExpansionRoom* result = other_room(static_cast<ExpansionRoom*>(p_room));
    return dynamic_cast<CompleteExpansionRoom*>(result);
}

geometry::planar::IntBox ExpansionDoor::get_shape() const
{
    if (!first_room_ || !second_room_)
        return geometry::planar::IntBox::EMPTY;

    geometry::planar::IntBox first_shape = first_room_->get_shape();
    geometry::planar::IntBox second_shape = second_room_->get_shape();

    return first_shape.intersection(second_shape);
}

int ExpansionDoor::maze_search_element_count() const
{
    return static_cast<int>(section_arr_.size());
}

MazeSearchElement* ExpansionDoor::get_maze_search_element(int p_no)
{
    if (p_no >= 0 && p_no < static_cast<int>(section_arr_.size()))
    {
        return &section_arr_[p_no];
    }
    return nullptr;
}

void ExpansionDoor::reset()
{
    for (auto& elem : section_arr_)
    {
        elem.reset();
    }
}

int64_t ExpansionDoor::get_id() const
{
    // Port of Java ExpansionDoor.getId(): a stable combination of the room ids.
    // min/max ensures order-independence.
    const int64_t id1 = first_room_ != nullptr ? first_room_->get_room_id() : 0;
    const int64_t id2 = second_room_ != nullptr ? second_room_->get_room_id() : 0;
    return std::min(id1, id2) * 31 + std::max(id1, id2);
}

void ExpansionDoor::allocate_sections(int p_count)
{
    section_arr_.resize(p_count);
}

std::vector<geometry::planar::FloatLine> ExpansionDoor::get_section_segments(double p_offset)
{
    double offset = p_offset + 2;

    geometry::planar::IntBox door_shape = get_shape();
    if (door_shape == geometry::planar::IntBox::EMPTY)
    {
        return {};
    }

    const double c_max_door_section_width = 10 * offset;

    if (dimension_ == 1)
    {
        int64_t dx = door_shape.ur.x - door_shape.ll.x;
        int64_t dy = door_shape.ur.y - door_shape.ll.y;
        int64_t length = std::sqrt(dx * dx + dy * dy);
        int section_count = static_cast<int>(length / c_max_door_section_width) + 1;

        allocate_sections(section_count);

        std::vector<geometry::planar::FloatLine> result;
        for (int i = 0; i < section_count; ++i)
        {
            double t1 = static_cast<double>(i) / section_count;
            double t2 = static_cast<double>(i + 1) / section_count;

            geometry::planar::FloatPoint a(door_shape.ll.x + dx * t1, door_shape.ll.y + dy * t1);
            geometry::planar::FloatPoint b(door_shape.ll.x + dx * t2, door_shape.ll.y + dy * t2);

            result.emplace_back(a, b);
        }
        return result;
    }
    else
    {
        int section_count = 1;
        allocate_sections(section_count);

        geometry::planar::FloatPoint center = geometry::planar::FloatPoint((door_shape.ll.x + door_shape.ur.x) / 2.0,
                                         (door_shape.ll.y + door_shape.ur.y) / 2.0);

        std::vector<geometry::planar::FloatLine> result;
        result.emplace_back(center, center);
        return result;
    }
}

IncompleteExpansionRoom::IncompleteExpansionRoom(ExpansionRoom* p_source_room, int p_layer, const geometry::planar::IntBox& p_shape)
    : layer_(p_layer), room_id_(0), shape_(p_shape)
{
    (void)p_source_room;
}

void IncompleteExpansionRoom::add_door(ExpansionDoor* p_door)
{
    if (p_door)
    {
        doors_.push_back(p_door);
    }
}

std::vector<ExpansionDoor*> IncompleteExpansionRoom::get_doors() const
{
    return doors_;
}

void IncompleteExpansionRoom::clear_doors()
{
    doors_.clear();
}

void IncompleteExpansionRoom::reset_doors()
{
    for (auto* door : doors_)
    {
        if (door)
        {
            door->reset();
        }
    }
}

bool IncompleteExpansionRoom::door_exists(ExpansionRoom* p_other) const
{
    for (auto* door : doors_)
    {
        if (door)
        {
            if (door->other_room(p_other) != nullptr)
            {
                return true;
            }
        }
    }
    return false;
}

bool IncompleteExpansionRoom::remove_door(ExpandableObject* p_door)
{
    auto it = std::find(doors_.begin(), doors_.end(), static_cast<ExpansionDoor*>(p_door));
    if (it != doors_.end())
    {
        doors_.erase(it);
        return true;
    }
    return false;
}

IncompleteFreeSpaceExpansionRoom::IncompleteFreeSpaceExpansionRoom(ExpansionRoom* p_source_room, int p_layer, const geometry::planar::IntBox& p_shape)
    : IncompleteExpansionRoom(p_source_room, p_layer, p_shape), shape_tile_(p_shape.to_Simplex()),
      contained_shape_tile_(p_shape.to_Simplex())
{
}

IncompleteFreeSpaceExpansionRoom::IncompleteFreeSpaceExpansionRoom(ExpansionRoom* p_source_room, int p_layer,
    const geometry::planar::Simplex& p_shape, const geometry::planar::Simplex& p_contained_shape)
    : IncompleteExpansionRoom(p_source_room, p_layer, p_shape.bounding_box()), shape_tile_(p_shape),
      contained_shape_tile_(p_contained_shape)
{
}

IncompleteFreeSpaceExpansionRoom::IncompleteFreeSpaceExpansionRoom(ExpansionRoom* p_source_room, int p_layer,
    WholePlaneShapeTag, const geometry::planar::Simplex& p_contained_shape)
    : IncompleteExpansionRoom(p_source_room, p_layer, geometry::planar::IntBox::EMPTY),
      shape_tile_(geometry::planar::Simplex::EMPTY), contained_shape_tile_(p_contained_shape), whole_plane_(true)
{
}

geometry::planar::IntBox IncompleteFreeSpaceExpansionRoom::get_shape() const
{
    return shape_tile_.bounding_box();
}

TargetItemExpansionDoor::TargetItemExpansionDoor(CompleteExpansionRoom* p_room, board::Item* p_item, int p_tree_entry_no,
    board::ShapeSearchTree* p_tree)
    : ExpansionDoor(p_room, nullptr, 2), room_(p_room)
{
    item = p_item;
    tree_entry_no = p_tree_entry_no;
    if (p_item != nullptr && p_tree != nullptr && p_room != nullptr)
    {
        const geometry::planar::TileShape* item_shape = p_item->get_tree_shape(*p_tree, p_tree_entry_no);
        if (item_shape != nullptr)
        {
            shape_ = p_room->get_shape().intersection(item_shape->bounding_box());
        }
    }
    // A target door has exactly one maze search element, which the Java
    // TargetItemExpansionDoor creates in its constructor. The C++ door stores
    // the maze search elements of a plain ExpansionDoor in the section array
    // of the base class, so the target door overrides the accessors below and
    // uses its own element instead.
}

MazeSearchElement* TargetItemExpansionDoor::get_maze_search_element(int p_no)
{
    // The Java TargetItemExpansionDoor.getMazeSearchElement() ignores the
    // index. Only the section number 0 is a valid section number for a target
    // door, so any other index is rejected instead of silently returning the
    // same element.
    if (p_no != 0)
    {
        return nullptr;
    }
    return &maze_search_info_;
}

void TargetItemExpansionDoor::reset()
{
    maze_search_info_.reset();
}

geometry::planar::IntBox TargetItemExpansionDoor::get_shape() const
{
    return shape_;
}

int64_t TargetItemExpansionDoor::get_id() const
{
    // Port of Java TargetItemExpansionDoor.getId(): hash of target item id and
    // room id.
    const int64_t item_id = item != nullptr ? item->get_id() : 0;
    const int room_id = room_ != nullptr ? room_->get_room_id() : 0;
    return 31 * item_id + room_id;
}

bool TargetItemExpansionDoor::is_destination_door() const
{
    if (item == nullptr)
    {
        return false;
    }
    auto info = item->get_autoroute_info();
    return !info->is_start_info();
}

} // namespace freerouting::autoroute