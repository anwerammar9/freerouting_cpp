#pragma once

#include <memory>
#include <string>
#include <vector>

#include "board/Package.h"
#include "board/Padstacks.h"

namespace freerouting::board {

/**
 * The padstacks, packages and via padstacks of a board. Port of Java
 * core.library.BoardLibrary (logical parts are not ported yet).
 */
class BoardLibrary
{
public:
    explicit BoardLibrary(int p_board_layer_count);

    Padstacks& padstacks() { return padstacks_; }
    const Padstacks& padstacks() const { return padstacks_; }
    Packages& packages() { return packages_; }
    const Packages& packages() const { return packages_; }

    int via_padstack_count() const { return static_cast<int>(via_padstacks_.size()); }
    /** Java BoardLibrary.getViaPadstack(int); nullptr if p_no is out of range. */
    Padstack* get_via_padstack(int p_no) const;
    /** Java BoardLibrary.getViaPadstack(String) (case sensitive, like Java). */
    Padstack* get_via_padstack(const std::string& p_name) const;
    const std::vector<Padstack*>& get_via_padstacks() const { return via_padstacks_; }
    void set_via_padstacks(std::vector<Padstack*> p_padstacks) { via_padstacks_ = std::move(p_padstacks); }
    /** Appends p_padstack unless a via padstack with that name exists. Java BoardLibrary.addViaPadstack. */
    bool add_via_padstack(Padstack* p_padstack);
    /**
     * Returns the via padstack with the mirrored layer range of p_via_padstack,
     * or nullptr. Java BoardLibrary.getMirroredViaPadstack.
     */
    Padstack* get_mirrored_via_padstack(Padstack* p_via_padstack) const;

private:
    Padstacks padstacks_;
    Packages packages_;
    std::vector<Padstack*> via_padstacks_;
};

} // namespace freerouting::board
