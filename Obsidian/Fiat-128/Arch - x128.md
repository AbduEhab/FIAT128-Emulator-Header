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

#Instructions #Layout :

-   Arithmetic instructions:
    
    -   ADD R1, R2, R3: Add the contents of register R2 to the contents of register R3 and store the result in register R1.
    -   SUB R4, R5, R6: Subtract the contents of register R6 from the contents of register R5 and store the result in register R4.
    -   MUL R7, R8, R9: Multiply the contents of register R8 and the contents of register R9 and store the result in register R7.
    -   DIV R10, R11, R12: Divide the contents of register R11 by the contents of register R12 and store the result in register R10.
    -   NEG R13, R14: Negate the contents of register R14 and store the result in register R13.
-   Logical instructions:
    
    -   AND R1, R2, R3: Perform a bitwise AND operation on the contents of register R2 and the contents of register R3 and store the result in register R1.
    -   OR R4, R5, R6: Perform a bitwise OR operation on the contents of register R5 and the contents of register R6 and store the result in register R4.
    -   NOT R7, R8: Perform a bitwise NOT operation on the contents of register R8 and store the result in register R7.
    -   XOR R9, R10, R11: Perform a bitwise XOR operation on the contents of register R10 and the contents of register R11 and store the result in register R9.
-   Data transfer instructions:
    
    -   LOAD R1, \[MEM\]: Load the contents of memory address MEM into register R1.
    -   STORE R2, \[MEM\]: Store the contents of register R2 into memory address MEM.
    
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
    
    -   MULH R9, R10, R11: Multiply the contents of register R10 and the contents of register R11 and store the high 128 bits of the result in register R9.
    -   MULL R12, R13, R14: Multiply the contents of register R13 and the contents of register R14 and store the low 128 bits of the result in register R12.
    -   DIV R15, R16, R17: Divide the contents of register R16 by the contents of register R17 and store the result in register R15.
    -   MOD R18, R19, R20: Calculate the remainder of dividing the contents of register R19 by the contents of register R20 and store the result in register R18.
    
-   Bit manipulation instructions:
    
    -   SETBIT R1, BIT: Set the BITth bit of the contents of register R1 to 1.
    -   CLEARBIT R2, BIT: Set the BITth bit of the contents of register R2 to 0.
    -   TOGGLEBIT R3, BIT: Toggle the BITth bit of the contents of register R3.
    -   TESTBIT R4, BIT: Test the BITth bit of the contents of register R4 and set a flag indicating the result.
    
-   String instructions:
    
    -   STRCMP R1, R2, R3: Compare the null-terminated strings pointed to by the contents of register R2 and the contents of register R3 and set a flag in register R1 indicating the result.
    -   STRCPY R4, R5, R6: Copy the null-terminated string pointed to by the contents of register R5 to the memory location pointed to by the contents of register R6.
    -   STRLEN R7, R8: Calculate the length of the null-terminated string pointed to by the contents of register R8 and store the result in register R7.
    
-   System instructions:
    
    -   HALT: Halt the processor.
    -   ECHO R1: Output the contents of register R1 to the console.
    -   INPUT R2: Input a value from the console and store it in register R2.

#Instructions #encoding:

