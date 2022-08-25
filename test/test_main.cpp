#include "uMon/z80.hpp"
#include "uMon/api.hpp"

#include <unity.h>

using namespace uMon::z80;

constexpr const uint16_t DATA_SIZE = 8;
uint8_t test_data[DATA_SIZE];
uCLI::CursorOwner<16> test_io;

struct TestAPI : public uMon::Base<TestAPI> {
  static void print_char(char c) { test_io.try_insert(c); }
  static void print_string(const char* str) { test_io.try_insert(str); }
  static void newline() { test_io.try_insert('\n'); }

  static uint8_t read_byte(uint16_t addr) { return test_data[addr % DATA_SIZE]; };
  static void write_byte(uint16_t addr, uint8_t data) { test_data[addr % DATA_SIZE] = data; }

  static void prompt_char(char c) { }
  static void prompt_string(const char* str) { }
};

struct AsmTest {
  const char* str;
  uint8_t n_bytes;
  const char* code;
  Instruction inst;
};

// Transform from text->tokens->code->tokens->text
void test_asm(const AsmTest& test) {
  const uint16_t addr = 0;

  // Prepare CLI tokens for assembler
  test_io.clear();
  test_io.try_insert(test.str);
  uCLI::Tokens args(test_io.contents());

  // Parse instruction from CLI tokens and validate
  Instruction inst_in;
  TEST_ASSERT_TRUE_MESSAGE(parse_instruction<TestAPI>(inst_in, args), test.str);
  TEST_ASSERT_EQUAL_MESSAGE(test.inst.mnemonic, inst_in.mnemonic, test.str);
  for (uint8_t i = 0; i < MAX_OPERANDS; ++i) {
    const Operand& ref_op = test.inst.operands[i];
    const Operand& dasm_op = inst_in.operands[i];
    TEST_ASSERT_EQUAL_MESSAGE(ref_op.token, dasm_op.token, test.str);
    TEST_ASSERT_EQUAL_MESSAGE(ref_op.value, dasm_op.value, test.str);
  }

  // Assemble instruction and validate machine code bytes
  uint8_t size;
  size = asm_instruction<TestAPI>(inst_in, addr);
  TEST_ASSERT_EQUAL_MESSAGE(test.n_bytes, size, test.str);
  for (uint8_t i = 0; i < size; ++i) {
    uint8_t expected = test.code[i];
    uint8_t assembled = test_data[addr + i];
    TEST_ASSERT_EQUAL_MESSAGE(expected, assembled, test.str);
  }

  // Disassemble instruction and validate tokens
  Instruction inst_out;
  size = dasm_instruction<TestAPI>(inst_out, addr);
  TEST_ASSERT_EQUAL_MESSAGE(test.n_bytes, size, test.str);
  TEST_ASSERT_EQUAL_MESSAGE(test.inst.mnemonic, inst_out.mnemonic, test.str);
  for (uint8_t i = 0; i < MAX_OPERANDS; ++i) {
    const Operand& ref_op = test.inst.operands[i];
    const Operand& dasm_op = inst_out.operands[i];
    // Ignore print formatting flags
    const uint8_t mask = ~(TOK_BYTE | TOK_DIGIT);
    TEST_ASSERT_EQUAL_MESSAGE(ref_op.token, dasm_op.token & mask, test.str);
    TEST_ASSERT_EQUAL_MESSAGE(ref_op.value, dasm_op.value, test.str);
  }

  // Print instruction from tokens and validate with input
  test_io.clear();
  print_instruction<TestAPI>(inst_out);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(test.str, test_io.contents(), test.str);
}

