CFLAGS += -O3 -std=c89 -Wall -Wextra -pedantic -Werror
LDFLAGS := -L.
LDLIBS := -ljson_sgp

a.out: libjson_sgp.a main.o
	$(CC) main.o $(LDFLAGS) -o $@ $(LDLIBS)

libjson_sgp.a: parser.o
	$(AR) -rcs $@ $^

clean:
	rm -f a.out
	rm -f libjson_sgp.a
	rm -f *.o
