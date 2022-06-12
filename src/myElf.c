#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "unistd.h"
#include "elf.h"
#include "errno.h"
#include "fcntl.h"
#include "sys/stat.h"
#include "sys/mman.h"

#define MAX_FILE_NAME_LENGTH 1024
#define COL_WIDTH 30
#define INDENT_SIZE 2

struct elf_file {
    Elf32_Ehdr *elf_header;
    Elf32_Shdr *sec_header;
    Elf32_Phdr *prog_header;
    Elf32_Sym *sym_header;
} ef = {
        0, 0, 0
};

struct section_col_sizes {
    int INDEX;
    int NAME;
    int TYPE;
    int ADDR;
    int OFFSET;
    int SIZE;
    int FLAGS;
} sec_col_sizes = {
        5, 18, 10, 8, 6, 6, 2
};

struct symbols_col_sizes {
    int INDEX;
    int VALUE;
    int SECTION_INDEX;
    int SECTION_NAME;
    int SYMBOL_NAME;
} sym_col_sizes = {
        8, 8, 15, 20, 20
};

char *section_types[15] = {"NULL", "PROGBITS", "SYMTAB", "STRTAB", "RELA", "HASH", "DYNAMIC", "NOTE", "NOBITS", "REL",
                           "SHLIB", "DYNSYM", "LOPROC", "LOUSER", "HIUSER"};

char *section_flags = "WAXMMSIL";

struct state {
    int debug_mode;
    int current_fd;
    void *map_start;
    struct elf_file *elf_headers;
    struct stat fd_stat;
} s = {
        0,
        -1,
        NULL,
        &ef,
        {0}
};

struct fun_desc {
    char *name;

    void (*fun)();
};

void toggle_debug_mode();

void examine_elf_file();

void print_section_names();

void print_symbols();

void quit();

void print_menu(struct fun_desc menu[], size_t menu_size);

int parse_input(int input, size_t menu_size);

void flush();

void reset_current_fd();

void print_section_titles();

void print_symbol_titles();

char *resolve_type(int sec_type);

char *resolve_flag(int sec_flag);

char *resolve_section_name(char *section_names, int sh_idx);

int find_symtab_idx();

void toggle_debug_mode() {
    if (s.debug_mode) {
        s.debug_mode = 0;
        printf("Debug flag is now off\n");
    } else {
        s.debug_mode = 1;
        printf("Debug flag is now on\n");
    }
}

void examine_elf_file() {
    printf("elf file name: ");
    char *elf_file_name = malloc(MAX_FILE_NAME_LENGTH * sizeof(char));
    scanf("%s", elf_file_name);
    if (s.current_fd != -1) {
        if (close(s.current_fd) == -1) {
            perror("could not close current file descriptor");
            reset_current_fd();
            exit(errno);
        }
    }
    s.current_fd = open(elf_file_name, O_RDONLY);
    if (s.current_fd == -1) {
        perror("could not open file");
        reset_current_fd();
        exit(errno);
    }
    if (fstat(s.current_fd, &s.fd_stat) != 0) {
        perror("stat failed");
        reset_current_fd();
        exit(errno);
    }

    if ((s.map_start = mmap(0, s.fd_stat.st_size, PROT_READ, MAP_SHARED, s.current_fd, 0)) == MAP_FAILED) {
        perror("mmap failed");
        reset_current_fd();
        exit(errno);
    }
    printf("%s\n", (char *) s.map_start);
    void * elf_bytes =  &(0x7f454c46);

    if (memcmp(s.map_start, elf_bytes, 4) != 0){
        printf("not an elf file\n");
        reset_current_fd();
        exit(1);
    }

    s.elf_headers->elf_header = (Elf32_Ehdr *) s.map_start;
    s.elf_headers->sec_header = (Elf32_Shdr *) (s.map_start + s.elf_headers->elf_header->e_shoff);
    s.elf_headers->prog_header = (Elf32_Phdr *) (s.map_start + s.elf_headers->elf_header->e_phoff);
    int sym_tab_idx = find_symtab_idx();
    s.elf_headers->sym_header = (Elf32_Sym *) (s.map_start + s.elf_headers->sec_header[sym_tab_idx].sh_offset);

    printf("%s", "Elf Header:\n");
    // TODO: print data encoding scheme
    union int_str_union {
        int int_value;
        char *str_value;
    };
    struct elf_print_format {
        char *format;
        int width;
        union int_str_union value;
    };

    struct elf_print_format current_fd_elf[19] = {
            {"%-*s",   5,          {.str_value = "Magic: "}},
            {"%-*x",   2,          {.int_value = s.elf_headers->elf_header->e_ident[EI_MAG0]}},
            {"%-*x",   2,          {.int_value = s.elf_headers->elf_header->e_ident[EI_MAG1]}},
            {"%-*x",   2,          {.int_value = s.elf_headers->elf_header->e_ident[EI_MAG2]}},
            {"%-*x\n", 2,          {.int_value = s.elf_headers->elf_header->e_ident[EI_MAG3]}},
            {"%-*s",    COL_WIDTH, {.str_value = "Entry point address: "}},
            {"%-#*x\n", COL_WIDTH, {.int_value = s.elf_headers->elf_header->e_entry}},
            {"%-*s",    COL_WIDTH, {.str_value = "Start of section headers: "}},
            {"%-*d\n",  COL_WIDTH, {.int_value = s.elf_headers->elf_header->e_shoff}},
            {"%-*s",    COL_WIDTH, {.str_value = "Number of section headers: "}},
            {"%-*d\n",  COL_WIDTH, {.int_value = s.elf_headers->elf_header->e_shnum}},
            {"%-*s",    COL_WIDTH, {.str_value = "Size of section headers: "}},
            {"%-*d\n",  COL_WIDTH, {.int_value = s.elf_headers->elf_header->e_shentsize}},
            {"%-*s",    COL_WIDTH, {.str_value = "Start of program headers: "}},
            {"%-*d\n",  COL_WIDTH, {.int_value = s.elf_headers->elf_header->e_phoff}},
            {"%-*s",    COL_WIDTH, {.str_value = "Number of program headers: "}},
            {"%-*d\n",  COL_WIDTH, {.int_value = s.elf_headers->elf_header->e_phnum}},
            {"%-*s",    COL_WIDTH, {.str_value = "Size of program headers: "}},
            {"%-*d\n",  COL_WIDTH, {.int_value = s.elf_headers->elf_header->e_phentsize}},
    };

    size_t elf_rows_num = sizeof(current_fd_elf) / sizeof(current_fd_elf[0]);

    for (int i = 0; i < elf_rows_num; ++i) {
        printf("%*s", INDENT_SIZE, " ");
        printf(current_fd_elf[i].format, current_fd_elf[i].width, current_fd_elf[i].value);
    }
}

