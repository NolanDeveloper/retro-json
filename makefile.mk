CFLAGS += -std=c89 -Wall -Wextra -pedantic -Werror
LDFLAGS := -L.
LDLIBS := -ljson_sgp

a.out: libjson_sgp.a main.o
	$(CC) main.o $(LDFLAGS) -o $@ $(LDLIBS)

libjson_sgp.a: $(filter-out main.o, $(patsubst ../%.c, %.o, $(wildcard ../*.c)))
	$(AR) -rcs $@ $^

$(patsubst ../%.c, %.o, $(wildcard ../*.c)): %.o: ../%.c
	$(CC) $(CFLAGS) -c $< -o $@
	@$(CC) -MM $< > $*.dep

-include $(wildcard *.dep)

clean:
	@mv makefile ..
	@rm -rf *
	@mv ../makefile .
