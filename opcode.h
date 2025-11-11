#pragma once
#include "state.h"

typedef enum {
    LDA    = 0x0, // Load into ACC
    STO    = 0x1, // Store from ACC
    ADD    = 0x2, // Add to ACC
    SUB    = 0x3, // Subtract from ACC
    JMP    = 0x4, // Set PC to address
    JGE    = 0x5, // Set PC if greater or equal
    JNE    = 0x6, // Set PC if zero not equal
    STP    = 0x7, // Stop
    CLL    = 0x8,
    RET    = 0x9,
    PSH    = 0xA,
    POP    = 0xB,
    LDR    = 0xC,
    STR    = 0xD,
    MOV_PC = 0xE, // Set PC to value of ACC
    MOV_SP = 0xF  // Set SP to value of ACC
} MU1Opcode;

mu1_word create_instruction_no_address(MU1Opcode opcode) {
    return ((mu1_word)opcode << 12); // Adresse ist 0
}

mu1_word create_instruction(MU1Opcode opcode, mu1_address address) {
    // OPCode 12 Bit nach links, dann bitweise ODER mit adresse zum kombinieren
    return (create_instruction_no_address(opcode) | (address & 0x0FFF)); 
}

mu1_address get_address_from_instruction(mu1_word instruction) {
    return (mu1_address)(instruction & 0x0FFF); // Letzte 12 Bits
}