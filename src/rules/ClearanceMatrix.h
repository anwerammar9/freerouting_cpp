#pragma once

#include <optional>
#include <string>
#include <vector>

#include "board/Layer.h"
#include "rules/DefaultItemClearanceClasses.h"

namespace freerouting {
namespace rules {

/**
 * The clearance values between clearance classes, per layer. Port of Java
 * rules.ClearanceMatrix: value(class_i, class_j, layer) is stored in
 * row[class_j].column[class_i].layer[layer] (not automatically symmetric).
 */
class ClearanceMatrix
{
public:
    static constexpr int clearance_safety_margin = 16;

    ClearanceMatrix(int p_class_count, const board::LayerStructure& p_layer_structure,
                 const std::vector<std::string>& p_name_arr);

    /** Classes "null" and "default", all default values p_default_value. Java getDefaultInstance. */
    static ClearanceMatrix get_default_instance(const board::LayerStructure& p_layer_structure,
                                               int p_default_value);

    /** The index of the class named p_name (ignoring case), or -1. Java getNo. */
    int get_no(const std::string& p_name) const;
    /** The name of a class; "" (Java null) with a warning if out of range. Java getName. */
    std::string get_name(int p_cl_class) const;

    /** Sets the value of all classes except "null" on all layers. Java setDefaultValue. */
    void set_default_value(int p_value);
    void set_default_value(int p_layer, int p_value);
    /** Java setValue: values are made non-negative and even. */
    void set_value(int p_cl_class1, int p_cl_class2, int p_layer, int p_value);
    void set_value(int p_cl_class1, int p_cl_class2, int p_value);
    /** Sets the value on all inner layers. Java setInnerValue. */
    void set_inner_value(int p_cl_class1, int p_cl_class2, int p_value);

    /** Java getValue(i, j, layer, false); 0 for out-of-range arguments. */
    int get_value(int p_cl_class1, int p_cl_class2, int p_layer) const;
    /** Java getValue; p_for_safety_check adds clearance_safety_margin. */
    int get_value(int p_cl_class1, int p_cl_class2, int p_layer, bool p_for_safety_check) const;
    /** C++ only: the value on layer 0. */
    int get_value(int p_cl_class1, int p_cl_class2) const { return get_value(p_cl_class1, p_cl_class2, 0); }

    /** Java maxValue(classI, layer): the maximum value in the row of the class. */
    int max_value(int p_cl_class, int p_layer) const;
    /** Java maxValue(layer). */
    int get_max_value_on_layer(int p_layer) const;
    bool is_layer_dependent(int p_cl_class1, int p_cl_class2) const;
    bool is_inner_layer_dependent(int p_cl_class1, int p_cl_class2) const;

    int get_class_count() const { return class_count_; }
    int get_layer_count() const { return layer_structure_.layer_count(); }

    /** Half of the clearance of the class to itself, rounded up. Java clearanceCompensationValue. */
    int get_clearance_compensation_value(int p_cl_class_no, int p_layer) const;

    /** Appends a class with the values of the default class; false if the name exists. Java appendClass. */
    bool append_class(const std::string& p_class_name);
    /** Removes the class with index p_index. Java removeClass. */
    void remove_class(int p_index);
    /** True, if the two classes have equal values to all classes. Java isEqual. */
    bool is_equal(int p_first, int p_second) const;

    std::string to_string() const;
    DefaultItemClearanceClasses& get_default_classes() { return default_classes_; }

private:
    struct Row
    {
        std::string name;
        /** column[class_i][layer] */
        std::vector<std::vector<int>> column;
        std::vector<int> max_value;
    };
    Row new_row(const std::string& p_name) const;

    board::LayerStructure layer_structure_;
    std::vector<int> max_value_on_layer_;
    int class_count_;
    std::vector<Row> rows_;
    DefaultItemClearanceClasses default_classes_;
};

} // namespace rules
} // namespace freerouting
