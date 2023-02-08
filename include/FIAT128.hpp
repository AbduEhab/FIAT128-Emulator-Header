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

    /**
     * @brief lamda to check if a number is a power of 2
     *
     * @param x number to check
     */
    auto is_power_of_2 = [](long long x) -> bool
    {
        return (x != 0) && ((x & (x - 1)) == 0);
    };

    template <size_t T>
    struct Register
    {
        bool data[T] = {0};
        size_t size = T;
        bool overflow = false;

        /**
         * @brief set the data of the register
         *
         * @param data the data to set
         */
        void set_data(size_t (&data)[T]) noexcept
        {
            for (size_t i = 0; i < T; i++)
            {
                this->data[i] = data[i];
            }
        }

        /**
         * @brief sets a bit in the register
         *
         * @param index the index of the bit to set
         * @param value the value to set the bit to
         */
        void set_bit(size_t index, bool value) noexcept
        {
            data[T - index - 1] = value;

            overflow = false;
        }

        /**
         * @brief gets a bit in the register
         *
         * @param index the index of the bit to get
         *
         * @return bool the value of the bit
         */
        auto get_bit(size_t index) const noexcept -> bool
        {
            return data[T - index - 1];
        }

        /**
         * @brief resets the overflow flag
         *
         */
        void reset_overflow() noexcept
        {
            overflow = false;
        }

        /**
         * @brief gets the overflow flag
         *
         * @return true if overflow
         * @return false if no overflow
         */
        auto get_overflow() const noexcept -> bool
        {
            if (overflow) [[unlikely]]
            {
                overflow = false;
                return true;
            }
            return false;
        }

        /**
         * @brief clears the register
         *
         */
        void clear() noexcept
        {
            for (size_t i = 0; i < T; i++)
            {
                data[i] = 0;
            }

            overflow = false;
        }

        /**
         * @brief checks if the register is zero
         *
         * @return true if zero
         * @return false if not zero
         */
        auto is_zero() const noexcept -> bool
        {
            for (size_t i = 0; i < T; i++)
            {
                if (data[i] == 1)
                {
                    return false;
                }
            }
            return true;
        }

        /**
         * @brief increments the register and sets the overflow flag
         *
         * @note if the register is already at the max value, the overflow flag is set and the register is cleared
         */
        void inc() noexcept
        {
            // inc and set overflow
            for (size_t i = T - 1; i > 0; i--)
            {
                if (data[i] == 0)
                {
                    data[i] = 1;
                    break;
                }
                else
                {
                    data[i] = 0;
                }
            }

            if (is_zero())
                overflow = true;
        }

        auto operator++() noexcept -> Register<T> &
        {
            inc();
            return *this;
        }

        auto operator--() noexcept -> Register<T> &
        {
            // dec
            for (size_t i = T - 1; i > 0; i--)
            {
                if (data[i] == 1)
                {
                    data[i] = 0;
                    break;
                }
                else
                {
                    data[i] = 1;
                }
            }
            // set overflow
            if (is_zero())
                overflow = true;

            return *this;
        }

        auto operator+=(const long long value) noexcept
        {
            for (size_t i = 0; i < value; i++)
            {
                inc();
            }
        }

        auto operator~() const noexcept -> Register<T>
        {
            Register<T> result;

            for (size_t i = 0; i < T; i++)
            {
                result.data[i] = !data[i];
            }

            return result;
        }

        auto operator=(const Register<T> &other) noexcept
        {
            for (size_t i = 0; i < T; i++)
            {
                data[i] = other.data[i];
            }
        }

        auto operator=(const bool other) noexcept
        {
            for (size_t i = 0; i < T; i++)
            {
                data[i] = other;
            }
        }

        auto operator+(const Register<T> &other) const noexcept -> Register<T>
        {
            Register<T> result;
            bool carry = false;
            for (size_t i = T - 1; i > 0; i--)
            {
                if (data[i] == 0 && other.data[i] == 0)
                {
                    if (carry)
                    {
                        result.data[i] = 1;
                        carry = false;
                    }
                    else
                    {
                        result.data[i] = 0;
                    }
                }
                else if (data[i] == 1 && other.data[i] == 1)
                {
                    if (carry)
                    {
                        result.data[i] = 1;
                    }
                    else
                    {
                        result.data[i] = 0;
                        carry = true;
                    }
                }
                else
                {
                    if (carry)
                    {
                        result.data[i] = 0;
                    }
                    else
                    {
                        result.data[i] = 1;
                    }
                }
            }

            if (carry)
                result.overflow = true;

            return result;
        }

        auto operator-(const Register<T> &other) const noexcept -> Register<T>
        {

            Register<T> reverse;

            for (size_t i = 0; i < T; i++)
            {
                reverse.data[i] = !other.data[i];
            }

            Register<T> one;
            one.set_bit(T - 1, 1);

            Register<T> two_complement = reverse + one;

            return *this + two_complement;
        }

        auto operator&(const Register<T> &other) const noexcept -> Register<T>
        {
            Register<T> result;
            for (size_t i = 0; i < T; i++)
            {
                result.data[i] = data[i] & other.data[i];
            }
            return result;
        }

        auto operator|(const Register<T> &other) const noexcept -> Register<T>
        {
            Register<T> result;
            for (size_t i = 0; i < T; i++)
            {
                result.data[i] = data[i] | other.data[i];
            }
            return result;
        }

        auto operator|(const char other) const noexcept -> Register<T>
        {
            std::bitset<T> bitset(other);

            Register<T> result;

            for (size_t i = 0; i < T; i++)
            {
                if (bitset[i])
                {
                    result.data[i] = 1;
                }
            }

            return result;
        }

        auto operator^(const Register<T> &other) const noexcept -> Register<T>
        {
            Register<T> result;
            for (size_t i = 0; i < T; i++)
            {
                result.data[i] = data[i] ^ other.data[i];
            }
            return result;
        }

        auto operator<<(const size_t value) const noexcept -> Register<T>
        {
            Register<T> result;
            for (size_t i = 0; i < T; i++)
            {
                if (i + value < T)
                {
                    result.data[i + value] = data[i];
                }
            }
            return result;
        }

        auto operator>>(const size_t value) const noexcept -> Register<T>
        {
            Register<T> result;
            for (size_t i = 0; i < T; i++)
            {
                if (i - value >= 0)
                {
                    result.data[i - value] = data[i];
                }
            }
            return result;
        }

        auto operator==(const Register<T> &other) const noexcept -> bool
        {
            for (size_t i = 0; i < T; i++)
            {
                if (data[i] != other.data[i])
                {
                    return false;
                }
            }
            return true;
        }

        auto operator>(const Register<T> &other) const noexcept -> bool
        {
            for (size_t i = 0; i < T; i++)
            {
                if (data[i] == 1 && other.data[i] == 0)
                {
                    return true;
                }
                else if (data[i] == 0 && other.data[i] == 1)
                {
                    return false;
                }
            }
            return false;
        }

        /**
         * @brief Get the bit object
         *
         * @param index
         * @return bool
         */
        auto get_byte(size_t index) const noexcept -> Register<8>
        {
            index = T - index - 1;

            Register<8> byte;
            for (size_t i = index; i > 0; i--)
            {
                byte.data[i] = data[index * 8 + i];
            }
            return byte;
        }

        /**
         * @brief Get the address from the register
         *
         * @return Register<T>
         */
        auto get_address() const noexcept -> Register<T>
        {
            Register<T> byte;
            for (size_t i = 9; i < T - 9; i++)
            {
                byte.data[i] = data[i];
            }

            return byte;
        }

        /**
         * @brief Set a byte of the register
         *
         * @param index
         * @param byte
         *
         * @note least significant bit is at index 0
         */
        auto set_byte(size_t index, const Register<8> byte) const noexcept
        {
            index = T - index - 1;

            for (size_t i = 0; i < 8; i++)
            {
                data[index * 8 + i] = byte.data[i];
            }
        }

        /**
         * @brief Set a bit of the register
         *
         * @param index
         * @param value
         *
         * @note least significant bit is at index 0
         */
        auto get_byte_as_char(size_t index) const noexcept -> unsigned char
        {
            Register<8> byte = get_byte(index);
            std::bitset<8> bitset;

            for (size_t i = 0; i < 8; i++)
            {
                bitset[i] = byte.data[i];
            }
            return (unsigned char)bitset.to_ulong();
        }

        /**
         * @brief Get tje numeric value of the register
         *
         * @return unsigned long long
         */
        auto get_value() const noexcept -> unsigned long long
        {
            std::bitset<T> bitset;
            for (size_t i = 0; i < T; i++)
            {
                bitset[i] = data[i];
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
        void (*opcode)() = nullptr;
        char cycles = 0;

        static auto decode_from_opcode(Register<8> opcode) -> Instruction;
    };

    /**
     * @brief calculates the power of a number at compile time
     *
     * @param base base of the power
     * @param exponent exponent of the power
     *
     * @return long long
     */
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

    template <char size>
    struct EmulatorState
    {

        EmulatorState(bool static_memory = true)
        {
            assert(is_power_of_2(size));

            // initialize memory
            if constexpr (static_memory) [[likely]]
            {
                for (long long i = 0; i < MEMORY_SIZE; ++i)
                {
                    memory[i] = 0;
                }
            }
            else
            {
                memory = new std::vector<Register<size>>(MEMORY_SIZE);
            }
        }

        // execute instruction
        auto execute_instruction()
        {
            if (flag.get_bit(4))
            {
                debug_print("CPU halted\n", "");
                return;
            }

            if (interrupt_enabled && new_instruction) [[unlikely]]
            {
                interrupt_enabled = false;
                flag = flag & 0b1111'1110;
                pc = sii;
            }

            if (step_mode)
            {
                switch (instruction_cycle)
                {
                case 0:

                    if (timer == 0) [[unlikely]]
                    {
                        interrupt_enabled = true;
                        flag = flag | 0b0000'0001;
                    }

                    instruction_cycle = 1;
                    total_cpu_cycles++;

                    // fetch instruction
                    acc = get_word();
                    current_instruction = decode_instruction(acc);

                    pc++;
                    timer--;
                    break;

                case 1:

                    if (timer == 0) [[unlikely]]
                    {
                        interrupt_enabled = true;
                        flag = flag | 0b0000'0001;
                    }

                    instruction_cycle = 2;
                    total_cpu_cycles++;

                    // execute instruction
                    (*current_instruction.opcode)();

                    if (current_instruction.cycles != 2) [[likely]]
                    {
                        instruction_cycle = 0;
                        new_instruction = true;
                        break;
                    }

                    pc++;
                    timer--;
                    break;

                case 2:

                    if (timer == 0) [[unlikely]]
                    {
                        interrupt_enabled = true;
                        flag = flag | 0b0000'0001;
                    }

                    instruction_cycle = 0;
                    total_cpu_cycles++;

                    // execute instruction
                    (*current_instruction.opcode)();

                    pc++;
                    timer--;
                    break;
                }
            }
            else
            {
                instruction_cycle = 1;
                total_cpu_cycles++;
                (*current_instruction.opcode)();
            }
        }

        // 128-bit general purpose registers
        Register<size> reg[8];

        // the flag register [0 interuptn flag, 1 overflow flag, 2 zero flag, 3 sign flag]
        Register<8> flag;

        // Timer register, counts till the end of time!
        Register<size> timer;

        // Stack pointer
        Register<size> sp;

        // Segment index (stack)
        Register<size> sis;

        // Segment limit (stack)
        Register<size> sls;

        // Segment index (interrupt)
        Register<size> sii;

        // Segment limit (interrupt)
        Register<size> sli;

        // Segment index (code)
        Register<size> sic;

        // Segment limit (code)
        Register<size> slc;

        // Segment index (io)
        Register<size> siio;

        // Segment limit (io)
        Register<size> slio;

        // Accumulator - for debugging
        Register<size> acc;

        // Interrupt enable flag
        bool interrupt_enabled;

        // Program counter
        Register<size> pc;

#ifdef STATIC_MEMORY
        // Memory
        Register<size> memory[MEMORY_SIZE];
#else
        // Memory
        std::vector<Register<size>> memory;

#endif // STATIC_MEMORY

    private:
        /**
         * @brief gets the next word from memory
         *
         * @return Register<size>
         * @note the size of the register is the same as the size of the memory, this is a helper function
         */
        auto get_word() -> Register<128>
        {
            return memory[pc.get_value()];
        }

        /**
         * @brief decodes the instruction from the opcode
         *
         * @param word The word containing the opcode
         * @return Instruction
         *
         * @note this is a helper function
         */
        template <char T>
        auto decode_instruction(Register<T> &word) -> Instruction
        {
            Register<8> opcode = word.get_byte(0);
            return Instruction::decode_from_opcode(opcode);
        }

        /* instruction function definitions */

        /**
         * @brief adds two registers and stores the result in the third register
         *
         * @note the overflow flag is set if the result is too large to fit in the register, the zero flag is set if the result is zero, and the sign flag is set if the result is negative
         */
        void ADD()
        {
            reg[current_word.get_byte(2).get_value()] = reg[current_word.get_byte(1).get_value()] + reg[current_word.get_byte(0).get_value()];

            bool overflow = reg[current_word.get_byte(2).get_value()].get_overflow();
            bool is_zero = reg[current_word.get_byte(2).get_value()].is_zero();
            bool sign = reg[current_word.get_byte(2).get_value()].get_bit(0);

            flag = flag & 0b1111'0001;

            flag = flag | (overflow << 1);
            flag = flag | (is_zero << 2);
            flag = flag | (sign << 3);

            debug_print("ADD executed\n", "");
        }

        /**
         * @brief subtracts two registers and stores the result in the third register
         *
         * @note the overflow flag is set if the result is too large to fit in the register, the zero flag is set if the result is zero, and the sign flag is set if the result is negative
         */
        void SUB()
        {
            reg[current_word.get_byte(2).get_value()] = reg[current_word.get_byte(1).get_value()] - reg[current_word.get_byte(0).get_value()];

            bool overflow = reg[current_word.get_byte(2).get_value()].get_overflow();
            bool is_zero = reg[current_word.get_byte(2).get_value()].is_zero();
            bool sign = reg[current_word.get_byte(2).get_value()].get_bit(0);

            flag = flag & 0b1111'0001;

            flag = flag | (overflow << 1);
            flag = flag | (is_zero << 2);
            flag = flag | (sign << 3);

            debug_print("SUB executed\n", "");
        }

        /**
         * @brief ands two registers and stores the result in the third register
         *
         * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
         */
        void AND()
        {
            reg[current_word.get_byte(2).get_value()] = reg[current_word.get_byte(1).get_value()] & reg[current_word.get_byte(0).get_value()];

            bool is_zero = reg[current_word.get_byte(2).get_value()].is_zero();
            bool sign = reg[current_word.get_byte(2).get_value()].get_bit(0);

            flag = flag & 0b1111'0011;

            flag = flag | (is_zero << 2);
            flag = flag | (sign << 3);

            debug_print("AND executed\n", "");
        }

        /**
         * @brief ors two registers and stores the result in the third register
         *
         * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
         */
        void OR()
        {
            reg[current_word.get_byte(2).get_value()] = reg[current_word.get_byte(1).get_value()] | reg[current_word.get_byte(0).get_value()];

            bool is_zero = reg[current_word.get_byte(2).get_value()].is_zero();
            bool sign = reg[current_word.get_byte(2).get_value()].get_bit(0);

            flag = flag & 0b1111'0011;

            flag = flag | (is_zero << 2);
            flag = flag | (sign << 3);

            debug_print("OR executed\n", "");
        }

        /**
         * @brief xors two registers and stores the result in the third register
         *
         * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
         */
        void XOR()
        {
            reg[current_word.get_byte(2).get_value()] = reg[current_word.get_byte(1).get_value()] ^ reg[current_word.get_byte(0).get_value()];

            bool is_zero = reg[current_word.get_byte(2).get_value()].is_zero();
            bool sign = reg[current_word.get_byte(2).get_value()].get_bit(0);

            flag = flag & 0b1111'0011;

            flag = flag | (is_zero << 2);
            flag = flag | (sign << 3);

            debug_print("XOR executed\n", "");
        }

        /**
         * @brief moves the value of one register to another
         *
         * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
         */
        void MOV()
        {
            reg[current_word.get_byte(2).get_value()] = reg[current_word.get_byte(1).get_value()];

            bool is_zero = reg[current_word.get_byte(2).get_value()].is_zero();
            bool sign = reg[current_word.get_byte(2).get_value()].get_bit(0);

            flag = flag & 0b1111'0011;

            flag = flag | (is_zero << 2);
            flag = flag | (sign << 3);

            debug_print("MOV executed\n", "");
        }

        /**
         * @brief branches to the address specified in the register uncconditionally
         */
        void BUN()
        {
            pc = current_word.get_address();

            debug_print("BUN executed\n", "");
        }

        /**
         * @brief branches to the address specified in the register if the zero flag is set
         */
        void BIZ()
        {
            if (flag & 0b0000'0100 > 0)
            {
                pc = current_word.get_address();

                debug_print("BIZ executed\n", "");
            }

            debug_print("BIZ did not execute\n", "");
        }

        /**
         * @brief branches to the address specified in the register if the zero flag is not set
         */
        void BNZ()
        {
            if (flag & 0b0000'0100 == 0)
            {
                pc = current_word.get_address();

                debug_print("BNZ executed\n", "");
            }

            debug_print("BNZ did not execute\n", "");
        }

        /**
         * @brief branches to the address specified in the register if the sign flag is set
         *
         * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
         */
        void LDA()
        {
            reg[current_word.get_byte(2).get_value()] = memory[current_word.get_address().get_value()];

            bool is_zero = reg[current_word.get_byte(2).get_value()].is_zero();
            bool sign = reg[current_word.get_byte(2).get_value()].get_bit(0);

            flag = flag & 0b1111'0011;

            flag = flag | (is_zero << 2);
            flag = flag | (sign << 3);

            debug_print("LDA executed\n", "");
        }

        /**
         * @brief stores the value in the register in the memory address specified in the register
         */
        void STA()
        {
            memory[current_word.get_address().get_value()] = reg[current_word.get_byte(2).get_value()];

            debug_print("STA executed\n", "");
        }

        /**
         * @brief compares two registers and sets the zero flag if they are equal
         */
        void EQ()
        {
            reg[current_word.get_byte(2).get_value()] = reg[current_word.get_byte(1).get_value()] - reg[current_word.get_byte(0).get_value()];

            if (reg[current_word.get_byte(2).get_value()].is_zero())
            {
                flag = flag | 0b0000'0100;
            }
            else
            {
                flag = flag & 0b1111'1011;
            }

            debug_print("EQ executed\n", "");
        }

        /**
         * @brief compares two registers and sets the sign flag if the first register is greater than the second register
         */
        void GT() // Note(AbduEhab): needs to be revised
        {
            reg[current_word.get_byte(2).get_value()] = reg[current_word.get_byte(1).get_value()] - reg[current_word.get_byte(0).get_value()];

            if (reg[current_word.get_byte(2).get_value()].get_bit(0))
            {
                flag = flag | 0b0000'1000;
            }
            else
            {
                flag = flag & 0b1111'0111;
            }

            debug_print("GT executed\n", "");
        }

        /**
         * @brief shifts a register left by one
         *
         * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
         */
        void SHL()
        {
            reg[current_word.get_byte(2).get_value()] = (reg[current_word.get_byte(2).get_value()] << 1);

            bool is_zero = reg[current_word.get_byte(2).get_value()].is_zero();
            bool sign = reg[current_word.get_byte(2).get_value()].get_bit(0);

            flag = flag & 0b1111'0011;

            flag = flag | (is_zero << 2);
            flag = flag | (sign << 3);

            debug_print("SHL executed\n", "");
        }

        /**
         * @brief shifts a register right by one
         *
         * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
         */
        void SHR()
        {
            reg[current_word.get_byte(2).get_value()] = (reg[current_word.get_byte(2).get_value()] >> 1);

            bool is_zero = reg[current_word.get_byte(2).get_value()].is_zero();
            bool sign = reg[current_word.get_byte(2).get_value()].get_bit(0);

            flag = flag & 0b1111'0011;

            flag = flag | (is_zero << 2);
            flag = flag | (sign << 3);

            debug_print("SHR executed\n", "");
        }

        /**
         * @brief rotates a register left by one
         *
         * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
         */
        void ROL()
        {
            reg[current_word.get_byte(2).get_value()] = (reg[current_word.get_byte(2).get_value()] << 1) | (reg[current_word.get_byte(2).get_value()].get_bit(size));

            bool is_zero = reg[current_word.get_byte(2).get_value()].is_zero();
            bool sign = reg[current_word.get_byte(2).get_value()].get_bit(0);

            flag = flag & 0b1111'0011;

            flag = flag | (is_zero << 2);
            flag = flag | (sign << 3);

            debug_print("ROL executed\n", "");
        }

        /**
         * @brief rotates a register right by one
         *
         * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
         */
        void ROR()
        {
            reg[current_word.get_byte(2).get_value()] = (reg[current_word.get_byte(2).get_value()] >> 1) | (reg[current_word.get_byte(2).get_value()].get_bit(0) << (size - 1));

            bool is_zero = reg[current_word.get_byte(2).get_value()].is_zero();
            bool sign = reg[current_word.get_byte(2).get_value()].get_bit(0);

            flag = flag & 0b1111'0011;

            flag = flag | (is_zero << 2);
            flag = flag | (sign << 3);

            debug_print("ROR executed\n", "");
        }

        /**
         * @brief Halts the cpu
         */
        void HLT()
        {
            flag.set_bit(4, 1);
            debug_print("HLT executed\n", "");
        }

        /**
         * @brief halts the cpu and prints a message
         *
         * @note this instruction should not be possible to execute if code is compiled correctly
         */
        void XXX()
        {
            HLT();
            debug_print("Invalid instruction\n", "");
        }

        struct Instruction
        {
            std::string_view name;
            void (*opcode)() = nullptr;
            char cycles = 0;

            /**
             * @brief decodes an opcode into an instruction
             * @param opcode the opcode to decode
             * @return the decoded instruction
             * @note if the opcode is invalid, the instruction XXX is returned
             */
            static auto decode_from_opcode(Register<8> opcode) -> Instruction
            {
                unsigned char opcode_as_char = opcode.get_byte_as_char(0);

                if (opcode_as_char > instruction_count || opcode_as_char < 0)
                    return instruction_table[instruction_count];

                return instruction_table[opcode_as_char];
            }
        };

        // instruction count
        static const int instruction_count = 19;

        // instrcution table
        static const Instruction instruction_table[instruction_count] = {
            {"ADD", &ADD, 2},
            {"SUB", &SUB, 2},
            {"AND", &AND, 2},
            {"OR", &OR, 2},
            {"XOR", &XOR, 2},
            {"MOV", &MOV, 2},
            {"BUN", &BUN, 2},
            {"BIZ", &BIZ, 3},
            {"BIN", &BNZ, 3},
            {"LDA", &LDA, 3},
            {"STA", &STA, 3},
            {"EQ", &EQ, 2},
            {"GT", &GT, 2},
            {"SHL", &SHL, 2},
            {"SHR", &SHR, 2},
            {"ROL", &ROL, 2},
            {"ROR", &ROR, 2},
            {"HALT", &HLT, 2},
            {"XXX", &XXX, 2},
        };

        // current instruction
        Instruction current_instruction;
        Register<size> current_word;

        // cpu cycles
        long long total_cpu_cycles = 0;
        char instruction_cycle = 0;

        bool step_mode = false;
        bool new_instruction = true;
    };

} // FIAT128