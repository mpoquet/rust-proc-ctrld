CC = gcc
CXX = g++
SYS = -std=gnu11 -Wall -Wextra -O2 -g
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g -I./include
FLATBUFFERS = -I/usr/local/include

SRCDIR = daemon-features
OBJDIR = obj
BINDIR = bin
DAEMONDIR = .
TESTDIR = test

DAEMON = $(BINDIR)/daemon
TEST = $(BINDIR)/test

# Objets C++
CPP_SRCS = $(SRCDIR)/serialize.cpp $(SRCDIR)/serialize_wrapper.cpp
CPP_OBJ = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(CPP_SRCS))

# Objets C
OBJ_COMMON = $(OBJDIR)/clone.o $(OBJDIR)/Errors.o $(OBJDIR)/events.o \
	$(OBJDIR)/process_manager.o $(OBJDIR)/healthcheck.o $(OBJDIR)/Network.o

OBJ_COMMON2 = $(OBJDIR)/clone.o $(OBJDIR)/Network.o 

OBJ_DAEMON = $(DAEMONDIR)/daemon-core.o
OBJ_TEST = $(TESTDIR)/Regression_test.o

# Règles pour les fichiers C++
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(FLATBUFFERS) -c $< -o $@

# Règles explicites pour s'assurer que serialize.cpp est compilé avant serialize_wrapper.cpp
$(OBJDIR)/serialize_wrapper.o: $(SRCDIR)/serialize_wrapper.cpp $(OBJDIR)/serialize.o
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(FLATBUFFERS) -c $< -o $@

$(OBJDIR)/serialize.o: $(SRCDIR)/serialize.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(FLATBUFFERS) -c $< -o $@

# Règles pour les fichiers C
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(SYS) -c $< -o $@

$(OBJ_TEST): $(TESTDIR)/%.o: $(TESTDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(SYS) -c $< -o $@

# EXEC daemon
$(DAEMON): $(OBJ_COMMON) $(CPP_OBJ) $(OBJ_DAEMON)
	@mkdir -p $(BINDIR)
	$(CXX) -o $@ $^ -L/usr/local/lib -lflatbuffers

# EXEC test
$(TEST): $(OBJ_COMMON2) $(OBJDIR)/serialize.o $(OBJDIR)/serialize_wrapper.o $(OBJ_TEST)
	@mkdir -p $(BINDIR)
	$(CXX) -o $@ $(OBJ_COMMON2) $(OBJDIR)/serialize.o $(OBJDIR)/serialize_wrapper.o $(OBJ_TEST) -L/usr/local/lib -lflatbuffers

# make daemon
daemon: dirs $(OBJ_COMMON) $(CPP_OBJ) $(OBJ_DAEMON)
	$(eval export OBJ_CRYPT = $(OBJ_DAEMON))
	$(MAKE) $(DAEMON)

# make test
test: dirs $(OBJ_COMMON2) $(CPP_OBJ) $(OBJ_TEST)
	$(eval export OBJ_CRYPT = $(OBJ_TEST))
	$(MAKE) $(TEST)

dirs:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(BINDIR)/*

.PHONY: clean dirs daemon test