#pragma once

#include <string>
#include <vector>
#include <memory>

namespace freerouting::core {

class Package
{
public:
    struct Pin
    {
        int number;
        int64_t x, y;
        int layer;
    };

    Package(const std::string& p_name, int p_no);

    const std::string& get_name() const { return name_; }
    int get_no() const { return no_; }

    void add_pin(int p_number, int64_t p_x, int64_t p_y, int p_layer);
    const Pin* get_pin(int p_index) const;
    int get_pin_count() const { return static_cast<int>(pins_.size()); }

    std::string to_string() const;

private:
    std::string name_;
    int no_;
    std::vector<Pin> pins_;
};

class Packages
{
public:
    Packages();

    void add(Package* p_package);
    Package* get(int p_no) const;
    Package* get_by_name(const std::string& p_name) const;

    int count() const { return static_cast<int>(packages_.size()); }

private:
    std::vector<Package*> packages_;
    std::vector<std::string> names_;
};

} // namespace freerouting::core