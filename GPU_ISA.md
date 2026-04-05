# FIAT128 GPU ISA

This document defines the fixed GPU instruction set for the FIAT128 emulator.

The design goal is a small shader ISA that can be executed by all GPU cores in parallel. The CPU uploads a shader program into GPU RAM, sets a start byte, and the GPU clears that byte when execution completes.

## Execution Model

- GPU state lives in memory module 3.
- Byte 0 of GPU RAM is the control byte.
- The CPU starts execution by writing `0xFF` to GPU RAM byte 0.
- The GPU clears byte 0 back to `0x00` when the shader pass finishes.
- Each GPU invocation executes the same shader over a distinct invocation id.
- The framebuffer is 400 by 600 logical pixels.

## Program Layout

The current runtime expects this RAM3 layout:

- word 0: control byte in the low 8 bits
- word 1: primary constant, typically the fill color
- word 2: reserved
- word 3 and onward: shader bytecode

The first executable instruction is at word 3.

## Instruction Encoding

Each GPU instruction uses the low 64 bits of a RAM word.

| Bits | Field |
| --- | --- |
| 0-7 | opcode |
| 8-15 | destination register |
| 16-23 | source register 1 |
| 24-31 | source register 2 |
| 32-63 | immediate value |
| 64-127 | reserved |

Register indices are modulo 16.

## Opcode Table

| Opcode | Value | Meaning |
| --- | --- | --- |
| `load` | 0 | Load a GPU RAM word into a register using the immediate address |
| `store` | 1 | Store a register into GPU RAM using the immediate address |
| `pixel_store` | 2 | Write a register value to the current invocation pixel |
| `add` | 3 | Scalar add |
| `sub` | 4 | Scalar subtract |
| `mul` | 5 | Scalar multiply |
| `div` | 6 | Scalar divide |
| `mod` | 7 | Scalar modulo |
| `neg` | 8 | Scalar negate |
| `abs` | 9 | Scalar absolute value |
| `dot` | 10 | Dot product of two 3-register vectors |
| `cross` | 11 | Cross product of two 3-register vectors |
| `length` | 12 | Vector length |
| `normalize` | 13 | Normalize a 3-register vector |
| `lerp` | 14 | Linear interpolation |
| `clamp` | 15 | Clamp a scalar |
| `eq` | 16 | Equal comparison |
| `ne` | 17 | Not-equal comparison |
| `lt` | 18 | Less-than comparison |
| `le` | 19 | Less-or-equal comparison |
| `gt` | 20 | Greater-than comparison |
| `ge` | 21 | Greater-or-equal comparison |
| `and` | 22 | Bitwise and |
| `or` | 23 | Bitwise or |
| `xor` | 24 | Bitwise xor |
| `not` | 25 | Bitwise not |
| `shl` | 26 | Shift left |
| `shr` | 27 | Shift right |
| `jmp` | 28 | Unconditional jump to the immediate address |
| `jz` | 29 | Jump if destination register is zero |
| `jnz` | 30 | Jump if destination register is nonzero |
| `halt` | 31 | Stop the current invocation |
| `read_invocation_id_x` | 32 | Load the invocation x coordinate |
| `read_invocation_id_y` | 33 | Load the invocation y coordinate |
| `read_width` | 34 | Load the framebuffer width |
| `read_height` | 35 | Load the framebuffer height |
| `pack_rgb` | 36 | Pack RGB channels into a 24-bit color |
| `pack_rgba` | 37 | Pack RGBA channels into a 32-bit color |
| `unpack_rgb` | 38 | Extract a packed RGB color |
| `unpack_rgba` | 39 | Extract a packed RGBA color |

## Shader Inputs

Each invocation gets these implicit values:

- `invocation_x`: the pixel x coordinate
- `invocation_y`: the pixel y coordinate
- `frame_width`: 400 (variable)
- `frame_height`: 600 (variable)
- `gpu_ram`: module 3 memory

## Shader Outputs

The shader may write directly to its pixel in the framebuffer using a dedicated pixel-store instruction.

The preferred behavior for a pixel-store is:

- compute the pixel color in the shader
- write it to the current invocation pixel

## Data Model

The first words of GPU RAM are reserved for shader parameters.

Suggested layout:

- word 0: control byte in the low 8 bits
- word 1: shader color or primary constant
- word 2+: optional shader parameters for future extensions

## Core Instruction Families

### Memory

- `load` from GPU RAM
- `store` to GPU RAM
- `pixel_store` to the current invocation pixel

### Arithmetic

- `add`
- `sub`
- `mul`
- `div`
- `mod`
- `neg`
- `abs`

### Vector Math

- `dot`
- `cross`
- `length`
- `normalize`
- `lerp`
- `clamp`

### Comparison

- `eq`
- `ne`
- `lt`
- `le`
- `gt`
- `ge`

### Bitwise

- `and`
- `or`
- `xor`
- `not`
- `shl`
- `shr`

### Control Flow

- `jmp`
- `jz`
- `jnz`
- `halt`

### Invocation and Frame Queries

- `read_invocation_id_x`
- `read_invocation_id_y`
- `read_width`
- `read_height`

### Packing Helpers

- `pack_rgb`
- `pack_rgba`
- `unpack_rgb`
- `unpack_rgba`

## Minimum Viable Shader

The first shader to support is a solid fill shader.

Required behavior:

- read the color value from GPU RAM word 1
- write that color to every pixel with `pixel_store`
- clear the control byte when finished

## Recommended Opcode Groups

A small initial opcode set is enough to implement useful shaders:

| Group | Opcodes |
| --- | --- |
| Memory | `load`, `store`, `pixel_store` |
| Arithmetic | `add`, `sub`, `mul`, `div` |
| Math | `dot`, `cross`, `length`, `normalize` |
| Comparison | `eq`, `lt`, `gt` |
| Control | `jmp`, `jz`, `jnz`, `halt` |
| GPU context | `read_invocation_id_x`, `read_invocation_id_y`, `read_width`, `read_height` |
| Packing | `pack_rgb`, `pack_rgba` |

## Notes

- Avoid adding synchronization or atomics until a shader needs shared-memory coordination.
- Keep the first version deterministic and side-effect free except for framebuffer writes and GPU RAM reads/writes.
- Favor explicit pixel writes over hidden rendering behavior.
