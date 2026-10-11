#include "designforms/specctra/DsnReader.h"

#include <filesystem>

#include "designforms/specctra/parser/DsnFile.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/Layer.h"
#include "designforms/specctra/parser/MinimalBoardManager.h"
#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "designforms/specctra/parser/ScopeKeyword.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra {

namespace {

using parser::Keyword;

const char* const kHeaderError = "Not a Specctra DSN file: expected '(pcb <name>' header";

/** Takes the board out of the parser's MinimalBoardManager (Java scopeParameter.getBoard()). */
std::unique_ptr<board::RoutingBoard> release_board(parser::ReadScopeParameter& p_par)
{
    auto* manager = dynamic_cast<parser::MinimalBoardManager*>(p_par.board_handling.get());
    return manager != nullptr ? manager->release_board() : nullptr;
}

/** Checks the "(pcb <name>" header; returns false if it is missing. Sets p_pcb_name if present. */
bool read_header(parser::SpecctraDsnStreamReader& p_scanner, std::string* p_pcb_name)
{
    for (int i = 0; i < 3; ++i)
    {
        const parser::Token token = p_scanner.next_token();
        bool ok = true;
        if (i == 0)
        {
            ok = token.is(Keyword::OPEN_BRACKET);
        }
        else if (i == 1)
        {
            ok = token.is(Keyword::PCB_SCOPE);
            // switch the scanner to NAME mode so the pcb-name token is consumed cleanly
            p_scanner.yybegin(parser::SpecctraDsnStreamReader::NAME);
        }
        else if (p_pcb_name != nullptr && token.is_string())
        {
            // i == 2: the design name string immediately following "(pcb"
            *p_pcb_name = token.string_value;
        }
        if (!ok)
        {
            return false;
        }
    }
    return true;
}

bool is_blank(const std::string& p_text)
{
    return p_text.find_first_not_of(" \t\n\r\f\v") == std::string::npos;
}

void warn_about_warnings(const std::string& p_design_name, const std::vector<std::string>& p_warnings)
{
    if (!p_warnings.empty())
    {
        logger::FRLogger::instance().warn("DSN file '" + p_design_name + "' was loaded with "
            + std::to_string(p_warnings.size()) + " warning(s).");
    }
}

} // namespace

BoardReadResult DsnReader::read_board(std::istream& p_input, const std::string& p_design_name)
{
    if (!p_input.good())
    {
        return board_read_result::IoError{"input stream is not readable"};
    }
    parser::SpecctraDsnStreamReader scanner(p_input);

    // Validate the "(pcb <name>" header — identical check to DsnFile.read
    std::string pcb_token_name;
    if (!read_header(scanner, &pcb_token_name))
    {
        return board_read_result::ParseError{"(pcb", kHeaderError};
    }

    // Resolve the effective design name for log messages: prefer the caller-supplied file name,
    // then the pcb-name token, then a fallback.
    std::string effective_design_name;
    if (!is_blank(p_design_name))
    {
        effective_design_name = std::filesystem::path(p_design_name).filename().string();
    }
    else if (!is_blank(pcb_token_name))
    {
        effective_design_name = pcb_token_name;
    }
    else
    {
        effective_design_name = "unknown";
    }

    // Parse the body — the board is constructed inside ReadScopeParameter's shim
    parser::ReadScopeParameter scope_parameter(scanner);
    const bool read_ok = Keyword::PCB_SCOPE->read_scope(scope_parameter);
    std::unique_ptr<board::RoutingBoard> board = release_board(scope_parameter);

    if (read_ok)
    {
        // Apply power-plane autoroute settings if the DSN had no (autoroute ...) scope
        if (scope_parameter.autoroute_settings == nullptr)
        {
            parser::DsnFile::adjust_plane_autoroute_settings(board.get());
        }
        warn_about_warnings(effective_design_name, scope_parameter.warnings);
        return board_read_result::Success{std::move(board), std::nullopt, scope_parameter.warnings};
    }
    if (!scope_parameter.board_outline_ok)
    {
        warn_about_warnings(effective_design_name, scope_parameter.warnings);
        return board_read_result::OutlineMissing{std::move(board), std::nullopt, scope_parameter.warnings};
    }
    return board_read_result::ParseError{"(pcb", "DSN structure parsing failed"};
}

BoardReadResult DsnReader::read_metadata(std::istream& p_input)
{
    if (!p_input.good())
    {
        return board_read_result::IoError{"input stream is not readable"};
    }
    parser::SpecctraDsnStreamReader scanner(p_input);
    if (!read_header(scanner, nullptr))
    {
        return board_read_result::ParseError{"(pcb", kHeaderError};
    }

    // Custom PCB-level loop — only parse metadata-relevant scopes; stop as soon as
    // (structure ...) ends, skipping all subsequent heavy scopes.
    parser::ReadScopeParameter scope_parameter(scanner);
    parser::Token next_token;
    for (;;)
    {
        const parser::Token prev_token = next_token;
        next_token = scanner.next_token();
        if (next_token.is_eof() || next_token.is(Keyword::CLOSED_BRACKET))
        {
            break; // EOF or end of (pcb ...) scope
        }
        if (prev_token.is(Keyword::OPEN_BRACKET))
        {
            if (next_token.is(Keyword::PARSER_SCOPE))
            {
                Keyword::PARSER_SCOPE->read_scope(scope_parameter);
            }
            else if (next_token.is(Keyword::RESOLUTION_SCOPE))
            {
                Keyword::RESOLUTION_SCOPE->read_scope(scope_parameter);
            }
            else if (next_token.is(Keyword::STRUCTURE_SCOPE))
            {
                // Creates the board via MinimalBoardManager (if a valid boundary exists); the return
                // value is ignored — whatever was populated is extracted.
                Keyword::STRUCTURE_SCOPE->read_scope(scope_parameter);
                break; // stop here — skip library, placement, network, wiring
            }
            else
            {
                parser::ScopeKeyword::skip_scope(scanner);
            }
        }
    }

    std::unique_ptr<board::RoutingBoard> board = release_board(scope_parameter);
    BoardMetadata metadata;
    if (scope_parameter.layer_structure != nullptr)
    {
        metadata.layer_count = static_cast<int>(scope_parameter.layer_structure->layers.size());
    }
    else if (board != nullptr)
    {
        metadata.layer_count = board->get_layer_count();
    }
    metadata.host_cad = scope_parameter.host_cad;
    metadata.host_version = scope_parameter.host_version;
    metadata.unit = scope_parameter.unit;
    metadata.resolution = scope_parameter.resolution;
    metadata.snap_angle = scope_parameter.snap_angle;
    metadata.router_settings = scope_parameter.autoroute_settings;
    return board_read_result::Success{std::move(board), std::move(metadata), scope_parameter.warnings};
}

} // namespace freerouting::designforms::specctra
