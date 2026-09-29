[BITS 16]
[ORG 0x7C00]

start:
    mov si, msg
    call print_string

    mov [boot_drive], dl

    mov si, dap
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13

    jc disk_error

    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp 0x08:protected_mode_start

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
boot_drive: db 0

; Disk Address Packet (for LBA read)
dap:
    db 0x10       ; size of packet
    db 0x00       ; reserved
    dw 10         ; number of sectors to read
    dw 0x8000     ; offset to load into
    dw 0x0000     ; segment to load into
    dq 1          ; starting LBA sector (sector 1, since sector 0 is bootloader)

gdt_start:

gdt_null:
    dd 0x0
    dd 0x0

gdt_code:
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10011010b
    db 11001111b
    db 0x0

gdt_data:
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

[BITS 32]
protected_mode_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    jmp 0x8000

times 510-($-$$) db 0
dw 0xAA55
