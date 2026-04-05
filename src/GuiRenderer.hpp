#pragma once

#include <SDL2/SDL.h>

#include "ProgramRepository.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <deque>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <string>
#include <unordered_map>
#include <vector>

class GuiRenderer
{
public:
    enum class UiCommand
    {
        None,
        LoadProgram1,
        LoadProgram2,
        LoadProgram3,
        ReloadProgram,
        RunInitOnly,
        LoadSelectedProgram,
        RefreshProgramList,
    };

    enum class ScrollTarget
    {
        None,
        Dropdown,
        Inspector,
    };

    enum class MemoryViewMode
    {
        Writes,
        Hex,
    };

    enum class NumberDisplayMode
    {
        Hex,
        Decimal,
    };

    GuiRenderer(int width = 1280, int height = 720)
        : window_width(width), window_height(height)
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
            throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());

        window = SDL_CreateWindow("FIAT128 Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  window_width, window_height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        if (!window)
            throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer)
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

        if (!renderer)
            throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());

        animation_start_ticks = SDL_GetTicks64();
    }

    ~GuiRenderer()
    {
        if (renderer)
            SDL_DestroyRenderer(renderer);
        if (window)
            SDL_DestroyWindow(window);
        SDL_Quit();
    }

    auto is_running() const -> bool
    {
        return running;
    }

    auto consume_command() -> UiCommand
    {
        UiCommand command = pending_command;
        pending_command = UiCommand::None;
        return command;
    }

    auto set_status_text(const std::string &status) -> void
    {
        status_text = status;
    }

    auto set_program_entries(std::vector<ProgramCatalogEntry> entries) -> void
    {
        program_entries = std::move(entries);
        if (program_entries.empty())
        {
            selected_program_index = 0;
            dropdown_scroll_offset = 0;
            return;
        }

        if (selected_program_index >= program_entries.size())
            selected_program_index = 0;

        clamp_menu_scroll();
        clamp_inspector_scroll();
    }

    auto selected_program_entry() const -> std::optional<ProgramCatalogEntry>
    {
        if (selected_program_index >= program_entries.size())
            return std::nullopt;

        return program_entries[selected_program_index];
    }

    auto selected_program_definition() const -> std::optional<ProgramDefinition>
    {
        const auto entry = selected_program_entry();
        if (!entry)
            return std::nullopt;

        if (entry->cached_definition)
            return entry->cached_definition;

        if (entry->kind == ProgramSourceKind::BuiltIn)
            return make_program(entry->built_in_id);

        return parse_program_file(entry->disk_path);
    }

    auto selected_program_index_value() const -> size_t
    {
        return selected_program_index;
    }

    auto set_selected_program_index(size_t index) -> void
    {
        if (index < program_entries.size())
            selected_program_index = index;

        clamp_menu_scroll();
        clamp_inspector_scroll();
    }

    auto is_running_mode() const -> bool
    {
        return run_continuous;
    }

    auto steps_per_second() const -> int
    {
        return sim_steps_per_second;
    }

    auto consume_step_requests() -> int
    {
        int requests = pending_step_requests;
        pending_step_requests = 0;
        return requests;
    }

    auto pause_execution() -> void
    {
        run_continuous = false;
    }

    template <typename EmulatorT>
    auto draw_frame(EmulatorT &emulator) -> void
    {
        poll_events();
        if (!running)
            return;

        auto cpu_states = emulator.get_cpu_render_state();
        auto new_events = emulator.get_memory_write_events_since(last_seen_memory_sequence);

        if (!new_events.empty())
        {
            changed_memory_positions_this_frame.clear();
            last_seen_memory_sequence = new_events.back().sequence;
            for (const auto &event : new_events)
            {
                memory_lines.push_back(format_memory_line(event));
                changed_memory_positions_this_frame.insert(memory_position_key(event.channel, event.index));
                if (memory_lines.size() > 18)
                    memory_lines.pop_front();
            }
        }

        std::vector<std::string> mem_lines = build_memory_lines(emulator);

        auto snapshot = emulator.get_memory_snapshot();
        active_module_count = std::max<size_t>(1, snapshot.size());

        if (!snapshot.empty())
            selected_visual_module = std::min(selected_visual_module, snapshot.size() - 1);
        if (!snapshot.empty())
            selected_memory_module = std::min(selected_memory_module, snapshot.size() - 1);

        std::vector<std::string> video_lines;
        if (!snapshot.empty())
        {
            const auto &active_module = snapshot[selected_visual_module];
            size_t non_zero = 0;
            for (const auto &word : active_module)
            {
                if (word.any())
                    ++non_zero;
            }

            std::ostringstream l0;
            l0 << "VIEWING M" << selected_visual_module << " - " << module_name(selected_visual_module);
            video_lines.push_back(l0.str());

            std::ostringstream l1;
            l1 << "WORDS: " << active_module.size() << "  NONZERO: " << non_zero;
            video_lines.push_back(l1.str());

            if (selected_visual_module == 2 && !active_module.empty())
            {
                std::ostringstream l2;
                l2 << "CONSOLE VALUE[0]: " << bitset_to_i128_string(active_module[0]);
                video_lines.push_back(l2.str());
            }
        }
        else
        {
            video_lines.push_back("NO MODULE DATA");
        }

        SDL_GetWindowSize(window, &window_width, &window_height);

        SDL_SetRenderDrawColor(renderer, 16, 20, 30, 255);
        SDL_RenderClear(renderer);

        const int padding = 16;
        const int top_height = (window_height / 3);
        const int bottom_width = window_width - (padding * 4);
        const int panel_width = std::max(120, bottom_width / 3);
        const int remainder = bottom_width - (panel_width * 3);
        const int bottom_height = window_height - top_height - (padding * 3);

        const SDL_Rect instructions_rect{padding, padding, window_width - (padding * 2), top_height};
        const SDL_Rect menu_rect{padding, top_height + (padding * 2), panel_width + (remainder > 0 ? remainder : 0), bottom_height};
        const SDL_Rect memory_rect{menu_rect.x + menu_rect.w + padding, top_height + (padding * 2), panel_width, bottom_height};
        const SDL_Rect video_rect{memory_rect.x + memory_rect.w + padding, top_height + (padding * 2), window_width - (memory_rect.x + memory_rect.w + (padding * 2)), bottom_height};

        memory_panel_rect = memory_rect;

        draw_instruction_panel(instructions_rect, cpu_states);
        draw_program_selector_panel(menu_rect);
        draw_memory_panel(memory_rect, mem_lines);
        draw_video_panel(video_rect, snapshot, video_lines);

        SDL_RenderPresent(renderer);
        ++frame_counter;
    }

