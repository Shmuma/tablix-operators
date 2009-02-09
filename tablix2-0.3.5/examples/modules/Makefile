CFLAGS = -Wall -O2
INCLUDES = -I../../src -I/usr/include/tablix2

MODULES = hello.so fixed.so row.so

all: $(MODULES)

%.so: %.o
	gcc -shared -Wl,-soname,$@ -o $@ $< -lc
%.o: %.c
	gcc $(CFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	-rm *.o *.so

install:
	cp $(MODULES) /usr/local/lib/tablix2
