#include "autoroute/DrillPageArray.h"
#include "autoroute/DrillPage.h"
#include "board/RoutingBoard.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/TileShape.h"

#include <cmath>
#include <algorithm>

namespace freerouting {
namespace autoroute {

DrillPageArray::DrillPageArray(std::shared_ptr<RoutingBoard> p_board, int p_max_page_width)
    : bounds_(p_board->bounding_box)
{
    // Calculate array dimensions
    double length = bounds_.ur.x - bounds_.ll.x;
    double height = bounds_.ur.y - bounds_.ll.y;
    
    column_count_ = static_cast<int>(std::ceil(length / p_max_page_width));
    row_count_ = static_cast<int>(std::ceil(height / p_max_page_width));
    
    // Calculate page dimensions
    auto page_dims = calculate_page_dimensions(bounds_, p_max_page_width);
    page_width_ = page_dims.first;
    page_height_ = page_dims.second;
    
    // Initialize the page array
    page_arr_.resize(row_count_);
    for (int j = 0; j < row_count_; ++j) {
        page_arr_[j].resize(column_count_);
        
        for (int i = 0; i < column_count_; ++i) {
            int ll_x = bounds_.ll.x + i * page_width_;
            int ur_x;
            if (i == column_count_ - 1) {
                ur_x = bounds_.ur.x;
            } else {
                ur_x = ll_x + page_width_;
            }
            
            int ll_y = bounds_.ll.y + j * page_height_;
            int ur_y;
            if (j == row_count_ - 1) {
                ur_y = bounds_.ur.y;
            } else {
                ur_y = ll_y + page_height_;
            }
            
            auto page_bounds = std::make_shared<IntBox>(ll_x, ll_y, ur_x, ur_y);
            page_arr_[j][i] = std::make_shared<DrillPage>(page_bounds, p_board);
        }
    }
}

void DrillPageArray::invalidate(std::shared_ptr<TileShape> p_shape) {
    auto overlaps = overlapping_pages(p_shape);
    for (const auto& curr_page : overlaps) {
        curr_page->invalidate();
    }
}

std::vector<std::shared_ptr<DrillPage>> DrillPageArray::overlapping_pages(
    std::shared_ptr<TileShape> p_shape) const {
    std::vector<std::shared_ptr<DrillPage>> result;
    
    auto shape_box = p_shape->bounding_box()->intersection(bounds_);
    
    int min_j = get_page_index(shape_box.ll.y, page_height_, bounds_.ll.y);
    int max_j = get_page_index(shape_box.ur.y, page_height_, bounds_.ll.y);
    
    int min_i = get_page_index(shape_box.ll.x, page_width_, bounds_.ll.x);
    int max_i = get_page_index(shape_box.ur.x, page_width_, bounds_.ll.x);
    
    // Clamp indices to valid range
    min_j = std::max(0, min_j);
    max_j = std::min(row_count_, max_j);
    min_i = std::max(0, min_i);
    max_i = std::min(column_count_, max_i);
    
    for (int j = min_j; j < max_j; ++j) {
        for (int i = min_i; i < max_i; ++i) {
            if (is_valid_page(j, i)) {
                auto curr_page = page_arr_[j][i];
                auto intersection = p_shape->intersection(curr_page->shape);
                if (intersection->get_dimension() > 1) {
                    result.push_back(curr_page);
                }
            }
        }
    }
    
    return result;
}

void DrillPageArray::reset() {
    for (int j = 0; j < row_count_; ++j) {
        for (int i = 0; i < column_count_; ++i) {
            if (is_valid_page(j, i)) {
                page_arr_[j][i]->reset();
            }
        }
    }
}

void DrillPageArray::draw(void* p_graphics, 
                        const graphics::GraphicsContext& p_graphics_context, 
                        double p_intensity) const {
    for (int j = 0; j < row_count_; ++j) {
        for (int i = 0; i < column_count_; ++i) {
            if (is_valid_page(j, i)) {
                page_arr_[j][i]->draw(p_graphics, p_graphics_context, p_intensity);
            }
        }
    }
}

std::shared_ptr<DrillPage> DrillPageArray::get_page(int row, int col) const {
    if (is_valid_page(row, col)) {
        return page_arr_[row][col];
    }
    return nullptr;
}

std::pair<int, int> DrillPageArray::calculate_page_dimensions(const IntBox& bounds, 
                                                            int max_page_width) const {
    double length = bounds.ur.x - bounds.ll.x;
    double height = bounds.ur.y - bounds.ll.y;
    
    int column_count = static_cast<int>(std::ceil(length / max_page_width));
    int row_count = static_cast<int>(std::ceil(height / max_page_width));
    
    int page_width = static_cast<int>(std::ceil(length / column_count));
    int page_height = static_cast<int>(std::ceil(height / row_count));
    
    return std::make_pair(page_width, page_height);
}

int DrillPageArray::get_page_index(int coord, int page_size, int min_coord) const {
    return static_cast<int>(std::floor(static_cast<double>(coord - min_coord) / page_size));
}

bool DrillPageArray::is_valid_page(int row, int col) const {
    return row >= 0 && row < row_count_ && col >= 0 && col < column_count_;
}

} // namespace autoroute
} // namespace freerouting
