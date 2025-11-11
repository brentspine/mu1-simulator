#include <stdio.h>
#include <stdint.h>
#include "state.h"
#include "alu.h"

MU1State state;
void mu1_step(MU1State *state);
static void mu1_set_phase_from_opcode(MU1State *state, MU1Opcode opcode);

#define PRINT_FETCH false
#define PRINT_DECODE false

#define LOOP 0x0000
#define ONE 0xFFC
#define TOTAL 0xFFD
#define COUNT 0xFFE
#define PTR 0xFFF

int main(void) {
    MU1State mu1;
    mu1_init(&mu1);

    mu1.memory[LOOP] = create_instruction(LDR, PTR); // Acc = Wert der Adresse in PTR
    mu1.memory[LOOP + 1] = create_instruction(ADD, TOTAL); // Acc += Total
    mu1.memory[LOOP + 2] = create_instruction(STO, TOTAL); // Total = Acc
    mu1.memory[LOOP + 3] = create_instruction(LDA, PTR); // Acc = Addresswert ptr
    mu1.memory[LOOP + 4] = create_instruction(ADD, ONE); // Acc += 1
    mu1.memory[LOOP + 5] = create_instruction(STO, PTR); // Speichern des neuen Wertes
    mu1.memory[LOOP + 6] = create_instruction(LDA, COUNT); // Acc = Count
    mu1.memory[LOOP + 7] = create_instruction(SUB, ONE); // Acc -= 1
    mu1.memory[LOOP + 8] = create_instruction(STO, COUNT); // Speichern des neuen Wertes
    mu1.memory[LOOP + 9] = create_instruction(JNE, 0x0000); // Springe zurück zum Anfang, wenn Count != 0
    mu1.memory[LOOP + 10] = create_instruction_no_address(STP); // Wird erreicht, wenn ACC/Count == 0

    for(int i = LOOP+11; i <= LOOP + 15; ++i) {
        mu1.memory[i] = i;
    }
    mu1.memory[ONE] = 1; // Number One
    mu1.memory[TOTAL] = 0; // Total
    mu1.memory[COUNT] = 0x5; // Count
    mu1.memory[PTR] = LOOP+11; // ptr

    int skippedSteps = 0;
    for (int i = 0; i < 10000 && !mu1.halted; ++i) {
        bool printState = true;
        if(mu1.phase == MU1_PHASE_FETCH_0 || mu1.phase == MU1_PHASE_FETCH_1) printState = PRINT_FETCH;
        else if(mu1.phase == MU1_PHASE_DECODE) printState = PRINT_DECODE;

        if(printState) {
            if(skippedSteps > 0)
                printf("\n(Es wurden %d Mikro-Schritte ohne Ausgabe übersprungen)\n", skippedSteps);
            skippedSteps = 0;
            printf("\n=== Mikro-Schritt %d ===\n", i);
            printf("State vor Schritt: ");
            mu1_print_state(&mu1);
            mu1_step(&mu1);
            printf("State nach Schritt: ");
            mu1_print_state(&mu1);
        } else {
            mu1_step(&mu1);
            skippedSteps++;
        }
    }

    printf("\nErgebnis in memory[TOTAL]: ");
    print_word_as_uint16(mu1.memory[TOTAL]);
    printf(" (signed: ");
    print_word_as_int16(mu1.memory[TOTAL]);
    printf(")\n");

    return 0;
}

