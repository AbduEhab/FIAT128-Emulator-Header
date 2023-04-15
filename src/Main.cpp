// #define STATIC_MEMORY

#include <FIAT128.hpp>

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
    Instrumentor::Get().beginSession("Main func");

    TimedBlock block("Main functions");

    auto Emulator = FIAT128::Emulator<1, 1>(0xFF);

    Emulator.add_word_to_cpu(0, 1, std::bitset<128>(2047));
    Emulator.add_word_to_cpu(0, 2, std::bitset<128>(0x1));
    Emulator.add_word_to_cpu(0, 3, std::bitset<128>(0x2));
    Emulator.add_instruction_to_cpu(0, 0, FIAT128::InstructionType::BUN, FIAT128::RegisterIndex::R0);
    Emulator.add_instruction_to_cpu(0, 0, FIAT128::InstructionType::MOV, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0);

    // Emulator.set_word(0, 0xFFFF00, 1);
    // Emulator.add_instruction(0, 0xFFFFFF - 1, FIAT128::InstructionType::ADD, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R0, FIAT128::RegisterIndex::R1);

    for (int i = 0; i < 100; i++)
    {
        Emulator.run(true);
    }

    return 0;
}

#endif // !FIAT128_IMPLEMENTATION