/*
 * loci_ramx — expansion RAM paginée façon GeoRAM (opcode $AF, extensions/ram-expansion-AF).
 *
 * Fenêtre de 32 octets en $03C0-$03DF, lue/écrite comme de la RAM ordinaire ; la page
 * visible se choisit par les registres $03E0 (bas) / $03E1 (haut) — effet immédiat,
 * page n = octets [n*32, n*32+32) de la RAM d'expansion. $03E2/$03E3 = nombre de
 * pages (0 = pas d'expansion sur ce firmware : build copy_to_ram), $03E4 = 32.
 * Accès direct (macro/registres) ou par l'opcode $AF (info / set_page) — même effet.
 */
#ifndef _LOCI_RAMX_H
#define _LOCI_RAMX_H

#define LOCI_RAMX_WINDOW ((unsigned char *)0x03C0)   /* 32 octets */
#define LOCI_RAMX_WSIZE  32
#define LOCI_RAMX_PAGE_LO (*(volatile unsigned char *)0x03E0)
#define LOCI_RAMX_PAGE_HI (*(volatile unsigned char *)0x03E1)
#define LOCI_RAMX_NPAGES  (*(volatile unsigned int *)0x03E2)   /* lecture seule */

/* Nombre de pages disponibles (0 = absente) — par l'opcode $AF. */
unsigned int __fastcall__ loci_ramx_pages(void);

/* Sélectionne la page visible dans la fenêtre (modulo NPAGES). Écriture directe des
 * registres : haut puis bas (le bas déclenche avec le haut déjà posé). */
#define loci_ramx_select(n) do { LOCI_RAMX_PAGE_HI = (unsigned char)((n) >> 8); \
                                 LOCI_RAMX_PAGE_LO = (unsigned char)(n); } while (0)

/* Copie len octets (≤ 32) depuis/vers la fenêtre — commodités. */
void __fastcall__ loci_ramx_read(unsigned int page, void *dst, unsigned char len);
void __fastcall__ loci_ramx_write(unsigned int page, const void *src, unsigned char len);

#endif /* _LOCI_RAMX_H */
