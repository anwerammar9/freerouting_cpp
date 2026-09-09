#include "drc/DesignRulesChecker.h"
#include "logger/FRLogger.h"

#include <algorithm>
#include <sstream>
#include <iomanip>

namespace freerouting {
namespace drc {

DesignRulesChecker::DesignRulesChecker(board::RoutingBoard* p_board,
                                       const settings::DesignRulesCheckerSettings& p_drc_settings)
    : routing_board_(p_board)
    , drc_settings_(p_drc_settings)
{
}

std::vector<ClearanceViolation*> DesignRulesChecker::get_all_clearance_violations()
{
    std::vector<ClearanceViolation*> all_violations;
    std::unordered_set<std::string> seen_violations;

    // Iterate through all items on the board
    // Note: This assumes the board has a method to get all items
    // If not, this will need to be adapted based on the actual board implementation
    if (routing_board_) {
        for (int i = 0; i < routing_board_->get_item_count(); ++i) {
            board::Item* item = routing_board_->get_item(i);
            if (item != nullptr) {
                // Get clearance violations for this item
                // Note: Item::clearance_violations() method needs to be implemented
                // For now, this is a placeholder
                // std::vector<ClearanceViolation*> item_violations = item->clearance_violations();

                // Deduplicate violations - A-B and B-A are the same violation
                // for (ClearanceViolation* violation : item_violations) {
                //     int id1 = violation->get_first_item()->get_id();
                //     int id2 = violation->get_second_item()->get_id();

                //     // Create a unique key using sorted IDs to avoid duplicates
                //     std::string key;
                //     if (id1 < id2) {
                //         key = std::to_string(id1) + "-" + std::to_string(id2) + "-" + std::to_string(violation->get_layer());
                //     } else {
                //         key = std::to_string(id2) + "-" + std::to_string(id1) + "-" + std::to_string(violation->get_layer());
                //     }

                //     if (seen_violations.find(key) == seen_violations.end()) {
                //         seen_violations.insert(key);
                //         all_violations.push_back(violation);
                //     }
                // }
            }
        }
    }

    return all_violations;
}

std::vector<UnconnectedItems*> DesignRulesChecker::get_all_unconnected_items()
{
    std::vector<UnconnectedItems*> unconnected_items;

    if (!routing_board_) {
        return unconnected_items;
    }

    // Group items by net
    std::unordered_map<int, std::vector<board::Item*>> items_by_net;
    for (int i = 0; i < routing_board_->get_item_count(); ++i) {
        board::Item* item = routing_board_->get_item(i);
        if (item != nullptr) {
            // Check if item is connectable and has nets
            auto connectable = dynamic_cast<board::Connectable*>(item);
            if (connectable && !item->get_net_nos().empty()) {
                int net_no = item->get_net_nos()[0];
                items_by_net[net_no].push_back(item);
            }
        }
    }

    // For each net, find truly unconnected items
    for (auto& entry : items_by_net) {
        int net_no = entry.first;
        std::vector<board::Item*>& net_items = entry.second;

        if (net_items.size() <= 1) {
            continue; // Single item nets are not unconnected
        }

        // Get all connected sets for this net
        std::vector<std::vector<board::Item*>> connected_sets;
        std::unordered_set<board::Item*> processed_items;

        for (board::Item* item : net_items) {
            if (processed_items.find(item) != processed_items.end()) {
                continue;
            }

            // Get the connected set for this item
            auto connectable = dynamic_cast<board::Connectable*>(item);
            if (connectable) {
                std::set<std::shared_ptr<board::Item>> connected_set = connectable->get_connected_set(net_no);
                std::vector<board::Item*> set_items;

                // Convert shared_ptr to raw pointers and filter to items in this net
                for (const auto& connected_item : connected_set) {
                    board::Item* raw_item = connected_item.get();
                    if (std::find(net_items.begin(), net_items.end(), raw_item) != net_items.end()) {
                        set_items.push_back(raw_item);
                    }
                }

                if (!set_items.empty()) {
                    connected_sets.push_back(set_items);
                    processed_items.insert(set_items.begin(), set_items.end());
                }
            }
        }

        // If there are multiple connected sets, we have unconnected items
        if (connected_sets.size() >= 2) {
            // Find representative items from the first two sets
            board::Item* item1 = find_representative_item(connected_sets[0]);
            board::Item* item2 = find_representative_item(connected_sets[1]);

            if (item1 != nullptr && item2 != nullptr) {
                // Include only items from the two disconnected groups for investigation
                std::vector<board::Item*> unconnected_group_items;
                unconnected_group_items.insert(unconnected_group_items.end(),
                                             connected_sets[0].begin(),
                                             connected_sets[0].end());
                unconnected_group_items.insert(unconnected_group_items.end(),
                                             connected_sets[1].begin(),
                                             connected_sets[1].end());

                unconnected_items.push_back(new UnconnectedItems(item1, item2, unconnected_group_items));
            }
        }
    }

    // Check for dangling traces - traces with unconnected ends
    for (int i = 0; i < routing_board_->get_item_count(); ++i) {
        board::Item* item = routing_board_->get_item(i);
        if (item != nullptr) {
            // Check if item is a trace by checking if it's trace type
            if (item->is_trace()) {
                auto connectable = dynamic_cast<board::Connectable*>(item);
                if (connectable) {
                    // Check if trace has contacts on both ends
                    // Note: This requires implementing get_start_contacts and get_end_contacts methods
                    // For now, this is a placeholder
                    // std::set<std::shared_ptr<board::Item>> start_contacts = trace->get_start_contacts();
                    // std::set<std::shared_ptr<board::Item>> end_contacts = trace->get_end_contacts();

                    // A trace is dangling if either its start or end has no contacts
                    // if (start_contacts.empty() || end_contacts.empty()) {
                    //     unconnected_items.push_back(new UnconnectedItems(item, nullptr, "track_dangling"));
                    // }
                }
            }
        }
    }

    // Check for dangling vias - vias not connected or connected on only one layer
    for (int i = 0; i < routing_board_->get_item_count(); ++i) {
        board::Item* item = routing_board_->get_item(i);
        if (item != nullptr) {
            auto via = dynamic_cast<board::Via*>(item);
            if (via) {
                // Use the is_tail() method which checks if via has contacts on at most 1 layer
                // Note: This requires implementing is_tail() method in Via class
                // if (via->is_tail()) {
                //     unconnected_items.push_back(new UnconnectedItems(via, nullptr, "via_dangling"));
                // }
            }
        }
    }

    return unconnected_items;
}

board::Item* DesignRulesChecker::find_representative_item(const std::vector<board::Item*>& p_connected_set)
{
    // Prefer Pins
    for (board::Item* item : p_connected_set) {
        if (dynamic_cast<board::Pin*>(item) != nullptr) {
            return item;
        }
    }
    // Then Traces
    for (board::Item* item : p_connected_set) {
        if (item->is_trace()) {
            return item;
        }
    }
    // Finally any item
    return p_connected_set.empty() ? nullptr : p_connected_set[0];
}

std::unique_ptr<DrcReport> DesignRulesChecker::generate_report(const std::string& p_source_file,
                                                               const std::string& p_coordinate_unit)
{
    std::string version = "Freerouting " + std::string(constants::FREEROUTING_VERSION);
    auto report = std::make_unique<DrcReport>(p_coordinate_unit, p_source_file, version);

    // Get all clearance violations
    std::vector<ClearanceViolation*> violations = get_all_clearance_violations();

    // Convert internal violations to DRC report format
    for (ClearanceViolation* violation : violations) {
        auto drc_violation = convert_to_drc_violation(violation, p_coordinate_unit);
        report->add_violation(drc_violation);
    }

    // Get all unconnected items
    std::vector<UnconnectedItems*> unconnected_items = get_all_unconnected_items();

    // Convert unconnected items to DRC report format
    for (UnconnectedItems* unconnected_item : unconnected_items) {
        auto drc_violation = convert_to_drc_violation(unconnected_item, p_coordinate_unit);
        if (unconnected_item->get_type() == "track_dangling" || unconnected_item->get_type() == "via_dangling") {
            report->add_violation(drc_violation);
        } else {
            report->add_unconnected_item(drc_violation);
        }
    }

    return report;
}

std::shared_ptr<DrcViolation> DesignRulesChecker::convert_to_drc_violation(const ClearanceViolation* p_violation,
                                                                           const std::string& p_coordinate_unit)
{
    (void)p_violation;
    (void)p_coordinate_unit;
    // Convert internal violation to DRC report format
    // This is a placeholder implementation
    std::vector<std::shared_ptr<DrcViolationItem>> items;

    // Create violation items for first and second item
    // Note: This requires implementing proper UUID generation and position conversion
    // For now, this is a placeholder

    std::string description = "Clearance violation";
    std::string severity = "error";
    std::string type = "clearance";

    return std::make_shared<DrcViolation>(type, description, severity, items);
}

std::shared_ptr<DrcViolation> DesignRulesChecker::convert_to_drc_violation(const UnconnectedItems* p_unconnected_item,
                                                                           const std::string& p_coordinate_unit)
{
    (void)p_coordinate_unit;
    // Convert unconnected items to DRC report format
    // This is a placeholder implementation
    std::vector<std::shared_ptr<DrcViolationItem>> items;

    // Create violation items for the unconnected items
    // Note: This requires implementing proper UUID generation and position conversion
    // For now, this is a placeholder

    std::string description = "Unconnected items";
    std::string severity = "error";
    std::string type = p_unconnected_item->get_type();

    return std::make_shared<DrcViolation>(type, description, severity, items);
}

} // namespace drc
} // namespace freerouting
