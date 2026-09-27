; mbr.asm — NASM, вывод картинки 320x200 256 цветов
[BITS 16]
[ORG 0x7C00]

LOAD_SEG    equ 0x1000
LOAD_OFF    equ 0x0000
SECTORS     equ 127        ; 127 * 512 = 65024 байт
DRIVE       equ 0x80

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    ; === Загрузка данных с диска (сектор 2..128) ===
    mov ax, LOAD_SEG
    mov es, ax
    xor bx, bx

    mov ah, 0x02
    mov al, SECTORS
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, DRIVE
    int 0x13
    jc  $

    ; === VGA Mode 13h ===
    mov ax, 0x0013
    int 0x10

    ; === Загрузка палитры (768 байт после 64000 пикселей) ===
    mov dx, 0x3C8
    xor al, al
    out dx, al

    mov dx, 0x3C9
    mov ax, LOAD_SEG
    mov ds, ax
    mov si, 64000
    mov cx, 768
.load_pal:
    lodsb
    out dx, al
    loop .load_pal

    ; === Копирование пикселей в видеопамять ===
    mov ax, 0xA000
    mov es, ax
    xor di, di
    mov ax, LOAD_SEG
    mov ds, ax
    xor si, si
    mov cx, 64000
    rep movsb

    jmp $

    times 510-($-$$) db 0
    dw 0xAA55