AsmTest misc_cases[] = {
  {"NOP",           1, "\x00",              {MNE_NOP}},
  {"EX AF,AF",      1, "\x08",              {MNE_EX, {TOK_AF}, {TOK_AF}}},
  {"DJNZ $0000",    2, "\x10\xFE",          {MNE_DJNZ, {TOK_IMMEDIATE, 0}}},
  {"JR $0002",      2, "\x18\x00",          {MNE_JR, {TOK_IMMEDIATE, 2}}},
  {"JR NZ,$0004",   2, "\x20\x02",          {MNE_JR, {TOK_NZ}, {TOK_IMMEDIATE, 4}}},
  {"JR Z,$0081",    2, "\x28\x7F",          {MNE_JR, {TOK_Z}, {TOK_IMMEDIATE, 0x81}}},
  {"JR NC,$FF82",   2, "\x30\x80",          {MNE_JR, {TOK_NC}, {TOK_IMMEDIATE, 0xFF82}}},
  {"JR C,$FFD2",    2, "\x38\xD0",          {MNE_JR, {TOK_C}, {TOK_IMMEDIATE, 0xFFD2}}},

  {"LD (BC),A",     1, "\x02",              {MNE_LD, {TOK_BC_IND}, {TOK_A}}},
  {"LD A,(BC)",     1, "\x0A",              {MNE_LD, {TOK_A}, {TOK_BC_IND}}},
  {"LD (DE),A",     1, "\x12",              {MNE_LD, {TOK_DE_IND}, {TOK_A}}},
  {"LD A,(DE)",     1, "\x1A",              {MNE_LD, {TOK_A}, {TOK_DE_IND}}},
  {"LD ($CAFE),HL", 3, "\x22\xFE\xCA",      {MNE_LD, {TOK_IMM_IND, 0xCAFE}, {TOK_HL}}},
  {"LD HL,($BABE)", 3, "\x2A\xBE\xBA",      {MNE_LD, {TOK_HL}, {TOK_IMM_IND, 0xBABE}}},
  {"LD ($CAFE),IX", 4, "\xDD\x22\xFE\xCA",  {MNE_LD, {TOK_IMM_IND, 0xCAFE}, {TOK_IX}}},
  {"LD IX,($BABE)", 4, "\xDD\x2A\xBE\xBA",  {MNE_LD, {TOK_IX}, {TOK_IMM_IND, 0xBABE}}},
  {"LD ($CAFE),IY", 4, "\xFD\x22\xFE\xCA",  {MNE_LD, {TOK_IMM_IND, 0xCAFE}, {TOK_IY}}},
  {"LD IY,($BABE)", 4, "\xFD\x2A\xBE\xBA",  {MNE_LD, {TOK_IY}, {TOK_IMM_IND, 0xBABE}}},
  {"LD ($DEAD),A",  3, "\x32\xAD\xDE",      {MNE_LD, {TOK_IMM_IND, 0xDEAD}, {TOK_A}}},
  {"LD A,($BEEF)",  3, "\x3A\xEF\xBE",      {MNE_LD, {TOK_A}, {TOK_IMM_IND, 0xBEEF}}},

  {"RLCA",          1, "\x07",              {MNE_RLCA}},
  {"RRCA",          1, "\x0F",              {MNE_RRCA}},
  {"RLA",           1, "\x17",              {MNE_RLA}},
  {"RRA",           1, "\x1F",              {MNE_RRA}},
  {"DAA",           1, "\x27",              {MNE_DAA}},
  {"CPL",           1, "\x2F",              {MNE_CPL}},
  {"SCF",           1, "\x37",              {MNE_SCF}},
  {"CCF",           1, "\x3F",              {MNE_CCF}},

  {"HALT",          1, "\x76",              {MNE_HALT}},

  {"RET",           1, "\xC9",              {MNE_RET}},
  {"CALL $FFD2",    3, "\xCD\xD2\xFF",      {MNE_CALL, {TOK_IMMEDIATE, 0xFFD2}}},
  {"EXX",           1, "\xD9",              {MNE_EXX}},
  {"JP (HL)",       1, "\xE9",              {MNE_JP, {TOK_HL_IND}}},
  {"JP (IX)",       2, "\xDD\xE9",          {MNE_JP, {TOK_IX_IND}}},
  {"JP (IY)",       2, "\xFD\xE9",          {MNE_JP, {TOK_IY_IND}}},
  {"LD SP,HL",      1, "\xF9",              {MNE_LD, {TOK_SP}, {TOK_HL}}},
  {"LD SP,IX",      2, "\xDD\xF9",          {MNE_LD, {TOK_SP}, {TOK_IX}}},
  {"LD SP,IY",      2, "\xFD\xF9",          {MNE_LD, {TOK_SP}, {TOK_IY}}},

  {"EX (SP),HL",    1, "\xE3",              {MNE_EX, {TOK_SP_IND}, {TOK_HL}}},
  {"EX (SP),IX",    2, "\xDD\xE3",          {MNE_EX, {TOK_SP_IND}, {TOK_IX}}},
  {"EX (SP),IY",    2, "\xFD\xE3",          {MNE_EX, {TOK_SP_IND}, {TOK_IY}}},
  {"EX DE,HL",      1, "\xEB",              {MNE_EX, {TOK_DE}, {TOK_HL}}},
  {"DI",            1, "\xF3",              {MNE_DI}},
  {"EI",            1, "\xFB",              {MNE_EI}},

  {"NEG",           2, "\xED\x44",          {MNE_NEG}},
  {"RETN",          2, "\xED\x45",          {MNE_RETN}},
  {"RETI",          2, "\xED\x4D",          {MNE_RETI}},
  {"IM 0",          2, "\xED\x46",          {MNE_IM, {TOK_IMMEDIATE, 0}}},
  {"IM 1",          2, "\xED\x56",          {MNE_IM, {TOK_IMMEDIATE, 1}}},
  {"IM 2",          2, "\xED\x5E",          {MNE_IM, {TOK_IMMEDIATE, 2}}},
  {"LD I,A",        2, "\xED\x47",          {MNE_LD, {TOK_I}, {TOK_A}}},
  {"LD R,A",        2, "\xED\x4F",          {MNE_LD, {TOK_R}, {TOK_A}}},
  {"LD A,I",        2, "\xED\x57",          {MNE_LD, {TOK_A}, {TOK_I}}},
  {"LD A,R",        2, "\xED\x5F",          {MNE_LD, {TOK_A}, {TOK_R}}},
  {"RRD",           2, "\xED\x67",          {MNE_RRD}},
  {"RLD",           2, "\xED\x6F",          {MNE_RLD}},

  {"LDI",           2, "\xED\xA0",          {MNE_LDI}},
  {"LDD",           2, "\xED\xA8",          {MNE_LDD}},
  {"LDIR",          2, "\xED\xB0",          {MNE_LDIR}},
  {"LDDR",          2, "\xED\xB8",          {MNE_LDDR}},
  {"CPI",           2, "\xED\xA1",          {MNE_CPI}},
  {"CPD",           2, "\xED\xA9",          {MNE_CPD}},
  {"CPIR",          2, "\xED\xB1",          {MNE_CPIR}},
  {"CPDR",          2, "\xED\xB9",          {MNE_CPDR}},
  {"INI",           2, "\xED\xA2",          {MNE_INI}},
  {"IND",           2, "\xED\xAA",          {MNE_IND}},
  {"INIR",          2, "\xED\xB2",          {MNE_INIR}},
  {"INDR",          2, "\xED\xBA",          {MNE_INDR}},
  {"OUTI",          2, "\xED\xA3",          {MNE_OUTI}},
  {"OUTD",          2, "\xED\xAB",          {MNE_OUTD}},
  {"OTIR",          2, "\xED\xB3",          {MNE_OTIR}},
  {"OTDR",          2, "\xED\xBB",          {MNE_OTDR}},
};

