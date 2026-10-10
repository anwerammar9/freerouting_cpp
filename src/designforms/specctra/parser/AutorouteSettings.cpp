#include "designforms/specctra/parser/AutorouteSettings.h"

#include <charconv>
#include <cstring>
#include <string>

#include "datastructures/IdentifierType.h"
#include "datastructures/IndentFileWriter.h"
#include "designforms/specctra/parser/DsnFile.h"
#include "designforms/specctra/parser/IJFlexScanner.h"
#include "designforms/specctra/parser/ScopeKeyword.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra::parser {

namespace {

std::shared_ptr<settings::RouterSettings> read_layer_rule_impl(
    IJFlexScanner& p_scanner, const LayerStructure& p_layer_structure,
    std::shared_ptr<settings::RouterSettings> p_settings);

/**
 * Formats a float like Java Float.toString: the shortest decimal that
 * round-trips, always carrying a fraction part for integral values
 * (e.g. "1.0", "0.5"). Java parity for the write scope.
 */
std::string java_float_string(float p_value)
{
    char buffer[64];
    const std::to_chars_result result = std::to_chars(buffer, buffer + sizeof(buffer), p_value);
    std::string text(buffer, result.ptr);
    if (text.find('.') == std::string::npos && text.find('e') == std::string::npos
        && text.find('E') == std::string::npos)
    {
        text += ".0";
    }
    return text;
}

} // namespace

std::shared_ptr<settings::RouterSettings> AutorouteSettings::read_scope(
    IJFlexScanner& p_scanner, const LayerStructure& p_layer_structure)
{
    auto& log = logger::FRLogger::instance();
    auto result = std::make_shared<settings::RouterSettings>();
    result->set_layer_count(static_cast<int>(p_layer_structure.layers.size()));
    bool with_autoroute = true;
    bool with_postroute = true;
    Token next_token;
    for (;;)
    {
        const Token prev_token = next_token;
        next_token = p_scanner.next_token();
        if (next_token.is_eof())
        {
            log.warn("AutorouteSettings.read_scope: unexpected end of file at '"
                + p_scanner.get_scope_identifier() + "'");
            return nullptr;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            // end of scope
            break;
        }
        if (prev_token.is(Keyword::OPEN_BRACKET))
        {
            if (next_token.is(Keyword::FANOUT))
            {
                DsnFile::read_on_off_scope(p_scanner);
            }
            else if (next_token.is(Keyword::AUTOROUTE))
            {
                with_autoroute = DsnFile::read_on_off_scope(p_scanner);
            }
            else if (next_token.is(Keyword::POSTROUTE))
            {
                with_postroute = DsnFile::read_on_off_scope(p_scanner);
            }
            else if (next_token.is(Keyword::VIAS))
            {
                result->set_vias_allowed(DsnFile::read_on_off_scope(p_scanner));
            }
            else if (next_token.is(Keyword::VIA_COSTS))
            {
                result->set_via_costs(DsnFile::read_integer_scope(p_scanner));
            }
            else if (next_token.is(Keyword::PLANE_VIA_COSTS))
            {
                result->set_plane_via_costs(DsnFile::read_integer_scope(p_scanner));
            }
            else if (next_token.is(Keyword::START_RIPUP_COSTS))
            {
                result->set_start_ripup_costs(DsnFile::read_integer_scope(p_scanner));
            }
            else if (next_token.is(Keyword::LAYER_RULE))
            {
                result = read_layer_rule_impl(p_scanner, p_layer_structure, result);
                if (result == nullptr)
                {
                    return nullptr;
                }
            }
            else
            {
                ScopeKeyword::skip_scope(p_scanner);
            }
        }
    }
    result->set_run_router(with_autoroute);
    result->set_run_optimizer(with_postroute);
    return result;
}

