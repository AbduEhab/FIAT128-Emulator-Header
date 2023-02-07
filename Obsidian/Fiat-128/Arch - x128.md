[[Chip Layout]]

#Instructions:
[[Memory Layout]]

>Arithmetic instructions:

- ADD r0, num //$decimal, \#Hexa, bBinary
- SUB ''
- MUL ''
- DIV ''
- NEG ''

>Logical instructions:

- AND
- OR
- NOT
- XOR


>Data transfer instructions:

- LOAD
- STORE
- MOVE

>Control flow instructions:

- BUN
- BIZ
- BIN
- RET

>Memory access instructions:

- MALLOC
- DALLOC
- LDA
- STA

>Comparison instructions:

- EQ (equal)
- NE (not equal)
- GT (greater than)
- GE (greater than or equal)
- LT (less than)
- LE (less than or equal)

>Shift instructions:

- SHL (shift left)
- SHR (shift right)

>Rotate instructions:

- ROL (rotate left)
- ROR (rotate right)

>Multiply and divide instructions:

- MULH (multiply high)
- MULL (multiply low)
- DIV & MOD (divide and modulous)

>Bit manipulation instructions:

- SETBIT (set bit)
- CLEARBIT (clear bit)
- TOGGLEBIT (toggle bit)
- TESTBIT (test bit)

>String instructions:

- STRCMP (string compare)
- STRCPY (string copy)
- STRLEN (string length)

>System instructions:

- HALT (halt the system)
- ECHO (write to the console)
- INPUT (read from the console)

>Vector instructions:

- VADD r0, num //$decimal, \#Hexa, bBinary
- VSUB ''
- VMUL ''
- VDIV ''
- VNEG ''

> Operant layout and details:

-   Arithmetic instructions: ADD, SUB, MUL, DIV, NEG (2 source operands, 1 destination operand)
    
-   Logical instructions: AND, OR, NOT, XOR (2 source operands, 1 destination operand)
    
-   Data transfer instructions: LOAD, STORE, MOVE (1 source operand, 1 destination operand)
    
-   Control flow instructions: BUN (1 target address operand), BIZ (1 target address operand, source operand), BIN (1 target address operand, 1 source operand), RET (no operands)
    
-   Memory access instructions: MALLOC (1 destination operand, 1 size operand), DALLOC (1 source operand), LDA (1 source operand, 1 destination operand), STA (1 source operand, 1 destination operand)
    
-   Comparison instructions: EQ, NE, GT, GE, LT, LE (2 source operands, 1 destination operand)
    
-   Shift instructions: SHL, SHR (1 source operand, 1 destination operand, 1 shift amount operand)
    
-   Rotate instructions: ROL, ROR (1 source operand, 1 destination operand, 1 rotate amount operand)
    
-   Multiply and divide instructions: MULH, MULL, DIV, MOD (2 source operands, 1 destination operand)
    
-   Bit manipulation instructions: SETBIT, CLEARBIT, TOGGLEBIT, TESTBIT (1 source operand, 1 bit index operand)
    
-   String instructions: STRCMP, STRCPY, STRLEN (1 source operand, 1 destination operand)
    
-   System instructions: HALT (no operands), ECHO (1 source operand), INPUT (1 destination operand)

#Registers:

-   General purpose registers: The architecture includes 8 128-bit general purpose registers (R0-R7) and 8 32-bit general purpose float registers (F0-F7) for storing data and performing operations.
    
- Accumulator: The architecture includes a 128-bit accumulator register for storing intermediate results during operations.
    
-   Program counter (PC): The architecture includes a program counter register that stores the address of the current instruction being executed.
    
-   Stack pointer (SP): The architecture includes a stack pointer register that stores the address of the top of the stack.
    
-   Status registers: The architecture includes one or more status registers that store information about the current state of the processor, such as flags indicating the result of the last operation or interrupt enable/disable status.
    
-   Interrupt vectors: The architecture includes one or more interrupt vectors that store the addresses of interrupt service routines (ISRs) for handling interrupts.
    
-   Configuration registers: The architecture includes one or more configuration registers that store information about the configuration of the processor, such as the clock speed or memory size.
    
-   Debug registers: The architecture includes one or more debug registers that store information about the current state of the processor for debugging purposes, such as register values or the current instruction being executed.
    
-   Timer register: The architecture includes a 128-bit timer register for measuring time. The timer value, resolution, and mode are to be determined.
    
-   Segment Register SR: Stores the base addresses for the stack and heap segments.
	
-   Limit Register SR: Stores the final addresses for the stack and heap segments.

#Instructions #Layout :

-   Arithmetic instructions:
    
    -   ADD R1, R2, R3: Add the contents of register R2 to the contents of register R3 and store the result in register R1.
    -   SUB R4, R5, R6: Subtract the contents of register R6 from the contents of register R5 and store the result in register R4.
    
-   Logical instructions:
    
    -   AND R1, R2, R3: Perform a bitwise AND operation on the contents of register R2 and the contents of register R3 and store the result in register R1.
    -   OR R4, R5, R6: Perform a bitwise OR operation on the contents of register R5 and the contents of register R6 and store the result in register R4.
    -   NOT R7, R8: Perform a bitwise NOT operation on the contents of register R8 and store the result in register R7.
    -   XOR R9, R10, R11: Perform a bitwise XOR operation on the contents of register R10 and the contents of register R11 and store the result in register R9.
    
