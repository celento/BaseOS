; boot.asm - Simple 16-bit bootloader
[org 0x7c00] ; BIOS loads us at this address
bits 16      ; We start in 16-bit real mode

KERNEL_LOAD_ADDR equ 0x8000  ; Load kernel right after bootloader
KERNEL_SECTOR    equ 2       ; Kernel starts at the second sector (LBA)
KERNEL_SECTORS   equ 20      ; Number of sectors to load for the kernel (adjust if kernel grows)

start:
    ; --- Setup Data Segment and Stack ---
    cli          ; Disable interrupts
    xor ax, ax   ; Setup DS to 0 (since ORG 0x7c00 handles the offset)
    mov ds, ax
    mov es, ax
    ; Setup stack below our code
    mov ss, ax
    mov sp, 0x7C00 ; Stack grows downwards from 0x7C00
    sti          ; Enable interrupts

    mov [boot_drive], dl ; Save boot drive number

    ; --- Load Kernel from Floppy ---
    mov si, msgLoading
    call print_string

    ; Reset disk system
    mov ah, 0
    mov dl, [boot_drive]
    int 0x13

    ; Check LBA Extensions
    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, [boot_drive]
    int 0x13
    jc use_chs
    
    ; Use LBA Read
    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, dap
    int 0x13
    jc disk_error
    
    jmp kernel_loaded

use_chs:
    ; Fallback to CHS
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

    ; --- Switch to Protected Mode ---
    cli               ; Disable interrupts during switch
    lgdt [gdt_descriptor] ; Load GDT Register
    mov eax, cr0      ; Get current Control Register 0
    or eax, 0x1       ; Set PE (Protection Enable) bit
    mov cr0, eax      ; Write back to CR0
    
    jmp CODE_SEG:protected_mode_start ; Far jump to flush CPU pipeline and load CS

; --- Data ---
msgLoading db 'Loading Kernel...', 0
msgKernelLoaded db ' Kernel Loaded.', 0Dh, 0Ah, 0 ; Newline
msgDiskError db 'Disk read error!', 0Dh, 0Ah, 0
boot_drive db 0
dap:
    db 0x10 ; Size
    db 0    ; Reserved
    dw KERNEL_SECTORS ; Count
    dw KERNEL_LOAD_ADDR ; Offset
    dw 0      ; Segment
    dq KERNEL_SECTOR - 1 ; LBA (Sector 2 is LBA 1)

bits 32 ; Now we are in 32-bit Protected Mode
protected_mode_start:
    ; --- Setup 32-bit Segments ---
    mov ax, DATA_SEG  ; Load data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax        ; Setup stack segment
    mov esp, 0x90000  ; Stack pointer (arbitrary high memory location)

    ; --- Jump to Kernel ---
    jmp KERNEL_LOAD_ADDR ; Jump to where the kernel was loaded

; --- Error Handling ---
disk_error:
    mov dx, 0x3f8
    mov al, 'E'
    out dx, al
    mov si, msgDiskError
    call print_string
    jmp halt

halt:
    hlt ; Halt the CPU
    jmp halt

; --- BIOS Print String Function (Real Mode) ---
print_string:      ; Expects SI to point to the null-terminated string
.loop:
    lodsb           ; Load byte [SI] into AL, increment SI
    or al, al       ; Check if AL is zero (end of string)
    jz .done
    mov ah, 0x0E    ; BIOS teletype output function
    mov bh, 0       ; Page number 0
    int 0x10        ; Call BIOS video service
    jmp .loop
.done:
    ret

; --- Global Descriptor Table (GDT) ---
gdt_start:
gdt_null: ; Null descriptor
    dd 0x0
    dd 0x0

gdt_code: ; Code segment descriptor (Base=0, Limit=4GB, Ring 0, Executable, Read/Write)
    ; Limit (bits 0-15), Base (bits 0-15)
    dw 0xFFFF
    dw 0x0000
    ; Base (bits 16-23)
    db 0x00
    ; Access byte: P=1, DPL=00, S=1, Type=1010 (Execute/Read)
    db 0x9A
    ; Flags (G=1, D=1), Limit (bits 16-19)
    db 0xCF ; Granularity=4KB, 32-bit default operand size
    ; Base (bits 24-31)
    db 0x00

gdt_data: ; Data segment descriptor (Base=0, Limit=4GB, Ring 0, Read/Write)
    ; Limit (bits 0-15), Base (bits 0-15)
    dw 0xFFFF
    dw 0x0000
    ; Base (bits 16-23)
    db 0x00
    ; Access byte: P=1, DPL=00, S=1, Type=0010 (Read/Write)
    db 0x92
    ; Flags (G=1, D=1), Limit (bits 16-19)
    db 0xCF ; Granularity=4KB, 32-bit default operand size
    ; Base (bits 24-31)
    db 0x00
gdt_end:

; GDT Descriptor
gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; GDT Limit (size)
    dd gdt_start              ; GDT Base address

; --- Selectors ---
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; --- Bootloader Padding and Signature ---
times 510 - ($ - $$) db 0 ; Pad remainder of boot sector with 0s
dw 0xAA55                ; Boot signature