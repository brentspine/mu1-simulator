#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

typedef uint16_t mu1_word;
typedef uint16_t mu1_address; // 12-bit address space
#include "opcode.h"

#define MEMORY_SIZE 4096 // 2^12

typedef enum {
    MU1_PHASE_FETCH_0,
    MU1_PHASE_FETCH_1,
    MU1_PHASE_DECODE,
    MU1_PHASE_EXECUTE,
    MU1_PHASE_LDA_0,
    MU1_PHASE_STO_0,
    MU1_PHASE_ADD_0,
    MU1_PHASE_SUB_0,
    MU1_PHASE_JMP_0,
    MU1_PHASE_JGE_0,
    MU1_PHASE_JNE_0,
    MU1_PHASE_STP_0,
    MU1_PHASE_CLL_0,
    MU1_PHASE_RET_0,
    MU1_PHASE_PSH_0,
    MU1_PHASE_POP_0,
    MU1_PHASE_LDR_0,
    MU1_PHASE_STR_0,
    MU1_PHASE_MOV_PC_0,
    MU1_PHASE_MOV_SP_0
} MU1Phase;



typedef struct {
    mu1_word IR;
    mu1_word DIN;

    mu1_word ACC;
    mu1_word SP;
    mu1_address PC;

    mu1_word DOUT;

    mu1_word memory[MEMORY_SIZE];
    bool halted;
    MU1Phase phase;
    mu1_word operand;
    bool Z;
    bool N;
    bool V; // Overflow flag
    bool C; // Carry flag
} MU1State;

void mu1_init(MU1State *state) {
    memset(state, 0, sizeof(*state));
    state->SP = 0x00FF;
    state->PC = 0x0000;
    state->phase = MU1_PHASE_FETCH_0;
    state->halted = 0;
}

void mu1_print_state(const MU1State *state) {
    printf("PC: %04X, IR: %04X, ACC: %04X, SP: %04X, Phase: %d, Z: %d, N: %d, V: %d, C: %d\n",
           state->PC, state->IR, state->ACC, state->SP, state->phase, state->Z, state->N, state->V, state->C);
}

void print_word_as_uint16(mu1_word word) {
    printf("%u", (uint16_t)word);
}

void print_word_as_int16(mu1_word word) {
    printf("%d", (int16_t)word);
}

void mu1_address_lookup(MU1State *state, mu1_address address) {
    if (address < MEMORY_SIZE) {
        state->DIN = state->memory[address];
    } else {
        // Handle invalid address access
        state->DIN = 0;
        printf("Invalid memory address %04X\n", address);
    }
}

void mu1_set_address(MU1State *state, mu1_address address, mu1_word value) {
    if (address < MEMORY_SIZE) {
        state->memory[address] = value;
    } else {
        printf("Invalid memory address %04X\n", address);
    }
}

void push_stack(MU1State *state, mu1_word value) {
    state->memory[state->SP] = value;
    state->SP = (state->SP - 1); // Might cause undeflow 
}

mu1_word pop_stack(MU1State *state) {
    state->SP = (state->SP + 1); 
    return state->memory[state->SP];
}