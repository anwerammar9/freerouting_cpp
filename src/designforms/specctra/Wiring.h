#pragma once

#include <memory>
#include <string>
#include <vector>

namespace freerouting {
namespace designforms {
namespace specctra {

class Wiring
{
public:
    Wiring() = default;

    bool read_scope(const class ReadScopeParameter& p_par);
    void write_scope(const class WriteScopeParameter& p_par);

private:
    bool read_wire_scope(const class ReadScopeParameter& p_par);
    bool read_via_scope(const class ReadScopeParameter& p_par);
    void write_via_scope(const class WriteScopeParameter& p_par, const std::shared_ptr<class Via>& p_via);
    void write_wire_scope(const class WriteScopeParameter& p_par, const std::shared_ptr<class Trace>& p_trace);
};

} // namespace specctra
} // namespace designforms
} // namespace freerouting