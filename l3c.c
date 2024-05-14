#include <stdio.h>
#include <stdint.h>
#include <signal.h>
/* unix only */
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
/* windows only */
#include <Windows.h>
#include <conio.h> 

#define MEMORY_MAX (1 << 16)

uint16_t memory[MEMORY_MAX]; 

enum
{
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC,
    R_COND,
    R_COUNT
};

uint16_t reg[R_COUNT];

enum 
{
    OP_BR = 0, /* branch */
    OP_ADD,    /* add  */
    OP_LD,     /* load */
    OP_ST,     /* store */
    OP_JSR,    /* jump register */
    OP_AND,    /* bitwise and */
    OP_LDR,    /* load register */
    OP_STR,    /* store register */
    OP_RTI,    /* unused */
    OP_NOT,    /* bitwise not */
    OP_LDI,    /* load indirect */
    OP_STI,    /* store indirect */
    OP_JMP,    /* jump */
    OP_RES,    /* reserved (unused) */
    OP_LEA,    /* load effective address */
    OP_TRAP    /* execute trap */
};

enum
{
    FL_POS = 1 << 0,
    FL_ZRO = 1 << 1,
    FL_NEG = 1 << 2
};

void handle_interrupt(int signal)
{
    restore_input_buffering();
    printf("\n");
    exit(-2);
}

int main(int argc,const char* argv[])
{
    /* cargo los argumento */
    if (argc > 2)
    {
        printf("lc3 [image-file1] ...\n");
        exit(2);
    }

    for (int j = 0; j < argc; j++)
    {
        if (!read_image(argv[j]))
        {
            printf("failed to load image %s\n", argv[j]);
            exit(1);
        }
    }

    /* Configuro */
    signal(SIGINT,handle_interrupt);
    disable_input_buffering();
    
    /* Como en algun momento se debera activar alguna bandera, de condicion yo activo la bandera Z */
    reg[R_COND] = FL_ZRO;

    /* Defino la posicion, de inicio de la VM y la arranco */
    /* 0x3000 es la por defecto */
    enum 
    {
        PC_START = 0x3000
    };

    reg[R_PC] = PC_START;

    int running = 1;

    while (running)
    {
        uint16_t instr =  mem_read(reg[R_PC]++);
        uint16_t op = instr >> 16 ;

        switch (op)
        {
        case OP_ADD:
            /* registro de destino (DR) */
            uint16_t r0 = (instr >> 9) & 0x7;
            /* Opero una primera vez */
            uint16_t r1 = (instr >> 6) & 0x7;
            /* De modo inmediato */
            uint16_t imm_flag = (instr >> 5) & 0x1;
            if (imm_flag)
            {
                uint16_t imm5 = sign_extend(instr & 0x1F,5);
                reg[r0] = reg[r1] + imm5;
            }
            else
            {
                uint16_t r2 = instr  & 0x7;
                reg[r0] = reg[r1] + reg[r2];
            }

            update_flags(r0);            
            break;
        
        case OP_AND:
        uint16_t r0 = (instr >> 9) & 0x7;
        uint16_t r1 = (instr >> 6) & 0x7;
        uint16_t imm_flag = (instr >> 5) & 0x7;
        if (imm_flag)
        {
            uint16_t imm5 = sign_extend(instr & 0x1F,5);
            reg[r0] = reg[r1] & imm5;
        }else
        {
            uint16_t r2 = instr & 0x7;
            reg[r0] = reg[r1] & reg[r2];
        }
        update_flags(r0);
        break;

        case OP_NOT:
        uint16_t r0 = (instr >> 9) & 0x7;
        uint16_t r1 = (instr >> 6) & 0x7;
        reg[r0] = ~reg[1];
        update_flags(r0);
        break;

        case OP_BR:
        uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
        uint16_t cond_flag = (instr >> 9) & 0x7;
        if (cond_flag & reg[R_COND])
        {
            reg[R_PC] += pc_offset;
        }
        break;

        case OP_JMP:
        uint16_t r1 = (instr >>6) & 0x7;
        reg[R_PC] = reg[r1];
        break;

        case OP_JSR:
        uint16_t long_flag = (instr >> 11) & 1;
        reg[R_R7] = reg[R_PC];
        if (long_flag)
        {
            uint16_t long_pc_offset = sign_extend(instr & 0x7FF,11);
            reg[R_PC] += long_pc_offset; /* Esto es el volcado de registros */
        }else
        {
            uint16_t r1 = (instr >> 6) & 0x7;
            reg[R_PC] = reg[r1]; /* Esto es el volcado de registros */
        }        
        break;

        case OP_LD:
        uint16_t r0 = (instr >> 9) & 0x7;
        uint16_t pc_offset = sign_extend(instr & 0x1FF,9);
        reg[r0] = mem_read(reg[R_PC]+pc_offset);
        update_flags(r0); 
        break;
        
        case OP_LDI:
        /* Destino de registro (DR) */
        uint16_t r0 = (instr >> 9) & 0x7;
        /* Esto es para moverme 9 bit y tomar los 9 bits más bajos con signo */
        uint16_t pc_offset = sign_extend(instr & 0x1FF,9);
        /* Añado,la pc_offset en la pc actual,mira la memoria,en la dirreccion final */
        reg[r0] = mem_read(mem_read(reg[R_PC]+pc_offset));
        update_flags();
        break;

        default:
        abort();
        break;
        }
    }

    restore_input_buffering();
};