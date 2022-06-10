#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "unistd.h"
#include "ctype.h"
#include "elf.h"
#include "errno.h"
#include "fcntl.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/stat.h"
#include "sys/mman.h"
#include "sys/ioctl.h"

#define MAX_FILE_NAME_LENGTH 1024
#define COL_WIDTH 30
#define INDENT_SIZE 4

struct elf_file {
    Elf32_Ehdr *elf_header;
    Elf32_Shdr *sec_header;
    Elf32_Phdr *prog_header;
} ef = {
        0, 0, 0
};


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

void print_menu();

void parse_input();

void flush();

void remove_ending_newline();

void reset_current_fd();

void print_column_data_int(int offset, int data_width, int data);
void print_column_data_int_nl(int offset, int data_width, int data);

void print_column_data_str(int offset, int data_width, char *data);
void print_section_titles();


//void print_padded_column_data();

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
//    scanf("%s", elf_file_name);
    elf_file_name = "abc";
    if (s.current_fd != -1) {
        if (close(s.current_fd) == -1) {
            perror("could not close current file descriptor");
            reset_current_fd();
            exit(errno);
        }
    }
//    printf("read file %s\n", elf_file_name);
    s.current_fd = open(elf_file_name, O_RDONLY);
//    printf("opened %s\n", elf_file_name);
    if (s.current_fd == -1) {
        perror("could not open elf file");
        reset_current_fd();
        exit(errno);
    }
    if (fstat(s.current_fd, &s.fd_stat) != 0) {
        perror("stat failed");
        reset_current_fd();
        exit(errno);
    }
//    printf("read stats %s\n", elf_file_name);

    if ((s.map_start = mmap(0, s.fd_stat.st_size, PROT_READ, MAP_SHARED, s.current_fd, 0)) == MAP_FAILED) {
        perror("mmap failed");
        reset_current_fd();
        exit(errno);
    }
//    write(STDOUT_FILENO, p, 100);
//    printf("mapped ehdr\n");
    s.elf_headers->elf_header = (Elf32_Ehdr *) s.map_start;
    s.elf_headers->sec_header = (Elf32_Shdr * )(s.map_start + s.elf_headers->elf_header->e_shoff);
    s.elf_headers->prog_header = (Elf32_Phdr * )(s.map_start + s.elf_headers->elf_header->e_phoff);
//    printf("mapped shdr\n");
    print_column_data_str(0, COL_WIDTH, "Elf Header");
//    printf("ELF Header:\n");
    printf("  %s:%3x%3x%3x\n", "Magic", s.elf_headers->elf_header->e_ident[EI_MAG0],
           s.elf_headers->elf_header->e_ident[EI_MAG1],
           s.elf_headers->elf_header->e_ident[EI_MAG2]);
    // TODO: print data encoding scheme
    printf("Entry point address: %#10x\n", s.elf_headers->elf_header->e_entry);
    print_column_data_str(1 * INDENT_SIZE, COL_WIDTH, "Start of section headers:");
    print_column_data_int_nl(0, COL_WIDTH, s.elf_headers->elf_header->e_shoff);
    print_column_data_str(1 * INDENT_SIZE, COL_WIDTH, "Number of section headers:");
    print_column_data_int_nl(0, COL_WIDTH, s.elf_headers->elf_header->e_shnum);
    print_column_data_str(1 * INDENT_SIZE, COL_WIDTH, "Size of section headers:");
    print_column_data_int_nl(0, COL_WIDTH, s.elf_headers->elf_header->e_shentsize);
    print_column_data_str(1 * INDENT_SIZE, COL_WIDTH, "Start of program headers:");
    print_column_data_int_nl(0, COL_WIDTH, s.elf_headers->elf_header->e_phoff);
    print_column_data_str(1 * INDENT_SIZE, COL_WIDTH, "Number of program headers:");
    print_column_data_int_nl(0, COL_WIDTH, s.elf_headers->elf_header->e_phnum);
    print_column_data_str(1 * INDENT_SIZE, COL_WIDTH, "Size of program headers:");
    print_column_data_int_nl(0, COL_WIDTH, s.elf_headers->elf_header->e_phentsize);
    printf("%d\n", s.elf_headers->sec_header->sh_entsize);
}

void reset_current_fd() {
    munmap(s.map_start, s.fd_stat.st_size);
    close(s.current_fd);
    s.current_fd = -1;
}

void remove_ending_newline(char *str) {
    int size = 0;
    while (str[size++]) {}
    str[size - 2] = '\0';
}

