#include <stdbool.h>
#include "cache.h"
#include "riscv.h"
#include "types.h"
#include "utils.h"
#include "pipeline.h"
#include "stage_helpers.h"

uint64_t total_cycle_counter = 0;
uint64_t mem_access_counter = 0;
uint64_t miss_count = 0;
uint64_t hit_count = 0;
uint64_t stall_counter = 0;
uint64_t branch_counter = 0;
uint64_t fwd_exex_counter = 0;
uint64_t fwd_exmem_counter= 0;
simulator_config_t sim_config = {0};

///////////////////////////////////////////////////////////////////////////////

void bootstrap(pipeline_wires_t* pwires_p, pipeline_regs_t* pregs_p, regfile_t* regfile_p)
{
  // PC src must get the same value as the default PC value
  pwires_p->pc_src0 = regfile_p->PC;
}

///////////////////////////
/// STAGE FUNCTIONALITY ///
///////////////////////////

/**
 * STAGE  : stage_fetch
 * output : ifid_reg_t
 **/ 
ifid_reg_t stage_fetch(pipeline_wires_t* pwires_p, regfile_t* regfile_p, Byte* memory_p)
{
  ifid_reg_t ifid_reg = {0};
 
  // MUX
  if(pwires_p->pcsrc){ 
      regfile_p->PC = pwires_p->pc_src1; // PC+offset
  } else {
      regfile_p->PC = pwires_p->pc_src0; // PC+4
  }

  //Hazard check
  if(pwires_p->PCWriteHZD == 1)
  {
    regfile_p->PC = regfile_p->PC - 4; // Re-fetch instruction
    pwires_p->pc_src0 = regfile_p->PC; 
    pwires_p->PCWriteHZD = 0;
  }

  pwires_p->pc_src0 = regfile_p->PC+8; // Next set of instructions

  // Get instruction from memory
  uint32_t instruction_bits = *(uint32_t *)(memory_p + regfile_p->PC);

  ifid_reg.instr.bits = instruction_bits;
  ifid_reg.instr_addr = regfile_p->PC;
  ifid_reg.rs1 = (instruction_bits >> 15) & ((1U << 5) - 1);
  ifid_reg.rs2 = (instruction_bits >> 20) & ((1U << 5)  -1);

  // Dual issue instruction data
  uint32_t instruction_bitsDUAL = *(uint32_t *)(memory_p + regfile_p->PC+4);
  ifid_reg.instrDUAL.bits = instruction_bitsDUAL;
  ifid_reg.instr_addrDUAL = regfile_p->PC+4;
  ifid_reg.rs1DUAL = (instruction_bitsDUAL >> 15) & ((1U << 5) - 1);
  ifid_reg.rs2DUAL = (instruction_bitsDUAL >> 20) & ((1U << 5)  -1);
  #ifdef DEBUG_CYCLE
  printf("[IF ]: Instruction [%08x]@[%08x]: ", instruction_bitsDUAL, regfile_p->PC+4);
  decode_instruction(instruction_bitsDUAL);
  #endif

  #ifdef DEBUG_CYCLE
  printf("[IF ]: Instruction [%08x]@[%08x]: ", instruction_bits, regfile_p->PC);
  decode_instruction(instruction_bits);
  #endif
  return ifid_reg;
}

/**
 * STAGE  : stage_decode
 * output : idex_reg_t
 **/ 
