#pragma once

#include <string>
#include <vector>

#include "geometry/planar/Shape.h"
#include "geometry/planar/IntBox.h"

namespace freerouting::board {

class Padstack
{
public:
    Padstack(const std::string& p_name);

    const std::string& get_name() const { return name_; }

    int get_from_layer() const { return from_layer_; }
    void set_from_layer(int p_layer) { from_layer_ = p_layer; }

    int get_to_layer() const { return to_layer_; }
    void set_to_layer(int p_layer) { to_layer_ = p_layer; }

    int get_drill() const { return drill_; }
    void set_drill(int p_drill) { drill_ = p_drill; }

    void add_shape(int p_layer, geometry::planar::Shape* p_shape);
    const geometry::planar::Shape* get_shape(int p_layer) const;

    geometry::planar::IntBox get_bounding_box() const;

    std::string to_string() const;

private:
    std::string name_;
    int from_layer_;
    int to_layer_;
    int drill_;
    std::vector<geometry::planar::Shape*> shapes_;
};

} // namespace freerouting::board