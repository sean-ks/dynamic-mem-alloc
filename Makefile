CC := gcc
SRCD := src
BLDD := build
BIND := bin
INCD := include

ALL_SRCF := $(shell find $(SRCD) -type f -name *.c)
ALL_OBJF := $(patsubst $(SRCD)/%,$(BLDD)/%,$(ALL_SRCF:.c=.o))

INC := -I $(INCD)

# Add -D_DEFAULT_SOURCE to define GNU extensions (e.g. sbrk)
CFLAGS := -D_DEFAULT_SOURCE -fcommon -Wall -Werror -Wno-unused-function -MMD \
          -g -O0 -fsanitize=address
COLORF := -DCOLOR
DFLAGS := -g -DDEBUG -DCOLOR
PRINT_STATEMENTS := -DERROR -DSUCCESS -DWARN -DINFO

STD := -std=c99
LIBS := -lm

CFLAGS += $(STD)

EXEC := malloc

.PHONY: clean all setup debug

all: setup $(BIND)/$(EXEC)

debug: CFLAGS += $(DFLAGS) $(PRINT_STATEMENTS) $(COLORF)
debug: all

setup: $(BIND) $(BLDD)
$(BIND):
	mkdir -p $(BIND)
$(BLDD):
	mkdir -p $(BLDD)

$(BIND)/$(EXEC): $(ALL_OBJF)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

$(BLDD)/%.o: $(SRCD)/%.c
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	rm -rf $(BLDD) $(BIND)

.PRECIOUS: $(BLDD)/*.d
-include $(BLDD)/*.d