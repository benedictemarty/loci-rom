;
; snapshot — save-state fichier (extensions/save-state-B0), en assembleur : la ROM
; 16 Ko est pleine, la version C coûtait ~270 o, celle-ci ~110.
;
; S'appuie sur le gel existant : au bouton court, le trap IRQ amène le 6502 dans
; cette ROM dont restore.s a rangé en XRAM [0, $3F0F) : ZP, RAM $0100-$1FFF (dont
; la pile, avec P/PC du programme interrompu), VRAM $A000-$BFFF, mode vidéo, registres
; VIA/CPU et le magic 'L'. Le menu n'utilise que ces zones ($0000-$1FFF, écran) : la
; RAM $2000-$9FFF du programme est INTACTE et se lit directement.
;
; L'E/S fichier (0:/LOCI.SNP, en-tête, XRAM [0, $3F0F), map_flags du gel) est faite
; par le firmware ($B0 A=2/3/4). Ici on ne fait que copier la RAM $2000-$9FFF vers /
; depuis le tampon XRAM SNAP_XRAM par tranches de 16 Ko (port $03A4, auto-incrément
; dans les deux sens) entre deux appels $B0 :
;   BEGIN (A=2, X=0 sauve / 1 charge) ; CHUNK (A=3) x2 ; END (A=4).
; Non capturé (dans les puces, hors RAM) : état interne ULA, timers en cours du VIA
; (les registres, eux, sont rejoués), périphériques LOCI en cours (disque, ACIA).
; La configuration du menu (Microdisc, ROM Atmos/Oric-1, cassette) doit être la même
; qu'au moment du gel : elle n'est pas dans le fichier.
;
; int snapshot_save(void);  -> 0 si OK, -1 (pas de gel) ou retour $B0 (-1, errno posé)
; int snapshot_load(void);  -> 0 si OK, sinon retour $B0
;
.include "loci.inc"
.export _snapshot_save, _snapshot_load
.import _mia_restore_buffer_ok, _mia_call_int
.importzp ptr1

MIA_OP_SNAP   = $B0
SNAP_BEGIN    = 2
SNAP_CHUNK    = 3
SNAP_END      = 4
SNAP_XRAM     = $8000           ; même tampon que filemanager ; boot() re-persiste la cfg
RAM_LO_PAGE   = $20             ; RAM $2000-$9FFF = 2 tranches de $4000
CHUNK_PAGES   = $40

.bss
loading:    .res 1
result:     .res 2

.code

_snapshot_save:
    jsr _mia_restore_buffer_ok
    tax
    bne @ok
    lda #$FF                    ; -1 : pas de programme gelé
    tax
    rts
@ok:
    lda #0
    beq common
_snapshot_load:
    lda #1
common:
    sta loading
    tax                         ; X = 0 sauve / 1 charge
    lda #SNAP_BEGIN
    jsr snap_op
    bne done
    lda #RAM_LO_PAGE
    jsr chunk
    bne done
    lda #RAM_LO_PAGE+CHUNK_PAGES
    jsr chunk
done:
    sta result
    stx result+1
    lda #SNAP_END
    ldx #0
    jsr snap_op
    lda result
    ldx result+1
    rts

; A = sous-fonction, X = argument -> appel $B0 ; retour AX, Z si A = 0
snap_op:
    stx MIA_X
    sta MIA_A
    lda #MIA_OP_SNAP
    jsr _mia_call_int
    cmp #0
    rts

; A = page haute de la tranche RAM ; copie $4000 octets RAM <-> XRAM[SNAP_XRAM]
; autour de l'appel CHUNK (avant en sauvegarde, après en chargement). Retour AX, Z si OK.
chunk:
    sta ptr1+1
    lda #0
    sta ptr1
    lda loading
    beq @save
    jsr op_chunk                ; charge : fichier -> XRAM d'abord
    bne @ret
    jsr set_addr
    ldy #0
@rd:
    lda MIA_RW0
    sta (ptr1),y
    iny
    bne @rd
    inc ptr1+1
    dex
    bne @rd
    lda #0
    tax
@ret:
    rts
@save:
    jsr set_addr
    ldy #0
@wr:
    lda (ptr1),y
    sta MIA_RW0
    iny
    bne @wr
    inc ptr1+1
    dex
    bne @wr
op_chunk:
    lda #SNAP_CHUNK
    ldx #0
    jmp snap_op

set_addr:                       ; XRAM addr0 = SNAP_XRAM, pas 1 ; X = nb de pages
    lda #<SNAP_XRAM
    sta MIA_ADDR0
    lda #>SNAP_XRAM
    sta MIA_ADDR0+1
    lda #1
    sta MIA_STEP0
    ldx #CHUNK_PAGES
    rts