idex_reg_t stage_decode(ifid_reg_t ifid_reg, pipeline_wires_t* pwires_p, regfile_t* regfile_p)
{
  idex_reg_t idex_reg = {0};

  // updating idex_reg
  idex_reg = gen_control(ifid_reg.instr, ifid_reg.instrDUAL);

  // flush the control if hazard detected
  if(pwires_p->ControlMUXHZD == 1)
  {
    idex_reg.ALUOp = 0;
    idex_reg.ALUSrc = 0;
    idex_reg.Branch = 0;
    idex_reg.Mem_Read = 0;
    idex_reg.Memto_Reg = 0;
    idex_reg.Mem_Write = 0;
    idex_reg.Reg_Write = 0;

    pwires_p->ControlMUXHZD = 0;
  }

  idex_reg.instr = ifid_reg.instr;  
  idex_reg.instr.bits = ifid_reg.instr.bits;
  idex_reg.instr_addr = ifid_reg.instr_addr;

  //some may be redundant and useless code
  switch((idex_reg.instr.bits) & ((1U << 7) - 1)) {
  case 0x33:
  case 0x23:
    idex_reg.rs1 = (idex_reg.instr.bits >> 15) & ((1U << 5) - 1);
    idex_reg.rs2 = (idex_reg.instr.bits >> 20) & ((1U << 5)  -1);
    idex_reg.rs1_val = regfile_p->R[idex_reg.rs1];
    idex_reg.rs2_val = regfile_p->R[idex_reg.rs2];
    break;
  case 0x63:
    idex_reg.rs1 = (idex_reg.instr.bits >> 15)&((1U << 5) - 1);
    idex_reg.rs2 = (idex_reg.instr.bits >> 20)&((1U << 5) - 1);
    idex_reg.rs1_val = regfile_p->R[idex_reg.rs1];
    idex_reg.rs2_val = regfile_p->R[idex_reg.rs2];
    break;
  case 0x03:
  case 0x13:
    idex_reg.rs1 = (idex_reg.instr.bits >> 15) & ((1U << 5) - 1);
    idex_reg.rs1_val = regfile_p->R[idex_reg.rs1];
    break;
  default:
    break;
  }

  idex_reg.imm = gen_imm(idex_reg.instr);
  idex_reg.rd = (idex_reg.instr.bits >> 7) & ((1U << 5) - 1);
  idex_reg.funct7 = (((idex_reg.instr.bits >> 25) & ((1U<<7)-1)));
  idex_reg.funct3 = ((idex_reg.instr.bits >> 12) & ((1U << 3) - 1));

  // repeat everything for dual instruction
  idex_reg.instrDUAL = ifid_reg.instrDUAL;  
  idex_reg.instrDUAL.bits = ifid_reg.instrDUAL.bits;
  idex_reg.instr_addrDUAL = ifid_reg.instr_addrDUAL;

  switch((idex_reg.instrDUAL.bits) & ((1U << 7) - 1)) {
  case 0x33:
  case 0x23:
    idex_reg.rs1DUAL = (idex_reg.instrDUAL.bits >> 15) & ((1U << 5) - 1);
    idex_reg.rs2DUAL = (idex_reg.instrDUAL.bits >> 20) & ((1U << 5)  -1);
    idex_reg.rs1_valDUAL = regfile_p->R[idex_reg.rs1DUAL];
    idex_reg.rs2_valDUAL = regfile_p->R[idex_reg.rs2DUAL];
    break;
  case 0x63:
    idex_reg.rs1DUAL = (idex_reg.instrDUAL.bits >> 15)&((1U << 5) - 1);
    idex_reg.rs2DUAL = (idex_reg.instrDUAL.bits >> 20)&((1U << 5) - 1);
    idex_reg.rs1_valDUAL = regfile_p->R[idex_reg.rs1DUAL];
    idex_reg.rs2_valDUAL = regfile_p->R[idex_reg.rs2DUAL];
    break;
  case 0x03:
  case 0x13:
    idex_reg.rs1DUAL = (idex_reg.instrDUAL.bits >> 15) & ((1U << 5) - 1);
    idex_reg.rs1_valDUAL = regfile_p->R[idex_reg.rs1DUAL];
    break;
  default:
    break;
  }

  idex_reg.immDUAL = gen_imm(idex_reg.instrDUAL);
  idex_reg.rdDUAL = (idex_reg.instrDUAL.bits >> 7) & ((1U << 5) - 1);
  idex_reg.funct7DUAL = (((idex_reg.instrDUAL.bits >> 25) & ((1U<<7)-1)));
  idex_reg.funct3DUAL = ((idex_reg.instrDUAL.bits >> 12) & ((1U << 3) - 1));

  #ifdef DEBUG_CYCLE
  printf("[ID ]: Instruction [%08x]@[%08x]: ", ifid_reg.instrDUAL.bits, ifid_reg.instr_addrDUAL);
  decode_instruction(ifid_reg.instrDUAL.bits);
  #endif

  #ifdef DEBUG_CYCLE
  printf("[ID ]: Instruction [%08x]@[%08x]: ", ifid_reg.instr.bits, ifid_reg.instr_addr);
  decode_instruction(ifid_reg.instr.bits);
  #endif
  return idex_reg;
}

/**
 * STAGE  : stage_execute
 * output : exmem_reg_t
 **/ 
