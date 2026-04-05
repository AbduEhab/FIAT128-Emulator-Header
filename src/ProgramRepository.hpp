#pragma once

#include "ProgramLibrary.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

enum class ProgramSourceKind
{
    BuiltIn,
    Disk,
};

struct ProgramCatalogEntry
{
    ProgramSourceKind kind = ProgramSourceKind::BuiltIn;
    ProgramId built_in_id = ProgramId::IdleLoop;
    std::string display_name;
    std::string description;
    std::filesystem::path disk_path;
    std::optional<ProgramDefinition> cached_definition;
};

inline auto to_upper_ascii(std::string text) -> std::string
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c)
    {
        return static_cast<char>(std::toupper(c));
    });
    return text;
}

inline auto trim_copy(const std::string &text) -> std::string
{
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return {};

    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

inline auto is_comment_or_empty(const std::string &line) -> bool
{
    const auto trimmed = trim_copy(line);
    return trimmed.empty() || trimmed.starts_with('#') || trimmed.starts_with("//");
}

inline auto parse_register_index(const std::string &token) -> std::optional<FIAT128::RegisterIndex>
{
    static const std::unordered_map<std::string, FIAT128::RegisterIndex> register_lookup = {
        {"R0", FIAT128::RegisterIndex::R0},
        {"R1", FIAT128::RegisterIndex::R1},
        {"R2", FIAT128::RegisterIndex::R2},
        {"R3", FIAT128::RegisterIndex::R3},
        {"R4", FIAT128::RegisterIndex::R4},
        {"R5", FIAT128::RegisterIndex::R5},
        {"R6", FIAT128::RegisterIndex::R6},
        {"R7", FIAT128::RegisterIndex::R7},
        {"R8", FIAT128::RegisterIndex::TIMER},
        {"TIMER", FIAT128::RegisterIndex::TIMER},
    };

    const auto it = register_lookup.find(to_upper_ascii(token));
    if (it == register_lookup.end())
        return std::nullopt;

    return it->second;
}

inline auto parse_instruction_type(const std::string &token) -> std::optional<FIAT128::InstructionType>
{
    static const std::unordered_map<std::string, FIAT128::InstructionType> instruction_lookup = {
        {"XXX", FIAT128::InstructionType::XXX},
        {"ADD", FIAT128::InstructionType::ADD},
        {"AND", FIAT128::InstructionType::AND},
        {"OR", FIAT128::InstructionType::OR},
        {"XOR", FIAT128::InstructionType::XOR},
        {"MOV", FIAT128::InstructionType::MOV},
        {"BUN", FIAT128::InstructionType::BUN},
        {"BIZ", FIAT128::InstructionType::BIZ},
        {"BNZ", FIAT128::InstructionType::BNZ},
        {"BIN", FIAT128::InstructionType::BNZ},
        {"LDA", FIAT128::InstructionType::LDA},
        {"STA", FIAT128::InstructionType::STA},
        {"LDR", FIAT128::InstructionType::LDR},
        {"STR", FIAT128::InstructionType::STR},
        {"EQL", FIAT128::InstructionType::EQL},
        {"GRT", FIAT128::InstructionType::GRT},
        {"SHL", FIAT128::InstructionType::SHL},
        {"SHR", FIAT128::InstructionType::SHR},
        {"ROL", FIAT128::InstructionType::ROL},
        {"ROR", FIAT128::InstructionType::ROR},
        {"INT", FIAT128::InstructionType::INT},
        {"HLT", FIAT128::InstructionType::HLT},
    };

    const auto it = instruction_lookup.find(to_upper_ascii(token));
    if (it == instruction_lookup.end())
        return std::nullopt;

    return it->second;
}

inline auto parse_module_index(const std::string &token) -> std::optional<unsigned char>
{
    std::string cleaned = to_upper_ascii(token);
    if (!cleaned.empty() && cleaned.front() == 'M')
        cleaned.erase(cleaned.begin());

    try
    {
        const auto value = std::stoul(cleaned, nullptr, 0);
        if (value > 15)
            return std::nullopt;
        return static_cast<unsigned char>(value);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

inline auto parse_u16_token(const std::string &token) -> std::optional<unsigned short>
{
    try
    {
        const auto value = std::stoul(token, nullptr, 0);
        if (value > 0xFFFF)
            return std::nullopt;
        return static_cast<unsigned short>(value);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

template <size_t word_size>
inline auto parse_binary_or_hex_word(const std::string &token) -> std::optional<std::bitset<word_size>>
{
    std::bitset<word_size> result;
    const std::string upper = to_upper_ascii(token);

    if (upper.starts_with("0X"))
    {
        const std::string digits = upper.substr(2);
        size_t bit_index = 0;
        for (auto it = digits.rbegin(); it != digits.rend(); ++it)
        {
            const char c = *it;
            int nibble = 0;
            if (c >= '0' && c <= '9')
                nibble = c - '0';
            else if (c >= 'A' && c <= 'F')
                nibble = 10 + (c - 'A');
            else
                return std::nullopt;

            for (int bit = 0; bit < 4; ++bit)
            {
                if (bit_index >= word_size)
                    break;
                result.set(bit_index, ((nibble >> bit) & 1) != 0);
                ++bit_index;
            }
        }

        return result;
    }

    if (upper.starts_with("0B"))
    {
        const std::string digits = upper.substr(2);
        size_t bit_index = 0;
        for (auto it = digits.rbegin(); it != digits.rend(); ++it)
        {
            if (*it != '0' && *it != '1')
                return std::nullopt;

            if (bit_index >= word_size)
                break;

            result.set(bit_index, *it == '1');
            ++bit_index;
        }

        return result;
    }

    if (!upper.empty() && std::all_of(upper.begin(), upper.end(), [](unsigned char c)
        {
            return c == '0' || c == '1';
        }))
    {
        size_t bit_index = 0;
        for (auto it = upper.rbegin(); it != upper.rend(); ++it)
        {
            if (bit_index >= word_size)
                break;
            result.set(bit_index, *it == '1');
            ++bit_index;
        }

        return result;
    }

    return std::nullopt;
}

inline auto parse_program_file(const std::filesystem::path &path) -> std::optional<ProgramDefinition>
{
    std::ifstream input(path);
    if (!input)
        return std::nullopt;

    ProgramDefinition program;
    program.id = ProgramId::IdleLoop;
    program.name = path.stem().string();
    program.description = std::string("Disk program: ") + program.name;

    std::string line;
    while (std::getline(input, line))
    {
        if (is_comment_or_empty(line))
            continue;

        std::istringstream stream(line);
        std::string command;
        stream >> command;
        command = to_upper_ascii(command);

        if (command == "NAME")
        {
            std::getline(stream, program.name);
            program.name = trim_copy(program.name);
            continue;
        }

        if (command == "DESCRIPTION")
        {
            std::getline(stream, program.description);
            program.description = trim_copy(program.description);
            continue;
        }

        if (command == "WORD" || command == "DATA")
        {
            size_t index = 0;
            std::string value_token;
            if (!(stream >> index >> value_token))
                return std::nullopt;

            auto value = parse_binary_or_hex_word<128>(value_token);
            if (!value)
                return std::nullopt;

            program.words.push_back({index, *value});
            continue;
        }

        if (command == "INSTR" || command == "INSTRUCTION")
        {
            size_t index = 0;
            std::string opcode_token;
            std::string dest_token;
            std::string src1_token;
            std::string src2_token;

            if (!(stream >> index >> opcode_token >> dest_token >> src1_token >> src2_token))
                return std::nullopt;

            auto opcode = parse_instruction_type(opcode_token);
            auto dest = parse_register_index(dest_token);
            if (!opcode || !dest)
                return std::nullopt;

            if (*opcode == FIAT128::InstructionType::LDA || *opcode == FIAT128::InstructionType::STA)
            {
                auto module = parse_module_index(src1_token);
                auto address = parse_u16_token(src2_token);
                if (!module || !address)
                    return std::nullopt;

                program.instructions.push_back({index, *opcode, *dest, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0, true, *module, *address});
            }
            else
            {
                auto src1 = parse_register_index(src1_token);
                auto src2 = parse_register_index(src2_token);
                if (!src1 || !src2)
                    return std::nullopt;

                program.instructions.push_back({index, *opcode, *dest, *src1, *src2});
            }
            continue;
        }

        return std::nullopt;
    }

    return program;
}

inline auto builtin_program_entries() -> std::vector<ProgramCatalogEntry>
{
    return {};
}

inline auto discover_program_entries(const std::filesystem::path &disk_directory) -> std::vector<ProgramCatalogEntry>
{
    std::vector<ProgramCatalogEntry> entries;

    if (!std::filesystem::exists(disk_directory))
        return entries;

    std::vector<std::filesystem::path> files;
    for (const auto &item : std::filesystem::directory_iterator(disk_directory))
    {
        if (!item.is_regular_file())
            continue;

        const auto extension = to_upper_ascii(item.path().extension().string());
        if (extension == ".FIATPROG" || extension == ".FIAT" || extension == ".PROG" || extension == ".TXT")
            files.push_back(item.path());
    }

    std::sort(files.begin(), files.end());

    for (const auto &file : files)
    {
        auto parsed = parse_program_file(file);
        if (!parsed)
            continue;

        entries.push_back({ProgramSourceKind::Disk, ProgramId::IdleLoop, parsed->name, parsed->description, file, parsed});
    }

    return entries;
}

template <size_t cores, size_t memory_modules, size_t word_size>
inline auto load_program_entry(FIAT128::Emulator<cores, memory_modules, word_size> &emulator, const ProgramCatalogEntry &entry) -> bool
{
    ProgramDefinition definition;

    if (entry.kind == ProgramSourceKind::Disk)
    {
        auto parsed = parse_program_file(entry.disk_path);
        if (!parsed)
            return false;
        definition = *parsed;
    }
    else if (entry.cached_definition)
    {
        definition = *entry.cached_definition;
    }
    else
    {
        definition = make_program(entry.built_in_id);
    }

    load_program(emulator, definition);
    return true;
}
