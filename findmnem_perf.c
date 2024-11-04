/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf risc8b.gperf  */
/* Computed positions: -k'1-3,5,$' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif

#line 14 "risc8b.gperf"

#include <stdint.h>
#include "risc8bins.h"
#line 19 "risc8b.gperf"
struct mnemonic_entry {
    char name[7];
    uint8_t len;
    struct opcode_info info;
};
#include <string.h>
/* maximum key range = 339, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash(register const char *str, register size_t len)
{
    static const unsigned short asso_values[] =
    {
      341, 341, 341, 341, 341, 341, 341, 341, 341, 341,
      341, 341, 341, 341, 341, 341, 341, 341, 341, 341,
      341, 341, 341, 341, 341, 341, 341, 341, 341, 341,
      341, 341, 341, 341, 341, 341, 341, 341, 341, 341,
      341, 341, 341, 341, 341, 341, 341, 341, 341,  20,
      110, 115, 341, 341, 341, 341, 341, 341, 341, 341,
      341, 341, 341, 341, 341,  45,   0,   5,   5,  10,
       25,  20,  40, 120,  40,   0,  10,  55,  30,   0,
       35,  80,   5,   0,  25,   0,   0,  40,  92, 341,
       50,   0, 341, 341, 341, 341, 341, 341, 341, 341,
      341, 341, 341, 341, 341, 341, 341, 341, 341, 341,
      341, 341, 341, 341, 341, 341, 341, 341, 341, 341,
      341, 341, 341, 341, 341, 341, 341, 341, 341
    };
    register unsigned int hval = len;

    switch (hval) {
    default:
        hval += asso_values[(unsigned char)str[4]];
        /*FALLTHROUGH*/
    case 4:
    case 3:
        hval += asso_values[(unsigned char)str[2] + 1];
        /*FALLTHROUGH*/
    case 2:
        hval += asso_values[(unsigned char)str[1]];
        /*FALLTHROUGH*/
    case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
    return hval + asso_values[(unsigned char)str[len - 1]];
}