exmem_reg_t stage_execute(idex_reg_t idex_reg, pipeline_wires_t* pwires_p)
{
  exmem_reg_t exmem_reg = {0};

  if (pwires_p->forwardA || pwires_p->forwardB) {
    exmem_reg.Write_Address = idex_reg.Write_Address;
  } 
  else {
    exmem_reg.Write_Address = idex_reg.rs2_val;
  }

  if (pwires_p->forwardA == 0x0 && idex_reg.ALUOp == 0x5) {
    idex_reg.rs1_val = idex_reg.instr_addr;
  }
  if (pwires_p->forwardA == 0x0 && idex_reg.ALUSrc == 1) {
    idex_reg.rs2_val = idex_reg.imm;
  }

  //increment counters
  if (pwires_p->forwardA == 0x1) {
    fwd_exmem_counter++;
  }
  else if (pwires_p->forwardA == 0x2) {
    fwd_exex_counter++;
  }

  if (pwires_p->forwardB == 0x1) {
    fwd_exmem_counter++;
  }
  else if (pwires_p->forwardB == 0x2) {
    fwd_exex_counter++;
  }

  if(idex_reg.instr.bits == 0x63) { //gen_branch here to ensure forwarded data
    idex_reg.Branch = gen_branch(idex_reg.rs1_val, idex_reg.rs2_val, idex_reg.funct3);
  }
  
  //ALU execution
  idex_reg.alu_control = gen_alu_control(idex_reg);
  exmem_reg.Read_Address = execute_alu(idex_reg.rs1_val, idex_reg.rs2_val, idex_reg.alu_control);

  if ((idex_reg.ALUOp == 0x1) && (exmem_reg.Read_Address == 0) && (idex_reg.funct3 == 0x0)) { // beq
    exmem_reg.zero = 1;
  }
  else if ((idex_reg.ALUOp == 0x1) && (exmem_reg.Read_Address != 0) && (idex_reg.funct3 == 0x1)) { // bne
    exmem_reg.zero = 1;
  }
  else if (idex_reg.ALUOp == 0x5) { // jal
	  exmem_reg.zero = 1;
  }
  else {
    exmem_reg.zero = 0;
  }
  
// adder
if (idex_reg.Branch) {
  exmem_reg.instr_addr_imm = idex_reg.instr_addr + idex_reg.imm;
} else {
  exmem_reg.instr_addr_imm = idex_reg.instr_addr;
}

  exmem_reg.instr_addr = idex_reg.instr_addr; 

  // Pass to exmem
  exmem_reg.rd = idex_reg.rd;
  exmem_reg.rs1_val = idex_reg.rs1_val;
  exmem_reg.rs2_val = idex_reg.rs2_val;
  exmem_reg.imm = idex_reg.imm;
  exmem_reg.instr = idex_reg.instr;  
  exmem_reg.instr.bits = idex_reg.instr.bits;
  exmem_reg.funct3 = idex_reg.funct3;

  exmem_reg.Mem_Read = idex_reg.Mem_Read;
  exmem_reg.Mem_Write = idex_reg.Mem_Write;
  exmem_reg.Memto_Reg = idex_reg.Memto_Reg;
  exmem_reg.Reg_Write = idex_reg.Reg_Write;
  exmem_reg.Branch = idex_reg.Branch;

  // assign needed dual values to perform check before executing
  exmem_reg.instrDUAL = idex_reg.instrDUAL;
  exmem_reg.rs1DUAL = idex_reg.rs1DUAL;
  exmem_reg.rs2DUAL = idex_reg.rs2DUAL; 
  exmem_reg.dualHazard = dualIssue_hazard_check(exmem_reg);
  if(exmem_reg.dualHazard){
    //execute dual
    if (pwires_p->forwardADUAL || pwires_p->forwardBDUAL) {
      exmem_reg.Write_AddressDUAL = idex_reg.Write_AddressDUAL;
    } 
    else {
      exmem_reg.Write_AddressDUAL = idex_reg.rs2_valDUAL;
    }

    if (pwires_p->forwardADUAL == 0x0 && idex_reg.ALUOpDUAL == 0x5) {
      idex_reg.rs1_valDUAL = idex_reg.instr_addrDUAL;
    }
    if (pwires_p->forwardADUAL == 0x0 && idex_reg.ALUSrcDUAL == 1) {
      idex_reg.rs2_valDUAL = idex_reg.immDUAL;
    }

    //increment counters
    if (pwires_p->forwardADUAL == 0x1) {
      fwd_exmem_counter++;
    }
    if (pwires_p->forwardBDUAL == 0x1) {
      fwd_exmem_counter++;
    }

    if(idex_reg.instrDUAL.bits == 0x63) { //gen_branch here to ensure forwarded data
      idex_reg.BranchDUAL = gen_branch(idex_reg.rs1_valDUAL, idex_reg.rs2_valDUAL, idex_reg.funct3DUAL);
    }
    
    //ALU execution
    idex_reg.alu_controlDUAL = gen_alu_controlDUAL(idex_reg);
    exmem_reg.Read_AddressDUAL = execute_alu(idex_reg.rs1_valDUAL, idex_reg.rs2_valDUAL, idex_reg.alu_controlDUAL);

    if ((idex_reg.ALUOpDUAL == 0x1) && (exmem_reg.Read_AddressDUAL == 0) && (idex_reg.funct3DUAL == 0x0)) { // beq
      exmem_reg.zeroDUAL = 1;
    }
    else if ((idex_reg.ALUOpDUAL == 0x1) && (exmem_reg.Read_AddressDUAL != 0) && (idex_reg.funct3DUAL == 0x1)) { // bne
      exmem_reg.zeroDUAL = 1;
    }
    else if (idex_reg.ALUOp == 0x5) { // jal
      exmem_reg.zeroDUAL = 1;
    }
    else {
      exmem_reg.zeroDUAL = 0;
    }
    
    // adder
    if (idex_reg.BranchDUAL) {
      exmem_reg.instr_addr_immDUAL = idex_reg.instr_addrDUAL + idex_reg.immDUAL;
    } else {
      exmem_reg.instr_addr_immDUAL = idex_reg.instr_addrDUAL;
    }
    exmem_reg.instr_addrDUAL = idex_reg.instr_addrDUAL; 

    // Pass to exmem
    exmem_reg.rdDUAL = idex_reg.rdDUAL;
    exmem_reg.rs1_valDUAL = idex_reg.rs1_valDUAL;
    exmem_reg.rs2_valDUAL = idex_reg.rs2_valDUAL;
    exmem_reg.immDUAL = idex_reg.immDUAL;
    exmem_reg.instrDUAL.bits = idex_reg.instrDUAL.bits;
    exmem_reg.funct3DUAL = idex_reg.funct3DUAL;

    exmem_reg.Mem_ReadDUAL = idex_reg.Mem_ReadDUAL;
    exmem_reg.Mem_WriteDUAL = idex_reg.Mem_WriteDUAL;
    exmem_reg.Memto_RegDUAL = idex_reg.Memto_RegDUAL;
    exmem_reg.Reg_WriteDUAL = idex_reg.Reg_WriteDUAL;
    exmem_reg.BranchDUAL = idex_reg.BranchDUAL; 
    #ifdef DEBUG_CYCLE
    printf("[EX ]: Instruction [%08x]@[%08x]: ", idex_reg.instrDUAL.bits, idex_reg.instr_addrDUAL);
    decode_instruction(idex_reg.instrDUAL.bits);
    #endif
  }
  else {
    /*
    perform stall by splitting in 2 packets:
    1. update exmem_regs by assigning dual values
    2. set dual values to 0
    3. update pc to stall it and not fetch another instruction
    */
    exmem_reg.Mem_ReadDUAL = 0;
  }

  #ifdef DEBUG_CYCLE
  printf("[EX ]: Instruction [%08x]@[%08x]: ", idex_reg.instr.bits, idex_reg.instr_addr);
  decode_instruction(idex_reg.instr.bits);
  #endif
  return exmem_reg;
}