void print_section_names() {
    if (s.current_fd == -1) {
        printf("There is no elf file currently opened");
        return;
    }
    int INDEX_WIDTH = 5;
    int NAME_WIDTH = 18;
    int TYPE_WIDTH = 16;
    int ADDR_WIDTH = 10;
    int OFF_WIDTH = 7;
    int SIZE_WIDTH = 6;
    int FLAGS_WIDTH = 2;
    int offset = INDENT_SIZE;

    Elf32_Shdr *curr = s.elf_headers->sec_header;

    printf("Section Headers:\n");
    print_section_titles();
    printf("[Nr]")
    for (int i = 0; i < s.elf_headers->elf_header->e_shnum; ++i) {
        while (offset--)
            printf(" ");
        printf("[%*d]",2, i);

//        print_column_data_st4r(1 * INDENT_SIZE, 1, "[");
//        print_column_data_int(0, 2, i);
//        print_column_data_str(0, 2, "]");
//        print_column_data_str(0, NAME_WIDTH, s.elf_headers->sec_header->sh_name);
        printf("%*d ", TYPE_WIDTH, curr->sh_type);
        printf("%0*x ", ADDR_WIDTH, curr->sh_addr);
        printf("%0*x ", OFF_WIDTH, curr->sh_offset);
        printf("%0*x ", SIZE_WIDTH, curr->sh_size);
        printf("%0*x ", FLAGS_WIDTH, curr->sh_entsize);
        printf("%*x ", FLAGS_WIDTH, curr->sh_flags);
        printf("%*x ", FLAGS_WIDTH, curr->sh_link);
        printf("%*d ", FLAGS_WIDTH, curr->sh_info);
        printf("%*x\n", FLAGS_WIDTH, curr->sh_addralign);
//        print_column_data_int(0, TYPE_WIDTH, curr->sh_type);
//        print_column_data_int(0, ADDR_WIDTH, curr->sh_addr);
//        print_column_data_int(0, OFF_WIDTH, curr->sh_offset);
//        print_column_data_int(0, SIZE_WIDTH, curr->sh_size);
//        print_column_data_int(0, FLAGS_WIDTH, curr->sh_entsize);
//        print_column_data_int(0, FLAGS_WIDTH, curr->sh_flags);
//        print_column_data_int(0, FLAGS_WIDTH, curr->sh_link);
//        print_column_data_int(0, FLAGS_WIDTH, curr->sh_info);
//        print_column_data_int_nl(0, FLAGS_WIDTH, curr->sh_addralign);

        offset = INDENT_SIZE;
        curr++;
    }
}

void print_section_titles(){
    int offest = INDENT_SIZE;
    while (offest--)
        printf(" ");
    printf("[Nr]");
    printf("%*s", )
}

void print_symbols() {
    printf("not implemented yed");
}

void quit() {
    exit(0);
}

void print_menu(struct fun_desc menu[], size_t menu_size) {
    int i;
    for (i = 0; i < menu_size; i++) {
        printf("%d-%s\n", i, menu[i].name);
    }
    printf("Choose option: ");
}

void parse_input(int input, size_t menu_size) {
//    if (!isdigit(input)) {
//        printf("ASCII - %d which corresponds to char '%c' is Not a valid input\n", input + '0', input + '0');
//        exit(1);
//    }
//    *input = *input - '0';
    if (input < 0 || input > menu_size) {
        printf("Not within bounds\n");
        exit(1);
    }
    printf("Within bounds\n");
}

void flush() {
    int c;
    while ((c = fgetc(stdin)) != '\n' && c != EOF) {
        printf("flushing %c\n", c);
    }
}

void print_column_data_int_nl(int offset, int data_width, int data) {
    while (offset--)
        printf(" ");
    printf("%-*d\n", data_width, data);
}

void print_column_data_int_dec(int offset, int data_width, int data) {
    while (offset--)
        printf(" ");
    printf("%-*d", data_width, data);
}

void print_column_data_str(int offset, int data_width, char *data) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
//    size_t data_length = strlen(data);
    while (offset--)
        printf(" ");
    printf("%-*s", data_width, data);

}

int main(int argc, char **argv) {
    int input;
    struct fun_desc menu[] = {
            {"Toggle Debug Mode",   toggle_debug_mode},
            {"Examine ELF File",    examine_elf_file},
            {"Print Section Names", print_section_names},
            {"Print Symbols",       print_symbols},
            {"Quit",                quit},
    };
    size_t menu_length = sizeof(menu) / sizeof(menu[0]);
    do {
        print_menu(menu, menu_length);
        scanf("%d", &input);
        flush();
        parse_input(input, menu_length);
        menu[input].fun();
/*		carray = map(carray, base_len, (*menu)[input].fun);*/
    } while (1);
}