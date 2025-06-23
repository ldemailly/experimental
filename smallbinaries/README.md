# Small binaries

From searching online I found the following great read/resources:

* http://ptspts.blogspot.com/2013/12/how-to-make-smaller-c-and-c-binaries.html

* http://www.muppetlabs.com/~breadbox/software/tiny/teensy.html

And adapted and enhanced the example to create the tiniest image that does something (prints "Hello World!" and returns code 42)

For the gory details, see [Makefile](Makefile) and compare the resulting `tiny`, 126 bytes (coming from [tiny.s](tiny.s)) vs `hello` (coming from [hello.c](hello.c)) 5520 bytes.

If you have a docker login you can see the [build status](https://cloud.docker.com/swarm/ldemailly/repository/docker/ldemailly/tiniest/builds), otherwise you can pull the image: [`docker pull ldemailly/tiniest`](https://hub.docker.com/r/ldemailly/tiniest/)

Note that if you see this on docker hub, the source of this file with working links is on GitHub: https://github.com/ldemailly/experimental/blob/master/smallbinaries/README.md
