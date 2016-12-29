REVERSER=reverser

C=gcc
CFLAGS= -Wall

OBJDIR=./obj
SRCDIR=./src
BINDIR=./bin



SRCS=$(shell find $(SRCDIR) -maxdepth 1 -type f -name "*.c")
OBJS=$(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))


all: $(REVERSER)

$(REVERSER): $(OBJS)
	$(C) $(CFLAGS) $^ -o $(BINDIR)/$(REVERSER)

$(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(C) $(CFLAGS) -c $< -o $@


.PHONY: clean

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(BINDIR)/*
