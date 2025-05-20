all: ezreload libezreload.so

ezreload: ezreload.c
	gcc ezreload.c -Wall -Wextra -pedantic -o ezreload -ldl

libezreload.so: lib.c
	gcc -shared -fPIC -Wall -Wextra -pedantic lib.c -o libezreload.so

clean:
	rm -f ezreload libezreload.so
