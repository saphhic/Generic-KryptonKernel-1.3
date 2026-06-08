;made by saphhic 

[BITS 32]

global rtc_get_seconds
global rtc_get_minutes
global rtc_get_hours

rtc_get_seconds:
    .wait:
        mov al, 0x0A
        out 0x70, al
        in al, 0x71

        test al, 0x80
        jnz .wait

        mov al, 0x00
        out 0x70, al
        in al, 0x71
        ret

rtc_get_minutes:
    .wait:
        mov al, 0x0A
        out 0x70, al
        in al, 0x71

        test al, 0x80
        jnz .wait

        mov al, 0x02
        out 0x70, al
        in al, 0x71
        ret

rtc_get_hours:
    .wait:
        mov al, 0x0A
        out 0x70, al
        in al, 0x71

        test al, 0x80
        jnz .wait

        mov al, 0x04
        out 0x70, al
        in al, 0x71
        ret