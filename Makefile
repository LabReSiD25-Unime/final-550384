# Makefile per progetto C
# Struttura directory:
# ./ (root) - contiene main.c
# ./src/ - contiene i file sorgente
# ./include/ - contiene i file header
# ./obj/ - contiene i file oggetto (creata automaticamente)
# ./bin/ - contiene l'eseguibile (creata automaticamente)

# Configurazione compilatore e flag
CC = gcc
CFLAGS = -Wall -Wextra -g
INCLUDES = -Iinclude 
LIBS = -Llib -lhiredis

# Directory
SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

# Nome dell'eseguibile
TARGET = $(BINDIR)/main

# Trova tutti i file .c in src/
SOURCES = $(wildcard $(SRCDIR)/*.c)
# Crea lista dei file oggetto corrispondenti
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
# Aggiungi main.o per main.c nella root
OBJECTS += $(OBJDIR)/main.o

# Regola principale
all: $(TARGET)

# Regola per creare l'eseguibile
$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) -o $@ $(LIBS)
	@echo "Compilazione completata: $(TARGET)"

# Regola per compilare main.c dalla root
$(OBJDIR)/main.o: main.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Regola per compilare i file .c da src/
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Crea la directory obj se non esiste
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Crea la directory bin se non esiste
$(BINDIR):
	mkdir -p $(BINDIR)

# Pulizia dei file generati
clean:
	rm -rf $(OBJDIR) $(BINDIR)
	@echo "File temporanei rimossi"

# Pulizia solo dei file oggetto
clean-obj:
	rm -rf $(OBJDIR)
	@echo "File oggetto rimossi"

# Ricompilazione completa
rebuild: clean all

# Esecuzione del programma
run: $(TARGET)
	./$(TARGET)

# Debug con gdb
debug: $(TARGET)
	gdb ./$(TARGET)

# Mostra informazioni sul progetto
info:
	@echo "Progetto C"
	@echo "=========="
	@echo "Compilatore: $(CC)"
	@echo "Flag: $(CFLAGS)"
	@echo "Include: $(INCLUDES)"
	@echo "Sorgenti: $(SOURCES) main.c"
	@echo "Oggetti: $(OBJECTS)"
	@echo "Target: $(TARGET)"

# Installa dipendenze automatiche
$(OBJDIR)/%.d: $(SRCDIR)/%.c | $(OBJDIR)
	@$(CC) -MM $(CFLAGS) $(INCLUDES) $< | sed 's|$*.o|$(OBJDIR)/$*.o $(OBJDIR)/$*.d|g' > $@

$(OBJDIR)/main.d: main.c | $(OBJDIR)
	@$(CC) -MM $(CFLAGS) $(INCLUDES) $< | sed 's|main.o|$(OBJDIR)/main.o $(OBJDIR)/main.d|g' > $@

# Include le dipendenze solo se non stiamo facendo clean
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),clean-obj)
-include $(OBJECTS:.o=.d)
endif
endif

# Dichiara target che non corrispondono a file
.PHONY: all clean clean-obj rebuild run debug info