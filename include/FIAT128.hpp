#pragma once

#include <algorithm>
#include <array>
#include <assert.h>
#include <bitset>
#include <chrono>
#include <deque>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <limits>
#include <map>
#include <math.h>
#include <memory>
#include <mutex>
#include <numbers>
#include <optional>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// Set value to 1 to use the Profiling system
#define PROFILING 1

#include "Profiling/Instrumentor.hpp"
#include "Profiling/Timer.hpp"

// define DEBUG macros here
#ifdef DEBUG

#endif

inline const int kCORE_COUNT = static_cast<int>(std::thread::hardware_concurrency()); // cpu cores, multi-threading

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <direct.h>
#include <windows.h>

// inline const std::string BINARY_DIRECTORY(std::string(_getcwd(NULL, 0)) + '/');
inline const std::string BINARY_DIRECTORY = std::filesystem::current_path().string(); // for referencing any file

#else

#include <unistd.h>

// inline const std::string BINARY_DIRECTORY_TEST(std::string(get_current_dir_name()) + "/");
inline const std::string BINARY_DIRECTORY = std::filesystem::current_path().string();

#endif

#define kEpsilon 0.000001

namespace Fiat128
{
    /**
     * @brief A random number generator
     *
     * @tparam T The type of the random number
     *
     * @param min The minimum value of the random number
     * @param max The maximum value of the random number
     */
    template <typename T>
    inline T random(T min = 0.0, T max = 1.0)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(min, max);
        return (T)dis(gen);
    }

    /**
     * @brief Map a value from one range to another
     *
     * @tparam T The type of the value
     * @param value The value to map
     * @param min The minimum value of the range
     * @param max The maximum value of the range
     * @param new_min The minimum value of the new range
     * @param new_max The maximum value of the new range
     * @return constexpr T
     */
    template <typename T>
    inline constexpr T map_to_range(T value, T min, T max, T new_min, T new_max)
    {
        return (T)(((value - min) / (max - min)) * (new_max - new_min) + new_min);
    }

    /**
     * @brief Prints the variables given to the console
     *
     * @tparam First stringstream-able type
     * @tparam Strings restof the stringstream-able types
     * @param arg
     * @param rest
     */
    template <typename First, typename... Strings>
    void print_by_force(First arg, [[maybe_unused]] const Strings &...rest)
    {
        std::cout << arg;
        if constexpr (sizeof...(rest) > 0) [[likely]]
        {
            print_by_force(rest...);
        }
    }

    /**
     * @brief Prints the variables given to the console in an async manner
     *
     * @tparam First stringstream-able type
     * @tparam Strings restof the stringstream-able types
     * @param arg
     * @param rest
     */
    template <typename First, typename... Strings>
    void async_print_by_force(const First arg, const Strings &...rest)
    {
        std::thread t([&]()
                      { print_by_force(arg, rest...); std::cout << std::endl; });
        t.detach();
    }

#ifdef DEBUG

#define debug_print(x, y) \
    print_by_force(x, y); \
    std::cout << std::endl;
#define debug_async_print(x, y) async_print_by_force(x, y)
#else
#define debug_print(x, y)
#define debug_async_print(x, y)

