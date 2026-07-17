%ifidn __OUTPUT_FORMAT__, macho64
    build_version macos, 14, 0
    %define extra_msg "Mach-O 64"
%else
    %define extra_msg "ELF64"
%endif

%pragma macho64 gprefix _

extern printf
global main

section .text
main:
    sub     rsp, 8              ; Align stack to 16 bytes before calling function
    lea     rdi, [rel msg]      ; Load address of the message (RIP-relative)
    call    printf              ; Call standard C library function
    add     rsp, 8              ; Restore stack pointer
    mov     rax, 0              ; Return 0
    ret

section .data
msg:    db `Hello, x86_64 World from Apple Silicon! (`, extra_msg, `)\n`, 0
