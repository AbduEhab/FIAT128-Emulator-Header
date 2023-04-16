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

inline const int kCORE_COUNT = static_cast<int>(std::thread::hardware_concurrency());

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <direct.h>
#include <windows.h>

// inline const std::string BINARY_DIRECTORY(std::string(_getcwd(NULL, 0)) + '/');
inline const std::string BINARY_DIRECTORY = std::filesystem::current_path().string();

#else

#include <unistd.h>

// inline const std::string BINARY_DIRECTORY_TEST(std::string(get_current_dir_name()) + "/");
inline const std::string BINARY_DIRECTORY = std::filesystem::current_path().string();

#endif

#define kEpsilon 0.000001

namespace FIAT128
{

#define u32(x) static_cast<uint32_t>(x)
#define to_char(x) static_cast<char>(x)
#define to_uchar(x) static_cast<unsigned char>(x)
#define to_bool(x) static_cast<bool>(x)
#define to_size_t(x) static_cast<size_t>(x)

    /**
     * @brief A random number generator
     *
     *@tparam T The type of the random number
     *
     *@param min The minimum value of the random number
     *@param max The maximum value of the random number
     */
    template <typename T>
    inline T random(T min = 0.0, T max = 1.0)
    {
        thread_local std::random_device rd;
        thread_local std::mt19937 gen(rd());
        thread_local std::uniform_real_distribution<> dis(min, max);
        return static_cast<T>(dis(gen));
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
        return static_cast<T>(((value - min) / (max - min)) * (new_max - new_min) + new_min);
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
            return;
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
#define debug_async_print(x, y) async_print_by_force(x, y);
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

    constexpr long long MEMORY_SIZE = const_pow(2, 8);

    enum InstructionType
    {

        XXX = u32(0),
        ADD = u32(1),
        AND = u32(2),
        OR = u32(3),
        XOR = u32(4),
        MOV = u32(5),
        BUN = u32(6),
        BIZ = u32(7),
        BNZ = u32(8),
        LDA = u32(9),
        STA = u32(10),
        LDR = u32(11),
        STR = u32(12),
        EQL = u32(13),
        GRT = u32(14),
        SHL = u32(15),
        SHR = u32(16),
        ROL = u32(17),
        ROR = u32(18),
        INT = u32(19),
        HLT = u32(20),

    };
    enum RegisterIndex
    {
        R0 = u32(0),
        R1 = u32(1),
        R2 = u32(2),
        R3 = u32(3),
        R4 = u32(4),
        R5 = u32(5),
        R6 = u32(6),
        R7 = u32(7),
        TIMER = u32(8),
    };

    static size_t id_counter = 0;

#ifdef DEBUG_CACHE
    constexpr static int cache_size = 8;
#else
    constexpr static int cache_size = 2048;
#endif

    template <size_t cores, size_t memory_modules, size_t word_size = 128>
    struct Emulator
    {

        Emulator(int memory_size = MEMORY_SIZE)
        {
            for (size_t i = 0; i < memory_modules; i++)
                memory[i] = Memory(to_size_t(memory_size));

            bus = BUS(memory, cpus);

            for (size_t i = 0; i <= cores; i++)
                cpus[i].set_bus(&bus);
        }

        auto run(bool step_mode = false)
        {
            for (auto &cpu : cpus)
            {
                cpu.execute_instruction(step_mode);
            }
        }

        /**
         * @brief Set the word on the given memory channel and index
         *
         * @param channel
         * @param index
         * @param value
         *
         * @note is thread safe
         */
        auto set_word_in_memory(size_t channel, size_t index, std::bitset<word_size> value)
        {
            bus.write(true, 0, channel, index, value);
        }

        /**
         * @brief Set the word on the given memory channel and index
         *
         * @param channel
         * @param index
         * @param value
         *
         * @note is thread safe
         */
        auto set_instruction_in_memory(size_t channel, size_t index, InstructionType type, RegisterIndex dest, RegisterIndex src_1 = RegisterIndex::R0, RegisterIndex src_2 = RegisterIndex::R0)
        {
            unsigned char instruction[4] = {to_uchar(type), to_uchar(dest), to_uchar(src_1), to_uchar(src_2)};
            bus.write(true, 0, channel, index, instruction);
        }

        auto set_instruction_in_cpu(char cpu_id, short index, InstructionType type, RegisterIndex dest, RegisterIndex src_1 = RegisterIndex::R0, RegisterIndex src_2 = RegisterIndex::R0)
        {
            if (to_size_t(cpu_id) < cores)
                cpus[to_size_t(cpu_id)].add_instruction(index, type, dest, src_1, src_2);
        }

        auto set_word_in_cpu(char cpu_id, short index, std::bitset<word_size> value)
        {
            if (to_size_t(cpu_id) < cores)
                cpus[to_size_t(cpu_id)].add_word(index, value);
        }

        // auto set = [](auto &array, auto &&value, auto &&...indices)
        // {
        //     if constexpr (sizeof...(indices) == 0)
        //     {
        //         array = std::forward<decltype(value)>(value);
        //     }
        //     else
        //     {
        //         set(array[std::forward<decltype(indices)>(indices)...],
        //             std::forward<decltype(value)>(value));
        //     }
        // };

    private:
        struct Memory
        {
            Memory() : memory(0, std::bitset<word_size>(0)) {}

            Memory(size_t size) : memory(size, std::bitset<word_size>(0)) {}

            ~Memory() {}

            // copy constructor
            Memory(const Memory &other)
            {
                for (size_t i = 0; i < other.memory.size(); i++)
                {
                    memory[i] = other.memory[i];
                }
            }

            // move constructor
            Memory(Memory &&other) : memory(std::move(other.memory)) {}

            // copy assignment
            Memory &operator=(const Memory &other)
            {
                for (size_t i = 0; i < other.memory.size(); i++)
                {
                    memory[i] = other.memory[i];
                }
                return *this;
            }

            // move assignment
            Memory &operator=(Memory &&other)
            {
                memory = std::move(other.memory);
                return *this;
            }

            auto read(size_t index) -> std::bitset<word_size>
            {
                std::lock_guard<std::mutex> lock(memory_mutex);
                return memory[index];
            }

            void write(size_t index, std::bitset<word_size> value)
            {
                std::lock_guard<std::mutex> lock(memory_mutex);
                memory[index] = value;
            }

            void write(size_t index, unsigned char (&value)[4])
            {
                std::lock_guard<std::mutex> lock(memory_mutex);
                memory[index] = (std::bitset<word_size>(u32(value[0]) << 24 | u32(value[1]) << 16 | u32(value[2]) << 8 | u32(value[3]))) <<= 96;
            }

            auto set_word(short index, std::bitset<word_size> word) -> bool
            {
                assert(index < cache_size);

                memory[index] = word;

                return true;
            }

            std::vector<std::bitset<word_size>> memory;
            std::mutex memory_mutex;
        };

        struct CPU;

        struct BUS
        {

            BUS() = default;

            BUS(Memory (&memory_array)[memory_modules], CPU (&cpu_array)[cores + 1])
            {
                for (size_t i = 0; i < memory_modules; i++)
                {
                    memory[i] = &memory_array[i];
                }

                for (size_t i = 0; i < in_connections; i++)
                {
                    in_state[i] = false;
                }
                for (size_t i = 0; i < memory_modules; i++)
                {
                    out_state[i] = false;
                }

                for (size_t i = 0; i < cores; i++)
                {
                    cpus[i] = &cpu_array[i];
                }
            }

            // copy constructor
            BUS(const BUS &other)
            {
                for (size_t i = 0; i < memory_modules; i++)
                {
                    memory[i] = other.memory[i];
                }

                for (size_t i = 0; i < in_connections; i++)
                {
                    in_state[i] = other.in_state[i];
                }
                for (size_t i = 0; i < memory_modules; i++)
                {
                    out_state[i] = other.out_state[i];
                }

                for (size_t i = 0; i < cores; i++)
                {
                    cpus[i] = other.cpus[i];
                }
            }

            // move constructor
            BUS(BUS &&other) : memory(std::move(other.memory)), in_state(std::move(other.in_state)), out_state(std::move(other.out_state)), cpus(std::move(other.cpus)) {}

            // copy assignment
            BUS &operator=(const BUS &other)
            {
                for (size_t i = 0; i < memory_modules; i++)
                {
                    memory[i] = other.memory[i];
                }

                for (size_t i = 0; i < in_connections; i++)
                {
                    in_state[i] = other.in_state[i];
                }
                for (size_t i = 0; i < memory_modules; i++)
                {
                    out_state[i] = other.out_state[i];
                }

                for (size_t i = 0; i < cores; i++)
                {
                    cpus[i] = other.cpus[i];
                }
                return *this;
            }

            // move assignment
            BUS &operator=(BUS &&other) = default;

            auto read(bool memory_operation, size_t id, size_t channel, size_t index) -> std::bitset<word_size>
            {
                if (memory_operation) [[likely]]
                {
                    assert(channel <= channels);

                    return memory[channel]->read(index);
                }
                else if (id == 0)
                {
                    assert((channel <= cores) && (index < cache_size));

                    return cpus[channel]->cache[index];
                }

                return std::bitset<word_size>(0);
            }

            auto write(bool memory_operation, size_t id, size_t channel, size_t index, std::bitset<word_size> value)
            {
                if (memory_operation) [[likely]]
                {
                    assert(channel < channels);

                    memory[channel]->write(index, value);
                }

                else if (id == 0)
                {
                    std::lock_guard<std::mutex> lock(cpu_mutex);
                    cpus[channel]->cache[index] = value;
                }
            }

            auto write(bool memory_operation, size_t id, size_t channel, size_t index, unsigned char (&value)[4])
            {
                if (memory_operation) [[likely]]
                {
                    assert(channel < channels);

                    memory[channel]->write(index, value);
                }
                else if (id == 0)
                {
                    std::lock_guard<std::mutex> lock(cpu_mutex);
                    cpus[channel]->cache[index] = (std::bitset<word_size>(u32(value[0]) << 24 | u32(value[1]) << 16 | u32(value[2]) << 8 | u32(value[3]))) <<= 96;
                }
            }

            size_t in_connections = cores;
            size_t channels = memory_modules;

            bool in_state[cores];
            bool out_state[memory_modules];

            Memory *memory[memory_modules];
            CPU *cpus[cores + 1];

            std::mutex cpu_mutex;
        };

        struct CPU
        {

            CPU(BUS *extern_bus = nullptr) : bus(extern_bus)
            {
                id = id_counter++;
                bus = extern_bus;
                decrement(stack_pointer);
                decrement(timer);

                if (id == 0)
                    flag.reset(1);
                else
                    flag.set(4);
            }

            auto set_bus(BUS *extern_bus) -> void
            {
                this->bus = extern_bus;
            }

            // bitset<word_size> increment
            template <size_t bitset_size>
            auto increment(std::bitset<bitset_size> &value) -> void
            {
                if (is_bitset_ones(value)) [[unlikely]]
                {
                    value = ~value;
                    flag.set(1);
                }

                for (size_t i = 0; i < bitset_size; i++)
                {
                    if (value[i] == 0)
                    {
                        value.set(i);
                    }
                    else
                    {
                        value.reset(i);
                        break;
                    }
                }
                for (size_t i = 0; i < bitset_size; i++)
                {
                    if (value[i] == 1)
                    {
                        value.reset(i);
                    }
                    else
                    {
                        value.set(i);
                        break;
                    }
                }
            }

            // bitset<word_size> decrement
            template <size_t bitset_size>
            auto decrement(std::bitset<bitset_size> &value) -> void
            {
                if (is_bitset_zero(value)) [[unlikely]]
                {
                    value = ~value;
                    flag.set(1);
                    return;
                }
                for (size_t i = 0; i < bitset_size; i++)
                {
                    if (value[i] == 0)
                    {
                        value.set(i);
                    }
                    else
                    {
                        value.reset(i);
                        break;
                    }
                }
            }

            // execute instruction
            auto execute_instruction(bool step_mode = 0) -> void
            {
                if (!this->bus)
                {
                    std::cerr << "Bus not connected" << std::endl;
                    return;
                }

                if (id == 0 && !initialized) [[unlikely]]
                {
                    if (flag[1] == 1) [[unlikely]]
                    {
                        flag.reset(1);
                        initialized = true;
                        goto emerg_break;
                    }

                    total_cpu_cycles++;

                    cache[stack_pointer.to_ulong()] = bus->read(true, 0, id, stack_pointer.to_ulong());

                    decrement(stack_pointer);
                    decrement(timer);
                    return;
                }
                else
                {
                emerg_break:

                    if ((flag & std::bitset<8>(0).set(4)).to_ulong() > 0)
                    {
                        debug_print(std::string("CPU ").append(std::to_string(id)), " halted");
                        return;
                    }

                    if (interrupt_enabled && new_instruction) [[unlikely]]
                    {
                        interrupt_enabled = false;
                        flag.reset(0);
                        stack_pointer = interrupt_seg_index;
                    }

                    if (new_instruction)
                        new_instruction = false;

                    if (step_mode)
                    {
                        switch (instruction_cycle)
                        {
                        case 0:

                            if (timer == 0) [[unlikely]]
                            {
                                interrupt_enabled = true;
                                flag.set(0);
                            }

                            instruction_cycle = 1;
                            total_cpu_cycles++;

                            // fetch instruction
                            current_word = cache[stack_pointer.to_ulong()];
                            acc = to_xbits<32>(current_word);
                            current_instruction = decode_instruction(acc);

                            decrement(stack_pointer);
                            decrement(timer);
                            break;

                        case 1:

                            if (timer == 0) [[unlikely]]
                            {
                                interrupt_enabled = true;
                                flag.set(0);
                            }

                            instruction_cycle = 2;
                            total_cpu_cycles++;

                            // execute instruction
                            (this->*current_instruction.opcode)();

                            new_instruction = true;

                            decrement(timer);
                            break;

                        case -1:
                            instruction_cycle = 0;
                            new_instruction = true;
                            break;
                        }
                    }
                    else
                    {
                        if (timer == 0) [[unlikely]]
                        {
                            interrupt_enabled = true;
                            flag.set(0);
                        }

                        instruction_cycle = 1;
                        total_cpu_cycles++;

                        // fetch instruction
                        current_word = cache[stack_pointer.to_ulong()];
                        acc = to_xbits<32>(current_word);
                        current_instruction = decode_instruction(acc);

                        total_cpu_cycles++;
                        decrement(stack_pointer);
                        decrement(timer);

                        for (size_t i = 1; i < to_size_t(current_instruction.cycles); i++)
                        {
                            execute_instruction(true);
                        }
                    }
                }
            }

            /**
             * @brief gets a certain byte from a bitset
             */
            struct Instruction
            {
                std::string_view name;
                void (FIAT128::Emulator<cores, memory_modules, word_size>::CPU::*opcode)() = nullptr;
                unsigned char cycles = 0;
                unsigned char dest = 0, src_1 = 0, src_2 = 0;

                /**
                 * @brief decodes an opcode into an instruction
                 * @param opcode the opcode to decode
                 * @return the decoded instruction
                 * @note if the opcode is invalid, the instruction XXX is returned
                 */
                static auto decode_from_opcode(std::bitset<8> &opcode) -> Instruction
                {
                    auto opcode_as_char = to_uchar(opcode.to_ulong());

                    if ((opcode_as_char > instruction_count) /* || opcode_as_char < 0 */)
                        return instruction_table[instruction_count]; // HLT instruction

                    return instruction_table[opcode_as_char];
                }
            };

            /**
             * @brief decodes the instruction from the opcode
             *
             * @param word The word containing the opcode
             * @return Instruction
             *
             * @note this is a helper function
             */
            auto decode_instruction(std::bitset<32> &word) -> Instruction
            {
                std::bitset<8> opcode = get_byte(word, 3);
                auto instruction = Instruction::decode_from_opcode(opcode);

                instruction.dest = to_uchar(get_byte(word, 2).to_ulong());
                instruction.src_1 = to_uchar(get_byte(word, 1).to_ulong());
                instruction.src_2 = to_uchar(get_byte(word, 0).to_ulong());

                return instruction;
            }

            template <size_t target_size>
            auto to_xbits(std::bitset<word_size> &word) -> std::bitset<target_size>
            {
                std::bitset<target_size> result;

                for (size_t i = word_size - 1, j = target_size - 1; i > word_size - target_size; i--)
                {
                    result[j--] = word[i];
                }

                return result;
            }

            /**
             * @brief gets a certain byte from a bitset
             *
             * @param word the bitset to get the byte from
             * @param byte the byte to get
             * @return std::bitset<8> the byte
             *
             * @note lowest byte is 0
             */
            template <size_t input_size>
            auto get_byte(std::bitset<input_size> &word, size_t byte) -> std::bitset<8>
            {
                std::bitset<8> result;

                for (size_t i = 0; i < 8; i++)
                {
                    result[i] = word[byte * 8 + i];
                }

                return result;
            }

            // add instruction into memory
            auto add_instruction(short index, InstructionType instruction, RegisterIndex operand_1, RegisterIndex operand_2 = R0, RegisterIndex operand_3 = R0) -> bool
            {
                if (!bus)
                {
                    std::cerr << "Bus not connected" << std::endl;
                    return false;
                }

                cache[index] = std::bitset<word_size>(u32(instruction) << 24 | u32(operand_1) << 16 | u32(operand_2) << 8 | u32(operand_3));

                return true;
            }

            auto add_word(short index, std::bitset<word_size> word) -> bool
            {
                assert(index < cache_size);

                if (!bus)
                {
                    std::cerr << "Bus not connected" << std::endl;
                    return false;
                }

                cache[index] = word;

                return true;
            }

            // print the current state of the cpu + 8 instructions
            auto print_state()
            {
                std::cout << "ID: " << id << std::endl;
                std::cout << "SP: " << stack_pointer << std::endl;
                std::cout << "II: " << interrupt_seg_index << std::endl;
                std::cout << "TI: " << timer << std::endl;
                std::cout << "FL: " << flag << std::endl;

                for (int i = 0; i < 8; i++)
                {
                    std::cout << "R" << i << ": " << reg[i] << std::endl;
                }

                // std::cout << "Next 7 instructions: " << std::endl;
                // for (int i = 1; i < 8; i++)
                // {
                //     std::cout << "Memory Address + " << i << " " << get_word(i) << std::endl;
                // }
            }

            // cpu cache
            std::bitset<word_size> cache[cache_size];

            // 128-bit general purpose registers [R0-R7, 6 & 7 are vector registers, 8 is a non-programable temp register]
            std::bitset<word_size> reg[9];

            // the flag register [0 interrupt flag, 1 overflow flag, 2 zero flag, 3 sign flag, 4 hlt flag]
            std::bitset<8> flag;

            // Timer register, counts till the end of time!
            std::bitset<word_size> timer;

            // Stack pointer [to index the cache]
            std::bitset<u32(std::log2(cache_size))> stack_pointer;

            // Segment index (interrupt)
            std::bitset<u32(std::log2(cache_size))> interrupt_seg_index;

            // return addresses stack
            std::bitset<u32(std::log2(cache_size))> return_address[16];

            // Accumulator - temp register, not visible to the user
            std::bitset<32> acc;

            // Interrupt enable flag
            bool interrupt_enabled = false;
            bool initialized = false;

            size_t id = 0;
            BUS *bus = nullptr;

            /* instruction function definitions */

            template <size_t input_size>
            auto add_bitset(std::bitset<input_size> &a, std::bitset<input_size> &b) -> std::pair<std::bitset<input_size>, bool>
            {
                std::bitset<input_size> result;
                bool overflow = false;

                for (size_t i = 0; i < input_size; i++)
                {
                    result[i] = (to_bool(a[i])) ^ (to_bool(b[i])) ^ overflow;
                    overflow = (a[i] & b[i]) | (a[i] & overflow) | (b[i] & overflow);
                }

                return {result, overflow};
            }

            template <size_t input_size>
            auto is_bitset_positive(std::bitset<input_size> &a) -> bool
            {
                return a[0] == 0;
            }

            template <size_t input_size>
            auto is_bitset_zero(std::bitset<input_size> &a) -> bool
            {
                for (size_t i = 0; i < input_size; i++)
                {
                    if (a[i] == 1)
                    {
                        return false;
                    }
                }

                return true;
            }

            template <size_t input_size>
            auto is_bitset_ones(std::bitset<input_size> &a) -> bool
            {
                for (size_t i = 0; i < input_size; i++)
                {
                    if (a[i] != 1)
                    {
                        return false;
                    }
                }

                return true;
            }

            /**
             * @brief adds two registers and stores the result in the third register
             *
             * @note the overflow flag is set if the result is too large to fit in the register, the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void ADD()
            {
                auto result = add_bitset(reg[current_instruction.src_1], reg[current_instruction.src_2]);

                if (result.second)
                    flag.set(1);

                if (is_bitset_zero(result.first))
                    flag.set(2);

                if (!is_bitset_positive(result.first))
                    flag.set(3);

                reg[current_instruction.dest] = result.first;

                debug_print(std::string("CPU ").append(std::to_string(id)), " ADD executed");
            }

            /**
             * @brief ands two registers and stores the result in the third register
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void AND()
            {
                reg[current_instruction.dest] = reg[current_instruction.src_1] & reg[current_instruction.src_2];

                if (is_bitset_zero(reg[current_instruction.dest]))
                    flag.set(2);
                if (!is_bitset_positive(reg[current_instruction.dest]))
                    flag.set(3);

                debug_print(std::string("CPU ").append(std::to_string(id)), " AND executed");
            }

            /**
             * @brief ors two registers and stores the result in the third register
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void OR()
            {
                reg[current_instruction.dest] = reg[current_instruction.src_1] | reg[current_instruction.src_2];

                if (is_bitset_zero(reg[current_instruction.dest]))
                    flag.set(2);

                if (!is_bitset_positive(reg[current_instruction.dest]))
                    flag.set(3);

                debug_print(std::string("CPU ").append(std::to_string(id)), " OR executed");
            }

            /**
             * @brief xors two registers and stores the result in the third register
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void XOR()
            {
                reg[current_instruction.dest] = reg[current_instruction.src_1] ^ reg[current_instruction.src_2];

                if (is_bitset_zero(reg[current_instruction.dest]))
                    flag.set(2);

                if (!is_bitset_positive(reg[current_instruction.dest]))
                    flag.set(3);

                debug_print(std::string("CPU ").append(std::to_string(id)), " XOR executed");
            }

            /**
             * @brief moves the value of one register to another
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void MOV()
            {
                reg[current_instruction.dest] = reg[current_instruction.src_1];

                if (is_bitset_zero(reg[current_instruction.dest]))
                    flag.set(2);

                if (!is_bitset_positive(reg[current_instruction.dest]))
                    flag.set(3);

                debug_print(std::string("CPU ").append(std::to_string(id)), " MOV executed");
            }

            /**
             * @brief branches to the address specified in the register uncconditionally
             */
            void BUN()
            {
                stack_pointer = reg[current_instruction.dest].to_ulong();

                debug_print(std::string("CPU ").append(std::to_string(id)), " BUN executed");
            }

            /**
             * @brief branches to the address specified in the register if the zero flag is set
             */
            void BIZ()
            {
                if ((flag & std::bitset<8>(0).set(2)).to_ulong() > 0)
                    stack_pointer = reg[current_instruction.dest].to_ulong();

                debug_print(std::string("CPU ").append(std::to_string(id)), " BIZ executed");
            }

            /**
             * @brief branches to the address specified in the register if the zero flag is not set
             */
            void BIN()
            {
                if ((flag & std::bitset<8>(0).set(3)).to_ulong() > 0)
                    stack_pointer = reg[current_instruction.dest].to_ulong();

                debug_print(std::string("CPU ").append(std::to_string(id)), " BIN executed");
            }

            /**
             * @brief loads the value in the memory address specified in a register into a register
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void LDA()
            {
                reg[current_instruction.dest] = bus->read(true, id, current_instruction.dest, reg[current_instruction.src_1].to_ulong());

                if (is_bitset_zero(reg[current_instruction.dest]))
                    flag.set(2);
                if (!is_bitset_positive(reg[current_instruction.dest]))
                    flag.set(3);

                debug_print(std::string("CPU ").append(std::to_string(id)), " LDA executed");
            }

            /**
             * @brief stores the value in the register in the memory address specified in the register
             */
            void STA()
            {
                bus->write(true, id, current_instruction.src_2, current_instruction.dest, reg[current_instruction.src_1]);

                debug_print(std::string("CPU ").append(std::to_string(id)), " STA executed");
            }

            /**
             * @brief loads a value in cache memory into a register
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void LDR()
            {
                reg[current_instruction.dest] = cache[reg[current_instruction.src_1].to_ulong()];

                debug_print(std::string("CPU ").append(std::to_string(id)), " LDR executed");
            }

            /**
             * @brief stores the value in the register in the cache memory address specified in the register
             */
            void STR()
            {
                cache[reg[current_instruction.dest].to_ulong()] = reg[current_instruction.src_1].to_ulong();

                debug_print(std::string("CPU ").append(std::to_string(id)), " STR executed");
            }

            /**
             * @brief compares two registers and sets the zero flag if they are equal
             */
            void EQL()
            {
                if (reg[current_instruction.src_1] == reg[current_instruction.src_2])
                    flag.set(2);

                debug_print(std::string("CPU ").append(std::to_string(id)), " EQL executed");
            }

            /**
             * @brief compares two registers and sets the sign flag if the first register is greater than the second register
             */
            void GRT()
            {
                if (reg[current_instruction.src_1].to_ulong() < reg[current_instruction.src_2].to_ulong())
                    flag.set(3);

                debug_print(std::string("CPU ").append(std::to_string(id)), " GRT executed");
            }

            /**
             * @brief shifts a register left by one
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void SHL()
            {
                for (size_t i = 0; i < 8; i++)
                    reg[current_instruction.src_1][i] = reg[current_instruction.src_1][i + 1];

                if (is_bitset_zero(reg[current_instruction.src_1]))
                    flag.set(2);
                if (!is_bitset_positive(reg[current_instruction.src_1]))
                    flag.set(3);

                debug_print(std::string("CPU ").append(std::to_string(id)), " SHL executed");
            }

            /**
             * @brief shifts a register right by one
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void SHR()
            {
                for (size_t i = 7; i > 0; i--)
                    reg[current_instruction.src_1][i] = reg[current_instruction.src_1][i - 1];

                if (is_bitset_zero(reg[current_instruction.src_1]))
                    flag.set(2);
                if (!is_bitset_positive(reg[current_instruction.src_1]))
                    flag.set(3);

                debug_print(std::string("CPU ").append(std::to_string(id)), " SHR executed");
            }

            /**
             * @brief rotates a register left by one
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void ROL() // Note(AbduEhab): Needs to be validated
            {
                reg[current_instruction.src_1] <<= 1;

                if (is_bitset_zero(reg[current_instruction.src_1]))
                    flag.set(2);

                if (!is_bitset_positive(reg[current_instruction.src_1]))
                    flag.set(3);

                debug_print(std::string("CPU ").append(std::to_string(id)), " ROL executed");
            }

            /**
             * @brief rotates a register right by one
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void ROR() // Note(AbduEhab): Needs to be validated
            {
                reg[current_instruction.src_1] >>= 1;

                if (is_bitset_zero(reg[current_instruction.src_1]))
                    flag.set(2);

                if (!is_bitset_positive(reg[current_instruction.src_1]))
                    flag.set(3);

                debug_print(std::string("CPU ").append(std::to_string(id)), " ROR executed");
            }

            /**
             * @brief Initializes the other CPUs
             */
            void INT()
            {

                for (size_t i = 1; i <= cores; i++)
                {
                    for (size_t j = 0; j < cache_size; j++)
                    {
                        bus->write(false, i, 0, j, bus->read(true, id, 0, cache_size * i + j));
                    }
                }

                debug_print(std::string("CPU ").append(std::to_string(id)), " INT executed");
            }

            /**
             * @brief Halts the cpu
             */
            void HLT()
            {
                flag |= 0b0001'0000;

                debug_print(std::string("CPU ").append(std::to_string(id)), " HLT executed");
            }

            /**
             * @brief halts the cpu and prints a message
             *
             * @note this instruction should not be possible to execute if code is compiled correctly
             */
            void XXX()
            {
                HLT();

                debug_print(std::string("CPU ").append(std::to_string(id)), " Invalid instruction called");
            }

            // instruction count
            static const unsigned char instruction_count = 21;

            // instrcution table
            static inline Instruction instruction_table[instruction_count] = {
                {"XXX", &CPU::XXX, 2},
                {"ADD", &CPU::ADD, 2},
                {"AND", &CPU::AND, 2},
                {"OR", &CPU::OR, 2},
                {"XOR", &CPU::XOR, 2},
                {"MOV", &CPU::MOV, 2},
                {"BUN", &CPU::BUN, 2},
                {"BIZ", &CPU::BIZ, 2},
                {"BIN", &CPU::BIN, 2},
                {"LDA", &CPU::LDA, 2},
                {"STA", &CPU::STA, 2},
                {"LDA", &CPU::LDR, 2},
                {"STA", &CPU::STR, 2},
                {"EQL", &CPU::EQL, 2},
                {"GRT", &CPU::GRT, 2},
                {"SHL", &CPU::SHL, 2},
                {"SHR", &CPU::SHR, 2},
                {"ROL", &CPU::ROL, 2},
                {"ROR", &CPU::ROR, 2},
                {"INT", &CPU::INT, 2},
                {"HLT", &CPU::HLT, 2},
            };

            // current instruction
            Instruction current_instruction;
            std::bitset<word_size> current_word;

            // cpu cycles
            long long total_cpu_cycles = 0;
            char instruction_cycle = 0;

            bool new_instruction = true;
        };

        CPU cpus[cores + 1];
        Memory memory[memory_modules];

        BUS bus;
    };
}; // FIAT128