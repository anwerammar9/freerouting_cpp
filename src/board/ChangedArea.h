#pragma once

#include <vector>
#include <memory>

#include "geometry/planar/IntBox.h"

namespace freerouting::board {

class ChangedArea
{
public:
    ChangedArea(int p_layer_count);
    
    void join(int64_t p_x, int64_t p_y, int p_layer);
    void join(int64_t p_right, int64_t p_top, int64_t p_left, int64_t p_bottom, int p_layer);
    
    bool is_empty() const;
    void clear();
    
    int layer_count() const { return layer_count_; }

private:
    int layer_count_;
    std::vector<int64_t> right_;
    std::vector<int64_t> top_;
    std::vector<int64_t> left_;
    std::vector<int64_t> bottom_;
};

} // namespace freerouting::board