void reset_current_fd() {
    munmap(s.map_start, s.fd_stat.st_size);
    close(s.current_fd);
    s.current_fd = -1;
}

void print_section_names() {
    if (s.current_fd == -1) {
        printf("There is no elf file currently opened\n");
        return;
    }


    void *sections_names = s.map_start + s.elf_headers->sec_header[s.elf_headers->elf_header->e_shstrndx].sh_offset;

    printf("Section Headers:\n");
    print_section_titles();
    for (int i = 1; i < s.elf_headers->elf_header->e_shnum; ++i) {
        printf("%*s", INDENT_SIZE, " ");
        printf("[%*d]", 2, i);
        printf(" %-*s", sec_col_sizes.NAME, (char *) (sections_names + s.elf_headers->sec_header[i].sh_name));
        printf(" %-*s", sec_col_sizes.TYPE, resolve_type(s.elf_headers->sec_header[i].sh_type));
        printf(" %0*x", sec_col_sizes.ADDR, s.elf_headers->sec_header[i].sh_addr);
        printf(" %0*x", sec_col_sizes.OFFSET, s.elf_headers->sec_header[i].sh_offset);
        printf(" %0*x", sec_col_sizes.SIZE, s.elf_headers->sec_header[i].sh_size);
        printf(" %0*x ", sec_col_sizes.FLAGS, s.elf_headers->sec_header[i].sh_entsize);
        printf(" %*s", sec_col_sizes.FLAGS, resolve_flag(s.elf_headers->sec_header[i].sh_flags));
        printf(" %*d", sec_col_sizes.FLAGS, s.elf_headers->sec_header[i].sh_link);
        printf(" %*d", sec_col_sizes.FLAGS, s.elf_headers->sec_header[i].sh_info);
        printf(" %*d\n", sec_col_sizes.FLAGS, s.elf_headers->sec_header[i].sh_addralign);
    }
}

void print_section_titles() {
    printf("%*s", INDENT_SIZE, " ");
    printf("%-*s", sec_col_sizes.INDEX, "[Nr]");
    printf("%-*s", sec_col_sizes.NAME, "Name");
    printf(" %-*s", sec_col_sizes.TYPE, "Type");
    printf(" %-*s", sec_col_sizes.ADDR, "Addr");
    printf(" %-*s", sec_col_sizes.OFFSET, "Off");
    printf(" %-*s", sec_col_sizes.SIZE, "Size");
    printf(" %-*s", sec_col_sizes.FLAGS, "ES");
    printf(" %-*s", sec_col_sizes.FLAGS, "Flg");
    printf(" %-*s", sec_col_sizes.FLAGS, "Lk");
    printf(" %-*s", sec_col_sizes.FLAGS, "Inf");
    printf(" %-*s", sec_col_sizes.FLAGS, "Al");
    printf("\n");
}

char *resolve_type(int sec_type) {
    // for some reason the 'man elf' is not updated
    if (sec_type == SHT_GNU_HASH)
        return "GNU_HASH";
    if (sec_type == SHT_GNU_versym)
        return "VERSYM";
    if (sec_type == SHT_GNU_verneed)
        return "VERNEED";
    if (sec_type == SHT_INIT_ARRAY)
        return "INIT_ARRAY";
    if (sec_type == SHT_FINI_ARRAY)
        return "FINI_ARRAY";
    return section_types[sec_type];
}

