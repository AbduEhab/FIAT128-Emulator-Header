#pragma once

#include <algorithm>
#include <chrono>
#include <deque>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class TerminalRenderer
{
public:
    TerminalRenderer(size_t width = 100, size_t height = 30)
        : screen_width(width), screen_height(height)
    {
    }

    template <typename EmulatorT>
    auto render(EmulatorT &emulator) -> void
    {
        auto cpu_states = emulator.get_cpu_render_state();
        auto new_events = emulator.get_memory_write_events_since(last_seen_memory_sequence);

        if (!new_events.empty())
        {
            last_seen_memory_sequence = new_events.back().sequence;
            for (const auto &event : new_events)
            {
                memory_lines.push_back(format_memory_event(event));
                if (memory_lines.size() > 14)
                    memory_lines.pop_front();
            }
        }

        std::vector<std::string> instruction_lines;
        for (const auto &cpu : cpu_states)
        {
            std::ostringstream line;
            line << "CPU " << cpu.id << " | SP=" << std::setw(4) << cpu.stack_pointer
                 << " | HALT=" << (cpu.halted ? "Y" : "N")
                 << " | NEXT=" << cpu.current_instruction;
            instruction_lines.push_back(line.str());
        }

        std::vector<std::string> memory_panel(memory_lines.begin(), memory_lines.end());
        if (memory_panel.empty())
            memory_panel.push_back("Waiting for memory writes...");

        auto video_lines = generate_video_placeholder();

        const size_t top_height = 10;
        const size_t bottom_height = screen_height - top_height - 1;
        const size_t left_width = screen_width / 2;
        const size_t right_width = screen_width - left_width - 1;

        auto instruction_box = make_box("Instruction Window", instruction_lines, screen_width, top_height);
        auto memory_box = make_box("Memory Changes", memory_panel, left_width, bottom_height);
        auto video_box = make_box("Video Card (Placeholder)", video_lines, right_width, bottom_height);

        std::cout << "\x1b[2J\x1b[H";
        for (const auto &line : instruction_box)
            std::cout << line << '\n';

        for (size_t i = 0; i < memory_box.size() && i < video_box.size(); ++i)
            std::cout << memory_box[i] << ' ' << video_box[i] << '\n';

        std::cout.flush();
        ++frame_index;
    }

private:
    template <typename MemoryEvent>
    auto format_memory_event(const MemoryEvent &event) -> std::string
    {
        std::string bits = event.value.to_string();
        const size_t preview_width = std::min<size_t>(16, bits.size());
        std::string preview = bits.substr(0, preview_width);

        std::ostringstream out;
        out << "#" << event.sequence
            << " CPU=" << event.cpu_id
            << " CH=" << event.channel
            << " IDX=" << event.index
            << " VAL=" << preview << "...";
        return out.str();
    }

    auto generate_video_placeholder() const -> std::vector<std::string>
    {
        constexpr size_t video_width = 32;
        constexpr size_t video_height = 12;

        std::vector<std::string> rows;
        rows.reserve(video_height);

        for (size_t y = 0; y < video_height; ++y)
        {
            std::string row;
            row.reserve(video_width);
            for (size_t x = 0; x < video_width; ++x)
            {
                const size_t signal = (x + y + frame_index) % 10;
                row.push_back(signal < 3 ? '#' : (signal < 6 ? '+' : '.'));
            }
            rows.push_back(row);
        }

        rows.push_back("Future VRAM will render here.");
        return rows;
    }

    auto make_box(const std::string &title, const std::vector<std::string> &content, size_t width, size_t height) const -> std::vector<std::string>
    {
        std::vector<std::string> box;
        if (width < 8 || height < 3)
            return box;

        const std::string horizontal(width - 2, '-');
        box.push_back("+" + horizontal + "+");

        std::string header = "| " + title;
        if (header.size() < width - 1)
            header.append(width - 1 - header.size(), ' ');
        header.back() = '|';
        box.push_back(header);

        const size_t lines_available = height - 3;
        for (size_t i = 0; i < lines_available; ++i)
        {
            std::string line = "| ";
            if (i < content.size())
                line += content[i];

            if (line.size() < width - 1)
                line.append(width - 1 - line.size(), ' ');
            else
                line.resize(width - 1);

            line.push_back('|');
            box.push_back(line);
        }

        box.push_back("+" + horizontal + "+");
        return box;
    }

    size_t screen_width = 100;
    size_t screen_height = 30;
    size_t frame_index = 0;
    size_t last_seen_memory_sequence = 0;
    std::deque<std::string> memory_lines;
};
