#include "rules/ClearanceMatrix.h"

#include <algorithm>
#include <stdexcept>
#include <sstream>

#include "logger/FRLogger.h"

namespace freerouting {
namespace rules {

ClearanceMatrix::Row::Row(const std::string& p_name)
    : name(p_name)
{
}

ClearanceMatrix::ClearanceMatrix(int p_class_count,
                             const board::LayerStructure& p_layer_structure,
                             const std::vector<std::string>& p_name_arr)
    : layer_structure(p_layer_structure),
      max_value_on_layer(p_layer_structure.layer_count(), 0),
      class_count(std::max(p_class_count, 1)),
      default_classes(),
      default_value(0)
{
    for (int i = 0; i < class_count; ++i)
    {
        rows.emplace_back(p_name_arr[i]);
        rows[i].values.resize(class_count, 0);
    }
}

ClearanceMatrix ClearanceMatrix::get_default_instance(const board::LayerStructure& p_layer_structure,
                                                       int p_default_value)
{
    std::vector<std::string> name_arr = {"null", "default"};
    ClearanceMatrix result(2, p_layer_structure, name_arr);
    result.set_default_value(p_default_value);
    return result;
}

int ClearanceMatrix::get_no(const std::string& p_name) const
{
    for (int i = 0; i < class_count; ++i)
    {
        if (rows[i].name == p_name)
            return i;
    }
    return -1;
}

std::string ClearanceMatrix::get_name(int p_cl_class) const
{
    if (p_cl_class < 0 || p_cl_class >= class_count)
    {
        logger::FRLogger::instance().warn("ClearanceMatrix.get_name: p_cl_class out of range");
        return "";
    }
    return rows[p_cl_class].name;
}

void ClearanceMatrix::set_default_value(int p_value)
{
    for (int i = 0; i < static_cast<int>(layer_structure.layer_count()); ++i)
    {
        set_default_value(i, p_value);
    }
}

void ClearanceMatrix::set_default_value(int p_layer, int p_value)
{
    for (int i = 1; i < class_count; ++i)
    {
        for (int j = 1; j < class_count; ++j)
        {
            rows[i].values[j] = p_value;
        }
    }
    max_value_on_layer[p_layer] = p_value;
}

int ClearanceMatrix::get_value(int p_cl_class1, int p_cl_class2, int p_layer) const
{
    if (p_layer < 0 || p_layer >= static_cast<int>(layer_structure.layer_count()))
    {
        return 0;
    }
    if (p_cl_class1 < 0 || p_cl_class1 >= class_count ||
        p_cl_class2 < 0 || p_cl_class2 >= class_count)
    {
        return 0;
    }
    return rows[p_cl_class1].values[p_cl_class2];
}

void ClearanceMatrix::set_value(int p_cl_class1, int p_cl_class2, int p_layer, int p_value)
{
    if (p_layer < 0 || p_layer >= static_cast<int>(layer_structure.layer_count()))
    {
        return;
    }
    if (p_cl_class1 < 0 || p_cl_class1 >= class_count ||
        p_cl_class2 < 0 || p_cl_class2 >= class_count)
    {
        return;
    }
    rows[p_cl_class1].values[p_cl_class2] = p_value;
    rows[p_cl_class2].values[p_cl_class1] = p_value;

    if (p_value > max_value_on_layer[p_layer])
    {
        max_value_on_layer[p_layer] = p_value;
    }
}

int ClearanceMatrix::get_value(int p_cl_class1, int p_cl_class2) const
{
    if (p_cl_class1 < 0 || p_cl_class1 >= class_count ||
        p_cl_class2 < 0 || p_cl_class2 >= class_count)
    {
        return default_value;
    }
    return rows[p_cl_class1].values[p_cl_class2];
}

void ClearanceMatrix::set_value(int p_cl_class1, int p_cl_class2, int p_value)
{
    for (int i = 0; i < static_cast<int>(layer_structure.layer_count()); ++i)
    {
        set_value(p_cl_class1, p_cl_class2, i, p_value);
    }
}

int ClearanceMatrix::get_max_value_on_layer(int p_layer) const
{
    if (p_layer < 0 || p_layer >= static_cast<int>(max_value_on_layer.size()))
    {
        return 0;
    }
    return max_value_on_layer[p_layer];
}

int ClearanceMatrix::get_value(int p_cl_class1, int p_cl_class2, int p_layer, bool p_for_safety_check) const
{
    int value = get_value(p_cl_class1, p_cl_class2, p_layer);
    if (p_for_safety_check)
    {
        return value + clearance_safety_margin;
    }
    return value;
}

int ClearanceMatrix::get_clearance_compensation_value(int p_cl_class_no, int p_layer) const
{
    return 0;
}

std::string ClearanceMatrix::to_string() const
{
    std::ostringstream ss;
    ss << "ClearanceMatrix[class_count=" << class_count
       << ", layer_count=" << layer_structure.layer_count() << "]";
    return ss.str();
}

} // namespace rules
} // namespace freerouting