void test_asm_misc(void) {
  for (AsmTest& test : misc_cases) {
    test_asm(test);
  }
}

// NOTE can't use TOK_STR since (HL) is encoded as TOK_HL | TOK_INDIRECT
const char* REG_STR[] = { "B", "C", "D", "E", "H", "L", "(HL)", "A" };

uCLI::CursorOwner<16> asm_buf;

void test_asm_ld_r(void) {
  // Test LD r,r
  for (uint8_t dst = 0; dst < 8; ++dst) {
    uint8_t dst_tok = REG_TOK[dst];
    const char* dst_str = REG_STR[dst];
    for (uint8_t src = 0; src < 8; ++src) {
      uint8_t src_tok = REG_TOK[src];
      const char* src_str = REG_STR[src];
      if (dst == 6 && src == 6) continue; // skip HALT
      asm_buf.clear();
      asm_buf.try_insert(MNE_STR_LD);
      asm_buf.try_insert(' ');
      asm_buf.try_insert(dst_str);
      asm_buf.try_insert(',');
      asm_buf.try_insert(src_str);
      uint8_t code = 0100 | dst << 3 | src;
      AsmTest test = {asm_buf.contents(), 1, (char*)&code, {MNE_LD, {dst_tok}, {src_tok}}};
      test_asm(test);
    }
    { // Test LD r,n
      asm_buf.clear();
      asm_buf.try_insert(MNE_STR_LD);
      asm_buf.try_insert(' ');
      asm_buf.try_insert(dst_str);
      asm_buf.try_insert(",$");
      uint8_t imm = dst * 8;
      uMon::format_hex8([&](char c){asm_buf.try_insert(c);}, imm);
      uint8_t code[2] = { uint8_t(0006 | dst << 3), imm };
      AsmTest test = {asm_buf.contents(), sizeof(code), (char*)code, {MNE_LD, {dst_tok}, {TOK_IMMEDIATE, imm}}};
      test_asm(test);
    }
    if (dst == 6) continue; // skip HALT
    { // Test LD r,(IX)
      asm_buf.clear();
      asm_buf.try_insert(MNE_STR_LD);
      asm_buf.try_insert(' ');
      asm_buf.try_insert(dst_str);
      asm_buf.try_insert(",(IX)");
      uint8_t code[3] = { PREFIX_IX, uint8_t(0106 | dst << 3), 0x00 };
      AsmTest test = {asm_buf.contents(), sizeof(code), (char*)code, {MNE_LD, {dst_tok}, {TOK_IX_IND}}};
      test_asm(test);
    }
    { // Test LD (IX),r
      asm_buf.clear();
      asm_buf.try_insert(MNE_STR_LD);
      asm_buf.try_insert(" (IX+$01),");
      asm_buf.try_insert(dst_str); // NOTE using dst as src
      uint8_t code[3] = { PREFIX_IX, uint8_t(0160 | dst), 0x01 };
      AsmTest test = {asm_buf.contents(), sizeof(code), (char*)code, {MNE_LD, {TOK_IX_IND, 1}, {dst_tok}}};
      test_asm(test);
    }
    { // Test LD r,(IY)
      asm_buf.clear();
      asm_buf.try_insert(MNE_STR_LD);
      asm_buf.try_insert(' ');
      asm_buf.try_insert(dst_str);
      asm_buf.try_insert(",(IY-$01)");
      uint8_t code[3] = { PREFIX_IY, uint8_t(0106 | dst << 3), 0xFF };
      AsmTest test = {asm_buf.contents(), sizeof(code), (char*)code, {MNE_LD, {dst_tok}, {TOK_IY_IND, uint16_t(-1)}}};
      test_asm(test);
    }
    { // Test LD (IY),r
      asm_buf.clear();
      asm_buf.try_insert(MNE_STR_LD);
      asm_buf.try_insert(" (IY+$7F),");
      asm_buf.try_insert(dst_str); // NOTE using dst as src
      uint8_t code[3] = { PREFIX_IY, uint8_t(0160 | dst), 0x7F };
      AsmTest test = {asm_buf.contents(), sizeof(code), (char*)code, {MNE_LD, {TOK_IY_IND, 0x7F}, {dst_tok}}};
      test_asm(test);
    }
  }
}

