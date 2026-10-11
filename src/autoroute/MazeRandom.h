#pragma once

#include <cstdint>

namespace freerouting::autoroute {

/**
 * Linear congruential pseudo random generator of the maze search. Port of
 * java.util.Random as used by Java MazeSearchEngine.randomGenerator, so that
 * the ripup randomization of the C++ port produces the same values as the Java
 * autorouter for the same seed.
 *
 * Java keeps a 48 bit state which is scrambled with the multiplier
 * 0x5DEECE66D and the increment 0xB. next(bits) returns the p_bits highest
 * bits of the state and nextDouble() combines two such calls.
 */
class MazeRandom
{
public:
    static constexpr uint64_t kMultiplier = 0x5DEECE66DULL;
    static constexpr uint64_t kAddend = 0xBULL;
    static constexpr uint64_t kMask = (1ULL << 48) - 1;

    MazeRandom() { set_seed(0); }

    explicit MazeRandom(int64_t p_seed) { set_seed(p_seed); }

    /**
     * Sets the state from p_seed. Mirrors java.util.Random.setSeed(long),
     * including the initial scrambling of the seed.
     */
    void set_seed(int64_t p_seed)
    {
        seed_ = (static_cast<uint64_t>(p_seed) ^ kMultiplier) & kMask;
    }

    /**
     * Returns the p_bits highest bits of the next state. Mirrors the
     * protected java.util.Random.next(int).
     */
    int32_t next(int p_bits)
    {
        seed_ = (seed_ * kMultiplier + kAddend) & kMask;
        return static_cast<int32_t>(static_cast<int64_t>(seed_ >> (48 - p_bits)));
    }

    /**
     * Returns a double in [0,1). Mirrors java.util.Random.nextDouble(), which
     * is built from the 26 highest and the 27 following bits of the state.
     */
    double next_double()
    {
        const int64_t high = static_cast<int64_t>(next(26)) << 27;
        const int64_t low = static_cast<int64_t>(next(27));
        return static_cast<double>(high + low) * (1.0 / 9007199254740992.0);
    }

private:
    uint64_t seed_ = 0;
};

} // namespace freerouting::autoroute