/**
 * STAGE  : stage_mem
 * output : memwb_reg_t
 **/ 
/*
Only for Load/Store
stage_mem: This function has access to exmem_reg, pipeline wires, and data memory. It
works on accessing the data memory and passing down the values to memwb_reg (of
memwb_reg_t type).
*/
memwb_reg_t stage_mem(exmem_reg_t exmem_reg, pipeline_wires_t* pwires_p, Byte* memory_p, Cache* cache_p)
{
  memwb_reg_t memwb_reg = {0};

  if (exmem_reg.Mem_Read) {
    switch (exmem_reg.funct3) {
        case 0x0: // lb
            exmem_reg.contents = sign_extend_number(load(memory_p, exmem_reg.Read_Address, LENGTH_BYTE), 8);
            break;
        case 0x1: // lh
            exmem_reg.contents = sign_extend_number(load(memory_p, exmem_reg.Read_Address, LENGTH_HALF_WORD), 16);
            break;
        case 0x2: // lw
            exmem_reg.contents = load(memory_p, exmem_reg.Read_Address, LENGTH_WORD);
            break;
        default:
            exmem_reg.contents = 0; // invalid funct3
            break;
    }
    memwb_reg.Read_Data = exmem_reg.contents;
    memwb_reg.Read_Address = exmem_reg.Read_AddressDUAL;
  }
  else if(exmem_reg.Mem_ReadDUAL) {
    switch (exmem_reg.funct3DUAL) {
        case 0x0: // lb
            exmem_reg.contents = sign_extend_number(load(memory_p, exmem_reg.Read_AddressDUAL, LENGTH_BYTE), 8);
            break;
        case 0x1: // lh
            exmem_reg.contents = sign_extend_number(load(memory_p, exmem_reg.Read_AddressDUAL, LENGTH_HALF_WORD), 16);
            break;
        case 0x2: // lw
            exmem_reg.contents = load(memory_p, exmem_reg.Read_AddressDUAL, LENGTH_WORD);
            break;
        default:
            exmem_reg.contents = 0; // invalid funct3
            break;
    }
    memwb_reg.Read_Data = exmem_reg.contents;
    memwb_reg.Read_Address = exmem_reg.Read_Address;
  }
  else if (exmem_reg.Mem_Write) {
    memwb_reg.Read_Address = exmem_reg.Read_Address;
    switch (exmem_reg.funct3) {
        case 0x0: // sb
            store(memory_p, exmem_reg.Read_Address, LENGTH_BYTE, exmem_reg.Write_Address & 0xFF);
            break;
        case 0x1: // sh
            store(memory_p, exmem_reg.Read_Address, LENGTH_HALF_WORD, exmem_reg.Write_Address & 0xFFFF);
            break;
        case 0x2: // sw
            store(memory_p, exmem_reg.Read_Address, LENGTH_WORD, exmem_reg.Write_Address);
            break;
        default:
            break;
    }
  }
  else if (exmem_reg.Mem_WriteDUAL) {
    memwb_reg.Read_Address = exmem_reg.Read_AddressDUAL;
    switch (exmem_reg.funct3DUAL) {
        case 0x0: // sb
            store(memory_p, exmem_reg.Read_AddressDUAL, LENGTH_BYTE, exmem_reg.Write_AddressDUAL & 0xFF);
            break;
        case 0x1: // sh
            store(memory_p, exmem_reg.Read_AddressDUAL, LENGTH_HALF_WORD, exmem_reg.Write_AddressDUAL & 0xFFFF);
            break;
        case 0x2: // sw
            store(memory_p, exmem_reg.Read_AddressDUAL, LENGTH_WORD, exmem_reg.Write_AddressDUAL);
            break;
        default:
            break;
    }
  }
  else {
    memwb_reg.Read_Address = exmem_reg.Read_Address;
  }

  memwb_reg.rd = exmem_reg.rd;
  memwb_reg.rs1_val = exmem_reg.rs1_val;
  memwb_reg.imm = exmem_reg.imm;
  
  memwb_reg.Memto_Reg = exmem_reg.Memto_Reg;
  memwb_reg.Reg_Write = exmem_reg.Reg_Write;
  memwb_reg.instr = exmem_reg.instr;
  memwb_reg.instr_addr = exmem_reg.instr_addr;
  memwb_reg.instr_addr_imm = exmem_reg.instr_addr_imm;

  // Return pc_src1 to IF MUX
  pwires_p->pc_src1 = memwb_reg.instr_addr_imm;

  #ifdef CACHE_ENABLE
  uint32_t address;
  uint32_t latency;
  if (exmem_reg.Mem_Write || exmem_reg.Mem_Read) {
    address = memwb_reg.rs1_val + memwb_reg.imm;
    if (processCacheOperation(address, cache_p) == CACHE_HIT_LATENCY) {
      latency = CACHE_HIT_LATENCY; 
      total_cycle_counter += (CACHE_HIT_LATENCY - 1);
      hit_count++;
    } else {
      latency = CACHE_MISS_LATENCY;
      total_cycle_counter += (CACHE_MISS_LATENCY - 1);
      miss_count++;
    }
    
    #ifdef PRINT_CACHE_TRACES
    printf("[MEM]: Cache latency at addr: 0x%08x: %d cycles\n", address, latency);
    #endif
  }  
  #endif

  if(exmem_reg.dualHazard) {
    memwb_reg.rdDUAL = exmem_reg.rdDUAL;
    memwb_reg.rs1_valDUAL = exmem_reg.rs1_valDUAL;
    memwb_reg.immDUAL = exmem_reg.immDUAL;
    
    memwb_reg.Memto_RegDUAL = exmem_reg.Memto_RegDUAL;
    memwb_reg.Reg_WriteDUAL = exmem_reg.Reg_WriteDUAL;
    memwb_reg.instrDUAL = exmem_reg.instrDUAL;
    memwb_reg.instr_addrDUAL = exmem_reg.instr_addrDUAL;
    memwb_reg.instr_addr_immDUAL = exmem_reg.instr_addr_immDUAL;

    //Create pcsrc wire
    pwires_p->pcsrc = exmem_reg.BranchDUAL & exmem_reg.zeroDUAL;

    // Return pc_src1 to IF MUX
    pwires_p->pc_src1 = memwb_reg.instr_addr_immDUAL;    

    #ifdef DEBUG_CYCLE
    printf("[MEM]: Instruction [%08x]@[%08x]: ", exmem_reg.instrDUAL.bits, exmem_reg.instr_addrDUAL);
    decode_instruction(exmem_reg.instrDUAL.bits);
    #endif

    /* NEED FIXING FOR DUAL ISSUE
    #ifdef CACHE_ENABLE
    uint32_t address;
    uint32_t latency;
    if (exmem_reg.Mem_Write || exmem_reg.Mem_Read) {
      address = memwb_reg.rs1_val + memwb_reg.imm;
      if (processCacheOperation(address, cache_p) == CACHE_HIT_LATENCY) {
        latency = CACHE_HIT_LATENCY; 
        total_cycle_counter += (CACHE_HIT_LATENCY - 1);
        hit_count++;
      } else {
        latency = CACHE_MISS_LATENCY;
        total_cycle_counter += (CACHE_MISS_LATENCY - 1);
        miss_count++;
      }
      
      #ifdef PRINT_CACHE_TRACES
      printf("[MEM]: Cache latency at addr: 0x%08x: %d cycles\n", address, latency);
      #endif
    }  
    #endif 
    */
  }
  else { //cannot have branch on 1st instruction when dual as that is control hazard between the two instructions
    pwires_p->pcsrc = exmem_reg.Branch & exmem_reg.zero;
    pwires_p->pc_src1 = memwb_reg.instr_addr_imm; 
  }
  memwb_reg.dualHazard = exmem_reg.dualHazard;

  #ifdef DEBUG_CYCLE
  printf("[MEM]: Instruction [%08x]@[%08x]: ", exmem_reg.instr.bits, exmem_reg.instr_addr);
  decode_instruction(exmem_reg.instr.bits);
  #endif

  return memwb_reg;
}