void mu1_step(MU1State *state) {
    switch (state->phase) {

    case MU1_PHASE_FETCH_0: {
        mu1_word addr = state->PC;

        if(PRINT_FETCH) {
            printf("[FETCH_0] PC=%04X -> Adressbus, MemReq=1, RnW=1\n", addr);
            printf("           (lese Instruktion aus memory[%04X])\n", addr);
        }
        state->phase = MU1_PHASE_FETCH_1;
        break;
    }

    case MU1_PHASE_FETCH_1: {
        mu1_word addr = state->PC;
        mu1_word instr = state->memory[addr];

        state->IR = instr;
        state->PC = (state->PC + 1); // Kann zu Overflow führen

        if(PRINT_FETCH) {
            printf("[FETCH_1] IR := memory[%04X] = %04X, PC := %04X\n",
               addr, instr, state->PC);
        }

        state->phase = MU1_PHASE_DECODE;
        break;
    }

    case MU1_PHASE_DECODE: {
        mu1_word instr = state->IR;
        MU1Opcode opcode = (MU1Opcode)(instr >> 12);
        mu1_word operand = instr;
        if(PRINT_DECODE) {
            printf("[DECODE] IR=%04X -> opcode=%X, operand=%03X\n",
               instr, opcode, operand);
        }
        state->operand = operand;
        mu1_set_phase_from_opcode(state, opcode);
        break;
    }

    case MU1_PHASE_LDA_0: {
        mu1_address addr = (mu1_address)(state->operand & 0x0FFF);
        printf("[LDA_0] Adresse=%03X -> Adressbus, MemReq=1, RnW=1\n", addr);
        mu1_address_lookup(state, addr);
        alu_load(state, state->DIN);
        printf("[LDA_0] ACC := memory[%03X] = %04X\n", addr, state->ACC);
        state->phase = MU1_PHASE_FETCH_0;
        break;
    }

    case MU1_PHASE_STO_0: {
        mu1_address addr = (mu1_address)(state->operand & 0x0FFF);
        printf("[STO_0] Adresse=%03X -> Adressbus, DOUT := ACC=%04X, MemReq=1, RnW=0\n",
               addr, state->ACC);
        state->DOUT = state->ACC;
        state->memory[addr] = state->DOUT;
        printf("[STO_0] memory[%03X] := DOUT = %04X\n", addr, state->DOUT);
        state->phase = MU1_PHASE_FETCH_0;
        break;
    }

    case MU1_PHASE_ADD_0: {
        mu1_address addr = (mu1_address)(state->operand & 0x0FFF);
        printf("[ADD_0] Adresse=%03X -> Adressbus, MemReq=1, RnW=1\n", addr);
        mu1_address_lookup(state, addr);
        mu1_word value = state->DIN;
        alu_add(state, value);
        printf("[ADD_0] ACC := ACC + memory[%03X] = %04X + %04X = %04X\n",
               addr, state->ACC - value, value, state->ACC);
        state->phase = MU1_PHASE_FETCH_0;
        break;
    }

    case MU1_PHASE_SUB_0: {
        mu1_address addr = (mu1_address)(state->operand & 0x0FFF);
        printf("[SUB_0] Adresse=%03X -> Adressbus, MemReq=1, RnW=1\n", addr);
        mu1_address_lookup(state, addr);
        mu1_word value = state->DIN;
        alu_sub(state, value);
        printf("[SUB_0] ACC := ACC - memory[%03X] = %04X - %04X = %04X\n",
               addr, state->ACC + value, value, state->ACC);
        state->phase = MU1_PHASE_FETCH_0;
        break;
    }

    case MU1_PHASE_JMP_0: {
        mu1_address addr = (mu1_address)(state->operand & 0x0FFF);
        printf("[JMP_0] PC := Adresse %03X\n", addr);
        state->PC = addr;
        state->phase = MU1_PHASE_FETCH_0;
        break;
    }

    case MU1_PHASE_JGE_0: {
        mu1_address addr = (mu1_address)(state->operand & 0x0FFF);
        if(!state->N) {
            printf("[JGE_0] Bedingung erfüllt (N=%d), PC := Adresse %03X\n", state->N, addr);
            state->PC = addr;
        } else {
            printf("[JGE_0] Bedingung nicht erfüllt (N=%d), kein Sprung\n", state->N);
        }
        state->phase = MU1_PHASE_FETCH_0;
        break;
    }

    case MU1_PHASE_JNE_0: {
        mu1_address addr = (mu1_address)(state->operand & 0x0FFF);
        if(!state->Z) {
            printf("[JNE_0] Bedingung erfüllt (Z=%d), PC := Adresse %03X\n", state->Z, addr);
            state->PC = addr;
        } else {
            printf("[JNE_0] Bedingung nicht erfüllt (Z=%d), kein Sprung\n", state->Z);
        }
        state->phase = MU1_PHASE_FETCH_0;
        break;
    }

    case MU1_PHASE_PSH_0: {
        push_stack(state, state->ACC);
        printf("[PSH_0] SP := %04X, memory[%04X] := ACC = %04X\n",
               state->SP + 1, state->SP + 1, state->ACC);
        state->phase = MU1_PHASE_FETCH_0;
        break;
    }

    case MU1_PHASE_POP_0: {
        state->SP = (state->SP + 1); // Kann zu Overflow führen
        mu1_word value = state->memory[state->SP];
        alu_load(state, value);
        printf("[POP_0] ACC := memory[%04X] = %04X, SP := %04X\n",
               state->SP, value, state->SP);
        state->phase = MU1_PHASE_FETCH_0;
        break;
    }

    case MU1_PHASE_CLL_0: {
        push_stack(state, state->PC);
        state->PC = state->operand;
        printf("[CLL_0] Push PC=%04X auf Stack, SP := %04X, PC := %04X\n",
               state->PC, state->SP + 1, state->PC);
        state->phase = MU1_PHASE_FETCH_0;
        break;
    }

    case MU1_PHASE_RET_0: {
        state->PC = pop_stack(state);
        printf("[RET_0] Pop Adresse %04X von Stack in PC, SP := %04X\n",
               state->PC, state->SP);
        state->phase = MU1_PHASE_FETCH_0;
        break;
    }

    case MU1_PHASE_MOV_PC_0: { // PC = ACC
        state->PC = state->ACC;
        printf("[MOV_PC_0] PC := ACC = %04X\n", state->ACC);
        state->phase = MU1_PHASE_FETCH_0;
        break;
    }

    case MU1_PHASE_MOV_SP_0: { // SP = ACC
        state->SP = state->ACC;
        printf("[MOV_SP_0] SP := ACC = %04X\n", state->ACC);
        state->phase = MU1_PHASE_FETCH_0;
        break;
    }

    case MU1_PHASE_LDR_0: { // ACC = [[S]]
        mu1_address addr = (mu1_address)(state->operand & 0x0FFF);
        mu1_address_lookup(state, addr); // DIN = [S]
        mu1_address_lookup(state, state->DIN); // DIN = [DIN]
        alu_load(state, state->DIN);
        printf("[LDR_0] ACC := memory[memory[%04X]] = memory[%04X] = %04X\n",
               addr, state->DIN, state->ACC);
        state->phase = MU1_PHASE_FETCH_0;
        break;
    }

    case MU1_PHASE_STR_0: { // [[S]] = ACC
        mu1_address addr = (mu1_address)(state->operand & 0x0FFF); // S
        mu1_address_lookup(state, addr); // DIN = [S]
        mu1_set_address(state, state->DIN, state->ACC); // [DIN] = ACC
        printf("[STR_0] memory[%04X] := ACC = %04X\n", state->DIN, state->ACC);
        state->phase = MU1_PHASE_FETCH_0;
        break;
    }

    case MU1_PHASE_STP_0: {
        printf("[STP_0] Stoppe die Ausführung.\n");
        state->halted = 1;
        break;
    }

    default:
        printf("[WARN] Unbekannte Phase\n");
        state->halted = 1;
        break;
    }
}

