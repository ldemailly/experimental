%ifidn __OUTPUT_FORMAT__, macho64
    build_version macos, 14, 0
    extern _printf
    %define print_fn _printf
    global _main
    %define main_lbl _main
    %define extra_msg "Mach-O 64"
%else
    extern printf
    %define print_fn printf
    global main
    %define main_lbl main
    %define extra_msg "ELF64"
%endif

LF equ 10

section .text
main_lbl: 
    sub     rsp, 8              ; Align stack to 16 bytes before calling function
    lea     rdi, [rel msg]      ; Load address of the message (RIP-relative)
    call    print_fn             ; Call standard C library function
    add     rsp, 8              ; Restore stack pointer
    mov     rax, 0              ; Return 0
    ret

section .data
msg:    db "Hello, x86_64 World from Apple Silicon! (", extra_msg, ")", LF, 0
