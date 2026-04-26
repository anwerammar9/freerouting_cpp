#pragma once

#include <memory>
#include <vector>
#include <set>
#include <map>
#include <string>

#include "geometry/planar/IntBox.h"

namespace freerouting::board {
class Item;
class BasicBoard;
}

namespace freerouting::geometry::planar {
class TileShape;
class RegularTileShape;
}

namespace freerouting::board {

struct ShapeSearchTreeEntry
{
    Item* object = nullptr;
    int shape_index_in_object = 0;
    ::freerouting::geometry::planar::IntOctagon bounding_octagon;

    ShapeSearchTreeEntry() = default;
    ShapeSearchTreeEntry(Item* p_object, int p_shape_index, const ::freerouting::geometry::planar::IntOctagon& p_bounding_octagon)
        : object(p_object), shape_index_in_object(p_shape_index), bounding_octagon(p_bounding_octagon)
    {
    }
};

class ShapeSearchTree
{
public:
    ShapeSearchTree(BasicBoard* p_board, int p_compensated_clearance_class_no);
    ~ShapeSearchTree();

    void insert(Item* p_item);
    void insert(Item* p_item, int p_shape_index);

    void remove(Item* p_item);

    std::vector<Item*> find_nearby(const ::freerouting::geometry::planar::IntBox& p_box, int p_layer, int p_from_x, int p_from_y) const;
    std::set<ShapeSearchTreeEntry*> overlaps(const ::freerouting::geometry::planar::IntBox& p_box) const;
    std::set<ShapeSearchTreeEntry*> overlaps(const ::freerouting::geometry::planar::IntOctagon& p_oct) const;

    int clearance_compensation_value(int p_clearance_class_no, int p_layer) const;

    int get_compensated_clearance_class_no() const { return compensated_clearance_class_no_; }

    bool is_clearance_compensation_used() const { return compensated_clearance_class_no_ > 0; }

    void clear();

    bool validate_entries(Item* p_item) const;

    std::string to_string() const;

private:
    BasicBoard* board_;
    int compensated_clearance_class_no_;
    std::vector<ShapeSearchTreeEntry> entries_;
    std::map<Item*, std::vector<ShapeSearchTreeEntry*>> item_entries_;
};

class ShapeSearchTree90Degree : public ShapeSearchTree
{
public:
    explicit ShapeSearchTree90Degree(BasicBoard* p_board, int p_compensated_clearance_class_no);
    ~ShapeSearchTree90Degree();
};

class ShapeSearchTree45Degree : public ShapeSearchTree
{
public:
    explicit ShapeSearchTree45Degree(BasicBoard* p_board, int p_compensated_clearance_class_no);
    ~ShapeSearchTree45Degree();
};

} // namespace freerouting::board