static void mu1_set_phase_from_opcode(MU1State *state, MU1Opcode opcode) {
    switch (opcode) {
    case LDA:
        state->phase = MU1_PHASE_LDA_0;
        return;
    case STO:
        state->phase = MU1_PHASE_STO_0;
        return;
    case ADD:
        state->phase = MU1_PHASE_ADD_0;
        return;
    case SUB:
        state->phase = MU1_PHASE_SUB_0;
        return;
    case STP:
        state->phase = MU1_PHASE_STP_0;
        return;
    case JMP:
        state->phase = MU1_PHASE_JMP_0;
        return;
    case JGE:
        state->phase = MU1_PHASE_JGE_0;
        return;
    case JNE:
        state->phase = MU1_PHASE_JNE_0;
        return;
    case CLL:
        state->phase = MU1_PHASE_CLL_0;
        return;
    case RET:
        state->phase = MU1_PHASE_RET_0;
        return;
    case PSH:
        state->phase = MU1_PHASE_PSH_0;
        return;
    case POP:
        state->phase = MU1_PHASE_POP_0;
        return;
    case LDR:
        state->phase = MU1_PHASE_LDR_0;
        return;
    case STR:
        state->phase = MU1_PHASE_STR_0;
        return;
    case MOV_PC:
        state->phase = MU1_PHASE_MOV_PC_0;
        return;
    case MOV_SP:
        state->phase = MU1_PHASE_MOV_SP_0;
        return;
    default:
        printf("[DECODE] Unbekannter Opcode %X, stoppe.\n", opcode);
        state->halted = true;
        return;
    }
}

bool isWordNegative(mu1_word word) {
    return (word & 0x8000) != 0;
}
