CC = cc
CFLAGS = -std=c11 -Wall -Wextra -g

OBJETS = json.o reconfort_io.o demo.o

demo: $(OBJETS)
	$(CC) $(CFLAGS) -o $@ $(OBJETS)

%.o: %.c json.h reconfort_io.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f demo $(OBJETS)

.PHONY: clean
