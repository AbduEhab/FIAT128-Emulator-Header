#include <bitset>
#include <algorithm>
#include <array>
#include <assert.h>
#include <chrono>
#include <deque>
#include <exception>
#include <filesystem>
#include <fstream>
#include <future>
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
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#define private public
#include "FIAT128.hpp"
#undef private

#include "ProgramLibrary.hpp"

namespace
{
    using Emu = FIAT128::Emulator<2, 4, 128>;
    using TinyEmu = FIAT128::Emulator<1, 1, 128>;

    struct TestResult
    {
        std::string name;
        bool passed;
        std::string detail;
    };

    auto make_word_with_bit(size_t bit_index) -> std::bitset<128>
    {
        std::bitset<128> word;
        word.set(bit_index);
        return word;
    }

    auto test_opcode_bounds_check_off_by_one() -> TestResult
    {
        std::bitset<8> opcode(Emu::CPU::instruction_count);

        auto decoded = Emu::CPU::Instruction::decode_from_opcode(opcode);

        // Correct behavior: opcode==instruction_count should map to HLT fallback.
        const bool ok = decoded.name == "HLT";

        return {
            "opcode_bounds_check_off_by_one",
            ok,
            "Expected opcode 21 to decode as HLT fallback."
        };
    }

    auto test_int_does_not_copy_to_other_cpu_cache() -> TestResult
    {
        Emu emu(10000);

        std::bitset<128> marker;
        marker.set(127);

        // INT copies from channel 0 memory at offset cache_size * i + j.
        emu.set_word_in_memory(0, FIAT128::cache_size, marker);
        emu.cpus[0].INT();

        const bool ok = (emu.cpus[1].cache[0] == marker);

        return {
            "int_should_copy_program_to_cpu1_cache",
            ok,
            "Expected INT to copy memory[cache_size] into CPU1 cache[0]."
        };
    }

    auto test_increment_logic() -> TestResult
    {
        Emu emu(10000);
        std::bitset<8> value(0);

        emu.cpus[0].increment(value);

        const bool ok = (value.to_ulong() == 1);

        return {
            "increment_zero_should_be_one",
            ok,
            "Expected increment(0) == 1."
        };
    }

    auto test_bnz_semantics_mismatch() -> TestResult
    {
        Emu emu(10000);

        emu.cpus[0].stack_pointer.reset();
        emu.cpus[0].reg[FIAT128::R1] = std::bitset<128>(7);

        // BIN should branch when the negative/sign flag is set.
        emu.cpus[0].flag.reset(2);
        emu.cpus[0].flag.set(3);

        emu.cpus[0].current_instruction.dest = FIAT128::R1;
        emu.cpus[0].BIN();

        const bool ok = (emu.cpus[0].stack_pointer.to_ulong() == 7);

        return {
            "bin_should_branch_when_negative_flag_set",
            ok,
            "Expected BIN to branch when the negative/sign flag is set."
        };
    }

    auto test_shift_only_affects_low_8_bits() -> TestResult
    {
        Emu emu(10000);

        emu.cpus[0].reg[FIAT128::R1].reset();
        emu.cpus[0].reg[FIAT128::R1].set(32);
        emu.cpus[0].current_instruction.src_1 = FIAT128::R1;

        emu.cpus[0].SHL();

        const bool ok = emu.cpus[0].reg[FIAT128::R1].test(33);

        return {
            "shl_should_shift_full_register_width",
            ok,
            "Expected bit 32 to move to bit 33 after SHL."
        };
    }

    auto test_sign_bit_detection() -> TestResult
    {
        Emu emu(10000);

        std::bitset<8> v;
        v.set(7);

        const bool positive = emu.cpus[0].is_bitset_positive(v);
        const bool ok = !positive;

        return {
            "sign_detection_should_use_msb",
            ok,
            "Expected value with MSB set to be treated as negative."
        };
    }

    auto test_str_truncates_or_throws_on_wide_values() -> TestResult
    {
        Emu emu(10000);

        emu.cpus[0].reg[FIAT128::R1].reset();
        emu.cpus[0].reg[FIAT128::R1].set(100);
        emu.cpus[0].reg[FIAT128::R2] = std::bitset<128>(0);

        emu.cpus[0].current_instruction.dest = FIAT128::R2;
        emu.cpus[0].current_instruction.src_1 = FIAT128::R1;

        bool threw = false;
        try
        {
            emu.cpus[0].STR();
        }
        catch (const std::exception &)
        {
            threw = true;
        }

        const bool preserved = (emu.cpus[0].cache[0] == emu.cpus[0].reg[FIAT128::R1]);
        const bool ok = !threw && preserved;

        return {
            "str_should_store_full_128_bit_word",
            ok,
            "Expected STR to store full value without to_ulong overflow/truncation."
        };
    }

