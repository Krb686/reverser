CC=gcc
CFLAGS=-g -Wall -O3 -Iinclude
LDFLAGS=

SRCDIR=src
OBJDIR=obj
BINDIR=bin

# Collect all sources in SRCDIR
SRCS=$(wildcard $(SRCDIR)/*.c)

BIN_SRCS=$(filter-out $(SRCDIR)/test.c, $(SRCS))
# Translate to object names in OBJDIR
BIN_OBJS=$(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(BIN_SRCS))
BIN=$(BINDIR)/reverser

TEST_SRC=$(SRCDIR)/test.c
TEST_OBJ=$(OBJDIR)/test.o
TEST_BIN=$(BINDIR)/test


# Build all binaries
all: $(BIN) $(TEST_BIN)

$(BIN): $(BIN_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(BIN_OBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.c PRINT
	$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

test: $(TEST_BIN)

$(TEST_BIN): $(TEST_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(TEST_OBJ): $(TEST_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

.PHONY: PRINT
PRINT:
	@printf "Making objects\n"


.PHONY: clean

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(BINDIR)/*
