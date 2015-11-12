BIN := anyipc anyips
CFLAGS := -Wall

all: $(BIN)

anyipc: anyipc.o mkaddr.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@ -lpthread

anyips: anyips.o mkaddr.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	-rm -rf $(BIN) *.o

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<


