# or release
MODE       ?= debug

ifeq "$(MODE)" "debug"
	CFLAGS += -g
	CFLAGS += -O0
else
	CFLAGS += -DNDEBUG
	CFLAGS += -Ofast
endif

CFLAGS     += -std=c89
CFLAGS     += -Wstrict-prototypes
CFLAGS     += -Wall
CFLAGS     += -Wextra
CFLAGS     += -pedantic
CFLAGS     += -Werror

BUILD_DIR  := $(shell mkdir -p "build-$(MODE)" ; echo build-$(MODE) ; )

PRETTIFY   := $(BUILD_DIR)/json-prettify
LIBRARY    := $(BUILD_DIR)/libretrojson.a

CFILES     := $(shell find src -name '*.c')
ifneq "$(MODE)" "debug"
	CFILES := $(filter-out dbg_%, $(CFILES))
endif
OFILES     := $(patsubst %.c, $(BUILD_DIR)/%.o, $(CFILES))

.PHONY: all
all: $(PRETTIFY) $(LIBRARY)

$(PRETTIFY): main.c $(LIBRARY) 
	$(CC) $(CFLAGS) $(filter %.c %.a, $^) -o $@ 
	$(CC) $(CFLAGS) $< -MT $@ -MM -MF $(BUILD_DIR)/main.dep

$(LIBRARY): $(OFILES)
	$(AR) -rcs $@ $^

$(patsubst %.c, $(BUILD_DIR)/%.o, $(CFILES)): $(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	$(CC) $(CFLAGS) -c $< -MT $@ -MM -MF $(BUILD_DIR)/$*.dep

-include $(shell find $(BUILD_DIR) -name '*.dep')

.PHONY: clean
clean: clean-debug clean-release

.PHONY: clean-debug
clean-debug:
	rm -rf build-debug

.PHONY: clean-release
clean-release:
	rm -rf build-release
