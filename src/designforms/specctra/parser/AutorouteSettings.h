#pragma once

#include <memory>

#include "board/Layer.h"
#include "designforms/specctra/parser/Layer.h"
#include "settings/RouterSettings.h"

namespace freerouting::datastructures {
class IdentifierType;
class IndentFileWriter;
} // namespace freerouting::datastructures

namespace freerouting::designforms::specctra::parser {

class IJFlexScanner;

/**
 * Reads and writes the (autoroute_settings ...) scope of a dsn file. Port of
 * Java parser.AutorouteSettings.
 */
class AutorouteSettings
{
public:
    AutorouteSettings() = delete;

    /**
     * Reads the (autoroute_settings ...) scope into a fresh RouterSettings.
     * Returns nullptr (std::nullopt) on an IO error or unexpected end of file.
     * Java AutorouteSettings.readScope.
     */
    static std::shared_ptr<settings::RouterSettings> read_scope(
        IJFlexScanner& p_scanner, const LayerStructure& p_layer_structure);

    /**
     * Reads a (layer_rule <layer> ...) scope into p_settings. Returns nullptr
     * on error. Java AutorouteSettings.readLayerRule.
     */
    static std::shared_ptr<settings::RouterSettings> read_layer_rule(
        IJFlexScanner& p_scanner, const LayerStructure& p_layer_structure,
        std::shared_ptr<settings::RouterSettings> p_settings);

    /**
     * Writes the (autoroute_settings ...) scope for the input settings.
     * Java AutorouteSettings.writeScope.
     */
    static void write_scope(datastructures::IndentFileWriter& p_file,
        const settings::RouterSettings& p_settings, const board::LayerStructure& p_layer_structure,
        const datastructures::IdentifierType& p_identifier_type);
};

} // namespace freerouting::designforms::specctra::parser