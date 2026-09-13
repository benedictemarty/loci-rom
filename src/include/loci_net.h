/*
 * loci_net — device réseau `N:` de LOCI (URL-as-file), côté 6502.
 *
 * Une URL s'ouvre, se lit et se ferme comme un fichier : LOCI pilote le modem AT
 * (PicoWiFiModemUSB) pour votre compte, retire l'habillage HTTP et vous livre le
 * CORPS, rien d'autre. Aucun octet de commande AT à écrire, aucune réponse à
 * analyser depuis l'Oric.
 *
 *   loci_net_t n;
 *   if (loci_net_open(&n, "N:https://exemple.fr/data.bin") == 0) {
 *       while ((got = loci_net_read(&n, 0x8000, 512)) > 0) { ... }
 *       loci_net_close(&n);
 *   }
 *
 * Opcodes utilisés : `open` ($14) — routé sur le préfixe `N:` côté firmware —,
 * `read_xram` ($17), `close` ($15) et `MIA_OP_NET_CONTROL` ($B7) pour l'état.
 *
 * CE QU'IL FAUT SAVOIR AVANT DE S'EN SERVIR
 *
 *  - **Une seule connexion à la fois** : le canal AT du modem est unique. Un
 *    second `loci_net_open` sans avoir fermé le premier échoue (`EMFILE`).
 *  - **Lecture seule** dans cette version (l'écriture demanderait `ATPOST`, qui
 *    ne transporte pas d'octets binaires).
 *  - **Exclusif avec l'ACIA transparente** : pendant une transaction `N:`, un
 *    programme qui parlerait au modem par `$0380` (un terminal, façon OricTel)
 *    ne verrait rien passer — les deux se partagent le même lien.
 *  - **Les données arrivent en XRAM**, pas en RAM Oric : `loci_net_read` prend
 *    une adresse XRAM, à relire ensuite par la fenêtre XRAM habituelle.
 *  - **La première lecture peut être LONGUE** : en HTTPS le dongle termine le TLS
 *    avant de recevoir le premier octet, ce qui prend plusieurs secondes. La
 *    lecture rend la main sans rien livrer plutôt que de bloquer ; voir
 *    `loci_net_read`.
 *
 * Réf : extensions/net-device-B7/spec-net-device.md (lot 1).
 */
#ifndef _LOCI_NET_H
#define _LOCI_NET_H

/* Handle de transaction réseau. */
typedef struct {
    int fd;              /* descripteur LOCI (< 0 = invalide) */
} loci_net_t;

/* Sous-fonctions de `MIA_OP_NET_CONTROL` ($B7). */
#define LOCI_NET_STATUS 0x00
#define LOCI_NET_JSON   0x04

/* État d'une transaction, tel que le rapporte le firmware. */
typedef struct {
    unsigned int  http;   /* code de statut HTTP (0 = pas encore connu) */
    unsigned char state;  /* 0 inactif, 1 envoi, 2 réception, 3 fin de corps, 4 erreur */
    unsigned int  avail;  /* octets déjà prêts à être lus */
} loci_net_status_t;

#define LOCI_NET_ST_IDLE 0
#define LOCI_NET_ST_SEND 1
#define LOCI_NET_ST_RECV 2
#define LOCI_NET_ST_EOF  3
#define LOCI_NET_ST_ERR  4
#define LOCI_NET_ST_WBUF 5   /* ouvert en écriture, corps en cours d'accumulation */
#define LOCI_NET_ST_DIAL 6   /* tcp:// : numérotation, attente de CONNECT */
#define LOCI_NET_ST_STREAM 7 /* tcp:// : connecté, flux brut (avail = octets prêts) */
#define LOCI_NET_ST_HANGUP 8 /* tcp:// : raccrochage en cours (canal encore tenu) */

