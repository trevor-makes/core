// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

#include "uMon/format.hpp"

namespace uMon {
namespace z80 {

constexpr const uint8_t PREFIX_IX = 0xDD;
constexpr const uint8_t PREFIX_IY = 0xFD;
constexpr const uint8_t PREFIX_ED = 0xED;
constexpr const uint8_t PREFIX_CB = 0xCB;

template <typename T, uint8_t N>
uint8_t index_of(const T (&table)[N], T value) {
  for (uint8_t i = 0; i < N; ++i) {
    if (table[i] == value) {
      return i;
    }
  }
  return N;
}

// ============================================================================
// Mnemonic Definitions
// ============================================================================

// Alphabetic index of assembly mnemonics
enum {
#define ITEM(x) MNE_##x,
#include "mnemonics.def"
#undef ITEM
  MNE_INVALID,
};

// Individual mnemonic strings in Flash memory
#define ITEM(x) const char MNE_STR_##x[] PROGMEM = #x;
#include "mnemonics.def"
#undef ITEM

// Flash memory table of mnemonic strings
const char* const MNE_STR[] PROGMEM = {
#define ITEM(x) MNE_STR_##x,
#include "mnemonics.def"
#undef ITEM
};

// ============================================================================
// ALU Encodings
// ============================================================================

#define ALU_LIST \
ITEM(ADD) ITEM(ADC) ITEM(SUB) ITEM(SBC) ITEM(AND) ITEM(XOR) ITEM(OR) ITEM(CP)

enum {
#define ITEM(x) ALU_##x,
ALU_LIST
#undef ITEM
};

// Mapping from ALU encoding to mnemonic
const uint8_t ALU_MNE[] = {
#define ITEM(x) MNE_##x,
ALU_LIST
#undef ITEM
};

#undef ALU_LIST

// ============================================================================
// CB-prefix OP Encodings
// ============================================================================

#define CB_LIST ITEM(BIT) ITEM(RES) ITEM(SET)

enum {
  CB_ROT, // Ops with additional 3-bit encoding
#define ITEM(x) CB_##x,
CB_LIST
#undef ITEM
};

// Mapping from CB op to mnemonic
const uint8_t CB_MNE[] = {
  MNE_INVALID,
#define ITEM(x) MNE_##x,
CB_LIST
#undef ITEM
};

#undef CB_LIST

// ============================================================================
// CB-prefix ROT Encodings
// ============================================================================

#define ROT_LIST \
ITEM(RLC) ITEM(RRC) ITEM(RL) ITEM(RR) ITEM(SLA) ITEM(SRA) ITEM(SL1) ITEM(SRL)

enum {
#define ITEM(x) ROT_##x,
ROT_LIST
#undef ITEM
};

// Mapping from ROT op to mnemonic
const uint8_t ROT_MNE[] = {
#define ITEM(x) MNE_##x,
ROT_LIST
#undef ITEM
};

#undef ROT_LIST

// ============================================================================
// Misc Encodings
// ============================================================================

#define MISC_LIST \
ITEM(RLCA) ITEM(RRCA) ITEM(RLA) ITEM(RRA) ITEM(DAA) ITEM(CPL) ITEM(SCF) ITEM(CCF)

enum {
#define ITEM(x) MISC_##x,
MISC_LIST
#undef ITEM
};

// Mapping from MISC op to mnemonic
const uint8_t MISC_MNE[] = {
#define ITEM(x) MNE_##x,
MISC_LIST
#undef ITEM
};

#undef MISC_LIST

// ============================================================================
// Token Definitions
// ============================================================================

// Alphabetic index of all registers, pairs, and conditions
enum {
  TOK_UNDEFINED,
#define ITEM(x) TOK_##x,
#include "tokens.def"
#undef ITEM
  TOK_INVALID,
  TOK_IMMEDIATE,
  TOK_MASK = 0x1F,
  TOK_BYTE = 0x20,
  TOK_DIGIT = 0x40,
  TOK_INDIRECT = 0x80,
  TOK_IMM_IND = TOK_IMMEDIATE | TOK_INDIRECT,
  TOK_BC_IND = TOK_BC | TOK_INDIRECT,
  TOK_DE_IND = TOK_DE | TOK_INDIRECT,
  TOK_HL_IND = TOK_HL | TOK_INDIRECT,
  TOK_SP_IND = TOK_SP | TOK_INDIRECT,
  TOK_IX_IND = TOK_IX | TOK_INDIRECT,
  TOK_IY_IND = TOK_IY | TOK_INDIRECT,
};

// Individual token strings in Flash memory
const char TOK_STR_UNDEFINED[] PROGMEM = "?";
#define ITEM(x) const char TOK_STR_##x[] PROGMEM = #x;
#include "tokens.def"
#undef ITEM

// Flash memory table of token strings
const char* const TOK_STR[] PROGMEM = {
  TOK_STR_UNDEFINED,
#define ITEM(x) TOK_STR_##x,
#include "tokens.def"
#undef ITEM
};

// Convert token to IX/IY prefix
uint8_t token_to_prefix(uint8_t token) {
  switch (token & TOK_MASK) {
  case TOK_IX: case TOK_IXH: case TOK_IXL:
    return PREFIX_IX;
  case TOK_IY: case TOK_IYH: case TOK_IYL:
    return PREFIX_IY;
  default:
    return 0;
  }
}

// ============================================================================
// Register Encodings
// ============================================================================

#define REG_LIST \
ITEM(B, TOK_B, TOK_B, TOK_B) \
ITEM(C, TOK_C, TOK_C, TOK_C) \
ITEM(D, TOK_D, TOK_D, TOK_D) \
ITEM(E, TOK_E, TOK_E, TOK_E) \
ITEM(H, TOK_H, TOK_IXH, TOK_IYH) \
ITEM(L, TOK_L, TOK_IXL, TOK_IYL) \
ITEM(M, TOK_HL_IND, TOK_IX_IND, TOK_IY_IND) \
ITEM(A, TOK_A, TOK_A, TOK_A)

enum {
#define ITEM(name, tok, tok_ix, tok_iy) REG_##name,
REG_LIST
#undef ITEM
  REG_INVALID,
};

// Mapping from reg encoding to token
const uint8_t REG_TOK[] = {
#define ITEM(name, tok, tok_ix, tok_iy) tok,
REG_LIST
#undef ITEM
};

// Mapping from reg encoding to token with IX prefix
const uint8_t REG_TOK_IX[] = {
#define ITEM(name, tok, tok_ix, tok_iy) tok_ix,
REG_LIST
#undef ITEM
};

// Mapping from reg encoding to token with IY prefix
const uint8_t REG_TOK_IY[] = {
#define ITEM(name, tok, tok_ix, tok_iy) tok_iy,
REG_LIST
#undef ITEM
};

// Convert token to primary register
uint8_t token_to_reg(uint8_t token, uint8_t prefix = 0) {
  switch (prefix) {
  case PREFIX_IX: return index_of(REG_TOK_IX, token);
  case PREFIX_IY: return index_of(REG_TOK_IY, token);
  default: return index_of(REG_TOK, token);
  }
}

// Translate reg to token, optionally with IX/IY prefix, or (HL)
// (IX/IY+disp) should be handled with read_index_ind instead
uint8_t reg_to_token(uint8_t reg, uint8_t prefix) {
  if (prefix != 0 && reg == REG_H) {
    return prefix == PREFIX_IX ? TOK_IXH : TOK_IYH;
  } else if (prefix != 0 && reg == REG_L) {
    return prefix == PREFIX_IX ? TOK_IXL : TOK_IYL;
  }
  return REG_TOK[reg];
}

#undef REG_LIST

// ============================================================================
// Register Pair Encodings
// ============================================================================

#define PAIR_LIST ITEM(BC) ITEM(DE) ITEM(HL) ITEM(SP)

enum {
#define ITEM(x) PAIR_##x,
PAIR_LIST
#undef ITEM
  PAIR_INVALID,
};

// Mapping from pair encoding to token
const uint8_t PAIR_TOK[] = {
#define ITEM(x) TOK_##x,
PAIR_LIST
#undef ITEM
};

// Convert token to register pair
uint8_t token_to_pair(uint8_t token, uint8_t prefix = 0, bool use_af = false) {
  if (prefix == PREFIX_IX) {
    if (token == TOK_IX) return PAIR_HL;
    if (token == TOK_HL) return PAIR_INVALID;
  } else if (prefix == PREFIX_IY) {
    if (token == TOK_IY) return PAIR_HL;
    if (token == TOK_HL) return PAIR_INVALID;
  }
  if (use_af) {
    if (token == TOK_AF) return PAIR_SP;
    if (token == TOK_SP) return PAIR_INVALID;
  }
  return index_of(PAIR_TOK, token);
}

// Translate pair to token, replacing HL with IX/IY if prefixed, and SP with AF if flagged
uint8_t pair_to_token(uint8_t pair, uint8_t prefix, bool use_af = false) {
  const bool has_prefix = prefix != 0;
  if (has_prefix && pair == PAIR_HL) {
    return prefix == PREFIX_IX ? TOK_IX : TOK_IY;
  } else if (use_af && pair == PAIR_SP) {
    return TOK_AF;
  }
  return PAIR_TOK[pair];
}

#undef PAIR_LIST

// ============================================================================
// Branch Condition Encodings
// ============================================================================

#define COND_LIST \
ITEM(NZ) ITEM(Z) ITEM(NC) ITEM(C) ITEM(PO) ITEM(PE) ITEM(P) ITEM(M)

enum {
#define ITEM(x) COND_##x,
COND_LIST
#undef ITEM
  COND_INVALID,
};

// Mapping from cond encoding to token
const uint8_t COND_TOK[] = {
#define ITEM(x) TOK_##x,
COND_LIST
#undef ITEM
};

// Convert token to branch condition
uint8_t token_to_cond(uint8_t token) {
  return index_of(COND_TOK, token);
}

#undef COND_LIST

// ============================================================================
// Token Functions
// ============================================================================

// Data fields common to all operand types
struct Operand {
  uint8_t token;
  uint16_t value;