void test_asm_alu_r() {
  // Test [alu] A,r
  for (uint8_t alu = 0; alu < 8; ++alu) {
    uint8_t alu_mne = ALU_MNE[alu];
    const char* alu_str = MNE_STR[alu_mne];
    for (uint8_t src = 0; src < 8; ++src) {
      uint8_t src_tok = REG_TOK[src];
      const char* src_str = REG_STR[src];
      asm_buf.clear();
      asm_buf.try_insert(alu_str);
      asm_buf.try_insert(" A,");
      asm_buf.try_insert(src_str);
      uint8_t code = 0200 | alu << 3 | src;
      AsmTest test = {asm_buf.contents(), 1, (char*)&code, {alu_mne, {TOK_A}, {src_tok}}};
      test_asm(test);
    }
    { // Test [alu] A,(IX+d)
      asm_buf.clear();
      asm_buf.try_insert(alu_str);
      asm_buf.try_insert(" A,(IX-$80)");
      uint8_t code[3] = { PREFIX_IX, uint8_t(0206 | alu << 3), 0x80 };
      AsmTest test = {asm_buf.contents(), sizeof(code), (char*)code, {alu_mne, {TOK_A}, {TOK_IX_IND, uint16_t(-0x80)}}};
      test_asm(test);
    }
    { // Test [alu] A,(IY+d)
      asm_buf.clear();
      asm_buf.try_insert(alu_str);
      asm_buf.try_insert(" A,(IY)");
      uint8_t code[3] = { PREFIX_IY, uint8_t(0206 | alu << 3), 0x00 };
      AsmTest test = {asm_buf.contents(), sizeof(code), (char*)code, {alu_mne, {TOK_A}, {TOK_IY_IND}}};
      test_asm(test);
    }
    { // Test [alu] A,n
      asm_buf.clear();
      asm_buf.try_insert(alu_str);
      asm_buf.try_insert(" A,$");
      uint8_t imm = alu * 8;
      uMon::format_hex8([&](char c){asm_buf.try_insert(c);}, imm);
      uint8_t code[] = { uint8_t(0306 | alu << 3), imm };
      AsmTest test = {asm_buf.contents(), sizeof(code), (char*)code, {alu_mne, {TOK_A}, {TOK_IMMEDIATE, imm}}};
      test_asm(test);
    }
  }
}

