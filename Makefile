# or debug
MODE ?= release
CFLAGS := -std=c11 -pedantic -Wall -Wextra\
	-Werror -Wstrict-prototypes -fPIC\
	$(if $(filter debug,$(MODE)),-g -O0,-O2)
LDLIBS := -lpthread
CPPFLAGS := -I includes -I src $(if $(filter debug,$(MODE)),,-DRELEASE)
BUILD_DIR := $(shell mkdir -p "build-$(MODE)" ; echo ./build-$(MODE) ; )
LIBRARY_A := $(BUILD_DIR)/libretrojson.a
LIBRARY_SO := $(BUILD_DIR)/libretrojson.so
PRETTIFY := $(BUILD_DIR)/prettify/a.out
TEST_STRESS := $(BUILD_DIR)/test-stress/a.out
TEST_UNIT := $(BUILD_DIR)/test-unit/a.out
TEST_COMPLIANCE := $(BUILD_DIR)/test-compliance/a.out
TEST_APPS := $(TEST_STRESS) $(TEST_UNIT) $(TEST_COMPLIANCE)
APPS := $(PRETTIFY) $(TEST_APPS)

.PHONY: all
all: $(LIBRARY_A) $(LIBRARY_SO) $(PRETTIFY)

# .c → .o compilation rule

CFILES := $(shell find * -name '*.c')
$(patsubst %.c, $(BUILD_DIR)/%.o, $(CFILES)): $(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< $(CPPFLAGS) -o $@
	@$(CC) $(CFLAGS) -c $< $(CPPFLAGS) -MT $@ -MM -MF $(BUILD_DIR)/$*.dep

# application compilation rule

.SECONDEXPANSION:
$(APPS): $(BUILD_DIR)/%/a.out: \
$$(shell find % -name '*.c' | sed -E 's:(.*)\.c:$(BUILD_DIR)/\1.o:g') $(LIBRARY_A) 
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

# libretrojson.a

CFILES     := $(shell find src -name '*.c')
ifeq "$(MODE)" "release"
	CFILES := $(filter-out dbg_%, $(CFILES))
endif
$(LIBRARY_A): $(patsubst %.c, $(BUILD_DIR)/%.o, $(CFILES))
	@mkdir -p $(dir $@)
	$(AR) -rcs $@ $^

$(LIBRARY_SO): $(patsubst %.c, $(BUILD_DIR)/%.o, $(CFILES))
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -shared -o $@ $(LDLIBS)

# other targets

.PHONY: tests
tests: $(TEST_APPS)

.PHONY: check
check: tests 
	$(TEST_STRESS)
	$(TEST_UNIT)
	for i in $$(ls ./test-compliance/JSONTestSuite) ;\
	do \
		if [ -f ./test-compliance/pretty/$$i ] ;\
		then \
			$(TEST_COMPLIANCE) ./test-compliance/JSONTestSuite/$$i ./test-compliance/pretty/$$i ;\
		else \
			$(TEST_COMPLIANCE) ./test-compliance/JSONTestSuite/$$i ;\
		fi || exit 2 ;\
	done

.PHONY: clean
clean:
	rm -rf build-*

-include $(shell find $(BUILD_DIR) -name '*.dep')

