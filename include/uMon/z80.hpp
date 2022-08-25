// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

#include "z80/asm.hpp"
#include "z80/dasm.hpp"
#include "uCLI.hpp"

namespace uMon {
namespace z80 {

template <typename API>
bool parse_operand(Operand& op, uCLI::Tokens tokens) {
  // Handle indirect operand surronded by parentheses
  bool is_indirect = false;
  if (tokens.peek_char() == '(') {
    is_indirect = true;
    tokens.split_at('(');
    tokens = tokens.split_at(')');

    // Split optional displacement following +/-
    uCLI::Tokens disp_tok = tokens;
    bool is_minus = false;
    disp_tok.split_at('+');
    if (!disp_tok.has_next()) {
      disp_tok = tokens;
      disp_tok.split_at('-');
      is_minus = true;
    }

    // Parse displacement and apply sign
    uMON_OPTION_UINT(API, uint16_t, disp, 0, disp_tok, return false);
    op.value = is_minus ? -disp : disp;
  }

  // Parse operand as char, number, or token
  bool is_string = tokens.is_string();
  auto op_str = tokens.next();
  uint16_t value;
  if (is_string) {
    uMON_FMT_ERROR(API, strlen(op_str) > 1, "chr", op_str, return false);
    op.token = TOK_IMMEDIATE;
    op.value = op_str[0];
  } else if (API::get_labels().get_addr(op_str, value)) {
    op.token = TOK_IMMEDIATE;
    op.value = value;
  } else if (parse_unsigned(value, op_str)) {
    op.token = TOK_IMMEDIATE;
    op.value = value;
  } else {
    op.token = pgm_bsearch(TOK_STR, op_str);
    uMON_FMT_ERROR(API, op.token == TOK_INVALID, "arg", op_str, return false);
  }

  if (is_indirect) {
    op.token |= TOK_INDIRECT;
  }
  return true;
}

template <typename API>
bool parse_instruction(Instruction& inst, uCLI::Tokens args) {
  const char* mnemonic = args.next();
  inst.mnemonic = pgm_bsearch(MNE_STR, mnemonic);
  uMON_FMT_ERROR(API, inst.mnemonic == MNE_INVALID, "op", mnemonic, return false);

  // Parse operands
  for (Operand& op : inst.operands) {
    if (!args.has_next()) break;
    if (!parse_operand<API>(op, args.split_at(','))) return false;
  }

  // Error if unparsed operands remain
  uMON_FMT_ERROR(API, args.has_next(), "rem", args.next(), return false);
  return true;
}

template <typename API>
void cmd_asm(uCLI::Args args) {
  uMON_EXPECT_ADDR(API, uint16_t, start, args, return);

  // Parse and assemble instruction
  Instruction inst;
  if (parse_instruction<API>(inst, args)) {
    uint8_t size = asm_instruction<API>(inst, start);
    if (size > 0) {
      set_prompt<API>(args.command(), start + size);
    }
  }
}

template <typename API, uint8_t MAX_ROWS = 24>
void cmd_dasm(uCLI::Args args) {
  // Default size to one instruction if not provided
  uMON_EXPECT_ADDR(API, uint16_t, start, args, return);
  uMON_OPTION_UINT(API, uint16_t, size, 1, args, return);
  uint16_t end_incl = start + size - 1;
  uint16_t next = dasm_range<API, MAX_ROWS>(start, end_incl);
  uint16_t part = next - start;
  if (part < size) {
    set_prompt<API>(args.command(), next, size - part);
  } else {
    set_prompt<API>(args.command(), next);
  }
}

} // namespace z80
} // namespace uMon
