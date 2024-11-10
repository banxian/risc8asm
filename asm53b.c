#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>
#include <stdarg.h>
#ifdef _MSC_VER
#include <malloc.h>
#endif
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#define _mkdir(path) mkdir(path, 0755)
#define stricmp(s1,s2) strcasecmp(s1,s2)
#endif
#include "risc8bins.h"


#define MAX_LINES 9999
#define MAX_ADDR 0x1000
#define MAX_SYMBOL 4096
#define ARRAY_INCREMENTAL 16
#define BANNER "MCU CH53X ASSEMBLER:  ASM53B Ver 0.2\nCompatible with original WASM53B assembler\nFor bug report, visit: https://github.com/banxian/risc8asm\n\n"


enum symbol_status
{
    ssUndefined,
    ssRedefined,
    ssUnsed,
    ssNormal
};

struct label_item_s
{
    char name[28];  // padded with spaces
    char status;    // undefined/redefined/unused/normal
    char type;      // 0=label, 1=constant
    int address;    // PC address or constant value
};

void save_symbol(char *line, int addr, char type);
struct label_item_s * find_symbol(const char* symbol);
int pass1_readlines(FILE *file, const char* srcdir, int addr);
int pass2_assemble(FILE *file, const char* srcdir, int addr);
int solve_number(char *str); // will fill parsed number
void cleanup(void);
void log_info(const char *info, int code);
void log_warning(const char *warn, int code);
void log_error(const char *error, int code);
#ifdef LITE_PRINTF
int listprintf(__in_z __format_string const char * _Format, ...);
#else
#define listprintf(fmt, ...) fprintf(g_listfile, fmt, ##__VA_ARGS__)
#endif


FILE *g_srcfile = NULL;
FILE *g_listfile = NULL;
struct label_item_s* g_symbolpool = NULL; // need store more than 4096 because equ
int Total_Label = 0;
int Total_Info = 0;
int Total_Warning = 0;
int Total_Error = 0;
int g_readedlinecount = 0;
int g_nestlevel = 0; // inclusion nest level
char g_indigi = 0; // indicate a number appeared during solve number
unsigned g_logicoplimit = 0; // controlled by assemble
char g_fileline[260]; // raw line from file
char g_linebuf[260]; // holds pre-filtered line, Uppercased
uint16_t g_opcodebuf[MAX_ADDR + 1];
char g_listvbuf[65536];
//char g_srcvbuf[8192];

void usage(char* exepath)
{
    char* ext;
#ifdef _WIN32
    char* cmdname = strrchr(exepath, '\\');
#else
    char* cmdname = strrchr(exepath, '/');
#endif
    cmdname = cmdname ? cmdname + 1 : exepath;
#ifdef _WIN32
    if ((ext = strrchr(cmdname, '.'))) {
        if (stricmp(ext, ".exe") == 0) {
            *ext = 0;
        }
    }
#endif
    printf("Usage: %s source_file [options]\n"
        "\n"
        "Options:\n"
        "  -f         Generate full size (8KB) binary output\n"
        "  -o <name>  Specify name for output file\n"
        "  -h         Generate header file with opcode array\n"
        "\n"
        "Example:\n"
        "  %s mode.asm -o demo -h      Output: demo.LST, demo.BIN, demo_inc.h\n", cmdname, cmdname);
}

bool is_path_delimiter(const char c)
{
#ifdef _WIN32
    return c== '\\' || c == '/';
#else
    return c == '/';
#endif
}