    auto test_grt_uses_narrow_to_ulong_compare() -> TestResult
    {
        Emu emu(10000);

        // R1 has only high bit set; R2 has low bit set.
        emu.cpus[0].reg[FIAT128::R1].reset();
        emu.cpus[0].reg[FIAT128::R2].reset();
        emu.cpus[0].reg[FIAT128::R1].set(100);
        emu.cpus[0].reg[FIAT128::R2].set(0);

        emu.cpus[0].current_instruction.src_1 = FIAT128::R1;
        emu.cpus[0].current_instruction.src_2 = FIAT128::R2;

        bool threw = false;
        try
        {
            emu.cpus[0].GRT();
        }
        catch (const std::exception &)
        {
            threw = true;
        }

        // Desired behavior: R1 > R2, so sign flag should remain clear.
        const bool sign_flag_set = emu.cpus[0].flag.test(3);
        const bool ok = !threw && !sign_flag_set;

        return {
            "grt_should_compare_full_128_bit_values",
            ok,
            "Expected GRT to compare full-width values without to_ulong overflow."
        };
    }

    auto test_bus_read_channel_bound_check() -> TestResult
    {
        Emu emu(10000);

        std::bitset<128> marker;
        marker.set(11);
        emu.set_word_in_memory(0, 0, marker);

        const auto in_range = emu.bus.read(true, 0, 0, 0);
        const auto out_of_range = emu.bus.read(true, 0, emu.bus.channels, 0);
        const bool ok = (in_range == marker) && out_of_range.none();

        return {
            "bus_read_should_reject_channel_equal_to_channels",
            ok,
            "Expected in-range reads to work and channel == channels to return zero safely."
        };
    }

    auto test_random_distribution_range_sticky() -> TestResult
    {
        bool observed_out_of_second_range = false;

        // First call initializes thread_local distribution [0.0, 1.0].
        (void)FIAT128::random<double>(0.0, 1.0);

        // Subsequent calls with different range should honor [10, 20], but currently do not.
        for (int i = 0; i < 50; ++i)
        {
            const double value = FIAT128::random<double>(10.0, 20.0);
            if (value < 10.0 || value > 20.0)
            {
                observed_out_of_second_range = true;
                break;
            }
        }

        const bool ok = !observed_out_of_second_range;

        return {
            "random_should_respect_new_min_max_each_call",
            ok,
            "Expected random(min,max) to use the provided range every call."
        };
    }

    auto test_tiny_five_instruction_program_runs_without_xxx() -> TestResult
    {
        TinyEmu emu(64);

        ProgramDefinition program;
        program.id = ProgramId::IdleLoop;
        program.name = "Tiny Five Instruction Program";
        program.description = "Five instruction regression for load and execution.";
        program.instructions = {
            {4, FIAT128::InstructionType::MOV, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0},
            {3, FIAT128::InstructionType::ADD, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0},
            {2, FIAT128::InstructionType::AND, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0},
            {1, FIAT128::InstructionType::XOR, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0},
            {0, FIAT128::InstructionType::HLT, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0},
        };

        load_program(emu, program);

        bool saw_xxx = false;
        bool halted = false;
        std::ostringstream trace;

        for (int step = 0; step < 20; ++step)
        {
            emu.run(false);

            const auto state = emu.get_cpu_render_state();
            trace << "[" << step << "] " << state[0].current_instruction << " sp=" << state[0].stack_pointer << " halted=" << (state[0].halted ? "Y" : "N") << '\n';

            if (state[0].halted)
            {
                halted = true;
                break;
            }

            if (state[0].current_instruction == "XXX")
            {
                saw_xxx = true;
                break;
            }
        }

        const bool ok = halted && !saw_xxx;

        return {
            "tiny_five_instruction_program_runs_without_xxx",
            ok,
            trace.str()
        };
    }

    auto test_add_does_not_touch_module_memory() -> TestResult
    {
        Emu emu(10000);

        std::bitset<128> marker;
        marker.set(77);
        emu.set_word_in_memory(2, 11, marker);

        emu.cpus[0].reg[FIAT128::R1] = std::bitset<128>(5);
        emu.cpus[0].reg[FIAT128::R2] = std::bitset<128>(9);
        emu.cpus[0].current_instruction.dest = FIAT128::R3;
        emu.cpus[0].current_instruction.src_1 = FIAT128::R1;
        emu.cpus[0].current_instruction.src_2 = FIAT128::R2;

        const auto before = emu.get_memory_snapshot();
        emu.cpus[0].ADD();
        const auto after = emu.get_memory_snapshot();

        const bool ok = (before == after) && (emu.cpus[0].reg[FIAT128::R3].to_ulong() == 14);

        return {
            "add_should_remain_cache_only",
            ok,
            "Expected ADD to leave module memory unchanged and only update the destination cache/register."
        };
    }

