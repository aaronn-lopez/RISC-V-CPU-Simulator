#include <stdbool.h>
#include "cache.h"
#include "riscv.h"
#include "types.h"
#include "utils.h"
#include "pipeline.h"
#include "stage_helpers.h"

uint64_t total_cycle_counter = 0;
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

  // Get instruction from memory
  uint32_t instruction_bits = *(uint32_t *)(memory_p + regfile_p->PC);

  #ifdef DEBUG_CYCLE
  printf("[IF ]: Instruction [%08x]@[%08x]: ", instruction_bits, regfile_p->PC);
  decode_instruction(instruction_bits);
  #endif
  ifid_reg.instr.bits = instruction_bits;
  ifid_reg.instr_addr = regfile_p->PC;
  return ifid_reg;
}

/**
 * STAGE  : stage_decode
 * output : idex_reg_t
 **/ 
idex_reg_t stage_decode(ifid_reg_t ifid_reg, pipeline_wires_t* pwires_p, regfile_t* regfile_p)
{
  idex_reg_t idex_reg = {0};
  /**
   * YOUR CODE HERE
   */

  // updating idex_reg
  idex_reg = gen_control(ifid_reg.instr);
  idex_reg.instr = ifid_reg.instr;  
  idex_reg.instr.bits = ifid_reg.instr.bits;
  idex_reg.instr_addr = ifid_reg.instr_addr;

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
  idex_reg.funct7 = (((idex_reg.instr.bits >> 30) & 1U));    
  idex_reg.funct3 = ((idex_reg.instr.bits >> 12) & ((1U << 3) - 1));

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
  /**
   * YOUR CODE HERE
   */

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
  
// ALU execution
  idex_reg.alu_control = gen_alu_control(idex_reg);
  exmem_reg.Read_Address = execute_alu(idex_reg.rs1_val, idex_reg.rs2_val, idex_reg.alu_control);

  if ((idex_reg.ALUOp == 0x1) && (exmem_reg.Read_Address == 0) && (idex_reg.funct3 == 0x0)) { //beq
    exmem_reg.zero = 1;
  }
  else if ((idex_reg.ALUOp == 0x1) && (exmem_reg.Read_Address != 0) && (idex_reg.funct3 == 0x1)) { //bne
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
  exmem_reg.instr = idex_reg.instr;  
  exmem_reg.instr.bits = idex_reg.instr.bits;
  exmem_reg.funct3 = idex_reg.funct3;

  // Pass to exmem
  exmem_reg.Mem_Read = idex_reg.Mem_Read;
  exmem_reg.Mem_Write = idex_reg.Mem_Write;
  exmem_reg.Memto_Reg = idex_reg.Memto_Reg;
  exmem_reg.Reg_Write = idex_reg.Reg_Write;
  exmem_reg.Branch = idex_reg.Branch;

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
  /**
   * YOUR CODE HERE
   */
  
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
  }
  else {
    memwb_reg.Read_Address = exmem_reg.Read_Address;
  }

  if (exmem_reg.Mem_Write) {
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

  memwb_reg.rd = exmem_reg.rd;
  
  memwb_reg.Memto_Reg = exmem_reg.Memto_Reg;
  memwb_reg.Reg_Write = exmem_reg.Reg_Write;
  memwb_reg.instr = exmem_reg.instr;
  memwb_reg.instr_addr = exmem_reg.instr_addr;
  memwb_reg.instr_addr_imm = exmem_reg.instr_addr_imm;

  //Create pcsrc wire
  pwires_p->pcsrc = exmem_reg.Branch & exmem_reg.zero;

  // Return pc_src1 to IF MUX
  pwires_p->pc_src1 = memwb_reg.instr_addr_imm;    

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
  /**
  * YOUR CODE HERE
  */

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

  pwires_p->pc_src0 = regfile_p->PC+4;

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
  
  pregs_p->idex_preg.inp  = stage_decode    (pregs_p->ifid_preg.out, pwires_p, regfile_p);

  gen_forward(pregs_p, pwires_p);

  pregs_p->exmem_preg.inp = stage_execute   (pregs_p->idex_preg.out, pwires_p);

  pregs_p->memwb_preg.inp = stage_mem       (pregs_p->exmem_preg.out, pwires_p, memory_p, cache_p);

                            stage_writeback (pregs_p->memwb_preg.out, pwires_p, regfile_p);

  #ifdef PRINT_STATS // only runs for ms2, 3, 4
  detect_hazard(pregs_p, pwires_p, regfile_p);

  //Flush Pipeline if Branch is taken
  if (pwires_p->pcsrc == 1){
    branch_counter++;
    printf("[CPL]: Pipeline Flushed\n");
    // Prevent update of PC and IF/ID register
    pwires_p->PC_haz = 0;            // Disable PC update
    pwires_p->ifid_haz = 0;          // Disable IF/ID register update
    pwires_p->control_mux_haz = 0; // Set control signals to zero (nop)

    //flush ifid_preg
    pregs_p->ifid_preg.inp.instr.ujtype.opcode = 0x13;
    pregs_p->ifid_preg.inp.instr.ujtype.rd = 0;
    pregs_p->ifid_preg.inp.instr.ujtype.imm = 0;
    pregs_p->ifid_preg.out.instr.ujtype.opcode = 0x13;
    pregs_p->ifid_preg.out.instr.ujtype.rd = 0;
    pregs_p->ifid_preg.out.instr.ujtype.imm = 0;

    //flush idex_preg
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

    //flush exmem_preg
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
  }
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
