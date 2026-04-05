#pragma once

#include <algorithm>
#include <array>
#include <assert.h>
#include <bitset>
#include <cstdint>
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
#include <type_traits>
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

        if constexpr (std::is_integral_v<T>)
        {
            std::uniform_int_distribution<T> dis(min, max);
            return dis(gen);
        }
        else
        {
            std::uniform_real_distribution<T> dis(min, max);
            return dis(gen);
        }
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

#ifdef DEBUGs

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

    enum class InstructionAccess
    {
        CacheOnly,
        MemoryOnly,
        Control,
        Internal,
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

        struct CpuIdResetter
        {
            CpuIdResetter()
            {
                id_counter = 0;
            }
        };

        Emulator(int memory_size = MEMORY_SIZE)
        {
            for (size_t i = 0; i < memory_modules; i++)
                memory[i] = Memory(to_size_t(memory_size));

            bus = BUS(memory, cpus);

            for (size_t i = 0; i <= cores; i++)
                cpus[i].set_bus(&bus);
        }

        Emulator(const std::array<size_t, memory_modules> &memory_sizes)
        {
            for (size_t i = 0; i < memory_modules; i++)
                memory[i] = Memory(memory_sizes[i]);

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

            execute_gpu_shader();
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
         * @brief Set the instruction on the given memory channel and index
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
            if (to_size_t(cpu_id) <= cores)
                cpus[to_size_t(cpu_id)].add_instruction(index, type, dest, src_1, src_2);
        }

        auto set_memory_instruction_in_memory(size_t channel, size_t index, InstructionType type, RegisterIndex reg, unsigned char module, unsigned short address) -> void
        {
            const auto packed_operand = static_cast<unsigned char>((to_uchar(reg) << 4) | (module & 0x0F));
            std::bitset<word_size> encoded(u32(type) << 24 | u32(packed_operand) << 16 | u32(address));
            encoded <<= (word_size - 32);
            bus.write(true, 0, channel, index, encoded);
        }

        auto set_memory_instruction_in_cpu(char cpu_id, short index, InstructionType type, RegisterIndex reg, unsigned char module, unsigned short address) -> void
        {
            if (to_size_t(cpu_id) <= cores)
                cpus[to_size_t(cpu_id)].add_memory_instruction(index, type, reg, module, address);
        }

        auto set_word_in_cpu(char cpu_id, short index, std::bitset<word_size> value)
        {
            if (to_size_t(cpu_id) <= cores)
                cpus[to_size_t(cpu_id)].add_word(index, value);
        }

        struct CpuRenderState
        {
            size_t id = 0;
            bool halted = false;
            size_t stack_pointer = 0;
            std::string current_instruction;
            std::string current_instruction_detail;
            std::bitset<8> flags;
            std::array<std::bitset<word_size>, 9> registers;
        };

        struct MemoryWriteRenderEvent
        {
            size_t sequence = 0;
            size_t cpu_id = 0;
            size_t channel = 0;
            size_t index = 0;
            std::bitset<word_size> value;
        };

        auto get_cpu_render_state() -> std::array<CpuRenderState, cores + 1>
        {
            std::array<CpuRenderState, cores + 1> states;

            for (size_t i = 0; i <= cores; ++i)
            {
                states[i].id = cpus[i].id;
                states[i].halted = cpus[i].flag.test(CPU::FlagIndex::HALT);
                states[i].stack_pointer = cpus[i].stack_pointer.to_ulong();
                states[i].current_instruction = cpus[i].next_instruction_name();
                states[i].current_instruction_detail = cpus[i].next_instruction_detail();
                states[i].flags = cpus[i].flag;

                for (size_t reg_index = 0; reg_index < states[i].registers.size(); ++reg_index)
                    states[i].registers[reg_index] = cpus[i].reg[reg_index];
            }

            return states;
        }

        auto get_memory_write_events_since(size_t last_sequence) const -> std::vector<MemoryWriteRenderEvent>
        {
            auto raw_events = bus.get_memory_write_events_since(last_sequence);
            std::vector<MemoryWriteRenderEvent> events;
            events.reserve(raw_events.size());

            for (const auto &event : raw_events)
            {
                events.push_back({event.sequence, event.cpu_id, event.channel, event.index, event.value});
            }

            return events;
        }

        auto get_memory_snapshot() const -> std::array<std::vector<std::bitset<word_size>>, memory_modules>
        {
            std::array<std::vector<std::bitset<word_size>>, memory_modules> snapshot;

            for (size_t channel = 0; channel < memory_modules; ++channel)
            {
                snapshot[channel].reserve(memory[channel].memory.size());
                for (const auto &word : memory[channel].memory)
                    snapshot[channel].push_back(word);
            }

            return snapshot;
        }

        auto get_gpu_framebuffer() const -> const std::vector<uint32_t> &
        {
            return gpu_framebuffer;
        }

        auto latest_memory_write_sequence() const -> size_t
        {
            return bus.latest_memory_write_sequence();
        }

        auto set_cpu_entry_point(size_t cpu_id, size_t entry_point) -> void
        {
            if (cpu_id > cores)
                return;

            cpus[cpu_id].stack_pointer = std::bitset<u32(std::log2(cache_size))>(entry_point);
            cpus[cpu_id].flag.reset();
            cpus[cpu_id].initialized = true;
            cpus[cpu_id].new_instruction = true;
            cpus[cpu_id].instruction_cycle = 0;
            cpus[cpu_id].interrupt_enabled = false;
        }

        auto set_cpu_halt_state(size_t cpu_id, bool halted) -> void
        {
            if (cpu_id > cores)
                return;

            if (halted)
                cpus[cpu_id].flag.set(CPU::FlagIndex::HALT);
            else
                cpus[cpu_id].flag.reset(CPU::FlagIndex::HALT);
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
        /* This is the Memory struct, which represents a memory module in the emulator. It has a vector of bitsets to represent the memory, and a mutex to protect it from concurrent access. */
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

    private:
        enum class GpuOpcode : uint8_t
        {
            Load = 0,
            Store = 1,
            PixelStore = 2,
            Add = 3,
            Sub = 4,
            Mul = 5,
            Div = 6,
            Mod = 7,
            Neg = 8,
            Abs = 9,
            Dot = 10,
            Cross = 11,
            Length = 12,
            Normalize = 13,
            Lerp = 14,
            Clamp = 15,
            Eq = 16,
            Ne = 17,
            Lt = 18,
            Le = 19,
            Gt = 20,
            Ge = 21,
            And = 22,
            Or = 23,
            Xor = 24,
            Not = 25,
            Shl = 26,
            Shr = 27,
            Jmp = 28,
            Jz = 29,
            Jnz = 30,
            Halt = 31,
            ReadInvocationIdX = 32,
            ReadInvocationIdY = 33,
            ReadWidth = 34,
            ReadHeight = 35,
            PackRgb = 36,
            PackRgba = 37,
            UnpackRgb = 38,
            UnpackRgba = 39,
        };

        struct GpuInstruction
        {
            GpuOpcode opcode = GpuOpcode::Halt;
            uint8_t dst = 0;
            uint8_t src1 = 0;
            uint8_t src2 = 0;
            uint32_t immediate = 0;
        };

        static constexpr size_t gpu_width = 400;
        static constexpr size_t gpu_height = 600;
        static constexpr size_t gpu_register_count = 16;
        static constexpr size_t gpu_entry_point = 3;
        static constexpr size_t gpu_step_limit = 2048;

        static auto decode_gpu_instruction(const std::bitset<word_size> &word) -> GpuInstruction
        {
            const uint64_t raw = word.to_ullong();

            return {
                static_cast<GpuOpcode>(raw & 0xFFULL),
                static_cast<uint8_t>((raw >> 8) & 0xFFULL),
                static_cast<uint8_t>((raw >> 16) & 0xFFULL),
                static_cast<uint8_t>((raw >> 24) & 0xFFULL),
                static_cast<uint32_t>((raw >> 32) & 0xFFFFFFFFULL),
            };
        }

        auto ensure_gpu_framebuffer() -> void
        {
            const size_t required_size = gpu_width * gpu_height;
            if (gpu_framebuffer.size() != required_size)
                gpu_framebuffer.assign(required_size, 0xFF000000U);
        }

        auto write_gpu_pixel(size_t x, size_t y, uint32_t color) -> void
        {
            if (x >= gpu_width || y >= gpu_height)
                return;

            ensure_gpu_framebuffer();
            gpu_framebuffer[y * gpu_width + x] = 0xFF000000U | (color & 0x00FFFFFFU);
        }

        static auto clamp_register_index(uint8_t index) -> size_t
        {
            return index % gpu_register_count;
        }

        static auto low_32(std::uint64_t value) -> uint32_t
        {
            return static_cast<uint32_t>(value & 0xFFFFFFFFULL);
        }

        static auto pack_rgb_from_scalars(int64_t red, int64_t green, int64_t blue) -> uint32_t
        {
            return (static_cast<uint32_t>(red) & 0xFFU) << 16U |
                   (static_cast<uint32_t>(green) & 0xFFU) << 8U |
                   (static_cast<uint32_t>(blue) & 0xFFU);
        }

        auto execute_gpu_invocation(size_t invocation_x, size_t invocation_y) -> void
        {
            std::array<int64_t, gpu_register_count> registers{};
            registers[12] = static_cast<int64_t>(invocation_x);
            registers[13] = static_cast<int64_t>(invocation_y);
            registers[14] = static_cast<int64_t>(gpu_width);
            registers[15] = static_cast<int64_t>(gpu_height);

            size_t pc = gpu_entry_point;

            for (size_t step = 0; step < gpu_step_limit && pc < memory[3].memory.size(); ++step)
            {
                const auto instruction_word = bus.read(true, 0, 3, pc);
                ++pc;

                const auto instruction = decode_gpu_instruction(instruction_word);
                const size_t dst = clamp_register_index(instruction.dst);
                const size_t src1 = clamp_register_index(instruction.src1);
                const size_t src2 = clamp_register_index(instruction.src2);

                auto &dst_reg = registers[dst];
                const auto &src1_reg = registers[src1];
                const auto &src2_reg = registers[src2];

                switch (instruction.opcode)
                {
                case GpuOpcode::Load:
                {
                    const auto loaded = bus.read(true, 0, 3, static_cast<size_t>(instruction.immediate));
                    dst_reg = static_cast<int64_t>(low_32(loaded.to_ullong()));
                    break;
                }
                case GpuOpcode::Store:
                    set_word_in_memory(3, static_cast<size_t>(instruction.immediate), std::bitset<word_size>(static_cast<uint64_t>(dst_reg)));
                    break;
                case GpuOpcode::PixelStore:
                    write_gpu_pixel(invocation_x, invocation_y, static_cast<uint32_t>(dst_reg));
                    break;
                case GpuOpcode::Add:
                    dst_reg = src1_reg + src2_reg;
                    break;
                case GpuOpcode::Sub:
                    dst_reg = src1_reg - src2_reg;
                    break;
                case GpuOpcode::Mul:
                    dst_reg = src1_reg * src2_reg;
                    break;
                case GpuOpcode::Div:
                    dst_reg = (src2_reg == 0) ? 0 : (src1_reg / src2_reg);
                    break;
                case GpuOpcode::Mod:
                    dst_reg = (src2_reg == 0) ? 0 : (src1_reg % src2_reg);
                    break;
                case GpuOpcode::Neg:
                    dst_reg = -src1_reg;
                    break;
                case GpuOpcode::Abs:
                    dst_reg = std::llabs(src1_reg);
                    break;
                case GpuOpcode::Dot:
                    dst_reg = registers[clamp_register_index(static_cast<uint8_t>(src1 + 0))] * registers[clamp_register_index(static_cast<uint8_t>(src2 + 0))] +
                              registers[clamp_register_index(static_cast<uint8_t>(src1 + 1))] * registers[clamp_register_index(static_cast<uint8_t>(src2 + 1))] +
                              registers[clamp_register_index(static_cast<uint8_t>(src1 + 2))] * registers[clamp_register_index(static_cast<uint8_t>(src2 + 2))];
                    break;
                case GpuOpcode::Cross:
                {
                    const auto ax = registers[clamp_register_index(static_cast<uint8_t>(src1 + 0))];
                    const auto ay = registers[clamp_register_index(static_cast<uint8_t>(src1 + 1))];
                    const auto az = registers[clamp_register_index(static_cast<uint8_t>(src1 + 2))];
                    const auto bx = registers[clamp_register_index(static_cast<uint8_t>(src2 + 0))];
                    const auto by = registers[clamp_register_index(static_cast<uint8_t>(src2 + 1))];
                    const auto bz = registers[clamp_register_index(static_cast<uint8_t>(src2 + 2))];

                    registers[clamp_register_index(instruction.dst + 0)] = ay * bz - az * by;
                    registers[clamp_register_index(instruction.dst + 1)] = az * bx - ax * bz;
                    registers[clamp_register_index(instruction.dst + 2)] = ax * by - ay * bx;
                    break;
                }
                case GpuOpcode::Length:
                {
                    const long double x = static_cast<long double>(registers[clamp_register_index(static_cast<uint8_t>(src1 + 0))]);
                    const long double y = static_cast<long double>(registers[clamp_register_index(static_cast<uint8_t>(src1 + 1))]);
                    const long double z = static_cast<long double>(registers[clamp_register_index(static_cast<uint8_t>(src1 + 2))]);
                    dst_reg = static_cast<int64_t>(std::sqrt((x * x) + (y * y) + (z * z)));
                    break;
                }
                case GpuOpcode::Normalize:
                {
                    const long double x = static_cast<long double>(registers[clamp_register_index(static_cast<uint8_t>(src1 + 0))]);
                    const long double y = static_cast<long double>(registers[clamp_register_index(static_cast<uint8_t>(src1 + 1))]);
                    const long double z = static_cast<long double>(registers[clamp_register_index(static_cast<uint8_t>(src1 + 2))]);
                    const long double length = std::sqrt((x * x) + (y * y) + (z * z));
                    const long double scale = (length == 0.0L) ? 0.0L : (1.0L / length);

                    registers[clamp_register_index(instruction.dst + 0)] = static_cast<int64_t>(std::llround(x * scale * 1000000.0L));
                    registers[clamp_register_index(instruction.dst + 1)] = static_cast<int64_t>(std::llround(y * scale * 1000000.0L));
                    registers[clamp_register_index(instruction.dst + 2)] = static_cast<int64_t>(std::llround(z * scale * 1000000.0L));
                    break;
                }
                case GpuOpcode::Lerp:
                {
                    const int64_t factor = static_cast<int64_t>(instruction.immediate & 0xFFFFU);
                    dst_reg = src1_reg + (((src2_reg - src1_reg) * factor) / 65535);
                    break;
                }
                case GpuOpcode::Clamp:
                {
                    const int64_t minimum = src2_reg;
                    const int64_t maximum = static_cast<int64_t>(instruction.immediate);
                    dst_reg = std::clamp(src1_reg, minimum, maximum);
                    break;
                }
                case GpuOpcode::Eq:
                    dst_reg = (src1_reg == src2_reg) ? 1 : 0;
                    break;
                case GpuOpcode::Ne:
                    dst_reg = (src1_reg != src2_reg) ? 1 : 0;
                    break;
                case GpuOpcode::Lt:
                    dst_reg = (src1_reg < src2_reg) ? 1 : 0;
                    break;
                case GpuOpcode::Le:
                    dst_reg = (src1_reg <= src2_reg) ? 1 : 0;
                    break;
                case GpuOpcode::Gt:
                    dst_reg = (src1_reg > src2_reg) ? 1 : 0;
                    break;
                case GpuOpcode::Ge:
                    dst_reg = (src1_reg >= src2_reg) ? 1 : 0;
                    break;
                case GpuOpcode::And:
                    dst_reg = src1_reg & src2_reg;
                    break;
                case GpuOpcode::Or:
                    dst_reg = src1_reg | src2_reg;
                    break;
                case GpuOpcode::Xor:
                    dst_reg = src1_reg ^ src2_reg;
                    break;
                case GpuOpcode::Not:
                    dst_reg = ~src1_reg;
                    break;
                case GpuOpcode::Shl:
                    dst_reg = src1_reg << static_cast<int>(instruction.immediate & 0x3FU);
                    break;
                case GpuOpcode::Shr:
                    dst_reg = static_cast<int64_t>(static_cast<uint64_t>(src1_reg) >> static_cast<int>(instruction.immediate & 0x3FU));
                    break;
                case GpuOpcode::Jmp:
                    pc = static_cast<size_t>(instruction.immediate);
                    break;
                case GpuOpcode::Jz:
                    if (dst_reg == 0)
                        pc = static_cast<size_t>(instruction.immediate);
                    break;
                case GpuOpcode::Jnz:
                    if (dst_reg != 0)
                        pc = static_cast<size_t>(instruction.immediate);
                    break;
                case GpuOpcode::ReadInvocationIdX:
                    dst_reg = static_cast<int64_t>(invocation_x);
                    break;
                case GpuOpcode::ReadInvocationIdY:
                    dst_reg = static_cast<int64_t>(invocation_y);
                    break;
                case GpuOpcode::ReadWidth:
                    dst_reg = static_cast<int64_t>(gpu_width);
                    break;
                case GpuOpcode::ReadHeight:
                    dst_reg = static_cast<int64_t>(gpu_height);
                    break;
                case GpuOpcode::PackRgb:
                    dst_reg = static_cast<int64_t>(pack_rgb_from_scalars(src1_reg, src2_reg, instruction.immediate));
                    break;
                case GpuOpcode::PackRgba:
                    dst_reg = static_cast<int64_t>(((static_cast<uint32_t>(src1_reg) & 0xFFU) << 24U) |
                                                   ((static_cast<uint32_t>(src2_reg) & 0xFFU) << 16U) |
                                                   ((instruction.immediate & 0xFFFFU) << 0U));
                    break;
                case GpuOpcode::UnpackRgb:
                    dst_reg = static_cast<int64_t>(static_cast<uint32_t>(src1_reg) & 0x00FFFFFFU);
                    break;
                case GpuOpcode::UnpackRgba:
                    dst_reg = static_cast<int64_t>(static_cast<uint32_t>(src1_reg));
                    break;
                case GpuOpcode::Halt:
                    return;
                default:
                    return;
                }
            }

        }

        auto execute_gpu_shader() -> void
        {
            if constexpr (memory_modules <= 3)
                return;

            ensure_gpu_framebuffer();

            const auto control_word = bus.read(true, 0, 3, 0);
            const uint8_t start_byte = static_cast<uint8_t>(control_word.to_ullong() & 0xFFU);
            if (start_byte != 0xFFU)
                return;

            const unsigned int requested_threads = std::max(1U, std::min<unsigned int>(std::thread::hardware_concurrency(), static_cast<unsigned int>(gpu_height)));
            const size_t rows_per_thread = (gpu_height + static_cast<size_t>(requested_threads) - 1) / static_cast<size_t>(requested_threads);
            std::vector<std::thread> workers;
            workers.reserve(requested_threads);

            for (unsigned int thread_index = 0; thread_index < requested_threads; ++thread_index)
            {
                const size_t start_row = static_cast<size_t>(thread_index) * rows_per_thread;
                const size_t end_row = std::min(gpu_height, start_row + rows_per_thread);
                if (start_row >= end_row)
                    break;

                workers.emplace_back([&, start_row, end_row]()
                {
                    for (size_t y = start_row; y < end_row; ++y)
                    {
                        for (size_t x = 0; x < gpu_width; ++x)
                            execute_gpu_invocation(x, y);
                    }
                });
            }

            for (auto &worker : workers)
                worker.join();

            set_word_in_memory(3, 0, std::bitset<word_size>(0));
        }

        std::vector<uint32_t> gpu_framebuffer;

        struct BUS
        {

            struct MemoryWriteLogEntry
            {
                size_t sequence = 0;
                size_t cpu_id = 0;
                size_t channel = 0;
                size_t index = 0;
                std::bitset<word_size> value;
            };

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

                cpus[cores] = &cpu_array[cores];
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

                cpus[cores] = other.cpus[cores];
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

                cpus[cores] = other.cpus[cores];
                return *this;
            }

            // move assignment
            BUS &operator=(BUS &&other) = default;

            auto read(bool memory_operation, size_t id, size_t channel, size_t index) -> std::bitset<word_size>
            {
                if (memory_operation) [[likely]]
                {
                    if (channel >= channels || index >= memory[channel]->memory.size())
                        return std::bitset<word_size>(0);

                    return memory[channel]->read(index);
                }
                else if (id == 0)
                {
                    if (channel > cores || index >= cache_size)
                        return std::bitset<word_size>(0);

                    return cpus[channel]->cache[index];
                }

                return std::bitset<word_size>(0);
            }

            auto write(bool memory_operation, size_t id, size_t channel, size_t index, std::bitset<word_size> value)
            {
                if (memory_operation) [[likely]]
                {
                    if (channel >= channels || index >= memory[channel]->memory.size())
                        return;

                    memory[channel]->write(index, value);
                    append_memory_write_event(id, channel, index, value);
                }
                else
                {
                    if (id > cores || index >= cache_size)
                        return;

                    std::lock_guard<std::mutex> lock(cpu_mutex);
                    cpus[id]->cache[index] = value;
                }
            }

            auto write(bool memory_operation, size_t id, size_t channel, size_t index, unsigned char (&value)[4])
            {
                if (memory_operation) [[likely]]
                {
                    if (channel >= channels || index >= memory[channel]->memory.size())
                        return;

                    memory[channel]->write(index, value);

                    auto encoded = (std::bitset<word_size>(u32(value[0]) << 24 | u32(value[1]) << 16 | u32(value[2]) << 8 | u32(value[3]))) <<= 96;
                    append_memory_write_event(id, channel, index, encoded);
                }
                else
                {
                    if (id > cores || index >= cache_size)
                        return;

                    std::lock_guard<std::mutex> lock(cpu_mutex);
                    cpus[id]->cache[index] = (std::bitset<word_size>(u32(value[0]) << 24 | u32(value[1]) << 16 | u32(value[2]) << 8 | u32(value[3]))) <<= 96;
                }
            }

            auto get_memory_write_events_since(size_t last_sequence) const -> std::vector<MemoryWriteLogEntry>
            {
                std::lock_guard<std::mutex> lock(memory_write_log_mutex);
                std::vector<MemoryWriteLogEntry> events;

                for (const auto &entry : memory_write_log)
                {
                    if (entry.sequence > last_sequence)
                        events.push_back(entry);
                }

                return events;
            }

            auto latest_memory_write_sequence() const -> size_t
            {
                std::lock_guard<std::mutex> lock(memory_write_log_mutex);
                return memory_write_sequence;
            }

            size_t in_connections = cores;
            size_t channels = memory_modules;

            bool in_state[(cores > 0) ? cores : 1];
            bool out_state[memory_modules];

            Memory *memory[memory_modules];
            CPU *cpus[cores + 1];

            std::deque<MemoryWriteLogEntry> memory_write_log;
            size_t memory_write_sequence = 0;
            mutable std::mutex memory_write_log_mutex;

            std::mutex cpu_mutex;

        private:
            auto append_memory_write_event(size_t cpu_id, size_t channel, size_t index, const std::bitset<word_size> &value) -> void
            {
                std::lock_guard<std::mutex> lock(memory_write_log_mutex);
                ++memory_write_sequence;

                memory_write_log.push_back({memory_write_sequence, cpu_id, channel, index, value});
                if (memory_write_log.size() > 512)
                    memory_write_log.pop_front();
            }
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
                    flag.reset(FlagIndex::OVERFLOW);
                else
                    flag.set(FlagIndex::HALT);
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
                    value.reset();
                    flag.set(FlagIndex::OVERFLOW);
                    return;
                }

                for (size_t i = 0; i < bitset_size; i++)
                {
                    if (value[i] == 0)
                    {
                        value.set(i);
                        break;
                    }

                    value.reset(i);
                }
            }

            // bitset<word_size> decrement
            template <size_t bitset_size>
            auto decrement(std::bitset<bitset_size> &value) -> void
            {
                if (is_bitset_zero(value)) [[unlikely]]
                {
                    value = ~value;
                    flag.set(FlagIndex::OVERFLOW);
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
                    if (flag[FlagIndex::OVERFLOW] == 1) [[unlikely]]
                    {
                        flag.reset(FlagIndex::OVERFLOW);
                        initialized = true;
                        goto emerg_break;
                    }

                    total_cpu_cycles++;

                    cache[stack_pointer.to_ulong()] = bus->read(true, id, 0, stack_pointer.to_ulong());

                    decrement(stack_pointer);
                    decrement(timer);
                    return;
                }
                else
                {
                emerg_break:

                    if ((flag & std::bitset<8>(0).set(FlagIndex::HALT)).to_ulong() > 0)
                    {
                        debug_print(std::string("CPU ").append(std::to_string(id)), " halted");
                        return;
                    }

                    if (interrupt_enabled && new_instruction) [[unlikely]]
                    {
                        interrupt_enabled = false;
                        flag.reset(FlagIndex::INTERRUPT);
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
                                flag.set(FlagIndex::INTERRUPT);
                            }

                            instruction_cycle = 1;
                            total_cpu_cycles++;

                            // fetch instruction
                            current_word = cache[stack_pointer.to_ulong()];
                            acc = to_xbits<32>(current_word);
                            current_instruction = decode_instruction(acc);

                            decrement(timer);
                            break;

                        case 1:

                            if (timer == 0) [[unlikely]]
                            {
                                interrupt_enabled = true;
                                flag.set(FlagIndex::INTERRUPT);
                            }

                            instruction_cycle = 0;
                            total_cpu_cycles++;

                            // execute instruction
                            (this->*current_instruction.opcode)();

                            new_instruction = true;

                            decrement(stack_pointer);
                            decrement(timer);
                            break;
                        }
                    }
                    else
                    {
                        if (timer == 0) [[unlikely]]
                        {
                            interrupt_enabled = true;
                            flag.set(FlagIndex::INTERRUPT);
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
                InstructionAccess access = InstructionAccess::Internal;
                unsigned char module = 0;
                unsigned short address = 0;

                /**
                 * @brief decodes an opcode into an instruction
                 * @param opcode the opcode to decode
                 * @return the decoded instruction
                 * @note if the opcode is invalid, the instruction XXX is returned
                 */
                static auto decode_from_opcode(std::bitset<8> &opcode) -> Instruction
                {
                    auto opcode_as_char = to_uchar(opcode.to_ulong());

                    if (opcode_as_char >= instruction_count)
                        return instruction_table[instruction_count - 1]; // HLT instruction

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

                const auto high_byte = to_uchar(get_byte(word, 2).to_ulong());
                const auto mid_byte = to_uchar(get_byte(word, 1).to_ulong());
                const auto low_byte = to_uchar(get_byte(word, 0).to_ulong());

                if (instruction.access == InstructionAccess::MemoryOnly)
                {
                    instruction.dest = static_cast<unsigned char>(high_byte >> 4);
                    instruction.module = static_cast<unsigned char>(high_byte & 0x0F);
                    instruction.address = static_cast<unsigned short>((u32(mid_byte) << 8) | u32(low_byte));
                }
                else
                {
                    instruction.dest = high_byte;
                    instruction.src_1 = mid_byte;
                    instruction.src_2 = low_byte;
                }

                return instruction;
            }

            template <size_t target_size>
            auto to_xbits(std::bitset<word_size> &word) -> std::bitset<target_size>
            {
                std::bitset<target_size> result;

                for (size_t i = 0; i < target_size; ++i)
                    result[i] = word[word_size - target_size + i];

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
                cache[index] <<= (word_size - 32);

                return true;
            }

            auto add_memory_instruction(short index, InstructionType instruction, RegisterIndex operand_1, unsigned char module, unsigned short address) -> bool
            {
                if (!bus)
                {
                    std::cerr << "Bus not connected" << std::endl;
                    return false;
                }

                const auto packed_operand = static_cast<unsigned char>((to_uchar(operand_1) << 4) | (module & 0x0F));
                cache[index] = std::bitset<word_size>(u32(instruction) << 24 | u32(packed_operand) << 16 | u32(address));
                cache[index] <<= (word_size - 32);

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

            auto next_instruction_name() -> std::string
            {
                auto current_stack_index = stack_pointer.to_ulong();
                if (current_stack_index >= cache_size)
                    return "OUT_OF_RANGE";

                std::bitset<word_size> next_word = cache[current_stack_index];
                auto next_word_u32 = to_xbits<32>(next_word);
                auto decoded = decode_instruction(next_word_u32);
                return std::string(decoded.name);
            }

            auto next_instruction_detail() -> std::string
            {
                auto current_stack_index = stack_pointer.to_ulong();
                if (current_stack_index >= cache_size)
                    return "OUT_OF_RANGE";

                std::bitset<word_size> next_word = cache[current_stack_index];
                auto next_word_u32 = to_xbits<32>(next_word);
                auto decoded = decode_instruction(next_word_u32);

                std::ostringstream out;
                out << decoded.name << " [" << instruction_access_name(decoded.access) << "]";

                if (decoded.access == InstructionAccess::MemoryOnly)
                {
                    out << " R:R" << static_cast<int>(decoded.dest)
                        << " M:" << static_cast<int>(decoded.module)
                        << " A:0x" << std::hex << std::setw(4) << std::setfill('0') << decoded.address << std::dec << std::setfill(' ');
                }
                else
                {
                    out << " D:R" << static_cast<int>(decoded.dest)
                        << " A:R" << static_cast<int>(decoded.src_1)
                        << " B:R" << static_cast<int>(decoded.src_2);
                }

                out << " CYC:" << static_cast<int>(decoded.cycles);

                return out.str();
            }

            auto instruction_access_name(InstructionAccess access) const -> std::string_view
            {
                switch (access)
                {
                case InstructionAccess::CacheOnly:
                    return "CACHE";
                case InstructionAccess::MemoryOnly:
                    return "MEM";
                case InstructionAccess::Control:
                    return "CTRL";
                case InstructionAccess::Internal:
                default:
                    return "INT";
                }
            }

            // cpu cache
            std::bitset<word_size> cache[cache_size];

            // 128-bit general purpose registers [R0-R7, 6 & 7 are vector registers, 8 is a non-programable temp register]
            std::bitset<word_size> reg[9];

            // the flag register [0 interrupt flag, 1 overflow flag, 2 zero flag, 3 sign flag, 4 hlt flag]
            enum FlagIndex
            {
                INTERRUPT = u32(0),
                OVERFLOW = u32(1),
                ZERO = u32(2),
                SIGN = u32(3),
                HALT = u32(4),
            };
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
                return a[input_size - 1] == 0;
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
                    flag.set(FlagIndex::OVERFLOW);

                if (is_bitset_zero(result.first))
                    flag.set(FlagIndex::ZERO);

                if (!is_bitset_positive(result.first))
                    flag.set(FlagIndex::SIGN);

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
                    flag.set(FlagIndex::ZERO);
                if (!is_bitset_positive(reg[current_instruction.dest]))
                    flag.set(FlagIndex::SIGN);

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
                    flag.set(FlagIndex::ZERO);

                if (!is_bitset_positive(reg[current_instruction.dest]))
                    flag.set(FlagIndex::SIGN);

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
                    flag.set(FlagIndex::ZERO);

                if (!is_bitset_positive(reg[current_instruction.dest]))
                    flag.set(FlagIndex::SIGN);

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
                    flag.set(FlagIndex::ZERO);

                if (!is_bitset_positive(reg[current_instruction.dest]))
                    flag.set(FlagIndex::SIGN);

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
             * @brief branches to the address specified in the register if the negative flag is set
             */
            void BIN()
            {
                if ((flag & std::bitset<8>(0).set(FlagIndex::SIGN)).to_ulong() > 0)
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
                reg[current_instruction.dest] = bus->read(true, id, current_instruction.module, current_instruction.address);

                if (is_bitset_zero(reg[current_instruction.dest]))
                    flag.set(FlagIndex::ZERO);
                if (!is_bitset_positive(reg[current_instruction.dest]))
                    flag.set(FlagIndex::SIGN);

                debug_print(std::string("CPU ").append(std::to_string(id)), " LDA executed");
            }

            /**
             * @brief stores the value in the register in the memory address specified in the register
             */
            void STA()
            {
                bus->write(true, id, current_instruction.module, current_instruction.address, reg[current_instruction.dest]);

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
                cache[reg[current_instruction.dest].to_ulong()] = reg[current_instruction.src_1];

                debug_print(std::string("CPU ").append(std::to_string(id)), " STR executed");
            }

            /**
             * @brief compares two registers and sets the zero flag if they are equal
             */
            void EQL()
            {
                if (reg[current_instruction.src_1] == reg[current_instruction.src_2])
                    flag.set(FlagIndex::ZERO);

                debug_print(std::string("CPU ").append(std::to_string(id)), " EQL executed");
            }

            /**
             * @brief compares two registers and sets the sign flag if the first register is greater than the second register
             */
            void GRT()
            {
                bool less_than = false;

                for (size_t i = word_size; i-- > 0;)
                {
                    if (reg[current_instruction.src_1][i] != reg[current_instruction.src_2][i])
                    {
                        less_than = reg[current_instruction.src_1][i] < reg[current_instruction.src_2][i];
                        break;
                    }
                }

                if (less_than)
                    flag.set(FlagIndex::SIGN);
                else
                    flag.reset(FlagIndex::SIGN);

                debug_print(std::string("CPU ").append(std::to_string(id)), " GRT executed");
            }

            /**
             * @brief shifts a register left by one
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void SHL()
            {
                reg[current_instruction.src_1] <<= 1;

                if (is_bitset_zero(reg[current_instruction.src_1]))
                    flag.set(FlagIndex::ZERO);
                if (!is_bitset_positive(reg[current_instruction.src_1]))
                    flag.set(FlagIndex::SIGN);

                debug_print(std::string("CPU ").append(std::to_string(id)), " SHL executed");
            }

            /**
             * @brief shifts a register right by one
             *
             * @note the zero flag is set if the result is zero, and the sign flag is set if the result is negative
             */
            void SHR()
            {
                reg[current_instruction.src_1] >>= 1;

                if (is_bitset_zero(reg[current_instruction.src_1]))
                    flag.set(FlagIndex::ZERO);
                if (!is_bitset_positive(reg[current_instruction.src_1]))
                    flag.set(FlagIndex::SIGN);

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
                    flag.set(FlagIndex::ZERO);

                if (!is_bitset_positive(reg[current_instruction.src_1]))
                    flag.set(FlagIndex::SIGN);

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
                    flag.set(FlagIndex::ZERO);

                if (!is_bitset_positive(reg[current_instruction.src_1]))
                    flag.set(FlagIndex::SIGN);

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
                        auto read_value = bus->read(true, id, 0, cache_size * i + j);
                        bus->write(false, i, 0, j, read_value);
                    }

                    bus->cpus[i]->initialized = true;
                    bus->cpus[i]->new_instruction = true;
                    bus->cpus[i]->instruction_cycle = 0;
                    bus->cpus[i]->flag.reset(FlagIndex::HALT);
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
                {"XXX", &CPU::XXX, 2, 0, 0, 0, InstructionAccess::Internal},
                {"ADD", &CPU::ADD, 2, 0, 0, 0, InstructionAccess::CacheOnly},
                {"AND", &CPU::AND, 2, 0, 0, 0, InstructionAccess::CacheOnly},
                {"OR", &CPU::OR, 2, 0, 0, 0, InstructionAccess::CacheOnly},
                {"XOR", &CPU::XOR, 2, 0, 0, 0, InstructionAccess::CacheOnly},
                {"MOV", &CPU::MOV, 2, 0, 0, 0, InstructionAccess::CacheOnly},
                {"BUN", &CPU::BUN, 2, 0, 0, 0, InstructionAccess::Control},
                {"BIZ", &CPU::BIZ, 2, 0, 0, 0, InstructionAccess::Control},
                {"BIN", &CPU::BIN, 2, 0, 0, 0, InstructionAccess::Control},
                {"LDA", &CPU::LDA, 2, 0, 0, 0, InstructionAccess::MemoryOnly},
                {"STA", &CPU::STA, 2, 0, 0, 0, InstructionAccess::MemoryOnly},
                {"LDR", &CPU::LDR, 2, 0, 0, 0, InstructionAccess::CacheOnly},
                {"STR", &CPU::STR, 2, 0, 0, 0, InstructionAccess::CacheOnly},
                {"EQL", &CPU::EQL, 2, 0, 0, 0, InstructionAccess::CacheOnly},
                {"GRT", &CPU::GRT, 2, 0, 0, 0, InstructionAccess::CacheOnly},
                {"SHL", &CPU::SHL, 2, 0, 0, 0, InstructionAccess::CacheOnly},
                {"SHR", &CPU::SHR, 2, 0, 0, 0, InstructionAccess::CacheOnly},
                {"ROL", &CPU::ROL, 2, 0, 0, 0, InstructionAccess::CacheOnly},
                {"ROR", &CPU::ROR, 2, 0, 0, 0, InstructionAccess::CacheOnly},
                {"INT", &CPU::INT, 2, 0, 0, 0, InstructionAccess::Internal},
                {"HLT", &CPU::HLT, 2, 0, 0, 0, InstructionAccess::Control},
            };

            // current instruction
            Instruction current_instruction;
            std::bitset<word_size> current_word;

            // cpu cycles
            long long total_cpu_cycles = 0;
            char instruction_cycle = 0;

            bool new_instruction = true;
        };

        CpuIdResetter cpu_id_resetter;
        CPU cpus[cores + 1];
        Memory memory[memory_modules];

        BUS bus;
    };
}; // FIAT128