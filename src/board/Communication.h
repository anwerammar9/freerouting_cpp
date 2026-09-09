#pragma once

#include <memory>
#include <string>

namespace freerouting::board {

class CoordinateTransform;

class Communication
{
public:
    Communication();
    Communication(const Communication&) = default;
    
    bool host_cad_is_eagle() const { return false; }
    bool host_is_old_kicad() const { return false; }
    bool host_cad_exists() const { return false; }

    int get_resolution() const { return resolution_; }
    void set_resolution(int p_resolution) { resolution_ = p_resolution; }

    std::string to_string() const { return "Communication"; }

private:
    int resolution_ = 1;
    std::string host_cad_;
    std::string host_version_;
};

} // namespace freerouting::board