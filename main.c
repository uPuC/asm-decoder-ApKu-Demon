/******************************************************************************
Prac 2 - AVR ASM OpCode Decoder

0x24 0x00 = 0010 0100 0000 0000 -> EOR R0, R0 -> CLR R0
0x24 -> EOR, CLR(d = r)

0xE0 0XA0 -> 1110 0000 1010 0000 -> LDI R26, 0x00
0xE -> LDI

0xE0 0xB2 -> 1110 0000 1011 0010 -> LDI R27, 0x02
0xE -> LDI

0X91 0X0D -> 1001 0001 0000 1101 -> LD R16, X+
0x9==D -> LD (), X+

0X30 0X00 -> 0011 0000 0000 0000 -> CPI R16, 0x00
0x3 -> CPI

0XF7 0XE9 -> 1111 0111 1110 1001 -> BRNE -3
0xF4=1 -> BRNE (-3 instrucciones atras)

0x97 0x11 -> 1001 0111 0001 0001 -> SBIW R26, 0x01
0x97 -> SBIW

0xE0 0xC0 -> 1110 0000 1100 0000 -> LDI R28, 0x00
0xE -> LDI

...

0xE0 0xD2 -> 1110 0000 1101 0010 -> LDI R29, 0x02
0xE -> LDI

0x91 0x09 -> 1001 0001 0000 1001 -> LD R16, Y+
0x9==9 -> LD (), Y+     -> LDD Y

0x91 0x1E -> 1001 0001 0001 1110 -> LD R17, -X
0x9==E -> LD (), -X

0x17 0x01 -> 0001 0111 0000 0001 -> CP R16, R17
0x14 -> CP

0xF4 0x51 -> 1111 0100 0101 0001 -> BRNE +10
0xF4=1 -> BRNE (+10 instrucciones adelante)

0x2F 0x0A -> 0010 1111 0000 1010 -> MOV R16, R26
0x2C -> MOV

0X95 0x0A -> 1001 0101 0000 1010  -> DEC R16
0x94=A -> DEC

0x2F 0x1C -> 0010 1111 0001 1100 -> MOV R17, R28
0x2C -> MOV

...

0X17 0x01 -> 0001 0111 0000 0001 -> CP R16, R17
0x14 -> CP

0xF7 0xB9 -> 1111 0111 1011 1001 -> BRNE -9
0xF4=1 -> BRNE (-9 instrucciones atras)

0x2F 0x0B -> 0010 1111 0000 1011 -> MOV R16, R27
0x2C -> MOV

0x2F 0x1D -> 0010 1111 0001 1101 -> MOV R17, R29
0x2C -> MOV

0x17 0x01 -> 0001 0111 0000 0001 -> CP R16, R17
0x14 -> CP

0xF7 0x99 -> 1111 0111 1001 1001 -> BRNE -13
0xF4=1 -> BRNE (-13 instrucciones atras)

0x94 0x03 -> 1001 0100 0000 0011 -> INC R0
0x94=3 -> INC R0

0x00 0x00 -> 0000 0000 0000 0000 -> NOP
0x00 -> NOP

*******************************************************************************/

#include <stdio.h>
#include <inttypes.h>f7

const uint8_t flash_mem[] ={ 
    0x00, 0x24, 0xA0, 0xE0, 0xB2, 0xE0, 0x0D, 0x91, 0x00, 0x30, 0xE9, 0xF7, 0x11, 0x97, 0xC0, 0xE0, 
    0xD2, 0xE0, 0x09, 0x91, 0x1E, 0x91, 0x01, 0x17, 0x51, 0xF4, 0x0A, 0x2F, 0x0A, 0x95, 0x1C, 0x2F, 
    0x01, 0x17, 0xB9, 0xF7, 0x0B, 0x2F, 0x1D, 0x2F, 0x01, 0x17, 0x99, 0xF7, 0x03, 0x94, 0x00, 0x00 };
//  1           2           3           4           5           6           7           8
// quitar los ultimos dos bytes de las 2 primeras filas de la memoria flash

const uint16_t inst16_table[] = {
  {0x0}, //NOP
  {0x9}, //EOR, CLR
  {0xE}, //LDI
  {0x48}, //LD (LD X, LD Y)
  {0x3}, //CPI
  {0x3D}, //BRNE
  {0x97}, //SBIW
  {0x5}, //CP
  {0xB}, //MOV
  {0x4A}, //DEC, INC
  {0x2E} //OUT
};

enum{
    e_NOP,
    e_EOR_CLR,
    e_LDI,
    e_LD,
    e_CPI,
    e_BRNE,
    e_SBIW,
    e_CP,
    e_MOV,
    e_DEC_INC,
    e_OUT
};


