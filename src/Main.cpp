#define STATIC_MEMORY

#include <FIAT128.hpp>

constexpr float itr_sqrt(float n)
{
    unsigned long i;
    float p0, p1;

    p0 = n;

    i = *(unsigned long *)&p0;

    i = 0x1FBD3F7D + (i >> 1);

    p0 = *(float *)&i;

    p1 = 0.5f * (p0 + n / p0);

    return p1;
}

int main([[maybe_unused]] int, [[maybe_unused]] char **)
{
    Instrumentor::Get().beginSession("Main func");

    TimedBlock block("Main functions");

    Fiat128::print_by_force("Hello World!\nThe largest number I can hold is: ", std::numeric_limits<int>::max(), "\n");

    std::cout << itr_sqrt(16) << std::endl;

    return 0;
}
