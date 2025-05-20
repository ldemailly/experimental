all: ezreload libezreload.so

ezreload: ezreload.c
	gcc ezreload.c -Wall -Wextra -pedantic -o ezreload -ldl

libezreload.so: lib.c
	gcc -shared -fPIC -Wall -Wextra -pedantic lib.c -o libezreload.so

newlib:
	@current=$$(grep "const int version" lib.c | sed 's/.*version = \([0-9]*\).*/\1/'); \
	next=$$((current + 1)); \
	sed "s/const int version = [0-9]*;/const int version = $$next;/" lib.c > lib.c.new; \
	mv lib.c.new lib.c
	gcc -shared -fPIC -Wall -Wextra -pedantic lib.c -o libezreload.so

run: ezreload libezreload.so
	@echo "Running ezreload, do `make newlib` to bump the version/trigger a reload."
	./ezreload

.PHONY: clean all newlib run

clean:
	rm -f ezreload libezreload.so