-   Control flow instructions:
    
    -   BUN LABEL: Unconditionally branch to the instruction at address LABEL.
    -   BIZ R1, LABEL: If the contents of register R1 are zero, branch to the instruction at address LABEL.
    -   BIN R2, LABEL: If the contents of register R2 are not zero, branch to the instruction at address LABEL.
    -   RET: Return from a function or interrupt service routine.
     
-   Memory access instructions:
    
    -   MALLOC R1, SIZE: Allocate SIZE bytes of memory and store the starting address in register R1.
    -   DALLOC R2: Deallocate the memory pointed to by the contents of register R2.
    -   LDA R3, \[MEM\]: Load the contents of memory address MEM into register R3.
    -   STA R4, \[MEM\]: Store the contents of register R4 into memory address MEM.
    
-   Comparison instructions:
    
    -   EQ R1, R2, R3: Compare the contents of register R2 and the contents of register R3 and set a flag in register R1 indicating if they are equal.
    -   NE R4, R5, R6: Compare the contents of register R5 and the contents of register R6 and set a flag in register R4 indicating if they are not equal.
    -   GT R7, R8, R9: Compare the contents of register R8 and the contents of register R9 and set a flag in register R7 indicating if R8 is greater than R9.
    -   GE R10, R11, R12: Compare the contents of register R11 and the contents of register R12 and set a flag in register R10 indicating if R11 is greater than or equal to R12.
    -   LT R13, R14, R15: Compare the contents of register R14 and the contents of register R15 and set a flag in register R13 indicating if R14 is less than R15.
    -   LE R16, R17, R18: Compare the contents of register R17 and the contents of register R18 and set a flag in register R16 indicating if R17 is less than or equal to R8
    
-   Shift instructions:
    
    -   SHL R1, R2, AMOUNT: Shift the contents of register R2 left by AMOUNT bits and store the result in register R1.
    -   SHR R3, R4, AMOUNT: Shift the contents of register R4 right by AMOUNT bits and store the result in register R3.
    
-   Rotate instructions:
    
    -   ROL R5, R6, AMOUNT: Rotate the contents of register R6 left by AMOUNT bits and store the result in register R5.
    -   ROR R7, R8, AMOUNT: Rotate the contents of register R8 right by AMOUNT bits and store the result in register R7.
    
-   Multiply and divide instructions:
    
    -   \[Note (AbduEhab)\] to impl MUL, DIV, and MOD we shall use this (inspired by RISC):

```
a = 60
b = 3
divide a b


divide a b:
c = 0
while a isn't less then b
a = a - b
c = c + 1

mod = -a
```

-   System instructions:
    
    -   HALT: Halt the processor.

#Instructions #encoding:

{ (1 indirect access bit) | (8 opcode bits) | (119 bits for memory access) }

#memory #Layout :

-   Interrupt segment: This segment could be used to store the code and data for handling interrupts in the system. It could be assigned a high level of protection to prevent unauthorized access or modification.
    
-   I/O segment: This segment could be used to store the code and data for interacting with external devices and peripherals. It could be assigned appropriate access permissions for the specific I/O operations being performed.
    
-   Heap segment: This segment could be used for dynamic memory allocation and deallocation. It could be implemented using a dynamic memory allocator such as a heap or free list to efficiently manage the available memory.
    
-   Stack segment: This segment could be used to store the stack for each process or task in the system. It could be implemented using a stack data structure to support fast push and pop operations.
    
-   Code segment: This segment could be used to store the code and data for the main program and other system components. It could be assigned a high level of protection to prevent unauthorized access or modification.

#Instructions #opcode :

```rust
// Arithmetic instructions

    ADD = 0x01, // Adds two operands and stores the result in the destination operand

    SUB = 0x02, // Subtracts two operands and stores the result in the destination operand

    AND = 0x03, // Performs bitwise AND on two operands and stores the result in the destination operand

    OR = 0x04,  // Performs bitwise OR on two operands and stores the result in the destination operand

    XOR = 0x05, // Performs bitwise XOR on two operands and stores the result in the destination operand

  

    // Data transfer instructions

    MOVE = 0x06, // Copies data from source operand to destination operand

    BUN = 0x07,  // Unconditionally branches to the target address operand

    BIZ = 0x08,  // Branches to the target address operand if the source operand is zero

    BIN = 0x09,  // Branches to the target address operand if the source operand is not zero

    RET = 0x0A,  // Returns from the current function call

  

    // Memory access instructions

    MALLOC = 0x0B, // Allocates memory on the heap with the size specified in the size operand

    DALLOC = 0x0C, // Deallocates memory at the address specified in the source operand

    LDA = 0x0D,    // Loads the data at the source operand into the destination operand

    STA = 0x0E,    // Stores the data from the source operand into the memory at the destination operand

  

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

    VOR = 0x1D,  // Performs bitwise OR on two vector operands and stores the result in the destination vector operand

    VXOR = 0x1E, // Performs bitwise XOR on two vector operands and stores the result in the destination vector operand

    VEQ = 0x1F,  // Compares two vector operands and sets a flag to 1 if they are equal, 0 otherwise

    VNE = 0x20,  // Compares two vector operands and sets a flag to 1 if they are not equal, 0 otherwise
```
