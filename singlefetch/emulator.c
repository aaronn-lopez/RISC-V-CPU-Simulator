#include <stdio.h> // for stderr
#include <stdlib.h> // for exit()
#include "types.h"
#include "utils.h"
#include "riscv.h"

void execute_rtype(Instruction, Processor *);
void execute_itype_except_load(Instruction, Processor *);
void execute_branch(Instruction, Processor *);
void execute_jal(Instruction, Processor *);
void execute_load(Instruction, Processor *, Byte *);
void execute_store(Instruction, Processor *, Byte *);
void execute_ecall(Processor *, Byte *);
void execute_lui(Instruction, Processor *);

void execute_instruction(uint32_t instruction_bits, Processor *processor,Byte *memory) {    
    Instruction instruction = parse_instruction(instruction_bits);
    switch(instruction.opcode) {
        case 0x33:
            execute_rtype(instruction, processor);
            break;
        case 0x13:
            execute_itype_except_load(instruction, processor);
            break;
        case 0x73:
            execute_ecall(processor, memory);
            break;
        case 0x63:
            execute_branch(instruction, processor);
            break;
        case 0x6F:
            execute_jal(instruction, processor);
            break;
        case 0x23:
            execute_store(instruction, processor, memory);
            break;
        case 0x03:
            execute_load(instruction, processor, memory);
            break;
        case 0x37:
            execute_lui(instruction, processor);
            break;
        default: // undefined opcode
            handle_invalid_instruction(instruction);
            exit(-1);
            break;
    }
}

void execute_rtype(Instruction instruction, Processor *processor) {
    switch (instruction.rtype.funct3){
        case 0x0:
            switch (instruction.rtype.funct7) {
                case 0x0:
                    // Add
                    processor->R[instruction.rtype.rd] =
                        ((sWord)processor->R[instruction.rtype.rs1]) +
                        ((sWord)processor->R[instruction.rtype.rs2]);
                    break;
                case 0x1:
                    // Mul
                    processor->R[instruction.rtype.rd] =
                        ((sWord)processor->R[instruction.rtype.rs1]) *
                        ((sWord)processor->R[instruction.rtype.rs2]);
                    break;
                case 0x20:
                    // Sub
                    processor->R[instruction.rtype.rd] =
                        ((sWord)processor->R[instruction.rtype.rs1]) -
                        ((sWord)processor->R[instruction.rtype.rs2]);
                    break;
                default:
                    handle_invalid_instruction(instruction);
                    exit(-1);
                    break;
            }
            break;
        /* YOUR CODE HERE */
	/* deal with other cases */
	    case 0x1:
            switch (instruction.rtype.funct7) {
                case 0x00:
                    // sll
                    processor->R[instruction.rtype.rd] =
                        ((sWord)processor->R[instruction.rtype.rs1]) <<
                        ((sWord)processor->R[instruction.rtype.rs2]);
                    break;
                case 0x01:
                    // mulh                    
                    processor->R[instruction.rtype.rd] =
                        (((sWord)processor->R[instruction.rtype.rs1]) *
                        ((sWord)processor->R[instruction.rtype.rs2])) >> 31;
                    break;
                default:
                    handle_invalid_instruction(instruction);
                    exit(-1);
                    break;
            }
            break;
        case 0x2:
            // slt
            if(((sWord)processor->R[instruction.rtype.rs1]) < ((sWord)processor->R[instruction.rtype.rs2])) {
                processor->R[instruction.rtype.rd] = 1;
            }
            else {
                processor->R[instruction.rtype.rd] = 0;
            }                   
            break;
        case 0x4:
            switch (instruction.rtype.funct7) {
                case 0x00:
                    // xor
                    processor->R[instruction.rtype.rd] =
                        ((sWord)processor->R[instruction.rtype.rs1]) ^
                        ((sWord)processor->R[instruction.rtype.rs2]);
                    break;
                case 0x01:
                    // div                    
                    processor->R[instruction.rtype.rd] =
                        ((sWord)processor->R[instruction.rtype.rs1]) /
                        ((sWord)processor->R[instruction.rtype.rs2]);
                    break;
                default:
                    handle_invalid_instruction(instruction);
                    exit(-1);
                    break;
            }
            break;
        case 0x5:
            switch (instruction.rtype.funct7) {
                case 0x00:
                    // srl
                    processor->R[instruction.rtype.rd] =
                        ((sWord)processor->R[instruction.rtype.rs1]) >>
                        ((sWord)processor->R[instruction.rtype.rs2]);
                    break;
                case 0x20:
                    // sra                    
                    processor->R[instruction.rtype.rd] =
                        ((sWord)processor->R[instruction.rtype.rs1]) >>
                        ((sWord)processor->R[instruction.rtype.rs2]);
                    break;
                default:
                    handle_invalid_instruction(instruction);
                    exit(-1);
                    break;
            }
            break;
        case 0x6:
            switch (instruction.rtype.funct7) {
                case 0x00:
                    // or 
                    processor->R[instruction.rtype.rd] =
                        ((sWord)processor->R[instruction.rtype.rs1]) |
                        ((sWord)processor->R[instruction.rtype.rs2]);
                    break;
                case 0x01:
                    // rem                    
                    processor->R[instruction.rtype.rd] =
                        ((sWord)processor->R[instruction.rtype.rs1]) %
                        ((sWord)processor->R[instruction.rtype.rs2]);
                    break;
                default:
                    handle_invalid_instruction(instruction);
                    exit(-1);
                    break;
            }
            break;
        case 0x7:
            // and                   
            processor->R[instruction.rtype.rd] =
                ((sWord)processor->R[instruction.rtype.rs1]) &
                ((sWord)processor->R[instruction.rtype.rs2]);
            break;
        default:
            handle_invalid_instruction(instruction);
            exit(-1);
            break;
    }
    // update PC
    processor->PC += 4;
}

