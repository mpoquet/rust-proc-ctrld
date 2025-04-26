CC  = gcc
SYS = -std=gnu11 -Wall -Wextra -O2 -g
CFLAGS += -I./flatcc/include

FLATCC_INC = -Iflatcc/include
FLATCC_LIB = flatcc/lib/libflatccrt.a

SRCDIR = daemon-features
OBJDIR = obj
BINDIR = bin
DAEMONDIR = .
TESTDIR = test

DAEMON = $(BINDIR)/daemon

TEST = $(BINDIR)/test

OBJ_COMMON = $(OBJDIR)/clone.o $(OBJDIR)/Errors.o $(OBJDIR)/events.o $(OBJDIR)/process_manager.o $(OBJDIR)/healthcheck.o $(OBJDIR)/Network.o 

OBJ_COMMON2 = $(OBJDIR)/clone.o $(OBJDIR)/Network.o 

OBJ_DAEMON = $(DAEMONDIR)/daemon-core.o

OBJ_TEST =  $(TESTDIR)/Regression_test.o



# make daemon
# -------
daemon: dirs $(OBJ_COMMON)
	$(eval export OBJ_CRYPT = $(OBJ_DAEMON))
	make $(DAEMON)

# make test
# -------
test: dirs $(OBJ_COMMON2)
	$(eval export OBJ_CRYPT = $(OBJ_TEST))
	make $(TEST)

# EXEC daemon
# -----
$(DAEMON): $(OBJ_COMMON) $(OBJ_DAEMON)
	$(CC) -o $(DAEMON) $(OBJ_COMMON) $(OBJ_DAEMON) $(FLATCC_LIB)

# EXEC test
# -----
$(TEST): $(OBJ_COMMON2) $(OBJ_TEST)
	$(CC) -o $(TEST) $(OBJ_COMMON2) $(OBJ_TEST) $(FLATCC_LIB)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -o $@ -c $< $(SYS) $(FLATCC_INC)


dirs:
	@if [ ! -d "./$(OBJDIR)" ]; then mkdir $(OBJDIR); fi
	@if [ ! -d "./$(BINDIR)" ]; then mkdir $(BINDIR); fi

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(BINDIR)/* 
