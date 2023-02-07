#include "Constants.hpp"

namespace Fiat128
{
    constexpr auto const_pow(long base, long exponent) -> long long
    {
        long long result = 1;
        for (long i = 0; i < exponent; ++i)
        {
            result *= base;
        }
        return result;
    }

    constexpr long long MEMORY_SIZE = const_pow(2, 16);

    struct EmulatorState
    {
        // 128-bit general purpose registers
        std::bitset<128> regs[8];

        // 128-bit general purpose vector registers
        std::bitset<128> vec_regs[8];

        // Timer register
        std::bitset<128> timer;

        // Stack pointer
        std::bitset<128> sp;

        // Segment index (stack)
        std::bitset<128> sis;

        // Segment limit (stack)
        std::bitset<128> sls;

        // Segment index (heap)
        std::bitset<128> sih;

        // Segment limit (heap)
        std::bitset<128> slh;

        // Segment index (code)
        std::bitset<128> sic;

        // Segment limit (code)
        std::bitset<128> slc;

        // Segment index (io)
        std::bitset<128> sii;

        // Segment limit (io)
        std::bitset<128> sli;

        // Accumulator
        std::bitset<128> acc;

        // Interrupt enable flag
        bool interrupt_enabled;

        // Memory
        std::bitset<128> memory[MEMORY_SIZE];

        // Program counter
        std::bitset<128> pc;
    };
}