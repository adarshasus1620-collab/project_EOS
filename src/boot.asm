[BITS 16]
[ORG 0x7C00]

start:
    mov si, msg
    call print_string

    mov ah, 0x02
    mov al, 5
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov bx, 0x8000
    int 0x13

    jc disk_error

    jmp 0x8000

disk_error:
    mov si, err_msg
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
err_msg: db "Disk Error", 0

times 510-($-$$) db 0
dw 0xAA55