#endif

    /**
     * @brief Checks if the given class is of type Base
     *
     * @tparam Base base class
     * @tparam T subclass to check
     * @param T
     */
    template <typename Base, typename T>
    inline constexpr int instanceof (const T *)
    {
        return std::is_base_of<Base, T>::value;
    }

    struct EmulatorState;

    template <size_t T>
    struct Register
    {
        bool array[T] = {0};
        size_t size = T;
        bool overflow = false;

        void set_data(size_t (&data)[T])
        {
            for (size_t i = 0; i < T; i++)
            {
                array[i] = data[i];
            }
        }

        void set_bit(size_t index, bool value)
        {
            array[index] = value;

            overflow = false;
        }

        auto get_bit(size_t index) -> bool
        {
            return array[index];
        }

        void reset_overflow()
        {
            overflow = false;
        }

        auto get_overflow() -> bool
        {
            return overflow;
        }

        void clear()
        {
            for (size_t i = 0; i < T; i++)
            {
                array[i] = 0;
            }

            overflow = false;
        }

        auto is_zero() -> bool
        {
            for (size_t i = 0; i < T; i++)
            {
                if (array[i] == 1)
                {
                    return false;
                }
            }
            return true;
        }

        void inc()
        {
            // inc and set overflow
            for (size_t i = T - 1; i > 0; i--)
            {
                if (array[i] == 0)
                {
                    array[i] = 1;
                    break;
                }
                else
                {
                    array[i] = 0;
                }
            }

            if (is_zero())
                overflow = true;
        }

        auto operator++() -> Register<T> &
        {
            inc();
            return *this;
        }

        auto operator+=(const long long value)
        {
            for (size_t i = 0; i < value; i++)
            {
                inc();
            }
        }

        auto operator~()
        {
            Register<T> result;

            for (size_t i = 0; i < T; i++)
            {
                result.array[i] = !array[i];
            }

            return result;
        }

        auto operator=(const Register<T> &other)
        {
            for (size_t i = 0; i < T; i++)
            {
                array[i] = other.array[i];
            }
        }

        auto operator=(const bool other)
        {
            for (size_t i = 0; i < T; i++)
            {
                array[i] = other;
            }
        }

        auto operator+(const Register<T> &other) -> Register<T>
        {
            Register<T> result;
            bool carry = false;
            for (size_t i = T - 1; i > 0; i--)
            {
                if (array[i] == 0 && other.array[i] == 0)
                {
                    if (carry)
                    {
                        result.array[i] = 1;
                        carry = false;
                    }
                    else
                    {
                        result.array[i] = 0;
                    }
                }
                else if (array[i] == 1 && other.array[i] == 1)
                {
                    if (carry)
                    {
                        result.array[i] = 1;
                    }
                    else
                    {
                        result.array[i] = 0;
                        carry = true;
                    }
                }
                else
                {
                    if (carry)
                    {
                        result.array[i] = 0;
                    }
                    else
                    {
                        result.array[i] = 1;
                    }
                }
            }

            if (carry)
                result.overflow = true;

            return result;
        }

        auto operator-(const Register<T> &other) -> Register<T>
        {

            Register<T> reverse;

            for (size_t i = 0; i < T; i++)
            {
                reverse.array[i] = !other.array[i];
            }

            Register<T> one;
            one.set_bit(T - 1, 1);

            Register<T> two_complement = reverse + one;

            return *this + two_complement;
        }

        auto operator&(const Register<T> &other) -> Register<T>
        {
            Register<T> result;
            for (size_t i = 0; i < T; i++)
            {
                result.array[i] = array[i] & other.array[i];
            }
            return result;
        }

        auto operator|(const Register<T> &other) -> Register<T>
        {
            Register<T> result;
            for (size_t i = 0; i < T; i++)
            {
                result.array[i] = array[i] | other.array[i];
            }
            return result;
        }

        auto operator^(const Register<T> &other) -> Register<T>
        {
            Register<T> result;
            for (size_t i = 0; i < T; i++)
            {
                result.array[i] = array[i] ^ other.array[i];
            }
            return result;
        }

        auto operator<<(const size_t value) -> Register<T>
        {
            Register<T> result;
            for (size_t i = 0; i < T; i++)
            {
                if (i + value < T)
                {
                    result.array[i + value] = array[i];
                }
            }
            return result;
        }

        auto operator>>(const size_t value) -> Register<T>
        {
            Register<T> result;
            for (size_t i = 0; i < T; i++)
            {
                if (i - value >= 0)
                {
                    result.array[i - value] = array[i];
                }
            }
            return result;
        }

        auto operator==(const Register<T> &other) -> bool
        {
            for (size_t i = 0; i < T; i++)
            {
                if (array[i] != other.array[i])
                {
                    return false;
                }
            }
            return true;
        }

        auto operator>(const Register<T> &other) -> bool
        {
            for (size_t i = 0; i < T; i++)
            {
                if (array[i] == 1 && other.array[i] == 0)
                {
                    return true;
                }
                else if (array[i] == 0 && other.array[i] == 1)
                {
                    return false;
                }
            }
            return false;
        }

        auto get_byte(size_t index) -> Register<8>
        {
            Register<8> byte;
            for (size_t i = 0; i < 8; i++)
            {
                byte.array[i] = array[index * 8 + i];
            }
            return byte;
        }

        auto get_address() -> Register<24>
        {
            Register<24> byte;
            for (size_t i = 8; i < 32; i++)
            {
                byte.array[i - 8] = array[i];
            }

            return byte;
        }

        auto set_byte(size_t index, Register<8> byte)
        {
            for (size_t i = 0; i < 8; i++)
            {
                array[index * 8 + i] = byte.array[i];
            }
        }

        auto get_byte_as_char(size_t index) -> unsigned char
        {
            Register<8> byte = get_byte(index);
            std::bitset<8> bitset;
            for (size_t i = 0; i < 8; i++)
            {
                bitset[i] = byte.array[i];
            }
            return (unsigned char)bitset.to_ulong();
        }

        auto get_value() -> unsigned long long
        {
            std::bitset<T> bitset;
            for (size_t i = 0; i < T; i++)
            {
                bitset[i] = array[i];
            }
            return bitset.to_ullong();
        }
    };

    // enum INSTRUCTION : uint8_t
    // {
    //     // Arithmetic instructions
    //     ADD = 0x01, // Adds two operands and stores the result in the destination operand
    //     SUB = 0x02, // Subtracts two operands and stores the result in the destination operand
    //     AND = 0x03, // Performs bitwise AND on two operands and stores the result in the destination operand
    //     OR = 0x04,  // Performs bitwise OR on two operands and stores the result in the destination operand
    //     XOR = 0x05, // Performs bitwise XOR on two operands and stores the result in the destination operand

    //     // Data transfer instructions
    //     MOVE = 0x06, // Copies data from source operand to destination operand
    //     BUN = 0x07,  // Unconditionally branches to the target address operand
    //     BIZ = 0x08,  // Branches to the target address operand if the source operand is zero
    //     BNZ = 0x09,  // Branches to the target address operand if the source operand is not zero
    //     RET = 0x0A,  // Returns from the current function call

    //     // Memory access instructions
    //     MALLOC = 0x0B, // Allocates memory on the heap with the size specified in the size operand
    //     DALLOC = 0x0C, // Deallocates memory at the address specified in the source operand
    //     LDA = 0x0D,    // Loads the data at the source operand into the destination operand
    //     STA = 0x0E,    // Stores the data from the source operand into the memory at the destination operand

    //     // Comparison instructions
    //     EQ = 0x0F, // Compares two operands and sets the destination operand to 1 if they are equal, 0 otherwise
    //     NE = 0x10, // Compares two operands and sets the destination operand to 1 if they are not equal, 0 otherwise
    //     GT = 0x11, // Compares two operands and sets the destination operand to 1 if the first operand is greater than the second, 0 otherwise
    //     GE = 0x12, // Compares two operands and sets the destination operand to 1 if the first operand is greater than or equal to the second, 0 otherwise
    //     LT = 0x13, // Compares two operands and sets the destination operand to 1 if the first operand is less than the second, 0 otherwise
    //     LE = 0x14, // Compares two operands and sets the destination operand to 1 if the first operand is less than or equal to the second, 0 otherwise

    //     // Shift instructions
    //     SHL = 0x15, // Shifts the bits of the source operand left by the shift amount specified in the third operand
    //     SHR = 0x16, // Shifts the bits of the source operand right by the shift amount specified in the third operand

    //     // Rotate instructions
    //     ROL = 0x17, // Rotates the bits of the source operand left by the rotate amount specified in the third operand
    //     ROR = 0x18, // Rotates the bits of the source operand right by the rotate amount specified in the third operand

    //     // System instructions
    //     HALT = 0x19, // Halts the CPU

    //     // Vector instructions
    //     VADD = 0x1A, // Adds two vector operands and stores the result in the destination vector operand
    //     VSUB = 0x1B, // Subtracts two vector operands and stores the result in the destination vector operand
    //     VAND = 0x1C, // Performs bitwise AND on two vector operands and stores the result in the destination vector operand
    //     VOR = 0x1D,  // Performs bitwise OR on two vector operands and stores the result in the destination vector operand
    //     VXOR = 0x1E, // Performs bitwise XOR on two vector operands and stores the result in the destination vector operand
    //     VEQ = 0x1F,  // Compares two vector operands and sets a flag to 1 if they are equal, 0 otherwise
    //     VNE = 0x20,  // Compares two vector operands and sets a flag to 1 if they are not equal, 0 otherwise
    // };

    struct Instruction
    {
        std::string_view name;
        void (*opcode)(EmulatorState &) = nullptr;
        char cycles = 0;

        static auto decode_from_opcode(Register<8> opcode) -> Instruction;
    };

    // instruction function declarations
    // void ADD(Register<128> &dest, Register<128> &src1, Register<128> &src2);
    // void SUB(Register<128> &dest, Register<128> &src1, Register<128> &src2);
    // void AND(Register<128> &dest, Register<128> &src1, Register<128> &src2);
    // void OR(Register<128> &dest, Register<128> &src1, Register<128> &src2);
    // void XOR(Register<128> &dest, Register<128> &src1, Register<128> &src2);
    // void MOVE(Register<128> &dest, Register<128> &src1);
    // void BUN();
    // void BIZ(Register<128> &dest);
    // void BNZ(Register<128> &dest);
    // void RET();
    // void LDA(Register<128> &dest);
    // void STA(Register<128> &dest);
    // void EQ(Register<128> &src1, Register<128> &src2);
    // void GT(Register<128> &src1, Register<128> &src2);
    // void SHL(Register<128> &src1);
    // void SHR(Register<128> &src1);
    // void ROL(Register<128> &src1);
    // void ROR(Register<128> &src1);
    // void HALT();
    // void VADD(Register<128> &dest, Register<128> &src1, Register<128> &src2);
    // void VSUB(Register<128> &dest, Register<128> &src1, Register<128> &src2);
    // void VAND(Register<128> &dest, Register<128> &src1, Register<128> &src2);
    // void VOR(Register<128> &dest, Register<128> &src1, Register<128> &src2);
    // void VXOR(Register<128> &dest, Register<128> &src1, Register<128> &src2);
    // void VEQ(Register<128> &src1, Register<128> &src2);
    // void VGT(Register<128> &src1, Register<128> &src2);
    // void XXX();

    void ADD(EmulatorState &state);
    void SUB(EmulatorState &state);
    void AND(EmulatorState &state);
    void OR(EmulatorState &state);
    void XOR(EmulatorState &state);
    void MOV(EmulatorState &state);
    void BUN(EmulatorState &state);
    void BIZ(EmulatorState &state);
    void BNZ(EmulatorState &state);
    void LDA(EmulatorState &state);
    void STA(EmulatorState &state);
    void EQ(EmulatorState &state);
    void GT(EmulatorState &state);
    void SHL(EmulatorState &state);
    void SHR(EmulatorState &state);
    void ROL(EmulatorState &state);
    void ROR(EmulatorState &state);
    void HALT(EmulatorState &state);
    void XXX(EmulatorState &state);

    constexpr int instruction_count = 19;

    // instrcution table
    constexpr Instruction instruction_table[instruction_count] = {
        {"ADD", &ADD, 1},
        {"SUB", &SUB, 1},
        {"AND", &AND, 1},
        {"OR", &OR, 1},
        {"XOR", &XOR, 1},
        {"MOV", &MOV, 1},
        {"BUN", &BUN, 1},
        {"BIZ", &BIZ, 2},
        {"BIN", &BNZ, 2},
        {"LDA", &LDA, 2},
        {"STA", &STA, 2},
        {"EQ", &EQ, 1},
        {"GT", &GT, 1},
        {"SHL", &SHL, 1},
        {"SHR", &SHR, 1},
        {"ROL", &ROL, 1},
        {"ROR", &ROR, 1},
        {"HALT", &HALT, 1},
        {"XXX", &XXX, 1},
    };

    auto Instruction::decode_from_opcode(Register<8> opcode) -> Instruction
    {
        unsigned char opcode_as_char = opcode.get_byte_as_char(0);

        if (opcode_as_char > instruction_count || opcode_as_char < 0)
            return instruction_table[instruction_count];

        return instruction_table[opcode_as_char];
    }

    constexpr auto const_pow(long base, long exponent) -> long long
    {
        long long result = 1;
        for (long i = 0; i < exponent; ++i)
        {
            result *= base;
        }
        return result;
    }

