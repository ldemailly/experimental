CFLAGS=-Wall -Wextra -pedantic -Werror -fsanitize=address,undefined
HOTRELOAD_CFLAGS=-O2 -Wall -Wextra -Werror -fno-stack-protector -fomit-frame-pointer -fno-asynchronous-unwind-tables

all: ezreload libezreload.so

ezreload: ezreload.c
	gcc ezreload.c $(CFLAGS) -o ezreload -ldl

libezreload.so: lib.c
	gcc -shared -fPIC $(CFLAGS) lib.c -o libezreload.so

newlib:
	@current=$$(grep "const int version" lib.c | sed 's/.*version = \([0-9]*\).*/\1/'); \
	next=$$((current + 1)); \
	sed "s/const int version = [0-9]*;/const int version = $$next;/" lib.c > lib.c.new; \
	mv lib.c.new lib.c && \
	echo "Bumped version from $$current to $$next"
	gcc -shared -fPIC $(CLFAGS) lib.c -o libezreload.so

run: ezreload libezreload.so
	@echo "Running ezreload, do 'make newlib' to bump the version/trigger a reload."
	./ezreload


sig: mandelbrot.c
	gcc $(CFLAGS) mandelbrot.c -o sig

sig.c: sig
	./sig > sig.c

run-sig: sig.c
	gcc -ansi -w sig.c -o sig.gen
	./sig.gen

run-stdin: sig.c
	@echo "Paste in the code to compile:"
	@cat sig.c
	gcc -ansi -w -x c - && ./a.out

alloc-fail-demo: alloc_fail3.c
	gcc $(CFLAGS) $<
	sh -c "ulimit -v 102400 && ./a.out"

happynull: happynull.c
	gcc $(CFLAGS) happynull.c -o happynull
	./happynull


data.bin: hotreload_impl2.c
	$(CC) -c $(HOTRELOAD_CFLAGS) hotreload_impl2.c -o hotreload_impl2.o
	@os=$$(uname -s); \
	if [ "$$os" = "Darwin" ]; then \
		if command -v llvm-objcopy >/dev/null 2>&1; then \
			llvm-objcopy --dump-section __TEXT,__text=$@ hotreload_impl2.o; \
		elif command -v gobjcopy >/dev/null 2>&1; then \
			gobjcopy --dump-section __TEXT,__text=$@ hotreload_impl2.o; \
		else \
			text_off=$$(otool -l hotreload_impl2.o | awk '/sectname __text/{f=1} f && /offset/{print $$2; exit}'); \
			text_size_hex=$$(otool -l hotreload_impl2.o | awk '/sectname __text/{f=1} f && /size/{print $$2; exit}'); \
			text_size=$$((text_size_hex)); \
			dd if=hotreload_impl2.o of=$@ bs=1 skip=$$text_off count=$$text_size status=none; \
		fi; \
	else \
		if command -v objcopy >/dev/null 2>&1; then \
			objcopy -O binary --only-section=.text hotreload_impl2.o $@; \
		elif command -v llvm-objcopy >/dev/null 2>&1; then \
			llvm-objcopy -O binary --only-section=.text hotreload_impl2.o $@; \
		else \
			echo "No objcopy or llvm-objcopy found; cannot build $@" >&2; \
			exit 1; \
		fi; \
	fi
	rm -f hotreload_impl2.o

.PHONY: clean all newlib run alloc-fail-demo

clean:
	rm -f ezreload libezreload.so a.out sig.gen sig.c sig happynull hotreload_impl2.o data.bin
