#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <variant>

namespace freerouting::designforms::specctra {

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

struct ComponentDef
{
    std::string name;
    std::string package;
    int64_t x, y;
    int rotation;
    bool placed;
};

struct Result
{
    bool success = false;
    std::string error_message;
    std::string board_name;
    int unit = 25400;
    int64_t board_width = 0;
    int64_t board_height = 0;
    std::vector<LayerDef> layers;
    std::vector<ViaDef> vias;
    std::vector<NetDef> nets;
    std::vector<PinDef> pins;
    std::vector<ComponentDef> components;
};

Result read_dsn(const std::string& p_filename);

} // namespace freerouting::designforms::specctra