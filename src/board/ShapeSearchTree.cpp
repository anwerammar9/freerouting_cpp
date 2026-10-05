#include "board/ShapeSearchTree.h"
#include "board/BasicBoard.h"
#include "board/Item.h"
#include "board/SearchTreeObject.h"
#include "geometry/planar/FortyfiveDegreeBoundingDirections.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/LineSegment.h"
#include "geometry/planar/OrthogonalBoundingDirections.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/RegularTileShape.h"
#include "geometry/planar/Simplex.h"
#include "logger/FRLogger.h"
#include "rules/ClearanceMatrix.h"

#include <algorithm>
#include <climits>
#include <cmath>

namespace freerouting::board {
namespace fp = freerouting::geometry::planar;

namespace {

/**
 * Used to sort the tree entries found by the clearance overlap search by
 * ascending clearance. Port of the Java EntrySortedByClearance.
 */
struct EntrySortedByClearance
{
    const ::freerouting::datastructures::ShapeTree::Leaf* leaf = nullptr;
    int clearance = 0;
    int entry_id_no = 0;
};

struct EntrySortedByClearanceLess
{
    bool operator()(const EntrySortedByClearance& p_a, const EntrySortedByClearance& p_b) const
    {
        if (p_a.clearance != p_b.clearance)
        {
            return p_a.clearance < p_b.clearance;
        }
        return p_a.entry_id_no < p_b.entry_id_no;
    }
};

int next_entry_id_no()
{
    static int last_generated_id_no = 0;
    if (last_generated_id_no == INT_MAX)
    {
        last_generated_id_no = 0;
    }
    else
    {
        ++last_generated_id_no;
    }
    return last_generated_id_no;
}

} // namespace

ShapeSearchTree::ShapeSearchTree(BasicBoard* p_board, int p_compensated_clearance_class_no)
    : ShapeSearchTree(fp::FortyfiveDegreeBoundingDirections::INSTANCE, p_board, p_compensated_clearance_class_no)
{
}

ShapeSearchTree::ShapeSearchTree(const fp::ShapeBoundingDirections& p_directions, BasicBoard* p_board,
    int p_compensated_clearance_class_no)
    : ::freerouting::datastructures::MinAreaTree(p_directions), board_(p_board), compensated_clearance_class_no_(p_compensated_clearance_class_no)
{
}

void ShapeSearchTree::insert(Item* p_item)
{
    if (!p_item)
    {
        return;
    }
    ::freerouting::datastructures::ShapeTree::insert(*p_item);
}

void ShapeSearchTree::insert(Item* p_item, int p_shape_index)
{
    if (!p_item)
    {
        return;
    }
    std::unique_ptr<ShapeTree::Leaf> new_leaf = ::freerouting::datastructures::ShapeTree::insert(*p_item, p_shape_index);
    if (!new_leaf)
    {
        return;
    }
    std::vector<std::unique_ptr<ShapeTree::Leaf>> entries = p_item->take_search_tree_entries(*this);
    entries.push_back(std::move(new_leaf));
    p_item->set_search_tree_entries(std::move(entries), *this);
}

void ShapeSearchTree::insert(SearchTreeObject& p_object)
{
    ::freerouting::datastructures::ShapeTree::insert(p_object);
}

void ShapeSearchTree::remove(Item* p_item)
{
    if (!p_item)
    {
        return;
    }
    std::vector<ShapeTree::Leaf*> entries = p_item->get_search_tree_entries(*this);
    ::freerouting::datastructures::ShapeTree::remove(entries);
}

std::set<ShapeSearchTree::Leaf*, ShapeSearchTree::LeafLess> ShapeSearchTree::overlaps(const fp::IntBox& p_box)
{
    fp::IntOctagon octagon = p_box.bounding_octagon();
    return overlaps(octagon);
}

std::set<ShapeSearchTree::Leaf*, ShapeSearchTree::LeafLess> ShapeSearchTree::overlaps(const fp::IntOctagon& p_oct)
{
    return ::freerouting::datastructures::MinAreaTree::overlaps(p_oct);
}

void ShapeSearchTree::overlapping_tree_entries(const fp::TileShape& p_shape, int p_layer,
    std::vector<TreeEntry>& p_tree_entries)
{
    overlapping_tree_entries(p_shape, p_layer, std::vector<int>{}, p_tree_entries);
}

void ShapeSearchTree::overlapping_tree_entries(const fp::TileShape& p_shape, int p_layer,
    const std::vector<int>& p_ignore_net_nos, std::vector<TreeEntry>& p_tree_entries)
{
    if (p_shape.is_empty())
    {
        return;
    }
    fp::IntOctagon bounds = p_shape.bounding_shape(bounding_directions);
    // Zero-size (point) query shapes yield a degenerate but bounded octagon;
    // only reject truly unbounded shapes like Java does.
    if (!p_shape.is_bounded())
    {
        logger::FRLogger::instance().warn("ShapeSearchTree.overlapping_tree_entries: p_shape not bounded");
        return;
    }
    std::set<Leaf*, LeafLess> tmp_list = overlaps(bounds);
    bool is_45_degree = dynamic_cast<const fp::IntOctagon*>(&p_shape) != nullptr;

    for (Leaf* curr_leaf : tmp_list)
    {
        if (curr_leaf == nullptr || curr_leaf->object == nullptr)
        {
            continue;
        }
        SearchTreeObject* curr_object = static_cast<SearchTreeObject*>(curr_leaf->object);
        int shape_index = curr_leaf->shape_index_in_object;
        bool ignore_object = p_layer >= 0 && curr_object->shape_layer(shape_index) != p_layer;
        if (!ignore_object)
        {
            for (int curr_net_no : p_ignore_net_nos)
            {
                if (!curr_object->is_obstacle(curr_net_no))
                {
                    ignore_object = true;
                    break;
                }
            }
        }
        if (!ignore_object)
        {
            const fp::TileShape* curr_shape = curr_object->get_tree_shape(*this, shape_index);
            if (curr_shape == nullptr)
            {
                continue;
            }
            bool add_item;
            if (is_45_degree && dynamic_cast<const fp::IntOctagon*>(curr_shape) != nullptr)
            {
                // The check for intersection is redundant and therefore skipped
                // for performance reasons.
                add_item = true;
            }
            else
            {
                add_item = shapes_intersect(*curr_shape, p_shape);
            }
            if (add_item)
            {
                p_tree_entries.emplace_back(curr_leaf->object, shape_index);
            }
        }
    }
}

std::vector<ShapeSearchTree::TreeEntry> ShapeSearchTree::overlapping_tree_entries(const fp::TileShape& p_shape, int p_layer)
{
    std::vector<TreeEntry> result;
    overlapping_tree_entries(p_shape, p_layer, std::vector<int>{}, result);
    return result;
}

void ShapeSearchTree::overlapping_objects(const fp::TileShape& p_shape, int p_layer,
    const std::vector<int>& p_ignore_net_nos, std::set<SearchTreeObject*, TreeStorableLess>& p_obstacles)
{
    std::vector<TreeEntry> tree_entries;
    overlapping_tree_entries(p_shape, p_layer, p_ignore_net_nos, tree_entries);
    for (const TreeEntry& curr_entry : tree_entries)
    {
        if (curr_entry.object != nullptr)
        {
            p_obstacles.insert(static_cast<SearchTreeObject*>(curr_entry.object));
        }
    }
}

std::set<SearchTreeObject*, TreeStorableLess> ShapeSearchTree::overlapping_objects(const fp::TileShape& p_shape, int p_layer)
{
    std::set<SearchTreeObject*, TreeStorableLess> result;
    overlapping_objects(p_shape, p_layer, std::vector<int>{}, result);
    return result;
}

void ShapeSearchTree::overlapping_tree_entries_with_clearance_exact(const fp::TileShape& p_shape, int p_layer,
    const std::vector<int>& p_ignore_net_nos, int p_cl_type, std::vector<TreeEntry>& p_obstacle_entries)
{
    if (p_shape.is_empty())
    {
        return;
    }
    if (board_ == nullptr)
    {
        logger::FRLogger::instance().warn("ShapeSearchTree.overlapping_tree_entries_with_clearance: board is null");
        return;
    }
    const rules::ClearanceMatrix& cl_matrix = board_->get_rules().get_clearance_matrix();
    fp::IntOctagon bounds = p_shape.bounding_shape(bounding_directions);
    if (!p_shape.is_bounded())
    {
        logger::FRLogger::instance().warn("ShapeSearchTree.overlapping_tree_entries_with_clearance: p_shape is not bounded");
        return;
    }
    // the maximum on the layer across all clearance classes is a safe upper
    // bound for the per-class maximum used in Java
    int max_clearance = static_cast<int>(1.2 * cl_matrix.get_max_value_on_layer(p_layer));
    // search with the bounds enlarged by the maximum clearance to get all
    // candidates for overlap
    fp::IntOctagon offset_bounds = bounds.offset(max_clearance);
    std::set<Leaf*, LeafLess> tmp_list = overlaps(offset_bounds);
    // sort the found items by their clearance to p_cl_type on layer p_layer
    std::set<EntrySortedByClearance, EntrySortedByClearanceLess> sorted_items;

    for (Leaf* curr_leaf : tmp_list)
    {
        if (curr_leaf == nullptr || curr_leaf->object == nullptr)
        {
            continue;
        }
        SearchTreeObject* curr_object = static_cast<SearchTreeObject*>(curr_leaf->object);
        int shape_index = curr_leaf->shape_index_in_object;
        bool ignore_item = p_layer >= 0 && curr_object->shape_layer(shape_index) != p_layer;
        if (!ignore_item)
        {
            for (int curr_net_no : p_ignore_net_nos)
            {
                if (!curr_object->is_obstacle(curr_net_no))
                {
                    ignore_item = true;
                    break;
                }
            }
        }
        if (!ignore_item)
        {
            const Item* curr_item = dynamic_cast<const Item*>(curr_object);
            int curr_clearance = 0;
            if (curr_item != nullptr)
            {
                curr_clearance = cl_matrix.get_value(p_cl_type, curr_item->get_clearance_class(), p_layer, true);
            }
            sorted_items.insert(EntrySortedByClearance{curr_leaf, curr_clearance, next_entry_id_no()});
        }
    }
    int curr_half_clearance = 0;
    std::unique_ptr<fp::TileShape> curr_offset_shape;
    for (const EntrySortedByClearance& tmp_entry : sorted_items)
    {
        int tmp_half_clearance = tmp_entry.clearance / 2;
        if (tmp_half_clearance != curr_half_clearance)
        {
            curr_half_clearance = tmp_half_clearance;
            curr_offset_shape = enlarge_tile(p_shape, curr_half_clearance);
        }
        if (!curr_offset_shape)
        {
            continue;
        }
        const fp::TileShape* tmp_shape = tmp_entry.leaf->object->get_tree_shape(*this, tmp_entry.leaf->shape_index_in_object);
        if (tmp_shape == nullptr)
        {
            continue;
        }
        std::unique_ptr<fp::TileShape> tmp_offset_shape = enlarge_tile(*tmp_shape, curr_half_clearance);
        if (!tmp_offset_shape)
        {
            continue;
        }
        if (shapes_intersect(*curr_offset_shape, *tmp_offset_shape))
        {
            p_obstacle_entries.emplace_back(tmp_entry.leaf->object, tmp_entry.leaf->shape_index_in_object);
        }
    }
}

std::vector<ShapeSearchTree::TreeEntry> ShapeSearchTree::overlapping_tree_entries_with_clearance(
    const fp::TileShape& p_shape, int p_layer, const std::vector<int>& p_ignore_net_nos, int p_cl_type)
{
    std::vector<TreeEntry> result;
    if (is_clearance_compensation_used())
    {
        overlapping_tree_entries(p_shape, p_layer, p_ignore_net_nos, result);
    }
    else
    {
        overlapping_tree_entries_with_clearance_exact(p_shape, p_layer, p_ignore_net_nos, p_cl_type, result);
    }
    return result;
}

void ShapeSearchTree::overlapping_objects_with_clearance(const fp::TileShape& p_shape, int p_layer,
    const std::vector<int>& p_ignore_net_nos, int p_cl_type, std::set<SearchTreeObject*, TreeStorableLess>& p_obstacles)
{
    std::vector<TreeEntry> tree_entries;
    if (is_clearance_compensation_used())
    {
        overlapping_tree_entries(p_shape, p_layer, p_ignore_net_nos, tree_entries);
    }
    else
    {
        overlapping_tree_entries_with_clearance_exact(p_shape, p_layer, p_ignore_net_nos, p_cl_type, tree_entries);
    }
    for (const TreeEntry& curr_entry : tree_entries)
    {
        if (curr_entry.object != nullptr)
        {
            p_obstacles.insert(static_cast<SearchTreeObject*>(curr_entry.object));
        }
    }
}

std::set<Item*, TreeStorableLess> ShapeSearchTree::overlapping_items_with_clearance(const fp::TileShape& p_shape,
    int p_layer, const std::vector<int>& p_ignore_net_nos, int p_clearance_class)
{
    std::set<SearchTreeObject*, TreeStorableLess> overlaps;
    overlapping_objects_with_clearance(p_shape, p_layer, p_ignore_net_nos, p_clearance_class, overlaps);
    std::set<Item*, TreeStorableLess> result;
    for (SearchTreeObject* curr_object : overlaps)
    {
        if (Item* curr_item = dynamic_cast<Item*>(curr_object))
        {
            result.insert(curr_item);
        }
    }
    return result;
}

bool ShapeSearchTree::shapes_intersect(const fp::TileShape& p_a, const fp::TileShape& p_b)
{
    const auto* a_regular = dynamic_cast<const fp::RegularTileShape*>(&p_a);
    const auto* b_regular = dynamic_cast<const fp::RegularTileShape*>(&p_b);
    if (a_regular != nullptr && b_regular != nullptr)
    {
        return a_regular->intersects(*b_regular);
    }
    // conservative fallback for non-regular shapes (e.g. Simplex): exact
    // intersection is not yet ported for the tile hierarchy
    fp::IntOctagon a_bounds = p_a.bounding_shape(fp::FortyfiveDegreeBoundingDirections::INSTANCE);
    fp::IntOctagon b_bounds = p_b.bounding_shape(fp::FortyfiveDegreeBoundingDirections::INSTANCE);
    return a_bounds.intersects(b_bounds);
}

std::unique_ptr<fp::TileShape> ShapeSearchTree::enlarge_tile(const fp::TileShape& p_shape, int p_offset)
{
    if (const auto* octagon = dynamic_cast<const fp::IntOctagon*>(&p_shape))
    {
        return std::make_unique<fp::IntOctagon>(octagon->enlarge(p_offset));
    }
    if (const auto* box = dynamic_cast<const fp::IntBox*>(&p_shape))
    {
        return std::make_unique<fp::IntOctagon>(box->enlarge(p_offset));
    }
    if (const auto* simplex = dynamic_cast<const fp::Simplex*>(&p_shape))
    {
        return std::make_unique<fp::Simplex>(simplex->enlarge(p_offset));
    }
    logger::FRLogger::instance().warn("ShapeSearchTree.enlarge_tile: unsupported tile shape type");
    return nullptr;
}

std::unique_ptr<fp::TileShape> ShapeSearchTree::intersection_of_two_shapes(
    const fp::TileShape& p_first, const fp::TileShape& p_second)
{
    return fp::TileShape::intersection_of(p_first, p_second);
}

bool ShapeSearchTree::shape_contains(const fp::TileShape& p_outer, const fp::TileShape& p_inner)
{
    const auto* outer_regular = dynamic_cast<const fp::RegularTileShape*>(&p_outer);
    const auto* inner_regular = dynamic_cast<const fp::RegularTileShape*>(&p_inner);
    if (outer_regular == nullptr || inner_regular == nullptr)
    {
        // Conservative: expansion rooms are only ignored when the containment
        // is exact. For arbitrary shapes (e.g. Simplex) containment is not
        // verified here.
        return false;
    }
    return outer_regular->contains(*inner_regular);
}

void ShapeSearchTree::restrain_shape(const CompleteShapePiece& p_room,
    const fp::TileShape& p_obstacle_shape, std::vector<CompleteShapePiece>& p_result)
{
    // Port of Java ShapeSearchTree.restrainShape(...). The obstacle is always
    // converted to a Simplex to match the Java semantics: "otherwise border_lines
    // of length 0 for octagons may not be handled correctly".
    fp::Simplex obstacle_simplex = p_obstacle_shape.to_Simplex();

    const fp::TileShape* shape_to_be_contained = p_room.contained_shape.get();
    if (shape_to_be_contained == nullptr || shape_to_be_contained->is_empty())
    {
        return;
    }
    const fp::TileShape* room_shape = p_room.shape.get();
    if (room_shape == nullptr)
    {
        return;
    }

    // Search the edge line of obstacle_simplex, so that shape_to_be_contained
    // are on the right side of this line, and that the line segment intersects
    // with the interior of the room shape. If there are more than 1 such lines
    // take the line which is furthest away from shape_to_be_contained. Then
    // intersect the room shape with the halfplane defined by the opposite of
    // this line.
    std::optional<fp::Line> cut_line;
    double cut_line_distance = -1;
    for (int i = 0; i < obstacle_simplex.border_line_count(); ++i)
    {
        fp::LineSegment current_line_segment(obstacle_simplex, i);
        if (room_shape->is_intersected_interior_by(current_line_segment))
        {
            fp::Line current_line = obstacle_simplex.border_line(i);
            double current_min_distance = shape_to_be_contained->distance_to_the_left(current_line);
            if (current_min_distance > cut_line_distance)
            {
                cut_line_distance = current_min_distance;
                cut_line = current_line.opposite();
            }
        }
    }

    if (cut_line.has_value())
    {
        std::unique_ptr<fp::TileShape> result_piece = fp::TileShape::get_instance(*cut_line);
        result_piece = intersection_of_two_shapes(*result_piece, *room_shape);
        if (result_piece->dimension() >= 2)
        {
            CompleteShapePiece new_piece;
            new_piece.shape = std::move(result_piece);
            new_piece.contained_shape = shape_to_be_contained->clone();
            p_result.push_back(std::move(new_piece));
        }
    }
    else
    {
        // There is no cut line, so that all of shape_to_be_contained is
        // completely on the right side of that line. Search a cut line, so
        // that at least part of shape_to_be_contained is on the right side.
        if (shape_to_be_contained->dimension() < 1)
        {
            // There is already a completed expansion room around shape_to_be_contained.
            return;
        }
        for (int i = 0; i < obstacle_simplex.border_line_count(); ++i)
        {
            fp::LineSegment current_line_segment(obstacle_simplex, i);
            if (room_shape->is_intersected_interior_by(current_line_segment))
            {
                fp::Line current_line = obstacle_simplex.border_line(i);
                if (shape_to_be_contained->side_of(current_line) == fp::Side::COLLINEAR)
                {
                    // current_line intersects with the interior of shape_to_be_contained
                    cut_line = current_line.opposite();
                    break;
                }
            }
        }
        if (!cut_line.has_value())
        {
            // cut line not found, parts or the whole of the shape may be
            // already occupied from somewhere else.
            return;
        }
        // Calculate the new shape to be contained in the result shape.
        std::unique_ptr<fp::TileShape> cut_half_plane = fp::TileShape::get_instance(*cut_line);
        std::unique_ptr<fp::TileShape> new_shape_to_be_contained =
            intersection_of_two_shapes(*shape_to_be_contained, *cut_half_plane);

        std::unique_ptr<fp::TileShape> result_piece = intersection_of_two_shapes(*room_shape, *cut_half_plane);
        if (result_piece->dimension() >= 2)
        {
            CompleteShapePiece new_piece;
            new_piece.shape = std::move(result_piece);
            new_piece.contained_shape = std::move(new_shape_to_be_contained);
            p_result.push_back(std::move(new_piece));
        }
        std::unique_ptr<fp::TileShape> opposite_half_plane = fp::TileShape::get_instance(cut_line->opposite());
        std::unique_ptr<fp::TileShape> rest_piece = intersection_of_two_shapes(*room_shape, *opposite_half_plane);
        if (rest_piece->dimension() >= 2)
        {
            std::unique_ptr<fp::TileShape> rest_shape_to_be_contained =
                intersection_of_two_shapes(*shape_to_be_contained, *opposite_half_plane);
            CompleteShapePiece rest_piece_struct;
            rest_piece_struct.shape = std::move(rest_piece);
            rest_piece_struct.contained_shape = std::move(rest_shape_to_be_contained);
            restrain_shape(rest_piece_struct, p_obstacle_shape, p_result);
        }
    }
}

void ShapeSearchTree::divide_large_room(const fp::IntBox& p_board_box, std::vector<CompleteShapePiece>& p_rooms)
{
    // Port of Java ShapeSearchTree.divideLargeRoom(...). Makes sure that on
    // each layer there will be more than 1 free space room, even if there are
    // no objects on the layer.
    if (p_rooms.size() != 1)
    {
        return;
    }
    CompleteShapePiece& current_room = p_rooms[0];
    fp::IntBox room_bounding_box = current_room.shape->bounding_box();
    if (2 * room_bounding_box.height() <= p_board_box.height()
        || 2 * room_bounding_box.width() <= p_board_box.width())
    {
        return;
    }
    double max_section_width = 0.5 * std::max(p_board_box.height(), p_board_box.width());
    std::vector<fp::IntBox> section_boxes = room_bounding_box.divide_into_sections(max_section_width);
    std::vector<CompleteShapePiece> result;
    result.reserve(section_boxes.size());
    for (const fp::IntBox& current_section_box : section_boxes)
    {
        CompleteShapePiece current_section;
        current_section.shape =
            intersection_of_two_shapes(*current_room.shape, current_section_box);
        if (current_section.shape->dimension() != 2)
        {
            continue;
        }
        current_section.contained_shape =
            intersection_of_two_shapes(*current_room.contained_shape, current_section_box);
        result.push_back(std::move(current_section));
    }
    p_rooms = std::move(result);
}

std::vector<ShapeSearchTree::CompleteShapePiece> ShapeSearchTree::complete_shape(
    const CompleteShapeInput& p_input)
{
    // Port of Java ShapeSearchTree.completeShapeUnlocked(...).
    std::vector<CompleteShapePiece> result;
    if (p_input.contained_shape == nullptr)
    {
        logger::FRLogger::instance().warn("ShapeSearchTree.complete_shape: contained shape expected");
        return result;
    }
    // The room to be completed starts as the whole board box (Java
    // shapeToBeContained = board.getBoundingBox()), intersected with the input
    // room shape if present.
    const fp::IntBox board_box = board_ != nullptr ? board_->get_bounding_box() : fp::IntBox::EMPTY;
    std::unique_ptr<fp::TileShape> start_shape = std::make_unique<fp::IntBox>(board_box);
    if (p_input.shape != nullptr)
    {
        start_shape = intersection_of_two_shapes(*start_shape, *p_input.shape);
    }
    fp::IntOctagon bounding_shape = start_shape->bounding_shape(bounding_directions);
    if (start_shape->dimension() == 2)
    {
        CompleteShapePiece new_piece;
        new_piece.shape = start_shape->clone();
        new_piece.contained_shape = p_input.contained_shape->clone();
        result.push_back(std::move(new_piece));
    }

    // Deviation from Java: Java returns an empty result when the root is null.
    // Here the whole board is used instead, so that a room is always completed
    // on an empty tree. This matches the expected drill behaviour for boards
    // without obstacles.
    std::vector<Leaf*> overlapping_leaves;
    if (root != nullptr)
    {
        std::vector<TreeNode*> stack;
        stack.reserve(64);
        stack.push_back(root);
        while (!stack.empty())
        {
            TreeNode* current_node = stack.back();
            stack.pop_back();
            if (current_node->bounding_shape != nullptr
                && current_node->bounding_shape->intersects(bounding_shape))
            {
                if (auto* leaf = dynamic_cast<Leaf*>(current_node))
                {
                    overlapping_leaves.push_back(leaf);
                }
                else if (auto* inner = dynamic_cast<InnerNode*>(current_node))
                {
                    stack.push_back(inner->first_child);
                    stack.push_back(inner->second_child);
                }
            }
        }
        // Sort the obstacles by the tree order of their leaves to get a
        // deterministic room partitioning (mirrors the Java Collections.sort).
        std::sort(overlapping_leaves.begin(), overlapping_leaves.end(),
            [](const Leaf* p_a, const Leaf* p_b)
            {
                return p_a->compare_to(*p_b) < 0;
            });
    }

    for (Leaf* current_leaf : overlapping_leaves)
    {
        if (current_leaf == nullptr || current_leaf->object == nullptr)
        {
            continue;
        }
        SearchTreeObject* current_object = static_cast<SearchTreeObject*>(current_leaf->object);
        int shape_index = current_leaf->shape_index_in_object;
        if (current_object->is_trace_obstacle(p_input.net_no)
            && current_object->shape_layer(shape_index) == p_input.layer
            && current_object != p_input.ignore_object)
        {
            const fp::TileShape* current_object_shape =
                current_object->get_tree_shape(*this, shape_index);
            if (current_object_shape == nullptr)
            {
                continue;
            }
            std::vector<CompleteShapePiece> new_result;
            fp::IntOctagon new_bounding_shape = fp::IntOctagon::EMPTY;
            for (const CompleteShapePiece& current_piece : result)
            {
                bool something_changed = false;
                std::unique_ptr<fp::TileShape> intersection =
                    intersection_of_two_shapes(*current_piece.shape, *current_object_shape);
                if (intersection->dimension() == 2)
                {
                    bool ignore_expansion_room =
                        current_object->is_free_space_room() && p_input.ignore_shape != nullptr
                        && shape_contains(*p_input.ignore_shape, *intersection);
                    if (!ignore_expansion_room)
                    {
                        something_changed = true;
                        restrain_shape(current_piece, *current_object_shape, new_result);
                        // The bounding shape must include all accumulated rooms.
                        for (const CompleteShapePiece& tmp_piece : new_result)
                        {
                            new_bounding_shape = new_bounding_shape.union_with(
                                tmp_piece.shape->bounding_shape(bounding_directions));
                        }
                    }
                }
                if (!something_changed)
                {
                    CompleteShapePiece clone_piece;
                    clone_piece.shape = current_piece.shape->clone();
                    clone_piece.contained_shape = current_piece.contained_shape->clone();
                    new_result.push_back(std::move(clone_piece));
                    new_bounding_shape = new_bounding_shape.union_with(
                        current_piece.shape->bounding_shape(bounding_directions));
                }
            }
            result = std::move(new_result);
            bounding_shape = new_bounding_shape;
        }
    }
    divide_large_room(board_box, result);
    return result;
}

std::vector<Item*> ShapeSearchTree::find_nearby(const fp::IntBox& p_box, int p_layer, int p_from_x, int p_from_y)
{
    (void)p_box;
    (void)p_layer;
    std::vector<Item*> result;

    // Search in expanding radius
    int search_radius = 5000;
    int step = 1000;

    for (int r = step; r <= search_radius; r += step)
    {
        fp::IntBox search_box(fp::IntPoint(p_from_x - r, p_from_y - r), fp::IntPoint(p_from_x + r, p_from_y + r));

        std::set<Leaf*, LeafLess> overlaps = this->overlaps(search_box);

        for (Leaf* leaf : overlaps)
        {
            if (leaf && leaf->object)
            {
                result.push_back(static_cast<Item*>(leaf->object));
            }
        }

        // Return items if we found any
        if (!result.empty())
        {
            break;
        }
    }

    return result;
}

int ShapeSearchTree::clearance_compensation_value(int p_clearance_class_no, int p_layer) const
{
    if (p_clearance_class_no <= 0 || !is_clearance_compensation_used() || board_ == nullptr)
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
    // Removes all leaves generically. This also covers non-Item storables
    // (e.g. complete expansion rooms), which would be an invalid
    // static_cast to Item otherwise.
    std::vector<ShapeTree::Leaf*> leaves = this->to_array();
    ::freerouting::datastructures::ShapeTree::remove(leaves);
}

bool ShapeSearchTree::validate_entries(Item* p_item) const
{
    if (p_item == nullptr)
    {
        return true;
    }
    std::vector<ShapeTree::Leaf*> entries = p_item->get_search_tree_entries(const_cast<ShapeSearchTree&>(*this));
    for (int i = 0; i < static_cast<int>(entries.size()); ++i)
    {
        if (entries[i] == nullptr || entries[i]->shape_index_in_object != i)
        {
            logger::FRLogger::instance().warn("ShapeSearchTree.validate_entries: entry " + std::to_string(i) + " invalid");
            return false;
        }
    }
    return true;
}

std::string ShapeSearchTree::to_string() const
{
    return "ShapeSearchTree" + std::string("_") + std::to_string(compensated_clearance_class_no_);
}

ShapeSearchTree90Degree::ShapeSearchTree90Degree(BasicBoard* p_board, int p_compensated_clearance_class_no)
    : ShapeSearchTree(fp::OrthogonalBoundingDirections::INSTANCE, p_board, p_compensated_clearance_class_no)
{
}

ShapeSearchTree45Degree::ShapeSearchTree45Degree(BasicBoard* p_board, int p_compensated_clearance_class_no)
    : ShapeSearchTree(fp::FortyfiveDegreeBoundingDirections::INSTANCE, p_board, p_compensated_clearance_class_no)
{
}

} // namespace freerouting::board