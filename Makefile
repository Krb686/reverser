CC=gcc
CFLAGS=-g -Wall -O3 -Iinclude
LDFLAGS=

SRCDIR=src
OBJDIR=obj
BINDIR=bin

# Collect all sources in SRCDIR
SRCS=$(wildcard $(SRCDIR)/*.c)
# Translate to object names in OBJDIR
OBJS=$(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))
BIN=$(BINDIR)/reverser


# Build all binaries
all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(OBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.c PRINT
	$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

.PHONY: PRINT
PRINT:
	@printf "Making objects\n"


.PHONY: clean

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(BINDIR)/*
