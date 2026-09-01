/*
 * loci_stream — streamer d'assets LOCI (flash/média -> banque 16 Ko -> $C000-$FFFF).
 *
 * Bibliothèque 6502 read-only au-dessus des opcodes MIA :
 *   open ($14), MIA_OP_STREAM_BANK ($A8), MIA_OP_SET_BANK ($A7), close ($15).
 * Permet à un logiciel Oric de dépasser 48 Ko en chargeant des assets/overlays
 * à la demande dans une banque 16 Ko servie en $C000-$FFFF.
 *
 * Réf : extensions/streamer-A8/spec-streamer-assets.md (niveaux 0 et 1).
 * ⚠ SEL 0..3 (banques allouées firmware) ; correspondance figée base = SEL*0x4000.
 */
#ifndef _LOCI_STREAM_H
#define _LOCI_STREAM_H

/* Handle de flux = fd LOCI + banque cible + offset XRAM dérivé. */
typedef struct {
    int           fd;    /* descripteur renvoyé par open() (< 0 = invalide) */
    unsigned char sel;   /* index de banque 16 Ko cible (0..3) */
    unsigned int  xbase; /* offset XRAM de la banque = sel * 0x4000 */
} loci_stream_t;

/* Ouvre un flux d'assets en lecture seule (ex. "0:level3.dat").
 * Renvoie 0 si OK, -1 sinon (errno positionné). */
int __fastcall__ loci_stream_open(loci_stream_t *s, const char *path,
                                  unsigned char sel);

/* Charge `len` octets depuis l'offset fichier `off` (SEEK_SET) vers l'offset
 * `dst` (0..0x3FFF) de la banque. Un seul fastcall $A8 (sans mapping).
 * Renvoie le nombre d'octets réellement chargés, ou -1 (errno). */
int __fastcall__ loci_stream_load(loci_stream_t *s, unsigned long off,
                                  unsigned int dst, unsigned int len);

/* Mappe la banque du flux en $C000-$FFFF (MIA_OP_SET_BANK, EN=1). */
void __fastcall__ loci_stream_show(loci_stream_t *s);

/* Repasse en ROM interne (MIA_OP_SET_BANK, EN=0). */
void __fastcall__ loci_stream_hide(void);

/* Ferme le flux (close du fd). */
void __fastcall__ loci_stream_close(loci_stream_t *s);

#endif /* _LOCI_STREAM_H */
