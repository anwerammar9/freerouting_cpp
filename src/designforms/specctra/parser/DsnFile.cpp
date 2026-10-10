#include "designforms/specctra/parser/DsnFile.h"
#include "rules/Net.h"
#include "board/RoutingBoard.h"
#include "board/ConductionArea.h"
#include "board/BoardOutline.h"

#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/ScopeKeyword.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra::parser {

bool DsnFile::read_on_off_scope(IJFlexScanner& p_scanner)
{
    const Token next_token = p_scanner.next_token();
    bool result = false;
    if (next_token.is(Keyword::ON))
    {
        result = true;
    }
    else if (!next_token.is(Keyword::OFF))
    {
        logger::FRLogger::instance().warn(
            "DsnFile.read_boolean: Keyword.OFF expected at '" + p_scanner.get_scope_identifier() + "'");
    }
    ScopeKeyword::skip_scope(p_scanner);
    return result;
}

int DsnFile::read_integer_scope(IJFlexScanner& p_scanner)
{
    Token next_token = p_scanner.next_token();
    if (!next_token.is_int())
    {
        logger::FRLogger::instance().warn(
            "DsnFile.read_integer_scope: number expected at '" + p_scanner.get_scope_identifier() + "'");
        return 0;
    }
    const int value = next_token.int_value;
    next_token = p_scanner.next_token();
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        logger::FRLogger::instance().warn(
            "DsnFile.read_integer_scope: closing bracket expected at '" + p_scanner.get_scope_identifier() + "'");
        return 0;
    }
    return value;
}

double DsnFile::read_float_scope(IJFlexScanner& p_scanner)
{
    Token next_token = p_scanner.next_token();
    if (!next_token.is_number())
    {
        logger::FRLogger::instance().warn(
            "DsnFile.read_float_scope: number expected at '" + p_scanner.get_scope_identifier() + "'");
        return 0;
    }
    const double value = next_token.number();
    next_token = p_scanner.next_token();
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        logger::FRLogger::instance().warn(
            "DsnFile.read_float_scope: closing bracket expected at '" + p_scanner.get_scope_identifier() + "'");
        return 0;
    }
    return value;
}

std::optional<std::string> DsnFile::read_string_scope(IJFlexScanner& p_scanner)
{
    std::string result = p_scanner.next_string(true);
    Token next_token = p_scanner.next_token();
    if (!next_token.is(Keyword::CLOSED_BRACKET))
    {
        logger::FRLogger::instance().warn("DsnFile.read_string_scope: closing bracket expected at '"
            + p_scanner.get_scope_identifier() + "', got: " + next_token.to_string());
        while (!next_token.is_eof() && !next_token.is(Keyword::CLOSED_BRACKET))
        {
            next_token = p_scanner.next_token();
        }
    }
    return result;
}

std::optional<std::vector<std::string>> DsnFile::read_string_list_scope(IJFlexScanner& p_scanner)
{
    std::vector<std::string> result = p_scanner.next_string_list();
    if (!p_scanner.next_closing_bracket())
    {
        return std::nullopt;
    }
    return result;
}

bool DsnFile::adjust_plane_autoroute_settings(board::RoutingBoard* p_board)
{
    if (p_board == nullptr)
    {
        return false;
    }
    const board::LayerStructure& layers = p_board->get_layer_structure();
    if (layers.layer_count() < 1)
    {
        return false;
    }
    for (int i = 0; i < layers.layer_count(); ++i)
    {
        if (!layers.get_layer(i)->is_signal)
        {
            return false;
        }
    }
    std::vector<bool> changed_layers(layers.layer_count(), false);
    bool nothing_changed = true;

    double board_area = 0;
    if (const board::BoardOutline* outline = p_board->get_outline())
    {
        for (int i = 0; i < outline->shape_count(); ++i)
        {
            for (const auto& piece : outline->get_shape(i)->split_to_convex())
            {
                board_area += piece->area();
            }
        }
    }
    if (board_area <= 0)
    {
        return false;
    }

    for (board::Item* item : p_board->get_items()) // Java getConductionAreas: item-list order
    {
        auto* area_item = dynamic_cast<board::ConductionArea*>(item);
        if (area_item == nullptr)
        {
            continue;
        }
        const int layer_index = area_item->get_layer();
        if (layer_index < 0 || layer_index >= layers.layer_count() || !layers.get_layer(layer_index)->is_signal)
        {
            continue;
        }
        double current_area = 0;
        for (const auto& piece : area_item->get_area()->split_to_convex())
        {
            current_area += piece->area();
        }
        // Relaxed area threshold: 30% of board area (supports outer-layer pours on 2-layer boards)
        if (current_area < 0.3 * board_area)
        {
            continue;
        }
        for (int i = 0; i < area_item->net_count(); ++i)
        {
            if (rules::Net* net = p_board->mutable_rules().get_nets().get(area_item->get_net_no(i)))
            {
                net->set_contains_plane(true);
                nothing_changed = false;
            }
        }
        changed_layers[layer_index] = true;
        if (static_cast<int>(area_item->get_fixed_state()) < static_cast<int>(board::FixedState::USER_FIXED))
        {
            area_item->set_fixed_state(board::FixedState::USER_FIXED);
        }
    }
    for (int i = 0; i < layers.layer_count(); ++i)
    {
        if (changed_layers[i])
        {
            logger::FRLogger::instance().info("Layer '" + layers.get_layer(i)->name
                + "' contains a power plane / large copper pour covering >=30% of the board; "
                  "associated net(s) configured for plane routing.");
        }
    }
    return !nothing_changed;
}

} // namespace freerouting::designforms::specctra::parser
