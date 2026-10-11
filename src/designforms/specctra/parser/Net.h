#pragma once

#include <optional>
#include <set>
#include <string>
#include <vector>

namespace freerouting::designforms::specctra::parser {

/**
 * A net read from the network scope of a dsn file. Port of the read part of Java parser.Net
 * (the write helpers belong to the DSN writer, T204c).
 */
class Net
{
public:
    /**
     * Net name plus subnet number. Java Net.Id.compareTo orders by name, then subnet number; the
     * defaulted comparison does the same (std::string comparison equals Java's for the ASCII names
     * of the test boards).
     */
    struct Id
    {
        std::string name;
        int subnet_number = 1;

        auto operator<=>(const Id&) const = default;
        bool operator==(const Id&) const = default;
    };

    /** A component pin of a net. Java Net.Pin, ordered by component name, then pin name. */
    struct Pin
    {
        std::string component_name;
        std::string pin_name;

        auto operator<=>(const Pin&) const = default;
        bool operator==(const Pin&) const = default;
    };

    explicit Net(Id p_id) : id(std::move(p_id)) {}

    const Id id;

    /** The pins of this net; nullopt (Java null) until set_pins was called. Java getPins. */
    const std::optional<std::set<Pin>>& get_pins() const { return pin_list_; }

    /** Java setPins: the pins are stored sorted (new TreeSet<>(pinList)). */
    void set_pins(const std::vector<Pin>& p_pins) { pin_list_ = std::set<Pin>(p_pins.begin(), p_pins.end()); }

private:
    std::optional<std::set<Pin>> pin_list_;
};

} // namespace freerouting::designforms::specctra::parser