void execute_itype_except_load(Instruction instruction, Processor *processor) {
	switch (instruction.itype.funct3) {
        /* YOUR CODE HERE */
	    case 0x0:
            // addi                  
            processor->R[instruction.itype.rd] =
                ((sWord)processor->R[instruction.itype.rs1]) +
                ((sWord)(sign_extend_number(instruction.itype.imm, 12)));
            break;
        case 0x1:
            switch (instruction.itype.imm >> 5) {
                case 0x00:
                    // slli                 
                    processor->R[instruction.itype.rd] =
                        ((sWord)processor->R[instruction.itype.rs1]) <<
                        ((sWord)(sign_extend_number(instruction.itype.imm, 12) & ((1U << 5) - 1)));
                    break;
                default:
                    handle_invalid_instruction(instruction);
                    exit(-1);
                    break;
            }
            break; 
        case 0x2:
            // slti
            if(((sWord)processor->R[instruction.itype.rs1]) < ((sWord)(sign_extend_number(instruction.itype.imm, 12)))) {
                processor->R[instruction.itype.rd] = 1;
            }
            else {
                processor->R[instruction.itype.rd] = 0;
            }                   
            break;
        case 0x4:
            // xori                   
            processor->R[instruction.itype.rd] =
                ((sWord)processor->R[instruction.itype.rs1]) ^
                ((sWord)(sign_extend_number(instruction.itype.imm, 12)));
            break;
        case 0x5:
            switch (instruction.itype.imm >> 5) {
                case 0x00:
                    // srli
                    processor->R[instruction.itype.rd] =
                        ((sWord)processor->R[instruction.itype.rs1]) >>
                        ((sWord)(sign_extend_number(instruction.itype.imm, 12) & ((1U << 5) - 1)));
                    break;
                case 0x20:
                    // srai                
                    processor->R[instruction.itype.rd] =
                        ((sWord)processor->R[instruction.itype.rs1]) >>
                        ((sWord)(sign_extend_number(instruction.itype.imm, 12)& ((1U << 5) - 1)));
                    break;
                default:
                    handle_invalid_instruction(instruction);
                    exit(-1);
                    break;
            }
            break; 
        case 0x6:
            // ori                   
            processor->R[instruction.itype.rd] =
                ((sWord)processor->R[instruction.itype.rs1]) |
                ((sWord)(sign_extend_number(instruction.itype.imm, 12)));
            break;  
        case 0x7:
            // andi                   
            processor->R[instruction.itype.rd] =
                ((sWord)processor->R[instruction.itype.rs1]) &
                ((sWord)(sign_extend_number(instruction.itype.imm, 12)));
            break;        
	default:
            handle_invalid_instruction(instruction);
            break;
    }
    processor->PC += 4;
}

void execute_ecall(Processor *p, Byte *memory) {
    Register i;
    
    // syscall number is given by a0 (x10)
    // argument is given by a1
    switch(p->R[10]) {
        case 1: // print an integer
            printf("%d",p->R[11]);
            p->PC += 4;
            break;
        case 4: // print a string
            for(i=p->R[11];i<MEMORY_SPACE && load(memory,i,LENGTH_BYTE);i++) {
                printf("%c",load(memory,i,LENGTH_BYTE));
            }
            p->PC += 4;
            break;
        case 10: // exit
            printf("exiting the simulator\n");
            exit(0);
            break;
        case 11: // print a character
            printf("%c",p->R[11]);
            p->PC += 4;
            break;
        default: // undefined ecall
            printf("Illegal ecall number %d\n", p->R[10]);
            exit(-1);
            break;
    }
}