namespace {

std::shared_ptr<settings::RouterSettings> read_layer_rule_impl(
    IJFlexScanner& p_scanner, const LayerStructure& p_layer_structure,
    std::shared_ptr<settings::RouterSettings> p_settings)
{
    auto& log = logger::FRLogger::instance();
    p_scanner.yybegin(SpecctraDsnStreamReader::NAME);
    Token next_token = p_scanner.next_token();
    if (!next_token.is_string())
    {
        log.warn("AutorouteSettings.read_layer_rule: String expected at '" + p_scanner.get_scope_identifier() + "'");
        return nullptr;
    }
    const int layer_index = p_layer_structure.get_no(next_token.string_value);
    if (layer_index < 0)
    {
        log.warn("AutorouteSettings.read_layer_rule: layer not found at '" + p_scanner.get_scope_identifier() + "'");
        return nullptr;
    }
    for (;;)
    {
        const Token prev_token = next_token;
        next_token = p_scanner.next_token();
        if (next_token.is_eof())
        {
            log.warn("AutorouteSettings.read_layer_rule: unexpected end of file at '"
                + p_scanner.get_scope_identifier() + "'");
            return nullptr;
        }
        if (next_token.is(Keyword::CLOSED_BRACKET))
        {
            // end of scope
            break;
        }
        if (prev_token.is(Keyword::OPEN_BRACKET))
        {
            if (next_token.is(Keyword::ACTIVE))
            {
                p_settings->set_layer_active(layer_index, DsnFile::read_on_off_scope(p_scanner));
            }
            else if (next_token.is(Keyword::PREFERRED_DIRECTION))
            {
                bool pref_dir_is_horizontal = true;
                next_token = p_scanner.next_token();
                if (next_token.is(Keyword::VERTICAL))
                {
                    pref_dir_is_horizontal = false;
                }
                else if (!next_token.is(Keyword::HORIZONTAL))
                {
                    log.warn("AutorouteSettings.read_layer_rule: unexpected key word at '"
                        + p_scanner.get_scope_identifier() + "'");
                    return nullptr;
                }
                p_settings->set_preferred_direction_is_horizontal(layer_index, pref_dir_is_horizontal);
                next_token = p_scanner.next_token();
                if (!next_token.is(Keyword::CLOSED_BRACKET))
                {
                    log.warn("AutorouteSettings.read_layer_rule: closing bracket expected at '"
                        + p_scanner.get_scope_identifier() + "'");
                    return nullptr;
                }
            }
            else if (next_token.is(Keyword::PREFERRED_DIRECTION_TRACE_COSTS))
            {
                p_settings->set_preferred_direction_trace_costs(
                    layer_index, DsnFile::read_float_scope(p_scanner));
            }
            else if (next_token.is(Keyword::AGAINST_PREFERRED_DIRECTION_TRACE_COSTS))
            {
                p_settings->set_against_preferred_direction_trace_costs(
                    layer_index, DsnFile::read_float_scope(p_scanner));
            }
            else
            {
                ScopeKeyword::skip_scope(p_scanner);
            }
        }
    }
    return p_settings;
}

} // namespace

std::shared_ptr<settings::RouterSettings> AutorouteSettings::read_layer_rule(
    IJFlexScanner& p_scanner, const LayerStructure& p_layer_structure,
    std::shared_ptr<settings::RouterSettings> p_settings)
{
    return read_layer_rule_impl(p_scanner, p_layer_structure, std::move(p_settings));
}

void AutorouteSettings::write_scope(datastructures::IndentFileWriter& p_file,
    const settings::RouterSettings& p_settings, const board::LayerStructure& p_layer_structure,
    const datastructures::IdentifierType& p_identifier_type)
{
    p_file.start_scope();
    p_file << "autoroute_settings";
    p_file.new_line();
    p_file << "(autoroute ";
    if (p_settings.get_run_router())
    {
        p_file << "on)";
    }
    else
    {
        p_file << "off)";
    }
    p_file.new_line();
    p_file << "(postroute ";
    if (p_settings.get_run_optimizer())
    {
        p_file << "on)";
    }
    else
    {
        p_file << "off)";
    }
    p_file.new_line();
    p_file << "(vias ";
    if (p_settings.get_vias_allowed())
    {
        p_file << "on)";
    }
    else
    {
        p_file << "off)";
    }
    p_file.new_line();
    p_file << "(via_costs " << p_settings.get_via_costs() << ")";
    p_file.new_line();
    p_file << "(plane_via_costs " << p_settings.get_plane_via_costs() << ")";
    p_file.new_line();
    p_file << "(start_ripup_costs " << p_settings.get_start_ripup_costs() << ")";
    p_file.new_line();
    for (int i = 0; i < p_layer_structure.layer_count(); ++i)
    {
        const std::shared_ptr<board::Layer>& current_layer = p_layer_structure.get_layer(i);
        p_file.start_scope();
        p_file << "layer_rule ";
        p_identifier_type.write(current_layer->name, p_file.stream());
        p_file.new_line();
        p_file << "(active ";
        if (p_settings.get_layer_active(i))
        {
            p_file << "on)";
        }
        else
        {
            p_file << "off)";
        }
        p_file.new_line();
        p_file << "(preferred_direction ";
        if (p_settings.get_preferred_direction_is_horizontal(i))
        {
            p_file << "horizontal)";
        }
        else
        {
            p_file << "vertical)";
        }
        p_file.new_line();
        const float preferred_costs = static_cast<float>(p_settings.get_preferred_direction_trace_costs(i));
        p_file << "(preferred_direction_trace_costs " << java_float_string(preferred_costs) << ")";
        p_file.new_line();
        const float against_costs = static_cast<float>(p_settings.get_against_preferred_direction_trace_costs(i));
        p_file << "(against_preferred_direction_trace_costs " << java_float_string(against_costs) << ")";
        p_file.end_scope();
    }
    p_file.end_scope();
}

} // namespace freerouting::designforms::specctra::parser