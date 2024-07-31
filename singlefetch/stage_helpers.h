#ifndef __STAGE_HELPERS_H__
#define __STAGE_HELPERS_H__

#include <stdio.h>
#include "utils.h"
#include "pipeline.h"

/// EXECUTE STAGE HELPERS ///

uint32_t gen_alu_control(idex_reg_t idex_reg)
{
  uint32_t alu_control = 0;
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
          if (idex_reg.funct7 == 0x0) {
            alu_control = 0x2; // add
          } else if (idex_reg.funct7 == 0x20) {
            alu_control = 0x6; // subt
          } else if (idex_reg.funct7 == 0x01) {
            alu_control = 0xC; // mul
            }
          break;
        case 0x1: 
          if (idex_reg.funct7 == 0x0) {
            alu_control = 0x7; // sll
          } else if (idex_reg.funct7 == 0x01) {
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
          if (idex_reg.funct7 == 0x0) {
            alu_control = 0xA; // srl
          } else if (idex_reg.funct7 == 0x20) {
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
    case 0x3: // I-type
	    switch (idex_reg.funct3) {
        case 0x0: 
          alu_control = 0x2; // addi
          break;
        case 0x1: 
          alu_control = 0x7; // slli
          break;
        case 0x2: 
          alu_control = 0x8; // slti
          break;
        case 0x4: 
          alu_control = 0x9; // xori
          break;
        case 0x5:
            if ((idex_reg.imm >> 5) == 0x0) {
              alu_control = 0xA; // srli
            } else if ((idex_reg.imm >> 5) == 0x20) {
              alu_control = 0xB; // srai
            }
            break;
        case 0x6: 
          alu_control = 0x1; // ori
          break;
        case 0x7: 
          alu_control = 0x0; // andi
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

uint32_t gen_imm(Instruction instruction)
{
  int imm_val = 0;
  switch(instruction.opcode) {
    case 0x03: // L-type
    case 0x13: // I-type
        imm_val = sign_extend_number(instruction.itype.imm,12); 
        break;
    case 0x23: // S-type
        imm_val = get_store_offset(instruction);
        break;
    case 0x37: // U-type
        imm_val = instruction.utype.imm;
        break;
    case 0x6f: // UJ-type
        imm_val = sign_extend_number(get_jump_offset(instruction), 21);
        break;
    case 0x63: // SB-type
        imm_val = sign_extend_number(get_branch_offset(instruction), 13);
        break;
    default: // R and undefined opcode
        break;
  };
  return imm_val;
}

// Generates all the control logic that flows around in the pipeline
idex_reg_t gen_control(Instruction instruction1, Instruction instruction2)
{
  idex_reg_t idex_reg = {0};
  switch(instruction1.opcode) {
    case 0x33:  // R-type
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
 switch(instruction2.opcode) {
    case 0x33:  // R-type
      idex_reg.ALUOpDUAL = 0x2;
      idex_reg.ALUSrcDUAL = 0;
      idex_reg.BranchDUAL = 0;
      idex_reg.Mem_ReadDUAL = 0;
      idex_reg.Memto_RegDUAL = 0;
      idex_reg.Mem_WriteDUAL = 0;
      idex_reg.Reg_WriteDUAL = 1;
      break;
    case 0x13: // I-type except load
      idex_reg.ALUOpDUAL = 0x3;
      idex_reg.ALUSrcDUAL = 1;
      idex_reg.BranchDUAL = 0;
      idex_reg.Mem_ReadDUAL = 0;
      idex_reg.Memto_RegDUAL = 0; 
      idex_reg.Mem_WriteDUAL = 0;
      idex_reg.Reg_WriteDUAL = 1;
      break;
    case 0x03: // Load
      idex_reg.ALUOpDUAL = 0;
      idex_reg.ALUSrcDUAL = 1;
      idex_reg.BranchDUAL = 0;
      idex_reg.Mem_ReadDUAL = 1;
      idex_reg.Memto_RegDUAL = 1;
      idex_reg.Mem_WriteDUAL = 0;
      idex_reg.Reg_WriteDUAL = 1;
      break;
    case 0x23: // Store
      idex_reg.ALUOpDUAL = 0;
      idex_reg.ALUSrcDUAL = 1;
      idex_reg.BranchDUAL = 0;
      idex_reg.Mem_ReadDUAL = 0;
      idex_reg.Memto_RegDUAL = 0;
      idex_reg.Mem_WriteDUAL = 1;
      idex_reg.Reg_WriteDUAL = 0;
      break;
    case 0x37: // U-type
      idex_reg.ALUOpDUAL = 0x4;
      idex_reg.ALUSrcDUAL = 1;
      idex_reg.BranchDUAL = 0;
      idex_reg.Mem_ReadDUAL = 0;
      idex_reg.Memto_RegDUAL = 0;
      idex_reg.Mem_WriteDUAL = 0;
      idex_reg.Reg_WriteDUAL = 1;
      break;
    case 0x6f: // J-type
      idex_reg.ALUOpDUAL = 0x5;
      idex_reg.ALUSrcDUAL = 0;
      idex_reg.BranchDUAL = 1;
      idex_reg.Mem_ReadDUAL = 0;
      idex_reg.Memto_RegDUAL = 0;
      idex_reg.Mem_WriteDUAL = 0;
      idex_reg.Reg_WriteDUAL = 1;
      break;
    case 0x63: // SB-type
      idex_reg.ALUOpDUAL = 0x1;
      idex_reg.ALUSrcDUAL = 0;
      idex_reg.BranchDUAL = 1;
      idex_reg.Mem_ReadDUAL = 0;
      idex_reg.Mem_WriteDUAL = 0;
      idex_reg.Reg_WriteDUAL = 0;
    default:  // Remaining opcodes
      break;
  }
  return idex_reg;
}

/// MEMORY STAGE HELPERS ///

// Evaluates whether a branch must be taken
bool gen_branch(uint32_t alu1, uint32_t alu2, uint32_t funct3)
{
  // Return true if bne or beq outcome is true and false otherwise
  if(alu1 == alu2 && (funct3 == 0x0))
  {
    return true;
  }
  else if(alu1 != alu2 && (funct3 == 0x1))
  {
    return true;
  }
  else{
    return false;
  }
}

/// PIPELINE FEATURES ///

void gen_forward(pipeline_regs_t* pregs_p, pipeline_wires_t* pwires_p)
{
  //By defualt, set both to 0. If there is no need for forwarding, A and B will exit function with value of 0
  pwires_p->forwardA = 0x0;
  pwires_p->forwardB = 0x0;

  if(pregs_p->exmem_preg.out.Reg_Write && (pregs_p->exmem_preg.out.rd != 0)){
    if(pregs_p->exmem_preg.out.rd == pregs_p->idex_preg.out.rs1){

      #ifdef DEBUG_CYCLE
      printf("[FWD]: Resolving EX hazard on rs1: x%d\n", pregs_p->idex_preg.out.rs1);
      #endif

      pwires_p->forwardA = 0x2;
    }
    if(pregs_p->exmem_preg.out.rd == pregs_p->idex_preg.out.rs2){

      #ifdef DEBUG_CYCLE
      printf("[FWD]: Resolving EX hazard on rs2: x%d\n", pregs_p->idex_preg.out.rs2);
      #endif

      pwires_p->forwardB = 0x2;
    }
  }
  if(pregs_p->memwb_preg.out.Reg_Write && (pregs_p->memwb_preg.out.rd != 0)){
    if ((pregs_p->memwb_preg.out.rd == pregs_p->idex_preg.out.rs1) &&
     !(pregs_p->exmem_preg.out.Reg_Write && (pregs_p->exmem_preg.out.rd != 0) && (pregs_p->exmem_preg.out.rd == pregs_p->idex_preg.out.rs1))){
      
      #ifdef DEBUG_CYCLE
      printf("[FWD]: Resolving MEM hazard on rs1: x%d\n", pregs_p->idex_preg.out.rs1);
      #endif

      pwires_p->forwardA = 0x1;
    }
    if ((pregs_p->memwb_preg.out.rd == pregs_p->idex_preg.out.rs2) &&
     !(pregs_p->exmem_preg.out.Reg_Write && (pregs_p->exmem_preg.out.rd != 0) && (pregs_p->exmem_preg.out.rd == pregs_p->idex_preg.out.rs2))){
      
      #ifdef DEBUG_CYCLE
      printf("[FWD]: Resolving MEM hazard on rs2: x%d\n", pregs_p->idex_preg.out.rs2);
      #endif

      pwires_p->forwardB = 0x1;
    }
  }

  //MUX for first ALU operand
  if (pwires_p->forwardA == 0x2) {
    pregs_p->idex_preg.out.rs1_val = pregs_p->exmem_preg.out.Read_Address;
    if (pregs_p->idex_preg.out.ALUSrc) { // store instruction
      pregs_p->idex_preg.out.Write_Address = pregs_p->idex_preg.out.rs2_val;
      pregs_p->idex_preg.out.rs2_val = pregs_p->idex_preg.out.imm;
    }
  } 
  else if (pwires_p->forwardA == 0x1) {
    if (pregs_p->memwb_preg.out.Memto_Reg) { // load instruction
      pregs_p->idex_preg.out.rs1_val = pregs_p->memwb_preg.out.Read_Data;
    } 
    else {
      pregs_p->idex_preg.out.rs1_val = pregs_p->memwb_preg.out.Read_Address;
    }
    if (pregs_p->idex_preg.out.ALUSrc) { // store instruction
      pregs_p->idex_preg.out.Write_Address = pregs_p->idex_preg.out.rs2_val;
      pregs_p->idex_preg.out.rs2_val = pregs_p->idex_preg.out.imm;
    }
  }

  //MUX for second ALU operand
  if (pwires_p->forwardB == 0x2) {
    pregs_p->idex_preg.out.rs2_val = pregs_p->exmem_preg.out.Read_Address;
    if (pregs_p->idex_preg.out.ALUSrc) { // store instruction
      pregs_p->idex_preg.out.Write_Address = pregs_p->idex_preg.out.rs2_val;
      pregs_p->idex_preg.out.rs2_val = pregs_p->idex_preg.out.imm;
    }
  } 
  else if (pwires_p->forwardB == 0x1) {
    if (pregs_p->memwb_preg.out.Memto_Reg) { // load instruction
      pregs_p->idex_preg.out.rs2_val = pregs_p->memwb_preg.out.Read_Data;
    } 
    else {
      pregs_p->idex_preg.out.rs2_val = pregs_p->memwb_preg.out.Read_Address;
    }
    if (pregs_p->idex_preg.out.ALUSrc) { // store instruction
      pregs_p->idex_preg.out.Write_Address = pregs_p->idex_preg.out.rs2_val;
      pregs_p->idex_preg.out.rs2_val = pregs_p->idex_preg.out.imm;
    }
  }
}

void detect_hazard(pipeline_regs_t* pregs_p, pipeline_wires_t* pwires_p, regfile_t* regfile_p)
{
  if (pregs_p->idex_preg.out.Mem_Read &&
    ((pregs_p->idex_preg.out.rd == pregs_p->ifid_preg.out.rs1) || 
    (pregs_p->idex_preg.out.rd == pregs_p->ifid_preg.out.rs2))) {

    // Stop PC and IF/ID register update
    pwires_p->PCWriteHZD = 1;
    pwires_p->IFIDWriteHZD = 1;
    pwires_p->ControlMUXHZD = 1;

    #ifdef DEBUG_CYCLE
    printf("[HZD]: Stalling and rewriting PC: 0x%08x\n", pregs_p->ifid_preg.inp.instr_addr);
    #endif
  }
  else {
    // If no hazard
    pwires_p->PCWriteHZD = 0;
    pwires_p->IFIDWriteHZD = 0;
    pwires_p->ControlMUXHZD = 0;
  }
}

uint64_t flush_pipeline(pipeline_regs_t* pregs_p, pipeline_wires_t* pwires_p, uint64_t branch_counter)
{
  // Flush Pipeline if Branch is taken
  if (pwires_p->pcsrc == 1){
    
    #ifdef DEBUG_CYCLE
    printf("[CPL]: Pipeline Flushed\n");
    #endif

    pregs_p->ifid_preg.inp.instr.ujtype.opcode = 0x13;
    pregs_p->ifid_preg.inp.instr.ujtype.rd = 0;
    pregs_p->ifid_preg.inp.instr.ujtype.imm = 0;
    pregs_p->ifid_preg.out.instr.ujtype.opcode = 0x13;
    pregs_p->ifid_preg.out.instr.ujtype.rd = 0;
    pregs_p->ifid_preg.out.instr.ujtype.imm = 0;

    pregs_p->idex_preg.inp.instr.rtype.opcode = 0x13;
    pregs_p->idex_preg.inp.instr.rtype.rd = 0;
    pregs_p->idex_preg.inp.instr.rtype.funct3 = 0;
    pregs_p->idex_preg.inp.instr.rtype.rs1 = 0;
    pregs_p->idex_preg.inp.instr.rtype.rs2 = 0;
    pregs_p->idex_preg.inp.instr.rtype.funct7 = 0;
    pregs_p->idex_preg.out.instr.rtype.opcode = 0x13;
    pregs_p->idex_preg.out.instr.rtype.rd = 0;
    pregs_p->idex_preg.out.instr.rtype.funct3 = 0;
    pregs_p->idex_preg.out.instr.rtype.rs1 = 0;
    pregs_p->idex_preg.out.instr.rtype.rs2 = 0;
    pregs_p->idex_preg.out.instr.rtype.funct7 = 0;

    pregs_p->exmem_preg.inp.instr.rtype.opcode = 0x13;
    pregs_p->exmem_preg.inp.instr.rtype.rd = 0;
    pregs_p->exmem_preg.inp.instr.rtype.funct3 = 0;
    pregs_p->exmem_preg.inp.instr.rtype.rs1 = 0;
    pregs_p->exmem_preg.inp.instr.rtype.rs2 = 0;
    pregs_p->exmem_preg.inp.instr.rtype.funct7 = 0;
    pregs_p->exmem_preg.out.instr.rtype.opcode = 0x13;
    pregs_p->exmem_preg.out.instr.rtype.rd = 0;
    pregs_p->exmem_preg.out.instr.rtype.funct3 = 0;
    pregs_p->exmem_preg.out.instr.rtype.rs1 = 0;
    pregs_p->exmem_preg.out.instr.rtype.rs2 = 0;
    pregs_p->exmem_preg.out.instr.rtype.funct7 = 0;

    // Clear control signals for the flushed stages
    pregs_p->idex_preg.inp.ALUOp = 0;
    pregs_p->idex_preg.inp.ALUSrc = 0;
    pregs_p->idex_preg.inp.Branch = 0;
    pregs_p->idex_preg.inp.Mem_Read = 0;
    pregs_p->idex_preg.inp.Mem_Write = 0;
    pregs_p->idex_preg.inp.Memto_Reg = 0;
    pregs_p->idex_preg.inp.Reg_Write = 0;
    pregs_p->idex_preg.out.ALUOp = 0;
    pregs_p->idex_preg.out.ALUSrc = 0;
    pregs_p->idex_preg.out.Branch = 0;
    pregs_p->idex_preg.out.Mem_Read = 0;
    pregs_p->idex_preg.out.Mem_Write = 0;
    pregs_p->idex_preg.out.Memto_Reg = 0;
    pregs_p->idex_preg.out.Reg_Write = 0;

    pregs_p->exmem_preg.inp.Branch = 0;
    pregs_p->exmem_preg.inp.Mem_Read = 0;
    pregs_p->exmem_preg.inp.Mem_Write = 0;
    pregs_p->exmem_preg.inp.Memto_Reg = 0;
    pregs_p->exmem_preg.inp.Reg_Write = 0;
    pregs_p->exmem_preg.out.Branch = 0;
    pregs_p->exmem_preg.out.Mem_Read = 0;
    pregs_p->exmem_preg.out.Mem_Write = 0;
    pregs_p->exmem_preg.out.Memto_Reg = 0;
    pregs_p->exmem_preg.out.Reg_Write = 0;

    //need to add for DUAL 
    return branch_counter + 1;
  }
  else { //If no hazard
    return branch_counter;
  }
}

uint32_t gen_alu_controlDUAL(idex_reg_t idex_reg)
{
  uint32_t alu_control = 0;
  switch (idex_reg.ALUOpDUAL) {
    case 0x0: // lw or sw
      alu_control = 0x2; // add
      break;
    case 0x1: // beq or bne
      alu_control = 0x6; // subtract
      break;
    case 0x2: // R-type
      switch (idex_reg.funct3DUAL)   {
        case 0x0:
          if (idex_reg.funct7DUAL == 0x0) {
            alu_control = 0x2; // add
          } else if (idex_reg.funct7DUAL == 0x20) {
            alu_control = 0x6; // subt
          } else if (idex_reg.funct7DUAL == 0x01) {
            alu_control = 0xC; // mul
            }
          break;
        case 0x1: 
          if (idex_reg.funct7DUAL == 0x0) {
            alu_control = 0x7; // sll
          } else if (idex_reg.funct7DUAL == 0x01) {
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
          if (idex_reg.funct7DUAL == 0x0) {
            alu_control = 0xA; // srl
          } else if (idex_reg.funct7DUAL == 0x20) {
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
    case 0x3: // I-type
	    switch (idex_reg.funct3DUAL) {
        case 0x0: 
          alu_control = 0x2; // addi
          break;
        case 0x1: 
          alu_control = 0x7; // slli
          break;
        case 0x2: 
          alu_control = 0x8; // slti
          break;
        case 0x4: 
          alu_control = 0x9; // xori
          break;
        case 0x5:
            if ((idex_reg.immDUAL>> 5) == 0x0) {
              alu_control = 0xA; // srli
            } else if ((idex_reg.immDUAL >> 5) == 0x20) {
              alu_control = 0xB; // srai
            }
            break;
        case 0x6: 
          alu_control = 0x1; // ori
          break;
        case 0x7: 
          alu_control = 0x0; // andi
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

// Check if the two instructions are able to be executed together
// Cannot be done if there are hazards as explained
bool dualIssue_hazard_check(exmem_reg_t instruction)
{
  if(instruction.instr.opcode != 0x03 && instruction.instr.opcode != 0x23) {
    instruction.ins1type = 1; //1 represents ALU/Branch type
  }
  else if(instruction.instr.opcode == 0x03 || instruction.instr.opcode == 0x23) {
    instruction.ins1type = 2; //2 represents Load/Store type
  }

  if(instruction.instrDUAL.opcode != 0x03 && instruction.instrDUAL.opcode != 0x23) {
    instruction.ins2type = 1; //1 represents ALU/Branch type
  }
  else if(instruction.instrDUAL.opcode == 0x03 || instruction.instrDUAL.opcode == 0x23) {
    instruction.ins2type = 2; //2 represents Load/Store type
  }

  if(instruction.ins2type == instruction.ins1type) { // check for structural hazard
    return false;
  }
  else { //check for data hazard
    if(instruction.Reg_Write && (instruction.rd != 0)){
      if(instruction.rd == instruction.rs1DUAL || instruction.rd == instruction.rs2DUAL){
        return false;
      }
      else { //check for control hazard
        if((instruction.Branch & instruction.zero) == true) {
          return false;
        }
        else {
          return true;
        }
      }
    }
    else { //check for control hazard
      if((instruction.Branch & instruction.zero) == true) {
        return false;
      }
      else {
        return true;
      }
    }
  }
}

// Resolves hazard between the second instruction of one cycle and the second of the next
// As dictated from the above function, we will not consider hazards between two instructions in the same cycle
void gen_forwardDUAL(pipeline_regs_t* pregs_p, pipeline_wires_t* pwires_p)
{
  //By defualt, set both to 0. If there is no need for forwarding, A and B will exit function with value of 0
  pwires_p->forwardADUAL = 0x0;
  pwires_p->forwardBDUAL = 0x0;

  if(pregs_p->memwb_preg.out.Reg_WriteDUAL && (pregs_p->memwb_preg.out.rdDUAL != 0)){
    if((pregs_p->memwb_preg.out.rdDUAL == pregs_p->idex_preg.out.rs1DUAL) &&
     !(pregs_p->exmem_preg.out.Reg_WriteDUAL && (pregs_p->exmem_preg.out.rdDUAL != 0) &&
      (pregs_p->exmem_preg.out.rdDUAL == pregs_p->idex_preg.out.rs1DUAL))){
      
      #ifdef DEBUG_CYCLE
      printf("[FWD]: Resolving MEM hazard on rs1: x%d\n", pregs_p->idex_preg.out.rs1DUAL);
      #endif

      pwires_p->forwardADUAL = 0x1;
    }
    if((pregs_p->memwb_preg.out.rdDUAL == pregs_p->idex_preg.out.rs2DUAL) &&
     !(pregs_p->exmem_preg.out.Reg_WriteDUAL && (pregs_p->exmem_preg.out.rdDUAL != 0) &&
      (pregs_p->exmem_preg.out.rdDUAL == pregs_p->idex_preg.out.rs2DUAL))){
      
      #ifdef DEBUG_CYCLE
      printf("[FWD]: Resolving MEM hazard on rs2: x%d\n", pregs_p->idex_preg.out.rs2DUAL);
      #endif

      pwires_p->forwardBDUAL = 0x1;
    }
  }

  //MUX for first ALU operand
  if (pwires_p->forwardADUAL == 0x1) {
    if (pregs_p->memwb_preg.out.Memto_RegDUAL) { // load instruction
      pregs_p->idex_preg.out.rs1_valDUAL = pregs_p->memwb_preg.out.Read_DataDUAL;
    } 
    else {
      pregs_p->idex_preg.out.rs1_valDUAL = pregs_p->memwb_preg.out.Read_AddressDUAL;
    }
    if (pregs_p->idex_preg.out.ALUSrcDUAL) { // store instruction
      pregs_p->idex_preg.out.Write_AddressDUAL = pregs_p->idex_preg.out.rs2_valDUAL;
      pregs_p->idex_preg.out.rs2_valDUAL = pregs_p->idex_preg.out.immDUAL;
    }
  }

  //MUX for second ALU operand
  if (pwires_p->forwardBDUAL== 0x1) {
    if (pregs_p->memwb_preg.out.Memto_RegDUAL) { // load instruction
      pregs_p->idex_preg.out.rs2_valDUAL = pregs_p->memwb_preg.out.Read_DataDUAL;
    } 
    else {
      pregs_p->idex_preg.out.rs2_valDUAL = pregs_p->memwb_preg.out.Read_AddressDUAL;
    }
    if (pregs_p->idex_preg.out.ALUSrcDUAL) { // store instruction
      pregs_p->idex_preg.out.Write_AddressDUAL = pregs_p->idex_preg.out.rs2_valDUAL;
      pregs_p->idex_preg.out.rs2_valDUAL = pregs_p->idex_preg.out.immDUAL;
    }
  }
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