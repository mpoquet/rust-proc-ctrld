CC  = gcc
SYS = -std=gnu11 -Wall -Wextra -O2
PTHREAD_SYS = $(SYS) -lpthread

SRCDIR = src
OBJDIR = obj
BINDIR = bin
FILESDIR = fichiers

SYM_CRYPT = $(BINDIR)/sym_crypt

LOGICIEL = $(BINDIR)/logiciel

CRACK_MASK = $(BINDIR)/crack_mask

BREAK_CODE = $(BINDIR)/break_code

BREAK_CODE_PERFORMANCE = $(BINDIR)/break_code_performance

OBJ_COMMON = $(OBJDIR)/chiffrement.o $(OBJDIR)/utils.o $(OBJDIR)/dh_prime.o $(OBJDIR)/stats.o

OBJ_COMMON2 = $(OBJDIR)/utils.o $(OBJDIR)/list.o $(OBJDIR)/dh_prime.o $(OBJDIR)/stats.o $(OBJDIR)/break_code_c1.o $(OBJDIR)/break_code_c2.o $(OBJDIR)/break_code_c3.o

OBJ_COMMON3 = $(OBJDIR)/utils.o $(OBJDIR)/dh_prime.o $(OBJDIR)/chiffrement.o

OBJ_COMMON4 = $(OBJDIR)/utils.o $(OBJDIR)/list.o $(OBJDIR)/dh_prime.o $(OBJDIR)/stats.o $(OBJDIR)/break_code_c1_multi_threaded.o $(OBJDIR)/break_code_c2_multi_threaded.o $(OBJDIR)/break_code_c3_multi_threaded.o

OBJ_COMMON5 = $(OBJDIR)/utils.o $(OBJDIR)/dh_prime.o

OBJ_CRYPT = $(OBJDIR)/sym_crypt.o

OBJ_CRACK = $(OBJDIR)/break_code_main.o

OBJ_CRACK_MASK = $(OBJDIR)/crack_mask.o

OBJ_CRACK_PERFORMANCE = $(OBJDIR)/break_code_main_multi_threaded.o

OBJ_LOGICIEL = $(OBJDIR)/Logiciel.o


# make crypt
# -------
crypt: dirs $(OBJ_COMMON)
	$(eval export OBJ_CRYPT = $(OBJ_CRYPT))
	make $(SYM_CRYPT)

# make crack
# -------
crack: dirs $(OBJ_COMMON2)
	$(eval export OBJ_CRYPT = $(OBJ_CRACK))
	make $(BREAK_CODE)

# make crack performance
# -------
crack_p: dirs $(OBJ_COMMON4)
	$(eval export OBJ_CRYPT = $(OBJ_CRACK_PERFORMANCE))
	make $(BREAK_CODE_PERFORMANCE)

# make crack_mask
# -------
crack_mask: dirs $(OBJ_COMMON3)
	$(eval export OBJ_CRYPT = $(OBJ_CRACK_MASK))
	make $(CRACK_MASK)


# make logiciel
# -------
logiciel: dirs $(OBJ_COMMON5) crypt crack
	$(eval export OBJ_CRYPT = $(OBJ_LOGICIEL))
	make $(LOGICIEL)

# EXEC Crypt
# -----
$(SYM_CRYPT): $(OBJ_COMMON) $(OBJ_CRYPT)
	$(CC) -o $(SYM_CRYPT) $(OBJ_COMMON) $(OBJ_CRYPT)

# EXEC Crack
# -----
$(BREAK_CODE): $(OBJ_COMMON2) $(OBJ_CRACK)
	$(CC) -o $(BREAK_CODE) $(OBJ_COMMON2) $(OBJ_CRACK)

# EXEC Crack Performance
# -----
$(BREAK_CODE_PERFORMANCE): $(OBJ_COMMON4) $(OBJ_CRACK_PERFORMANCE)
	$(CC) -o $(BREAK_CODE_PERFORMANCE) $(OBJ_COMMON4) $(OBJ_CRACK_PERFORMANCE)

# EXEC Crack_mask
# -----
$(CRACK_MASK): $(OBJ_COMMON3) $(OBJ_CRACK_MASK)
	$(CC) -o $(CRACK_MASK) $(OBJ_COMMON3) $(OBJ_CRACK_MASK)

# EXEC Logiciel
# -----
$(LOGICIEL): $(OBJ_COMMON5) $(OBJ_LOGICIEL)
	$(CC) -o $(LOGICIEL) $(OBJ_COMMON5) $(OBJ_LOGICIEL)

$(OBJDIR)/break_code_c2_multi_threaded.o: $(SRCDIR)/break_code_c2_multi_threaded.c
	$(CC) -o $@ -c $< $(PTHREAD_SYS)

$(OBJDIR)/break_code_c3_multi_threaded.o: $(SRCDIR)/break_code_c3_multi_threaded.c
	$(CC) -o $@ -c $< $(PTHREAD_SYS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -o $@ -c $< $(SYS)


dirs:
	@if [ ! -d "./$(OBJDIR)" ]; then mkdir $(OBJDIR); fi
	@if [ ! -d "./$(BINDIR)" ]; then mkdir $(BINDIR); fi

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(BINDIR)/* $(FILESDIR)/out.*
