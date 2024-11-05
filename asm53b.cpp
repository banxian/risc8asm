#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>
#include <stdarg.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#define _mkdir(path) mkdir(path, 0755)
#endif
#include <string>
#include <vector>
#include <algorithm>
#include "risc8bins.h"
#ifdef _STL70_
#include <hash_map>
#define hashmap stdext::hash_map
#else
#include <unordered_map>
#define hashmap std::unordered_map
#endif


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
    int address;    // PC address or constant value
    char status;    // undefined/redefined/unused/normal
    char type;      // 0=label, 1=constant
};
typedef hashmap <std::string, label_item_s> symbol_map_t;

void save_symbol(char *line, int addr, char type);
struct label_item_s * find_symbol(const char* symbol);
int pass1_readlines(FILE *file, const char* srcdir, int addr);
int pass2_assemble(FILE *file, const char* srcdir, int addr);
int solve_number(char *str, unsigned short logicoplimit = 0xFF); // will fill parsed number
void cleanup(void);
void log_info(const char *info, int code);
void log_warning(const char *warn, int code);
void log_error(const char *error, int code);
#ifdef LITE_PRINTF
int __attribute__((format(printf, 1, 2))) listprintf(const char * _Format, ...);
#else
#define listprintf(fmt, ...) fprintf(g_listfile, fmt, ##__VA_ARGS__)
#endif


