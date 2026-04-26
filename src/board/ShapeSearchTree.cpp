#include "board/ShapeSearchTree.h"
#include "board/BasicBoard.h"
#include "board/Item.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include "logger/FRLogger.h"

#include <algorithm>
#include <cmath>

namespace freerouting::board {
namespace fp = freerouting::geometry::planar;

ShapeSearchTree::ShapeSearchTree(BasicBoard* p_board, int p_compensated_clearance_class_no)
    : board_(p_board), compensated_clearance_class_no_(p_compensated_clearance_class_no)
{
}

ShapeSearchTree::~ShapeSearchTree() = default;

void ShapeSearchTree::insert(Item* p_item)
{
    if (!p_item)
        return;

    for (int i = 0; i < p_item->tile_shape_count(); ++i)
    {
        insert(p_item, i);
    }
}

void ShapeSearchTree::insert(Item* p_item, int p_shape_index)
{
    if (!p_item)
        return;

    auto octagon = p_item->get_tile_shape(p_shape_index);

    entries_.push_back(ShapeSearchTreeEntry(p_item, p_shape_index, octagon));
    ShapeSearchTreeEntry* entry_ptr = &entries_.back();

    auto it = item_entries_.find(p_item);
    if (it == item_entries_.end())
    {
        item_entries_[p_item] = {entry_ptr};
    }
    else
    {
        it->second.push_back(entry_ptr);
    }
}

void ShapeSearchTree::remove(Item* p_item)
{
    if (!p_item)
        return;

    auto it = item_entries_.find(p_item);
    if (it != item_entries_.end())
    {
        for (auto* entry_ptr : it->second)
        {
            entry_ptr->object = nullptr;
        }
        item_entries_.erase(it);
    }
}

std::set<ShapeSearchTreeEntry*> ShapeSearchTree::overlaps(const geometry::planar::IntBox& p_box) const
{
    std::set<ShapeSearchTreeEntry*> result;
    auto oct = p_box.bounding_octagon();
    return overlaps(oct);
}

std::set<ShapeSearchTreeEntry*> ShapeSearchTree::overlaps(const geometry::planar::IntOctagon& p_oct) const
{
    std::set<ShapeSearchTreeEntry*> result;

    for (auto& entry : entries_)
    {
        if (entry.object == nullptr)
            continue;

        const auto& entry_oct = entry.bounding_octagon;

        bool intersects = !(entry_oct.right < p_oct.left ||
                          entry_oct.left > p_oct.right ||
                          entry_oct.top < p_oct.bottom ||
                          entry_oct.bottom > p_oct.top ||
                          entry_oct.right_bottom < p_oct.left_top ||
                          entry_oct.left_top > p_oct.right_bottom ||
                          entry_oct.right_top < p_oct.left_bottom ||
                          entry_oct.left_bottom > p_oct.right_top);

        if (intersects)
        {
            result.insert(const_cast<ShapeSearchTreeEntry*>(&entry));
        }
    }
    return result;
}

std::vector<Item*> ShapeSearchTree::find_nearby(const fp::IntBox& p_box, int p_layer, int p_from_x, int p_from_y) const
{
    std::vector<Item*> result;
    
    // Search in expanding radius
    int search_radius = 5000;
    int step = 1000;
    
    for (int r = step; r <= search_radius; r += step)
    {
        fp::IntBox search_box(fp::IntPoint(p_from_x - r, p_from_y - r), fp::IntPoint(p_from_x + r, p_from_y + r));
        
        auto overlaps = this->overlaps(search_box);
        
        for (auto* entry : overlaps)
        {
            if (entry && entry->object)
            {
                result.push_back(entry->object);
            }
        }
        
        // Return items if we found any
        if (!result.empty())
            break;
    }
    
    return result;
}

int ShapeSearchTree::clearance_compensation_value(int p_clearance_class_no, int p_layer) const
{
    if (p_clearance_class_no <= 0 || !is_clearance_compensation_used())
    {
        return 0;
    }

    const auto& rules = board_->get_rules();
    int value = rules.get_clearance_matrix().get_value(p_clearance_class_no, compensated_clearance_class_no_, p_layer, false);
    int compensation = rules.get_clearance_matrix().get_clearance_compensation_value(compensated_clearance_class_no_, p_layer);

    return std::max(0, value - compensation);
}

void ShapeSearchTree::clear()
{
    entries_.clear();
    item_entries_.clear();
}

std::string ShapeSearchTree::to_string() const
{
    return "ShapeSearchTree_cc" + std::to_string(compensated_clearance_class_no_);
}

bool ShapeSearchTree::validate_entries(Item* p_item) const
{
    (void)p_item;
    return true;  // Stub implementation
}

ShapeSearchTree90Degree::ShapeSearchTree90Degree(BasicBoard* p_board, int p_compensated_clearance_class_no)
    : ShapeSearchTree(p_board, p_compensated_clearance_class_no)
{
}

ShapeSearchTree90Degree::~ShapeSearchTree90Degree() = default;

ShapeSearchTree45Degree::ShapeSearchTree45Degree(BasicBoard* p_board, int p_compensated_clearance_class_no)
    : ShapeSearchTree(p_board, p_compensated_clearance_class_no)
{
}

ShapeSearchTree45Degree::~ShapeSearchTree45Degree() = default;

} // namespace freerouting::board