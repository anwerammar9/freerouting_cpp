#include "interactive/HeadlessBoardManager.h"

#include "board/RoutingBoard.h"

namespace freerouting {
namespace interactive {

HeadlessBoardManager::HeadlessBoardManager(const std::locale& p_locale)
    : locale_(p_locale)
    , settings_(std::make_shared<InteractiveSettings>())
{
}

HeadlessBoardManager::HeadlessBoardManager(const std::locale& p_locale,
                                     const std::shared_ptr<RoutingJob>& p_routing_job)
    : locale_(p_locale)
    , routing_job_(p_routing_job)
    , settings_(std::make_shared<InteractiveSettings>())
{
}

std::shared_ptr<board::RoutingBoard> HeadlessBoardManager::get_routing_board()
{
    return board_;
}

void HeadlessBoardManager::replace_routing_board(const std::shared_ptr<board::RoutingBoard>& p_board)
{
    board_ = p_board;
}

void HeadlessBoardManager::initialize_manual_trace_half_widths()
{
    if (!board_ || !board_->rules)
    {
        return;
    }
    auto net_class = board_->rules->get_default_net_class();
    if (net_class)
    {
        for (size_t i = 0; i < 10; i++)
        {
            settings_->set_trace_half_width(static_cast<int>(i), net_class->get_trace_half_width(static_cast<int>(i)));
        }
    }
}

void HeadlessBoardManager::create_board(const geometry::planar::IntBox& p_bounding_box,
                                     const board::LayerStructure& p_layer_structure,
                                     const std::vector<std::shared_ptr<geometry::planar::PolylineShape>>& p_outline_shapes,
                                     const std::string& p_outline_clearance_class_name,
                                     std::shared_ptr<rules::BoardRules> p_rules,
                                     board::Communication* p_board_communication)
{
    if (board_)
    {
        return;
    }

    int outline_cl_class_no = 0;
    if (p_rules)
    {
        if (!p_outline_clearance_class_name.empty())
        {
            outline_cl_class_no = p_rules->clearance_matrix.get_no(p_outline_clearance_class_name);
            outline_cl_class_no = std::max(outline_cl_class_no, 0);
        }
        else
        {
            outline_cl_class_no = p_rules->get_default_net_class()
                ->default_item_clearance_classes.get(
                    rules::DefaultItemClearanceClasses::ItemClass::AREA);
        }
    }

    board_ = std::make_shared<board::RoutingBoard>(p_bounding_box, p_layer_structure,
                                              std::move(p_rules), p_board_communication);
    settings_ = std::make_shared<InteractiveSettings>();
}

int64_t HeadlessBoardManager::calculate_crc32() const
{
    return 0;
}

bool HeadlessBoardManager::load_from_specctra_dsn(std::istream& input_stream)
{
    return false;
}

bool HeadlessBoardManager::save_as_specctra_session(std::ostream& output_stream,
                                            const std::string& design_name)
{
    return false;
}

} // namespace interactive
} // namespace freerouting