FILE *g_srcfile = NULL;
FILE *g_listfile = NULL;
symbol_map_t g_symbolpool; // need store more than 4096 because equ
int Total_Info = 0;
int Total_Warning = 0;
int Total_Error = 0;
int g_readedlinecount = 0;
int g_nestlevel = 0; // inclusion nest level
char g_indigi = 0; // indicate a number appeared during solve number
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
        if (_stricmp(ext, ".exe") == 0) {
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

bool compare_by_name(const std::pair<std::string, label_item_s>& a, const std::pair<std::string, label_item_s>& b)
{
    return a.first < b.first;
}

bool is_path_delimiter(const char c)
{
#ifdef _WIN32
    return c== '\\' || c == '/';
#else
    return c == '/';
#endif
}

int find_file_ext(const char* filename, size_t len, int& extpos)
{
    int basepos = 0;
    for (int i = 0; i < len; i++) {
        if (filename[i] == '.') {
            extpos = i + 1;
        } else if (is_path_delimiter(filename[i])) {
            extpos = 0;
            basepos = i + 1;
        }
    }
    return basepos;
}

int main(int argc, char *argv[])
{
    const char *srcfname = NULL, *outbase = NULL;
    int fnlen;
    int extpos, folderlen;
    int pass1pc, pass2pc;
    time_t now;
    char* outfname, *lpext;
    bool fullsizebin = false, exportheader = false;

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
    std::string fullname = srcfname;
    std::string outbaseext = outbase?outbase:std::string();

    extpos = 0;
    folderlen = find_file_ext(srcfname, fnlen, extpos);
    // Append .ASM if no extension found
    if (extpos == 0) {
        extpos = fnlen + 1;
        fullname.append(".ASM");
    }
    fullname.reserve(extpos - 1 + sizeof("_inc.h"));

    printf("Open source file %s\n", fullname.c_str());
    g_srcfile = fopen(fullname.c_str(), "rt");
    if (!g_srcfile) {
        return 11;
    }
    //setvbuf(g_srcfile, g_srcvbuf, _IOFBF, sizeof(g_srcvbuf)); // no need _IOLBF
    if (outbase) {
        int extpos = 0;
        int slashpos = find_file_ext(outbase, outbaseext.size(), extpos);
        if (slashpos) {
            _mkdir(outbaseext.substr(0, slashpos - 1).c_str());
        }
        outfname = &outbaseext[0];
        if (extpos == 0) {
            outbaseext.reserve(outbaseext.size() + sizeof("_inc.h"));
            lpext = &outbaseext[outbaseext.size()];
            *lpext++ = '.';
        } else {
            outbaseext.reserve(extpos - 1 + sizeof("_inc.h"));
            lpext = &outbaseext[extpos];
        }
    } else {
        outfname = &fullname[folderlen]; // make sure output files is placed on build folder rather than source folder
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
    if (struct tm* tm = localtime(&now)) {
        listprintf("Date: %04d.%02d.%02d  Time: %02d:%02d:%02d\n", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    }
    //atexit(&cleanup);// will invoke after exit or normal return
    memset(g_opcodebuf, 0, sizeof(g_opcodebuf));
    listprintf("\nPass1 -------------------------------------------------------------------------\n");
    listprintf("LINE ,  PC ,  CODE/DATA: SOURCE\n");
    std::string srcdir = folderlen?fullname.substr(0, folderlen):std::string(); // get folder path

    pass1pc = pass1_readlines(g_srcfile, srcdir.c_str(), 0); // solve labels, EQU, ORG
    g_readedlinecount = 0;
    g_fileline[0] = 0;
    if (pass1pc != -1 && Total_Error < 100) {
        rewind(g_srcfile);
        listprintf("\nPass2 -------------------------------------------------------------------------\n");
        listprintf("LINE ,  PC ,  CODE/DATA: SOURCE\n");
        pass2pc = pass2_assemble(g_srcfile, srcdir.c_str(), 0); // generate opcodes
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
    listprintf("\nLabel = %3d -------------------------------------------------------------------\n", (int)g_symbolpool.size());
    listprintf("......name....................value.....type....\n");
    std::vector<std::pair<std::string, label_item_s>> sorted_symbols(g_symbolpool.begin(), g_symbolpool.end());
    std::sort(sorted_symbols.begin(), sorted_symbols.end(), compare_by_name);
    for (std::vector<std::pair<std::string, label_item_s>>::iterator it = sorted_symbols.begin(); it != sorted_symbols.end(); it++) {
        const struct label_item_s *sym = &it->second;
        it->first.resize(27, ' ');
        listprintf(".. %s .. %04X .. %s\n",
            it->first.c_str(),
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
    fflush(g_listfile);
    fclose(g_listfile);
    return 0;
}

void cleanup(void)
{
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

/// Save a symbol (label or constant) to the global symbol table
/// @param line Source line containing the symbol (will be wiped)
/// @param addr Symbol value - PC address for labels, constant value for EQU
/// @param type Symbol type: 0 for labels, 1 for constants(EQU)
/// @note Maintains symbol table in sorted order for binary search
/// @note Detects and handles symbol redefinition
void save_symbol(char *line, int addr, char type)
{
    size_t i = 0;
    while (i < 27 && is_valid_label_char(line[i])) {
        ++i;
    }
    std::string name(line, i);
    memset(line, ' ', i); // wipe input line element

    symbol_map_t::iterator it = g_symbolpool.find(name);
    if (it == g_symbolpool.end()) {
        // setup new symbol
        label_item_s item = { addr, (char)(addr == -1 ? ssUndefined : ssUnsed), type };
        g_symbolpool[name] = item;
    } else {
        // alternative found symbol
        if (it->second.address != addr) {
            it->second.address = addr;
            log_warning("label redefined to new value", addr);
        } else {
            log_warning("label redefined", addr);
        }
        it->second.status = ssRedefined;
    }
}

/// Find a symbol in the global symbol table using binary search
/// @param symbol Symbol name to search for
/// @return Pointer to found symbol item, or NULL if not found
/// @note Symbol names in table was padded with spaces
struct label_item_s * find_symbol(const char* symbol)
{
    size_t i = 0;
    while (i < 27 && is_valid_label_char(symbol[i])) {
        ++i;
    }
    std::string name(symbol, i); // strip symbol name

    symbol_map_t::iterator it = g_symbolpool.find(name);
    return it != g_symbolpool.end() ? &it->second : NULL;
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
/// @param logicoplimit limitation for current expression's bit-inverse and shift op
/// @return Evaluated value, or -1 on error
/// @note Handles:
/// @note - Decimal/hex/binary/character constants
/// @note - Labels and symbols
/// @note - Unary operators (-,~)
/// @note - Binary operators (+,-,*,/,%,&,|,^,<<,>>)
/// @note Recursively evaluates complex expressions
/// @note Modified input string by replacing processed parts with spaces
int solve_number(char *str, unsigned short logicoplimit)
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
            save_symbol(head, -1, -1); // prevent next error?
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
    operand = solve_number(opstr, logicoplimit); // nested
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
    case '~' : return logicoplimit & ~operand;
    case '<<': return logicoplimit & (address << operand);
    case '>>': return logicoplimit & (address >> operand);
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
        std::string joinedpath = (srcdir?srcdir:std::string()) + std::string(&g_fileline[incname - g_linebuf], inclen); // take from fileline with raw case 
        incfile = fopen(joinedpath.c_str(), "rt");
        joinedpath.resize(srcdirlen+slashpos); // file path shrined to folder path
        if (incfile) {
            int savedlinecnt = g_readedlinecount;
            g_nestlevel++;
            *addr = nestcall(incfile, joinedpath.c_str(), *addr);
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
        if (head[0] == 'E' && head[1] == 'N' && head[2] == 'D' && head[3] <= ' ') {
            if (g_nestlevel) {
                log_warning("END in include file", g_readedlinecount);
            }
            g_readedlinecount++;
            break;
        } else if (head[0] == 'O' && head[1] == 'R' && head[2] == 'G' && is_tab_or_space(head[3])) {
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
        } else if (*(uint32_t*)head == 'LCNI' && *(uint16_t*)&head[4] == 'DU' && head[6] == 'E' && is_tab_or_space(head[7])) {
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
        if (head[0] == 'E' && head[1] == 'N' && head[2] == 'D' && head[3] <= ' ') {
            if (g_nestlevel) {
                log_warning("END in include file", g_readedlinecount);
            }
            listprintf("L=%04d, P=%04X, .END.. : %s\n", g_readedlinecount, addr, g_fileline);
            g_readedlinecount++;
            break;
        } else if (head[0] == 'O' && head[1] == 'R' && head[2] == 'G' && is_tab_or_space(head[3])) {
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
        } else if (*(uint32_t*)head == 'LCNI' && *(uint16_t*)&head[4] == 'DU' && head[6] == 'E' && is_tab_or_space(head[7])) {
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

#ifndef _USE_GPERF
typedef hashmap < std::string, opcode_info > mnemonic_map_t;

mnemonic_map_t mnemonic_table =
{
    // Control Category Instructions
    {"NOP",      {_OP_NOP,      et_None}},
    {"CLRWDT",   {_OP_CLRWDT,   et_None}}, 
    {"WDT",      {_OP_CLRWDT,   et_None}},  // CLRWDT=WDT
    {"SLEEP",    {_OP_SLEEP,    et_None}},
    {"HALT",     {_OP_SLEEP,    et_None}},  // SLEEP=HALT
    {"SLEEPX",   {_OP_SLEEPX,   et_k2}},
    {"WAITB",    {_OP_WAITB,    et_b3}},
    {"WAITRD",   {_OP_WAITRD,   et_None}},
    {"WAITWR",   {_OP_WAITWR,   et_None}},
    {"WAITSPI",  {_OP_WAITWR,   et_None}},  // WAITWR=WAITSPI
    {"RDCODE",   {_OP_RDCODE,   et_None}},
    {"RCODE",    {_OP_RCODE,    et_k2}},
    {"WRCODE",   {_OP_WRCODE,   et_None}},
    {"EXEC",     {_OP_EXEC,     et_k2}},
    {"PUSHAS",   {_OP_PUSHAS,   et_None}},
    {"PUSH",     {_OP_PUSHAS,   et_None}},  // PUSHAS=PUSH
    {"POPAS",    {_OP_POPAS,    et_None}},
    {"POP",      {_OP_POPAS,    et_None}},  // POPAS=POP
    {"PUSHA2",   {_OP_PUSHA2,   et_None}},
    {"POPA2",    {_OP_POPA2,    et_None}},
    {"RET",      {_OP_RET,      et_None}},
    {"RETURN",   {_OP_RET,      et_None}},  // RET=RETURN
    {"RETZ",     {_OP_RETZ,     et_None}},
    {"RETOK",    {_OP_RETZ,     et_None}},  // RETZ=RETOK
    {"RETIE",    {_OP_RETIE,    et_None}},
    {"RETI",     {_OP_RETIE,    et_None}},  // RETIE=RETI

    // Byte Operation Category Instructions  
    {"CLRA",     {_OP_CLRA,     et_None}},
    {"CLR",      {_OP_CLR,      et_f8}},
    {"CLRF",     {_OP_CLR,      et_f8}},       // CLR=CLRF
    {"MOV",      {_OP_MOV,      et_F9_d1}},
    {"MOVF",     {_OP_MOV,      et_F9_d1}},    // MOV=MOVF
    {"MOVA",     {_OP_MOVA,     et_F9}},
    {"INC",      {_OP_INC,      et_f8_d1}}, 
    {"INCF",     {_OP_INC,      et_f8_d1}},    // INC=INCF 
    {"DEC",      {_OP_DEC,      et_f8_d1}},
    {"DECF",     {_OP_DEC,      et_f8_d1}},    // DEC=DECF
    {"INCSZ",    {_OP_INCSZ,    et_f8_d1}},
    {"INCFSZ",   {_OP_INCSZ,    et_f8_d1}},    // INCSZ=INCFSZ
    {"DECSZ",    {_OP_DECSZ,    et_f8_d1}},
    {"DECFSZ",   {_OP_DECSZ,    et_f8_d1}},    // DECSZ=DECFSZ
    {"SWAP",     {_OP_SWAP,     et_f8_d1}},
    {"SWAPF",    {_OP_SWAP,     et_f8_d1}},    // SWAP=SWAPF
    {"AND",      {_OP_AND,      et_f8_d1}},
    {"ANDF",     {_OP_AND,      et_f8_d1}},    // AND=ANDF
    {"IOR",      {_OP_IOR,      et_f8_d1}},
    {"IORF",     {_OP_IOR,      et_f8_d1}},    // IOR=IORF
    {"XOR",      {_OP_XOR,      et_f8_d1}},
    {"XORF",     {_OP_XOR,      et_f8_d1}},    // XOR=XORF
    {"ADD",      {_OP_ADD,      et_f8_d1}},
    {"ADDF",     {_OP_ADD,      et_f8_d1}},    // ADD=ADDF
    {"SUB",      {_OP_SUB,      et_f8_d1}},
    {"SUBF",     {_OP_SUB,      et_f8_d1}},    // SUB=SUBF
    {"RCL",      {_OP_RCL,      et_f8_d1}},
    {"RCLF",     {_OP_RCL,      et_f8_d1}}, 
    {"RLF",      {_OP_RCL,      et_f8_d1}},    // RCL=RCLF=RLF
    {"RCR",      {_OP_RCR,      et_f8_d1}},
    {"RCRF",     {_OP_RCR,      et_f8_d1}},
    {"RRF",      {_OP_RCR,      et_f8_d1}},    // RCR=RCRF=RRF

    // Constant Operation Category Instructions
    {"RETL",     {_OP_RETL,     et_k8}},
    {"DB",       {_OP_RETL,     et_k8}},       // RETL=DB 
    {"RETLN",    {_OP_RETLN,    et_k8}},
    {"RETER",    {_OP_RETLN,    et_k8}},       // RETLN=RETER
    {"MOVIP",    {_OP_MOVIP,    et_k9}},
    {"MOVIA",    {_OP_MOVIA,    et_k10}},
    {"MOVA1F",   {_OP_MOVA1F,   et_k8}},
    {"MOVA2F",   {_OP_MOVA2F,   et_k8}},
    {"MOVA2P",   {_OP_MOVA2P,   et_k8}},
    {"MOVA1P",   {_OP_MOVA1P,   et_k8}},
    {"MOVL",     {_OP_MOVL,     et_k8}},
    {"ANDL",     {_OP_ANDL,     et_k8}},
    {"IORL",     {_OP_IORL,     et_k8}},
    {"XORL",     {_OP_XORL,     et_k8}},
    {"ADDL",     {_OP_ADDL,     et_k8}},
    {"SUBL",     {_OP_SUBL,     et_k8}},
    {"CMPLN",    {_OP_CMPLN,    et_k8}},
    {"CMPL",     {_OP_CMPL,     et_k8}},

    // Bit Operation Category Instructions
    {"BC",       {_OP_BC,       et_f8_b3}},
    {"BCF",      {_OP_BC,       et_f8_b3}},    // BC=BCF
    {"BS",       {_OP_BS,       et_f8_b3}},
    {"BSF",      {_OP_BS,       et_f8_b3}},    // BS=BSF
    {"BTSC",     {_OP_BTSC,     et_f8_b3}},
    {"BTFSC",    {_OP_BTSC,     et_f8_b3}},    // BTSC=BTFSC
    {"BTSS",     {_OP_BTSS,     et_f8_b3}},
    {"BTFSS",    {_OP_BTSS,     et_f8_b3}},    // BTSS=BTFSS
    {"BCTC",     {_OP_BCTC,     et_k2}},
    {"BP1F",     {_OP_BP1F,     et_a2_b3}},
    {"BP2F",     {_OP_BP2F,     et_a2_b3}},
    {"BG1F",     {_OP_BG1F,     et_a2_b3}},
    {"BG2F",     {_OP_BG2F,     et_a2_b3}},

    // Transfer Category Instructions  
    {"JMP",      {_OP_JMP,      et_k12}},
    {"GOTO",     {_OP_JMP,      et_k12}},      // JMP=GOTO
    {"CALL",     {_OP_CALL,     et_k12}},
    {"JNZ",      {_OP_JNZ,      et_k10_pc}},
    {"JZ",       {_OP_JZ,       et_k10_pc}},
    {"JNC",      {_OP_JNC,      et_k10_pc}},
    {"JC",       {_OP_JC,       et_k10_pc}},
    {"CMPZ",     {_OP_CMPZ,     et_K7_k8}},

    // Special Pseudo Mnemonic
    {"DW",       {0,            et_dw}}
};
#else
extern "C" const struct opcode_info* find_mnemonic(register const char *str, register size_t len);
#endif

int parse_f_F_address(char* regnumber, bool isf8);

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
    unsigned short prefixd = (mnemhead[0] << 8) | mnemhead[1]; // TODO: Endian macro to make double/triple
    char* mnemtail = (char*)skip_printable_chars(mnemhead);
#ifdef _USE_GPERF
    const opcode_info* info = find_mnemonic(mnemhead, mnemtail - mnemhead);
    if (info) {
        opcode = info->opcode;
        instype = info->type;
#else
    mnemonic_map_t::const_iterator it = mnemonic_table.find(std::string(mnemhead, mnemtail));
    if (it != mnemonic_table.end()) {
        opcode = it->second.opcode;
        instype = it->second.type;
#endif
    } else {
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

    char* paras = (char*)skip_prefix_spaces(mnemtail);
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
            int val = solve_number(paras, 0xFFFF); // Allow full 16-bit values on ~,<<,>>
            opcode = val; // no extra error chk
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
