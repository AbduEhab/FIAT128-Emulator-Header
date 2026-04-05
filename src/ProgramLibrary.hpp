#pragma once

#include <FIAT128.hpp>

#include <array>
#include <bitset>
#include <string>
#include <vector>

enum class ProgramId
{
    IdleLoop = 0,
    HaltDemo = 1,
    BranchNegativeDemo = 2,
};

struct ProgramWord
{
    size_t index = 0;
    std::bitset<128> value;
};

struct ProgramInstruction
{
    size_t index = 0;
    FIAT128::InstructionType type = FIAT128::InstructionType::XXX;
    FIAT128::RegisterIndex dest = FIAT128::RegisterIndex::R0;
    FIAT128::RegisterIndex src_1 = FIAT128::RegisterIndex::R0;
    FIAT128::RegisterIndex src_2 = FIAT128::RegisterIndex::R0;
    bool memory_access = false;
    unsigned char module = 0;
    unsigned short address = 0;
};

struct ProgramDefinition
{
    ProgramId id = ProgramId::IdleLoop;
    std::string name;
    std::string description;
    std::vector<ProgramWord> words;
    std::vector<ProgramInstruction> instructions;
};

inline auto program_name(ProgramId id) -> std::string
{
    switch (id)
    {
    case ProgramId::IdleLoop:
        return "Idle Loop";
    case ProgramId::HaltDemo:
        return "Halt Demo";
    case ProgramId::BranchNegativeDemo:
        return "Branch Negative Demo";
    }

    return "Unknown";
}

inline auto make_program(ProgramId id) -> ProgramDefinition
{
    switch (id)
    {
    case ProgramId::IdleLoop:
        return {
            ProgramId::IdleLoop,
            "Idle Loop",
            "Simple MOV/BUN loop for stepping and renderer testing.",
            {
                {0, std::bitset<128>(0)},
                {1, std::bitset<128>(1)},
            },
            {
                {7, FIAT128::InstructionType::MOV, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0},
                {6, FIAT128::InstructionType::BUN, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0},
            }
        };

    case ProgramId::HaltDemo:
        return {
            ProgramId::HaltDemo,
            "Halt Demo",
            "Runs two instructions then halts.",
            {
                {0, std::bitset<128>(0)},
                {1, std::bitset<128>(1)},
                {2, std::bitset<128>(2)},
            },
            {
                {7, FIAT128::InstructionType::MOV, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0},
                {6, FIAT128::InstructionType::ADD, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0},
                {5, FIAT128::InstructionType::HLT, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0},
            }
        };

    case ProgramId::BranchNegativeDemo:
        return {
            ProgramId::BranchNegativeDemo,
            "Branch Negative Demo",
            "Exercises BIN/BUN control flow for renderer visualization.",
            {
                {0, std::bitset<128>(0)},
                {1, std::bitset<128>(3)},
                {2, std::bitset<128>(7)},
            },
            {
                {7, FIAT128::InstructionType::BNZ, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0},
                {6, FIAT128::InstructionType::BUN, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0},
                {5, FIAT128::InstructionType::MOV, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0},
            }
        };
    }

    return {};
}

