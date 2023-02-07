#include "Constants.hpp"

enum INSTRUCTION : uint8_t
{
    // Arithmetic instructions
    ADD = 0x01, // Adds two operands and stores the result in the destination operand
    SUB = 0x02, // Subtracts two operands and stores the result in the destination operand
    AND = 0x03, // Performs bitwise AND on two operands and stores the result in the destination operand
    OR = 0x04,  // Performs bitwise OR on two operands and stores the result in the destination operand
    XOR = 0x05, // Performs bitwise XOR on two operands and stores the result in the destination operand

    // Data transfer instructions
    MOVE = 0x06, // Copies data from source operand to destination operand
    BUN = 0x07,  // Unconditionally branches to the target address operand
    BIZ = 0x08,  // Branches to the target address operand if the source operand is zero
    BIN = 0x09,  // Branches to the target address operand if the source operand is not zero
    RET = 0x0A,  // Returns from the current function call

    // Memory access instructions
    MALLOC = 0x0B, // Allocates memory on the heap with the size specified in the size operand
    DALLOC = 0x0C, // Deallocates memory at the address specified in the source operand
    LDA = 0x0D,    // Loads the data at the source operand into the destination operand
    STA = 0x0E,    // Stores the data from the source operand into the memory at the destination operand

    // Comparison instructions
    EQ = 0x0F, // Compares two operands and sets the destination operand to 1 if they are equal, 0 otherwise
    NE = 0x10, // Compares two operands and sets the destination operand to 1 if they are not equal, 0 otherwise
    GT = 0x11, // Compares two operands and sets the destination operand to 1 if the first operand is greater than the second, 0 otherwise
    GE = 0x12, // Compares two operands and sets the destination operand to 1 if the first operand is greater than or equal to the second, 0 otherwise
    LT = 0x13, // Compares two operands and sets the destination operand to 1 if the first operand is less than the second, 0 otherwise
    LE = 0x14, // Compares two operands and sets the destination operand to 1 if the first operand is less than or equal to the second, 0 otherwise

    // Shift instructions
    SHL = 0x15, // Shifts the bits of the source operand left by the shift amount specified in the third operand
    SHR = 0x16, // Shifts the bits of the source operand right by the shift amount specified in the third operand

    // Rotate instructions
    ROL = 0x17, // Rotates the bits of the source operand left by the rotate amount specified in the third operand
    ROR = 0x18, // Rotates the bits of the source operand right by the rotate amount specified in the third operand

    // System instructions
    HALT = 0x19, // Halts the CPU

    // Vector instructions
    VADD = 0x1A, // Adds two vector operands and stores the result in the destination vector operand
    VSUB = 0x1B, // Subtracts two vector operands and stores the result in the destination vector operand
    VAND = 0x1C, // Performs bitwise AND on two vector operands and stores the result in the destination vector operand
    VOR = 0x1D,  // Performs bitwise OR on two vector operands and stores the result in the destination vector operand
    VXOR = 0x1E, // Performs bitwise XOR on two vector operands and stores the result in the destination vector operand
    VEQ = 0x1F,  // Compares two vector operands and sets a flag to 1 if they are equal, 0 otherwise
    VNE = 0x20,  // Compares two vector operands and sets a flag to 1 if they are not equal, 0 otherwise
};

int instruction_table[32][3] = {{INSTRUCTION::ADD, 1, 0}, {INSTRUCTION::SUB, 1, 0}, {INSTRUCTION::AND, 1, 0}, {INSTRUCTION::OR, 1, 0}, {INSTRUCTION::XOR, 1, 0}, {INSTRUCTION::MOVE, 1, 0}, {INSTRUCTION::BUN, 1, 0}, {INSTRUCTION::BIZ, 1, 0}, {INSTRUCTION::BIN, 1, 0}, {INSTRUCTION::RET, 1, 0}, {INSTRUCTION::MALLOC, 1, 0}, {INSTRUCTION::DALLOC, 1, 0}, {INSTRUCTION::LDA, 1, 1}, {INSTRUCTION::STA, 1, 1}, {INSTRUCTION::EQ, 2, 0}, {INSTRUCTION::NE, 2, 0}, {INSTRUCTION::GT, 2, 0}, {INSTRUCTION::GE, 2, 0}, {INSTRUCTION::LT, 2, 0}, {INSTRUCTION::LE, 2, 0}, {INSTRUCTION::SHL, 1, 0}, {INSTRUCTION::SHR, 1, 0}, {INSTRUCTION::ROL, 1, 0}, {INSTRUCTION::ROR, 1, 0}, {INSTRUCTION::HALT, 1, 0}, {INSTRUCTION::VADD, 1, 0}, {INSTRUCTION::VSUB, 1, 0}, {INSTRUCTION::VAND, 1, 0}, {INSTRUCTION::VOR, 1, 0}, {INSTRUCTION::VXOR, 1, 0}, {INSTRUCTION::VEQ, 2, 0}, {INSTRUCTION::VNE, 2, 0}};