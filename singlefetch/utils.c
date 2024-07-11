#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

/* Unpacks the 32-bit machine code instruction given into the correct
 * type within the instruction struct */
Instruction parse_instruction(uint32_t instruction_bits) {
  /* YOUR CODE HERE */
  Instruction instruction;
  // add x9, x20, x21   hex: 01 5A 04 B3, binary = 0000 0001 0101 1010 0000 0100 1011 0011
  // Opcode: 0110011 (0x33) Get the Opcode by &ing 0x1111111, bottom 7 bits
  instruction.opcode = instruction_bits & ((1U << 7) - 1);

  // Shift right to move to pointer to interpret next fields in instruction.
  instruction_bits >>= 7;

  switch (instruction.opcode) {
  // R-Type
  case 0x33:
    // instruction: 0000 0001 0101 1010 0000 0100 1, destination : 01001
    instruction.rtype.rd = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;

    // instruction: 0000 0001 0101 1010 0000, func3 : 000
    instruction.rtype.funct3 = instruction_bits & ((1U << 3) - 1);
    instruction_bits >>= 3;

    // instruction: 0000 0001 0101 1010 0, src1: 10100
    instruction.rtype.rs1 = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;

    // instruction: 0000 0001 0101, src2: 10101
    instruction.rtype.rs2 = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;

    // funct7: 0000 000
    instruction.rtype.funct7 = instruction_bits & ((1U << 7) - 1);
    break;
  // cases for other types of instructions
  /* YOUR CODE HERE */
case 0x03: 
    instruction.itype.rd = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;

    instruction.itype.funct3 = instruction_bits & ((1U << 3) - 1);
    instruction_bits >>= 3; 

    instruction.itype.rs1 = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;  

    instruction.itype.imm = instruction_bits & ((1U << 12) - 1);
    break;
    // I-Type
    case 0x13: 
    instruction.itype.rd = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;

    instruction.itype.funct3 = instruction_bits & ((1U << 3) - 1);
    instruction_bits >>= 3; 

    instruction.itype.rs1 = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;  
    
    instruction.itype.imm = instruction_bits & ((1U << 12) - 1);
    break;
    // System Instructions (I-Type)
    case 0x73: 
    instruction.itype.rd = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;

    instruction.itype.funct3 = instruction_bits & ((1U << 3) - 1);
    instruction_bits >>= 3; 

    instruction.itype.rs1 = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;  

    instruction.itype.imm = instruction_bits & ((1U << 12) - 1);
    break;
    // S-Type
    case 0x23: 
    instruction.stype.imm5 = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;

    instruction.stype.funct3 = instruction_bits & ((1U << 3) - 1);
    instruction_bits >>= 3;

    instruction.stype.rs1 = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;

    instruction.stype.rs2 = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;

    instruction.stype.imm7 = instruction_bits & ((1U << 7) - 1);
    break;
    // SB-Type
    case 0x63: 
    instruction.sbtype.imm5 = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;

    instruction.sbtype.funct3 = instruction_bits & ((1U << 3) - 1);
    instruction_bits >>= 3;

    instruction.sbtype.rs1 = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;

    instruction.sbtype.rs2 = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;

    instruction.sbtype.imm7 = instruction_bits & ((1U << 7) - 1);
    break;
    // U-Type
    case 0x37:
    instruction.utype.rd = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;

    instruction.utype.imm = instruction_bits & ((1U << 20) - 1);
    break;
    // UJ-Type
    case 0x6f:
    instruction.ujtype.rd = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;

    instruction.ujtype.imm = instruction_bits & ((1U << 20) - 1);
    break; 
  #ifndef TESTING
  default:
    exit(EXIT_FAILURE);
  #endif
  }
  return instruction;
}

/************************Helper functions************************/
/* Here, you will need to implement a few common helper functions, 
 * which you will call in other functions when parsing, printing, 
 * or executing the instructions. */

/* Sign extends the given field to a 32-bit integer where field is
 * interpreted an n-bit integer. */
int sign_extend_number(unsigned int field, unsigned int n) {
  /* YOUR CODE HERE */
  // Check if the sign bit of the n-bit integer is set
  unsigned int sign_bit = 1U << (n - 1);

  // If the sign bit is set, extend the sign
  if((field & sign_bit)) {
	// Create a mask with all bits set above the sign bit
        unsigned int mask = ~((1U << n) - 1);
        return field | mask;
    } else {
        // If the sign bit is not set, just return the field
        field &= (1U << n) - 1;
	    return field;
    }
}

/* Return the number of bytes (from the current PC) to the branch label using
 * the given branch instruction */
int get_branch_offset(Instruction instruction) {
  /* YOUR CODE HERE */
  int imm4_1 = (instruction.sbtype.imm5 >> 1) & ((1U << 4) - 1); // extract imm[4:1] from imm5
  int imm11 = instruction.sbtype.imm5 & (1U); // extract imm[11] from imm 5
  int imm10_5 = instruction.sbtype.imm7 & ((1U << 6) - 1); // etract imm[10:5] from imm7
  int imm12 = instruction.sbtype.imm7 & (1U << 6); // etract imm[12] from imm7

  // combining imms together
  int offset = (imm12 << 6) | (imm11<< 11) | (imm10_5 << 5) | (imm4_1 << 1);
  return offset;
}

/* Returns the number of bytes (from the current PC) to the jump label using the
 * given jump instruction */
int get_jump_offset(Instruction instruction) {
  /* YOUR CODE HERE */
  int imm20 = instruction.ujtype.imm & (1U << 19); // extract imm[20]
  int imm10_1 = (instruction.ujtype.imm >> 9) & ((1U << 10) - 1); // extract imm[10:1]
  int imm11 = instruction.ujtype.imm & (1U << 8); // extract imm[11]
  int imm19_12 = instruction.ujtype.imm & ((1U << 8) - 1); // extract imm[19:12]

  // combining imms together
  int offset = (imm20 << 1) | (imm19_12 << 12) | (imm11 << 3) |(imm10_1 << 1);
  return offset;
}

/* Returns the number of bytes (from the current PC) to the base address using the
 * given store instruction */
int get_store_offset(Instruction instruction) {
  /* YOUR CODE HERE */
  // combining imms together
  int offset = (instruction.stype.imm7 << 5) | (instruction.stype.imm5);
    return offset;
}
/************************Helper functions************************/

void handle_invalid_instruction(Instruction instruction) {
  printf("Invalid Instruction: 0x%08x\n", instruction.bits);
}

void handle_invalid_read(Address address) {
  printf("Bad Read. Address: 0x%08x\n", address);
  exit(-1);
}

void handle_invalid_write(Address address) {
  printf("Bad Write. Address: 0x%08x\n", address);
  exit(-1);
}
