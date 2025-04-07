# Compilatore e opzioni
CC = gcc
CFLAGS = -Wall -Wextra -I./include

# Cartelle
SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = $(BIN_DIR)
LIB_DIR = lib

# Librerie
LIBS = -lwinmm -lcomctl32 -lgdi32 -lole32 -lshell32 -lgdiplus
BASS_LIB = $(LIB_DIR)/bass.lib

# File sorgenti
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

# Eseguibili
CLI_APP = $(BIN_DIR)/mp3player.exe
GUI_APP = $(BIN_DIR)/mp3player_gui.exe

# File oggetto per i diversi eseguibili
COMMON_OBJ = $(OBJ_DIR)/library.o $(OBJ_DIR)/scanner.o $(OBJ_DIR)/id3parser.o $(OBJ_DIR)/audio.o
# L'applicazione CLI ha bisogno di main.c, gui.c e guimain.c
CLI_OBJ = $(COMMON_OBJ) $(OBJ_DIR)/gui.o $(OBJ_DIR)/main.o $(OBJ_DIR)/guimain.o
# L'applicazione GUI ha bisogno solo di gui.c e guimain.c
GUI_OBJ = $(COMMON_OBJ) $(OBJ_DIR)/gui.o $(OBJ_DIR)/guimain.o

# Target principale
all: $(CLI_APP) $(GUI_APP)

# Creazione dell'eseguibile CLI
$(CLI_APP): $(CLI_OBJ)
	$(CC) $^ -o $@ $(LIBS) $(BASS_LIB)

# Creazione dell'eseguibile GUI (senza finestra console)
$(GUI_APP): $(GUI_OBJ)
	$(CC) $^ -o $@ $(LIBS) $(BASS_LIB) -mwindows

# Compilazione dei file oggetto
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Pulizia
clean:
	rm -f $(OBJ_DIR)/*.o $(CLI_APP) $(GUI_APP)

# Assicura che la directory bin esista
$(shell mkdir -p $(BIN_DIR))

# Esecuzione
run-cli: $(CLI_APP)
	$(CLI_APP)

run-gui: $(GUI_APP)
	$(GUI_APP)

.PHONY: all run-cli run-gui clean 