private:
    template <typename MemoryEvent>
    auto format_memory_line(const MemoryEvent &event) -> std::string
    {
           const std::string int128_text = bitset_to_i128_string(event.value);

        std::ostringstream line;
        line << std::setw(4) << event.sequence
             << "  " << std::setw(3) << event.cpu_id
             << "  " << std::setw(2) << event.channel
             << "  " << std::setw(4) << event.index
               << "  " << int128_text;
        return line.str();
    }

    template <typename EmulatorT>
    auto build_memory_lines(EmulatorT &emulator) -> std::vector<std::string>
    {
        if (memory_view_mode == MemoryViewMode::Writes)
        {
            std::vector<std::string> mem_lines;
            mem_lines.reserve(memory_lines.size() + 1);
            mem_lines.push_back("SEQ   CPU CH   IDX   INT128");
            for (auto it = memory_lines.rbegin(); it != memory_lines.rend(); ++it)
                mem_lines.push_back(*it);

            if (mem_lines.size() == 1)
                mem_lines.push_back("WAITING FOR MEMORY WRITES");

            return mem_lines;
        }

        auto snapshot = emulator.get_memory_snapshot();
        std::vector<std::string> mem_lines;
        mem_lines.push_back(number_display_mode == NumberDisplayMode::Hex ? "IDX  WORD                                                 CH" : "IDX  WORD    CH");

        if (snapshot.empty())
        {
            mem_lines.push_back("NO MEMORY SNAPSHOT AVAILABLE");
            return mem_lines;
        }

        selected_memory_module = std::min(selected_memory_module, snapshot.size() - 1);

        const auto &module_words = snapshot[selected_memory_module];
        const int total_lines = static_cast<int>(module_words.size()) + 1;
        const int max_scroll = std::max(0, total_lines - 1);
        memory_hex_scroll_offset = std::clamp(memory_hex_scroll_offset, 0, max_scroll);

        std::vector<std::string> all_lines;
        all_lines.reserve(static_cast<size_t>(total_lines));

        {
            std::ostringstream header;
            header << "[MODULE " << selected_memory_module << " - " << module_name(selected_memory_module) << "]";
            all_lines.push_back(header.str());
        }

        for (size_t index = 0; index < module_words.size(); ++index)
        {
            std::ostringstream line;
            line << std::setw(4) << index << "  " << format_word_value(module_words[index], true)
                 << "    " << printable_char_preview(module_words[index]);
            all_lines.push_back(line.str());
        }

        const int visible_lines = 18;
        const int start_index = std::clamp(memory_hex_scroll_offset, 0, std::max(0, static_cast<int>(all_lines.size()) - visible_lines));

        for (int i = start_index; i < static_cast<int>(all_lines.size()) && (i - start_index) < visible_lines; ++i)
            mem_lines.push_back(all_lines[static_cast<size_t>(i)]);

        if (mem_lines.size() == 1)
            mem_lines.push_back("NO MEMORY SNAPSHOT AVAILABLE");

        return mem_lines;
    }

    template <size_t word_size>
    auto preview_word(const std::bitset<word_size> &value) const -> std::string
    {
        if (number_display_mode == NumberDisplayMode::Decimal)
        {
            std::bitset<16> low16;
            for (size_t i = 0; i < 16 && i < word_size; ++i)
                low16[i] = value[i];

            return bitset_to_unsigned_decimal(low16);
        }

        const std::string bits = value.to_string();
        const std::string low16 = bits.substr(bits.size() - 16);

        const unsigned long low = std::stoul(low16, nullptr, 2);

        std::ostringstream out;
        out << "0x"
            << std::hex << std::setw(4) << std::setfill('0') << low
            << std::dec << std::setfill(' ');
        return out.str();
    }

    static auto flags_to_string(const std::bitset<8> &flags) -> std::string
    {
        std::vector<std::string> parts;

        if (flags.test(0))
            parts.push_back("INT");
        if (flags.test(1))
            parts.push_back("OVF");
        if (flags.test(2))
            parts.push_back("ZERO");
        if (flags.test(3))
            parts.push_back("SIGN");
        if (flags.test(4))
            parts.push_back("HLT");

        if (parts.empty())
            return "NONE";

        std::ostringstream out;
        for (size_t i = 0; i < parts.size(); ++i)
        {
            if (i > 0)
                out << '|';
            out << parts[i];
        }

        return out.str();
    }

    auto poll_events() -> void
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = false;

            if (event.type == SDL_MOUSEWHEEL)
            {
                if (dropdown_open)
                {
                    dropdown_scroll_offset -= event.wheel.y;
                    clamp_menu_scroll();
                }
                else if (inspector_visible && point_in_rect(last_mouse_x, last_mouse_y, program_inspector_rect))
                {
                    program_preview_scroll_offset -= event.wheel.y;
                    clamp_inspector_scroll();
                }
                else if (point_in_rect(last_mouse_x, last_mouse_y, memory_panel_rect))
                {
                    memory_hex_scroll_offset -= event.wheel.y;
                }
            }

            if (event.type == SDL_MOUSEMOTION)
            {
                last_mouse_x = event.motion.x;
                last_mouse_y = event.motion.y;
            }

            if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_1:
                    pending_command = UiCommand::LoadProgram1;
                    break;
                case SDLK_2:
                    pending_command = UiCommand::LoadProgram2;
                    break;
                case SDLK_3:
                    pending_command = UiCommand::LoadProgram3;
                    break;
                case SDLK_r:
                    pending_command = UiCommand::ReloadProgram;
                    break;
                case SDLK_i:
                    pending_command = UiCommand::RunInitOnly;
                    break;
                case SDLK_l:
                    pending_command = UiCommand::LoadSelectedProgram;
                    break;
                case SDLK_f:
                    pending_command = UiCommand::ReloadProgram;
                    break;
                case SDLK_SPACE:
                    run_continuous = !run_continuous;
                    break;
                case SDLK_n:
                    ++pending_step_requests;
                    break;
                case SDLK_MINUS:
                    sim_steps_per_second = std::max(min_steps_per_second, sim_steps_per_second - 5);
                    break;
                case SDLK_EQUALS:
                case SDLK_PLUS:
                    sim_steps_per_second = std::min(max_steps_per_second, sim_steps_per_second + 5);
                    break;
                default:
                    break;
                }
            }

            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
                handle_mouse_click(event.button.x, event.button.y);
        }
    }

    auto draw_program_selector_panel(const SDL_Rect &rect) -> void
    {
        draw_panel_background(rect);
        draw_text(rect.x + 12, rect.y + 10, "PROGRAM SELECTOR", 2, {242, 246, 255, 255});

        button_inspector_toggle = {rect.x + rect.w - 170, rect.y + 8, 158, 22};
        draw_button(button_inspector_toggle, inspector_visible ? "HIDE INSPECTOR" : "SHOW INSPECTOR", {255, 200, 140, 255});

        const SDL_Rect header_rect{rect.x + 10, rect.y + 36, rect.w - 20, 30};

        const int inspector_top = header_rect.y + header_rect.h + 10;
        const int inspector_height = rect.y + rect.h - inspector_top - 42;
        program_inspector_rect = {rect.x + 10, inspector_top, rect.w - 20, std::max(80, inspector_height)};

        if (inspector_visible)
            draw_program_inspector(program_inspector_rect);

        draw_program_dropdown(header_rect);

        const SDL_Rect footer_rect{rect.x + 10, rect.y + rect.h - 32, rect.w - 20, 20};
        draw_text(footer_rect.x, footer_rect.y, clamp_text_to_width(selected_program_footer(), footer_rect.w, 1), 1, {190, 212, 240, 255});

        SDL_SetRenderDrawColor(renderer, 120, 132, 160, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }

    auto draw_program_dropdown(const SDL_Rect &rect) -> void
    {
        SDL_SetRenderDrawColor(renderer, 40, 50, 68, 255);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 120, 132, 160, 255);
        SDL_RenderDrawRect(renderer, &rect);

        const int button_width = 74;
        const int gap = 6;
        const int dropdown_width = std::max(120, rect.w - (button_width * 2) - (gap * 3));

        button_dropdown = {rect.x, rect.y, dropdown_width, rect.h};
        button_menu_load = {rect.x + dropdown_width + gap, rect.y, button_width, rect.h};
        button_menu_refresh = {rect.x + dropdown_width + gap + button_width + gap, rect.y, button_width, rect.h};

        draw_button(button_dropdown, selected_program_index < program_entries.size() ? program_entries[selected_program_index].display_name : std::string("No program selected"), {242, 246, 255, 255});
        draw_button(button_menu_load, "LOAD", {130, 168, 255, 255});
        draw_button(button_menu_refresh, "RELOAD", {120, 220, 150, 255});

        draw_text(button_dropdown.x + button_dropdown.w - 12, button_dropdown.y + 8, dropdown_open ? "-" : "+", 1, {180, 210, 255, 255});

        if (!dropdown_open)
            return;

        const int row_height = 28;
        const int visible_rows = std::min(7, static_cast<int>(program_entries.size()));
        const int dropdown_height = std::max(0, visible_rows * row_height);
        const SDL_Rect dropdown_rect{button_dropdown.x, rect.y + rect.h + 4, button_dropdown.w, dropdown_height};

        SDL_SetRenderDrawColor(renderer, 32, 40, 58, 245);
        SDL_RenderFillRect(renderer, &dropdown_rect);
        SDL_SetRenderDrawColor(renderer, 120, 132, 160, 255);
        SDL_RenderDrawRect(renderer, &dropdown_rect);

        const int max_scroll = std::max(0, static_cast<int>(program_entries.size()) - visible_rows);
        dropdown_scroll_offset = std::clamp(dropdown_scroll_offset, 0, max_scroll);
        dropdown_entry_rects.clear();

        for (int row = 0; row < visible_rows; ++row)
        {
            const size_t entry_index = static_cast<size_t>(dropdown_scroll_offset + row);
            if (entry_index >= program_entries.size())
                break;

            const SDL_Rect entry_rect{dropdown_rect.x + 2, dropdown_rect.y + (row * row_height), dropdown_rect.w - 4, row_height - 2};
            dropdown_entry_rects.push_back(entry_rect);
            draw_program_entry(entry_rect, program_entries[entry_index], entry_index == selected_program_index);
        }
    }

    auto draw_program_inspector(const SDL_Rect &rect) -> void
    {
        draw_panel_background(rect);
        draw_text(rect.x + 10, rect.y + 8, "PROGRAM INSPECTOR", 2, {242, 246, 255, 255});

        const auto definition = selected_program_definition();
        if (!definition)
        {
            draw_text(rect.x + 10, rect.y + 34, "No program loaded.", 1, {192, 210, 240, 255});
            return;
        }

        std::vector<std::string> lines = build_program_lines(*definition);
        const int line_height = 12;
        const int max_visible_lines = std::max(1, (rect.h - 28) / line_height);
        clamp_inspector_scroll(static_cast<int>(lines.size()), max_visible_lines);

        const int max_scroll = std::max(0, static_cast<int>(lines.size()) - max_visible_lines);
        const int start_index = std::clamp(program_preview_scroll_offset, 0, max_scroll);

        button_inspector_up = {rect.x + rect.w - 64, rect.y + 6, 26, 20};
        button_inspector_down = {rect.x + rect.w - 34, rect.y + 6, 26, 20};
        draw_button(button_inspector_up, "^", {220, 220, 220, 255});
        draw_button(button_inspector_down, "v", {220, 220, 220, 255});

        int y = rect.y + 34;
        for (int i = start_index; i < static_cast<int>(lines.size()) && (i - start_index) < max_visible_lines; ++i)
        {
            draw_text(rect.x + 8, y, lines[static_cast<size_t>(i)], 1, {214, 224, 240, 255});
            y += line_height;
        }

        SDL_SetRenderDrawColor(renderer, 120, 132, 160, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }

    auto draw_program_entry(const SDL_Rect &rect, const ProgramCatalogEntry &entry, bool selected) -> void
    {
        const SDL_Color background = selected ? SDL_Color{70, 92, 132, 255} : SDL_Color{34, 42, 60, 255};
        const SDL_Color border = selected ? SDL_Color{144, 190, 255, 255} : SDL_Color{90, 100, 120, 255};
        SDL_SetRenderDrawColor(renderer, background.r, background.g, background.b, background.a);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
        SDL_RenderDrawRect(renderer, &rect);

        int text_width_px = std::max(0, rect.w - 16);
        std::string file_label;
        int file_label_width = 0;
        if (entry.kind == ProgramSourceKind::Disk)
        {
            file_label = clamp_text_to_width(entry.disk_path.filename().string(), 120, 1);
            file_label_width = static_cast<int>(file_label.size()) * 6;
            text_width_px = std::max(0, text_width_px - file_label_width - 8);
        }

        const int max_name_chars = std::max(1, text_width_px / 6);
        const auto wrapped_name = wrap_text(entry.display_name, static_cast<size_t>(max_name_chars));

        if (!wrapped_name.empty())
            draw_text(rect.x + 8, rect.y + 4, clamp_text_to_width(wrapped_name[0], text_width_px, 1), 1, {242, 246, 255, 255});

        if (wrapped_name.size() > 1)
        {
            draw_text(rect.x + 8, rect.y + 16, clamp_text_to_width(wrapped_name[1], text_width_px, 1), 1, {210, 224, 246, 255});
        }
        else
        {
            draw_text(rect.x + 8, rect.y + 16, clamp_text_to_width(entry.description, text_width_px, 1), 1, {180, 196, 220, 255});
        }

        if (entry.kind == ProgramSourceKind::Disk)
        {
            const int file_x = rect.x + rect.w - 8 - file_label_width;
            draw_text(file_x, rect.y + 4, file_label, 1, {160, 210, 190, 255});
        }

    }

    template <typename CpuStateArray>
    auto draw_instruction_panel(const SDL_Rect &rect, const CpuStateArray &cpu_states) -> void
    {
        draw_panel_background(rect);
        draw_text(rect.x + 12, rect.y + 10, "CURRENT INSTRUCTIONS", 2, {242, 246, 255, 255});

        const int controls_height = 34;
        const SDL_Rect controls_rect{rect.x + 10, rect.y + rect.h - controls_height - 8, rect.w - 20, controls_height};
        draw_controls(controls_rect);

        const int cpu_area_top = rect.y + 38;
        const int cpu_area_bottom = controls_rect.y - 8;
        const int cpu_area_height = std::max(40, cpu_area_bottom - cpu_area_top);
        const SDL_Rect cpu_area{rect.x + 10, cpu_area_top, rect.w - 20, cpu_area_height};

        const int cpu_count = static_cast<int>(cpu_states.size());
        const int columns = std::max(1, std::min(2, cpu_count));
        const int rows = (cpu_count + columns - 1) / columns;
        const int gap = 8;

        const int cell_width = std::max(100, (cpu_area.w - ((columns - 1) * gap)) / columns);
        const int cell_height = std::max(60, (cpu_area.h - ((rows - 1) * gap)) / rows);

        for (int idx = 0; idx < cpu_count; ++idx)
        {
            const int col = idx % columns;
            const int row = idx / columns;

            SDL_Rect cpu_rect{
                cpu_area.x + (col * (cell_width + gap)),
                cpu_area.y + (row * (cell_height + gap)),
                cell_width,
                cell_height};

            std::vector<std::string> lines;
            const auto &cpu = cpu_states[static_cast<size_t>(idx)];

            std::ostringstream l1;
            l1 << "SP:" << std::setw(4) << cpu.stack_pointer
               << " HALT:" << (cpu.halted ? "Y" : "N")
                    << " FLAGS:" << flags_to_string(cpu.flags);
            lines.push_back(l1.str());
            lines.push_back("NEXT: " + cpu.current_instruction_detail);

            // Render all registers in compact grouped rows so the panel stays readable.
            constexpr size_t registers_per_row = 3;
            for (size_t reg_index = 0; reg_index < cpu.registers.size(); reg_index += registers_per_row)
            {
                std::ostringstream reg_line;
                for (size_t column = 0; column < registers_per_row; ++column)
                {
                    const size_t index = reg_index + column;
                    if (index >= cpu.registers.size())
                        break;

                    if (column > 0)
                        reg_line << " ";

                    reg_line << "R" << index << ":" << preview_word(cpu.registers[index]);
                }
                lines.push_back(reg_line.str());
            }

            draw_panel(cpu_rect, std::string("CPU ") + std::to_string(cpu.id), lines);
        }

        SDL_SetRenderDrawColor(renderer, 120, 132, 160, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }

    auto draw_controls(const SDL_Rect &rect) -> void
    {
        SDL_SetRenderDrawColor(renderer, 40, 50, 68, 255);
        SDL_RenderFillRect(renderer, &rect);

        int x = rect.x + 8;
        const int y = rect.y + 5;
        const int h = rect.h - 10;

        button_step = {x, y, 74, h};
        draw_button(button_step, "STEP", {130, 168, 255, 255});
        x += 82;

        button_init_only = {x, y, 88, h};
        draw_button(button_init_only, "INIT", {200, 220, 255, 255});
        x += 96;

        button_run_pause = {x, y, 110, h};
        draw_button(button_run_pause, run_continuous ? "PAUSE" : "RUN", run_continuous ? SDL_Color{255, 150, 120, 255} : SDL_Color{120, 220, 150, 255});
        x += 118;

        button_speed_down = {x, y, 42, h};
        draw_button(button_speed_down, "-", {220, 220, 220, 255});
        x += 50;

        button_speed_up = {x, y, 42, h};
        draw_button(button_speed_up, "+", {220, 220, 220, 255});
        x += 54;

        std::ostringstream speed_line;
        speed_line << "SPEED " << sim_steps_per_second << " STEPS/S";
        draw_text(x, y + 6, speed_line.str(), 1, {230, 238, 252, 255});
        x += 174;

        button_value_mode = {x, y, 64, h};
        draw_button(button_value_mode, number_display_mode == NumberDisplayMode::Hex ? "HEX" : "DEC", {255, 225, 140, 255});

        draw_text(rect.x + rect.w - 340, y + 6, "PROGRAM: " + status_text, 1, {192, 210, 240, 255});
        draw_text(rect.x + rect.w - 340, y + 18, "HOTKEYS 1/2/3 LOAD  R RELOAD  I INIT", 1, {160, 184, 222, 255});
    }

    auto draw_button(const SDL_Rect &rect, const std::string &label, SDL_Color tint) -> void
    {
        SDL_SetRenderDrawColor(renderer, 54, 68, 92, 255);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, tint.r, tint.g, tint.b, tint.a);
        SDL_RenderDrawRect(renderer, &rect);
        draw_text(rect.x + 10, rect.y + 7, clamp_text_to_width(label, std::max(0, rect.w - 14), 1), 1, tint);
    }

    auto handle_mouse_click(int mouse_x, int mouse_y) -> void
    {
        if (point_in_rect(mouse_x, mouse_y, button_step))
        {
            ++pending_step_requests;
            return;
        }

        if (point_in_rect(mouse_x, mouse_y, button_init_only))
        {
            pending_command = UiCommand::RunInitOnly;
            return;
        }

        if (point_in_rect(mouse_x, mouse_y, button_run_pause))
        {
            run_continuous = !run_continuous;
            return;
        }

        if (point_in_rect(mouse_x, mouse_y, button_speed_down))
        {
            sim_steps_per_second = std::max(min_steps_per_second, sim_steps_per_second - 5);
            return;
        }

        if (point_in_rect(mouse_x, mouse_y, button_speed_up))
        {
            sim_steps_per_second = std::min(max_steps_per_second, sim_steps_per_second + 5);
            return;
        }

        if (point_in_rect(mouse_x, mouse_y, button_inspector_toggle))
        {
            inspector_visible = !inspector_visible;
            if (!inspector_visible)
            {
                program_preview_scroll_offset = 0;
                program_inspector_rect = {0, 0, 0, 0};
            }
            return;
        }

        if (point_in_rect(mouse_x, mouse_y, button_dropdown))
        {
            dropdown_open = !dropdown_open;
            return;
        }

        if (point_in_rect(mouse_x, mouse_y, button_menu_load))
        {
            pending_command = UiCommand::LoadSelectedProgram;
            return;
        }

        if (point_in_rect(mouse_x, mouse_y, button_menu_refresh))
        {
            pending_command = UiCommand::ReloadProgram;
            return;
        }

        if (point_in_rect(mouse_x, mouse_y, button_memory_toggle))
        {
            memory_view_mode = (memory_view_mode == MemoryViewMode::Writes) ? MemoryViewMode::Hex : MemoryViewMode::Writes;
            memory_hex_scroll_offset = 0;
            return;
        }

        if (point_in_rect(mouse_x, mouse_y, button_value_mode))
        {
            number_display_mode = (number_display_mode == NumberDisplayMode::Hex) ? NumberDisplayMode::Decimal : NumberDisplayMode::Hex;
            return;
        }

        if (memory_view_mode == MemoryViewMode::Hex && point_in_rect(mouse_x, mouse_y, button_memory_module_prev))
        {
            const size_t module_count = std::max<size_t>(1, active_module_count);
            if (selected_memory_module == 0)
                selected_memory_module = module_count - 1;
            else
                --selected_memory_module;
            memory_hex_scroll_offset = 0;
            return;
        }

        if (memory_view_mode == MemoryViewMode::Hex && point_in_rect(mouse_x, mouse_y, button_memory_module_next))
        {
            const size_t module_count = std::max<size_t>(1, active_module_count);
            selected_memory_module = (selected_memory_module + 1) % module_count;
            memory_hex_scroll_offset = 0;
            return;
        }

        if (point_in_rect(mouse_x, mouse_y, button_video_module_prev))
        {
            const size_t module_count = std::max<size_t>(1, active_module_count);
            if (selected_visual_module == 0)
                selected_visual_module = module_count - 1;
            else
                --selected_visual_module;
            return;
        }

        if (point_in_rect(mouse_x, mouse_y, button_video_module_next))
        {
            const size_t module_count = std::max<size_t>(1, active_module_count);
            selected_visual_module = (selected_visual_module + 1) % module_count;
            return;
        }

        if (dropdown_open)
        {
            for (size_t i = 0; i < dropdown_entry_rects.size(); ++i)
            {
                if (point_in_rect(mouse_x, mouse_y, dropdown_entry_rects[i]))
                {
                    const int visible_index = dropdown_scroll_offset + static_cast<int>(i);
                    if (visible_index >= 0 && static_cast<size_t>(visible_index) < program_entries.size())
                    {
                        selected_program_index = static_cast<size_t>(visible_index);
                        dropdown_open = false;
                        clamp_inspector_scroll();
                    }
                    return;
                }
            }
        }

        if (point_in_rect(mouse_x, mouse_y, button_inspector_up))
        {
            program_preview_scroll_offset = std::max(0, program_preview_scroll_offset - 1);
            return;
        }

        if (point_in_rect(mouse_x, mouse_y, button_inspector_down))
        {
            ++program_preview_scroll_offset;
            clamp_inspector_scroll();
            return;
        }
    }

    static auto point_in_rect(int x, int y, const SDL_Rect &rect) -> bool
    {
        return x >= rect.x && y >= rect.y && x < (rect.x + rect.w) && y < (rect.y + rect.h);
    }

    auto clamp_menu_scroll() -> void
    {
        const int visible_rows = 7;
        const int max_scroll = std::max(0, static_cast<int>(program_entries.size()) - visible_rows);
        dropdown_scroll_offset = std::clamp(dropdown_scroll_offset, 0, max_scroll);
    }

    auto clamp_inspector_scroll(int lines = -1, int visible_lines = -1) -> void
    {
        if (lines < 0 || visible_lines < 0)
        {
            if (selected_program_index >= program_entries.size())
            {
                program_preview_scroll_offset = 0;
                return;
            }

            const auto definition = selected_program_definition();
            if (!definition)
            {
                program_preview_scroll_offset = 0;
                return;
            }

            lines = static_cast<int>(build_program_lines(*definition).size());
            visible_lines = 10;
        }

        const int max_scroll = std::max(0, lines - visible_lines);
        program_preview_scroll_offset = std::clamp(program_preview_scroll_offset, 0, max_scroll);
    }

    auto draw_memory_panel(const SDL_Rect &rect, const std::vector<std::string> &lines) -> void
    {
        draw_panel_background(rect);
        draw_text(rect.x + 12, rect.y + 10, memory_view_mode == MemoryViewMode::Writes ? "MEMORY CHANGES" : "MEMORY HEX VIEW", 2, {242, 246, 255, 255});

        button_memory_toggle = {rect.x + rect.w - 118, rect.y + 8, 106, 22};
        draw_button(button_memory_toggle, memory_view_mode == MemoryViewMode::Writes ? "HEX VIEW" : "WRITES", {200, 230, 180, 255});

        if (memory_view_mode == MemoryViewMode::Hex)
        {
            button_memory_module_prev = {rect.x + rect.w - 186, rect.y + 8, 24, 22};
            button_memory_module_next = {rect.x + rect.w - 156, rect.y + 8, 24, 22};
            draw_button(button_memory_module_prev, "<", {220, 220, 220, 255});
            draw_button(button_memory_module_next, ">", {220, 220, 220, 255});

            std::ostringstream label;
            label << "M" << selected_memory_module << " " << module_name(selected_memory_module);
            draw_text(rect.x + rect.w - 280, rect.y + 34, label.str(), 1, {180, 210, 255, 255});
        }

        int y = rect.y + (memory_view_mode == MemoryViewMode::Hex ? 52 : 38);
        const int line_height = 12;
        const int max_y = rect.y + rect.h - 8;
        for (const auto &line : lines)
        {
            if (y + line_height > max_y)
                break;

            SDL_Color line_color{214, 224, 240, 255};
            if (memory_view_mode == MemoryViewMode::Hex)
            {
                size_t channel = 0;
                size_t index = 0;
                if (try_parse_memory_hex_line(line, channel, index) &&
                    changed_memory_positions_this_frame.count(memory_position_key(channel, index)) > 0)
                {
                    line_color = {255, 120, 120, 255};
                }
            }

            draw_text(rect.x + 12, y, line, 1, line_color);
            y += line_height;
        }

        SDL_SetRenderDrawColor(renderer, 120, 132, 160, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }

    auto build_program_lines(const ProgramDefinition &program) const -> std::vector<std::string>
    {
        std::vector<std::string> lines;
        lines.push_back("NAME: " + program.name);
        const auto wrapped_desc = wrap_text(program.description, 48);
        if (wrapped_desc.empty())
        {
            lines.push_back("DESC:");
        }
        else
        {
            lines.push_back("DESC: " + wrapped_desc.front());
            for (size_t i = 1; i < wrapped_desc.size(); ++i)
                lines.push_back("      " + wrapped_desc[i]);
        }
        lines.push_back("WORDS:");

        for (const auto &word : program.words)
        {
            std::ostringstream line;
            line << "  W" << std::setw(4) << word.index << " = " << format_word_value(word.value, false);
            lines.push_back(line.str());
        }

        lines.push_back("INSTRUCTIONS:");
        for (const auto &instruction : program.instructions)
        {
            std::ostringstream line;
            line << "  I" << std::setw(4) << instruction.index
                 << " " << instruction_name(instruction.type);

            if (instruction.memory_access)
            {
                line << " R:R" << static_cast<int>(instruction.dest)
                     << " M:" << static_cast<int>(instruction.module)
                     << " A:0x" << std::hex << std::setw(4) << std::setfill('0') << instruction.address << std::dec << std::setfill(' ');
            }
            else
            {
                line << " D:R" << static_cast<int>(instruction.dest)
                     << " A:R" << static_cast<int>(instruction.src_1)
                     << " B:R" << static_cast<int>(instruction.src_2);
            }

            lines.push_back(line.str());
        }

        return lines;
    }

    static auto instruction_name(FIAT128::InstructionType instruction) -> std::string
    {
        switch (instruction)
        {
        case FIAT128::InstructionType::XXX:
            return "XXX";
        case FIAT128::InstructionType::ADD:
            return "ADD";
        case FIAT128::InstructionType::AND:
            return "AND";
        case FIAT128::InstructionType::OR:
            return "OR";
        case FIAT128::InstructionType::XOR:
            return "XOR";
        case FIAT128::InstructionType::MOV:
            return "MOV";
        case FIAT128::InstructionType::BUN:
            return "BUN";
        case FIAT128::InstructionType::BIZ:
            return "BIZ";
        case FIAT128::InstructionType::BNZ:
            return "BNZ";
        case FIAT128::InstructionType::LDA:
            return "LDA";
        case FIAT128::InstructionType::STA:
            return "STA";
        case FIAT128::InstructionType::LDR:
            return "LDR";
        case FIAT128::InstructionType::STR:
            return "STR";
        case FIAT128::InstructionType::EQL:
            return "EQL";
        case FIAT128::InstructionType::GRT:
            return "GRT";
        case FIAT128::InstructionType::SHL:
            return "SHL";
        case FIAT128::InstructionType::SHR:
            return "SHR";
        case FIAT128::InstructionType::ROL:
            return "ROL";
        case FIAT128::InstructionType::ROR:
            return "ROR";
        case FIAT128::InstructionType::INT:
            return "INT";
        case FIAT128::InstructionType::HLT:
            return "HLT";
        }

        return "UNK";
    }

    template <size_t word_size>
    auto bitset_to_hex(const std::bitset<word_size> &value) const -> std::string
    {
        static constexpr size_t nibble_count = (word_size + 3) / 4;
        std::string out;
        out.reserve(2 + nibble_count);
        out += "0x";

        for (size_t nibble = 0; nibble < nibble_count; ++nibble)
        {
            int digit = 0;
            for (int bit = 0; bit < 4; ++bit)
            {
                const size_t index = (nibble_count - 1 - nibble) * 4 + static_cast<size_t>(bit);
                if (index < word_size && value.test(index))
                    digit |= (1 << (3 - bit));
            }

            if (digit < 10)
                out.push_back(static_cast<char>('0' + digit));
            else
                out.push_back(static_cast<char>('A' + (digit - 10)));
        }

        return out;
    }

    template <size_t word_size>
    auto bitset_to_hex_spaced(const std::bitset<word_size> &value) const -> std::string
    {
        const std::string compact = bitset_to_hex(value);
        std::string spaced;
        spaced.reserve(compact.size() + (compact.size() / 2));

        // Keep the 0x prefix intact, then group hex digits by byte.
        spaced += "0x";
        size_t digits = 0;
        for (size_t i = 2; i < compact.size(); ++i)
        {
            spaced.push_back(compact[i]);
            ++digits;
            if ((digits % 2) == 0 && (i + 1) < compact.size())
                spaced.push_back(' ');
        }

        return spaced;
    }

    auto decimal_mul2(std::string &value) const -> void
    {
        int carry = 0;
        for (size_t i = value.size(); i-- > 0;)
        {
            const int digit = (value[i] - '0') * 2 + carry;
            value[i] = static_cast<char>('0' + (digit % 10));
            carry = digit / 10;
        }

        if (carry > 0)
            value.insert(value.begin(), static_cast<char>('0' + carry));
    }

    auto decimal_add1(std::string &value) const -> void
    {
        int carry = 1;
        for (size_t i = value.size(); i-- > 0 && carry > 0;)
        {
            const int digit = (value[i] - '0') + carry;
            value[i] = static_cast<char>('0' + (digit % 10));
            carry = digit / 10;
        }

        if (carry > 0)
            value.insert(value.begin(), static_cast<char>('0' + carry));
    }

    template <size_t word_size>
    auto bitset_to_unsigned_decimal(const std::bitset<word_size> &value) const -> std::string
    {
        std::string text = "0";
        for (size_t i = word_size; i-- > 0;)
        {
            decimal_mul2(text);
            if (value.test(i))
                decimal_add1(text);
        }

        return text;
    }

    template <size_t word_size>
    auto bitset_to_i128_string(const std::bitset<word_size> &value) const -> std::string
    {
        static_assert(word_size <= 128, "int128 view supports up to 128-bit words.");

        const bool is_negative = word_size > 0 && value.test(word_size - 1);
        if (!is_negative)
            return bitset_to_unsigned_decimal(value);

        std::bitset<word_size> magnitude_bits = ~value;
        bool carry = true;
        for (size_t i = 0; i < word_size && carry; ++i)
        {
            const bool bit = magnitude_bits.test(i);
            magnitude_bits.set(i, !bit);
            carry = bit;
        }

        const std::string magnitude = bitset_to_unsigned_decimal(magnitude_bits);

        return std::string("-") + magnitude;
    }

    template <size_t word_size>
    auto format_word_value(const std::bitset<word_size> &value, bool spaced_hex) const -> std::string
    {
        if (number_display_mode == NumberDisplayMode::Decimal)
            return bitset_to_i128_string(value);

        return spaced_hex ? bitset_to_hex_spaced(value) : bitset_to_hex(value);
    }

    template <size_t word_size>
    static auto printable_char_preview(const std::bitset<word_size> &value) -> std::string
    {
        unsigned int ch = 0;
        for (size_t bit = 0; bit < 8 && bit < word_size; ++bit)
        {
            if (value.test(bit))
                ch |= (1U << static_cast<unsigned int>(bit));
        }

        if (ch >= 32 && ch <= 126)
            return std::string("'") + static_cast<char>(ch) + "'";

        if (ch == 0)
            return "' '";

        if (ch == 10)
            return "'\\n'";

        if (ch == 13)
            return "'\\r'";

        return ".";
    }

    auto selected_program_footer() const -> std::string
    {
        if (selected_program_index >= program_entries.size())
            return "NO PROGRAM SELECTED";

        const auto &entry = program_entries[selected_program_index];
        if (entry.kind == ProgramSourceKind::Disk)
            return std::string("SELECTED: ") + entry.disk_path.string();

        return std::string("SELECTED: ") + entry.display_name;
    }

    template <size_t module_count>
    auto draw_video_panel(const SDL_Rect &rect, const std::array<std::vector<std::bitset<128>>, module_count> &snapshot, const std::vector<std::string> &overlay_lines) -> void
    {
        draw_panel_background(rect);

        const int grid_padding = 14;
        const int grid_x = rect.x + grid_padding;
        const int grid_y = rect.y + 36;
        const int grid_w = rect.w - (grid_padding * 2);
        const int grid_h = rect.h - 90;

        button_video_module_prev = {rect.x + rect.w - 120, rect.y + 8, 24, 20};
        button_video_module_next = {rect.x + rect.w - 30, rect.y + 8, 24, 20};
        draw_button(button_video_module_prev, "<", {220, 220, 220, 255});
        draw_button(button_video_module_next, ">", {220, 220, 220, 255});

        const size_t module_index = std::min(selected_visual_module, module_count > 0 ? module_count - 1 : 0);

        SDL_SetRenderDrawColor(renderer, 22, 28, 40, 255);
        SDL_Rect viewport{grid_x, grid_y, grid_w, grid_h};
        SDL_RenderFillRect(renderer, &viewport);

        if (module_index == 3)
        {
            const int cols = 32;
            const int rows = 18;
            const int cell_w = std::max(1, grid_w / cols);
            const int cell_h = std::max(1, grid_h / rows);
            const Uint64 elapsed_ms = SDL_GetTicks64() - animation_start_ticks;
            const int phase = static_cast<int>((elapsed_ms / 80U) % static_cast<Uint64>(cols));

            for (int y = 0; y < rows; ++y)
            {
                for (int x = 0; x < cols; ++x)
                {
                    const int pulse = (x + (y * 2) + phase) % cols;
                    const Uint8 r = static_cast<Uint8>(16 + (pulse * 7));
                    const Uint8 g = static_cast<Uint8>(32 + (pulse * 5));
                    const Uint8 b = static_cast<Uint8>(64 + (pulse * 6));

                    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
                    SDL_Rect cell{grid_x + (x * cell_w), grid_y + (y * cell_h), cell_w - 1, cell_h - 1};
                    SDL_RenderFillRect(renderer, &cell);
                }
            }
        }
        else if (module_index == 2)
        {
            const auto &console_words = snapshot[module_index];
            const size_t console_capacity = std::min<size_t>(512, console_words.size());
            std::string text;
            text.reserve(console_capacity);

            for (size_t i = 0; i < console_capacity; ++i)
            {
                unsigned int ch = 0;
                for (size_t bit = 0; bit < 8; ++bit)
                {
                    if (console_words[i].test(bit))
                        ch |= (1U << static_cast<unsigned int>(bit));
                }

                if (ch == 0)
                    text.push_back(' ');
                else if (ch == 10 || ch == 13)
                    text.push_back('\n');
                else if (ch >= 32 && ch <= 126)
                    text.push_back(static_cast<char>(ch));
                else
                    text.push_back(' ');
            }

            const int max_columns = std::max(1, (grid_w - 8) / 6);
            const int max_rows = std::max(1, (grid_h - 8) / 12);
            int x = grid_x + 4;
            int y = grid_y + 4;
            int current_col = 0;
            int current_row = 0;

            for (char c : text)
            {
                if (current_row >= max_rows)
                    break;

                if (c == '\n' || current_col >= max_columns)
                {
                    ++current_row;
                    current_col = 0;
                    x = grid_x + 4;
                    y += 12;
                    if (c == '\n')
                        continue;
                }

                draw_text(x, y, std::string(1, c), 1, {180, 255, 180, 255});
                x += 6;
                ++current_col;
            }
        }
        else
        {
            const int cols = 32;
            const int rows = 16;
            const int cell_w = std::max(1, grid_w / cols);
            const int cell_h = std::max(1, grid_h / rows);
            const auto &module_words = snapshot[module_index];

            for (int y = 0; y < rows; ++y)
            {
                for (int x = 0; x < cols; ++x)
                {
                    const size_t index = static_cast<size_t>(y * cols + x);
                    Uint8 r = 18;
                    Uint8 g = 22;
                    Uint8 b = 28;

                    if (index < module_words.size())
                    {
                        unsigned int intensity = 0;
                        for (size_t bit = 0; bit < 8; ++bit)
                        {
                            if (module_words[index].test(bit))
                                intensity |= (1U << static_cast<unsigned int>(bit));
                        }

                        if (module_words[index].any())
                        {
                            const Uint8 scaled = static_cast<Uint8>(40 + (intensity % 216));
                            r = static_cast<Uint8>(scaled / 2);
                            g = static_cast<Uint8>(scaled / 3);
                            b = scaled;
                        }
                    }

                    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
                    SDL_Rect cell{grid_x + (x * cell_w), grid_y + (y * cell_h), cell_w - 1, cell_h - 1};
                    SDL_RenderFillRect(renderer, &cell);
                }
            }
        }
        {
            std::ostringstream title;
            title << "MODULE VIEW M" << module_index << " " << module_name(module_index);
            draw_text(rect.x + 12, rect.y + 10, title.str(), 2, {242, 246, 255, 255});
        }

        int line_y = rect.y + rect.h - 44;
        for (const auto &line : overlay_lines)
        {
            draw_text(rect.x + 12, line_y, line, 1, {180, 210, 255, 255});
            line_y += 12;
        }

        SDL_SetRenderDrawColor(renderer, 120, 132, 160, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }

    auto draw_panel(const SDL_Rect &rect, const std::string &title, const std::vector<std::string> &lines) -> void
    {
        draw_panel_background(rect);
        draw_text(rect.x + 12, rect.y + 10, title, 2, {242, 246, 255, 255});

        const int line_height = 12;
        int y = rect.y + 38;
        for (const auto &line : lines)
        {
            if (y + line_height > rect.y + rect.h - 8)
                break;
            draw_text(rect.x + 12, y, line, 1, {214, 224, 240, 255});
            y += line_height;
        }

        SDL_SetRenderDrawColor(renderer, 120, 132, 160, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }

    auto draw_panel_background(const SDL_Rect &rect) -> void
    {
        SDL_SetRenderDrawColor(renderer, 28, 36, 52, 255);
        SDL_RenderFillRect(renderer, &rect);
    }

    auto draw_text(int x, int y, const std::string &text, int scale, SDL_Color color) -> void
    {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

        int cursor_x = x;
        for (char c : text)
        {
            const auto glyph = lookup_glyph(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
            for (size_t row = 0; row < 7; ++row)
            {
                for (size_t col = 0; col < 5; ++col)
                {
                    const bool on = ((glyph[row] >> (4U - static_cast<unsigned int>(col))) & 0x1U) != 0U;
                    if (!on)
                        continue;

                    SDL_Rect pixel{cursor_x + (static_cast<int>(col) * scale), y + (static_cast<int>(row) * scale), scale, scale};
                    SDL_RenderFillRect(renderer, &pixel);
                }
            }
            cursor_x += (6 * scale);
        }
    }

    static auto clamp_text_to_width(const std::string &text, int max_width_pixels, int scale) -> std::string
    {
        if (max_width_pixels <= 0 || text.empty())
            return {};

        const int glyph_w = std::max(1, 6 * scale);
        const int max_chars = max_width_pixels / glyph_w;
        if (max_chars <= 0)
            return {};
        if (static_cast<int>(text.size()) <= max_chars)
            return text;
        if (max_chars <= 3)
            return text.substr(0, static_cast<size_t>(max_chars));

        return text.substr(0, static_cast<size_t>(max_chars - 3)) + "...";
    }

    static auto wrap_text(const std::string &text, size_t max_chars_per_line) -> std::vector<std::string>
    {
        std::vector<std::string> lines;
        if (max_chars_per_line == 0)
            return lines;

        std::istringstream in(text);
        std::string word;
        std::string current;

        while (in >> word)
        {
            if (word.size() > max_chars_per_line)
            {
                if (!current.empty())
                {
                    lines.push_back(current);
                    current.clear();
                }

                size_t start = 0;
                while (start < word.size())
                {
                    const size_t count = std::min(max_chars_per_line, word.size() - start);
                    lines.push_back(word.substr(start, count));
                    start += count;
                }
                continue;
            }

            if (current.empty())
            {
                current = word;
                continue;
            }

            if (current.size() + 1 + word.size() <= max_chars_per_line)
            {
                current += ' ';
                current += word;
            }
            else
            {
                lines.push_back(current);
                current = word;
            }
        }

        if (!current.empty())
            lines.push_back(current);

        return lines;
    }

    static auto memory_position_key(size_t channel, size_t index) -> uint64_t
    {
        return (static_cast<uint64_t>(channel) << 32U) | static_cast<uint64_t>(index);
    }

    static auto try_parse_memory_hex_line(const std::string &line, size_t &channel, size_t &index) -> bool
    {
        // Data rows begin with "  <channel> <index> ..." while headers do not.
        std::istringstream in(line);
        size_t parsed_channel = 0;
        size_t parsed_index = 0;

        if (!(in >> parsed_channel >> parsed_index))
            return false;

        channel = parsed_channel;
        index = parsed_index;
        return true;
    }

    static auto module_name(size_t module) -> std::string
    {
        switch (module)
        {
        case 0:
            return "ROM";
        case 1:
            return "RAM";
        case 2:
            return "CONSOLE";
        case 3:
            return "GPU";
        default:
            return "MODULE";
        }
    }

    static auto lookup_glyph(char c) -> const std::array<uint8_t, 7> &
    {
        static const std::array<uint8_t, 7> blank{0, 0, 0, 0, 0, 0, 0};
        static const std::unordered_map<char, std::array<uint8_t, 7>> glyphs = {
            {'A', {14, 17, 17, 31, 17, 17, 17}}, {'B', {30, 17, 17, 30, 17, 17, 30}}, {'C', {14, 17, 16, 16, 16, 17, 14}}, {'D', {30, 17, 17, 17, 17, 17, 30}}, {'E', {31, 16, 16, 30, 16, 16, 31}}, {'F', {31, 16, 16, 30, 16, 16, 16}}, {'G', {14, 17, 16, 23, 17, 17, 14}}, {'H', {17, 17, 17, 31, 17, 17, 17}}, {'I', {31, 4, 4, 4, 4, 4, 31}}, {'J', {1, 1, 1, 1, 17, 17, 14}}, {'K', {17, 18, 20, 24, 20, 18, 17}}, {'L', {16, 16, 16, 16, 16, 16, 31}}, {'M', {17, 27, 21, 21, 17, 17, 17}}, {'N', {17, 25, 21, 19, 17, 17, 17}}, {'O', {14, 17, 17, 17, 17, 17, 14}}, {'P', {30, 17, 17, 30, 16, 16, 16}}, {'Q', {14, 17, 17, 17, 21, 18, 13}}, {'R', {30, 17, 17, 30, 20, 18, 17}}, {'S', {15, 16, 16, 14, 1, 1, 30}}, {'T', {31, 4, 4, 4, 4, 4, 4}}, {'U', {17, 17, 17, 17, 17, 17, 14}}, {'V', {17, 17, 17, 17, 17, 10, 4}}, {'W', {17, 17, 17, 21, 21, 21, 10}}, {'X', {17, 17, 10, 4, 10, 17, 17}}, {'Y', {17, 17, 10, 4, 4, 4, 4}}, {'Z', {31, 1, 2, 4, 8, 16, 31}},
            {'0', {14, 17, 19, 21, 25, 17, 14}}, {'1', {4, 12, 4, 4, 4, 4, 14}}, {'2', {14, 17, 1, 2, 4, 8, 31}}, {'3', {30, 1, 1, 14, 1, 1, 30}}, {'4', {2, 6, 10, 18, 31, 2, 2}}, {'5', {31, 16, 16, 30, 1, 1, 30}}, {'6', {14, 16, 16, 30, 17, 17, 14}}, {'7', {31, 1, 2, 4, 8, 8, 8}}, {'8', {14, 17, 17, 14, 17, 17, 14}}, {'9', {14, 17, 17, 15, 1, 1, 14}},
            {' ', {0, 0, 0, 0, 0, 0, 0}}, {':', {0, 4, 0, 0, 4, 0, 0}}, {'#', {10, 31, 10, 10, 31, 10, 0}}, {'.', {0, 0, 0, 0, 0, 4, 0}}, {'-', {0, 0, 0, 31, 0, 0, 0}}, {'_', {0, 0, 0, 0, 0, 0, 31}}, {'(', {2, 4, 8, 8, 8, 4, 2}}, {')', {8, 4, 2, 2, 2, 4, 8}}, {'/', {1, 2, 4, 8, 16, 0, 0}}, {'|', {4, 4, 4, 4, 4, 4, 4}}, {'=', {0, 31, 0, 31, 0, 0, 0}}, {'+', {0, 4, 4, 31, 4, 4, 0}},
            {'*', {0, 10, 4, 31, 4, 10, 0}}, {'!', {4, 4, 4, 4, 4, 0, 4}}, {'?', {14, 17, 1, 2, 4, 0, 4}}, {'[', {14, 8, 8, 8, 8, 8, 14}}, {']', {14, 2, 2, 2, 2, 2, 14}}, {',', {0, 0, 0, 0, 0, 4, 8}}, {';', {0, 4, 0, 0, 0, 4, 8}}
        };

        const auto it = glyphs.find(c);
        return it == glyphs.end() ? blank : it->second;
    }

    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;

    int window_width = 1280;
    int window_height = 720;
    bool running = true;
    size_t frame_counter = 0;
    Uint64 animation_start_ticks = 0;
    size_t last_seen_memory_sequence = 0;
    UiCommand pending_command = UiCommand::None;
    std::string status_text = "Idle Loop";
    std::deque<std::string> memory_lines;
    std::vector<ProgramCatalogEntry> program_entries;
    size_t selected_program_index = 0;
    int dropdown_scroll_offset = 0;
    int program_preview_scroll_offset = 0;
    bool dropdown_open = false;
    bool inspector_visible = true;
    MemoryViewMode memory_view_mode = MemoryViewMode::Writes;
    NumberDisplayMode number_display_mode = NumberDisplayMode::Hex;
    int memory_hex_scroll_offset = 0;
    std::unordered_set<uint64_t> changed_memory_positions_this_frame;
    std::vector<SDL_Rect> dropdown_entry_rects;
    SDL_Rect program_inspector_rect{0, 0, 0, 0};
    SDL_Rect memory_panel_rect{0, 0, 0, 0};

    bool run_continuous = false;
    int sim_steps_per_second = 30;
    int pending_step_requests = 0;
    static constexpr int min_steps_per_second = 1;
    static constexpr int max_steps_per_second = 300;

    SDL_Rect button_step{0, 0, 0, 0};
    SDL_Rect button_init_only{0, 0, 0, 0};
    SDL_Rect button_run_pause{0, 0, 0, 0};
    SDL_Rect button_speed_down{0, 0, 0, 0};
    SDL_Rect button_speed_up{0, 0, 0, 0};
    SDL_Rect button_dropdown{0, 0, 0, 0};
    SDL_Rect button_inspector_toggle{0, 0, 0, 0};
    SDL_Rect button_memory_toggle{0, 0, 0, 0};
    SDL_Rect button_memory_module_prev{0, 0, 0, 0};
    SDL_Rect button_memory_module_next{0, 0, 0, 0};
    SDL_Rect button_value_mode{0, 0, 0, 0};
    SDL_Rect button_menu_load{0, 0, 0, 0};
    SDL_Rect button_menu_refresh{0, 0, 0, 0};
    SDL_Rect button_inspector_up{0, 0, 0, 0};
    SDL_Rect button_inspector_down{0, 0, 0, 0};
    SDL_Rect button_video_module_prev{0, 0, 0, 0};
    SDL_Rect button_video_module_next{0, 0, 0, 0};
    size_t selected_visual_module = 3;
    size_t selected_memory_module = 1;
    size_t active_module_count = 4;
    int last_mouse_x = 0;
    int last_mouse_y = 0;
};
