
# Compare tiny and hello's sizes

tiny: tiny.s
	 nasm -f bin -o $@ $<
	 chmod 755 $@
	 ls -l $@
	 sh -c './$@ ; echo "Code was $$?"'

GCCFLAGS:= -fno-ident -Os -s -static
STRIPFLAGS:= -s --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.GNU-stack --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag
hello.s: hello.c
	gcc $(GCCFLAGS) -S $<

hello: hello.s
	gcc $(GCCFLAGS) -o $@ $<
	ls -l $@
	strip $(STRIPFLAGS) $@
	ls -l $@
	objdump -x $@

clean:
	$(RM) hello hello.s a.out

.PHONY: clean
