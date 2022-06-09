CC			:=	gcc -m32
CC_FLAGS	:=	-Wall -g
ASM			:=	nasm
ASM_FLAGS	:=	-f elf -g
LINK		:=	ld

SRC_DIR		:=	src
OBJ_DIR		:=	obj
LIST_DIR	:=	list
BIN_DIR		:=	bin

all: clean task0 run # task1 # task2

task0:	$(OBJ_DIR)/myElf.o
	$(CC) -o $(BIN_DIR)/myElf.bin $(OBJ_DIR)/myElf.o

task1:	$(OBJ_DIR)/main1.o $(OBJ_DIR)/task1.o
	$(CC) -o $(BIN_DIR)/task1.bin $(OBJ_DIR)/main1.o $(OBJ_DIR)/task1.o

# task2:
# add your makefile code here...
# uncomment task2 in section 'all'

# .c/.s compile rules
$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) -c $(CC_FLAGS) $< -o $@

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.s
	$(ASM) $(ASM_FLAGS) $< -o $@ -l $(subst .o,.lst,$(subst $(OBJ_DIR),$(LIST_DIR),$@))

run:
	./bin/myElf.bin

clean:
	rm -f $(BIN_DIR)/*.bin $(OBJ_DIR)/*.o $(LIST_DIR)/*.lst