// Op Code struct
typedef union {
    uint16_t op16; // e.g.: watchdog, nop

    // e.g.: EOR, CLR
    struct{
        uint16_t r4:4;
        uint16_t d5:5;
        uint16_t r1:1;
        uint16_t op6:6;
    }type0; // 0010 01rd dddd rrrr

    // e.g.: LDI, CPI
    struct{
        uint16_t k4_1:4;
        uint16_t d4:4;
        uint16_t k4_2:4;
        uint16_t op4:4;
    }type1; // 1110 KKKK dddd KKKK (LDI)
            // 0011 KKKK dddd KKKK (CPI)

    // e.g.: LD (LD X, LD Y)
    struct{
        uint16_t op4:4;
        uint16_t d5:5;
        uint16_t op7:7;
    }type2; // 1001 000d dddd 1101 X+
            // 1001 000d dddd 1110 -X
            // 1001 000d dddd 1001 Y+

    // e.g.: BRNE
    struct{
        uint16_t op3:3;
        uint16_t k7:7;
        uint16_t op6:6;
    }type3; // 1111 01KK KKKK K001

    // e.g.: SBIW
    struct{
        uint16_t k4:4;
        uint16_t d2:2;
        uint16_t k2:2;
        uint16_t op8:8;
    }type4; // 1001 0111 KKdd KKKK

    // e.g.: CP,MOV,OUT, MUL,ADC,ADD,AND,
    struct{
        uint16_t r4:4;
        uint16_t d5:5;
        uint16_t r1:1;
        uint16_t op6:6;
    }type5; // 0001 01rd dddd rrrr (CP)
            // 0010 11rd dddd rrrr (MOV)
            // 1011 10rd dddd rrrr (OUT)

    // e.g.: DEC, INC
    struct{
        uint16_t op4:4;
        uint16_t d5:5;
        uint16_t op7:7;
    }type6; // 1001 010d dddd 1010 (DEC)
            // 1001 010d dddd 0011 (INC)

    // TO-DO: Add more types as needed
} Op_Code_t;


int main()
{
    Op_Code_t *instruction;
    printf("- Practica 2: AVR OpCode -\n");
    // Decode the instructions by cycling through the array
    for (uint8_t idx = 0; idx < sizeof(flash_mem); idx+=2)
    {
        instruction = (Op_Code_t*) &flash_mem[idx];
        if (instruction->op16 == inst16_table[e_NOP])
        {
            printf("NOP\n");
        }
        else if (instruction->type0.op6 == inst16_table[e_EOR_CLR])
        {
            printf("EOR R%d, R%d\n", instruction->type0.d5, instruction->type0.r4);
        }
        else if (instruction->type1.op4 == inst16_table[e_LDI])
        {
            printf("LDI R%d, 0x%02X\n", instruction->type1.d4 + 16, (instruction->type1.k4_2 << 4) | instruction->type1.k4_1);
        }
        else if (instruction->type2.op7 == inst16_table[e_LD])
        {
            if (instruction->type2.op4 == 0x0D)
            {
                printf("LD R%d, X+\n", instruction->type2.d5);
            }
            else if (instruction->type2.op4 == 0x0E)
            {
                printf("LD R%d, -X\n", instruction->type2.d5);
            }
            else if (instruction->type2.op4 == 0x09)
            {
                printf("LD R%d, Y+\n", instruction->type2.d5);
            }
        }
        else if(instruction->type1.op4 == inst16_table[e_CPI])
        {
            printf("CPI R%d, 0x%02X\n", instruction->type1.d4 + 16, (instruction->type1.k4_2 << 4) | instruction->type1.k4_1);
        }
        else if (instruction->type3.op6 == inst16_table[e_BRNE])
        {
            int8_t offset = instruction->type3.k7;      // Le pasas a 8 bits 7 bits y el bit de signo queda en 0
            if (offset & 0x40) // Revisar si es negativo (bit de signo)
            {
                offset = offset - 0x80 + 1; // Si es negativo le restas 128 para agregarle el signo negativo
            }
            printf("BRNE %d\n", offset);
        }
        else if (instruction->type4.op8 == inst16_table[e_SBIW])
        {
            printf("SBIW R%d, 0x%02X\n", (instruction->type4.d2 << 1) + 24, (instruction->type4.k2 << 2) | instruction->type4.k4);
        }
        else if (instruction->type5.op6 == inst16_table[e_CP])
        {
            printf("CP R%d, R%d\n", instruction->type5.d5, instruction->type5.r4 | (instruction->type5.r1 << 4));
        }
        else if (instruction->type5.op6 == inst16_table[e_MOV])
        {
            printf("MOV R%d, R%d\n", instruction->type5.d5, instruction->type5.r4 | (instruction->type5.r1 << 4));
        }
        else if (instruction->type5.op6 == inst16_table[e_OUT])
        {
            printf("OUT 0x%02X, R%d\n", instruction->type5.r4, instruction->type5.d5);
        }
        else if (instruction->type6.op7 == inst16_table[e_DEC_INC])
        {
            if (instruction->type6.op4 == 0xA)
            {
                printf("DEC R%d\n", instruction->type6.d5);
            }
            else if (instruction->type6.op4 == 0x3)
            {
                printf("INC R%d\n", instruction->type6.d5);
            }
        }
        else
        {
            printf("unknown\n");
        }
    }
    return 0;
}