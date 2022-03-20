#include "uMon/z80.hpp"

#include <Arduino.h>
#include <unity.h>

using namespace uMon::z80;

constexpr const uint16_t DATA_SIZE = 64;
uint8_t test_data[DATA_SIZE];
uCLI::CursorOwner<16> test_io;

struct TestAPI {
  static void print_char(char c) { test_io.try_insert(c); }
  static void print_string(const char* str) { test_io.try_insert(str); }
  static uint8_t read_byte(uint16_t addr) { return test_data[addr % DATA_SIZE]; };
  template <uint8_t N>
  static void read_bytes(uint16_t addr, uint8_t (&buf)[N]) {
    for (uint8_t i = 0; i < N; ++i) {
      buf[i] = read_byte(addr + i);
    }
  }
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
void test_asm(AsmTest& test) {
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
    Operand& ref_op = test.inst.operands[i];
    Operand& dasm_op = inst_in.operands[i];
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
    Operand& ref_op = test.inst.operands[i];
    Operand& dasm_op = inst_out.operands[i];
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

AsmTest test_cases[] = {
  {"NOP",           1, "\x00",              {MNE_NOP}},
  {"EX AF,AF",      1, "\x08",              {MNE_EX, {TOK_AF}, {TOK_AF}}},
  {"DJNZ $0000",    2, "\x10\xFE",          {MNE_DJNZ, {TOK_IMMEDIATE, 0}}},

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
  {"RRD",           2, "\xED\x67",          {MNE_RRD}},
  {"RLD",           2, "\xED\x6F",          {MNE_RLD}},
};

void test_asm_cases(void) {
  for (AsmTest& test : test_cases) {
    test_asm(test);
  }
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_asm_cases);
  UNITY_END();
}

void loop() {}