char *resolve_flag(int sec_flag) {
    char *flags = malloc(8);
    int i = 1;
    for (int j = 0; j < 8; ++j) {
        if ((sec_flag & i) == i)
            strncat(flags, section_flags + j, 1);
        i = i << 1;
    }
    return flags;
}

void print_symbols() {
    if (s.current_fd == -1) {
        printf("There is no elf file currently opened\n");
        return;
    }

    int sym_tab_idx = find_symtab_idx();
    if (sym_tab_idx == -1){
        perror("could not find symtab section header index");
        exit(errno);
    }

    int symbol_num = (int) (s.elf_headers->sec_header[sym_tab_idx].sh_size / s.elf_headers->sec_header[sym_tab_idx].sh_entsize);
    char *symbol_names = (char *) (s.map_start + s.elf_headers->sec_header[s.elf_headers->sec_header[sym_tab_idx].sh_link].sh_offset);
    char *sections_names = (char *) (s.map_start + s.elf_headers->sec_header[s.elf_headers->elf_header->e_shstrndx].sh_offset);

    printf("`Symbol table '.symtab' contains %d entries:\n", symbol_num);
    print_symbol_titles();
    for (int j = 1; j < symbol_num; j++) {
        printf("%*s", INDENT_SIZE, " ");
        printf("%-*d", sym_col_sizes.INDEX, j);
        printf(" %0*x", sym_col_sizes.VALUE, s.elf_headers->sym_header[j].st_value);
        printf(" %-*d", sym_col_sizes.SECTION_INDEX, s.elf_headers->sym_header[j].st_shndx);
        char *curr_section_name = resolve_section_name(sections_names, s.elf_headers->sym_header[j].st_shndx);
        printf(" %-*s", sym_col_sizes.SECTION_NAME, curr_section_name);
        if (s.elf_headers->sym_header[j].st_info == STT_SECTION) {
            printf(" %-*s\n", sym_col_sizes.SYMBOL_NAME, curr_section_name);
            continue;
        }
        printf(" %-*s\n", sym_col_sizes.SYMBOL_NAME, symbol_names + s.elf_headers->sym_header[j].st_name);
    }
}

void print_symbol_titles() {
    // [Ndx] value section_index section_name symbol_name
    printf("%*s", INDENT_SIZE, " ");
    printf("%-*s", sym_col_sizes.INDEX, "[Ndx]");
    printf(" %-*s", sym_col_sizes.VALUE, "Value");
    printf(" %-*s", sym_col_sizes.SECTION_INDEX, "section_index");
    printf(" %-*s", sym_col_sizes.SECTION_NAME, "section_name");
    printf(" %-*s\n", sym_col_sizes.SYMBOL_NAME, "symbol_name");
}

int find_symtab_idx(){
    for (int i = 0; i < s.elf_headers->elf_header->e_shnum; ++i) {
        if (s.elf_headers->sec_header[i].sh_type == SHT_SYMTAB)
            return i;
    }
    return -1;
}


char *resolve_section_name(char *section_names, int sh_idx) {
    switch (sh_idx) {
        case SHN_ABS:
            return "ABS";
        case SHN_UNDEF:
            return "UND";
        case SHN_LORESERVE:
            return "LRS";
        case SHN_AFTER:
            return "AFT";
        case SHN_HIPROC:
            return "HIP";
        case SHN_COMMON:
            return "CMN";
        case SHN_HIRESERVE:
            return "HRS";
        default:
            return section_names + s.elf_headers->sec_header[sh_idx].sh_name;
    }
}

void quit() {
    exit(0);
}

void print_menu(struct fun_desc menu[], size_t menu_size) {
    int i;
    printf("\n");
    for (i = 0; i < menu_size; i++) {
        printf("%d-%s\n", i, menu[i].name);
    }
    printf("Choose option: ");
}

int parse_input(int input, size_t menu_size) {
    if (input < 0 || input >= menu_size) {
        printf("Not within bounds\n");
        return 1;
    }
    return 0;
}

void flush() {
    int c;
    while ((c = fgetc(stdin)) != '\n' && c != EOF) {
        printf("flushing %c\n", c);
    }
}

int main(int argc, char **argv) {
    int input;
    struct fun_desc menu[] = {
            {"Toggle Debug Mode",   toggle_debug_mode},
            {"Examine ELF File",    examine_elf_file},
            {"Print Section Names", print_section_names},
            {"Print Symbols",       print_symbols},
            {"Quit",                quit}
    };
    size_t menu_length = sizeof(menu) / sizeof(menu[0]);
    do {
        print_menu(menu, menu_length);
        scanf("%d", &input);
        flush();
        if (parse_input(input, menu_length))
            continue;
        menu[input].fun();
    } while (1);
}