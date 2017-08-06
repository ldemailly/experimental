
# Compare tiny and hello's sizes

tiny: tiny.s
	 nasm -f bin -o $@ $<
	 chmod 755 $@
	 ls -l $@
	 sh -c './$@ ; echo "Code was $$?"'

GCCFLAGS:= -fno-ident -Os -s -static -fno-asynchronous-unwind-tables
STRIPFLAGS:= -s --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.GNU-stack --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag
hello.s: hello.c
	gcc $(GCCFLAGS) -S $<

hello: hello.s
	gcc $(GCCFLAGS) -o $@ $<
	ls -l $@
	strip $(STRIPFLAGS) $@
	ls -l $@
	objdump -x $@

hello_syscall.s: hello_syscall.c
	gcc $(GCCFLAGS) -S $<

hello_syscall: hello_syscall.s
	gcc $(GCCFLAGS) -nostartfiles -o $@ $<
	ls -l $@
	strip $(STRIPFLAGS) $@
	ls -l $@
	objdump -x $@

clean:
	$(RM) hello hello.s hello_syscall hello_syscall.s a.out

.PHONY: clean
