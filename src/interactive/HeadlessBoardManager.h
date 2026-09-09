#pragma once

#include <memory>
#include <string>
#include <locale>

#include "interactive/BoardManager.h"
#include "interactive/InteractiveSettings.h"

namespace freerouting {
namespace interactive {

class HeadlessBoardManager : public BoardManager
{
public:
    HeadlessBoardManager(const std::locale& p_locale);
    HeadlessBoardManager(const std::locale& p_locale,
                        const std::shared_ptr<class RoutingJob>& p_routing_job);

    std::shared_ptr<board::RoutingBoard> get_routing_board() override;
    void replace_routing_board(const std::shared_ptr<board::RoutingBoard>& p_board);

    std::shared_ptr<InteractiveSettings> get_settings() override { return settings_; }

    void initialize_manual_trace_half_widths() override;

    void create_board(const geometry::planar::IntBox& p_bounding_box,
                      const board::LayerStructure& p_layer_structure,
                      const std::vector<std::shared_ptr<geometry::planar::PolylineShape>>& p_outline_shapes,
                      const std::string& p_outline_clearance_class_name,
                      std::shared_ptr<rules::BoardRules> p_rules,
                      board::Communication* p_board_communication) override;

    std::locale get_locale() override { return locale_; }

    std::shared_ptr<class RoutingJob> get_current_routing_job() const { return routing_job_; }

    int64_t calculate_crc32() const;

    bool load_from_specctra_dsn(std::istream& input_stream);
    bool save_as_specctra_session(std::ostream& output_stream, const std::string& design_name);

private:
    std::locale locale_;
    std::shared_ptr<InteractiveSettings> settings_;
    std::shared_ptr<board::RoutingBoard> board_;
    std::shared_ptr<class RoutingJob> routing_job_;
    int64_t original_board_checksum_ = 0;
};

} // namespace interactive
} // namespace freerouting