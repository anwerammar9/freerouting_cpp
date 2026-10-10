#pragma once

#include <cstdint>
#include <utility>
#include <vector>

namespace freerouting::datastructures {

/**
 * Bit-exact port of java.util.Random (48 bit linear congruential generator).
 *
 * Several Java classes ported here depend on its exact output: PolygonShape picks the
 * start corner of its convex split with nextInt, PlanarDelaunayTriangulation shuffles its
 * input with Collections.shuffle. Use this class instead of <random> wherever Java uses
 * java.util.Random. Verified against the JDK (see JavaRandom_test.cpp).
 */
class JavaRandom
{
public:
    static constexpr uint64_t kMultiplier = 0x5DEECE66DULL;
    static constexpr uint64_t kAddend = 0xBULL;
    static constexpr uint64_t kMask = (1ULL << 48) - 1;

    explicit JavaRandom(int64_t p_seed) { set_seed(p_seed); }

    /** java.util.Random.setSeed(long): the seed is scrambled with the multiplier. */
    void set_seed(int64_t p_seed) { seed_ = (static_cast<uint64_t>(p_seed) ^ kMultiplier) & kMask; }

    /** The protected java.util.Random.next(int): the p_bits highest bits of the new state. */
    int32_t next(int p_bits)
    {
        seed_ = (seed_ * kMultiplier + kAddend) & kMask;
        return static_cast<int32_t>(static_cast<uint32_t>(seed_ >> (48 - p_bits)));
    }

    /** java.util.Random.nextInt(). */
    int32_t next_int() { return next(32); }

    /**
     * java.util.Random.nextInt(int bound), including the power-of-two shortcut and the
     * rejection loop with Java's signed 32 bit overflow check. p_bound must be positive.
     */
    int32_t next_int(int32_t p_bound)
    {
        if ((p_bound & -p_bound) == p_bound)
        {
            return static_cast<int32_t>((static_cast<int64_t>(p_bound) * static_cast<int64_t>(next(31))) >> 31);
        }
        int32_t bits;
        int32_t val;
        do
        {
            bits = next(31);
            val = bits % p_bound;
            // Java: while (bits - val + (bound - 1) < 0), evaluated with int overflow
        } while (static_cast<int32_t>(static_cast<uint32_t>(bits) - static_cast<uint32_t>(val)
                     + static_cast<uint32_t>(p_bound - 1))
            < 0);
        return val;
    }

    /** java.util.Random.nextDouble(). */
    double next_double()
    {
        const int64_t high = static_cast<int64_t>(next(26)) << 27;
        const int64_t low = static_cast<int64_t>(next(27));
        return static_cast<double>(high + low) * (1.0 / 9007199254740992.0);
    }

private:
    uint64_t seed_ = 0;
};

/**
 * java.util.Collections.shuffle(List, Random): for i = size down to 2 swap(i - 1, nextInt(i)).
 * Java uses the same index sequence for array-backed and linked lists, so this matches both.
 */
template <class T>
void java_shuffle(std::vector<T>& p_list, JavaRandom& p_random)
{
    for (int i = static_cast<int>(p_list.size()); i > 1; --i)
    {
        std::swap(p_list[i - 1], p_list[p_random.next_int(i)]);
    }
}

} // namespace freerouting::datastructures