#ifdef STATIC_MEMORY

    constexpr long long MEMORY_SIZE = const_pow(2, 16);

#else

    constexpr long long MEMORY_SIZE = const_pow(2, 24);

#endif // STATIC_MEMORY

    struct EmulatorState
    {
        // 128-bit general purpose registers
        Register<128> reg[8];

        // 128-bit general purpose vector registers
        Register<128> vec_regs[8];

        // Timer register
        Register<128> timer;

        // Stack pointer
        Register<128> sp;

        // Segment index (stack)
        Register<128> sis;

        // Segment limit (stack)
        Register<128> sls;

        // Segment index (heap)
        Register<128> sih;

        // Segment limit (heap)
        Register<128> slh;

        // Segment index (code)
        Register<128> sic;

        // Segment limit (code)
        Register<128> slc;

        // Segment index (io)
        Register<128> sii;

        // Segment limit (io)
        Register<128> sli;

        // Accumulator
        Register<32> acc;

        // Interrupt enable flag
        bool interrupt_enabled;

        // Program counter
        Register<128> pc;

#ifdef STATIC_MEMORY
        // Memory
        Register<128> memory[MEMORY_SIZE];
#else
        // Memory
        Register<128> memory[MEMORY_SIZE];

#endif // STATIC_MEMORY

        // current instruction
        Instruction current_instruction;

        // Flags
        bool interupt_flag;
        bool overflow_flag;
        bool HLT_flag;

        // cpu cycles
        long long total_cpu_cycles = 0;
        char instruction_cycle = 0;
    };

    // get word from the memory
    auto get_word(EmulatorState &state) -> Register<128>
    {
        Register<128> word = state.memory[state.pc.get_value()];
        state.pc += 1;
        return word;
    }

    // decode instruction
    auto decode_instruction(EmulatorState &state)
    {
        Register<8> opcode = get_word(state).get_byte(0);
        state.current_instruction = Instruction::decode_from_opcode(opcode);
    }

    // // execute instruction
    // auto execute_instruction(EmulatorState &state)
    // {
    //     state.instruction_cycle = 1;
    //     state.total_cpu_cycles++;
    //     state.current_instruction.opcode();
    // }

    // instruction function definitions
    void ADD(EmulatorState &state)
    {
        auto word = get_word(state);

        auto dest = state.reg[word.get_byte(1).get_value()];
        auto src1 = state.reg[word.get_byte(2).get_value()];
        auto src2 = state.reg[word.get_byte(3).get_value()];

        dest = src1 + src2;
    }

    void SUB(EmulatorState &state)
    {
        auto word = get_word(state);

        auto dest = state.reg[word.get_byte(1).get_value()];
        auto src1 = state.reg[word.get_byte(2).get_value()];
        auto src2 = state.reg[word.get_byte(3).get_value()];

        dest = src1 - src2;
    }

    void AND(EmulatorState &state)
    {
        auto word = get_word(state);

        auto dest = state.reg[word.get_byte(1).get_value()];
        auto src1 = state.reg[word.get_byte(2).get_value()];
        auto src2 = state.reg[word.get_byte(3).get_value()];

        dest = src1 & src2;
    }

    void OR(EmulatorState &state)
    {
        auto word = get_word(state);

        auto dest = state.reg[word.get_byte(1).get_value()];
        auto src1 = state.reg[word.get_byte(2).get_value()];
        auto src2 = state.reg[word.get_byte(3).get_value()];

        dest = src1 | src2;
    }

    void XOR(EmulatorState &state)
    {
        auto word = get_word(state);

        auto dest = state.reg[word.get_byte(1).get_value()];
        auto src1 = state.reg[word.get_byte(2).get_value()];
        auto src2 = state.reg[word.get_byte(3).get_value()];

        dest = src1 ^ src2;
    }

    void MOV(EmulatorState &state)
    {
        auto word = get_word(state);

        auto dest = state.reg[word.get_byte(1).get_value()];
        auto src = state.reg[word.get_byte(2).get_value()];

        dest = src;
    }

    // branch unconditionally
    void BUN(EmulatorState &state)
    {
        auto word = get_word(state);

        auto dest = state.reg[word.get_address().get_value()];

        state.pc = dest;
    }

    // branch if zero
    void BIZ(EmulatorState &state)
    {
        auto word = get_word(state);

        auto dest = state.reg[word.get_address().get_value()];

        if (state.acc.get_value() == 0)
        {
            state.pc = dest;
        }
    }

    // branch if not zero
    void BNZ(EmulatorState &state)
    {
        auto word = get_word(state);

        auto dest = state.reg[word.get_address().get_value()];

        if (state.acc.get_value() != 0)
        {
            state.pc = dest;
        }
    }

    // LDA
    void LDA(EmulatorState &state)
    {
        auto word = get_word(state);

        auto dest = state.reg[word.get_byte(1).get_value()];
        auto src = state.reg[word.get_address().get_value()];

        dest = src;
    }

    // STA
    void STA(EmulatorState &state)
    {
        auto word = get_word(state);

        auto src = state.reg[word.get_byte(1).get_value()];
        state.memory[word.get_address().get_value()] = src;
    }

    // EQ - set reg to 1 if equal
    void EQ(EmulatorState &state)
    {
        auto word = get_word(state);

        auto dest = state.reg[word.get_byte(1).get_value()];
        auto src1 = state.reg[word.get_byte(2).get_value()];
        auto src2 = state.reg[word.get_byte(3).get_value()];

        dest = src1 == src2;
    }

    // GT - set reg to 1 if greater than
    void GT(EmulatorState &state)
    {
        auto word = get_word(state);

        auto dest = state.reg[word.get_byte(1).get_value()];
        auto src1 = state.reg[word.get_byte(2).get_value()];
        auto src2 = state.reg[word.get_byte(3).get_value()];

        dest = src1 > src2;
    }

    void SHL(EmulatorState &state)
    {
        auto word = get_word(state);

        auto src = state.reg[word.get_byte(1).get_value()];

        src = src << 1;
    }

    void SHR(EmulatorState &state)
    {
        auto word = get_word(state);

        auto src = state.reg[word.get_byte(1).get_value()];

        src = src >> 1;
    }

    // ROL - rotate left one
    void ROL(EmulatorState &state)
    {
        auto word = get_word(state);

        auto src = state.reg[word.get_byte(1).get_value()];

        src = (src << 1) | (src >> 15);
    }

    // ROR - rotate right one
    void ROR(EmulatorState &state)
    {
        auto word = get_word(state);

        auto src = state.reg[word.get_byte(1).get_value()];

        src = (src >> 1) | (src << 15);
    }

    // HALT
    void HALT(EmulatorState &state)
    {
        state.HLT_flag = true;
    }

    void XXX(EmulatorState &state)
    {
        std::cout << "Invalid instruction" << std::endl;
        HALT(state);
    }

} // FIAT128