const struct opcode_info *
find_mnemonic (register const char *str, register size_t len)
{
    enum
    {
        TOTAL_KEYWORDS = 98,
        MIN_WORD_LENGTH = 2,
        MAX_WORD_LENGTH = 7,
        MIN_HASH_VALUE = 2,
        MAX_HASH_VALUE = 340
    };

    static const struct mnemonic_entry mnemonic_table[] =
    {
      {""}, {""},
#line 104 "risc8b.gperf"
      {"BS",       2, {_OP_BS,       et_f8_b3}},
      {""}, {""}, {""}, {""},
#line 85 "risc8b.gperf"
      {"DB",       2, {_OP_RETL,     et_k8}},
#line 76 "risc8b.gperf"
      {"SUB",      3, {_OP_SUB,      et_f8_d1}},
      {""}, {""}, {""},
#line 102 "risc8b.gperf"
      {"BC",       2, {_OP_BC,       et_f8_b3}},
      {""},
#line 110 "risc8b.gperf"
      {"BCTC",     4, {_OP_BCTC,     et_k2}},
      {""}, {""}, {""},
#line 81 "risc8b.gperf"
      {"RCR",      3, {_OP_RCR,      et_f8_d1}},
#line 99 "risc8b.gperf"
      {"SUBL",     4, {_OP_SUBL,     et_k8}},
#line 49 "risc8b.gperf"
      {"RETOK",    5, {_OP_RETZ,     et_None}},
      {""}, {""},
#line 53 "risc8b.gperf"
      {"CLR",      3, {_OP_CLR,      et_f8}},
#line 116 "risc8b.gperf"
      {"GOTO",     4, {_OP_JMP,      et_k12}},
      {""}, {""}, {""},
#line 60 "risc8b.gperf"
      {"DEC",      3, {_OP_DEC,      et_f8_d1}},
#line 84 "risc8b.gperf"
      {"RETL",     4, {_OP_RETL,     et_k8}},
#line 87 "risc8b.gperf"
      {"RETER",    5, {_OP_RETLN,    et_k8}},
      {""}, {""}, {""},
#line 77 "risc8b.gperf"
      {"SUBF",     4, {_OP_SUB,      et_f8_d1}},
      {""},
#line 36 "risc8b.gperf"
      {"RDCODE",   6, {_OP_RDCODE,   et_None}},
      {""}, {""},
#line 82 "risc8b.gperf"
      {"RCRF",     4, {_OP_RCR,      et_f8_d1}},
#line 50 "risc8b.gperf"
      {"RETIE",    5, {_OP_RETIE,    et_None}},
      {""}, {""},
#line 46 "risc8b.gperf"
      {"RET",      3, {_OP_RET,      et_None}},
#line 54 "risc8b.gperf"
      {"CLRF",     4, {_OP_CLR,      et_f8}},
      {""}, {""}, {""},
#line 105 "risc8b.gperf"
      {"BSF",      3, {_OP_BS,       et_f8_b3}},
#line 61 "risc8b.gperf"
      {"DECF",     4, {_OP_DEC,      et_f8_d1}},
#line 109 "risc8b.gperf"
      {"BTFSS",    5, {_OP_BTSS,     et_f8_b3}},
#line 27 "risc8b.gperf"
      {"CLRWDT",   6, {_OP_CLRWDT,   et_None}},
#line 121 "risc8b.gperf"
      {"JC",       2, {_OP_JC,       et_k10_pc}},
#line 103 "risc8b.gperf"
      {"BCF",      3, {_OP_BC,       et_f8_b3}},
#line 108 "risc8b.gperf"
      {"BTSS",     4, {_OP_BTSS,     et_f8_b3}},
      {""},
#line 47 "risc8b.gperf"
      {"RETURN",   6, {_OP_RET,      et_None}},
      {""},
#line 83 "risc8b.gperf"
      {"RRF",      3, {_OP_RCR,      et_f8_d1}},
#line 106 "risc8b.gperf"
      {"BTSC",     4, {_OP_BTSC,     et_f8_b3}},
#line 107 "risc8b.gperf"
      {"BTFSC",    5, {_OP_BTSC,     et_f8_b3}},
      {""}, {""},
#line 80 "risc8b.gperf"
      {"RLF",      3, {_OP_RCL,      et_f8_d1}},
#line 52 "risc8b.gperf"
      {"CLRA",     4, {_OP_CLRA,     et_None}},
      {""}, {""}, {""},
#line 74 "risc8b.gperf"
      {"ADD",      3, {_OP_ADD,      et_f8_d1}},
#line 48 "risc8b.gperf"
      {"RETZ",     4, {_OP_RETZ,     et_None}},
#line 37 "risc8b.gperf"
      {"RCODE",    5, {_OP_RCODE,    et_k2}},
#line 38 "risc8b.gperf"
      {"WRCODE",   6, {_OP_WRCODE,   et_None}},
      {""},
#line 28 "risc8b.gperf"
      {"WDT",      3, {_OP_CLRWDT,   et_None}},
#line 98 "risc8b.gperf"
      {"ADDL",     4, {_OP_ADDL,     et_k8}},
      {""},
#line 65 "risc8b.gperf"
      {"DECFSZ",   6, {_OP_DECSZ,    et_f8_d1}},
      {""},
#line 78 "risc8b.gperf"
      {"RCL",      3, {_OP_RCL,      et_f8_d1}},
#line 66 "risc8b.gperf"
      {"SWAP",     4, {_OP_SWAP,     et_f8_d1}},
#line 86 "risc8b.gperf"
      {"RETLN",    5, {_OP_RETLN,    et_k8}},
      {""}, {""},
#line 120 "risc8b.gperf"
      {"JNC",      3, {_OP_JNC,      et_k10_pc}},
      {""}, {""}, {""},
#line 123 "risc8b.gperf"
      {"DW",       2, {0,            et_dw}},
      {""},
#line 75 "risc8b.gperf"
      {"ADDF",     4, {_OP_ADD,      et_f8_d1}},
      {""}, {""}, {""},
#line 68 "risc8b.gperf"
      {"AND",      3, {_OP_AND,      et_f8_d1}},
#line 79 "risc8b.gperf"
      {"RCLF",     4, {_OP_RCL,      et_f8_d1}},
#line 67 "risc8b.gperf"
      {"SWAPF",    5, {_OP_SWAP,     et_f8_d1}},
      {""}, {""},
#line 55 "risc8b.gperf"
      {"MOV",      3, {_OP_MOV,      et_F9_d1}},
#line 95 "risc8b.gperf"
      {"ANDL",     4, {_OP_ANDL,     et_k8}},
#line 72 "risc8b.gperf"
      {"XOR",      3, {_OP_XOR,      et_f8_d1}},
      {""}, {""}, {""},
#line 41 "risc8b.gperf"
      {"PUSH",     4, {_OP_PUSHAS,   et_None}},
      {""},
#line 97 "risc8b.gperf"
      {"XORL",     4, {_OP_XORL,     et_k8}},
      {""}, {""},
#line 94 "risc8b.gperf"
      {"MOVL",     4, {_OP_MOVL,     et_k8}},
#line 29 "risc8b.gperf"
      {"SLEEP",    5, {_OP_SLEEP,    et_None}},
#line 40 "risc8b.gperf"
      {"PUSHAS",   6, {_OP_PUSHAS,   et_None}},
      {""}, {""},
#line 69 "risc8b.gperf"
      {"ANDF",     4, {_OP_AND,      et_f8_d1}},
      {""}, {""}, {""}, {""},
#line 117 "risc8b.gperf"
      {"CALL",     4, {_OP_CALL,     et_k12}},
#line 42 "risc8b.gperf"
      {"POPAS",    5, {_OP_POPAS,    et_None}},
#line 73 "risc8b.gperf"
      {"XORF",     4, {_OP_XOR,      et_f8_d1}},
      {""},
#line 118 "risc8b.gperf"
      {"JNZ",      3, {_OP_JNZ,      et_k10_pc}},
#line 56 "risc8b.gperf"
      {"MOVF",     4, {_OP_MOV,      et_F9_d1}},
#line 64 "risc8b.gperf"
      {"DECSZ",    5, {_OP_DECSZ,    et_f8_d1}},
      {""}, {""},
#line 70 "risc8b.gperf"
      {"IOR",      3, {_OP_IOR,      et_f8_d1}},
      {""},
#line 32 "risc8b.gperf"
      {"WAITB",    5, {_OP_WAITB,    et_b3}},
      {""}, {""}, {""},
#line 96 "risc8b.gperf"
      {"IORL",     4, {_OP_IORL,     et_k8}},
      {""},
#line 39 "risc8b.gperf"
      {"EXEC",     4, {_OP_EXEC,     et_k2}},
      {""}, {""},
#line 51 "risc8b.gperf"
      {"RETI",     4, {_OP_RETIE,    et_None}},
      {""},
#line 33 "risc8b.gperf"
      {"WAITRD",   6, {_OP_WAITRD,   et_None}},
#line 119 "risc8b.gperf"
      {"JZ",       2, {_OP_JZ,       et_k10_pc}},
      {""},
#line 57 "risc8b.gperf"
      {"MOVA",     4, {_OP_MOVA,     et_F9}},
      {""},
#line 90 "risc8b.gperf"
      {"MOVA1F",   6, {_OP_MOVA1F,   et_k8}},
      {""},
#line 26 "risc8b.gperf"
      {"NOP",      3, {_OP_NOP,      et_None}},
#line 71 "risc8b.gperf"
      {"IORF",     4, {_OP_IOR,      et_f8_d1}},
      {""}, {""}, {""},
#line 43 "risc8b.gperf"
      {"POP",      3, {_OP_POPAS,    et_None}},
#line 101 "risc8b.gperf"
      {"CMPL",     4, {_OP_CMPL,     et_k8}},
      {""},
#line 93 "risc8b.gperf"
      {"MOVA1P",   6, {_OP_MOVA1P,   et_k8}},
      {""}, {""},
#line 113 "risc8b.gperf"
      {"BG1F",     4, {_OP_BG1F,     et_a2_b3}},
      {""}, {""}, {""},
#line 58 "risc8b.gperf"
      {"INC",      3, {_OP_INC,      et_f8_d1}},
#line 114 "risc8b.gperf"
      {"BG2F",     4, {_OP_BG2F,     et_a2_b3}},
      {""}, {""}, {""},
#line 31 "risc8b.gperf"
      {"SLEEPX",   6, {_OP_SLEEPX,   et_k2}},
#line 30 "risc8b.gperf"
      {"HALT",     4, {_OP_SLEEP,    et_None}},
#line 88 "risc8b.gperf"
      {"MOVIP",    5, {_OP_MOVIP,    et_k9}},
      {""}, {""}, {""},
#line 111 "risc8b.gperf"
      {"BP1F",     4, {_OP_BP1F,     et_a2_b3}},
      {""},
#line 34 "risc8b.gperf"
      {"WAITWR",   6, {_OP_WAITWR,   et_None}},
      {""}, {""},
#line 112 "risc8b.gperf"
      {"BP2F",     4, {_OP_BP2F,     et_a2_b3}},
      {""}, {""}, {""}, {""},
#line 59 "risc8b.gperf"
      {"INCF",     4, {_OP_INC,      et_f8_d1}},
      {""}, {""}, {""}, {""}, {""},
#line 89 "risc8b.gperf"
      {"MOVIA",    5, {_OP_MOVIA,    et_k10}},
      {""}, {""}, {""},
#line 122 "risc8b.gperf"
      {"CMPZ",     4, {_OP_CMPZ,     et_K7_k8}},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
#line 100 "risc8b.gperf"
      {"CMPLN",    5, {_OP_CMPLN,    et_k8}},
      {""}, {""}, {""}, {""}, {""},
#line 63 "risc8b.gperf"
      {"INCFSZ",   6, {_OP_INCSZ,    et_f8_d1}},
      {""},
#line 115 "risc8b.gperf"
      {"JMP",      3, {_OP_JMP,      et_k12}},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 44 "risc8b.gperf"
      {"PUSHA2",   6, {_OP_PUSHA2,   et_None}},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""},
#line 91 "risc8b.gperf"
      {"MOVA2F",   6, {_OP_MOVA2F,   et_k8}},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 92 "risc8b.gperf"
      {"MOVA2P",   6, {_OP_MOVA2P,   et_k8}},
      {""}, {""}, {""}, {""}, {""},
#line 35 "risc8b.gperf"
      {"WAITSPI",  7, {_OP_WAITWR,   et_None}},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 62 "risc8b.gperf"
      {"INCSZ",    5, {_OP_INCSZ,    et_f8_d1}},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 45 "risc8b.gperf"
      {"POPA2",    5, {_OP_POPA2,    et_None}}
    };

    if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH) {
        register unsigned int key = hash(str, len);

        if (key <= MAX_HASH_VALUE) {
            if (len == mnemonic_table[key].len) {
                register const char *s = mnemonic_table[key].name;

                //if (*str == *s && !memcmp(str + 1, s + 1, len - 1))
                //    return &mnemonic_table[key].info;
                if (*str == *s) {
                    unsigned long long ll1 = *(unsigned long long*)s;
                    unsigned long long ll2 = *(unsigned long long*)str;
                    // 2->FFFF, 3->FFFFFF
                    unsigned long long mask = (1ULL << (len * 8)) - 1;
                    if (ll1 ^ ll2 & mask) {
                        return &mnemonic_table[key].info;
                    }
                }
            }
        }
    }
    return 0;
}
