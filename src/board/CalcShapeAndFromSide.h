#pragma once

#include <memory>

namespace freerouting::geometry::planar
{
class TileShape;
}

namespace freerouting::board
{
class CalcFromSide;
class PolylineTrace;

/**
 * Used in the shove algorithm to calculate the fromside for pushing and to cut
 * off dog ears of the trace shape. Mirrors Java CalcShapeAndFromSide.java.
 */
class CalcShapeAndFromSide
{
public:
    /**
     * In the check shove functions, p_in_shove_check is expected to be true.
     * In the actual shove functions p_in_shove_check is expected to be false.
     */
    CalcShapeAndFromSide(const PolylineTrace& p_trace, int p_index, bool p_orthogonal,
                         bool p_in_shove_check);

    const geometry::planar::TileShape& shape() const { return *shape_; }

    /** Returns null if no fromside could be calculated. */
    const CalcFromSide* from_side() const { return from_side_.get(); }

private:
    std::shared_ptr<const geometry::planar::TileShape> shape_;
    std::unique_ptr<CalcFromSide> from_side_;
};

} // namespace freerouting::board