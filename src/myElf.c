#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "ctype.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "sys/mman.h"
#include "errno.h"
#include "sys/stat.h"
#include "elf.h"

#define MAX_FILE_NAME_LENGTH 1024

int DEBUG = 0;
int current_fd = -1;

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

void toggle_debug_mode() {
    if (DEBUG) {
        printf("Debug flag is now off\n");
        DEBUG = 0;
    } else {
        DEBUG = 1;
        printf("Debug flag is now on\n");
    }
}

void examine_elf_file() {
    Elf32_Ehdr * header;
    void * map_start;
    printf("elf file name: ");
    char *elf_file_name = malloc(MAX_FILE_NAME_LENGTH * sizeof(char));
    scanf("%s", elf_file_name);
    if (current_fd != -1) {
        close(current_fd);
    }
    current_fd = open(elf_file_name, O_RDONLY);
    if (current_fd == -1) {
        perror("could not open elf file");
        exit(errno);
    }
    struct stat fd_stat;

    if( fstat(current_fd, &fd_stat) != 0 ) {
        perror("stat failed");
        exit(-1);
    }
    if ( (map_start = mmap(0, fd_stat.st_size, PROT_READ , MAP_SHARED, current_fd, 0)) == MAP_FAILED ) {
        perror("mmap failed");
        exit(-4);
    }
//    write(STDOUT_FILENO, p, 100);
    header = (Elf32_Ehdr *) map_start;
    printf("ELF Header:\n");
    printf("  %-8s:%-3X%-3X%-3X%-3X\n" ,"Magic",header->e_ident[EI_MAG0], header->e_ident[EI_MAG1], header->e_ident[EI_MAG2], header->e_ident[EI_MAG3]);
}

void remove_ending_newline(char *str){
    int size = 0;
    while (str[size++]){}
    str[size - 2] = '\0';
}

void print_section_names() {
    printf("not implemented yed");
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