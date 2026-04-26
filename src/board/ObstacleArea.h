#pragma once

#include <memory>
#include <string>
#include <vector>

#include "board/Item.h"

namespace freerouting {
namespace board {

class ObstacleArea : public Item
{
public:
    ObstacleArea() = default;

    explicit ObstacleArea(const std::string& p_name);

    const std::string& get_name() const { return name_; }
    void set_name(const std::string& p_name) { name_ = p_name; }

    std::shared_ptr<class Area> get_area() const;
    void set_area(const std::shared_ptr<class Area>& p_area);

    int get_layer() const { return layer_; }
    void set_layer(int p_layer) { layer_ = p_layer; }

    std::shared_ptr<TileShape> get_tile_shape() const;

    std::string to_string() const override;

    std::unique_ptr<Item> clone() const override;

private:
    std::string name_;
    std::shared_ptr<class Area> relative_area_;
    int layer_ = 0;
    std::shared_ptr<class Vector> translation_;
    double rotation_in_degree_ = 0.0;
    bool side_changed_ = false;
};

} // namespace board
} // namespace freerouting