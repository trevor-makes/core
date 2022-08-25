// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

#include "uMon/z80/common.hpp"
#include "uMon/format.hpp"
#include "uCLI.hpp"

#include <stdint.h>

namespace uMon {
namespace z80 {

template <typename API>
void print_operand_error(Operand& op) {
  print_operand<API>(op);
  API::print_char('?');
  API::newline();
}

// Write [code] at address and return bytes written
template <typename API>
uint8_t write_code(uint16_t addr, uint8_t code) {
  API::write_byte(addr, code);
  return 1;
}

// Write [code, data] at address and return bytes written
template <typename API>
uint8_t write_code_byte(uint16_t addr, uint8_t code, uint8_t data) {
  API::write_byte(addr, code);
  API::write_byte(addr + 1, data);
  return 2;
}

// Write [(prefix,) code] at address and return bytes written
template <typename API>
uint8_t write_pfx_code(uint16_t addr, uint8_t prefix, uint8_t code) {
  bool has_prefix = prefix != 0;
  if (has_prefix) API::write_byte(addr++, prefix);
  return has_prefix + write_code<API>(addr, code);
}

// Write [(prefix,) code (,index)] at address and return bytes written
template <typename API>
uint8_t write_pfx_code_idx(uint16_t addr, uint8_t prefix, uint8_t code, Operand& index) {
  bool has_index = index.token == TOK_IX_IND || index.token == TOK_IY_IND;
  uint8_t size = write_pfx_code<API>(addr, prefix, code);
  if (has_index) API::write_byte(addr + size, index.value);
  return has_index + size;
}

// Write [code, lsb, msb] at address and return bytes written
template <typename API>
uint8_t write_code_word(uint16_t addr, uint8_t code, uint16_t data) {
  API::write_byte(addr, code);
  API::write_byte(addr + 1, data & 0xFF);
  API::write_byte(addr + 2, data >> 8);
  return 3;
}

// Write [(prefix,) code, lsb, msb] at address and return bytes written
template <typename API>
uint8_t write_pfx_code_word(uint16_t addr, uint8_t prefix, uint8_t code, uint16_t data) {
  bool has_prefix = prefix != 0;
  if (has_prefix) API::write_byte(addr++, prefix);
  return has_prefix + write_code_word<API>(addr, code, data);
}

// Write A arithmetic instruction at address
template <typename API>
uint8_t write_alu_a(uint16_t addr, uint8_t alu, Operand& src) {
  // ALU A,n
  if (src.token == TOK_IMMEDIATE) {
    uint8_t code = 0306 | alu << 3;
    return write_code_byte<API>(addr, code, src.value);
  // ALU A,r
  } else {
    // Validate source operand is register
    uint8_t prefix = token_to_prefix(src.token);
    uint8_t reg = token_to_reg(src.token, prefix);
    if (reg == REG_INVALID) {
      print_operand_error<API>(src);
      return 0;
    }
    uint8_t code = 0200 | alu << 3 | reg;
    return write_pfx_code_idx<API>(addr, prefix, code, src);
  }
}

// Write HL arithmetic instruction at address
template <typename API>
uint8_t write_alu_hl(uint16_t addr, uint8_t alu, Operand& dst, Operand& src) {
  // Validate dst operand is HL/IX/IY pair
  uint8_t prefix = token_to_prefix(dst.token);
  if (token_to_pair(dst.token, prefix) != PAIR_HL) {
    print_operand_error<API>(dst);
    return 0;
  }
  // Validate src operand is pair with same prefix
  uint8_t src_pair = token_to_pair(src.token, prefix);
  if (src_pair == PAIR_INVALID) {
    print_operand_error<API>(src);
    return 0;
  }
  // ADD HL,rr
  if (alu == ALU_ADD) {
    uint8_t code = 0011 | src_pair << 4;
    return write_pfx_code<API>(addr, prefix, code);
  // ADC HL,rr
  } else if (prefix == 0 && alu == ALU_ADC) {
    uint8_t code = 0112 | src_pair << 4;
    return write_pfx_code<API>(addr, PREFIX_ED, code);
  // SBC HL,rr
  } else if (prefix == 0 && alu == ALU_SBC) {
    uint8_t code = 0102 | src_pair << 4;
    return write_pfx_code<API>(addr, PREFIX_ED, code);
  // ?
  } else {
    print_operand_error<API>(dst);
    return 0;
  }
}

// Write arithmetic instruction at address
template <typename API>
uint8_t write_alu(uint16_t addr, uint8_t alu, Operand& op1, Operand& op2) {
  if (op2.token == TOK_INVALID) {
    return write_alu_a<API>(addr, alu, op1);
  } else if (op1.token == TOK_A) {
    return write_alu_a<API>(addr, alu, op2);
  } else {
    return write_alu_hl<API>(addr, alu, op1, op2);
  }
}

// Write CB prefix instruction at address
template <typename API>
uint8_t write_cb_code(uint16_t addr, uint8_t code, Operand& op) {
  // Validate register
  uint8_t prefix = token_to_prefix(op.token);
  uint8_t reg = token_to_reg(op.token, prefix);
  if (reg == REG_INVALID || (prefix != 0 && reg != REG_M)) {
    print_operand_error<API>(op);
    return 0;
  }
  if (prefix != 0) {
    // NOTE index comes before code with double prefix
    API::write_byte(addr, prefix);
    API::write_byte(addr + 1, PREFIX_CB);
    API::write_byte(addr + 2, op.value);
    API::write_byte(addr + 3, code | reg);
    return 4;
  } else {
    return write_pfx_code<API>(addr, PREFIX_CB, code | reg);
  }
}

// Write rotate/shift instruction at address
template <typename API>
uint8_t write_cb_rot(uint16_t addr, uint8_t rot, Operand& op) {
  uint8_t code = rot << 3;
  return write_cb_code<API>(addr, code, op);
}

// Write BIT/RES/SET instruction at address
template <typename API>
uint8_t write_cb_bit(uint16_t addr, uint8_t cb, Operand& op1, Operand& op2) {
  // Validate bit index
  if (op1.token != TOK_IMMEDIATE || op1.value > 7) {
    print_operand_error<API>(op1);
    return 0;
  }
  uint8_t code = cb << 6 | op1.value << 3;
  return write_cb_code<API>(addr, code, op2);
}

// Write CALL/JP instruction at address
template <typename API>
uint8_t write_call_jp(uint16_t addr, uint8_t code_cc, uint8_t code_nn, Operand& op1, Operand& op2) {
  // CALL/JP cc,nn
  uint8_t cond = token_to_cond(op1.token);
  if (cond != COND_INVALID && op2.token == TOK_IMMEDIATE) {
    uint8_t code = code_cc | cond << 3;
    return write_code_word<API>(addr, code, op2.value);
  // CALL/JP nn
  } else if (op1.token == TOK_IMMEDIATE) {
    return write_code_word<API>(addr, code_nn, op1.value);
  // ?
  } else {
    print_operand_error<API>(op1);
    return 0;
  }
}

// Write CALL instruction at address
template <typename API>
uint8_t write_call(uint16_t addr, Operand& op1, Operand& op2) {
  uint8_t code_cc = 0304; // CALL cc,nn
  uint8_t code_nn = 0315; // CALL nn
  return write_call_jp<API>(addr, code_cc, code_nn, op1, op2);
}

// Write JP instruction at address
template <typename API>
uint8_t write_jp(uint16_t addr, Operand& op1, Operand& op2) {
  // JP (HL/IX/IY)
  uint8_t prefix = token_to_prefix(op1.token);
  uint8_t reg = token_to_reg(op1.token, prefix);
  if (reg == REG_M) {
    return write_pfx_code<API>(addr, prefix, 0xE9);
  }
  uint8_t code_cc = 0302; // JP cc,nn
  uint8_t code_nn = 0303; // JP nn
  return write_call_jp<API>(addr, code_cc, code_nn, op1, op2);
}

// Write INC/DEC instruction at address
template <typename API>
uint8_t write_inc_dec(uint16_t addr, uint8_t code_r, uint8_t code_rr, Operand& op) {
  uint8_t prefix = token_to_prefix(op.token);
  uint8_t reg = token_to_reg(op.token, prefix);
  uint8_t pair = token_to_pair(op.token, prefix);
  // INC/DEC r
  if (reg != REG_INVALID) {
    uint8_t code = code_r | reg << 3;
    return write_pfx_code_idx<API>(addr, prefix, code, op);
  // INC/DEC rr
  } else if (pair != PAIR_INVALID) {
    uint8_t code = code_rr | pair << 4;
    return write_pfx_code<API>(addr, prefix, code);
  // ?
  } else {
    print_operand_error<API>(op);
    return 0;
  }
}

// Write INC instruction at address
template <typename API>
uint8_t write_inc(uint16_t addr, Operand& op) {
  uint8_t code_r = 0004; // INC r
  uint8_t code_rr = 0003; // INC rr
  return write_inc_dec<API>(addr, code_r, code_rr, op);
}

// Write DEC instruction at address
template <typename API>
uint8_t write_dec(uint16_t addr, Operand& op) {
  uint8_t code_r = 0005; // DEC r
  uint8_t code_rr = 0013; // DEC rr
  return write_inc_dec<API>(addr, code_r, code_rr, op);
}

// Write EX instruction at address
template <typename API>
uint8_t write_ex(uint16_t addr, Operand& op1, Operand& op2) {
  // EX (SP),HL
  if (op1.token == (TOK_SP | TOK_INDIRECT)) {
    uint8_t prefix = token_to_prefix(op2.token);
    if (token_to_pair(op2.token, prefix) != PAIR_HL) {
      print_operand_error<API>(op2);
      return 0;
    }
    return write_pfx_code<API>(addr, prefix, 0xE3);
  // EX DE,HL
  } else if (op1.token == TOK_DE && op2.token == TOK_HL) {
    return write_code<API>(addr, 0xEB);
  // EX AF,AF
  } else if (op1.token == TOK_AF && (op2.token == TOK_AF || op2.token == TOK_INVALID)) {
    return write_code<API>(addr, 0x08);
  // ?
  } else {
    print_operand_error<API>(op1);
    return 0;
  }
}

// Write IM instruction at address
template <typename API>
uint8_t write_im(uint16_t addr, Operand& op) {
  // IM 0/1/2
  if (op.token == TOK_IMMEDIATE && op.value < 3) {
    static constexpr const uint8_t IM[] = { 0x46, 0x56, 0x5E };
    return write_pfx_code<API>(addr, PREFIX_ED, IM[op.value]);
  // IM ?
  } else if (op.token == TOK_UNDEFINED) {
    return write_pfx_code<API>(addr, PREFIX_ED, 0x4E);
  // ?
  } else {
    print_operand_error<API>(op);
    return 0;
  }
}

// Write IN/OUT instruction at address
template <typename API>
uint8_t write_in_out(uint16_t addr, uint8_t code_an, uint8_t code_rc, Operand& data, Operand& port) {
  // IN A,(n) / OUT (n),A
  if (data.token == TOK_A && port.token == TOK_IMM_IND) {
    return write_code_byte<API>(addr, code_an, port.value);
  // IN r,(C) / OUT (C),r
  } else if (port.token == (TOK_C | TOK_INDIRECT)) {
    uint8_t reg = token_to_reg(data.token);
    if (reg == REG_INVALID || reg == REG_M) {
      print_operand_error<API>(data);
      return 0;
    }
    uint8_t code = code_rc | reg << 3;
    return write_pfx_code<API>(addr, PREFIX_ED, code);
  // ?
  } else {
    print_operand_error<API>(port);
    return 0;
  }
}

// Write IN instruction at address
template <typename API>
uint8_t write_in(uint16_t addr, Operand& data, Operand& port) {
  uint8_t code_an = 0333; // IN A,(n)
  uint8_t code_rc = 0100; // IN r,(C)
  return write_in_out<API>(addr, code_an, code_rc, data, port);
}

// Write OUT instruction at address
template <typename API>
uint8_t write_out(uint16_t addr, Operand& port, Operand& data) {
  uint8_t code_an = 0323; // OUT (n),A
  uint8_t code_rc = 0101; // OUT (C),r
  return write_in_out<API>(addr, code_an, code_rc, data, port);
}

// Write DJNZ/JR instruction at address
template <typename API>
uint8_t write_djnz_jr(uint16_t addr, uint8_t code, Operand& op) {
  // DJNZ/JR imm16
  int16_t disp = op.value - (addr + 2);
  if (op.token != TOK_IMMEDIATE) {
    print_operand_error<API>(op);
    return 0;
  }
  if (disp < -128 || disp > 127) {
    API::print_string("too far");
    API::newline();
    return 0;
  }
  return write_code_byte<API>(addr, code, disp);
}

// Write DJNZ instruction at address
template <typename API>
uint8_t write_djnz(uint16_t addr, Operand& op) {
  return write_djnz_jr<API>(addr, 0x10, op);
}

// Write JR instruction at address
template <typename API>
uint8_t write_jr(uint16_t addr, Operand& op1, Operand& op2) {
  // JR nn
  if (op2.token == TOK_INVALID) {
    return write_djnz_jr<API>(addr, 0x18, op1);
  // JR cc,nn
  } else {
    uint8_t cond = token_to_cond(op1.token);
    if (cond > 3) {
      print_operand_error<API>(op1);
      return 0;
    }
    uint8_t code = 0040 | cond << 3;
    return write_djnz_jr<API>(addr, code, op2);
  }
}

// Write LD instruction at address
template <typename API>
uint8_t write_ld(uint16_t addr, Operand& dst, Operand& src) {
  // Special cases for destination A
  if (dst.token == TOK_A) {
    switch (src.token) {
    case TOK_I: // LD A,I
      return write_pfx_code<API>(addr, PREFIX_ED, 0x57);
    case TOK_R: // LD A,R
      return write_pfx_code<API>(addr, PREFIX_ED, 0x5F);
    case TOK_BC_IND: // LD A,(BC)
      return write_code<API>(addr, 0x0A);
    case TOK_DE_IND: // LD A,(DE)
      return write_code<API>(addr, 0x1A);
    case TOK_IMM_IND: // LD A,(nn)
      return write_code_word<API>(addr, 0x3A, src.value);
    }
  }

  // Special cases for source A
  if (src.token == TOK_A) {
    switch (dst.token) {
    case TOK_I: // LD I,A
      return write_pfx_code<API>(addr, PREFIX_ED, 0x47);
    case TOK_R: // LD R,A
      return write_pfx_code<API>(addr, PREFIX_ED, 0x4F);
    case TOK_BC_IND: // LD (BC),A
      return write_code<API>(addr, 0x02);
    case TOK_DE_IND: // LD (DE),A
      return write_code<API>(addr, 0x12);
    case TOK_IMM_IND: // LD (nn),A
      return write_code_word<API>(addr, 0x32, dst.value);
    }
  }

  // Special cases for destination HL/IX/IY
  uint8_t dst_prefix = token_to_prefix(dst.token);
  uint8_t dst_pair = token_to_pair(dst.token, dst_prefix);
  // LD HL/IX/IY,(nn)
  if (dst_pair == PAIR_HL && src.token == TOK_IMM_IND) {
    return write_pfx_code_word<API>(addr, dst_prefix, 0x2A, src.value);
  }

  // Special cases for source HL/IX/IY
  uint8_t src_prefix = token_to_prefix(src.token);
  uint8_t src_pair = token_to_pair(src.token, src_prefix);
  if (src_pair == PAIR_HL) {
    // LD (nn),HL/IX/IY
    if (dst.token == TOK_IMM_IND) {
      return write_pfx_code_word<API>(addr, src_prefix, 0x22, dst.value);
    // LD SP,HL/IX/IY
    } else if (dst.token == TOK_SP) {
      return write_pfx_code<API>(addr, src_prefix, 0xF9);
    }
  }

  // Catch-all cases for any register/pair
  uint8_t dst_reg = token_to_reg(dst.token, dst_prefix);
  if (dst_reg != REG_INVALID) {
    // LD r,r
    uint8_t src_reg = token_to_reg(src.token, src_prefix);
    if (src_reg != REG_INVALID) {
      bool src_is_m = src_reg == REG_M;
      bool dst_is_m = dst_reg == REG_M;
      bool dst_in_src = token_to_reg(dst.token, src_prefix) != REG_INVALID;
      bool src_in_dst = token_to_reg(src.token, dst_prefix) != REG_INVALID;
      // Validate reg-reg permutation
      // - only one can be (HL/IX/IY) and other can't be IXH/IXL/IYH/IYL
      // - H/L, IXH/IXL, IYH/IYL can't be mixed; prefix affects both regs
      if ((src_is_m && !dst_is_m && dst_prefix == 0)
        || (dst_is_m && !src_is_m && src_prefix == 0)
        || (!src_is_m && !dst_is_m && (dst_in_src || src_in_dst))) {
        uint8_t prefix = dst_prefix | src_prefix;
        uint8_t code = 0100 | dst_reg << 3 | src_reg;
        Operand& index = dst_reg == REG_M ? dst : src;
        return write_pfx_code_idx<API>(addr, prefix, code, index);
      }
    // LD r,n
    } else if (src.token == TOK_IMMEDIATE) {
      uint8_t code = 0006 | dst_reg << 3;
      uint8_t size = write_pfx_code_idx<API>(addr, dst_prefix, code, dst);
      API::write_byte(addr + size, src.value);
      return size + 1;
    }
  } else if (dst_pair != PAIR_INVALID) {
    // LD rr,nn
    if (src.token == TOK_IMMEDIATE) {
      uint8_t code = 0001 | dst_pair << 4;
      return write_pfx_code_word<API>(addr, dst_prefix, code, src.value);
    // LD rr,(nn)
    } else if (src.token == TOK_IMM_IND) {
      // NOTE LD HL/IX/IY,(nn) already handled by special case; only BC/DE/SP
      uint8_t code = 0113 | dst_pair << 4;
      return write_pfx_code_word<API>(addr, PREFIX_ED, code, src.value);
    }
  // LD (nn),rr
  } else if (src_pair != PAIR_INVALID && dst.token == TOK_IMM_IND) {
    // NOTE LD (nn),HL/IX/IY already handled by special case; only BC/DE/SP
    uint8_t code = 0103 | src_pair << 4;
    return write_pfx_code_word<API>(addr, PREFIX_ED, code, dst.value);
  }
  // ?
  print_operand_error<API>(src);
  return 0;
}

// Write PUSH/POP instruction at address
template <typename API>
uint8_t write_push_pop(uint16_t addr, uint8_t code, Operand& op) {
  uint8_t prefix = token_to_prefix(op.token);
  uint8_t pair = token_to_pair(op.token, prefix, true);
  if (pair == PAIR_INVALID) {
    print_operand_error<API>(op);
    return 0;
  }
  return write_pfx_code<API>(addr, prefix, code | pair << 4);
}

// Write PUSH instruction at address
template <typename API>
uint8_t write_push(uint16_t addr, Operand& op) {
  return write_push_pop<API>(addr, 0305, op);
}

// Write POP instruction at address
template <typename API>
uint8_t write_pop(uint16_t addr, Operand& op) {
  return write_push_pop<API>(addr, 0301, op);
}

// Write RET instruction at address
template <typename API>
uint8_t write_ret(uint16_t addr, Operand& op) {
  // RET
  if (op.token == TOK_INVALID) {
    return write_code<API>(addr, 0xC9);
  // RET cc
  } else {
    uint8_t cond = token_to_cond(op.token);
    if (cond == COND_INVALID) {
      print_operand_error<API>(op);
      return 0;
    }
    return write_code<API>(addr, 0300 | cond << 3);
  }
}

// Write RST instruction at address
template <typename API>
uint8_t write_rst(uint16_t addr, Operand& op) {
  if (op.token == TOK_IMMEDIATE && (op.value & 0307) == 0) {
    return write_code<API>(addr, 0307 | op.value);
  } else {
    print_operand_error<API>(op);
    return 0;
  }
}

template <typename API>
uint8_t asm_instruction(Instruction& inst, uint16_t addr) {
  Operand& op1 = inst.operands[0];
  Operand& op2 = inst.operands[1];
  switch (inst.mnemonic) {
  case MNE_ADC:
    return write_alu<API>(addr, ALU_ADC, op1, op2);
  case MNE_ADD:
    return write_alu<API>(addr, ALU_ADD, op1, op2);
  case MNE_AND:
    return write_alu<API>(addr, ALU_AND, op1, op2);
  case MNE_BIT:
    return write_cb_bit<API>(addr, CB_BIT, op1, op2);
  case MNE_CALL:
    return write_call<API>(addr, op1, op2);
  case MNE_CCF:
    return write_code<API>(addr, 0x3F);
  case MNE_CP:
    return write_alu<API>(addr, ALU_CP, op1, op2);
  case MNE_CPD:
    return write_pfx_code<API>(addr, PREFIX_ED, 0xA9);
  case MNE_CPDR:
    return write_pfx_code<API>(addr, PREFIX_ED, 0xB9);
  case MNE_CPI:
    return write_pfx_code<API>(addr, PREFIX_ED, 0xA1);
  case MNE_CPIR:
    return write_pfx_code<API>(addr, PREFIX_ED, 0xB1);
  case MNE_CPL:
    return write_code<API>(addr, 0x2F);
  case MNE_DAA:
    return write_code<API>(addr, 0x27);
  case MNE_DEC:
    return write_dec<API>(addr, op1);
  case MNE_DI:
    return write_code<API>(addr, 0xF3);
  case MNE_DJNZ:
    return write_djnz<API>(addr, op1);
  case MNE_EI:
    return write_code<API>(addr, 0xFB);
  case MNE_EX:
    return write_ex<API>(addr, op1, op2);
  case MNE_EXX:
    return write_code<API>(addr, 0xD9);
  case MNE_HALT:
    return write_code<API>(addr, 0x76);
  case MNE_IM:
    return write_im<API>(addr, op1);
  case MNE_IN:
    return write_in<API>(addr, op1, op2);
  case MNE_INC:
    return write_inc<API>(addr, op1);
  case MNE_IND:
    return write_pfx_code<API>(addr, PREFIX_ED, 0xAA);
  case MNE_INDR:
    return write_pfx_code<API>(addr, PREFIX_ED, 0xBA);
  case MNE_INI:
    return write_pfx_code<API>(addr, PREFIX_ED, 0xA2);
  case MNE_INIR:
    return write_pfx_code<API>(addr, PREFIX_ED, 0xB2);
  case MNE_JP:
    return write_jp<API>(addr, op1, op2);
  case MNE_JR:
    return write_jr<API>(addr, op1, op2);
  case MNE_LD:
    return write_ld<API>(addr, op1, op2);
  case MNE_LDD:
    return write_pfx_code<API>(addr, PREFIX_ED, 0xA8);
  case MNE_LDDR:
    return write_pfx_code<API>(addr, PREFIX_ED, 0xB8);
  case MNE_LDI:
    return write_pfx_code<API>(addr, PREFIX_ED, 0xA0);
  case MNE_LDIR:
    return write_pfx_code<API>(addr, PREFIX_ED, 0xB0);
  case MNE_NEG:
    return write_pfx_code<API>(addr, PREFIX_ED, 0x44);
  case MNE_NOP:
    return write_code<API>(addr, 0x00);
  case MNE_OR:
    return write_alu<API>(addr, ALU_OR, op1, op2);
  case MNE_OTDR:
    return write_pfx_code<API>(addr, PREFIX_ED, 0xBB);
  case MNE_OTIR:
    return write_pfx_code<API>(addr, PREFIX_ED, 0xB3);
  case MNE_OUT:
    return write_out<API>(addr, op1, op2);
  case MNE_OUTD:
    return write_pfx_code<API>(addr, PREFIX_ED, 0xAB);
  case MNE_OUTI:
    return write_pfx_code<API>(addr, PREFIX_ED, 0xA3);
  case MNE_POP:
    return write_pop<API>(addr, op1);
  case MNE_PUSH:
    return write_push<API>(addr, op1);
  case MNE_RES:
    return write_cb_bit<API>(addr, CB_RES, op1, op2);
  case MNE_RET:
    return write_ret<API>(addr, op1);
  case MNE_RETI:
    return write_pfx_code<API>(addr, PREFIX_ED, 0x4D);
  case MNE_RETN:
    return write_pfx_code<API>(addr, PREFIX_ED, 0x45);
  case MNE_RL:
    return write_cb_rot<API>(addr, ROT_RL, op1);
  case MNE_RLA:
    return write_code<API>(addr, 0x17);
  case MNE_RLC:
    return write_cb_rot<API>(addr, ROT_RLC, op1);
  case MNE_RLCA:
    return write_code<API>(addr, 0x07);
  case MNE_RLD:
    return write_pfx_code<API>(addr, PREFIX_ED, 0x6F);
  case MNE_RR:
    return write_cb_rot<API>(addr, ROT_RR, op1);
  case MNE_RRA:
    return write_code<API>(addr, 0x1F);
  case MNE_RRC:
    return write_cb_rot<API>(addr, ROT_RRC, op1);
  case MNE_RRCA:
    return write_code<API>(addr, 0x0F);
  case MNE_RRD:
    return write_pfx_code<API>(addr, PREFIX_ED, 0x67);
  case MNE_RST:
    return write_rst<API>(addr, op1);
  case MNE_SBC:
    return write_alu<API>(addr, ALU_SBC, op1, op2);
  case MNE_SCF:
    return write_code<API>(addr, 0x37);
  case MNE_SET:
    return write_cb_bit<API>(addr, CB_SET, op1, op2);
  case MNE_SL1:
    return write_cb_rot<API>(addr, ROT_SL1, op1);
  case MNE_SLA:
    return write_cb_rot<API>(addr, ROT_SLA, op1);
  case MNE_SRA:
    return write_cb_rot<API>(addr, ROT_SRA, op1);
  case MNE_SRL:
    return write_cb_rot<API>(addr, ROT_SRL, op1);
  case MNE_SUB:
    return write_alu<API>(addr, ALU_SUB, op1, op2);
  case MNE_XOR:
    return write_alu<API>(addr, ALU_XOR, op1, op2);
  }
  return 0;
}

} // namespace z80
} // namespace uMon
