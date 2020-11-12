CFLAGS     := -std=c11 -pedantic -Wall -Wextra -Werror -Wstrict-prototypes -fPIC -g -O2
LDLIBS     := -lpthread
CPPFLAGS   := -I includes -I src
LIBRARY_A  := build/libretrojson.a
LIBRARY_SO := build/libretrojson.so
PRETTIFY   := build/prettify
TEST       := build/test

.PHONY: all
all: $(LIBRARY_A) $(LIBRARY_SO) $(PRETTIFY) check

# .c → .o compilation rule

.c.o:
	$(CC) $(CFLAGS) -c $< $(CPPFLAGS) -o $@
	$(CC) $(CFLAGS) -c $< $(CPPFLAGS) -MT $@ -MM -MF $*.dep

# application compilation rule

LINK = $(CC) $(CFLAGS) $(filter %.o,$^) $(LIBRARY_A) -o $@ $(LDLIBS) 

$(PRETTIFY): $(LIBRARY_A) $(patsubst %.c,%.o,$(wildcard prettify/*.c))
	$(LINK)

$(TEST): $(LIBRARY_A) $(patsubst %.c,%.o,$(wildcard test/*.c))
	$(LINK)

# libretrojson.a

$(LIBRARY_A): $(patsubst %.c,%.o,$(wildcard src/*.c))
	$(AR) -rcs $@ $^

$(LIBRARY_SO): $(patsubst %.c,%.o,$(wildcard src/*.c))
	$(CC) $(CFLAGS) $^ -shared -o $@ $(LDLIBS)

# other targets

.PHONY: tests
tests: $(TEST)

.PHONY: check
check: tests 
	./build/test

.PHONY: leakcheck
leakcheck: tests
	valgrind --error-exitcode=1 --leak-check=full ./build/test

.PHONY: clean
clean:
	$(RM) -- $(wildcard src/*.o) 
	$(RM) -- $(wildcard test/*.o)
	$(RM) -- $(wildcard src/*.dep) 
	$(RM) -- $(wildcard test/*.dep)
	$(RM) -- $(wildcard build/*)

-include $(wildcard src/*.dep)
-include $(wildcard test/*.dep)

