#pragma once

/*
 * Register assignment on x86/x86-64 platform
 * ATN Machine scope
 *  CONTEXT_REG     MM2/R8
 *  INPUT_REG       ESI/RSI
 *  INPUT_BASE_REG  EBP/RBP
 *  OUTPUT_REG      EDI/RDI
 *  OUTPUT_BOUND    EDX/RDX
 *  Stack backup    MM3/R9
 * ATN Machine scope (Chaser mode)
 *  CONTEXT_REG     MM2/R8
 *  INPUT_REG       ESI/RSI
 * ATN Machine scope (marker writing)
 *  MARKER_REG      EAX/RAX
 * DFA Routine scope
 *  BACKUP_REG      EBX/RBX
 *  CHAR_REG        EAX/RAX
 *  CHAR2_REG       ECX/RCX
 *  INDEX_REG       ECX/RCX
 *  LOAD_REG        XMM0
 *  PATTERN2_REG    XMM2
 * LDFA Routine scope
 *  PEEK_REG        EBX/RBX
 *  CHAR_REG        EAX/RAX
 *  CHAR2_REG       ECX/RCX
 * Match Routine scope
 *  LOAD_REG        XMM0
 *  INDEX_REG       ECX/RCX
 *  CHAR_REG        EAX/RAX
 * Skip Routine scope
 *  LOAD_REG        XMM0
 *  PATTERN_REG     XMM1
 *  INDEX_REG       ECX/RCX
 * Chaser Routine scope
 *  CONTEXT_REG		MM2/R8
 *  INPUT_REG		ESI/RSI
 *  CHECKPOINT_REG  EDI/RDI
 */

//Parser/Machine scope registers
#define CONTEXT_REG asmjit::x86::r8
#define INPUT_REG asmjit::x86::rsi
#define OUTPUT_REG asmjit::x86::rdi
#define OUTPUT_BOUND_REG asmjit::x86::rdx
#define INPUT_BASE_REG asmjit::x86::rbp
#define MARKER_REG asmjit::x86::rax
#define ID_REG asmjit::x86::rbx
#define STACK_BACKUP_REG asmjit::x86::r9

//DFA/LDFA routine scope registers
#define BACKUP_REG asmjit::x86::rbx
#define PEEK_REG asmjit::x86::rbx
#define CHAR_REG asmjit::x86::rax
#define CHAR2_REG asmjit::x86::rcx
#define PATTERN2_REG asmjit::x86::xmm2

//Match/Skip routine scope registers
#define LOAD_REG asmjit::x86::xmm0
#define PATTERN_REG asmjit::x86::xmm1
#define INDEX_REG asmjit::x86::rcx

//Chaser routine scope registers
#define CHECKPOINT_REG asmjit::x86::rdi

#if defined(CENTAURUS_BUILD_WINDOWS)
#define ARG1_REG asmjit::x86::rcx
#define ARG2_REG asmjit::x86::rdx
#define ARG3_REG asmjit::x86::r8
#define ARG4_REG asmjit::x86::r9
#define ARG1_STACK_OFFSET (8)
#define ARG2_STACK_OFFSET (16)
#define ARG3_STACK_OFFSET (48)
#define ARG4_STACK_OFFSET (56)
#elif defined(CENTAURUS_BUILD_LINUX)
#define ARG1_REG asmjit::x86::rdi
#define ARG2_REG asmjit::x86::rsi
#define ARG3_REG asmjit::x86::rdx
#define ARG4_REG asmjit::x86::rcx
#define ARG1_STACK_OFFSET (32)
#define ARG2_STACK_OFFSET (24)
#define ARG3_STACK_OFFSET (16)
#define ARG4_STACK_OFFSET (8)
#endif

