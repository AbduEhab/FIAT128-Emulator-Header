#include <Constants.hpp>
#include <QuineMcCluskey.h>

float itr_sqrt(float n)
{
    // Extract the exponent and mantissa
    int32_t bits = *reinterpret_cast<int32_t *>(&n);
    int32_t exponent = ((bits >> 23) & 0xff) - 127;
    int32_t mantissa = bits & 0x7fffff;

    // Normalize the mantissa
    while (mantissa & 0x400000)
    {
        mantissa <<= 1;
        exponent--;
    }
    mantissa &= 0x3fffff;

    // Initialize the approximation
    float x = n;

    // Iterate until the approximation converges
    for (int i = 0; i < 10; i++)
    {
        // Compute a new approximation
        float y = (x + n / x) / 2;

        // Check if the approximation has converged
        if (std::abs(x - y) < 0.00001)
        {
            break;
        }

        // Update the approximation
        x = y;
    }

    // Combine the exponent and mantissa to produce the final result
    int32_t result = ((exponent + 127) << 23) | mantissa;
    return *reinterpret_cast<float *>(&result);
}

int main([[maybe_unused]] int, [[maybe_unused]] char **)
{
    Instrumentor::Get().beginSession("Main func");

    TimedBlock block("Main functions");

    print_by_force("Hello World!\nThe largest number I can hold is: ", std::numeric_limits<int>::max(), "\n");

    std::cout << itr_sqrt(16) << std::endl;

    return 0;
}