void test_asm_inc_r() {
  static uint8_t OPS[] = { MNE_INC, MNE_DEC };
  for (uint8_t mne : OPS) {
    // inc/dec r
    for (uint8_t reg = 0; reg < 8; ++reg) {
      uint8_t reg_tok = REG_TOK[reg];
      const char* reg_str = REG_STR[reg];
      asm_buf.clear();
      asm_buf.try_insert(MNE_STR[mne]);
      asm_buf.try_insert(' ');
      asm_buf.try_insert(reg_str);
      uint8_t code = (mne == MNE_DEC ? 0005 : 0004) | reg << 3;
      AsmTest test = {asm_buf.contents(), 1, (char*)&code, {mne, {reg_tok}}};
      test_asm(test);
    }
    // inc/dec ixh/ixl/iyh/iyl
    static uint8_t TOK[] = {TOK_IXL, TOK_IXH, TOK_IYL, TOK_IYH};
    for (uint8_t i = 0; i < 4; ++i) {
      uint8_t reg_tok = TOK[i];
      const char* reg_str = TOK_STR[reg_tok];
      uint8_t prefix = token_to_prefix(reg_tok);
      uint8_t reg = token_to_reg(reg_tok, prefix);
      asm_buf.clear();
      asm_buf.try_insert(MNE_STR[mne]);
      asm_buf.try_insert(' ');
      asm_buf.try_insert(reg_str);
      uint8_t code[] = {prefix, uint8_t((mne == MNE_DEC ? 0005 : 0004) | reg << 3)};
      AsmTest test = {asm_buf.contents(), sizeof(code), (char*)code, {mne, {reg_tok}}};
      test_asm(test);}
  }
}

template <uint8_t N>
void assert_sorted(const char* const (&table)[N]) {
  for (uint8_t i = 1; i < N; ++i) {
    const char* entry = table[i];
    // Assert ascending sort; each entry greater than previous
    int cmp = strcasecmp(entry, table[i - 1]);
    TEST_ASSERT_TRUE_MESSAGE(cmp > 0, entry);
    // Assert each entry can be found by binary search
    TEST_ASSERT_EQUAL_MESSAGE(i, uMon::pgm_bsearch(table, entry), entry);
  }
}

void test_str_sort() {
  assert_sorted(MNE_STR);
  assert_sorted(TOK_STR);
}

int main(int argc, char* argv[]) {
  UNITY_BEGIN();
  RUN_TEST(test_str_sort);
  RUN_TEST(test_asm_misc);
  RUN_TEST(test_asm_ld_r);
  RUN_TEST(test_asm_alu_r);
  RUN_TEST(test_asm_inc_r);
  UNITY_END();
}