void execute_branch(Instruction instruction, Processor *processor) {
    switch (instruction.sbtype.funct3) {
        /* YOUR CODE HERE */
	case 0x0:
 	// beq
 	if(((sWord)processor->R[instruction.sbtype.rs1]) == ((sWord)processor->R[instruction.sbtype.rs2])) {
 		processor->PC += sign_extend_number(get_branch_offset(instruction), 13);
 	} else {
		processor->PC += 4;
	}
 	break;
 	case 0x1:
 	// bne
 	if(((sWord)processor->R[instruction.sbtype.rs1]) != ((sWord)instruction.sbtype.rs2)) {
 	processor->PC = processor->PC +
	sign_extend_number(get_branch_offset(instruction), 13); //PC = PC+ ;
 	} else {
		processor->PC += 4;
	}
	 break;
        default:
            handle_invalid_instruction(instruction);
            exit(-1);
            break;
    }
 }

void execute_load(Instruction instruction, Processor *processor, Byte *memory) {
    switch (instruction.itype.funct3) {
        /* YOUR CODE HERE */
	case 0x0: // lb
		processor->R[instruction.rtype.rd] = sign_extend_number(load(memory, ((sWord)processor->R[instruction.rtype.rs1]) + sign_extend_number(instruction.itype.imm, 12), LENGTH_BYTE), 8);
	break;
	case 0x1: // lh
		processor->R[instruction.rtype.rd] = sign_extend_number(load(memory, ((sWord)processor->R[instruction.rtype.rs1]) + sign_extend_number(instruction.itype.imm, 12), LENGTH_HALF_WORD), 16);
	break;
	case 0x2: // lw
		processor->R[instruction.rtype.rd] = load(memory, ((sWord)processor->R[instruction.rtype.rs1]) + sign_extend_number(instruction.itype.imm, 12), LENGTH_WORD);
	break;
        default:
            handle_invalid_instruction(instruction);
            break;
    }
    processor->PC += 4;
}

void execute_store(Instruction instruction, Processor *processor, Byte *memory) {
    switch (instruction.stype.funct3) {
        /* YOUR CODE HERE */
	case 0x0: //sb
	store(memory, ((sWord)processor->R[instruction.rtype.rs1]) + get_store_offset(instruction), LENGTH_BYTE, ((sWord)processor->R[instruction.rtype.rs2]));
	break;
	case 0x1: // sh
	store(memory, ((sWord)processor->R[instruction.rtype.rs1]) + get_store_offset(instruction), LENGTH_HALF_WORD, ((sWord)processor->R[instruction.rtype.rs2]));
	break;
	case 0x2: //sw
	store(memory, ((sWord)processor->R[instruction.rtype.rs1]) + get_store_offset(instruction), LENGTH_WORD, ((sWord)processor->R[instruction.rtype.rs2]));
	break;
        default:
            handle_invalid_instruction(instruction);
            exit(-1);
            break;
    }
	processor->PC += 4;
}

void execute_jal(Instruction instruction, Processor *processor) {
    /* YOUR CODE HERE */
	processor->R[instruction.ujtype.rd] = processor->PC + 4;
    processor->PC = processor->PC + sign_extend_number(get_jump_offset(instruction), 13);
}

void execute_lui(Instruction instruction, Processor *processor) {
    /* YOUR CODE HERE */
	processor->R[instruction.ujtype.rd] = ((sWord)instruction.utype.imm) << 12;
	processor->PC += 4;
}

void store(Byte *memory, Address address, Alignment alignment, Word value) {
    /* YOUR CODE HERE */
	if (alignment == LENGTH_BYTE) {
        memory[address] = value & 0xff;
    } else if (alignment == LENGTH_HALF_WORD) {
        memory[address] = value & 0xff;
        memory[address+1] = (value >> 8) & 0xff;
    } else if (alignment == LENGTH_WORD) {
        memory[address] = value & 0xff;
        memory[address+1] = (value >> 8) & 0xff;
        memory[address+2] = (value >> 16) & 0xff;
        memory[address+3] = (value >> 24) & 0xff;
    } else {
        printf("Error: Unrecognized alignment %d\n", alignment);
        exit(-1);
    }
}

Word load(Byte *memory, Address address, Alignment alignment) {
    if(alignment == LENGTH_BYTE) {
        return memory[address];
    } else if(alignment == LENGTH_HALF_WORD) {
        return (memory[address+1] << 8) + memory[address];
    } else if(alignment == LENGTH_WORD) {
        return (memory[address+3] << 24) + (memory[address+2] << 16)
               + (memory[address+1] << 8) + memory[address];
    } else {
        printf("Error: Unrecognized alignment %d\n", alignment);
        exit(-1);
    }
}
