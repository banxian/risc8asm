#ifndef _RISC8B_INS_H
#define _RISC8B_INS_H

// Control Category Instructions
#define _OP_NOP      0x0000  // 00000000 000000xx NOP
#define _OP_CLRWDT   0x0008  // 00000000 000010xx CLRWDT
#define _OP_SLEEP    0x000C  // 00000000 00001100 SLEEP
#define _OP_SLEEPX   0x000C  // 00000000 000011kk SLEEPX k2
#define _OP_WAITB    0x0010  // 00000000 00010bbb WAITB b
#define _OP_WAITRD   0x0010  // 00000000 00010000 WAITRD
#define _OP_WAITWR   0x0014  // 00000000 00010100 WAITWR/WAITSPI
#define _OP_RDCODE   0x0018  // 00000000 00011000 RDCODE
#define _OP_RCODE    0x0018  // 00000000 000110kk RCODE k2
#define _OP_WRCODE   0x001C  // 00000000 00011100 WRCODE
#define _OP_EXEC     0x001C  // 00000000 000111kk EXEC k2
#define _OP_PUSHAS   0x0020  // 00000000 001000xx PUSHAS
#define _OP_POPAS    0x0024  // 00000000 001001xx POPAS
#define _OP_PUSHA2   0x0028  // 00000000 001010xx PUSHA2
#define _OP_POPA2    0x002C  // 00000000 001011xx POPA2
#define _OP_RET      0x0030  // 00000000 001100xx RET
#define _OP_RETZ     0x0034  // 00000000 001101xx RETZ
#define _OP_RETIE    0x0038  // 00000000 001110xx RETIE

// Byte Operation Category Instructions  
#define _OP_CLRA     0x0004  // 00000000 000001xx CLRA
#define _OP_CLR      0x0100  // 00000001 ffffffff CLR f
#define _OP_MOVA     0x1000  // 0001000F FFFFFFFF MOVA F
#define _OP_MOV      0x0200  // 000d001F FFFFFFFF MOV F,d
#define _OP_INC      0x0400  // 000d0100 ffffffff INC f,d
#define _OP_DEC      0x0500  // 000d0101 ffffffff DEC f,d
#define _OP_INCSZ    0x0600  // 000d0110 ffffffff INCSZ f,d
#define _OP_DECSZ    0x0700  // 000d0111 ffffffff DECSZ f,d
#define _OP_SWAP     0x0800  // 000d1000 ffffffff SWAP f,d
#define _OP_AND      0x0900  // 000d1001 ffffffff AND f,d
#define _OP_IOR      0x0A00  // 000d1010 ffffffff IOR f,d
#define _OP_XOR      0x0B00  // 000d1011 ffffffff XOR f,d
#define _OP_ADD      0x0C00  // 000d1100 ffffffff ADD f,d
#define _OP_SUB      0x0D00  // 000d1101 ffffffff SUB f,d
#define _OP_RCL      0x0E00  // 000d1110 ffffffff RCL f,d
#define _OP_RCR      0x0F00  // 000d1111 ffffffff RCR f,d

// Constant Operation Category Instructions
#define _OP_RETL     0x2000  // 00100000 kkkkkkkk RETL k
#define _OP_RETLN    0x2100  // 00100001 kkkkkkkk RETLN k
#define _OP_MOVIP    0x2200  // 0010001k kkkkkkkk MOVIP k9
#define _OP_MOVIA    0x2400  // 001001kk kkkkkkkk MOVIA k10
#define _OP_MOVA1F   0x2300  // 00100011 kkkkkkkk MOVA1F k
#define _OP_MOVA2F   0x2500  // 00100101 kkkkkkkk MOVA2F k
#define _OP_MOVA2P   0x2600  // 00100110 kkkkkkkk MOVA2P k
#define _OP_MOVA1P   0x2700  // 00100111 kkkkkkkk MOVA1P k
#define _OP_MOVL     0x2800  // 00101000 kkkkkkkk MOVL k
#define _OP_ANDL     0x2900  // 00101001 kkkkkkkk ANDL k
#define _OP_IORL     0x2A00  // 00101010 kkkkkkkk IORL k
#define _OP_XORL     0x2B00  // 00101011 kkkkkkkk XORL k
#define _OP_ADDL     0x2C00  // 00101100 kkkkkkkk ADDL k
#define _OP_SUBL     0x2D00  // 00101101 kkkkkkkk SUBL k
#define _OP_CMPLN    0x2E00  // 00101110 kkkkkkkk CMPLN k
#define _OP_CMPL     0x2F00  // 00101111 kkkkkkkk CMPL k

// Bit Operation Category Instructions
#define _OP_BC       0x4000  // 01000bbb ffffffff BC f,b
#define _OP_BS       0x4800  // 01001bbb ffffffff BS f,b
#define _OP_BTSC     0x5000  // 01010bbb ffffffff BTSC f,b
#define _OP_BTSS     0x5800  // 01011bbb ffffffff BTSS f,b
#define _OP_BCTC     0x001C  // 00000000 000111aa BCTC a
#define _OP_BP1F     0x0080  // 00000000 100aabbb BP1F a,b
#define _OP_BP2F     0x00A0  // 00000000 101aabbb BP2F a,b
#define _OP_BG1F     0x00C0  // 00000000 110aabbb BG1F a,b
#define _OP_BG2F     0x00E0  // 00000000 111aabbb BG2F a,b

// Transfer Category Instructions
#define _OP_JMP      0x6000  // 0110kkkk kkkkkkkk JMP k12
#define _OP_CALL     0x7000  // 0111kkkk kkkkkkkk CALL k12
#define _OP_JNZ      0x3000  // 001100kk kkkkkkkk JNZ k10
#define _OP_JZ       0x3400  // 001101kk kkkkkkkk JZ k10
#define _OP_JNC      0x3800  // 001110kk kkkkkkkk JNC k10
#define _OP_JC       0x3C00  // 001111kk kkkkkkkk JC k10
#define _OP_CMPZ     0x8000  // 1KKKKKKK kkkkkkkk CMPZ K7,k

enum encoding_type {
    et_Invalid = -1,
    et_None = 1,        // no arguments                NOP/RET
    et_f8,              // 8bit register address f     CLR f 
    et_F9,              // 9bit register address F     MOVA F
    et_F9_d1,           // 9bit F, 1bit destination d  MOV F,d
    et_f8_d1,           // 8bit f, 1bit destination d  ADD f,d
    et_k8,              // 8bit constant               RETL k
    et_k9,              // 9bit constant               MOVIP k9
    et_k10,             // 10bit constant              MOVIA k10
    et_f8_b3,           // 8bit f, 3bit selector b     BC f,b
    et_k12,             // 12bit PC address            JMP k12
    et_k10_pc,          // Lower 10bit PC address      JZ k10
    et_K7_k8,           // 7bit constant, 8bit address CMPZ K7,k
    et_k2,              // 2bit constant               SLEEPX k2
    et_b3,              // 3bit bit selector           WAITB b
    et_a2_b3,           // 2bit a, 3bit selector b     BP1F a,b
    et_dw               // 16bit raw data              DW data
};

struct opcode_info {
	uint16_t opcode;
	uint8_t type;
};

#define SFR_PRG_COUNT 2

#endif
