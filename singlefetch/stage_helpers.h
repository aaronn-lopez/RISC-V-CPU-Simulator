#ifndef __STAGE_HELPERS_H__
#define __STAGE_HELPERS_H__

#include <stdio.h>
#include "utils.h"
#include "pipeline.h"

/// EXECUTE STAGE HELPERS ///

/**
 * input  : idex_reg_t
 * output : uint32_t alu_control signal
 **/
uint32_t gen_alu_control(idex_reg_t idex_reg)
{
  uint32_t alu_control = 0;
  /**
   * YOUR CODE HERE
   */
  switch (idex_reg.ALUOp) {
    case 0x0: // lw or sw
      alu_control = 0x2; // add
      break;
    case 0x1: // beq or bne
      alu_control = 0x6; // subtract
      break;
    case 0x2: // R-type
      switch (idex_reg.funct3)   {
        case 0x0:
          if (idex_reg.funct7 == 0) {
            alu_control = 0x2; // add
          } else if (idex_reg.funct7 == 1) {
            alu_control = 0x6; // subt
          } else {
            alu_control = 0xC; // mul
            }
          break;
        case 0x1: 
          if (idex_reg.funct7 == 0) {
            alu_control = 0x7; // sll
          } else if (idex_reg.funct7 == 1) {
            alu_control = 0xD; // mulh
          }
          break;
        case 0x2: 
          alu_control = 0x8; // slt
          break;
        case 0x4: 
          alu_control = 0x9; // xor
          break;
        case 0x5:
          if (idex_reg.funct7 == 0) {
            alu_control = 0xA; // srl
          } else if (idex_reg.funct7 == 1) {
            alu_control = 0xB; // sra
          }
          break;
        case 0x6: 
          alu_control = 0x1; // or
          break;
        case 0x7: 
          alu_control = 0x0; // and
          break;
        default:
          break;
      }
    case 0x3: // I-type
	    switch (idex_reg.funct3) {
        case 0x0: 
          alu_control = 0x2; // add
          break;
        case 0x1: 
          alu_control = 0x7; // sll
          break;
        case 0x2: 
          alu_control = 0x8; // slt
          break;
        case 0x4: 
          alu_control = 0x9; // xor
          break;
        case 0x5:
            if (idex_reg.funct7 == 0) {
              alu_control = 0xA; // srl
            } else if (idex_reg.funct7 == 1) {
              alu_control = 0xB; // sra
            }
            break;
        case 0x6: 
          alu_control = 0x1; // or
          break;
        case 0x7: 
          alu_control = 0x0; // and
          break;
        default:
            break;
      }      
        break; 
    case 0x4: // lui
      alu_control = 0xE;
      break;
    case 0x5: // jal
      alu_control = 0xF;
      break;
    default:
        break;
  }
  return alu_control;
}

/**
 * input  : alu_inp1, alu_inp2, alu_control
 * output : uint32_t alu_result
 **/
uint32_t execute_alu(uint32_t alu_inp1, uint32_t alu_inp2, uint32_t alu_control)
{
  uint32_t result;
  switch(alu_control){
    case 0x0: //and
      result = alu_inp1 & alu_inp2;
      break;
    case 0x1: // or
      result = alu_inp1 | alu_inp2;
      break;
    case 0x2: // add
      result = alu_inp1 + alu_inp2;
      break;
    case 0x6: // sub
      result = alu_inp1 - alu_inp2;
      break;
    case 0x7: // sll
      result = alu_inp1 << alu_inp2;
      break;
    case 0x8: // slt
      result = (int32_t)alu_inp1 < (int32_t)alu_inp2 ? 1 : 0;
      break;
    case 0x9: // xor
      result = alu_inp1 ^ alu_inp2;
      break;
    case 0xA: // srl
      result = alu_inp1 >> alu_inp2;
      break;
    case 0xB: // sra
      result = (int32_t)alu_inp1 >> alu_inp2;
      break;
    case 0xC: // mul
      result = alu_inp1 * alu_inp2;
      break;
    case 0xD: // mulh
      result = ((uint64_t)alu_inp1 * (uint64_t)alu_inp2) >> 32;
      break;
    case 0xE: // lui
      result = alu_inp2 << 12;
      break;
    case 0xF: // jal
      result = alu_inp1 + 4;
      break;
    default:
      result = 0xBADCAFFE;
      break;
  };
  return result;
}

/// DECODE STAGE HELPERS ///

/**
 * input  : Instruction
 * output : idex_reg_t
 **/
uint32_t gen_imm(Instruction instruction)
{
  int imm_val = 0;
  /**
   * YOUR CODE HERE
   */
  switch(instruction.opcode) {
        case 0x03: //L-type
	      case 0x13: //I-type
            imm_val = sign_extend_number(instruction.itype.imm,12); 
            break;
	      case 0x23: //S-type
            imm_val = get_store_offset(instruction);
            break;
	      case 0x37: //U-type
            imm_val = instruction.utype.imm;
            break;
        case 0x6f: //UJ-type
            imm_val = sign_extend_number(get_jump_offset(instruction), 21);
            break;
        case 0x63: //SB-type
            imm_val = sign_extend_number(get_branch_offset(instruction), 13);
            break;
        default: // R and undefined opcode
            break;
    };
    return imm_val;
}

