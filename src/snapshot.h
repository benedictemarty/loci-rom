/* snapshot — save-state fichier (extensions/save-state-B0). Voir snapshot.c. */
#ifndef _SNAPSHOT_H
#define _SNAPSHOT_H
#include <stdbool.h>
#define SNAP_PATH "0:/LOCI.SNP"   /* fixé côté firmware ($B0) */
/* Écrit l'état gelé courant (tampon restore.s en XRAM + RAM $2000-$9FFF) dans SNAP_PATH.
 * Exige un gel en cours (mia_restore_buffer_ok). Renvoie 0 si OK, errno sinon. */
int snapshot_save(void);
/* Recharge SNAP_PATH dans le tampon XRAM et la RAM $2000-$9FFF, pose les map_flags ($B0)
 * → la reprise (boot(true)) rejoue l'état. Renvoie 0 si OK, errno sinon. */
int snapshot_load(void);
#endif
