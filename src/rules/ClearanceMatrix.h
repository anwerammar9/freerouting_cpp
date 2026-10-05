#pragma once

#include <string>
#include <vector>

#include "board/Layer.h"
#include "rules/DefaultItemClearanceClasses.h"

namespace freerouting {
namespace rules {

class ClearanceMatrix
{
public:
    static constexpr int clearance_safety_margin = 16;

    ClearanceMatrix(int p_class_count, const board::LayerStructure& p_layer_structure,
                 const std::vector<std::string>& p_name_arr);

    static ClearanceMatrix get_default_instance(const board::LayerStructure& p_layer_structure,
                                               int p_default_value);

    int get_no(const std::string& p_name) const;
    std::string get_name(int p_cl_class) const;

    void set_default_value(int p_value);
    void set_default_value(int p_layer, int p_value);

    int get_value(int p_cl_class1, int p_cl_class2, int p_layer) const;
    void set_value(int p_cl_class1, int p_cl_class2, int p_layer, int p_value);

    int get_value(int p_cl_class1, int p_cl_class2, int p_layer, bool p_for_safety_check) const;
    int get_clearance_compensation_value(int p_cl_class_no, int p_layer) const;

    int get_value(int p_cl_class1, int p_cl_class2) const;
    void set_value(int p_cl_class1, int p_cl_class2, int p_value);

    int get_class_count() const { return class_count; }
    int get_layer_count() const { return layer_structure.layer_count(); }
    int get_max_value_on_layer(int p_layer) const;

    std::string to_string() const;

    DefaultItemClearanceClasses& get_default_classes() { return default_classes; }

private:
    class Row
    {
    public:
        explicit Row(const std::string& p_name);

        std::string name;
        std::vector<int> values;
    };

    board::LayerStructure layer_structure;
    std::vector<int> max_value_on_layer;
    int class_count;
    std::vector<Row> rows;
    DefaultItemClearanceClasses default_classes;
    int default_value;
};

} // namespace rules
} // namespace freerouting