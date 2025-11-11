#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "state.h"

bool is_word_negative(mu1_word word) {
    const uint16_t SIGN_BIT = 0x8000; // Alles 0 außer die erste Ziffer (16-bit -> 1000 0000 0000 0000)
    return ((uint16_t)word & SIGN_BIT) != 0; // Bitweise AND, wenn bit 1 -> negativ
}

void alu_add(MU1State *state, mu1_word value) {
    bool isValueNeg = is_word_negative(value);
    bool isAccNegBefore = is_word_negative(state->ACC);
    state->ACC = (state->ACC + value) & 0xFFFF; // Simulate 16-bit overflow
    state->Z = (state->ACC == 0);
    state->N = is_word_negative(state->ACC);
    bool isAccNegAfter = is_word_negative(state->ACC);
    // Overflow, falls beide Operanden gleiches Vorzeichen haben und Ergebnis anderes Vorzeichen
    state->V = (isValueNeg == isAccNegBefore) && (isAccNegAfter != isValueNeg);
    // Carry, falls Ergebnis kleiner als einer der Operanden ist
    state->C = (state->ACC < value);
}

void alu_sub(MU1State *state, mu1_word value) {
    alu_add(state, (~value + 1)); // Negation durch Zweierkomplement
}

void alu_load(MU1State *state, mu1_word value) {
    state->ACC = value;
    state->Z = (state->ACC == 0);
    state->N = ((state->ACC & 0x8000) != 0);
}