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
  if(idex_reg.ALUOp == 0x10) {
    switch(idex_reg.instr.rtype.funct3) {
      case 0x0:
      switch(idex_reg.instr.rtype.funct7) {
        case 0x0: //add 
        alu_control = 0x2;
          break;
        case 0x20: //sub
        alu_control = 0x6;
        default:
          break;
      };
        break;
      case 0x7: //and
      alu_control = 0x0;
        break;
      case 0x6: //or
      alu_control = 0x1;
      default:
        break;
    };
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
    case 0x2: //add
      result = alu_inp1 + alu_inp2;
      break;
    /**
     * YOUR CODE HERE
     */
    case 0x6: //sub
      result = alu_inp1 - alu_inp2;
      break;
    case 0x0: //and
      result = alu_inp1 & alu_inp2;
      break;
    case 0x1: //or
      result = alu_inp1 | alu_inp2;
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
        case 0x63: //B-type
            imm_val = get_branch_offset(instruction);
            break;
        /**
         * YOUR CODE HERE
         */
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
        /**
         * YOUR CODE HERE
         */
        idex_reg.ALUOp = 0x10;
        idex_reg.ALUSrc = 0;
        idex_reg.Branch = 0;
        idex_reg.Mem_Read = 0;
        idex_reg.Mem_Write = 0;
        idex_reg.Reg_Write = 1;
        idex_reg.Memto_Reg = 0;
        break;
      case 0x03:  //lw
        if(instruction.itype.funct3 == 0x2) {
          idex_reg.ALUOp = 0x00;
          idex_reg.ALUSrc = 1;
          idex_reg.Branch = 0;
          idex_reg.Mem_Read = 1;
          idex_reg.Mem_Write = 0;
          idex_reg.Reg_Write = 1;
          idex_reg.Memto_Reg = 1;
        }
        break;
      case 0x23:  //sw
        if(instruction.stype.funct3 == 0x2) {
          idex_reg.ALUOp = 0x00;
          idex_reg.ALUSrc = 1;
          idex_reg.Branch = 0;
          idex_reg.Mem_Read = 0;
          idex_reg.Mem_Write = 1;
          idex_reg.Reg_Write = 0;
        }
        break;
      case 0x63:  //beq
        if(instruction.sbtype.funct3 == 0x0) {
          idex_reg.ALUOp = 0x01;
          idex_reg.ALUSrc = 0;
          idex_reg.Branch = 1;
          idex_reg.Mem_Read = 0;
          idex_reg.Mem_Write = 0;
          idex_reg.Reg_Write = 0;
        }
        break;
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
bool gen_branch(/*<args>*/)
{
  /**
   * YOUR CODE HERE
   */
  return false;
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
