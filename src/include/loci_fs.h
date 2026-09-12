/*
 * loci_fs — complément POSIX fichiers/répertoires (extensions/fs-posix).
 *
 * Wrappers fastcall au-dessus des opcodes MIA $1E SYNCFS, $1F STAT, $84 CHDIR,
 * $85 GETFREE. STAT et SYNCFS ont le MÊME opcode et la MÊME ABI que RP6502
 * (cc65 rp6502.h : f_stat / syncfs) ; GETFREE et CHDIR suivent la forme des
 * wrappers amont (f_getfree, chdir) avec les numéros du bloc répertoires LOCI.
 * Chemins : "1:/DIR/FICHIER" (clé USB FAT), "0:/FICHIER" (flash interne littlefs),
 * relatifs au répertoire courant après loci_chdir (FAT seulement).
 * Un firmware sans fs-posix rend -1 avec errno = ENOSYS (opcode inconnu).
 */
#ifndef _LOCI_FS_H
#define _LOCI_FS_H

/* Attributs FatFS (fattrib) */
#define LOCI_AM_RDO 0x01
#define LOCI_AM_HID 0x02
#define LOCI_AM_SYS 0x04
#define LOCI_AM_DIR 0x10
#define LOCI_AM_ARC 0x20

/* Identique à f_stat_t de rp6502.h (282 octets, dépilée champ par champ). */
typedef struct {
    unsigned long fsize;
    unsigned fdate;
    unsigned ftime;
    unsigned crdate;        /* 0 sur LOCI (pas de date de création FatFS) */
    unsigned crtime;        /* 0 sur LOCI */
    unsigned char fattrib;  /* LOCI_AM_* (littlefs : LOCI_AM_DIR ou 0) */
    char altname[12 + 1];   /* nom court 8.3 (FAT), vide sur littlefs */
    char fname[255 + 1];
} loci_stat_t;

/* Renseigne *st pour `path`. 0 si OK, -1 sinon (errno). */
int __fastcall__ loci_stat(const char *path, loci_stat_t *st);

/* Change le répertoire courant (et le volume courant si "N:" en tête). 0 / -1. */
int __fastcall__ loci_chdir(const char *path);

/* Espace libre du volume ("0:", "1:"…) en UNITÉS (cluster FAT, bloc de 4 Ko sur
 * littlefs). Renvoie csize = secteurs de 512 octets par unité (> 0), ou -1.
 * Ko libres = *nfree * csize / 2. */
int __fastcall__ loci_getfree(const char *volume, unsigned long *nfree, unsigned long *total);

/* Force l'écriture d'un fichier ouvert (fd), ou de tous (fd = -1). 0 / -1. */
int __fastcall__ loci_syncfs(int fd);

#endif /* _LOCI_FS_H */