template <size_t cores, size_t memory_modules, size_t word_size>
inline auto load_program(FIAT128::Emulator<cores, memory_modules, word_size> &emulator, const ProgramDefinition &program, size_t channel = 0) -> void
{
    (void)channel;
    constexpr bool use_master_slave_boot = (cores >= 1 && memory_modules >= 3);

    size_t entry_point = 0;
    bool has_cpu_instruction = false;

    for (size_t module = 0; module < memory_modules; ++module)
    {
        for (size_t i = 0; i < static_cast<size_t>(FIAT128::cache_size); ++i)
            emulator.set_word_in_memory(module, i, std::bitset<word_size>(0));
    }

    for (size_t i = 0; i < static_cast<size_t>(FIAT128::cache_size); ++i)
    {
        for (size_t cpu = 0; cpu <= cores; ++cpu)
            emulator.set_word_in_cpu(static_cast<char>(cpu), static_cast<short>(i), std::bitset<word_size>(0));
    }

    const size_t slave_cpu_id = use_master_slave_boot ? 1 : 0;
    const size_t slave_rom_base = static_cast<size_t>(FIAT128::cache_size) * slave_cpu_id;

    for (const auto &word : program.words)
    {
        emulator.set_word_in_memory(0, word.index, std::bitset<word_size>(word.value.to_string()));

        if constexpr (use_master_slave_boot)
        {
            if (word.index < static_cast<size_t>(FIAT128::cache_size))
                emulator.set_word_in_memory(0, slave_rom_base + word.index, std::bitset<word_size>(word.value.to_string()));
        }
        else if (word.index < static_cast<size_t>(FIAT128::cache_size))
        {
            emulator.set_word_in_cpu(0, static_cast<short>(word.index), word.value);
        }
    }

    for (const auto &instruction : program.instructions)
    {
        std::bitset<word_size> encoded_instruction(0);

        if (instruction.memory_access)
        {
            const auto packed_operand = static_cast<unsigned char>((static_cast<unsigned char>(instruction.dest) << 4) | (instruction.module & 0x0F));
            encoded_instruction = std::bitset<word_size>(
                static_cast<uint32_t>(instruction.type) << 24 |
                static_cast<uint32_t>(packed_operand) << 16 |
                static_cast<uint32_t>(instruction.address));
            encoded_instruction <<= (word_size - 32);

            emulator.set_memory_instruction_in_memory(0,
                                                      instruction.index,
                                                      instruction.type,
                                                      instruction.dest,
                                                      instruction.module,
                                                      instruction.address);

            if constexpr (cores == 0)
            {
                if (instruction.index < static_cast<size_t>(FIAT128::cache_size))
                {
                    emulator.set_memory_instruction_in_cpu(0,
                                                           static_cast<short>(instruction.index),
                                                           instruction.type,
                                                           instruction.dest,
                                                           instruction.module,
                                                           instruction.address);
                    entry_point = std::max(entry_point, instruction.index);
                    has_cpu_instruction = true;
                }
            }
        }
        else
        {
            encoded_instruction = std::bitset<word_size>(
                static_cast<uint32_t>(instruction.type) << 24 |
                static_cast<uint32_t>(instruction.dest) << 16 |
                static_cast<uint32_t>(instruction.src_1) << 8 |
                static_cast<uint32_t>(instruction.src_2));
            encoded_instruction <<= (word_size - 32);

            emulator.set_instruction_in_memory(0,
                                               instruction.index,
                                               instruction.type,
                                               instruction.dest,
                                               instruction.src_1,
                                               instruction.src_2);

            if constexpr (cores == 0)
            {
                if (instruction.index < static_cast<size_t>(FIAT128::cache_size))
                {
                    emulator.set_instruction_in_cpu(0,
                                                    static_cast<short>(instruction.index),
                                                    instruction.type,
                                                    instruction.dest,
                                                    instruction.src_1,
                                                    instruction.src_2);
                    entry_point = std::max(entry_point, instruction.index);
                    has_cpu_instruction = true;
                }
            }
        }

        if constexpr (use_master_slave_boot)
        {
            if (instruction.index < static_cast<size_t>(FIAT128::cache_size))
            {
                emulator.set_word_in_memory(0, slave_rom_base + instruction.index, encoded_instruction);
                entry_point = std::max(entry_point, instruction.index);
                has_cpu_instruction = true;
            }
        }
    }

    if (!has_cpu_instruction)
        entry_point = 0;

    if constexpr (use_master_slave_boot)
    {
        emulator.set_cpu_entry_point(1, entry_point);
        emulator.set_cpu_halt_state(1, true);

        emulator.set_instruction_in_cpu(0, 1, FIAT128::InstructionType::INT, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0);
        emulator.set_instruction_in_cpu(0, 0, FIAT128::InstructionType::HLT, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0);
        emulator.set_cpu_entry_point(0, 1);
    }
    else
    {
        emulator.set_cpu_entry_point(0, entry_point);
    }
}
