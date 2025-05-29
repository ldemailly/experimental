all: ezreload libezreload.so

ezreload: ezreload.c
	gcc ezreload.c -Wall -Wextra -pedantic -o ezreload -ldl

libezreload.so: lib.c
	gcc -shared -fPIC -Wall -Wextra -pedantic lib.c -o libezreload.so

newlib:
	@current=$$(grep "const int version" lib.c | sed 's/.*version = \([0-9]*\).*/\1/'); \
	next=$$((current + 1)); \
	sed "s/const int version = [0-9]*;/const int version = $$next;/" lib.c > lib.c.new; \
	mv lib.c.new lib.c && \
	echo "Bumped version from $$current to $$next"
	gcc -shared -fPIC -Wall -Wextra -pedantic lib.c -o libezreload.so

run: ezreload libezreload.so
	@echo "Running ezreload, do 'make newlib' to bump the version/trigger a reload."
	./ezreload


sig: mandelbrot.c
	gcc mandelbrot.c -Wall -Wextra -pedantic -o sig

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
	gcc -Wall -Wextra -Werror -pedantic $<
	sh -c "ulimit -v 102400 && ./a.out"

.PHONY: clean all newlib run alloc-fail-demo

clean:
	rm -f ezreload libezreload.so a.out sig.gen sig.c sig
