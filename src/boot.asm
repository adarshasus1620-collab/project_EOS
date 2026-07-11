[BITS 16]
[ORG 0x7C00]

start:
    mov si, msg
    call print_string
    jmp $

print_string:
    lodsb
    cmp al, 0
    je return
    mov ah, 0x0E
    int 0x10
    jmp print_string
return:
    ret

msg: db "Hello EOS", 0

times 510-($-$$) db 0
dw 0xAA55