  Operand(): token(TOK_INVALID), value(0) {}
  Operand(uint8_t token): token(token), value(0) {}
  Operand(uint8_t token, uint16_t value): token(token), value(value) {}
};

// Maximum number of operands encoded by an instruction
constexpr const uint8_t MAX_OPERANDS = 2;

// Data fields common to all instruction types
struct Instruction {
  uint8_t mnemonic;
  Operand operands[MAX_OPERANDS];

  Instruction(): mnemonic(MNE_INVALID) {}
  Instruction(uint8_t mnemonic): mnemonic(mnemonic) {}
  Instruction(uint8_t mnemonic, Operand op1): mnemonic(mnemonic), operands{op1, {}} {}
  Instruction(uint8_t mnemonic, Operand op1, Operand op2): mnemonic(mnemonic), operands{op1, op2} {}
};

// Nicely format an instruction operand
template <typename API>
void print_operand(Operand& op) {
  const bool is_indirect = (op.token & TOK_INDIRECT) != 0;
  const bool is_byte = (op.token & TOK_BYTE) != 0;
  const bool is_digit = (op.token & TOK_DIGIT) != 0;
  const uint8_t token = op.token & TOK_MASK;
  if (is_indirect) API::print_char('(');
  if (token < TOK_INVALID) {
    print_pgm_table<API>(TOK_STR, token);
    if (op.value != 0) {
      int8_t value = op.value;
      API::print_char(value < 0 ? '-' : '+');
      API::print_char('$');
      format_hex8(API::print_char, value < 0 ? -value : value);
    }
  } else if (token == TOK_IMMEDIATE) {
    if (is_digit) {
      API::print_char('0' + op.value);
    } else if (is_byte) {
      API::print_char('$');
      format_hex8(API::print_char, op.value);
    } else {
      const char* label;
      if (API::get_labels().get_name(op.value, label)) {
        API::print_string(label);
      } else {
        API::print_char('$');
        format_hex16(API::print_char, op.value);
      }
    }
  } else {
    API::print_char('?');
  }
  if (is_indirect) API::print_char(')');
}

// Nicely format an instruction and its operands
template <typename API>
void print_instruction(Instruction& inst) {
  if (inst.mnemonic == MNE_INVALID) {
    API::print_char('?');
    return;
  }
  print_pgm_table<API>(MNE_STR, inst.mnemonic);
  for (uint8_t i = 0; i < MAX_OPERANDS; ++i) {
    Operand& op = inst.operands[i];
    if (op.token == TOK_INVALID) break;
    API::print_char(i == 0 ? ' ' : ',');
    print_operand<API>(op);
  }
}

} // namespace z80
} // namespace uMon
