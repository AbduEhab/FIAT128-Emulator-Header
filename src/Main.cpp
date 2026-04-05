#include <FIAT128.hpp>
#include "GuiRenderer.hpp"
#include "ProgramLibrary.hpp"
#include "ProgramRepository.hpp"

#include <chrono>
#include <array>
#include <filesystem>
#include <iostream>
#include <memory>

// constexpr float itr_sqrt(float n) // Note(AbduEhab): should be removed!!!
// {
//     unsigned long i;
//     float p0, p1;

//     p0 = n;

//     i = *(unsigned long *)&p0;

//     i = 0x1FBD3F7D + (i >> 1);

//     p0 = *(float *)&i;

//     p1 = 0.5f * (p0 + n / p0);

//     return p1;
// }

#ifndef FIAT128_IMPLEMENTATION

int main([[maybe_unused]] int, [[maybe_unused]] char **)
{
    try
    {
        Instrumentor::Get().beginSession("Main func");

    TimedBlock block("Main functions");

    using EmulatorType = FIAT128::Emulator<1, 4>;

    const std::filesystem::path program_directory = std::filesystem::path("programs");
    auto program_catalog = discover_program_entries(program_directory);
    if (program_catalog.empty())
    {
        std::cerr << "No disk programs found in 'programs' directory." << std::endl;
        return 1;
    }

    size_t active_program_index = 0;

    auto create_loaded_emulator = [&](const ProgramCatalogEntry &entry)
    {
        const std::array<size_t, 4> module_sizes = {
            static_cast<size_t>(FIAT128::cache_size) * 2, // M0 ROM
            static_cast<size_t>(FIAT128::cache_size),     // M1 RAM
            512,                                           // M2 Console IO
            512                                            // M3 GPU
        };

        auto emulator = std::make_unique<EmulatorType>(module_sizes);
        load_program_entry(*emulator, entry);
        return emulator;
    };

    auto run_init_only = [&](EmulatorType &emulator)
    {
        constexpr int max_init_steps = 10000;
        emulator.set_cpu_halt_state(1, true);

        bool init_complete = false;
        for (int step = 0; step < max_init_steps; ++step)
        {
            emulator.run(true);

            // Keep CPU1 paused while CPU0 runs the bootstrap sequence.
            emulator.set_cpu_halt_state(1, true);

            const auto states = emulator.get_cpu_render_state();
            if (states[0].halted)
            {
                init_complete = true;
                break;
            }
        }

        emulator.set_cpu_halt_state(1, false);
        return init_complete;
    };

    auto Emulator = create_loaded_emulator(program_catalog[active_program_index]);
    GuiRenderer renderer(1280, 720);
    renderer.set_program_entries(program_catalog);
    renderer.set_selected_program_index(active_program_index);
    renderer.set_status_text(program_catalog[active_program_index].display_name);

    auto update_console_from_m2 = [&](EmulatorType &emulator)
    {
        auto snapshot = emulator.get_memory_snapshot();

        // M2 is module 2 (console IO)
        if (snapshot.size() <= 2)
            return;

        const auto &m2_module = snapshot[2];

        // Keep console rendering consistent with memory hex viewer char preview:
        // one character per word from the low 8 bits.
        std::string console_text;
        console_text.reserve(m2_module.size());

        for (size_t i = 0; i < m2_module.size(); ++i)
        {
            const auto &word = m2_module[i];

            uint8_t ch = 0;
            for (int bit = 0; bit < 8; ++bit)
            {
                if (word[static_cast<size_t>(bit)])
                    ch |= static_cast<uint8_t>(1U << bit);
            }

            if (ch == 10 || ch == 13)
                console_text.push_back('\n');
            else if (ch == 0)
                console_text.push_back(' ');
            else if (ch >= 32 && ch <= 126)
                console_text.push_back(static_cast<char>(ch));
            else
                console_text.push_back('.');
        }

        renderer.console_clear();
        renderer.console_append_text(console_text);
    };

    // Emulator.set_word(0, 0xFFFF00, 1);
    // Emulator.add_instruction(0, 0xFFFFFF - 1, FIAT128::InstructionType::ADD, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R1);

    auto previous_tick = std::chrono::steady_clock::now();
    double pending_steps = 0.0;

    int i = 0;
    while (renderer.is_running())
    {
        switch (renderer.consume_command())
        {
        case GuiRenderer::UiCommand::LoadProgram1:
            active_program_index = 0;
            renderer.set_selected_program_index(active_program_index);
            Emulator = create_loaded_emulator(program_catalog[active_program_index]);
            renderer.set_status_text(program_catalog[active_program_index].display_name);
            renderer.console_clear();
            break;
        case GuiRenderer::UiCommand::LoadProgram2:
            if (program_catalog.size() > 1)
            {
                active_program_index = 1;
                renderer.set_selected_program_index(active_program_index);
                Emulator = create_loaded_emulator(program_catalog[active_program_index]);
                renderer.set_status_text(program_catalog[active_program_index].display_name);
                renderer.console_clear();
            }
            break;
        case GuiRenderer::UiCommand::LoadProgram3:
            if (program_catalog.size() > 2)
            {
                active_program_index = 2;
                renderer.set_selected_program_index(active_program_index);
                Emulator = create_loaded_emulator(program_catalog[active_program_index]);
                renderer.set_status_text(program_catalog[active_program_index].display_name);
                renderer.console_clear();
            }
            break;
        case GuiRenderer::UiCommand::ReloadProgram:
            Emulator = create_loaded_emulator(program_catalog[active_program_index]);
            renderer.set_status_text(program_catalog[active_program_index].display_name);
            renderer.pause_execution();
            pending_steps = 0.0;
            renderer.console_clear();
            break;
        case GuiRenderer::UiCommand::RunInitOnly:
        {
            Emulator = create_loaded_emulator(program_catalog[active_program_index]);
            const bool init_complete = run_init_only(*Emulator);
            renderer.pause_execution();
            pending_steps = 0.0;
            if (init_complete)
                renderer.set_status_text(program_catalog[active_program_index].display_name + " [INIT READY]");
            else
                renderer.set_status_text(program_catalog[active_program_index].display_name + " [INIT TIMEOUT]");
            renderer.console_clear();
            break;
        }
        case GuiRenderer::UiCommand::LoadSelectedProgram:
        {
            const auto selected = renderer.selected_program_entry();
            if (selected)
            {
                const auto it = std::find_if(program_catalog.begin(), program_catalog.end(),
                    [&](const ProgramCatalogEntry &entry)
                    {
                        return entry.display_name == selected->display_name && entry.description == selected->description && entry.kind == selected->kind;
                    });

                if (it != program_catalog.end())
                    active_program_index = static_cast<size_t>(std::distance(program_catalog.begin(), it));

                Emulator = create_loaded_emulator(*selected);
                renderer.set_status_text(selected->display_name);
                renderer.pause_execution();
                pending_steps = 0.0;
                renderer.console_clear();
            }
            break;
        }
        case GuiRenderer::UiCommand::RefreshProgramList:
        {
            program_catalog = discover_program_entries(program_directory);
            if (active_program_index >= program_catalog.size())
                active_program_index = 0;

            renderer.set_program_entries(program_catalog);
            renderer.set_selected_program_index(active_program_index);
            Emulator = create_loaded_emulator(program_catalog[active_program_index]);
            renderer.set_status_text(program_catalog[active_program_index].display_name);
            renderer.console_clear();
            break;
        }
        case GuiRenderer::UiCommand::None:
            break;
        }

        auto now = std::chrono::steady_clock::now();
        const std::chrono::duration<double> dt = now - previous_tick;
        previous_tick = now;

        const int manual_step_requests = renderer.consume_step_requests();
        int steps_to_execute = manual_step_requests;

        if (renderer.is_running_mode())
        {
            pending_steps += dt.count() * static_cast<double>(renderer.steps_per_second());
            const int run_steps = static_cast<int>(pending_steps);
            if (run_steps > 0)
            {
                steps_to_execute += run_steps;
                pending_steps -= static_cast<double>(run_steps);
            }
        }
        else
        {
            pending_steps = 0.0;
        }

        for (int step = 0; step < steps_to_execute; ++step)
        {
            const bool trace_step = manual_step_requests > 0;

            size_t before_sp = 0;
            std::string before_next;
            if (trace_step)
            {
                const auto before_states = Emulator->get_cpu_render_state();
                before_sp = before_states[0].stack_pointer;
                before_next = before_states[0].current_instruction_detail;
            }

            Emulator->run(true);

            if (trace_step)
            {
                const auto after_states = Emulator->get_cpu_render_state();
                const auto &after_cpu0 = after_states[0];
                std::cout << "[STEP] cpu0 sp:" << before_sp
                          << " -> " << after_cpu0.stack_pointer
                          << "  instr:" << before_next
                          << "  next:" << after_cpu0.current_instruction_detail
                          << "  halted:" << (after_cpu0.halted ? "Y" : "N")
                          << '\n';
            }
        }

        update_console_from_m2(*Emulator);
        renderer.draw_frame(*Emulator);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        ++i;
    }

    std::cout << "\nRenderer closed.\n";

        return 0;
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Fatal runtime error: " << ex.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cerr << "Fatal runtime error: unknown exception" << '\n';
        return 1;
    }
}

#endif // !FIAT128_IMPLEMENTATION