    auto test_memory_instruction_uses_module_and_address() -> TestResult
    {
        Emu emu(10000);

        std::bitset<128> load_marker;
        load_marker.set(91);
        std::bitset<128> store_marker;
        store_marker.set(103);

        emu.set_word_in_memory(3, 42, load_marker);
        emu.set_memory_instruction_in_cpu(0, 0, FIAT128::InstructionType::LDA, FIAT128::R1, 3, 42);

        std::bitset<32> lda_word = emu.cpus[0].to_xbits<32>(emu.cpus[0].cache[0]);
        auto lda_decoded = emu.cpus[0].decode_instruction(lda_word);

        emu.cpus[0].current_instruction = lda_decoded;
        emu.cpus[0].LDA();

        emu.cpus[0].reg[FIAT128::R2] = store_marker;
        emu.set_memory_instruction_in_cpu(0, 1, FIAT128::InstructionType::STA, FIAT128::R2, 1, 7);

        std::bitset<32> sta_word = emu.cpus[0].to_xbits<32>(emu.cpus[0].cache[1]);
        auto sta_decoded = emu.cpus[0].decode_instruction(sta_word);

        emu.cpus[0].current_instruction = sta_decoded;
        emu.cpus[0].STA();

        const auto stored = emu.bus.read(true, 0, 1, 7);
         std::ostringstream detail;
         detail << "lda_word=0x" << std::hex << lda_word.to_ulong()
             << " sta_word=0x" << std::hex << sta_word.to_ulong() << std::dec;
         detail << "LDA name=" << lda_decoded.name
             << " access=" << static_cast<int>(lda_decoded.access)
             << " dest=" << static_cast<int>(lda_decoded.dest)
             << " module=" << static_cast<int>(lda_decoded.module)
             << " address=" << lda_decoded.address
             << " loaded=" << emu.cpus[0].reg[FIAT128::R1]
             << " STA name=" << sta_decoded.name
             << " access=" << static_cast<int>(sta_decoded.access)
             << " dest2=" << static_cast<int>(sta_decoded.dest)
             << " module2=" << static_cast<int>(sta_decoded.module)
             << " address2=" << sta_decoded.address
             << " stored=" << stored;

         const bool ok =
            lda_decoded.access == FIAT128::InstructionAccess::MemoryOnly &&
            lda_decoded.dest == FIAT128::R1 &&
            lda_decoded.module == 3 &&
            lda_decoded.address == 42 &&
            emu.cpus[0].reg[FIAT128::R1] == load_marker &&
            sta_decoded.access == FIAT128::InstructionAccess::MemoryOnly &&
            sta_decoded.dest == FIAT128::R2 &&
            sta_decoded.module == 1 &&
            sta_decoded.address == 7 &&
            stored == store_marker;

        return {
            "memory_instructions_should_encode_module_and_address",
            ok,
            detail.str()
        };
    }
}

int main()
{
    std::vector<TestResult> results;
    results.push_back(test_opcode_bounds_check_off_by_one());
    results.push_back(test_int_does_not_copy_to_other_cpu_cache());
    results.push_back(test_increment_logic());
    results.push_back(test_bnz_semantics_mismatch());
    results.push_back(test_shift_only_affects_low_8_bits());
    results.push_back(test_sign_bit_detection());
    results.push_back(test_str_truncates_or_throws_on_wide_values());
    results.push_back(test_grt_uses_narrow_to_ulong_compare());
    results.push_back(test_bus_read_channel_bound_check());
    results.push_back(test_random_distribution_range_sticky());
    results.push_back(test_tiny_five_instruction_program_runs_without_xxx());
    results.push_back(test_add_does_not_touch_module_memory());
    results.push_back(test_memory_instruction_uses_module_and_address());

    int failures = 0;
    for (const auto &r : results)
    {
        if (r.passed)
        {
            std::cout << "[PASS] " << r.name << '\n';
        }
        else
        {
            ++failures;
            std::cout << "[FAIL] " << r.name << " :: " << r.detail << '\n';
        }
    }

    std::cout << "Total: " << results.size() << ", Failures: " << failures << '\n';

    return failures == 0 ? 0 : 1;
}