/* Ouvre une URL en lecture. `url` commence par « N: » suivi du schéma
 * (`http://`, `https://`). Renvoie 0 si la transaction est armée, -1 sinon
 * (`errno` : `ENODEV` pas de modem, `EMFILE` une transaction déjà en cours,
 * `EINVAL` URL vide ou trop longue).
 *
 * N'ATTEND PAS la réponse du serveur : l'échec d'une URL injoignable se découvre
 * à la première lecture, pas ici. */
int __fastcall__ loci_net_open(loci_net_t *n, const char *url);

/* Lit au plus `len` octets du CORPS vers l'adresse XRAM `xaddr`.
 *
 *   > 0  octets livrés
 *   = 0  fin du corps (tout a été lu)
 *   < 0  erreur (`errno`)
 *
 * Un retour de 0 signifie bien la FIN, jamais « rien pour l'instant » : le
 * firmware garde la main tant que des données sont en route, et ne répond qu'une
 * fois qu'il a des octets ou que le corps est terminé. Une attente réseau se voit
 * donc comme un appel qui dure, pas comme une lecture vide à réessayer. */
int __fastcall__ loci_net_read(loci_net_t *n, unsigned int xaddr,
                               unsigned int len);

/* Ouvre une URL en ÉCRITURE (PUT binaire via la commande `ATDISKWR` du dongle).
 * Le corps s'accumule avec loci_net_write() (au plus 2 Ko) ; il part au
 * loci_net_close(), qui rend la main aussitôt : interroger loci_net_status()
 * jusqu'à LOCI_NET_ST_EOF (http = code de réponse) ou LOCI_NET_ST_ERR. Le canal
 * est libéré par le loci_net_open() suivant. Renvoie 0, -1 sinon (errno). */
int __fastcall__ loci_net_open_write(loci_net_t *n, const char *url);

/* Ajoute `len` octets pris en XRAM `xaddr` au corps du PUT. Renvoie le nombre
 * d'octets acceptés, -1 sinon (ENOSPC : corps > 2 Ko). */
int __fastcall__ loci_net_write(loci_net_t *n, unsigned int xaddr,
                                unsigned int len);

/* Ouvre un flux TCP brut : « N:tcp://hôte:port » (ou « N:telnet://hôte:port »
 * avec négociation telnet par le dongle). Bidirectionnel : loci_net_read() sert
 * les octets reçus (bloque tant qu'il n'y en a pas : interroger loci_net_status()
 * — state STREAM, avail > 0 — avant de lire), loci_net_write() les envoie
 * aussitôt. La fin distante se voit par read = 0 (trame « NO CARRIER » du modem).
 * loci_net_close() raccroche en fond (+++ puis ATH, ~2,5 s) : le canal reste
 * tenu jusque-là (EMFILE à un open trop tôt). Renvoie 0, -1 sinon (errno). */
int __fastcall__ loci_net_open_tcp(loci_net_t *n, const char *url);

/* Extrait un champ du corps JSON d'une réponse GET encore NON LUE (≤ 2 Ko, de
 * préférence après LOCI_NET_ST_EOF) : `path` = « a.b[2].c ». Copie la valeur dans
 * `out` (au plus cap-1 caractères, terminée par 0) : chaîne sans guillemets,
 * nombre/true/false/null tels quels, objet/tableau = texte brut. Renvoie la
 * longueur, -1 sinon (ENOENT : absent ; EINVAL : pas un JSON). Non destructif. */
int __fastcall__ loci_net_json(const char *path, char *out, unsigned char cap);

/* Lit l'état de la transaction (code HTTP, avancement, octets disponibles).
 * Renvoie 0 si OK, -1 sinon.
 *
 * Utile pour distinguer un corps vide d'une erreur du serveur : un `404` est une
 * réponse valide du point de vue du transport, `loci_net_read` rendra simplement
 * le corps de la page d'erreur. C'est `http` qui tranche. */
int __fastcall__ loci_net_status(loci_net_status_t *st);

/* Ferme la transaction et rend le lien modem. À appeler même après une erreur :
 * sans quoi le canal reste pris et le prochain `loci_net_open` échouera. */
void __fastcall__ loci_net_close(loci_net_t *n);

#endif /* _LOCI_NET_H */
