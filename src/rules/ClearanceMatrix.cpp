#include "rules/ClearanceMatrix.h"

#include <algorithm>
#include <cctype>
#include <climits>
#include <sstream>

#include "logger/FRLogger.h"

namespace freerouting {
namespace rules {

namespace {
bool equals_ignore_case(const std::string& p_a, const std::string& p_b)
{
    return p_a.size() == p_b.size()
        && std::equal(p_a.begin(), p_a.end(), p_b.begin(),
            [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); });
}
} // namespace

ClearanceMatrix::Row ClearanceMatrix::new_row(const std::string& p_name) const
{
    const int layer_count = layer_structure_.layer_count();
    Row row;
    row.name = p_name;
    row.column.assign(class_count_, std::vector<int>(layer_count, 0));
    row.max_value.assign(layer_count, 0);
    return row;
}

ClearanceMatrix::ClearanceMatrix(int p_class_count, const board::LayerStructure& p_layer_structure,
    const std::vector<std::string>& p_name_arr)
    : layer_structure_(p_layer_structure)
    , max_value_on_layer_(p_layer_structure.layer_count(), 0)
    , class_count_(std::max(p_class_count, 1))
{
    for (int i = 0; i < class_count_; ++i)
    {
        rows_.push_back(new_row(i < static_cast<int>(p_name_arr.size()) ? p_name_arr[i] : std::string()));
    }
}

ClearanceMatrix ClearanceMatrix::get_default_instance(const board::LayerStructure& p_layer_structure,
    int p_default_value)
{
    ClearanceMatrix result(2, p_layer_structure, { "null", "default" });
    result.set_default_value(p_default_value);
    return result;
}

int ClearanceMatrix::get_no(const std::string& p_name) const
{
    for (int i = 0; i < class_count_; ++i)
    {
        if (equals_ignore_case(rows_[i].name, p_name))
        {
            return i;
        }
    }
    return -1;
}

std::string ClearanceMatrix::get_name(int p_cl_class) const
{
    if (p_cl_class < 0 || p_cl_class >= static_cast<int>(rows_.size()))
    {
        logger::FRLogger::instance().warn("ClearanceMatrix.get_name: clearanceClassIndex out of range");
        return "";
    }
    return rows_[p_cl_class].name;
}

void ClearanceMatrix::set_default_value(int p_value)
{
    for (int layer = 0; layer < layer_structure_.layer_count(); ++layer)
    {
        set_default_value(layer, p_value);
    }
}

void ClearanceMatrix::set_default_value(int p_layer, int p_value)
{
    for (int i = 1; i < class_count_; ++i)
    {
        for (int j = 1; j < class_count_; ++j)
        {
            set_value(i, j, p_layer, p_value);
        }
    }
}

void ClearanceMatrix::set_value(int p_cl_class1, int p_cl_class2, int p_value)
{
    for (int layer = 0; layer < layer_structure_.layer_count(); ++layer)
    {
        set_value(p_cl_class1, p_cl_class2, layer, p_value);
    }
}

void ClearanceMatrix::set_value(int p_cl_class1, int p_cl_class2, int p_layer, int p_value)
{
    // Java indexes the arrays unchecked; out-of-range arguments are ignored here.
    if (p_cl_class1 < 0 || p_cl_class1 >= class_count_ || p_cl_class2 < 0 || p_cl_class2 >= class_count_
        || p_layer < 0 || p_layer >= layer_structure_.layer_count())
    {
        return;
    }
    Row& curr_row = rows_[p_cl_class2];
    int value = std::max(p_value, 0);
    if (value % 2 != 0)
    {
        value = value == INT_MAX ? value - 1 : value + 1;
    }
    curr_row.column[p_cl_class1][p_layer] = value;
    curr_row.max_value[p_layer] = std::max(curr_row.max_value[p_layer], value);
    max_value_on_layer_[p_layer] = std::max(max_value_on_layer_[p_layer], value);
}

void ClearanceMatrix::set_inner_value(int p_cl_class1, int p_cl_class2, int p_value)
{
    for (int layer = 1; layer < layer_structure_.layer_count() - 1; ++layer)
    {
        set_value(p_cl_class1, p_cl_class2, layer, p_value);
    }
}

int ClearanceMatrix::get_value(int p_cl_class1, int p_cl_class2, int p_layer) const
{
    return get_value(p_cl_class1, p_cl_class2, p_layer, false);
}

int ClearanceMatrix::get_value(int p_cl_class1, int p_cl_class2, int p_layer, bool p_for_safety_check) const
{
    if (p_cl_class1 < 0 || p_cl_class1 >= class_count_ || p_cl_class2 < 0 || p_cl_class2 >= class_count_
        || p_layer < 0 || p_layer >= layer_structure_.layer_count())
    {
        return 0;
    }
    const int value_from_the_matrix = rows_[p_cl_class2].column[p_cl_class1][p_layer];
    return p_for_safety_check ? value_from_the_matrix + clearance_safety_margin : value_from_the_matrix;
}

int ClearanceMatrix::max_value(int p_cl_class, int p_layer) const
{
    const int i = std::min(std::max(p_cl_class, 0), class_count_ - 1);
    const int layer = std::min(std::max(p_layer, 0), layer_structure_.layer_count() - 1);
    return rows_[i].max_value[layer];
}

int ClearanceMatrix::get_max_value_on_layer(int p_layer) const
{
    if (max_value_on_layer_.empty())
    {
        return 0;
    }
    const int layer = std::min(std::max(p_layer, 0), static_cast<int>(max_value_on_layer_.size()) - 1);
    return max_value_on_layer_[layer];
}

bool ClearanceMatrix::is_layer_dependent(int p_cl_class1, int p_cl_class2) const
{
    const std::vector<int>& values = rows_[p_cl_class2].column[p_cl_class1];
    for (size_t l = 1; l < values.size(); ++l)
    {
        if (values[l] != values[0])
        {
            return true;
        }
    }
    return false;
}

bool ClearanceMatrix::is_inner_layer_dependent(int p_cl_class1, int p_cl_class2) const
{
    const int layer_count = layer_structure_.layer_count();
    if (layer_count <= 2)
    {
        return false; // no inner layers
    }
    const std::vector<int>& values = rows_[p_cl_class2].column[p_cl_class1];
    for (int l = 2; l < layer_count - 1; ++l)
    {
        if (values[l] != values[1])
        {
            return true;
        }
    }
    return false;
}

int ClearanceMatrix::get_clearance_compensation_value(int p_cl_class_no, int p_layer) const
{
    return (get_value(p_cl_class_no, p_cl_class_no, p_layer, false) + 1) / 2;
}

bool ClearanceMatrix::append_class(const std::string& p_class_name)
{
    if (get_no(p_class_name) >= 0)
    {
        return false;
    }
    const int old_class_count = class_count_;
    ++class_count_;
    const int layer_count = layer_structure_.layer_count();
    for (Row& row : rows_)
    {
        row.column.push_back(std::vector<int>(layer_count, 0));
    }
    rows_.push_back(new_row(p_class_name));
    for (int i = 0; i < old_class_count; ++i)
    {
        for (int j = 0; j < layer_count; ++j)
        {
            const int default_value = get_value(1, i, j, false);
            set_value(old_class_count, i, j, default_value);
            set_value(i, old_class_count, j, default_value);
        }
    }
    for (int j = 0; j < layer_count; ++j)
    {
        set_value(old_class_count, old_class_count, j, get_value(1, 1, j, false));
    }
    return true;
}

void ClearanceMatrix::remove_class(int p_index)
{
    if (p_index < 0 || p_index >= class_count_)
    {
        return;
    }
    --class_count_;
    rows_.erase(rows_.begin() + p_index);
    for (Row& row : rows_)
    {
        row.column.erase(row.column.begin() + p_index);
        // Java parity: the new rows of removeClass get fresh (zero) maxValue arrays.
        row.max_value.assign(layer_structure_.layer_count(), 0);
    }
}

bool ClearanceMatrix::is_equal(int p_first, int p_second) const
{
    if (p_first == p_second)
    {
        return true;
    }
    if (p_first < 0 || p_second < 0 || p_first >= class_count_ || p_second >= class_count_)
    {
        return false;
    }
    for (int i = 1; i < class_count_; ++i)
    {
        if (rows_[p_first].column[i] != rows_[p_second].column[i])
        {
            return false;
        }
    }
    return true;
}

std::string ClearanceMatrix::to_string() const
{
    std::ostringstream ss;
    ss << "ClearanceMatrix[class_count=" << class_count_ << ", layer_count=" << layer_structure_.layer_count() << "]";
    return ss.str();
}

} // namespace rules
} // namespace freerouting
