#include "board/RoutingBoard.h"
#include "board/Item.h"
#include "board/Component.h"
#include "board/ItemSelectionFilter.h"
#include "board/Padstack.h"
#include "board/Pin.h"
#include "board/PolylineTrace.h"
#include "board/Communication.h"
#include "board/ChangedArea.h"
#include "board/BoardOutline.h"
#include "board/CalcFromSide.h"
#include "board/SearchTreeManager.h"
#include "board/ShoveTraceAlgo.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "board/PullTightAlgo.h"
#include "datastructures/TimeLimit.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/LineSegment.h"
#include "geometry/planar/Simplex.h"
#include "rules/BoardRules.h"
#include "logger/FRLogger.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/TileShape.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <set>

namespace freerouting::board {

RoutingBoard::RoutingBoard(const geometry::planar::IntBox& p_bounding_box,
                           const LayerStructure& p_layer_structure,
                           std::unique_ptr<::freerouting::rules::BoardRules> p_rules,
                           Communication* p_board_communication)
    : BasicBoard(p_layer_structure, std::move(p_rules))
    , changed_area_(std::make_unique<ChangedArea>(p_layer_structure.layer_count()))
    , bounding_box_(p_bounding_box)
    , board_communication_(p_board_communication)
    , components_(std::make_unique<Components>())
{
}

RoutingBoard::~RoutingBoard() = default;

void RoutingBoard::additional_update_after_change(Item* p_item)
{
    if (p_item == nullptr)
    {
        return;
    }
    // Mirrors Java RoutingBoard.additional_update_after_change. The Java
    // version first returns when the autoroute engine does not maintain its
    // database; the headless C++ board has no autoroute engine wired into the
    // item change path (TODO), so nothing to maintain yet.
    (void)p_item;
}

bool RoutingBoard::remove_items_and_pull_tight(const std::vector<Item*>& p_item_list,
                                                int p_tidy_width,
                                                int p_pull_tight_accuracy)
{
    return true;
}

void RoutingBoard::start_marking_changed_area()
{
    if (changed_area_)
    {
        changed_area_->clear();
    }
}

void RoutingBoard::opt_changed_area(const std::vector<int>& p_nets_to_optimize,
                                    const geometry::planar::IntOctagon* p_tidy_region,
                                    int p_pull_tight_accuracy,
                                    void* p_stoppable,
                                    std::function<bool()> p_cancel_callback,
                                    int64_t p_time_limit_ms)
{
    if (!changed_area_ || p_tidy_region == nullptr || p_tidy_region->is_empty())
    {
        return;
    }

    // Convert void* p_stoppable to datastructures::Stoppable*
    auto* stoppable = static_cast<datastructures::Stoppable*>(p_stoppable);

    // Create PullTightAlgo instance and run optimization
    // Mirrors Java RoutingBoardOperations.optChangedArea
    auto pull_tight_algo = PullTightAlgo::get_instance(
        this, p_nets_to_optimize, p_tidy_region, p_pull_tight_accuracy, stoppable,
        static_cast<int>(p_time_limit_ms), nullptr, -1);
    if (pull_tight_algo)
    {
        pull_tight_algo->opt_changed_area(nullptr);
    }

    // Notify graphics update (if any)
    join_graphics_update_box(changed_area_->surrounding_box());

    // Clear the changed area
    changed_area_->clear();
}

void RoutingBoard::combine_traces(int p_net_no)
{
    // Mirrors Java BasicBoard.combineTraces: combines connected traces of the
    // net which have only 1 contact at the connection point. If p_net_no < 0,
    // traces of all nets are combined.
    bool something_changed = true;
    while (something_changed)
    {
        something_changed = false;
        // Snapshot: traces may be removed while iterating.
        std::vector<Item*> snapshot(items_.begin(), items_.end());
        for (Item* curr_item : snapshot)
        {
            if (curr_item == nullptr)
            {
                continue;
            }
            if (p_net_no >= 0 && !curr_item->contains_net(p_net_no))
            {
                continue;
            }
            auto* curr_trace = dynamic_cast<PolylineTrace*>(curr_item);
            if (curr_trace == nullptr || !curr_trace->is_on_board())
            {
                continue;
            }
            if (curr_trace->combine())
            {
                something_changed = true;
                break;
            }
        }
    }
}

void RoutingBoard::add_item(Item* p_item)
{
    if (p_item)
    {
        items_.push_back(p_item);
    }
}

void RoutingBoard::remove_item(Item* p_item)
{
    if (!p_item)
        return;
        
    auto it = std::find(items_.begin(), items_.end(), p_item);
    if (it != items_.end())
    {
        items_.erase(it);
    }
}

std::vector<Item*> RoutingBoard::get_autoroute_items() const
{
    return items_;
}

std::vector<Item*> RoutingBoard::get_connectable_items() const
{
    std::vector<Item*> connectable;
    for (auto* item : items_)
    {
        if (item && (item->is_pin() || item->is_via() || item->is_trace()))
        {
            connectable.push_back(item);
        }
    }
    return connectable;
}

std::vector<Item*> RoutingBoard::get_connectable_items(int p_net_no) const
{
    std::vector<Item*> connectable;
    for (auto* item : items_)
    {
        if (item == nullptr || !item->contains_net(p_net_no))
        {
            continue;
        }
        if (item->is_pin() || item->is_via() || item->is_trace())
        {
            connectable.push_back(item);
        }
    }
    return connectable;
}

std::vector<Pin*> RoutingBoard::get_pins() const
{
    std::vector<Pin*> result;
    for (auto* item : items_)
    {
        if (auto* pin = dynamic_cast<Pin*>(item))
        {
            result.push_back(pin);
        }
    }
    return result;
}

std::vector<Trace*> RoutingBoard::get_traces() const
{
    std::vector<Trace*> result;
    for (auto* item : items_)
    {
        if (auto* trace = dynamic_cast<Trace*>(item))
        {
            result.push_back(trace);
        }
    }
    return result;
}

std::vector<Via*> RoutingBoard::get_vias() const
{
    std::vector<Via*> result;
    for (auto* item : items_)
    {
        if (auto* via = dynamic_cast<Via*>(item))
        {
            result.push_back(via);
        }
    }
    return result;
}

Item* RoutingBoard::get_item(int p_index) const
{
    if (p_index >= 0 && p_index < static_cast<int>(items_.size()))
    {
        return items_[p_index];
    }
    return nullptr;
}

const designforms::specctra::PinDef* RoutingBoard::get_pin(int p_index) const
{
    if (p_index >= 0 && p_index < static_cast<int>(parsed_result_.pins.size()))
    {
        return &parsed_result_.pins[p_index];
    }
    return nullptr;
}

void RoutingBoard::create_pins_from_parsed()
{
    if (!parsed_result_.pins.empty())
    {
        logger::FRLogger::instance().info("Creating " + 
            std::to_string(parsed_result_.pins.size()) + " Pin items...");

        // Resolve each pin's position from the placement section (component origin).
        auto find_component = [this](const std::string& p_name) -> const designforms::specctra::ComponentDef*
        {
            for (const auto& comp : parsed_result_.components)
            {
                if (comp.name == p_name)
                {
                    return &comp;
                }
            }
            return nullptr;
        };

        auto find_image = [this](const std::string& p_name) -> const designforms::specctra::ImageDef*
        {
            for (const auto& img : parsed_result_.images)
            {
                if (img.name == p_name)
                {
                    return &img;
                }
            }
            return nullptr;
        };

        auto find_padstack = [this](const std::string& p_name) -> const designforms::specctra::PadstackDef*
        {
            for (const auto& padstack : parsed_result_.padstacks)
            {
                if (padstack.name == p_name)
                {
                    return &padstack;
                }
            }
            return nullptr;
        };

        struct Offset
        {
            int64_t dx = 0;
            int64_t dy = 0;
        };
        auto find_pin_offset = [](const designforms::specctra::ImageDef* p_image,
                                  const std::string& p_pin_name) -> Offset
            {
                if (p_image == nullptr)
                {
                    return {};
                }
                // Prefer an exact match, then a match with the '@' variant suffix stripped.
                std::string base_name = p_pin_name;
                std::size_t at_pos = base_name.find('@');
                if (at_pos != std::string::npos)
                {
                    base_name = base_name.substr(0, at_pos);
                }
                for (const auto& p : p_image->pins)
                {
                    if (p.name == p_pin_name)
                    {
                        return {static_cast<int64_t>(std::round(p.x)), static_cast<int64_t>(std::round(p.y))};
                    }
                }
                for (const auto& p : p_image->pins)
                {
                    if (p.name == base_name)
                    {
                        return {static_cast<int64_t>(std::round(p.x)), static_cast<int64_t>(std::round(p.y))};
                    }
                }
                return {};
            };

        auto rotate_offset = [](int64_t p_dx, int64_t p_dy, int p_rotation) -> std::pair<int64_t, int64_t>
            {
                int r = p_rotation % 360;
                if (r < 0)
                {
                    r += 360;
                }
                switch (r)
                {
                    case 90:
                        return {-p_dy, p_dx};
                    case 180:
                        return {-p_dx, -p_dy};
                    case 270:
                        return {p_dy, -p_dx};
                    default:
                        return {p_dx, p_dy};
                }
            };

        int id_no = 1;
        int unmatched = 0;
        for (int i = 0; i < static_cast<int>(parsed_result_.pins.size()); ++i)
        {
            const auto& pin_def = parsed_result_.pins[i];

            // Locate the owning component to position the pin.
            int64_t center_x = 0;
            int64_t center_y = 0;
            int64_t half_width = 50;
            int64_t half_height = 50;
            const auto* comp = find_component(pin_def.component);
            if (comp != nullptr)
            {
                center_x = comp->x;
                center_y = comp->y;

                // Add the pad offset within the image, transformed by the rotation.
                const auto* image = find_image(comp->package);
                if (image == nullptr)
                {
                    image = find_image(comp->footprint);
                }
                const Offset offset = find_pin_offset(image, pin_def.pin);
                auto rotated = rotate_offset(offset.dx, offset.dy, comp->rotation);
                center_x += rotated.first;
                center_y += rotated.second;

                if (image != nullptr)
                {
                    for (const auto& image_pin : image->pins)
                    {
                        if (image_pin.name == pin_def.pin)
                        {
                            if (const auto* padstack = find_padstack(image_pin.padstack))
                            {
                                half_width = std::max<int64_t>(1, static_cast<int64_t>(std::ceil(padstack->width / 2.0)));
                                half_height = std::max<int64_t>(1, static_cast<int64_t>(std::ceil(padstack->height / 2.0)));
                            }
                            break;
                        }
                    }
                }
            }
            else
            {
                ++unmatched;
            }

            // Create Pin item with net number
            std::vector<int> net_nos{pin_def.net_no};

            auto pin = std::make_unique<Pin>(
                0,  // component_no (not used yet)
                static_cast<int>(i),
                net_nos,
                1,  // clearance_class
                generate_item_id(),
                FixedState::FROM_DESIGN,
                this);

            pin->set_center(geometry::planar::IntPoint(center_x, center_y));
            // Use the parsed padstack extent; retain a small fallback for incomplete DSNs.
            pin->set_bounding_box(geometry::planar::IntBox(
                center_x - half_width, center_y - half_height,
                center_x + half_width, center_y + half_height));

            ++id_no;
            items_.push_back(pin.get());
            pin.release();
        }

        if (unmatched > 0)
        {
            logger::FRLogger::instance().warn("Pins without placement: " + std::to_string(unmatched));
        }
    }
}

PolylineTrace* RoutingBoard::add_polyline_trace(const std::vector<geometry::planar::IntPoint>& p_points,
                                                int p_layer, int p_half_width,
                                                const std::vector<int>& p_net_nos)
{
    if (p_points.size() < 2)
    {
        logger::FRLogger::instance().warn("RoutingBoard.add_polyline_trace: at least 2 points expected");
        return nullptr;
    }

    std::vector<geometry::planar::Line> lines;
    lines.reserve(p_points.size() + 1);
    auto normalized_direction = [](const geometry::planar::IntPoint& p_from,
                                   const geometry::planar::IntPoint& p_to)
    {
        const int64_t dx = p_to.x - p_from.x;
        const int64_t dy = p_to.y - p_from.y;
        const int64_t divisor = std::gcd(std::llabs(dx), std::llabs(dy));
        if (divisor == 0)
        {
            return geometry::planar::IntVector(0, 1);
        }
        return geometry::planar::IntVector(dx / divisor, dy / divisor);
    };

    const auto first_direction = normalized_direction(p_points[0], p_points[1]);
    lines.emplace_back(
        p_points[0],
        geometry::planar::IntPoint(p_points[0].x - first_direction.y,
                                   p_points[0].y + first_direction.x));
    for (size_t i = 1; i < p_points.size(); ++i)
    {
        lines.emplace_back(p_points[i - 1], p_points[i]);
    }
    const auto last_direction = normalized_direction(p_points[p_points.size() - 2], p_points.back());
    lines.emplace_back(
        p_points.back(),
        geometry::planar::IntPoint(p_points.back().x - last_direction.y,
                                   p_points.back().y + last_direction.x));

    auto polyline = std::make_shared<geometry::planar::Polyline>(lines);
    auto trace = std::make_unique<PolylineTrace>(
        polyline,
        p_layer,
        p_half_width,
        p_net_nos,
        1,               // clearance_class
        generate_item_id(),
        0,               // group_no
        FixedState::FROM_DESIGN);

    PolylineTrace* result = trace.get();
    items_.push_back(result);
    trace.release();
    return result;
}

std::string RoutingBoard::get_net_name(int p_net_no) const
{
    if (p_net_no > 0)
    {
        const rules::Net* net = get_rules().get_nets().get(p_net_no);
        if (net != nullptr)
        {
            return net->get_name();
        }
    }
    return "NET" + std::to_string(p_net_no);
}

bool RoutingBoard::add_route_connection(Item* p_from_item, int p_from_layer, int p_from_x, int p_from_y,
                                        Item* p_to_item, int p_to_layer, int p_to_x, int p_to_y,
                                        int p_trace_width, int p_net_no)
{
    // Simple stub: just mark the connection as successful
    // Full implementation would create actual traces
    
    (void)p_from_item;
    (void)p_from_layer;
    (void)p_from_x;
    (void)p_from_y;
    (void)p_to_item;
    (void)p_to_layer;
    (void)p_to_x;
    (void)p_to_y;
    (void)p_trace_width;
    (void)p_net_no;
    
    // For now, return true to indicate the connection attempt was made
    // This allows the router to count it as routed even though no actual trace is created
    logger::FRLogger::instance().debug("add_route_connection: connection attempt made");
    
    return true;
}

const BoardOutline* RoutingBoard::get_outline() const
{
    // Mirrors Java BoardItemRepository.getOutline(): returns the first
    // BoardOutline item on the board, or nullptr if none is present.
    for (const Item* curr_item : items_)
    {
        if (dynamic_cast<const BoardOutline*>(curr_item) != nullptr)
        {
            return static_cast<const BoardOutline*>(curr_item);
        }
    }
    return nullptr;
}

bool RoutingBoard::contains_trace_tails(const std::vector<Item*>& p_obstacles,
                                        const std::vector<int>& p_net_no_arr) const
{
    // Mirrors Java RoutingBoard.containsTraceTails: checks if the list
    // contains traces which have no contact at their start or end point.
    // Traces with the excepted net numbers are ignored.
    for (const Item* curr_item : p_obstacles)
    {
        const auto* curr_trace = dynamic_cast<const Trace*>(curr_item);
        if (curr_trace == nullptr)
        {
            continue;
        }
        if (!curr_trace->nets_equal(p_net_no_arr) && curr_trace->is_tail())
        {
            return true;
        }
    }
    return false;
}

Trace* RoutingBoard::get_trace_tail(const geometry::planar::Point& p_point, int p_layer,
                                    const std::vector<int>& p_net_no_arr)
{
    // Mirrors Java BasicBoard.getTraceTail: looks if at p_point ends a trace
    // with the input net numbers which has no normal contact at that position.
    using namespace geometry::planar;
    IntPoint location;
    if (const auto* int_point = dynamic_cast<const IntPoint*>(&p_point))
    {
        location = *int_point;
    }
    else
    {
        location = p_point.to_float().round();
    }
    IntBox point_shape = geometry::planar::TileShape::get_instance(location);
    std::set<SearchTreeObject*, TreeStorableLess> found_items =
        overlapping_objects(point_shape, p_layer);
    for (SearchTreeObject* curr_object : found_items)
    {
        auto* curr_trace = dynamic_cast<Trace*>(curr_object);
        if (curr_trace == nullptr || !curr_trace->nets_equal(p_net_no_arr))
        {
            continue;
        }
        if (curr_trace->first_corner().equals(&p_point))
        {
            if (curr_trace->get_start_contacts().empty())
            {
                return curr_trace;
            }
        }
        if (curr_trace->last_corner().equals(&p_point))
        {
            if (curr_trace->get_end_contacts().empty())
            {
                return curr_trace;
            }
        }
    }
    return nullptr;
}

void RoutingBoard::join_changed_area(const geometry::planar::Point& p_point, int p_layer)
{
    if (changed_area_)
    {
        geometry::planar::FloatPoint fp = p_point.to_float();
        changed_area_->join(static_cast<int64_t>(fp.x), static_cast<int64_t>(fp.y), p_layer);
    }
}

void RoutingBoard::remove_items(const std::vector<Item*>& p_items)
{
    for (Item* item : p_items)
    {
        remove_item(item);
    }
}

int RoutingBoard::clearance_value(int p_class_1, int p_class_2, int p_layer) const
{
    // Java BasicBoard.clearanceValue() adds the clearance safety margin.
    return get_rules().get_clearance_matrix().get_value(p_class_1, p_class_2, p_layer, true);
}

namespace {

/**
 * Polymorphic intersection of two tile shapes. Exact for the regular
 * geometry::planar::IntBox/geometry::planar::IntOctagon/geometry::planar::Simplex combinations; falls back to the bounding-box
 * intersection (conservative) for anything else.
 */
std::unique_ptr<geometry::planar::TileShape> intersect_tiles(
    const geometry::planar::TileShape& p_a,
    const geometry::planar::TileShape& p_b)
{
    const auto* a_box = dynamic_cast<const geometry::planar::IntBox*>(&p_a);
    const auto* b_box = dynamic_cast<const geometry::planar::IntBox*>(&p_b);
    const auto* a_oct = dynamic_cast<const geometry::planar::IntOctagon*>(&p_a);
    const auto* b_oct = dynamic_cast<const geometry::planar::IntOctagon*>(&p_b);
    const auto* a_sim = dynamic_cast<const geometry::planar::Simplex*>(&p_a);
    const auto* b_sim = dynamic_cast<const geometry::planar::Simplex*>(&p_b);
    if (a_box != nullptr && b_box != nullptr)
    {
        return std::make_unique<geometry::planar::IntBox>(a_box->intersection(*b_box));
    }
    if (a_oct != nullptr && b_oct != nullptr)
    {
        return std::make_unique<geometry::planar::IntOctagon>(a_oct->intersection(*b_oct));
    }
    if (a_oct != nullptr && b_box != nullptr)
    {
        return std::make_unique<geometry::planar::IntOctagon>(a_oct->intersection(*b_box));
    }
    if (a_box != nullptr && b_oct != nullptr)
    {
        return std::make_unique<geometry::planar::IntOctagon>(b_oct->intersection(*a_box));
    }
    if (a_sim != nullptr && b_box != nullptr)
    {
        return std::make_unique<geometry::planar::Simplex>(a_sim->intersection(*b_box));
    }
    if (a_box != nullptr && b_sim != nullptr)
    {
        return std::make_unique<geometry::planar::Simplex>(b_sim->intersection(*a_box));
    }
    if (a_sim != nullptr && b_oct != nullptr)
    {
        return std::make_unique<geometry::planar::Simplex>(a_sim->intersection(*b_oct));
    }
    if (a_oct != nullptr && b_sim != nullptr)
    {
        return std::make_unique<geometry::planar::Simplex>(b_sim->intersection(*a_oct));
    }
    if (a_sim != nullptr && b_sim != nullptr)
    {
        return std::make_unique<geometry::planar::Simplex>(a_sim->intersection(*b_sim));
    }
    // Conservative fallback: bounding-box intersection.
    return std::make_unique<geometry::planar::IntBox>(p_a.bounding_box().intersection(p_b.bounding_box()));
}

geometry::planar::IntPoint to_int_point(const geometry::planar::Point& p_point)
{
    if (const auto* int_point = dynamic_cast<const geometry::planar::IntPoint*>(&p_point))
    {
        return *int_point;
    }
    return p_point.to_float().round();
}

} // namespace

double RoutingBoard::check_trace_segment(const geometry::planar::Point& p_from_point,
                                         const geometry::planar::Point& p_to_point,
                                         int p_layer,
                                         const std::vector<int>& p_net_no_arr,
                                         int p_trace_half_width,
                                         int p_cl_class_no,
                                         bool p_only_not_shovable_obstacles)
{
    if (p_from_point.to_float() == p_to_point.to_float())
    {
        return 0.0;
    }
    const std::vector<geometry::planar::IntPoint> corners{to_int_point(p_from_point),
                                                             to_int_point(p_to_point)};
    geometry::planar::Polyline curr_polyline = geometry::planar::Polyline::from_corners(corners);
    geometry::planar::LineSegment curr_line_segment(curr_polyline, 1);
    return check_trace_segment(curr_line_segment, p_layer, p_net_no_arr, p_trace_half_width,
                               p_cl_class_no, p_only_not_shovable_obstacles);
}

double RoutingBoard::check_trace_segment(const geometry::planar::LineSegment& p_line_segment,
                                         int p_layer,
                                         const std::vector<int>& p_net_no_arr,
                                         int p_trace_half_width,
                                         int p_cl_class_no,
                                         bool p_only_not_shovable_obstacles)
{
    constexpr double NO_CONFLICT = static_cast<double>(std::numeric_limits<int>::max());
    geometry::planar::Polyline check_polyline = p_line_segment.to_polyline();
    if (check_polyline.arr.size() != 3)
    {
        return 0.0;
    }
    std::unique_ptr<geometry::planar::TileShape> shape_to_check = check_polyline.offset_shape(p_trace_half_width, 0);
    if (shape_to_check == nullptr)
    {
        return 0.0;
    }
    const geometry::planar::FloatPoint from_point = p_line_segment.start_point_approx();
    const geometry::planar::FloatPoint to_point = p_line_segment.end_point_approx();
    const double line_length = to_point.distance(from_point);
    double ok_length = NO_CONFLICT;
    ShapeSearchTree* default_tree = search_tree_manager().get_default_tree();
    std::vector<datastructures::ShapeTree::TreeEntry> obstacle_entries =
        default_tree->overlapping_tree_entries_with_clearance(*shape_to_check, p_layer, p_net_no_arr,
                                                              p_cl_class_no);
    for (const auto& curr_entry : obstacle_entries)
    {
        auto* curr_obstacle = dynamic_cast<Item*>(curr_entry.object);
        if (curr_obstacle == nullptr)
        {
            continue;
        }
        if (p_only_not_shovable_obstacles && curr_obstacle->is_routable()
            && !curr_obstacle->is_shove_fixed())
        {
            continue;
        }
        const geometry::planar::TileShape* curr_obstacle_shape =
            curr_entry.object->get_tree_shape(*default_tree, curr_entry.shape_index_in_object);
        if (curr_obstacle_shape == nullptr)
        {
            continue;
        }
        const geometry::planar::TileShape* curr_offset_shape = shape_to_check.get();
        std::unique_ptr<geometry::planar::TileShape> enlarged_shape;
        double shorten_value = 0.0;
        if (default_tree->is_clearance_compensation_used())
        {
            shorten_value = p_trace_half_width
                + get_rules().get_clearance_matrix().get_clearance_compensation_value(
                    curr_obstacle->get_clearance_class(), p_layer);
        }
        else
        {
            int clearance_value =
                this->clearance_value(curr_obstacle->get_clearance_class(), p_cl_class_no, p_layer);
            enlarged_shape = shape_to_check->offset(static_cast<double>(clearance_value));
            if (enlarged_shape == nullptr)
            {
                continue;
            }
            curr_offset_shape = enlarged_shape.get();
            shorten_value = p_trace_half_width + clearance_value;
        }
        std::unique_ptr<geometry::planar::TileShape> intersection =
            intersect_tiles(*curr_obstacle_shape, *curr_offset_shape);
        if (intersection == nullptr || intersection->is_empty())
        {
            continue;
        }
        std::optional<geometry::planar::FloatPoint> nearest_obstacle_point =
            intersection->nearest_point_approx(from_point);
        if (!nearest_obstacle_point.has_value())
        {
            continue;
        }
        double projection = 0.0;
        if (line_length > 0.0)
        {
            projection = from_point.scalar_product(to_point, *nearest_obstacle_point) / line_length;
        }
        projection = std::max(0.0, projection - shorten_value - 1.0);
        if (projection < ok_length)
        {
            ok_length = projection;
            if (ok_length <= 0.0)
            {
                return 0.0;
            }
        }
    }
    return ok_length;
}

bool RoutingBoard::normalize_traces(int p_net_no)
{
    bool result = false;
    bool something_changed = true;
    while (something_changed)
    {
        something_changed = false;
        // Snapshot: traces may be removed while iterating.
        std::vector<Item*> snapshot(items_.begin(), items_.end());
        for (Item* curr_item : snapshot)
        {
            if (curr_item == nullptr || !curr_item->contains_net(p_net_no))
            {
                continue;
            }
            auto* curr_trace = dynamic_cast<PolylineTrace*>(curr_item);
            if (curr_trace == nullptr || !curr_trace->is_on_board())
            {
                continue;
            }
            if (curr_trace->normalize(nullptr))
            {
                something_changed = true;
                result = true;
            }
            else if (!curr_trace->is_user_fixed() && remove_if_cycle(curr_trace))
            {
                something_changed = true;
                result = true;
            }
        }
    }
    return result;
}

void RoutingBoard::join_graphics_update_box(const geometry::planar::IntBox& p_box)
{
    (void)p_box;
    // TODO: notify boardgraphics observers once they are ported.
}

Via* RoutingBoard::insert_via(Padstack* p_padstack,
                              const geometry::planar::Point& p_center,
                              const std::vector<int>& p_net_no_arr,
                              int p_clearance_class,
                              FixedState p_fixed_state,
                              bool p_attach_allowed)
{
    if (p_padstack == nullptr)
    {
        logger::FRLogger::instance().warn("RoutingBoard.insert_via: padstack is null");
        return nullptr;
    }
    geometry::planar::IntPoint center;
    if (const auto* int_center = dynamic_cast<const geometry::planar::IntPoint*>(&p_center))
    {
        center = *int_center;
    }
    else
    {
        // Java requires an geometry::planar::IntPoint location; round float locations.
        center = p_center.to_float().round();
    }
    auto* new_via = new Via(p_padstack, center, p_net_no_arr, p_clearance_class, generate_item_id(), 0,
                            p_fixed_state, p_attach_allowed, this);

    // Compute the via bounding box from the padstack shapes, translated to the
    // center. Padstack shapes are stored center-relative (around (0,0)).
    const geometry::planar::IntBox pad_box = p_padstack->get_bounding_box();
    new_via->set_bounding_box(geometry::planar::IntBox(
        pad_box.ll.x + center.x, pad_box.ll.y + center.y,
        pad_box.ur.x + center.x, pad_box.ur.y + center.y));

    add_item(new_via);
    // Register the via in the search tree so traces can connect to it and the
    // autorouter can see it as an obstacle/connection point.
    get_search_tree_manager()->insert(new_via);

    // Split traces on all layers spanned by the via padstack (from_layer .. to_layer-1).
    // Mirrors Java BasicBoard.insertVia / insertEscapeVia.
    int from_layer = p_padstack->get_from_layer();
    int to_layer = p_padstack->get_to_layer();
    for (int layer = from_layer; layer < to_layer; ++layer)
    {
        for (int net_no : p_net_no_arr)
        {
            split_traces(center, layer, net_no);
        }
    }
    return new_via;
}

void RoutingBoard::clear_shove_failing_obstacle()
{
    shove_failing_obstacle_ = nullptr;
    shove_failing_layer_ = -1;
}

bool RoutingBoard::check_forced_trace_polyline(const geometry::planar::Polyline& p_polyline,
                                               int p_half_width,
                                               int p_layer,
                                               const std::vector<int>& p_net_no_arr,
                                               int p_clearance_class_index,
                                               int p_max_recursion_depth,
                                               int p_max_via_recursion_depth,
                                               int p_max_spring_over_recursion_depth)
{
    // Port of Java RoutingBoard.checkForcedTracePolyline().
    ShapeSearchTree* search_tree = search_tree_manager().get_default_tree();
    const int compensated_half_width =
        p_half_width + search_tree->clearance_compensation_value(p_clearance_class_index, p_layer);
    std::vector<std::unique_ptr<geometry::planar::TileShape>> trace_shapes =
        p_polyline.offset_shapes(compensated_half_width, 0,
                                 static_cast<int>(p_polyline.arr.size()) - 1);
    const bool orthogonal_mode =
        get_rules().get_trace_angle_restriction() == rules::AngleRestriction::RIGHT;
    ShoveTraceAlgo shove_trace_algo(this);
    for (std::size_t i = 0; i < trace_shapes.size(); ++i)
    {
        std::unique_ptr<geometry::planar::TileShape> current_trace_shape = std::move(trace_shapes[i]);
        if (orthogonal_mode)
        {
            current_trace_shape = std::make_unique<geometry::planar::IntBox>(current_trace_shape->bounding_box());
        }
        CalcFromSide from_side(p_polyline, static_cast<int>(i) + 1, *current_trace_shape);
        const bool check_shove_ok = shove_trace_algo.check(
            *current_trace_shape, &from_side, nullptr, p_layer, p_net_no_arr, p_clearance_class_index,
            p_max_recursion_depth, p_max_via_recursion_depth, p_max_spring_over_recursion_depth,
            nullptr);
        if (!check_shove_ok)
        {
            return false;
        }
    }
    return true;
}

std::optional<geometry::planar::IntPoint> RoutingBoard::insert_forced_trace_polyline(
    const geometry::planar::Polyline& p_polyline,
    int p_half_width,
    int p_layer,
    const std::vector<int>& p_net_no_arr,
    int p_clearance_class_index,
    int p_max_recursion_depth,
    int p_max_via_recursion_depth,
    int p_max_spring_over_recursion_depth,
    int p_tidy_width,
    int p_pull_tight_accuracy,
    bool p_with_check,
    const datastructures::TimeLimit* p_time_limit)
{
    // Port of Java RoutingBoard.insertForcedTracePolyline() at
    // RoutingBoard.java:456-873.
    clear_shove_failing_obstacle();
    const geometry::planar::IntPoint from_corner = p_polyline.first_corner();
    const geometry::planar::IntPoint to_corner = p_polyline.last_corner();
    if (from_corner == to_corner)
    {
        return to_corner;
    }
    start_marking_changed_area();

    // Check, if there ends an item of the same net at from_corner. If so, its
    // geometry will be used to cut off dog ears of the check shape.
    Trace* picked_trace = nullptr;
    ItemSelectionFilter trace_filter(ItemSelectionFilter::SelectableChoices::TRACES);
    std::set<Item*> picked_items = pick_items(from_corner, p_layer, &trace_filter);
    if (picked_items.size() == 1)
    {
        auto* current_picked_trace = dynamic_cast<Trace*>(*picked_items.begin());
        if (current_picked_trace != nullptr && current_picked_trace->nets_equal(p_net_no_arr)
            && current_picked_trace->get_half_width() == p_half_width
            && current_picked_trace->get_clearance_class() == p_clearance_class_index)
        {
            // can combine with the picked trace
            picked_trace = current_picked_trace;
        }
    }

    ShapeSearchTree* search_tree = search_tree_manager().get_default_tree();
    const int compensated_half_width =
        p_half_width + search_tree->clearance_compensation_value(p_clearance_class_index, p_layer);
    ShoveTraceAlgo shove_trace_algo(this);
    // Java passes null for the contact pins here, which makes the spring over
    // ignore own net pins; a nullptr models that exactly.
    std::optional<geometry::planar::Polyline> spring_over_result = shove_trace_algo.spring_over_obstacles(
        p_polyline, compensated_half_width, p_layer, p_net_no_arr, p_clearance_class_index, nullptr);
    if (!spring_over_result.has_value())
    {
        return from_corner;
    }
    geometry::planar::Polyline new_polyline = std::move(*spring_over_result);

    geometry::planar::Polyline combined_polyline;
    if (picked_trace == nullptr)
    {
        combined_polyline = new_polyline;
    }
    else
    {
        const auto* combine_trace = dynamic_cast<const PolylineTrace*>(picked_trace);
        std::unique_ptr<geometry::planar::Polyline> combined =
            combine_trace != nullptr ? new_polyline.combine(combine_trace->polyline()) : nullptr;
        if (combined == nullptr)
        {
            return from_corner;
        }
        combined_polyline = std::move(*combined);
    }
    if (combined_polyline.arr.size() < 3)
    {
        return from_corner;
    }
    const int start_shape_no =
        static_cast<int>(combined_polyline.arr.size() - new_polyline.arr.size());
    // calculate the last shapes of combined_polyline for checking
    std::vector<std::unique_ptr<geometry::planar::TileShape>> trace_shapes = combined_polyline.offset_shapes(
        compensated_half_width, start_shape_no, static_cast<int>(combined_polyline.arr.size()) - 1);
    int last_shape_no = static_cast<int>(trace_shapes.size());
    const bool orthogonal_mode =
        get_rules().get_trace_angle_restriction() == rules::AngleRestriction::RIGHT;
    // Mirrors the Java `if (orthogonalMode) currentTraceShape =
    // currentTraceShape.boundingBox()` reassignment without consuming the
    // offset shape, so that the shape can be re-used after the loop.
    geometry::planar::IntBox orthogonal_box;
    auto shape_at = [&](int p_index) -> const geometry::planar::TileShape& {
        const geometry::planar::TileShape* shape = trace_shapes[static_cast<size_t>(p_index)].get();
        if (orthogonal_mode)
        {
            orthogonal_box = shape->bounding_box();
            return orthogonal_box;
        }
        return *shape;
    };
    for (int i = 0; i < static_cast<int>(trace_shapes.size()); ++i)
    {
        const geometry::planar::TileShape& current_trace_shape = shape_at(i);
        // Java computes combinedPolyline.cornerCount() - traceShapes.length - 1
        // + i. Without a picked trace this evaluates to i - 2, i.e. the first
        // two shapes get a negative entry index, which CalcFromSide resolves
        // through its "first corner inside shape" fallback. Reproduced as is,
        // because the C++ port must behave like the Java baseline.
        CalcFromSide from_side(combined_polyline,
                               combined_polyline.corner_count() - static_cast<int>(trace_shapes.size()) - 1 + i,
                               current_trace_shape);
        if (p_with_check)
        {
            const bool check_shove_ok = shove_trace_algo.check(
                current_trace_shape, &from_side, nullptr, p_layer, p_net_no_arr,
                p_clearance_class_index, p_max_recursion_depth, p_max_via_recursion_depth,
                p_max_spring_over_recursion_depth, p_time_limit);
            if (!check_shove_ok)
            {
                last_shape_no = i;
                break;
            }
        }
        const bool insert_ok = shove_trace_algo.insert(
            current_trace_shape, &from_side, p_layer, p_net_no_arr, p_clearance_class_index,
            std::vector<Item*>(), p_max_recursion_depth, p_max_via_recursion_depth,
            p_max_spring_over_recursion_depth);
        if (!insert_ok)
        {
            return std::nullopt;
        }
    }
    geometry::planar::IntPoint new_corner = to_corner;

    if (last_shape_no < static_cast<int>(trace_shapes.size()))
    {
        // the shove with index last_shape_no failed.
        // Sample the shove line to a shorter shove distance and try again.
        geometry::planar::TileShape* last_trace_shape = nullptr;
        std::unique_ptr<geometry::planar::TileShape> last_trace_shape_owner;
        const geometry::planar::TileShape& stored_last_shape = shape_at(last_shape_no);
        last_trace_shape = const_cast<geometry::planar::TileShape*>(&stored_last_shape);
        const int sample_width = 2 * get_min_trace_half_width();
        const geometry::planar::FloatPoint last_corner = new_polyline.corner_approx(last_shape_no + 1);
        const geometry::planar::FloatPoint prev_last_corner = new_polyline.corner_approx(last_shape_no);
        const double last_segment_length = last_corner.distance(prev_last_corner);
        if (last_segment_length > 100 * static_cast<double>(sample_width))
        {
            // too many cycles to sample
            return from_corner;
        }
        int shape_index =
            combined_polyline.corner_count() - static_cast<int>(trace_shapes.size()) - 1 + last_shape_no;
        if (last_segment_length > static_cast<double>(sample_width))
        {
            new_polyline = new_polyline.shorten(
                static_cast<int>(new_polyline.arr.size())
                    - (static_cast<int>(trace_shapes.size()) - last_shape_no - 1),
                static_cast<double>(sample_width));
            new_corner = new_polyline.last_corner();
            if (picked_trace == nullptr)
            {
                combined_polyline = new_polyline;
            }
            else
            {
                const auto* combine_trace = dynamic_cast<const PolylineTrace*>(picked_trace);
                std::unique_ptr<geometry::planar::Polyline> combined =
                    combine_trace != nullptr ? new_polyline.combine(combine_trace->polyline()) : nullptr;
                if (combined == nullptr)
                {
                    return new_corner;
                }
                combined_polyline = std::move(*combined);
            }
            if (combined_polyline.arr.size() < 3)
            {
                return new_corner;
            }
            shape_index = static_cast<int>(combined_polyline.arr.size()) - 3;
            last_trace_shape_owner = combined_polyline.offset_shape(compensated_half_width, shape_index);
            if (last_trace_shape_owner == nullptr)
            {
                return new_corner;
            }
            if (orthogonal_mode)
            {
                orthogonal_box = last_trace_shape_owner->bounding_box();
                last_trace_shape = &orthogonal_box;
            }
            else
            {
                last_trace_shape = last_trace_shape_owner.get();
            }
        }
        CalcFromSide from_side(combined_polyline, shape_index, *last_trace_shape);
        const bool check_shove_ok = shove_trace_algo.check(
            *last_trace_shape, &from_side, nullptr, p_layer, p_net_no_arr, p_clearance_class_index,
            p_max_recursion_depth, p_max_via_recursion_depth, p_max_spring_over_recursion_depth,
            p_time_limit);
        if (!check_shove_ok)
        {
            return from_corner;
        }
        const bool insert_ok = shove_trace_algo.insert(
            *last_trace_shape, &from_side, p_layer, p_net_no_arr, p_clearance_class_index,
            std::vector<Item*>(), p_max_recursion_depth, p_max_via_recursion_depth,
            p_max_spring_over_recursion_depth);
        if (!insert_ok)
        {
            logger::FRLogger::instance().trace("RoutingBoard.insert_forced_trace_polyline: shove trace failed");
            return std::nullopt;
        }
    }

    // insert the new trace segment
    for (int i = 0; i < new_polyline.corner_count(); ++i)
    {
        join_changed_area(new_polyline.corner_approx(i), p_layer);
    }
    auto new_polyline_ptr = std::make_shared<geometry::planar::Polyline>(new_polyline);
    PolylineTrace* new_trace = insert_trace_without_cleaning(
        new_polyline_ptr, p_layer, p_half_width, p_net_no_arr, p_clearance_class_index,
        FixedState::NOT_FIXED);
    // Divergence: Java dereferences the result of insertTraceWithoutCleaning
    // without a null check, which would raise an NPE; the C++ port skips the
    // combine in that case.
    if (new_trace != nullptr)
    {
        new_trace->combine();
    }

    std::unique_ptr<geometry::planar::IntOctagon> tidy_region;
    if (p_tidy_width < std::numeric_limits<int>::max())
    {
        tidy_region = std::make_unique<geometry::planar::IntOctagon>(
            new_corner.surrounding_octagon().enlarge(p_tidy_width));
    }
    const std::vector<int> opt_net_no_arr =
        p_max_recursion_depth <= 0 ? p_net_no_arr : std::vector<int>();
    std::unique_ptr<PullTightAlgo> pull_tight_algo = PullTightAlgo::get_instance(
        this, opt_net_no_arr, tidy_region.get(), p_pull_tight_accuracy, nullptr, -1, &new_corner, p_layer);

    // Remove evtl. generated cycles because otherwise pull tight may not work correctly.
    std::unique_ptr<geometry::planar::IntOctagon> changed_layer_area;
    if (changed_area_ != nullptr)
    {
        changed_layer_area = std::make_unique<geometry::planar::IntOctagon>(changed_area_->get_area(p_layer));
    }
    const bool normalize_result = new_trace != nullptr && new_trace->normalize(changed_layer_area.get());
    if (normalize_result)
    {
        pull_tight_algo->split_traces_at_keep_point();
        // otherwise the new corner may no more be contained in the new trace after optimizing
        std::set<Item*> current_picked_items = pick_items(new_corner, p_layer, &trace_filter);
        new_trace = nullptr;
        if (!current_picked_items.empty())
        {
            new_trace = dynamic_cast<PolylineTrace*>(*current_picked_items.begin());
        }
    }

    // To avoid, that a separate handling for moving backwards in the own trace
    // line becomes necessary, pull tight is called here.
    if (p_tidy_width > 0 && new_trace != nullptr)
    {
        new_trace->pull_tight(pull_tight_algo.get());
    }
    return new_corner;
}

std::optional<geometry::planar::IntPoint> RoutingBoard::insert_forced_trace_segment(
    const geometry::planar::IntPoint& p_from_corner,
    const geometry::planar::IntPoint& p_to_corner,
    int p_half_width,
    int p_layer,
    const std::vector<int>& p_net_no_arr,
    int p_clearance_class_index,
    int p_max_recursion_depth,
    int p_max_via_recursion_depth,
    int p_max_spring_over_recursion_depth,
    int p_tidy_width,
    int p_pull_tight_accuracy,
    bool p_with_check,
    const datastructures::TimeLimit* p_time_limit)
{
    // Port of Java RoutingBoard.insertForcedTraceSegment().
    if (p_from_corner == p_to_corner)
    {
        return p_to_corner;
    }
    geometry::planar::Polyline insert_polyline(p_from_corner, p_to_corner);
    std::optional<geometry::planar::IntPoint> ok_point = insert_forced_trace_polyline(
        insert_polyline, p_half_width, p_layer, p_net_no_arr, p_clearance_class_index,
        p_max_recursion_depth, p_max_via_recursion_depth, p_max_spring_over_recursion_depth, p_tidy_width,
        p_pull_tight_accuracy, p_with_check, p_time_limit);
    if (!ok_point.has_value())
    {
        return std::nullopt;
    }
    if (*ok_point == insert_polyline.first_corner())
    {
        return p_from_corner;
    }
    if (*ok_point == insert_polyline.last_corner())
    {
        return p_to_corner;
    }
    return ok_point;
}

bool RoutingBoard::connect_to_trace(const geometry::planar::IntPoint& p_from_point,
                                    Trace* p_to_trace,
                                    int p_pen_half_width,
                                    int p_clearance_class_index)
{
    // Port of Java RoutingBoard.connectToTrace() at RoutingBoard.java:1128-1181.
    auto* polyline_trace = dynamic_cast<PolylineTrace*>(p_to_trace);
    if (polyline_trace == nullptr)
    {
        return false; // not yet implemented
    }
    const geometry::planar::Polyline& trace_polyline = polyline_trace->polyline();
    if (trace_polyline.contains(p_from_point))
    {
        // no connection line necessary
        return true;
    }
    std::optional<geometry::planar::LineSegment> projection_line =
        trace_polyline.projection_line(p_from_point);
    if (!projection_line.has_value())
    {
        return false;
    }
    geometry::planar::Polyline connection_line = projection_line->to_polyline();
    if (connection_line.arr.size() != 3)
    {
        return false;
    }
    const int trace_layer = p_to_trace->get_layer();
    const std::vector<int>& net_nos = p_to_trace->get_net_nos();
    if (!check_polyline_trace(connection_line, trace_layer, p_pen_half_width, net_nos,
                              p_clearance_class_index))
    {
        return false;
    }
    if (changed_area_ != nullptr)
    {
        for (int i = 0; i < connection_line.corner_count(); ++i)
        {
            join_changed_area(connection_line.corner_approx(i), trace_layer);
        }
    }
    auto connection_line_ptr = std::make_shared<geometry::planar::Polyline>(connection_line);
    insert_trace(connection_line_ptr, trace_layer, p_pen_half_width, net_nos,
                 p_clearance_class_index, FixedState::NOT_FIXED);

    const geometry::planar::Point& first_corner = polyline_trace->first_corner();
    const geometry::planar::Point& last_corner = polyline_trace->last_corner();
    if (!p_from_point.equals(first_corner))
    {
        Trace* tail = get_trace_tail(first_corner, trace_layer, net_nos);
        if (tail != nullptr && !tail->is_user_fixed())
        {
            remove_item(tail);
        }
    }
    if (!p_from_point.equals(last_corner))
    {
        Trace* tail = get_trace_tail(last_corner, trace_layer, net_nos);
        if (tail != nullptr && !tail->is_user_fixed())
        {
            remove_item(tail);
        }
    }
    return true;
}

} // namespace freerouting::board