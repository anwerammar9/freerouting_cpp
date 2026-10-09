#include "designforms/specctra/parser/Plane.h"

#include <algorithm>

#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "designforms/specctra/parser/Shape.h"
#include "logger/FRLogger.h"

namespace freerouting::designforms::specctra::parser {

namespace {

bool equals_ignore_case(const std::string& p_left, const std::string& p_right)
{
    return p_left.size() == p_right.size()
        && std::equal(p_left.begin(), p_left.end(), p_right.begin(),
            [](unsigned char p_a, unsigned char p_b) { return std::tolower(p_a) == std::tolower(p_b); });
}

} // namespace

bool Plane::read_scope(ReadScopeParameter& p_par) const
{
    auto& log = logger::FRLogger::instance();
    // read the net name
    const Token next_token = p_par.scanner.next_token();
    if (!next_token.is_string())
    {
        log.warn("Plane.read_scope: String expected at '" + p_par.scanner.get_scope_identifier() + "'");
        return false;
    }
    const std::string net_name = next_token.string_value;
    p_par.scanner.set_scope_identifier(net_name);
    // Cadence Allegro cuts out the pins on power planes, which leads to
    // performance problems when dividing a conduction area into convex pieces.
    const bool skip_window_scopes = equals_ignore_case(p_par.host_cad, "allegro");
    std::optional<Shape::ReadAreaScopeResult> conduction_area =
        Shape::read_area_scope(p_par.scanner, p_par.layer_structure.get(), skip_window_scopes);
    if (!conduction_area)
    {
        return false;
    }
    p_par.plane_list.push_back(ReadScopeParameter::PlaneInfo{ std::move(*conduction_area), net_name });
    return true;
}

} // namespace freerouting::designforms::specctra::parser
