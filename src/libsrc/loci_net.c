/*
 * loci_net — implémentation (voir include/loci_net.h).
 *
 * S'appuie sur les helpers fastcall de mia.s et sur les opcodes `open` ($14),
 * `read_xram` ($17), `close` ($15) et `MIA_OP_NET_CONTROL` ($B7). Style calqué
 * sur libsrc/loci_stream.c et libsrc/read_xram.c.
 */
#include <loci.h>
#include <loci_net.h>
#include <fcntl.h>
#include <errno.h>

int __fastcall__ loci_net_open(loci_net_t *n, const char *url)
{
    /* Le routage se fait côté firmware sur le préfixe « N: » (std_api_open) :
     * un `open` ordinaire suffit donc, et le fd obtenu s'utilise avec les mêmes
     * opcodes que pour un fichier. */
    n->fd = open(url, O_RDONLY);
    return (n->fd < 0) ? -1 : 0;
}

int __fastcall__ loci_net_read(loci_net_t *n, unsigned int xaddr,
                               unsigned int len)
{
    if (n->fd < 0)
        return -1;
    /* Même ordre de push que read_xram() : buf, count, puis fd dans A. */
    mia_push_int(xaddr);
    mia_push_int(len);
    mia_set_ax(n->fd);
    return mia_call_int_errno(MIA_OP_READ_XRAM);
}

int __fastcall__ loci_net_open_write(loci_net_t *n, const char *url)
{
    n->fd = open(url, O_WRONLY);      /* routage « N: » + mode écriture côté firmware */
    return (n->fd < 0) ? -1 : 0;
}

int __fastcall__ loci_net_write(loci_net_t *n, unsigned int xaddr,
                                unsigned int len)
{
    if (n->fd < 0)
        return -1;
    /* Même ordre de push que write_xram() : buf, count, puis fd dans A. */
    mia_push_int(xaddr);
    mia_push_int(len);
    mia_set_ax(n->fd);
    return mia_call_int_errno(MIA_OP_WRITE_XRAM);
}

int __fastcall__ loci_net_open_tcp(loci_net_t *n, const char *url)
{
    n->fd = open(url, O_RDWR);        /* flux bidirectionnel ; « N:tcp:// » routé par le firmware */
    return (n->fd < 0) ? -1 : 0;
}

int __fastcall__ loci_net_json(const char *path, char *out, unsigned char cap)
{
    int r, i;
    unsigned char n;
    /* Chaîne sur le xstack comme un chemin de fichier : dernier caractère poussé
     * en premier, pour que xstack[xstack_ptr] soit le 1er caractère. */
    n = 0;
    while (path[n]) ++n;
    mia_call_void(MIA_OP_ZXSTACK);
    for (i = n - 1; i >= 0; --i)
        mia_push_char(path[i]);
    mia_set_ax(LOCI_NET_JSON);
    r = mia_call_int_errno(MIA_OP_NET_CONTROL);
    if (r < 0)
        return -1;
    for (i = 0; i < r && i < (int)cap - 1; ++i)
        out[i] = mia_pop_char();
    out[i] = 0;
    return r;
}

int __fastcall__ loci_net_prefix(const char *base)
{
    int i;
    unsigned char n = 0;
    while (base[n]) ++n;
    mia_call_void(MIA_OP_ZXSTACK);
    for (i = n - 1; i >= 0; --i)
        mia_push_char(base[i]);
    mia_set_ax(LOCI_NET_PREFIX);
    return mia_call_int_errno(MIA_OP_NET_CONTROL) < 0 ? -1 : 0;
}

int __fastcall__ loci_net_time(unsigned long *epoch, char *str, unsigned char set_rtc)
{
    long r;
    unsigned char i;
    mia_set_ax(LOCI_NET_TIME | ((unsigned int)(set_rtc & 1) << 8));
    r = mia_call_long_errno(MIA_OP_NET_CONTROL);
    if (r == -1L) {
        if (errno == EAGAIN)
            return 1;                       /* requête en cours : rappeler */
        return -1;
    }
    if (r == 0L)
        return 1;                           /* requête lancée : rappeler */
    *epoch = (unsigned long)r;
    for (i = 0; i < 19; ++i)
        str[i] = mia_pop_char();
    str[19] = 0;
    return 0;
}

int __fastcall__ loci_net_status(loci_net_status_t *st)
{
    int r;

    mia_set_ax(LOCI_NET_STATUS);
    r = mia_call_int_errno(MIA_OP_NET_CONTROL);
    if (r < 0)
        return -1;

    /* ⚠️ CONTRAT NON COUVERT PAR UN TEST — à tenir à la main, des deux côtés.
     *
     * Le xstack est LIFO et `api_push_n` DÉCRÉMENTE le pointeur : le DERNIER champ
     * poussé par le firmware est le PREMIER dépilé ici. `net_api_control`
     * (`firmware/src/mia/api/net.c`) pousse avail (u16), state (u8), status (u16)
     * — on dépile donc dans l'ordre inverse : status, state, avail.
     *
     * Changer l'un des deux côtés sans l'autre décale les trois valeurs SANS
     * aucune erreur visible : `http` prendrait la valeur d'`avail`, etc. Ce qui est
     * vérifié aujourd'hui, c'est que l'opcode pousse bien 5 octets ; l'ORDRE ne
     * l'est pas (cf. la note H de `emul/tests/test_net.c`). Toute modification de
     * `net_api_control` doit donc être répercutée ici DANS LE MÊME COMMIT. */
    st->http  = (unsigned int)mia_pop_int();
    st->state = (unsigned char)mia_pop_char();
    st->avail = (unsigned int)mia_pop_int();
    return 0;
}

void __fastcall__ loci_net_close(loci_net_t *n)
{
    if (n->fd >= 0) {
        close(n->fd);
        n->fd = -1;
    }
}
