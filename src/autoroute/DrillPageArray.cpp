#include "autoroute/DrillPageArray.h"

#include <algorithm>
#include <cmath>

#include "autoroute/DrillPage.h"
#include "board/RoutingBoard.h"

namespace freerouting::autoroute {

namespace {
using geometry::planar::IntBox;

int compute_length(const IntBox& p_bounds)
{
    return static_cast<int>(p_bounds.ur.x - p_bounds.ll.x);
}

int compute_height(const IntBox& p_bounds)
{
    return static_cast<int>(p_bounds.ur.y - p_bounds.ll.y);
}
} // namespace

int DrillPageArray::calc_column_count(const IntBox& p_bounds, int p_max_page_width)
{
    if (p_max_page_width <= 0)
    {
        return 0;
    }
    int length = compute_length(p_bounds);
    int result = static_cast<int>(std::ceil(length / static_cast<double>(p_max_page_width)));
    return std::max(result, 1);
}

int DrillPageArray::calc_row_count(const IntBox& p_bounds, int p_max_page_width)
{
    if (p_max_page_width <= 0)
    {
        return 0;
    }
    int height = compute_height(p_bounds);
    int result = static_cast<int>(std::ceil(height / static_cast<double>(p_max_page_width)));
    return std::max(result, 1);
}

int DrillPageArray::calc_page_size(int p_length, int p_segment_count)
{
    if (p_segment_count <= 0)
    {
        return 0;
    }
    return static_cast<int>(std::ceil(p_length / static_cast<double>(p_segment_count)));
}

DrillPageArray::DrillPageArray(board::RoutingBoard* p_board, int p_max_page_width)
    : bounds_(p_board != nullptr ? p_board->get_bounding_box() : IntBox(0, 0, 0, 0)),
      column_count_(calc_column_count(bounds_, p_max_page_width)),
      row_count_(calc_row_count(bounds_, p_max_page_width)),
      page_width_(calc_page_size(compute_length(bounds_), column_count_)),
      page_height_(calc_page_size(compute_height(bounds_), row_count_))
{
    if (p_board == nullptr || column_count_ == 0 || row_count_ == 0)
    {
        return;
    }
    pages_.resize(row_count_);
    for (int j = 0; j < row_count_; ++j)
    {
        pages_[j].resize(column_count_);
        for (int i = 0; i < column_count_; ++i)
        {
            int ll_x = static_cast<int>(bounds_.ll.x) + i * page_width_;
            int ur_x = (i == column_count_ - 1) ? static_cast<int>(bounds_.ur.x) : ll_x + page_width_;
            int ll_y = static_cast<int>(bounds_.ll.y) + j * page_height_;
            int ur_y = (j == row_count_ - 1) ? static_cast<int>(bounds_.ur.y) : ll_y + page_height_;
            pages_[j][i] =
                std::make_unique<DrillPage>(IntBox(ll_x, ll_y, ur_x, ur_y), p_board);
        }
    }
}

void DrillPageArray::invalidate(const geometry::planar::IntBox& p_shape)
{
    for (auto* current_page : overlapping_pages(p_shape))
    {
        current_page->invalidate();
    }
}

std::vector<DrillPage*> DrillPageArray::overlapping_pages(const geometry::planar::IntBox& p_shape) const
{
    std::vector<DrillPage*> result;
    if (column_count_ == 0 || row_count_ == 0)
    {
        return result;
    }

    IntBox shape_box = p_shape.intersection(bounds_);
    if (shape_box.is_empty())
    {
        return result;
    }

    int min_j = static_cast<int>(
        std::floor(static_cast<double>(shape_box.ll.y - bounds_.ll.y) / page_height_));
    double max_j = static_cast<double>(shape_box.ur.y - bounds_.ll.y) / page_height_;

    int min_i = static_cast<int>(
        std::floor(static_cast<double>(shape_box.ll.x - bounds_.ll.x) / page_width_));
    double max_i = static_cast<double>(shape_box.ur.x - bounds_.ll.x) / page_width_;

    min_j = std::max(0, min_j);
    min_i = std::max(0, min_i);

    for (int j = min_j; j < max_j && j < row_count_; ++j)
    {
        for (int i = min_i; i < max_i && i < column_count_; ++i)
        {
            IntBox intersection = p_shape.intersection(pages_[j][i]->shape);
            if (intersection.dimension() > 1)
            {
                result.push_back(pages_[j][i].get());
            }
        }
    }
    return result;
}

void DrillPageArray::reset()
{
    for (auto& row : pages_)
    {
        for (auto& page : row)
        {
            page->reset();
        }
    }
}

DrillPage* DrillPageArray::get_page(int p_row, int p_col) const
{
    if (p_row < 0 || p_row >= row_count_ || p_col < 0 || p_col >= column_count_)
    {
        return nullptr;
    }
    return pages_[p_row][p_col].get();
}

} // namespace freerouting::autoroute