int main(int argc, char *argv[])
{
    const char *srcfname = NULL, *outbase = NULL;
    int fnlen;
    int extpos, slashpos;
    int pass1pc, pass2pc;
    time_t now;
    struct tm tm;
    char* outfname, *lpext;
    char outfpre;
#ifdef _MSC_VER
    char* fullname, *outbaseext;
#endif
    bool fullsizebin = false, exportheader = false;

    g_logicoplimit = 0xFF;
    printf(BANNER);
    for (int i = 1; i < argc; i++) {
        const char * argi = argv[i];
        if ((argi[0] == '/' || argi[0] == '-') &&
            (argi[1] == 'F' || argi[1] == 'f') && argi[2] == 0) {
            fullsizebin = true;
        } else if (strcmp(argi, "-o") == 0) {
            outbase = argv[++i];
        } else if (strcmp(argi, "-h") == 0) {
            exportheader = true;
        } else {
            srcfname = argv[i];
        }
    }
    if (argc < 2 || srcfname == NULL) {
        usage(argv[0]);
        return 5;
    }
    fnlen = strlen(srcfname);
#ifdef _MSC_VER
    fullname = (char*)_alloca(fnlen + sizeof("_inc.h"));
    if (outbase) {
        outbaseext = (char*)_alloca(strlen(outbase) + sizeof("_inc.h")); // may overalloc
    }
#else
    char fullname[fnlen + sizeof("_inc.h")];
    char outbaseext[outbase?strlen(outbase) + sizeof("_inc.h"):1];
#endif
    strcpy(fullname, srcfname);
    if (outbase) {
        strcpy(outbaseext, outbase);
    }
    extpos = 0;
    slashpos = 0;
    for (int i = 0; i < fnlen; i++) {
        if (srcfname[i] == '.') {
            extpos = i + 1;
        } else if (is_path_delimiter(srcfname[i])) {
            extpos = 0;
            slashpos = i + 1;
        }
    }
    // Append .ASM if no extension found
    if (extpos == 0) {
        extpos = fnlen + 1;
        strcpy(&fullname[fnlen], ".ASM");
    }

    printf("Open source file %s\n", fullname);
    g_srcfile = fopen(fullname, "rt");
    if (!g_srcfile) {
        return 11;
    }
    //setvbuf(g_srcfile, g_srcvbuf, _IOFBF, sizeof(g_srcvbuf)); // no need _IOLBF
    if (outbase) {
        int slashpos = 0, extpos = 0;
        for (int i = 0; outbase[i]; i++) {
            if (outbase[i] == '.') {
                extpos = i + 1;
            } else if (outbase[i] == '\\' || outbase[i] == '/') {
                extpos = 0;
                slashpos = i; 
            }
        }
        if (slashpos) {
            char c = outbaseext[slashpos];
            outbaseext[slashpos] = 0;
            _mkdir(outbaseext);
            outbaseext[slashpos] = c;
        }
        outfname = outbaseext;
        if (extpos == 0) {
            lpext = &outbaseext[strlen(outbaseext)];
            *lpext++ = '.';
        } else {
            lpext = &outbaseext[extpos];
        }
    } else {
        outfname = &fullname[slashpos]; // make sure output files is placed on build folder rather than source folder
        lpext = &fullname[extpos];
    }
    strcpy(lpext, "LST");
    printf("Create list file %s\n", outfname);
    g_listfile = fopen(outfname, "wt");
    if (!g_listfile) {
        fclose(g_srcfile);
        return 12;
    }
    setvbuf(g_listfile, g_listvbuf, _IOFBF, sizeof(g_listvbuf));
    listprintf(BANNER);
    listprintf("List file: %s\n", outfname);
    time(&now);
#ifdef _WIN32
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    listprintf("Date: %04d.%02d.%02d  Time: %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    //atexit(&cleanup);// will invoke after exit or normal return
    g_symbolpool = (struct label_item_s *)malloc(sizeof(struct label_item_s) * MAX_SYMBOL);
    if (g_symbolpool == NULL) {
        fclose(g_listfile);
        fclose(g_srcfile);
        return 21;
    }
    memset(g_opcodebuf, 0, sizeof(g_opcodebuf));
    listprintf("\nPass1 -------------------------------------------------------------------------\n");
    listprintf("LINE ,  PC ,  CODE/DATA: SOURCE\n");
    if (slashpos) {
        outfpre = fullname[slashpos];
        fullname[slashpos] = 0; // fullname became folder path
    }

    pass1pc = pass1_readlines(g_srcfile, slashpos?fullname:NULL, 0); // solve labels, EQU, ORG
    g_readedlinecount = 0;
    g_fileline[0] = 0;
    if (pass1pc != -1 && Total_Error < 100) {
        rewind(g_srcfile);
        listprintf("\nPass2 -------------------------------------------------------------------------\n");
        listprintf("LINE ,  PC ,  CODE/DATA: SOURCE\n");
        pass2pc = pass2_assemble(g_srcfile, slashpos?fullname:NULL, 0); // generate opcodes
        g_readedlinecount = 0;
        g_fileline[0] = 0;
        if (pass2pc != -1 && pass2pc != pass1pc) {
            printf("PC of pass1 is %04XH, PC of pass2 is %04XH\n", pass1pc, pass2pc);
            log_error("PC of pass 2 is different from pass 1", pass1pc);
        }
    }
    fclose(g_srcfile);
    g_srcfile = 0;
    // list symbols (label and equ)
    listprintf("\nLabel = %3d -------------------------------------------------------------------\n", Total_Label);
    listprintf("......name....................value.....type....\n");
    for (int i = 0; i < Total_Label; i++) {
        const struct label_item_s *sym = &g_symbolpool[i];
        listprintf(".. %s .. %04X .. %s\n",
            sym->name,
            sym->address,
            sym->status == ssUndefined ? "undefined" :
            sym->status == ssRedefined ? "redefined" :
            sym->status == ssUnsed ? "unused" :
            sym->status == ssNormal ? "normal" : "??????");
    }

    listprintf("\nEnd = %04XH -------------------------------------------------------------------\n", pass2pc);
    if (pass2pc != -1 && pass2pc == pass1pc) {
        FILE* binfile, *hdrfile;
        strcpy(lpext, "BIN");
        if (slashpos) {
            fullname[slashpos] = outfpre; // dir back to file path
        }
        printf("Create data file %s\n", outfname);
        binfile = fopen(outfname, "wb");
        if (binfile) {
            size_t writecnt = fullsizebin?4096:pass2pc;
            size_t writed = fwrite(g_opcodebuf, 2, writecnt, binfile); // 2byte per instruction
            fclose(binfile);
            if (writed != writecnt) {
                printf("Can not write code to data file\n");
                log_error("error writing code to data file", writed);
            }
        } else {
            printf("Can not create data file\n");
            log_error("error creating data file", 0);
        }
        if (exportheader) {
            strcpy(lpext - 1, "_inc.h");
            hdrfile = fopen(outfname, "wt");
            if (hdrfile) {
                size_t bytecnt = (fullsizebin ? 4096 : pass2pc)*2;
                fprintf(hdrfile, "const char PICO_CODE[] = {\n");
                for (size_t i = 0; i < bytecnt; i++) {
                    const char* fmt = (i % 16 == 0)?"    0x%02X,":(i == bytecnt - 1)?" 0x%02X\n":(i % 16 == 15)?" 0x%02X,\n":" 0x%02X,";
                    fprintf(hdrfile, fmt, *((unsigned char*)g_opcodebuf + i));
                }
                fprintf(hdrfile, "}\n");
                fclose(hdrfile);
            }
        }
    }
    // if warning and error present
    if (Total_Warning || Total_Error) {
        printf("END=%04XH, Total_Info=%02d, Total_Warning=%02d, Total_Error=%02d\n", pass2pc, Total_Info, Total_Warning, Total_Error);
    } else if (Total_Info) {
        printf("Success,  END=%04XH,  Total_Info=%02d\n", pass2pc, Total_Info);
    } else {
        printf("Success,  END=%04XH\n", pass2pc);
    }
    listprintf("Total_Info: %02d, Total_Warning: %02d, Total_Error: %02d\n", Total_Info, Total_Warning, Total_Error);
    if (g_symbolpool) {
        free(g_symbolpool);
    }
    fflush(g_listfile);
    fclose(g_listfile);
    return 0;
}

void cleanup(void)
{
    if (g_symbolpool) {
        free(g_symbolpool);
    }
    if (g_listfile) {
        fflush(g_listfile);
        fclose(g_listfile);
    }
    if (g_srcfile) {
        fclose(g_srcfile);
    }
}

bool is_valid_label_char(char c) {
    return (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
            c == '_' || c == '$' || c == '#' || c == '@';
}

bool is_first_label_char(char c) {
    return (c >= 'A' && c <= 'Z') ||
            c == '_' || c == '$' || c == '#' || c == '@';
}

// Binary search in ordered symbol table
// Returns index if found, or -(insertion_point + 1) if not found
int binary_search(const char *name) {
    int left = 0, right = Total_Label - 1;

    while (left <= right) {
        int mid = (left + right) >> 1;
        int cmp = memcmp(g_symbolpool[mid].name, name, 28); // memcmp may faster than strncmp
        if (cmp == 0) return mid;
        if (cmp < 0) left = mid + 1;
        else right = mid - 1;
    }
    return -(left + 1);
}

/// Save a symbol (label or constant) to the global symbol table
/// @param line Source line containing the symbol (will be wiped)
/// @param addr Symbol value - PC address for labels, constant value for EQU
/// @param type Symbol type: 0 for labels, 1 for constants(EQU)
/// @note Maintains symbol table in sorted order for binary search
/// @note Detects and handles symbol redefinition
void save_symbol(char *line, int addr, char type)
{
    int pos;
    char name[28];
    char c = 255;
    if (Total_Label >= MAX_SYMBOL) {
        log_error("too many label", Total_Label);
        return;
    }
    for (int i = 0; i < 27; i++) {
        if (c != ' ') {
            c = line[i];
            if (is_valid_label_char(c)) {
                line[i] = ' '; // wipe input line element
            } else {
                c = ' '; // input untouch, store space on symbol name
            }
        }
        name[i] = c;
    }
    name[27] = 0;

    // ordered insertion runs 13x faster than unordered table + sort on print
    pos = binary_search(name);
    if (pos >= 0) {
        // alternative found symbol
        if (g_symbolpool[pos].address != addr) {
            g_symbolpool[pos].address = addr;
            log_warning("label redefined to new value", addr);
        } else {
            log_warning("label redefined", addr);
        }
        g_symbolpool[pos].status = ssRedefined;
        return;
    }
    pos = -pos - 1;

    // TODO: next stores index so can enlarge dynamically
    //if (Total_Label % ARRAY_INCREMENTAL == 0) {
    //    g_symbolpool = (struct label_item_s*)realloc(g_symbolpool, sizeof(struct label_item_s) * (Total_Label + ARRAY_INCREMENTAL));
    //}

    // leave a blank for new symbol
    memmove(&g_symbolpool[pos + 1], &g_symbolpool[pos], (Total_Label - pos) * sizeof(struct label_item_s));

    // setup new symbol
    memcpy(g_symbolpool[pos].name, name, sizeof(name));
    g_symbolpool[pos].address = addr;
    g_symbolpool[pos].type = type;
    g_symbolpool[pos].status = (addr == -1) ? ssUndefined : ssUnsed;

    ++Total_Label;
}

/// Find a symbol in the global symbol table using binary search
/// @param symbol Symbol name to search for
/// @return Pointer to found symbol item, or NULL if not found
/// @note Symbol names in table was padded with spaces
struct label_item_s * find_symbol(const char* symbol)
{
    int pos;
    char name[28];
    char c = 255;
    // normalize symbol name
    for (int i = 0; i < 27; i++) {
        // tail will fill with space if once meet space in line
        if (c != ' ') {
            c = symbol[i];
            if (false == is_valid_label_char(c)) {
                c = ' '; // space on symbol name
            }
        }
        name[i] = c;
    }
    name[27] = 0;

    pos = binary_search(name);
    return pos >= 0 ? &g_symbolpool[pos] : NULL;
}

bool is_tab_or_space(const char c)
{
    return (c == ' ') || (c == '\t');
}

bool is_HEX(const char c)
{
    return ((unsigned char)c >= '0' && (unsigned char)c <= '9') || ((unsigned char)c >= 'A' && (unsigned char)c <= 'Z');
}

const char* skip_prefix_spaces(const char* str)
{
    while (*str && is_tab_or_space(*str)) {
        str++;
    }
    return str;
}

const char* skip_printable_chars(const char* str)
{
    while (*str && *str > ' ') {
        str++;
    }
    return str;
}

/// Parse and evaluate numeric expressions in assembly source
/// @param str Input uppercase string containing expression (processed part will be wiped)
/// @return Evaluated value, or -1 on error
/// @note Handles:
/// @note - Decimal/hex/binary/character constants
/// @note - Labels and symbols
/// @note - Unary operators (-,~)
/// @note - Binary operators (+,-,*,/,%,&,|,^,<<,>>)
/// @note Recursively evaluates complex expressions
/// @note Modified input string by replacing processed parts with spaces
int solve_number(char *str)
{
    unsigned int address;
    char *head;
    char *opstr;
    char c0;
    unsigned short w0, op;
    struct label_item_s *curr;
    bool havepost;
    unsigned int operand;

    // skip leading whitespace
    head = (char*)skip_prefix_spaces(str);
    c0 = head[0];
    w0 = c0 << 8 | head[1];
    // numbers starting with: digits, single quote, or H' D' B' prefix
    if ((c0 >= '0' && c0 <= '9') || c0 == '\'' || w0 == 'H\'' || w0 == 'D\'' || w0 == 'B\'') {
        address = 0;
        g_indigi = 1;
        if (c0 == '\'') {
            address = (unsigned char)g_fileline[head - g_linebuf + 1]; // get raw character from original line
            head += 2; // bug on {'a } 
        } else if (c0 == 'H' || (w0 == '0X')) {
            head += 2;
            address = strtol(head, &head, 16);
        } else if (c0 == 'B' || (w0 == '0B')) {
            head += 2;
            address = strtol(head, &head, 2);
        //} else if (c0 == '0' && head[1] >= '0' && head[1] <= '9') {
        //    address = strtol(head, &head, 8);
        } else {
            if (w0 == '0D') {
                head += 2;
            }
            address = strtol(head, &head, 10);
        }
        // head moved to end of processed number now
        c0 = head[0];
        w0 = c0 << 8 | head[1];
        havepost = (w0 == '<<') || (w0 == '>>');

        if (c0 > ' ') {
            if (c0 == '\'') {
                ++head;
            } else if (c0 != ';' && c0 != '+' && c0 != '-' && c0 != '*' && c0 != '/' && c0 != '%' && c0 != '&' && c0 != '|' && c0 != '^' && !havepost) {
                address = -1;
            }
        }

        // fill space from begin to current
        while (head > str) {
            *--head = ' ';
        }
    } else if (is_first_label_char(c0)) {
        // search in symbol list
        curr = find_symbol(head);
        if (curr) {
            if (curr->status == ssUnsed)
                curr->status = ssNormal;
        } else {
            log_error("label undefined", g_readedlinecount);
            save_symbol(head, -1, -1); // prevent next error
            return -1;
        }
        address = curr->address;
        // fill space until meet non label chars
        while (*head && is_valid_label_char(*head)) {
            *head++ = ' ';
        }
    } else if (c0 == '-' || c0 == '~') {
        address = 0; // for unary operator -
    } else {
        return -1;
    }

    if (address == -1) {
        return address;
    }

    opstr = (char*)skip_prefix_spaces(head); // head is closer to operator than str?
    c0 = opstr[0];
    w0 = c0 << 8 | opstr[1];
    havepost = (w0 == '<<') || (w0 == '>>');

    // does expression finished here?
    if (c0 != '+' && c0 != '-' && c0 != '*' && c0 != '/' && c0 != '%' && c0 != '&' && c0 != '|' && c0 != '^' && c0 != '~' && !havepost) {
        return address;
    }

    *opstr++ = ' ';
    if (havepost) {
        *opstr++ = ' ';
    }
    // operator str became operand str
    operand = solve_number(opstr); // nested
    g_indigi = 0; // clear flag once recursion parse occupied
    if (operand == -1) {
        return -1;
    }

    op = havepost?(w0):c0;
    switch (op) {
    case '+' : return (uint16_t)(address + operand);
    case '-' : return (uint16_t)(address - operand);
    case '*' : return (uint16_t)(address * operand);
    case '/' : return operand == 0 ? -1 : address / operand;
    case '%' : return operand == 0 ? -1 : address % operand;
    case '&' : return address & operand;
    case '|' : return address | operand;
    case '^' : return address ^ operand;
    case '~' : return g_logicoplimit & ~operand;
    case '<<': return g_logicoplimit & (address << operand);
    case '>>': return g_logicoplimit & (address >> operand);
    default  : return address;
    }
}

// used for debug
/*int solve_number(char *str)
{
    printf("[%s] -> ", str);
    int num = solve_number2(str);
    printf("[%s] = %d\n", str, num);
    return num;
}*/

/// Pre-process a source line for pass1/pass2
/// @return Length of processed line
/// @note Performs:
/// @note - Convert to uppercase
/// @note - Replace tabs/commas with spaces
/// @note - Remove comments
/// @note - Store result in g_linebuf
int prefilter_fileline()
{
    size_t linelen = strlen(g_fileline);
    bool incharconst = false;
    while (linelen && (g_fileline[linelen - 1] == '\n' || g_fileline[linelen - 1] == '\r')) {
        g_fileline[--linelen] = 0; // remove tail LF n CR
    }
    if (linelen >= 250) {
        log_warning("line too long", linelen);
    }
    for (size_t i = 0; i < linelen; i++) {
        char c = g_fileline[i];
        if (c >= 'a' && c <= 'z') {
            c -= 0x20; // Uppercase
        }
        if (c == '\'') {
            incharconst = !incharconst;
        } else if (c < ' ') {
            c = ' ';
        } else if (c == ';' && !incharconst) {
            g_linebuf[i] = 0;
            return i;
        } else if (c == ',' && !incharconst) {
            c = ' '; // Replace comma to space
        }
        g_linebuf[i] = c;
    }
    g_linebuf[linelen] = 0;
    return linelen;
}

typedef int (*pass_func)(FILE *file, const char* srcdir, int addr);

/// Process an INCLUDE directive
/// @param head Pointer to the include statement
/// @param addr Current PC address (modified after nestcall)
/// @param srcdir Source directory path
/// @param nestcall Function to process included file (pass1 or pass2)
/// @note Limits nesting to 8 levels
/// @note Resolves included file path relative to parent file
void process_inclusion(char* head, int* addr, const char* srcdir, pass_func nestcall)
{
    if (g_nestlevel <= 8) {
        size_t srcdirlen = srcdir ? strlen(srcdir) : 0;
#ifdef _MSC_VER
        char* joinedpath;
#endif
        FILE* incfile;
        int slashpos = 0;
        char* incname = (char*)skip_prefix_spaces(&head[7]);
        int inclen = 0;
        for (; incname[inclen]; inclen++) {
            if (incname[inclen] == '\\' || incname[inclen] == '/') {
                slashpos = inclen + 1;
            }
            if (is_tab_or_space(incname[inclen])) {
                incname[inclen] = 0;
                break;
            }
        }
#ifdef _MSC_VER
        joinedpath = (char*)_alloca(srcdirlen + inclen + 2);
#else
        char joinedpath[srcdirlen + inclen + 2];
#endif
        if (srcdirlen) {
            strncpy(joinedpath, srcdir, srcdirlen);
        }
        strncpy(&joinedpath[srcdirlen], &g_fileline[incname - g_linebuf], inclen); // take from fileline with raw case
        joinedpath[srcdirlen+inclen] = 0;
        incfile = fopen(joinedpath, "rt");
        //printf("joinedpath: %s\n", joinedpath);
        joinedpath[srcdirlen + slashpos] = 0;
        if (incfile) {
            int savedlinecnt = g_readedlinecount;
            g_nestlevel++;
            *addr = nestcall(incfile, joinedpath, *addr);
            g_nestlevel--;
            g_readedlinecount = savedlinecnt;
            fclose(incfile);
            listprintf("## return from nesting file\n");
        } else {
            log_error("error opening include file", 0);
        }
    } else {
        log_error("too many nesting layer", g_nestlevel);
    }
}

bool is_end_directive(const char* head)
{
    return head[0] == 'E' && head[1] == 'N' && head[2] == 'D' && head[3] <= ' ';
}

bool is_org_directive(const char* head)
{
    return head[0] == 'O' && head[1] == 'R' && head[2] == 'G' && is_tab_or_space(head[3]);
}

bool is_include_directive(const char* head)
{
    return *(uint32_t*)head == 'LCNI' && *(uint16_t*)&head[4] == 'DU' && head[6] == 'E' && is_tab_or_space(head[7]);
}

/// Perform first pass of assembly to collect all symbols
/// @param file Source file handle
/// @param srcdir Source file directory path for resolving includes
/// @param addr Starting PC for code generation
/// @return Final PC after pass 1, or -1 on error
/// @note Collects all symbols but does not generate code
int pass1_readlines(FILE *file, const char* srcdir, int addr)
{
    for (g_readedlinecount = 1; g_readedlinecount < MAX_LINES && addr < MAX_ADDR; g_readedlinecount++) {
        size_t linelen;
        char* head;
        if (NULL == fgets(g_fileline, 252, file)) {
            if (g_nestlevel == 0) {
                log_warning("END not found", g_readedlinecount);
            }
            break;
        }
        linelen = prefilter_fileline();
        head = (char*)skip_prefix_spaces(g_linebuf);
        //printf("%d %d [%s] %zu %04X\n", g_nestlevel, g_readedlinecount, g_linebuf, linelen, addr);
        if (linelen == 0 || head[0] == 0) {
            continue;
        }
        // END, ORG, INCLUDE
        if (is_end_directive(head)) {
            if (g_nestlevel) {
                log_warning("END in include file", g_readedlinecount);
            }
            g_readedlinecount++;
            break;
        } else if (is_org_directive(head)) {
            int orgaddr = solve_number(&head[3]);
            if (orgaddr > 0xFFF) {
                log_error("invalid ORG address", orgaddr);
            } else {
                if (orgaddr < addr) {
                    log_warning("go back by ORG", orgaddr);
                }
                addr = orgaddr;
            }
            continue;
        } else if (is_include_directive(head)) {
            listprintf("%s\n", g_linebuf);
            process_inclusion(head, &addr, srcdir, &pass1_readlines);
            continue;
        }
        // not one of the above 3 cases
        if (head == g_linebuf) {
            if (is_first_label_char(g_linebuf[0])) {
                // label
                int lablen = 0;
                for (; !is_tab_or_space(g_linebuf[lablen]) && lablen < linelen; lablen++);
                if (lablen > 27) {
                    log_warning("label too long", lablen);
                }
                char* equ = (char*)skip_prefix_spaces(&g_linebuf[lablen]);
                lablen = equ - g_linebuf;
                if (equ[0] == 'E' && equ[1] == 'Q' && equ[2] == 'U' && is_tab_or_space(equ[3])) {
                    int val = solve_number(&equ[3]);
                    save_symbol(g_linebuf, val, 1); // named constant
                } else {
                    save_symbol(g_linebuf, addr, 0); // address symbol
                    if (lablen < linelen) {
                        addr++; // label + non-space (maybe instruction)
                    }
                }
            } else {
                log_error("invalid label name", (unsigned char)g_linebuf[0]);
            }
        } else {
            addr++; // maybe instruction line
        }
    }
    if (g_readedlinecount > MAX_LINES) {
        log_error("too many line", g_readedlinecount);
    }
    if (addr > MAX_ADDR) {
        log_error("too many code", addr);
    }
    return addr;
}

int gen_opcode(char* mnemhead, int addr, int* table);

/// Perform second pass of assembly to generate opcodes
/// @param file Source file handle  
/// @param srcdir Source file directory path for resolving includes
/// @param addr Starting PC for code generation
/// @return Final PC after pass 2, or -1 on error
/// @note Uses symbol table from pass 1 to resolve all references
/// @note Generates binary output and listing file
int pass2_assemble(FILE *file, const char* srcdir, int addr)
{
    int table = 0;
    // addr may reach 4097 in original implement
    for (g_readedlinecount = 1; g_readedlinecount < MAX_LINES && addr < MAX_ADDR; g_readedlinecount++) {
        size_t linelen;
        char* head;
        if (NULL == fgets(g_fileline, 252, file)) {
            if (g_nestlevel == 0) {
                log_warning("END not found", g_readedlinecount);
            }
            break;
        }
        linelen = prefilter_fileline();
        head = (char*)skip_prefix_spaces(g_linebuf);
        //printf("%d %d [%s] %d %04X\n", g_nestlevel, g_readedlinecount, g_linebuf, linelen, addr);
        if (linelen == 0 || head[0] == 0) {
            listprintf("L=%04d, ......, D=%04X : %s\n", g_readedlinecount, 0, g_fileline); // type 0, data 0, comment or empty line
            continue;
        }
        // END, ORG, INCLUDE
        if (is_end_directive(head)) {
            if (g_nestlevel) {
                log_warning("END in include file", g_readedlinecount);
            }
            listprintf("L=%04d, P=%04X, .END.. : %s\n", g_readedlinecount, addr, g_fileline);
            g_readedlinecount++;
            break;
        } else if (is_org_directive(head)) {
            int orgaddr = solve_number(&head[3]);
            if (orgaddr > 0xFFF) {
                log_error("invalid ORG address", orgaddr);
            } else {
                if (orgaddr < addr) {
                    log_warning("go back by ORG", orgaddr);
                }
                addr = orgaddr;
            }
            listprintf("L=%04d, P=%04X, ...... : %s\n", g_readedlinecount, addr, g_fileline); // type 1
            continue;
        } else if (is_include_directive(head)) {
            listprintf("L=%04d, NEST_INCLUDE=%d : %s\n", g_readedlinecount, g_nestlevel + 1, g_fileline); // type 2
            process_inclusion(head, &addr, srcdir, &pass2_assemble);
            continue;
        }
        // not one of the above 3 cases, check label
        char* mnemhead = 0;
        // content start at line begin
        if (head == g_linebuf) {
            if (is_first_label_char(g_linebuf[0])) {
                // label
                int lablen;
                for (lablen = 0; !is_tab_or_space(g_linebuf[lablen]) && lablen < linelen; lablen++);
                if (lablen > 27) {
                    log_warning("label too long", lablen);
                }
                char* equ = (char*)skip_prefix_spaces(&g_linebuf[lablen]);
                lablen = equ - g_linebuf;
                if (equ[0] == 'E' && equ[1] == 'Q' && equ[2] == 'U' && is_tab_or_space(equ[3])) {
                    int val = solve_number(&equ[3]);
                    listprintf("L=%04d, ......, D=%04X : %s\n", g_readedlinecount, val, g_fileline); // type 0, data
                } else {
                    if (lablen < linelen) {
                        mnemhead = equ; // label + non-space (maybe instruction)
                    } else {
                        listprintf("L=%04d, P=%04X, ...... : %s\n", g_readedlinecount, addr, g_fileline); // type 1, addr
                    }
                }
            } else {
                log_error("invalid label name", (unsigned char)g_linebuf[0]);
            }
        } else {
            mnemhead = head;
        }
        if (mnemhead) {
            // check prefix, do assemble
            int opcode = gen_opcode(mnemhead, addr, &table);
            if (opcode != -1) {
                addr++;
                //g_opcodebuf[addr++];
            }
        }
    }
    if (g_readedlinecount > MAX_LINES) {
        log_error("too many line", g_readedlinecount);
    }
    if (addr > MAX_ADDR) {
        log_error("too many code", addr); // only hit on bug
    }
    return addr;
}

int parse_f_F_address(char* regnumber, bool isf8);
#define optional_F_post(c,n) (c <= ' ' || (c == 'F' && n <= ' '))

/// Generate opcode for an instruction
/// @param mnemhead Pointer to instruction mnemonic
/// @param addr Current PC address
/// @param table Table boundary tracking from callee
/// @return Generated opcode, or -1 on error
/// @note Updates listing file with generated code
int gen_opcode(char* mnemhead, int addr, int* table)
{
    int opcode = 0;
    int instype = -1;
    char pf0 = mnemhead[0];
    char pf1 = mnemhead[1];
    char pf2 = mnemhead[2];
    char pf3 = mnemhead[3];
    char pf4 = mnemhead[4];
    char pf5 = mnemhead[5];
    char pf6 = mnemhead[6];

    // TODO: Endian macro to make double/triple
    unsigned short prefixd = (pf0 << 8) | pf1;
    unsigned int prefixt = (pf0 << 16) | (pf1 << 8) | pf2;
    unsigned short mid2 = (pf3 << 8) | pf4;
    unsigned int mid3 = (pf3 << 16) | (pf4 << 8) | pf5;

    char* paras;

    switch (prefixt) {
    case 'NOP':
        if (pf3 <= ' ') {
            opcode = _OP_NOP;
            instype = et_None;
        }
        break;
    case 'CLR': // CLR=CLRF/CLRA/CLRWDT=WDT
        // CLRA
        if (pf3 == 'A' && pf4 <= ' ') {
            opcode = _OP_CLRA;
            instype = et_None;
        }
        // CLR,f=CLRF,f
        if (optional_F_post(pf3, pf4)) {
            opcode = _OP_CLR;
            instype = et_f8; // f
        }
        if (mid3 == 'WDT') {
            opcode = _OP_CLRWDT;
            instype = et_None;
        }
        // trick! continue to WDT if no break
        break;
    case 'WDT':
        if (pf3 <= ' ') {
            opcode = _OP_CLRWDT;
            instype = et_None;
        }
        break;
    case 'SLE': // SLEEP/SLEEPX
        if (mid2 == 'EP' && pf5 <= ' ') {
            opcode = _OP_SLEEP;
            instype = et_None;
        }
        if (mid3 == 'EPX' && pf6 <= ' ') {
            opcode = _OP_SLEEPX;
            instype = et_k2; // k2
        }
        break;
    case 'HAL':
        if (pf3 == 'T' && pf4 <= ' ') {
            opcode= _OP_SLEEP;
            instype = et_None;
        }
        break;
    case 'WAI': // WAITB/WAITRD/WAITWR/WAITSPI
        if (pf3 == 'T' && !is_tab_or_space(pf4)) {
            if (pf4 == 'B' && pf5 <= ' ') {
                opcode = _OP_WAITB;
                instype = et_b3; // b
            }
            if (pf4 == 'R' && pf5 == 'D' && pf6 <= ' ') {
                opcode = _OP_WAITRD;
                instype = et_None;
            }
            // WAITWR/WAITSPI
            if ((pf4 == 'W' && pf5 == 'R' && pf6 <= ' ') || (pf4 == 'S' && pf5 == 'P' && pf6 == 'I' && mnemhead[7] <= ' ')) {
                opcode = _OP_WAITWR;
                instype = et_None;
            }
        }
        break;
    case 'RDC':
        if (mid3 == 'ODE' && pf6 <= ' ') {
            opcode = _OP_RDCODE;
            instype = et_None;
        }
        break;
    case 'RCO':
        if (mid2 == 'DE' && pf5 <= ' ') {
            opcode = _OP_RCODE;
            instype = et_k2; // k2
        }
        break;
    case 'RCL': // RCL=RCLF=RLF
        if (optional_F_post(pf3, pf4)) {
            opcode = _OP_RCL;
            instype = et_f8_d1; // f,d
        }
        break;
    case 'RLF':
        if (pf3 <= ' ') {
            opcode = _OP_RCL;
            instype = et_f8_d1; // f,d
        }
        break;
    case 'RCR': // RCR=RCRF=RRF
        if (optional_F_post(pf3, pf4)) {
            opcode = _OP_RCR;
            instype = et_f8_d1; // f,d
        }
        break;
    case 'RRF':
        if (pf3 <= ' ') {
            opcode = _OP_RCR;
            instype = et_f8_d1; // f,d
        }
        break;
    case 'WRC':
        if (mid3 == 'ODE' && pf6 <= ' ') {
            opcode = _OP_WRCODE;
            instype = et_None;
        }
        break;
    case 'EXE':
        if (pf3 == 'C' && pf4 <= ' ') {
            opcode = _OP_EXEC;
            instype = et_k2; // k2
        }
        break;
    case 'BTS': // BTSC=BTFSC/BTSS=BTFSS
        if ((pf3 == 'C' || pf3 == 'S') && pf4 <= ' ') {
            opcode = pf3 == 'C'?_OP_BTSC:_OP_BTSS;
            instype = et_f8_b3; // f,b
        }
        break;
    case 'BTF':
        if (pf3 == 'S') {
            if ((pf4 == 'C' || pf4 == 'S') && pf5 <= ' ') {
                opcode = pf4 == 'C' ? _OP_BTSC : _OP_BTSS;
                instype = et_f8_b3; // f,b
            }
        }
        break;
    case 'BCT':
        if (pf3 == 'C' && pf4 <= ' ') {
            opcode = _OP_BCTC;
            instype = et_k2; // a?!
        }
        break;
    case 'BP1':
    case 'BP2':
        if (pf3 == 'F' && pf4 <= ' ') {
            opcode = pf2=='1'?_OP_BP1F:_OP_BP2F;
            instype = et_a2_b3; // a,b
        }
        break;
    case 'BG1':
    case 'BG2':
        if (pf3 == 'F' && pf4 <= ' ') {
            opcode = pf2 == '1' ? _OP_BG1F : _OP_BG2F;
            instype = et_a2_b3; // a,b
        }
        break;
    case 'MOV': // MOV=MOVF/MOVA=MOVAF/MOVIP/MOVIA/MOVA1F/MOVA2F/MOVA1P/MOVA2P/MOVL
        if (pf3 == 'A') {
            // MOVA,F=MOVAF,F
            if (optional_F_post(pf4, pf5)) {
                opcode = _OP_MOVA;
                instype = et_F9; // F
            }
            if ((pf4 == '1' || pf4 == '2') && (pf5 == 'F' || pf5 == 'P') && pf6 <= ' ') {
                if (pf5 == 'F') {
                    // MOVA1F/MOVA2F
                    opcode = pf4 == '1'?_OP_MOVA1F:_OP_MOVA2F;
                    instype = et_k8; // k
                } else {
                    // MOVA1P/MOVA2P
                    opcode = pf4 == '1'?_OP_MOVA1P:_OP_MOVA2P;
                    instype = et_k8; // k
                }
            }
        }
        // MOVIP/MOVIA
        if (pf3 == 'I' && pf5 <= ' ') {
            if (pf4 == 'P') {
                opcode = _OP_MOVIP;
                instype = et_k9; // k9
            } else if (pf4 == 'A') {
                opcode = _OP_MOVIA;
                instype = et_k10; // k10
            }
        }
        // MOVL
        if (pf3 == 'L' && pf4 <= ' ') {
            opcode = _OP_MOVL;
            instype = et_k8; // k
        }
        // MOV=MOVF
        if (optional_F_post(pf3, pf4)) {
            opcode = _OP_MOV;
            instype = et_F9_d1; // F,d
        }
        break;
    case 'INC': // INC=INCF/INCSZ=INCFSZ
    case 'DEC': // DEC=DECF/DECSZ=DECFSZ
        if (optional_F_post(pf3, pf4)) {
            opcode = pf0 == 'I'?_OP_INC:_OP_DEC;
            instype = et_f8_d1; // f,d
        }
        if ((mid2 == 'SZ' && pf5 <= ' ') || (mid3 == 'FSZ' && pf6 <= ' ')) {
            opcode = pf0 == 'I' ? _OP_INCSZ : _OP_DECSZ;
            instype = et_f8_d1; // f,d
        }
        break;
    case 'SWA': // SWAP/SWAPF
        if (pf3 == 'P' && (optional_F_post(pf4, pf5))) {
            opcode = _OP_SWAP;
            instype = et_f8_d1; // f,d
        }
        break;
    case 'AND': // AND=ANDF/ANDL
    case 'IOR': // IOR=IORF/IORL
    case 'XOR': // XOR=XORF/XORL
    case 'ADD': // ADD=ADDF/ADDL
    case 'SUB': // SUB=SUBF/SUBL
        if (optional_F_post(pf3, pf4)) {
            switch (prefixd) {
            case 'AN':opcode = _OP_AND; break;
            case 'IO':opcode = _OP_IOR; break;
            case 'XO':opcode = _OP_XOR; break;
            case 'AD':opcode = _OP_ADD; break;
            default  :opcode = _OP_SUB; break;
            }
            instype = et_f8_d1; // f,d
        }
        if (pf3 == 'L' && pf4 <= ' ') {
            switch (prefixd) {
            case 'AN':opcode = _OP_ANDL; break;
            case 'IO':opcode = _OP_IORL; break;
            case 'XO':opcode = _OP_XORL; break;
            case 'AD':opcode = _OP_ADDL; break;
            default  :opcode = _OP_SUBL; break;
            }
            instype= et_k8; // k
        }
        break;
    case 'CMP': // CMPLN/CMPL/CMPZ
        if (pf3 == 'L') {
            if (pf4 == 'N' && pf5 <= ' ') {
                opcode = _OP_CMPLN;
                instype = et_k8; // k
            }
            if (pf4 <= ' ') {
                opcode = _OP_CMPL;
                instype = et_k8; // k
            }
        }
        if (pf3 == 'Z' && pf4 <= ' ') {
            opcode = _OP_CMPZ;
            instype = et_K7_k8; // K7,k
        }
        break;
    case 'JMP':
        if (pf3 <= ' ') {
            opcode = _OP_JMP;
            instype = et_k12; // k12
        }
        break;
    case 'GOT':
        if (pf3 == 'O' && pf4 <= ' ') {
            opcode = _OP_JMP;
            instype = et_k12; // k12
        }
        break;
    case 'CAL':
        if (pf3 == 'L' && pf4 <= ' ') {
            opcode = _OP_CALL;
            instype = et_k12; // k12
        }
        break;
    case 'JNZ':
    case 'JNC':
        if (pf3 <= ' ') {
            opcode = pf2=='Z'?_OP_JNZ:_OP_JNC;
            instype = et_k10_pc; // k10
        }
        break;
    case 'PUS': // PUSHAS=PUSH/PUSHA2
        if (pf3 == 'H') {
            if ((pf4 == 'A' && pf5 == 'S' && pf6 <= ' ') || pf4 <= ' ') {
                opcode= _OP_PUSHAS;
                instype = et_None;
            }
            if (pf4 == 'A' && pf5 == '2' && pf6 <= ' ') {
                opcode = _OP_PUSHA2;
                instype = et_None;
            }
        }
        break;
    case 'POP': // POPAS=POP/POPA2
        if ((mid2 == 'AS' && pf5 <= ' ') || pf3 <= ' ') {
            opcode = _OP_POPAS;
            instype = et_None;
        }
        if (pf3 == 'A' && pf4 == '2' && pf5 <= ' ') {
            opcode = _OP_POPA2;
            instype = et_None;
        }
        break;
    case 'RET': // RET=RETURN/RETZ=RETOK/RETIE=RETI/RETL/RETLN=RETER
        if (pf3 <= ' ' || (mid3 == 'URN' && pf6 <= ' ')) {
            opcode = _OP_RET;
            instype = et_None;
        }
        if ((pf3 == 'Z' && pf4 <= ' ') || (mid2 == 'OK' && pf5 <= ' ')) {
            opcode = _OP_RETZ;
            instype = et_None;
        }
        if ((mid2 == 'IE' && pf5 <= ' ') || (pf3 == 'I' && pf4 <= ' ')) {
            opcode = _OP_RETIE;
            instype = et_None;
        }
        if (pf3 == 'L' && pf4 <= ' ') {
            opcode = _OP_RETL;
            instype = et_k8; // k
        }
        if ((mid2 == 'LN' || mid2 == 'ER') && pf5 <= ' ') {
            opcode = _OP_RETLN;
            instype = et_k8; // k
        }
        break;
    }

    switch (prefixd) {
    case 'BC': // BC=BCF/BCTC
        if (optional_F_post(pf2, pf3)) {
            opcode = _OP_BC;
            instype = et_f8_b3; // f,b
        }
        break;
    case 'BS': // BS=BSF
        if (optional_F_post(pf2, pf3)) {
            opcode = _OP_BS;
            instype = et_f8_b3; // f,b
        }
        break;
    case 'JZ':
    case 'JC':
        if (pf2 <= ' ') {
            opcode = pf1 == 'Z'?_OP_JZ:_OP_JC;
            instype = et_k10_pc; // k10
        }
        break;
    case 'DB':
        if (pf2 <= ' ') {
            opcode = _OP_RETL;
            instype = et_k8; // k
        }
        break;
    case 'DW':
        if (pf2 <= ' ') {
            instype = et_dw; // special
        }
        break;
    }

    if (instype == -1) {
        log_error("unknown instruction", prefixd); // better use FOURCC
        opcode = _OP_NOP; // Does not return and treat as NOP, Compatible to official behavior
        //return -1;
    }

    // Track program counter to ensure table operations stay within 256-byte page boundaries
    if (*table) {
        if (opcode == _OP_RETL || opcode == _OP_RETLN) {
            if ((addr >> 8) != (*table >> 8)) {
                log_warning("table overflow from address", *table);
                *table = 0;
            }
        } else if (opcode == _OP_JMP) {
            if ((addr >> 8) != (*table >> 8)) {
                log_info("table overflow from address", *table);
                *table = 0;
            }
        } else if (instype) {
            *table = 0;
        }
    }

    paras = (char*)skip_prefix_spaces(skip_printable_chars(mnemhead));
    g_indigi = 0; // bugfix
    switch (instype) {
    case et_f8: // CLR f
    case et_F9: // MOVA F
        {
            int faddr = parse_f_F_address(paras, instype == et_f8);
            if (faddr != -1) {
                opcode |= faddr;
            }
        }
        paras = (char*)skip_printable_chars(paras);
        break;
    case et_F9_d1: // MOV F,d 
    case et_f8_d1: // ADD/SUB/AND/IOR/XOR f,d
        {
            int faddr = parse_f_F_address(paras, instype == et_f8_d1);
            if (faddr != -1) {
                char c;
                paras = (char*)skip_prefix_spaces(skip_printable_chars(paras));
                c = *paras;
                if (c == 0) {
                    // Store back to source
                    if (instype == et_F9_d1) {
                        log_info("read data without saving", faddr);
                    } else if (opcode == _OP_ADD && faddr == SFR_PRG_COUNT) {
                        *table = addr + 1; // save PC for next RETL/JMP check, only perform on d is omit
                    }
                    opcode |= faddr | 0x1000; // d
                } else if (paras[1] > ' ') {
                    log_error("unknown destination register", faddr);
                } else if (c == '0' || c == 'A') {
                    opcode |= faddr;
                } else if (c == '1' || c == 'F') {
                    opcode |= faddr | 0x1000; // d
                } else {
                    log_error("unknown destination register", faddr);
                }
            }
        }
        paras = (char*)skip_printable_chars(paras);
        break;
    case et_k8: // RETL/DB/RETLN/RETER k8
                // MOVA1F/MOVA2F k8
                // MOVA1P/MOVA2P k8
        {
            int lit = solve_number(paras);
            if (lit > 0xFF) {
                log_error("invalid literal", lit);
            } else {
                opcode |= lit;
            }
        }
        paras = (char*)skip_printable_chars(paras);
        break;
    case et_k9: // MOVIP k9
        {
            int lit = solve_number(paras);
            if (lit > 0x1FF) {
                log_error("invalid literal", lit);
            } else {
                opcode |= lit;
            }
        }
        paras = (char*)skip_printable_chars(paras);
        break;
    case et_k10: // MOVIA k10
        {
            int lit = solve_number(paras);
            if (lit > 0x3FF) {
                log_error("invalid literal", lit);
            } else {
                opcode |= lit;
            }
        }
        paras = (char*)skip_printable_chars(paras);
        break;
    case et_f8_b3: // BC/BS/BTSC/BTSS f,b
        {
            int faddr = parse_f_F_address(paras, true);
            if (faddr != -1) {
                int bit;
                paras = (char*)skip_prefix_spaces(skip_printable_chars(paras));
                bit = solve_number(paras);
                if (bit > 7) {
                    log_error("invalid bit selection address", bit);
                } else {
                    opcode |= (bit << 8) | faddr;
                }
            }
        }
        paras = (char*)skip_printable_chars(paras);
        break;
    case et_k12: // JMP/CALL k12
        {
            int target = solve_number(paras);
            if (target == -1 || target > 0xFFF) {
                log_error("invalid address of transfer", target);
            } else {
                opcode |= target;
            }
        }
        paras = (char*)skip_printable_chars(paras);
        break;
    case et_k10_pc: // JZ/JNZ/JC/JNC k10
        {
            int target = solve_number(paras); // target can be full range (0..FFF)
            if (target != -1 && target > 0xFFF) {
                log_error("invalid address of transfer", target);
            } else if ((target >> 10) != ((addr + 1) >> 10)) {
                log_error(" 10-bit branch jump out of range", target);
            } else {
                // target[11:10]==(addr+1)[11:10]
                opcode |= target & 0x3FF;
            }
        }
        paras = (char*)skip_printable_chars(paras);
        break;
    case et_K7_k8: // CMPZ K7,k8
        {
            int lit7 = solve_number(paras);
            if (lit7 == -1 || lit7 > 0x7F) {
                log_error("invalid 7-bit literal", lit7);
                break; // fit buggy official behavior
            } else {
                paras = (char*)skip_prefix_spaces(skip_printable_chars(paras));
                int target = solve_number(paras);
                if (target != -1 && target > 0xFFF) {
                    log_error("invalid address of transfer", target);
                } else if ((target >> 8) != ((addr + 1) >> 8)) {
                    log_error(" 8-bit branch jump out of range", target);
                } else {
                    // target[11:8]==(addr+1)[11:8]
                    opcode |= (unsigned char)target | (lit7 << 8);
                }
            }
        }
        paras = (char*)skip_printable_chars(paras);
        break;
    case et_k2: // SLEEPX/BCTC/EXEC k2
        {
            int lit = solve_number(paras);
            if (lit > 3) {
                log_error("invalid literal", lit);
            } else {
                if (opcode == _OP_BCTC && prefixd == 'BC') {
                    // BCTC,0=WRCODE?
                    if (g_indigi) {
                        log_info("using label is better", lit);
                    }
                }
                opcode |= lit;
            }
        }
        paras = (char*)skip_printable_chars(paras);
        break;
    case et_b3: // WAITB b
        {
            int bit = solve_number(paras);
            if (bit == -1 || bit > 7) {
                log_error("invalid literal", bit);
            } else {
                if (g_indigi) {
                    log_info("using label is better", bit);
                }
                opcode |= bit;
            }
        }
        paras = (char*)skip_printable_chars(paras);
        break;
    case et_a2_b3: // BPxF/BGxF a,b
        {
            int alonebit = solve_number(paras);
            if (alonebit == -1 || alonebit > 3) {
                log_error("invalid alone bit", alonebit);
                break;
            }
            if (g_indigi) {
                log_info("using label is better", alonebit);
            }
            paras = (char*)skip_prefix_spaces(skip_printable_chars(paras));
            int bitaddr = solve_number(paras);
            if (bitaddr > 7) {
                log_error("invalid bit selection address", bitaddr);
            } else {
                opcode |= (alonebit << 3) | bitaddr; // aabbb
            }
        }
        paras = (char*)skip_printable_chars(paras);
        break;
    case et_dw: // DW data
        {
            g_logicoplimit = 0xFFFF;  // Allow full 16-bit values on ~,<<,>>
            int val = solve_number(paras);
            opcode = val; // no extra error chk
            g_logicoplimit = 0xFF;    // Restore normal 8-bit limit
        }
        paras = (char*)skip_printable_chars(paras);
        break;
    }
    // encoding_as_is do not skip one arg
    //paras = (char*)skip_printable_chars(paras);

    if (instype) {
        listprintf("L=%04d, P=%04X, C=%04X : %s\n", g_readedlinecount, addr, opcode, g_fileline);
        if (addr != -1) {
            g_opcodebuf[addr++] = opcode;
        }
        paras = (char*)skip_prefix_spaces(paras);
        if (*paras) {
            log_warning("too many parameters", paras - g_linebuf); // position
        }
    }

    return opcode;
}

int parse_f_F_address(char* regnumber, bool isf8)
{
    int faddr = solve_number(regnumber);
    if (isf8 && faddr > 0xFF) {
        log_error("invalid address of f register", faddr);
        faddr = (unsigned char)faddr;
    } else if (faddr > 0x1FF) {
        log_error("invalid address of F register", faddr);
        return -1;
    }
    if (g_indigi) {
        log_info("using label is better", faddr);
    }
    return faddr;
}

#ifdef LITE_PRINTF
// Custom listprintf function that supports a limited set of format specifiers
int listprintf(const char * _Format, ...)
{
    char buffer[256];
    char *buf_ptr = buffer;
    const char *fmt_ptr = _Format;
    va_list args;
    va_start(args, _Format);

    while (*fmt_ptr) {
        // Check if the buffer is full and flush it before processing the next format
        if (buf_ptr - buffer >= sizeof(buffer) - 20) {
            fwrite(buffer, 1, buf_ptr - buffer, g_listfile);
            buf_ptr = buffer;  // Reset buffer pointer after flushing
        }

        if (*fmt_ptr == '%') {
            fmt_ptr++;  // Skip the '%'
            if (*fmt_ptr == '0') {
                fmt_ptr++;  // Handle zero-padding
                if (fmt_ptr[0] >= '1' && fmt_ptr[0] <= '9' && *(fmt_ptr + 1) == 'd') {
                    int num = va_arg(args, int);
                    char fmt[5] = "%04d";
                    fmt[2] = fmt_ptr[0];
                    buf_ptr += sprintf(buf_ptr, fmt, num);
                    fmt_ptr += 2;  // Skip '2d'
                } else if (fmt_ptr[0] >= '1' && fmt_ptr[0] <= '9' && *(fmt_ptr + 1) == 'X') {
                    int num = va_arg(args, int);
                    char fmt[5] = "%04X";
                    fmt[2] = fmt_ptr[0];
                    buf_ptr += sprintf(buf_ptr, fmt, num);
                    fmt_ptr += 2;  // Skip '4X'
                } else {
                    // Handle unknown format specifier
                    *buf_ptr++ = '%';
                    *buf_ptr++ = '0';  // Restore '%0'
                    *buf_ptr++ = *fmt_ptr;
                }
            } else if (*fmt_ptr == 'd') {
                int num = va_arg(args, int);
                buf_ptr += sprintf(buf_ptr, "%d", num);
                fmt_ptr++;  // Skip 'd'
            } else if (fmt_ptr[0] >= '1' && fmt_ptr[0] <= '9' && fmt_ptr[1] == 'd') {
                int num = va_arg(args, int);
                char fmt[4] = "%3d";
                fmt[1] = fmt_ptr[0];
                buf_ptr += sprintf(buf_ptr, fmt, num);
                fmt_ptr+=2;  // Skip '3d'
            } else if (*fmt_ptr == 's') {
                // Handle %s -> string
                const char *str = va_arg(args, const char *);

                // Flush the buffer before writing the string directly
                if (buf_ptr > buffer) {
                    fwrite(buffer, 1, buf_ptr - buffer, g_listfile);
                    buf_ptr = buffer;  // Reset buffer pointer
                }

                // Write the string directly to the file
                fwrite(str, 1, strlen(str), g_listfile);

                fmt_ptr++;  // Skip 's'
            } else {
                // Unsupported format specifier, treat as literal
                *buf_ptr++ = '%';
                *buf_ptr++ = *fmt_ptr++;
            }
        } else {
            // Non-format characters, copy directly to the buffer
            *buf_ptr++ = *fmt_ptr++;
        }
    }
    va_end(args);

    // Check if there's any leftover data in the buffer and flush it
    if (buf_ptr > buffer) {
        fwrite(buffer, 1, buf_ptr - buffer, g_listfile);
    }

    return 0;
}
#endif

void log_info(const char *info, int code)
{
    ++Total_Info;
    if (listprintf("## INFO %02d: %s , %04X\n", Total_Info, info, code) == -1) {
        printf("Can not write note information to list file\n");
        cleanup();
        exit(40);
    }
}

void log_warning(const char *warn, int code)
{
    ++Total_Warning;
    printf("## LINE=%04d, WARNING : %s , %04X \n", g_readedlinecount, warn, code);
    if (listprintf("L=%04d, .............. : %s\n", g_readedlinecount, g_fileline) == -1 ||
        listprintf("## WARNING %02d: %s , %04X\n", Total_Warning, warn, code) == -1) {
        printf("Can not write warning information to list file\n");
        cleanup();
        exit(41);
    }
}

void log_error(const char *error, int code)
{
    ++Total_Error;
    printf("## LINE=%04d, ERROR   : %s , %04X \n", g_readedlinecount, error, code);
    if (listprintf("L=%04d, .............. : %s\n", g_readedlinecount, g_fileline) == -1 ||
        listprintf("## ERROR   %02d: %s , %04X\n", Total_Error, error, code) == -1) {
        printf("Can not write error information to list file\n");
        cleanup();
        exit(42);
    }
}