/**
 * STAGE  : stage_writeback
 * output : nothing - The state of the register file may be changed
 **/ 
/*
Only for Load, arithmetic, logical instructions
stage_writeback: This function has access to memwb_reg, pipeline wires, and register file.
It is working on writing the results to the destination register (rd).
*/
void stage_writeback(memwb_reg_t memwb_reg, pipeline_wires_t* pwires_p, regfile_t* regfile_p)
{
  // Only write back if Reg_Write is true
  if (memwb_reg.Reg_Write) {
    // Determine whether to write from Read_Data or Read_Address
    if (memwb_reg.Memto_Reg) {
      memwb_reg.Write_Data = memwb_reg.Read_Data;
    } else {
      memwb_reg.Write_Data = memwb_reg.Read_Address; // stored in Read_Address
    }

    // Write data to register
    if (memwb_reg.rd != 0) { // Avoid writing to register x0
      regfile_p->R[memwb_reg.rd] = memwb_reg.Write_Data;
    }
  }

  if(memwb_reg.dualHazard) {
    // Only write back if Reg_Write is true
    if (memwb_reg.Reg_WriteDUAL) {
      // Determine whether to write from Read_Data or Read_Address
      if (memwb_reg.Memto_RegDUAL) {
        memwb_reg.Write_DataDUAL = memwb_reg.Read_DataDUAL;
      } else {
        memwb_reg.Write_DataDUAL = memwb_reg.Read_AddressDUAL; // stored in Read_Address
      }

      // Write data to register
      if (memwb_reg.rdDUAL != 0) { // Avoid writing to register x0
        regfile_p->R[memwb_reg.rdDUAL] = memwb_reg.Write_DataDUAL;
      }
    }
    #ifdef DEBUG_CYCLE
    printf("[WB ]: Instruction [%08x]@[%08x]: ", memwb_reg.instrDUAL.bits, memwb_reg.instr_addrDUAL);
    decode_instruction(memwb_reg.instrDUAL.bits);
    #endif
  }

  #ifdef DEBUG_CYCLE
  printf("[WB ]: Instruction [%08x]@[%08x]: ", memwb_reg.instr.bits, memwb_reg.instr_addr );
  decode_instruction(memwb_reg.instr.bits);
  #endif
}

