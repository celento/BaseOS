; boot.asm - Simple 16-bit bootloader
[org 0x7c00]
bits 16

KERNEL_LOAD_ADDR equ 0x8000
KERNEL_SECTOR    equ 2
KERNEL_SECTORS   equ 20

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl

    mov si, msgLoading
    call print_string

    ; Reset disk
    mov ah, 0
    mov dl, [boot_drive]
    int 0x13

    ; Check LBA
    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, [boot_drive]
    int 0x13
    jc use_chs
    
    ; LBA Read
    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, dap
    int 0x13
    jc disk_error
    jmp kernel_loaded

use_chs:
    mov ah, 0x02
    mov al, KERNEL_SECTORS
    mov ch, 0
    mov cl, KERNEL_SECTOR
    mov dh, 0
    mov dl, [boot_drive]
    mov bx, KERNEL_LOAD_ADDR
    int 0x13
    jc disk_error

kernel_loaded:
    mov si, msgKernelLoaded
    call print_string

    ; Mode 13h
    mov ah, 0x00
    mov al, 0x13
    int 0x10

    ; Switch to Protected Mode
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:protected_mode_start

; Data
msgLoading db 'Loading...', 0
msgKernelLoaded db ' Done.', 0Dh, 0Ah, 0
msgDiskError db 'Disk Error!', 0Dh, 0Ah, 0
boot_drive db 0

dap:
    db 0x10
    db 0
    dw KERNEL_SECTORS
    dw KERNEL_LOAD_ADDR
    dw 0
    dq KERNEL_SECTOR - 1

bits 32
protected_mode_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    jmp KERNEL_LOAD_ADDR

disk_error:
    mov si, msgDiskError
    call print_string
    jmp $

print_string:
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp .loop
.done:
    ret

; GDT
gdt_start:
gdt_null:
    dd 0x0
    dd 0x0

gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x9A
    db 0xCF
    db 0x00

gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0xCF
    db 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

times 510 - ($ - $$) db 0
dw 0xAA55