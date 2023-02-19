[[Chip Layout]]

#Instructions:
[[Memory Layout]]

>Arithmetic instructions:

- ADD r0, num //$decimal, \#Hexa, bBinary

>Logical instructions:

- AND
- OR
- NOT
- XOR


>Data transfer instructions:

- LDR
- STR
- MOV

>Control flow instructions:

- BUN
- BIZ
- BIN

>Memory access instructions:

- LDA
- STA

>Comparison instructions:

- EQL (equal)
- GT (greater than)

>Shift instructions:

- SHL (shift left)
- SHR (shift right)

>Rotate instructions:

- ROL (rotate left)
- ROR (rotate right)

>System instructions:

- HLT (halt the system)

> Operant layout and details:

-   Arithmetic instructions: ADD  (2 source operands, 1 destination operand)
    
-   Logical instructions: AND, OR,  XOR (2 source operands, 1 destination operand)
    
-   Data transfer instructions: LDR, STR, MOV (1 source operand, 1 destination operand)
    
-   Control flow instructions: BUN (1 target address operand), BIZ (1 target address operand, source operand), BIN (1 target address operand, 1 source operand)
    
-   Memory access instructions: LDA (1 source operand, 1 destination operand), STA (1 source operand, 1 destination operand)
    
-   Comparison instructions: EQ,  GT  (2 source operands, 1 destination operand)
    
-   Shift instructions: SHL, SHR (1 source operand, 1 destination operand, 1 shift amount operand) (might be turned into a compiler function instead)
    
-   Rotate instructions: ROL, ROR (1 source operand, 1 destination operand, 1 rotate amount operand) (might be turned into a compiler function instead)
     
-   System instructions: HALT (no operands)

#Registers:

-   General purpose registers: The architecture includes 8 128-bit general purpose registers (R0-R7) and 8 32-bit general purpose float registers (F0-F7) for storing data and performing operations. (could be removed, i still havent decided)
    
- Accumulator: The architecture includes a 32-bit accumulator register for storing current instructions.
    
-   Stack pointer (SP): The architecture includes a stack pointer register that stores the address of the top of the stack.
    
-   Status registers: The architecture includes one or more status registers that store information about the current state of the processor, such as flags indicating the result of the last operation or interrupt enable/disable status.
    
-   Interrupt vectors: The architecture includes one or more interrupt vectors that store the addresses of interrupt service routines (ISRs) for handling interrupts.
    
-   Configuration registers: The architecture includes one or more configuration registers that store information about the configuration of the processor, such as the clock speed or memory size.
    
-   Debug registers: The architecture includes one or more debug registers that store information about the current state of the processor for debugging purposes, such as register values or the current instruction being executed.
    
-   Timer register: The architecture includes a 128-bit timer register for measuring time. The timer value, resolution, and mode are to be determined.
    

#Instructions #Layout :

-   Arithmetic instructions:
    
    -   ADD R1, R2, R3: Add the contents of register R2 to the contents of register R3 and store the result in register R1.
    
-   Logical instructions:
    
    -   AND R1, R2, R3: Perform a bitwise AND operation on the contents of register R2 and the contents of register R3 and store the result in register R1.
    -   OR R4, R5, R6: Perform a bitwise OR operation on the contents of register R5 and the contents of register R6 and store the result in register R4.
    -   XOR R9, R10, R11: Perform a bitwise XOR operation on the contents of register R10 and the contents of register R11 and store the result in register R9.
    
-   Control flow instructions:
    
    -   BUN LABEL: Unconditionally branch to the instruction at address LABEL.
    -   BIZ R1, LABEL: If the contents of register R1 are zero, branch to the instruction at address LABEL.
    -   BIN R2, LABEL: If the contents of register R2 are negative, branch to the instruction at address LABEL.
     
-   Memory access instructions:
    
    -   LDA R3, \[MEM\]: Load the contents of memory address MEM into register R3.
    -   STA R4, \[MEM\]: Store the contents of register R4 into memory address MEM.
    
-   Comparison instructions:
    
    -   EQ R1, R2, R3: Compare the contents of register R2 and the contents of register R3 and set a flag in register R1 indicating if they are equal.
    -   GT R7, R8, R9: Compare the contents of register R8 and the contents of register R9 and set a flag in register R7 indicating if R8 is greater than R9.
    
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