///////////////////////////////////////////////////////////////////////////////

/** 
 * excite the pipeline with one clock cycle
 **/
void cycle_pipeline(regfile_t* regfile_p, Byte* memory_p, Cache* cache_p, pipeline_regs_t* pregs_p, pipeline_wires_t* pwires_p, bool* ecall_exit)
{
  #ifdef DEBUG_CYCLE
  printf("v==============");
  printf("Cycle Counter = %5ld", total_cycle_counter);
  printf("==============v\n\n");
  #endif

  // process each stage

  /* Output               |    Stage      |       Inputs  */
  pregs_p->ifid_preg.inp  = stage_fetch     (pwires_p, regfile_p, memory_p);

  detect_hazard(pregs_p, pwires_p, regfile_p);
  
  #ifdef PRINT_STATS // only runs for defined configs
  if(pwires_p->IFIDWriteHZD == 1) {
    stall_counter++;
    pregs_p->ifid_preg.inp = pregs_p->ifid_preg.out;
    pwires_p->IFIDWriteHZD = 0;
  }
  #endif

  pregs_p->idex_preg.inp  = stage_decode    (pregs_p->ifid_preg.out, pwires_p, regfile_p);
  
  #ifdef PRINT_STATS // only runs for defined configs
  gen_forward(pregs_p, pwires_p);
  gen_forwardDUAL(pregs_p, pwires_p);
  #endif

  pregs_p->exmem_preg.inp = stage_execute   (pregs_p->idex_preg.out, pwires_p);

  pregs_p->memwb_preg.inp = stage_mem       (pregs_p->exmem_preg.out, pwires_p, memory_p, cache_p);

                            stage_writeback (pregs_p->memwb_preg.out, pwires_p, regfile_p);

  #ifdef PRINT_STATS // only runs for defined configs
  branch_counter = flush_pipeline(pregs_p, pwires_p, branch_counter);
  #endif

  // update all the output registers for the next cycle from the input registers in the current cycle
  pregs_p->ifid_preg.out  = pregs_p->ifid_preg.inp;
  pregs_p->idex_preg.out  = pregs_p->idex_preg.inp;
  pregs_p->exmem_preg.out = pregs_p->exmem_preg.inp;
  pregs_p->memwb_preg.out = pregs_p->memwb_preg.inp;

  /////////////////// NO CHANGES BELOW THIS ARE REQUIRED //////////////////////

  // increment the cycle
  total_cycle_counter++;

  #ifdef DEBUG_REG_TRACE
  print_register_trace(regfile_p);
  #endif

  /**
   * check ecall condition
   * To do this, the value stored in R[10] (a0 or x10) should be 10.
   * Hence, the ecall condition is checked by the existence of following
   * two instructions in sequence:
   * 1. <instr>  x10, <val1>, <val2> 
   * 2. ecall
   * 
   * The first instruction must write the value 10 to x10.
   * The second instruction is the ecall (opcode: 0x73)
   * 
   * The condition checks whether the R[10] value is 10 when the
   * `memwb_reg.instr.opcode` == 0x73 (to propagate the ecall)
   * 
   * If more functionality on ecall needs to be added, it can be done
   * by adding more conditions on the value of R[10]
   */
  if( (pregs_p->memwb_preg.out.instr.bits == 0x00000073) &&
      (regfile_p->R[10] == 10) )
  {
    *(ecall_exit) = true;
  }
}