/**
 * generates all the control logic that flows around in the pipeline
 * input  : Instruction
 * output : idex_reg_t
 **/
idex_reg_t gen_control(Instruction instruction)
{
  idex_reg_t idex_reg = {0};
  switch(instruction.opcode) {
      case 0x33:  //R-type
        idex_reg.ALUOp = 0x2;
        idex_reg.ALUSrc = 0;
        idex_reg.Branch = 0;
        idex_reg.Mem_Read = 0;
        idex_reg.Memto_Reg = 0;
        idex_reg.Mem_Write = 0;
        idex_reg.Reg_Write = 1;
        break;
      case 0x13: // I-type except load
        idex_reg.ALUOp = 0x3;
        idex_reg.ALUSrc = 1;
        idex_reg.Branch = 0;
        idex_reg.Mem_Read = 0;
        idex_reg.Memto_Reg = 0; 
        idex_reg.Mem_Write = 0;
        idex_reg.Reg_Write = 1;
        break;
      case 0x03: // Load
        idex_reg.ALUOp = 0;
        idex_reg.ALUSrc = 1;
        idex_reg.Branch = 0;
        idex_reg.Mem_Read = 1;
        idex_reg.Memto_Reg = 1;
        idex_reg.Mem_Write = 0;
        idex_reg.Reg_Write = 1;
        break;
      case 0x23: // Store
        idex_reg.ALUOp = 0;
        idex_reg.ALUSrc = 1;
        idex_reg.Branch = 0;
        idex_reg.Mem_Read = 0;
        idex_reg.Memto_Reg = 0;
        idex_reg.Mem_Write = 1;
        idex_reg.Reg_Write = 0;
        break;
      case 0x37: // U-type
        idex_reg.ALUOp = 0x4;
        idex_reg.ALUSrc = 1;
        idex_reg.Branch = 0;
        idex_reg.Mem_Read = 0;
        idex_reg.Memto_Reg = 0;
        idex_reg.Mem_Write = 0;
        idex_reg.Reg_Write = 1;
        break;
      case 0x6f: // J-type
        idex_reg.ALUOp = 0x5;
        idex_reg.ALUSrc = 0;
        idex_reg.Branch = 1;
        idex_reg.Mem_Read = 0;
        idex_reg.Memto_Reg = 0;
        idex_reg.Mem_Write = 0;
        idex_reg.Reg_Write = 1;
        break;
      case 0x63: // SB-type
        idex_reg.ALUOp = 0x1;
        idex_reg.ALUSrc = 0;
        idex_reg.Branch = 1;
        idex_reg.Mem_Read = 0;
        idex_reg.Mem_Write = 0;
        idex_reg.Reg_Write = 0;
      default:  // Remaining opcodes
        break;
  }
  return idex_reg;
}

/// MEMORY STAGE HELPERS ///

/**
 * evaluates whether a branch must be taken
 * input  : <open to implementation>
 * output : bool
 **/
bool gen_branch(Instruction instr, regfile_t * regfile_p)
{
  switch (instr.sbtype.funct3) {
    case 0x0: // beq
        return regfile_p->R[instr.sbtype.rs1] == regfile_p->R[instr.sbtype.rs2];
    case 0x1: // bne
        return regfile_p->R[instr.sbtype.rs1] != regfile_p->R[instr.sbtype.rs2];
	default:
  return false;
  }
}

/// PIPELINE FEATURES ///

/**
 * Task   : Sets the pipeline wires for the forwarding unit's control signals
 *           based on the pipeline register values.
 * input  : pipeline_regs_t*, pipeline_wires_t*
 * output : None
*/
void gen_forward(pipeline_regs_t* pregs_p, pipeline_wires_t* pwires_p)
{
  /**
   * YOUR CODE HERE
   */
}

/**
 * Task   : Sets the pipeline wires for the hazard unit's control signals
 *           based on the pipeline register values.
 * input  : pipeline_regs_t*, pipeline_wires_t*
 * output : None
*/
void detect_hazard(pipeline_regs_t* pregs_p, pipeline_wires_t* pwires_p, regfile_t* regfile_p)
{
  /**
   * YOUR CODE HERE
   */
}


///////////////////////////////////////////////////////////////////////////////


/// RESERVED FOR PRINTING REGISTER TRACE AFTER EACH CLOCK CYCLE ///
void print_register_trace(regfile_t* regfile_p)
{
  // print
  for (uint8_t i = 0; i < 8; i++)       // 8 columns
  {
    for (uint8_t j = 0; j < 4; j++)     // of 4 registers each
    {
      printf("r%2d=%08x ", i * 4 + j, regfile_p->R[i * 4 + j]);
    }
    printf("\n");
  }
  printf("\n");
}

#endif // __STAGE_HELPERS_H__
