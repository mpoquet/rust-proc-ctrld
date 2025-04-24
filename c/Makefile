CC  = gcc
SYS = -std=gnu11 -Wall -Wextra -O2

FLATCC_INC = -Iflatcc/include
FLATCC_LIB = flatcc/lib/libflatccrt.a

SRCDIR = daemon-features
OBJDIR = obj
BINDIR = bin
DAEMONDIR = .

DAEMON = $(BINDIR)/daemon

OBJ_COMMON = $(OBJDIR)/clone.o $(OBJDIR)/Errors.o $(OBJDIR)/events.o $(OBJDIR)/process_manager.o $(OBJDIR)/healthcheck.o $(OBJDIR)/Network.o 

OBJ_DAEMON = $(DAEMONDIR)/daemon-core.o


# make daemon
# -------
daemon: dirs $(OBJ_COMMON)
	$(eval export OBJ_CRYPT = $(OBJ_DAEMON))
	make $(DAEMON)

# EXEC daemon
# -----
$(DAEMON): $(OBJ_COMMON) $(OBJ_DAEMON)
	$(CC) -o $(DAEMON) $(OBJ_COMMON) $(OBJ_DAEMON) $(FLATCC_LIB)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -o $@ -c $< $(SYS) $(FLATCC_INC)


dirs:
	@if [ ! -d "./$(OBJDIR)" ]; then mkdir $(OBJDIR); fi
	@if [ ! -d "./$(BINDIR)" ]; then mkdir $(BINDIR); fi

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(BINDIR)/* 
