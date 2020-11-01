# or release
MODE       ?= debug

ifeq "$(MODE)" "debug"
	CFLAGS += -g
	CFLAGS += -O0
else ifeq "$(MODE)" "debug-fast"
	CFLAGS += -Ofast
else ifeq "$(MODE)" "release"
	CFLAGS += -DNDEBUG
	CFLAGS += -Ofast
else 
	ERR := $(error $$(MODE) = $(MODE) but should be one of: debug debug-fast release)
endif

CFLAGS     += -std=c11
CFLAGS     += -pedantic
CFLAGS     += -Wall
CFLAGS     += -Wextra
CFLAGS     += -Werror
CFLAGS     += -Wstrict-prototypes

CPPFLAGS   += -I src

BUILD_DIR  := $(shell mkdir -p "build-$(MODE)" ; echo build-$(MODE) ; )

LIBRARY    := $(BUILD_DIR)/libretrojson.a
PRETTIFY   := $(BUILD_DIR)/prettify/a.out
TEST_STRESS     := $(BUILD_DIR)/test-stress/a.out
TEST_UNIT       := $(BUILD_DIR)/test-unit/a.out
TEST_COMPLIANCE := $(BUILD_DIR)/test-compliance/a.out
TEST_APPS  := $(TEST_STRESS) $(TEST_UNIT) $(TEST_COMPLIANCE)
APPS       := $(PRETTIFY) $(TEST_APPS)

.PHONY: all
all: $(LIBRARY) $(PRETTIFY)

# file compilation rule

CFILES := $(shell find * -name '*.c')
$(patsubst %.c, $(BUILD_DIR)/%.o, $(CFILES)): $(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< $(CPPFLAGS) -o $@
	@$(CC) $(CFLAGS) -c $< $(CPPFLAGS) -MT $@ -MM -MF $(BUILD_DIR)/$*.dep

# application compilation rule
# applications are all directories with main.c

.SECONDEXPANSION:
$(APPS): $(BUILD_DIR)/%/a.out: $$(shell find % -name '*.c' | sed -E 's:(.*)\.c:$(BUILD_DIR)/\1.o:g') $(LIBRARY) 
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@ 

# libretrojson.a

CFILES     := $(shell find src -name '*.c')
ifeq "$(MODE)" "release"
	CFILES := $(filter-out dbg_%, $(CFILES))
endif
$(LIBRARY): $(patsubst %.c, $(BUILD_DIR)/%.o, $(CFILES))
	$(AR) -rcs $@ $^

# other targets

.PHONY: tests
tests: $(TEST_APPS)

.PHONY: check
check: tests 
	$(TEST_STRESS)
	$(TEST_UNIT)
	$(TEST_COMPLIANCE) ./test-compliance/JSONTestSuite/*

.PHONY: clean
clean:
	rm -rf build-*

-include $(shell find $(BUILD_DIR) -name '*.dep')

