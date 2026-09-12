;
; int mkdir (const char* name, ...);
;
; cc65 déclare mkdir VARIADIQUE (mode optionnel) : les arguments sont sur la pile C,
; Y = nombre d'octets d'arguments. Le mkdir.s commun de cc65 appelle __sysmkdir
; avec cette convention, alors que sysmkdir.c de loci.lib est __fastcall__ (nom
; attendu en A/X) : le nom était du bruit → mkdir échouait toujours (ENODEV).
; Même recette que cc65 libsrc/rp6502/mkdir.s : jeter les variadiques, dépiler
; le nom dans A/X, appeler _sysmkdir qui rend 0/-1 avec errno déjà posé.
;
        .export         _mkdir
        .import         __sysmkdir
        .import         addysp, popax

.proc   _mkdir
        dey
        dey
        jsr     addysp          ; ne garde que le nom (poussé en premier)
        jsr     popax           ; A/X = name
        jmp     __sysmkdir      ; fastcall : 0 / -1, errno posé (mia_call_int_errno)
.endproc
