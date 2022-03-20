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
  Instruction inst;
  const char* code;
  uint8_t n_bytes;
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
  {"NOP", {MNE_NOP}, "\0", 1},
  {"HALT", {MNE_HALT}, "\x76", 1},
  {"INC C", {MNE_INC, {TOK_C}}, "\014", 1},
  {"LD A,R", {MNE_LD, {TOK_A}, {TOK_R}}, "\xED\137", 2},
  {"LD (HL),$05", {MNE_LD, {TOK_HL_IND}, {TOK_IMMEDIATE, 5}}, "\066\x05", 2},
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
