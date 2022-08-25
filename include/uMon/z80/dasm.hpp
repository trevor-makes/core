// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

// 8080/Z80 (and even x86!) opcodes are organized by octal groupings
// http://z80.info/decoding.htm is a great resource for decoding these

#pragma once

#include "uMon/z80/common.hpp"
#include "uMon/format.hpp"

#include <stdint.h>

namespace uMon {
namespace z80 {

// Print given code as hex followed by '?'
template <typename API>
void print_prefix_error(uint8_t prefix, uint8_t code) {
  API::print_char('$');
  format_hex8(API::print_char, prefix);
  format_hex8(API::print_char, code);
  API::print_char('?');
}

// Convert 1-byte immediate at addr to Operand
template <typename API>
Operand read_imm_byte(uint16_t addr, bool is_indirect = false) {
  uint8_t token = TOK_IMMEDIATE | TOK_BYTE;
  if (is_indirect) token |= TOK_INDIRECT;
  uint16_t value = API::read_byte(addr);
  return { token, value };
}

// Convert 2-byte immediate at addr to Operand
template <typename API>
Operand read_imm_word(uint16_t addr, bool is_indirect = false) {
  uint8_t token = TOK_IMMEDIATE;
  if (is_indirect) token |= TOK_INDIRECT;
  uint8_t lsb = API::read_byte(addr);
  uint8_t msb = API::read_byte(addr + 1);
  uint16_t value = msb << 8 | lsb;
  return { token, value };
}

// Convert displacement byte at addr to Operand
template <typename API>
Operand read_branch_disp(uint16_t addr) {
  uint8_t token = TOK_IMMEDIATE;
  int8_t disp = API::read_byte(addr);
  uint16_t value = addr + 1 + disp;
  return { token, value };
}

// Encode (IX/IY+disp) given address of displacement byte
template <typename API>
Operand read_index_ind(uint16_t addr, uint8_t prefix) {
  uint8_t token = (prefix == PREFIX_IX ? TOK_IX : TOK_IY) | TOK_INDIRECT;
  uint16_t value = int8_t(API::read_byte(addr));
  return { token, value };
}

// Decode IN/OUT (c): ED [01 --- 00-]
uint8_t decode_in_out_c(Instruction& inst, uint8_t code) {
  const bool is_out = (code & 01) == 01;
  const uint8_t reg = (code & 070) >> 3;
  const bool is_ind = reg == REG_M;
  inst.mnemonic = is_out ? MNE_OUT : MNE_IN;
  inst.operands[is_out ? 0 : 1].token = TOK_C | TOK_INDIRECT;
  // NOTE reg (HL) is undefined; OUT sends 0 and IN sets flags without storing
  inst.operands[is_out ? 1 : 0].token = is_ind ? TOK_UNDEFINED : REG_TOK[reg];
  return 1;
}

// Decode 16-bit ADC/SBC: ED [01 --- 010]
uint8_t decode_hl_adc(Instruction& inst, uint8_t code) {
  const bool is_adc = (code & 010) == 010;
  const uint8_t pair = (code & 060) >> 4;
  inst.mnemonic = is_adc ? MNE_ADC : MNE_SBC;
  inst.operands[0].token = TOK_HL;
  inst.operands[1].token = PAIR_TOK[pair];
  return 1;
}

// Decode 16-bit LD ind: ED [01 --- 011]
template <typename API>
uint8_t decode_ld_pair_ind(Instruction& inst, uint16_t addr, uint8_t code) {
  const bool is_load = (code & 010) == 010;
  const uint8_t pair = (code & 060) >> 4;
  inst.mnemonic = MNE_LD;
  inst.operands[is_load ? 0 : 1].token = PAIR_TOK[pair];
  inst.operands[is_load ? 1 : 0] = read_imm_word<API>(addr + 1, true);
  return 3;
}

// Decode IM 0/1/2: ED [01 --- 110]
uint8_t decode_im(Instruction& inst, uint8_t code) {
  inst.mnemonic = MNE_IM;
  // NOTE only 0x46, 0x56, 0x5E are documented; '?' sets an undefined mode
  const uint8_t mode = (code & 030) >> 3;
  if (mode == 1) {
    inst.operands[0].token = TOK_UNDEFINED;
  } else {
    inst.operands[0].token = TOK_IMMEDIATE | TOK_DIGIT;
    inst.operands[0].value = mode > 0 ? mode - 1 : mode;
  }
  return 1;
}

// Decode LD I/R and RRD/RLD: ED [01 --- 111]
template <typename API>
uint8_t decode_ld_ir(Instruction& inst, uint8_t code) {
  const bool is_rot = (code & 040) == 040; // is RRD/RLD
  const bool is_load = (code & 020) == 020; // is LD A,I/R
  const bool is_rl = (code & 010) == 010; // is LD -R- or RLD
  if (is_rot) {
    if (is_load) {
      print_prefix_error<API>(PREFIX_ED, code);
    } else {
      inst.mnemonic = is_rl ? MNE_RLD : MNE_RRD;
    }
  } else {
    inst.mnemonic = MNE_LD;
    inst.operands[is_load ? 0 : 1].token = TOK_A;
    inst.operands[is_load ? 1 : 0].token = is_rl ? TOK_R : TOK_I;
  }
  return 1;
}

// Decode block transfer ops: ED [10 1-- 0--]
uint8_t decode_block_ops(Instruction& inst, uint8_t code) {
  static constexpr const uint8_t OPS[4][4] = {
    { MNE_LDI, MNE_LDD, MNE_LDIR, MNE_LDDR },
    { MNE_CPI, MNE_CPD, MNE_CPIR, MNE_CPDR },
    { MNE_INI, MNE_IND, MNE_INIR, MNE_INDR },
    { MNE_OUTI, MNE_OUTD, MNE_OTIR, MNE_OTDR },
  };
  const uint8_t op = (code & 03);
  const uint8_t var = (code & 030) >> 3;
  inst.mnemonic = OPS[op][var];
  return 1;
}

// Disassemble extended opcodes prefixed by $ED
template <typename API>
uint8_t decode_ed(Instruction& inst, uint16_t addr) {
  const uint8_t code = API::read_byte(addr);
  if ((code & 0300) == 0100) {
    switch (code & 07) {
    case 0: case 1:
      return decode_in_out_c(inst, code);
    case 2:
      return decode_hl_adc(inst, code);
    case 3:
      return decode_ld_pair_ind<API>(inst, addr, code);
    case 4:
      // NOTE all 1-4 codes do NEG, but only 104 is documented
      inst.mnemonic = MNE_NEG;
      return 1;
    case 5:
      // NOTE all 1-5 codes (except 115 RETI) do RETN, but only 105 is documented
      inst.mnemonic = code == 0115 ? MNE_RETI : MNE_RETN;
      return 1;
    case 6:
      return decode_im(inst, code);
    case 7:
      return decode_ld_ir<API>(inst, code);
    }
  } else if ((code & 0344) == 0240) {
    return decode_block_ops(inst, code);
  }
  print_prefix_error<API>(PREFIX_ED, code);
  return 1;
}

// Disassemble extended opcodes prefixed by $CB
template <typename API>
uint8_t decode_cb(Instruction& inst, uint16_t addr, uint8_t prefix) {
  const bool has_prefix = prefix != 0;
  // If prefixed, index displacement byte comes before opcode
  const uint8_t code = API::read_byte(has_prefix ? addr + 1 : addr);
  const uint8_t op = (code & 0300) >> 6;
  const uint8_t index = (code & 070) >> 3;
  const uint8_t reg = (code & 07);
  // Print opcode
  inst.mnemonic = op == CB_ROT ? ROT_MNE[index] : CB_MNE[op];
  Operand& reg_op = inst.operands[op == CB_ROT ? 0 : 1];
  // Print bit index (only for BIT/RES/SET)
  if (op != CB_ROT) {
    inst.operands[0].token = TOK_IMMEDIATE | TOK_DIGIT;
    inst.operands[0].value = index;
  }
  if (has_prefix) {
    if (op != CB_BIT && reg != REG_M) {
      // NOTE operand other than (HL) is undocumented
      // (IX/IY) is still used, but result also copied to reg
      print_pgm_string<API>(MNE_STR_LD);
      API::print_char(' ');
      print_pgm_table<API>(TOK_STR, REG_TOK[reg]);
      API::print_char(';');
    }
    // Print (IX/IY+disp)
    reg_op = read_index_ind<API>(addr, prefix);
    return 2;
  } else {
    // Print register operand
    reg_op.token = REG_TOK[reg];
    return 1;
  }
}

// Disassemble relative jumps: [00 --- 000]
template <typename API>
uint8_t decode_jr(Instruction& inst, uint16_t addr, uint8_t code) {
  switch (code & 070) {
  case 000:
    inst.mnemonic = MNE_NOP;
    return 1;
  case 010:
    inst.mnemonic = MNE_EX;
    inst.operands[0].token = TOK_AF;
    inst.operands[1].token = TOK_AF;
    return 1;
  case 020:
    inst.mnemonic = MNE_DJNZ;
    inst.operands[0] = read_branch_disp<API>(addr + 1);
    return 2;
  case 030:
    inst.mnemonic = MNE_JR;
    inst.operands[0] = read_branch_disp<API>(addr + 1);
    return 2;
  default:
    inst.mnemonic = MNE_JR;
    inst.operands[0].token = COND_TOK[(code & 030) >> 3];
    inst.operands[1] = read_branch_disp<API>(addr + 1);
    return 2;
  }
}

// Disassemble LD/ADD pair: [00 --- 001]
template <typename API>
uint8_t decode_ld_add_pair(Instruction& inst, uint16_t addr, uint8_t code, uint8_t prefix) {
  const bool is_load = (code & 010) == 0;
  const uint16_t pair = (code & 060) >> 4;
  if (is_load) {
    inst.mnemonic = MNE_LD;
    inst.operands[0].token = pair_to_token(pair, prefix);
    inst.operands[1] = read_imm_word<API>(addr + 1);
    return 3;
  } else {
    inst.mnemonic = MNE_ADD;
    inst.operands[0].token = pair_to_token(PAIR_HL, prefix);
    inst.operands[1].token = pair_to_token(pair, prefix);
    return 1;
  }
}

// Disassemble indirect loads: [00 --- 010]
template <typename API>
uint8_t decode_ld_ind(Instruction& inst, uint16_t addr, uint8_t code, uint8_t prefix) {
  // Decode 070 bitfield
  const bool is_store = (code & 010) == 0; // A/HL is src instead of dst
  const bool use_hl = (code & 060) == 040; // Use HL instead of A
  const bool use_pair = (code & 040) == 0; // Use (BC/DE) instead of (nn)
  Operand& op_reg = inst.operands[is_store ? 1 : 0];
  Operand& op_addr = inst.operands[is_store ? 0 : 1];
  // Convert instruction to tokens
  inst.mnemonic = MNE_LD;
  op_reg.token = use_hl ? pair_to_token(PAIR_HL, prefix) : TOK_A;
  if (use_pair) {
    op_addr.token = PAIR_TOK[(code & 020) >> 4] | TOK_INDIRECT;
  } else {
    op_addr = read_imm_word<API>(addr + 1, true);
  }
  // Opcodes followed by (nn) consume 2 extra bytes
  return use_pair ? 1 : 3;
}

// Disassemble LD r, n: ([ix/iy]) [00 r 110] ([d]) [n]
template <typename API>
uint8_t decode_ld_reg_imm(Instruction& inst, uint16_t addr, uint8_t code, uint8_t prefix) {
  const uint8_t reg = (code & 070) >> 3;
  const bool has_prefix = prefix != 0;
  inst.mnemonic = MNE_LD;
  if (has_prefix && reg == REG_M) {
    inst.operands[0] = read_index_ind<API>(addr + 1, prefix);
    inst.operands[1] = read_imm_byte<API>(addr + 2);
    return 3;
  } else {
    inst.operands[0].token = reg_to_token(reg, prefix);
    inst.operands[1] = read_imm_byte<API>(addr + 1);
    return 2;
  }
}

// Disassemble INC/DEC: [00 --- 011/100/101]
template <typename API>
uint8_t decode_inc_dec(Instruction& inst, uint16_t addr, uint8_t code, uint8_t prefix) {
  const bool is_pair = (code & 04) == 0;
  const bool is_inc = is_pair ? (code & 010) == 0 : (code & 01) == 0;
  inst.mnemonic = is_inc ? MNE_INC : MNE_DEC;
  if (is_pair) {
    const uint8_t pair = (code & 060) >> 4;
    inst.operands[0].token = pair_to_token(pair, prefix);
    return 1;
  } else {
    const bool has_prefix = prefix != 0;
    const uint8_t reg = (code & 070) >> 3;
    if (has_prefix && reg == REG_M) {
      // Replace (HL) with (IX/IY+disp)
      inst.operands[0] = read_index_ind<API>(addr + 1, prefix);
      return 2;
    } else {
      inst.operands[0].token = reg_to_token(reg, prefix);
      return 1;
    }
  }
}

// Decode LD r, r: [01 --- ---]
template <typename API>
uint8_t decode_ld_reg_reg(Instruction& inst, uint16_t addr, uint8_t code, uint8_t prefix) {
  // Replace LD (HL),(HL) with HALT
  if (code == 0x76) {
    inst.mnemonic = MNE_HALT;
    return 1;
  }
  inst.mnemonic = MNE_LD;
  const uint8_t dest = (code & 070) >> 3;
  const uint8_t src = (code & 07);
  // If (HL) used, replace with (IX/IY+disp)
  // Otherwise, replace H/L with IXH/IXL
  // NOTE the latter effect is undocumented!
  const bool has_prefix = prefix != 0;
  const bool has_dest_index = has_prefix && dest == REG_M;
  const bool has_src_index = has_prefix && src == REG_M;
  const bool has_index = has_dest_index || has_src_index;
  // Print destination register
  if (has_dest_index) {
    inst.operands[0] = read_index_ind<API>(addr + 1, prefix);
  } else {
    inst.operands[0].token = reg_to_token(dest, has_index ? 0 : prefix);
  }
  // Print source register
  if (has_src_index) {
    inst.operands[1] = read_index_ind<API>(addr + 1, prefix);
  } else {
    inst.operands[1].token = reg_to_token(src, has_index ? 0 : prefix);
  }
  // Skip displacement byte if (IX/IY+disp) is used
  return has_index ? 2 : 1;
}

// Decode [ALU op] A, r: [10 --- ---]
template <typename API>
uint8_t decode_alu_a_reg(Instruction& inst, uint16_t addr, uint8_t code, uint8_t prefix) {
  const uint8_t op = (code & 070) >> 3;
  const uint8_t reg = code & 07;
  const bool has_prefix = prefix != 0;
  inst.mnemonic = ALU_MNE[op];
  inst.operands[0].token = TOK_A;
  // Print operand reg
  if (has_prefix && reg == REG_M) {
    inst.operands[1] = read_index_ind<API>(addr + 1, prefix);
    return 2;
  } else {
    inst.operands[1].token = reg_to_token(reg, prefix);
    return 1;
  }
}

// Decode conditional RET/JP/CALL: [11 --- 000/010/100]
template <typename API>
uint8_t decode_jp_cond(Instruction& inst, uint16_t addr, uint8_t code) {
  static constexpr const uint8_t OPS[] = { MNE_RET, MNE_JP, MNE_CALL };
  const uint8_t op = (code & 06) >> 1;
  const uint8_t cond = (code & 070) >> 3;
  inst.mnemonic = OPS[op];
  inst.operands[0].token = COND_TOK[cond];
  if (op != 0) {
    inst.operands[1] = read_imm_word<API>(addr + 1);
    return 3;
  } else {
    return 1;
  }
}

// Decode PUSH/POP/CALL/RET and misc: [11 --- -01]
template <typename API>
uint8_t decode_push_pop(Instruction& inst, uint16_t addr, uint8_t code, uint8_t prefix) {
  const bool is_push = (code & 04) == 04;
  switch (code & 070) {
  case 010:
    if (is_push) {
      inst.mnemonic = MNE_CALL;
      inst.operands[0] = read_imm_word<API>(addr + 1);
      return 3;
    } else {
      inst.mnemonic = MNE_RET;
      return 1;
    }
  case 030:
    inst.mnemonic = MNE_EXX;
    return 1;
  case 050:
    inst.mnemonic = MNE_JP;
    inst.operands[0].token = pair_to_token(PAIR_HL, prefix) | TOK_INDIRECT;
    return 1;
  case 070:
    inst.mnemonic = MNE_LD;
    inst.operands[0].token = TOK_SP;
    inst.operands[1].token = pair_to_token(PAIR_HL, prefix);
    return 1;
  default: // 000, 020, 040, 060
    inst.mnemonic = is_push ? MNE_PUSH : MNE_POP;
    inst.operands[0].token = pair_to_token((code & 060) >> 4, prefix, true);
    return 1;
  }
}

// Decode misc ops with pattern [11 --- 011]
template <typename API>
uint8_t decode_misc_hi(Instruction& inst, uint16_t addr, uint8_t code, uint8_t prefix) {
  switch (code & 070) {
  case 000:
    inst.mnemonic = MNE_JP;
    inst.operands[0] = read_imm_word<API>(addr + 1);
    return 3;
  case 010:
    // Add 1 to size for prefix
    return 1 + decode_cb<API>(inst, addr + 1, prefix);
  case 020:
    inst.mnemonic = MNE_OUT;
    inst.operands[0] = read_imm_byte<API>(addr + 1, true);
    inst.operands[1].token = TOK_A;
    return 2;
  case 030:
    inst.mnemonic = MNE_IN;
    inst.operands[0].token = TOK_A;
    inst.operands[1] = read_imm_byte<API>(addr + 1, true);
    return 2;
  case 040:
    inst.mnemonic = MNE_EX;
    inst.operands[0].token = TOK_SP + TOK_INDIRECT;
    inst.operands[1].token = pair_to_token(PAIR_HL, prefix);
    return 1;
  case 050:
    // NOTE EX DE,HL unaffected by prefix
    inst.mnemonic = MNE_EX;
    inst.operands[0].token = TOK_DE;
    inst.operands[1].token = TOK_HL;
    return 1;
  case 060:
    inst.mnemonic = MNE_DI;
    return 1;
  default: // 070
    inst.mnemonic = MNE_EI;
    return 1;
  }
}

// Decode instruction at address, returning bytes read
template <typename API>
uint8_t dasm_instruction(Instruction& inst, uint16_t addr, uint8_t prefix = 0) {
  uint8_t code = API::read_byte(addr);
  // Handle prefix codes
  if (code == PREFIX_IX || code == PREFIX_ED || code == PREFIX_IY) {
    if (prefix != 0) {
      // Discard old prefix and start over
      print_prefix_error<API>(prefix, code);
      return 0;
    } else {
      // Add 1 to size for prefix
      if (code == PREFIX_ED) {
        return 1 + decode_ed<API>(inst, addr + 1);
      } else {
        return 1 + dasm_instruction<API>(inst, addr + 1, code);
      }
    }
  }
  // Decode by leading octal digit
  switch (code & 0300) {
  case 0000:
    // Decode by trailing octal digit
    switch (code & 07) {
    case 0:
      return decode_jr<API>(inst, addr, code);
    case 1:
      return decode_ld_add_pair<API>(inst, addr, code, prefix);
    case 2:
      return decode_ld_ind<API>(inst, addr, code, prefix);
    case 6:
      return decode_ld_reg_imm<API>(inst, addr, code, prefix);
    case 7:
      // Misc AF ops with no operands
      inst.mnemonic = MISC_MNE[(code & 070) >> 3];
      return 1;
    default: // 3, 4, 5
      return decode_inc_dec<API>(inst, addr, code, prefix);
    }
  case 0100:
    return decode_ld_reg_reg<API>(inst, addr, code, prefix);
  case 0200:
    return decode_alu_a_reg<API>(inst, addr, code, prefix);
  default: // 0300
    // Decode by trailing octal digit
    switch (code & 07) {
    case 3:
      return decode_misc_hi<API>(inst, addr, code, prefix);
    case 6:
      inst.mnemonic = ALU_MNE[(code & 070) >> 3];
      inst.operands[0].token = TOK_A;
      inst.operands[1] = read_imm_byte<API>(addr + 1);
      return 2;
    case 7:
      inst.mnemonic = MNE_RST;
      inst.operands[0].token = TOK_IMMEDIATE | TOK_BYTE;
      inst.operands[0].value = code & 070;
      return 1;
    default: // 0, 1, 2, 4, 5
      if ((code & 01) == 01) {
        return decode_push_pop<API>(inst, addr, code, prefix);
      } else {
        return decode_jp_cond<API>(inst, addr, code);
      }
    }
  }
}

template <typename API, uint8_t MAX_ROWS = 24>
uint16_t dasm_range(uint16_t addr, uint16_t end) {
  for (uint8_t i = 0; i < MAX_ROWS; ++i) {
    // If address has label, print it
    const char* label;
    if (API::get_labels().get_name(addr, label)) {
      API::print_string(label);
      API::print_char(':');
      API::newline();
    }

    // Print instruction address
    API::print_char(' ');
    format_hex16(API::print_char, addr);
    API::print_string("  ");

    // Translate machine code to mnemonic and operands for printing
    Instruction inst;
    uint8_t size = dasm_instruction<API>(inst, addr);
    if (inst.mnemonic != MNE_INVALID) {
      print_instruction<API>(inst);
    }
    API::newline();

    // Do while end does not overlap with opcode
    uint16_t prev = addr;
    addr += size;
    if (uint16_t(end - prev) < size) { break; }
  }
  return addr;
}

} // namespace z80
} // namespace uMon
