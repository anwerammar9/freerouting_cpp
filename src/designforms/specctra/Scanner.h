#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <variant>
#include <algorithm>
#include <cctype>

namespace freerouting::designforms::specctra {

inline std::string to_lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

struct Location
{
    int line = 0;
    int column = 0;
};

class Scanner
{
public:
    enum class TokenType
    {
        STRING,
        INTEGER,
        FLOAT,
        OPEN_PAREN,
        CLOSED_PAREN
    };

    struct Token
    {
        TokenType type;
        std::variant<std::string, int64_t, double> value;
        Location location;
    };

    explicit Scanner(const std::string& p_filename);
    explicit Scanner(std::istream& p_input);
    ~Scanner();

    bool is_open() const;

    Token next_token();
    bool expect(TokenType p_type);
    bool expect_keyword(const std::string& p_keyword);
    std::string read_string();
    int64_t read_int();
    double read_float();

    bool eof() const { return at_end_; }
    Location current_location() const { return cur_location_; }
    int current_level() const { return paren_level_; }

private:
    void read_char();

    std::istream* input_stream_ = nullptr;
    std::ifstream file_stream_;
    bool owns_stream_ = false;
    bool at_end_ = false;
    int cur_char_ = 0;
    int paren_level_ = 0;
    Location cur_location_;
};

struct LayerDef
{
    int index;
    std::string name;
    bool is_signal;
};

struct ViaDef
{
    std::string name;
    int drill;
    std::vector<int> layer_indices;
};

struct NetDef
{
    int id;
    std::string name;
    int class_index;
    std::vector<std::pair<std::string, std::string>> pins;
};

struct PinDef
{
    std::string component;
    std::string pin;
    int net_no;
    int64_t x, y;
};

struct ImagePinDef
{
    std::string name;
    std::string padstack;
    double x = 0;
    double y = 0;
};

struct PadstackDef
{
    std::string name;
    double width = 0;
    double height = 0;
};

struct ImageDef
{
    std::string name;
    std::vector<ImagePinDef> pins;
};

struct ComponentDef
{
    std::string name;
    std::string package;
    std::string footprint;
    int64_t x, y;
    int rotation;
    bool placed;
};

/**
 * A single wire from the DSN wiring scope. Represents one polyline trace, one
 * polygon trace or a conduction area (plane). Coordinates are kept in DSN
 * units (doubles) and converted to board coordinates when the board items are
 * materialized by BoardLoader.
 */
struct WireDef
{
    int layer_no = 0;                 // resolved from the layer name at parse time
    double width = 0;                 // full trace width in DSN units
    std::vector<double> coords;       // alternating x, y doubles (4 values per line segment)
    bool is_polygon_path = false;
    bool is_conduction_area = false;  // rect/polygon/circle wire (plane)
    std::vector<int> net_nos;
    int clearance_class = 0;
    bool fixed = false;
};

/**
 * A via placement from the DSN wiring scope, referencing a via padstack by
 * name (resolved against Result.vias when the board is materialized).
 */
struct ViaPlacementDef
{
    std::string padstack_name;
    std::vector<double> location;     // [x, y] in DSN units
    std::vector<int> net_nos;
    int clearance_class = 0;
    bool fixed = false;
};

struct Result
{
    bool success = false;
    std::string error_message;
    std::string board_name;
    int unit = 25400;
    int64_t board_width = 0;
    int64_t board_height = 0;
    int64_t board_min_x = 0;
    int64_t board_min_y = 0;
    std::vector<LayerDef> layers;
    std::vector<ViaDef> vias;
    std::vector<PadstackDef> padstacks;
    std::vector<ImageDef> images;
    std::vector<NetDef> nets;
    std::vector<PinDef> pins;
    std::vector<ComponentDef> components;
    std::vector<WireDef> wires;
    std::vector<ViaPlacementDef> via_placements;
};

Result read_dsn(const std::string& p_filename);
Result read_dsn(std::istream& p_input);

} // namespace freerouting::designforms::specctra