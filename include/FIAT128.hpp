#pragma once

#include <algorithm>
#include <array>
#include <assert.h>
#include <bitset>
#include <chrono>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
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

namespace FIAT128
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

        XXX = (uint32_t)0,
        ADD = (uint32_t)1,
        AND = (uint32_t)2,
        OR = (uint32_t)3,
        XOR = (uint32_t)4,
        MOV = (uint32_t)5,
        BUN = (uint32_t)6,
        BIZ = (uint32_t)7,
        BNZ = (uint32_t)8,
        LDA = (uint32_t)9,
        STA = (uint32_t)10,
        LDR = (uint32_t)11,
        STR = (uint32_t)12,
        EQL = (uint32_t)13,
        GRT = (uint32_t)14,
        SHL = (uint32_t)15,
        SHR = (uint32_t)16,
        ROL = (uint32_t)17,
        ROR = (uint32_t)18,
        HLT = (uint32_t)19,

    };
    enum RegisterIndex
    {
        R0 = (uint32_t)0,
        R1 = (uint32_t)1,
        R2 = (uint32_t)2,
        R3 = (uint32_t)3,
        R4 = (uint32_t)4,
        R5 = (uint32_t)5,
        R6 = (uint32_t)6,
        R7 = (uint32_t)7,
        TIMER = (uint32_t)8,
    };

    template <size_t cores, size_t memory_modules, size_t word_size = 128>
    struct Emulator
    {

        Emulator(int memory_size = MEMORY_SIZE)
        {
            Memory memory[memory_modules];

            for (size_t i = 0; i < memory_modules; i++)
            {
                memory[i] = Memory(memory_size);
            }

            bus = BUS(memory, cpus);

            _0 = CPU(0, &bus);

            for (size_t i = 1; i <= cores; i++)
            {
                cpus[i] = CPU(i, &bus);
            }
        }

        auto run(bool step_mode = false)
        {
            _0.execute_instruction(step_mode);

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
        auto set_word(size_t channel, size_t index, std::bitset<word_size> value)
        {
            bus.write(channel, index, value);
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
        auto add_instruction(size_t channel, size_t index, InstructionType type, RegisterIndex dest, RegisterIndex src_1 = RegisterIndex::R0, RegisterIndex src_2 = RegisterIndex::R0)
        {
            char instruction[4] = {(char)type, (char)dest, (char)src_1, (char)src_2};
            bus.write(channel, index, instruction);
        }

        auto add_instruction_to_cpu(char cpu_id, short index, InstructionType type, RegisterIndex dest, RegisterIndex src_1 = RegisterIndex::R0, RegisterIndex src_2 = RegisterIndex::R0)
        {
            if (cpu_id < cores)
                cpus[cpu_id].add_instruction(index, type, dest, src_1, src_2);
        }

        auto add_word_to_cpu(char cpu_id, short index, std::bitset<word_size> value)
        {
            if (cpu_id < cores)
                cpus[cpu_id].add_word(index, value);
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
            Memory() : memory(MEMORY_SIZE, std::bitset<word_size>(0)) {}

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

            void write(size_t index, char (&value)[4])
            {
                std::lock_guard<std::mutex> lock(memory_mutex);
                memory[index] = std::bitset<word_size>((uint32_t)value[0] << 24 | (uint32_t)value[1] << 16 | (uint32_t)value[2] << 8 | (uint32_t)value[3]);
            }

            std::vector<std::bitset<word_size>> memory;
            std::mutex memory_mutex;
        };

        struct CPU;

        struct BUS
        {

            BUS()
            {
                assert(0);
            }

            BUS(const Memory (&memory_array)[memory_modules], CPU (&cpu_array)[cores])
            {
                for (size_t i = 0; i < memory_modules; i++)
                {
                    memory[i] = memory_array[i];
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
                    cpus[i] = cpu_array[i];
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

            auto read(bool memory_operation, char id, size_t channel, size_t index) -> std::bitset<word_size>
            {
                if (memory_operation) [[likely]]
                {
                    assert(channel < channels);

                    return memory[channel].read(index);
                }
                else if (id == 0)
                {
                    assert((channel <= cores) && (channel != 0) && (index < 2028));

                    return cpus[channel].cache[index];
                }

                return std::bitset<word_size>(0);
            }

            auto write(bool memory_operation, char id, size_t channel, size_t index, std::bitset<word_size> value)
            {
                if (memory_operation) [[likely]]
                {
                    assert(channel < channels);

                    memory[channel].write(index, value);
                }

                else if (id == 0)
                {
                    std::lock_guard<std::mutex> lock(cpu_mutex);
                    cpus[channel].cache[index] = value;
                }
            }

            auto write(bool memory_operation, char id, size_t channel, size_t index, char (&value)[4])
            {
                if (memory_operation) [[likely]]
                {
                    assert(channel < channels);

                    memory[channel].write(index, value);
                }
                else if (id == 0)
                {
                    std::lock_guard<std::mutex> lock(cpu_mutex);
                    cpus[channel].cache[index] = std::bitset<word_size>((uint32_t)value[0] << 24 | (uint32_t)value[1] << 16 | (uint32_t)value[2] << 8 | (uint32_t)value[3]);
                }
            }

            size_t in_connections = cores;
            size_t channels = memory_modules;

            bool in_state[cores];
            bool out_state[memory_modules];

            Memory memory[memory_modules];
            CPU cpus[cores];

            std::mutex cpu_mutex;
        };

        struct CPU
        {

            CPU() = default;

            CPU(size_t id, BUS *bus) : id(id), bus(bus)
            {
                decrement(stack_pointer);
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
                if (!bus)
                {
                    std::cerr << "Bus not connected" << std::endl;
                    return;
                }

                if (id == 0) [[unlikely]]
                {
                    if (step_mode)
                    {
                        switch (instruction_cycle)
                        {
                        case 0:

                            if (!initialized)
                            {
                                {
                                    total_cpu_cycles++;

                                    cache[stack_pointer.to_ulong()] = bus->read(false, 0, id, stack_pointer.to_ulong());
                                }

                                {
                                    total_cpu_cycles++;

                                    if (flag[1] == 1) [[unlikely]]
                                    {
                                        flag.reset(1);
                                        initialized = true;
                                        return;
                                    }
                                }
                            }

                            total_cpu_cycles++;
                            instruction_cycle = 1;

                            current_word = cache[stack_pointer.to_ulong()];
                            acc = to_xbits<32>(current_word);
                            current_instruction = decode_instruction(acc);

                            decrement(stack_pointer);
                            decrement(timer);

                        case 1:

                            total_cpu_cycles++;
                            instruction_cycle = 2;

                            current_word = cache[stack_pointer.to_ulong()];
                            acc = to_xbits<32>(current_word);
                            current_instruction = decode_instruction(acc);

                            decrement(stack_pointer);
                            decrement(timer);
                        }
                    }
                }
                else
                {

                    if (((flag & std::bitset<8>(0).set(4)) == std::bitset<8>(0).set(4)))
                    {
                        debug_print("CPU halted\n", "");
                        return;
                    }

                    if (interrupt_enabled && new_instruction) [[unlikely]]
                    {
                        interrupt_enabled = false;
                        flag.reset(0);
                        stack_pointer = interrupt_seg_index;
                    }

                    print_by_force("CPU: ", id);

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

                            total_cpu_cycles++;
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

                            if (current_instruction.cycles != 2) [[likely]]
                            {
                                instruction_cycle = 0;
                                new_instruction = true;
                                break;
                            }

                            decrement(timer);
                            break;

                        case 2:

                            if (timer == 0) [[unlikely]]
                            {
                                interrupt_enabled = true;
                                flag.set(0);
                            }

                            instruction_cycle = 0;
                            total_cpu_cycles++;

                            // execute instruction
                            (this->*current_instruction.opcode)();

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

                        for (size_t i = 1; i < current_instruction.cycles; i++)
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
                // void (*opcode)(void) = nullptr;
                void (FIAT128::Emulator<cores, memory_modules, word_size>::CPU::*opcode)() = nullptr;
                char cycles = 0;
                char dest = 0, src_1 = 0, src_2 = 0;

                /**
                 * @brief decodes an opcode into an instruction
                 * @param opcode the opcode to decode
                 * @return the decoded instruction
                 * @note if the opcode is invalid, the instruction XXX is returned
                 */
                static auto decode_from_opcode(std::bitset<8> opcode) -> Instruction
                {
                    unsigned long opcode_as_char = opcode.to_ulong();

                    if (opcode_as_char > instruction_count || opcode_as_char < 0)
                        return instruction_table[instruction_count];

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

                instruction.dest = (char)get_byte(word, 2).to_ulong();
                instruction.src_1 = (char)get_byte(word, 1).to_ulong();
                instruction.src_2 = (char)get_byte(word, 0).to_ulong();

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
                    result[i] = word[((input_size / 8) - byte - 1) * 8 + i];
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

                cache[index] = std::bitset<word_size>((uint32_t)instruction << 24 | (uint32_t)operand_1 << 16 | (uint32_t)operand_2 << 8 | (uint32_t)operand_3);

                return true;
            }

            auto add_word(short index, std::bitset<word_size> word) -> bool
            {
                assert(index < 2048);

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
            std::bitset<word_size> cache[2048];

            // 128-bit general purpose registers [R0-R7, 6 & 7 are vector registers, 8 is a non-programable temp register]
            std::bitset<word_size> reg[9];

            // the flag register [0 interrupt flag, 1 overflow flag, 2 zero flag, 3 sign flag, 4 hlt flag]
            std::bitset<8> flag;

            // Timer register, counts till the end of time!
            std::bitset<word_size> timer;

            // Stack pointer [to index the cache]
            std::bitset<11> stack_pointer;

            // Segment index (interrupt)
            std::bitset<11> interrupt_seg_index;

            // return addresses stack
            std::bitset<11> return_address[16];

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
                    result[i] = ((bool)a[i]) ^ ((bool)b[i]) ^ overflow;
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

                if (is_bitset_positive(result.first))
                    flag.set(3);

                reg[current_instruction.dest] = result.first;

                debug_print("ADD executed\n", "");
            }

            /**
             * @brief ands two registers and stores the result in the third register
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void AND()
            {
                XXX();
                // reg[current_word.get_byte(2).get_value()] = reg[current_word.get_byte(1).get_value()] & reg[current_word.get_byte(0).get_value()];

                // bool is_zero = reg[current_word.get_byte(2).get_value()].is_zero();
                // bool sign = reg[current_word.get_byte(2).get_value()].get_bit(0);

                // flag = flag & 0b1111'0011;

                // flag = flag | (is_zero << 2);
                // flag = flag | (sign << 3);

                debug_print("AND executed\n", "");
            }

            /**
             * @brief ors two registers and stores the result in the third register
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void OR()
            {
                XXX();
                // reg[current_word.get_byte(2).get_value()] = reg[current_word.get_byte(1).get_value()] | reg[current_word.get_byte(0).get_value()];

                // bool is_zero = reg[current_word.get_byte(2).get_value()].is_zero();
                // bool sign = reg[current_word.get_byte(2).get_value()].get_bit(0);

                // flag = flag & 0b1111'0011;

                // flag = flag | (is_zero << 2);
                // flag = flag | (sign << 3);

                debug_print("OR executed\n", "");
            }

            /**
             * @brief xors two registers and stores the result in the third register
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void XOR()
            {
                XXX();
                // reg[current_word.get_byte(2).get_value()] = reg[current_word.get_byte(1).get_value()] ^ reg[current_word.get_byte(0).get_value()];

                // bool is_zero = reg[current_word.get_byte(2).get_value()].is_zero();
                // bool sign = reg[current_word.get_byte(2).get_value()].get_bit(0);

                // flag = flag & 0b1111'0011;

                // flag = flag | (is_zero << 2);
                // flag = flag | (sign << 3);

                debug_print("XOR executed\n", "");
            }

            /**
             * @brief moves the value of one register to another
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void MOV()
            {
                reg[current_instruction.dest] = reg[current_instruction.src_1];

                debug_print("MOV executed\n", "");
            }

            /**
             * @brief branches to the address specified in the register uncconditionally
             */
            void BUN()
            {
                XXX();
                // pc = current_word.get_address();

                debug_print("BUN executed\n", "");
            }

            /**
             * @brief branches to the address specified in the register if the zero flag is set
             */
            void BIZ()
            {
                XXX();
                // if ((flag & 0b0000'0100) > 0)
                // {
                //     pc = current_word.get_address();

                //     debug_print("BIZ executed\n", "");
                // }

                debug_print("BIZ did not execute\n", "");
            }

            /**
             * @brief branches to the address specified in the register if the zero flag is not set
             */
            void BIN()
            {
                XXX();
                // if ((flag & 0b0000'0100) == 0)
                // {
                //     pc = current_word.get_address();

                //     debug_print("BNZ executed\n", "");
                // }

                debug_print("BNZ did not execute\n", "");
            }

            /**
             * @brief loads the value in the memory address specified in a register into a register
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void LDA()
            {
                XXX();
                // reg[current_word.get_byte(2).get_value()] = memory[current_word.get_address().get_value()];

                // bool is_zero = reg[current_word.get_byte(2).get_value()].is_zero();
                // bool sign = reg[current_word.get_byte(2).get_value()].get_bit(0);

                // flag = flag & 0b1111'0011;

                // flag = flag | (is_zero << 2);
                // flag = flag | (sign << 3);

                debug_print("LDA executed\n", "");
            }

            /**
             * @brief stores the value in the register in the memory address specified in the register
             */
            void STA()
            {
                XXX();
                // memory[current_word.get_address().get_value()] = reg[current_word.get_byte(2).get_value()];

                debug_print("STA executed\n", "");
            }

            /**
             * @brief loads a value in cache memory into a register
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void LDR()
            {
                reg[current_instruction.dest] = cache[reg[current_instruction.src_1].to_ulong()];

                debug_print("LDR executed\n", "");
            }

            /**
             * @brief stores the value in the register in the cache memory address specified in the register
             */
            void STR()
            {
                XXX();
                // memory[current_word.get_address().get_value()] = reg[current_word.get_byte(2).get_value()];

                debug_print("STR executed\n", "");
            }

            /**
             * @brief compares two registers and sets the zero flag if they are equal
             */
            void EQL()
            {
                XXX();
                // reg[current_word.get_byte(2).get_value()] = reg[current_word.get_byte(1).get_value()] - reg[current_word.get_byte(0).get_value()];

                // if (reg[current_word.get_byte(2).get_value()].is_zero())
                // {
                //     flag = flag | 0b0000'0100;
                // }
                // else
                // {
                //     flag = flag & 0b1111'1011;
                // }

                debug_print("EQ executed\n", "");
            }

            /**
             * @brief compares two registers and sets the sign flag if the first register is greater than the second register
             */
            void GRT() // Note(AbduEhab): needs to be revised
            {
                XXX();
                // reg[current_word.get_byte(2).get_value()] = reg[current_word.get_byte(1).get_value()] - reg[current_word.get_byte(0).get_value()];

                // if (reg[current_word.get_byte(2).get_value()].get_bit(0))
                // {
                //     flag = flag | 0b0000'1000;
                // }
                // else
                // {
                //     flag = flag & 0b1111'0111;
                // }

                debug_print("GT executed\n", "");
            }

            /**
             * @brief shifts a register left by one
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void SHL()
            {
                XXX();
                // reg[current_word.get_byte(2).get_value()] = (reg[current_word.get_byte(2).get_value()] << 1);

                // bool is_zero = reg[current_word.get_byte(2).get_value()].is_zero();
                // bool sign = reg[current_word.get_byte(2).get_value()].get_bit(0);

                // flag = flag & 0b1111'0011;

                // flag = flag | (is_zero << 2);
                // flag = flag | (sign << 3);

                debug_print("SHL executed\n", "");
            }

            /**
             * @brief shifts a register right by one
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void SHR()
            {
                XXX();
                // reg[current_word.get_byte(2).get_value()] = (reg[current_word.get_byte(2).get_value()] >> 1);

                // bool is_zero = reg[current_word.get_byte(2).get_value()].is_zero();
                // bool sign = reg[current_word.get_byte(2).get_value()].get_bit(0);

                // flag = flag & 0b1111'0011;

                // flag = flag | (is_zero << 2);
                // flag = flag | (sign << 3);

                debug_print("SHR executed\n", "");
            }

            /**
             * @brief rotates a register left by one
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void ROL()
            {
                XXX();
                // reg[current_word.get_byte(2).get_value()] = (reg[current_word.get_byte(2).get_value()] << 1) | (reg[current_word.get_byte(2).get_value()].get_bit(128));

                // bool is_zero = reg[current_word.get_byte(2).get_value()].is_zero();
                // bool sign = reg[current_word.get_byte(2).get_value()].get_bit(0);

                // flag = flag & 0b1111'0011;

                // flag = flag | (is_zero << 2);
                // flag = flag | (sign << 3);

                debug_print("ROL executed\n", "");
            }

            /**
             * @brief rotates a register right by one
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void ROR()
            {
                XXX();
                // reg[Byte::get_byte(current_word, 2).get_value()] = (reg[Byte::get_byte(current_word, 2).get_value()] >> 1) | (reg[Byte::get_byte(current_word, 2).get_value()][0] << (128 - 1));

                // bool is_zero = reg[Byte::get_byte(current_word, 2).get_value()] == 0;
                // bool sign = reg[Byte::get_byte(current_word, 2).get_value()][0];

                // flag = flag & 0b1111'0011;

                // flag = flag | (is_zero << 2);
                // flag = flag | (sign << 3);

                debug_print("ROR executed\n", "");
            }

            /**
             * @brief Halts the cpu
             */
            void HLT()
            {
                flag |= 0b0001'0000;
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

            // instruction count
            static const int instruction_count = 20;

            // instrcution table
            static inline Instruction instruction_table[instruction_count] = {
                {"XXX", &CPU::XXX, 2},
                {"ADD", &CPU::ADD, 2},
                {"AND", &CPU::AND, 2},
                {"OR", &CPU::OR, 2},
                {"XOR", &CPU::XOR, 2},
                {"MOV", &CPU::MOV, 2},
                {"BUN", &CPU::BUN, 2},
                {"BIZ", &CPU::BIZ, 3},
                {"BIN", &CPU::BIN, 3},
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

        CPU _0;
        CPU cpus[cores];
        BUS bus;
    };
}; // FIAT128