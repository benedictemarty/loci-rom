/*
 * loci_stream — implémentation (voir include/loci_stream.h).
 *
 * S'appuie sur les helpers fastcall de mia.s (mia_push_*, mia_set_ax,
 * mia_call_int_errno) et les opcodes MIA_OP_STREAM_BANK ($A8) / MIA_OP_SET_BANK
 * ($A7). Style calqué sur libsrc/lseek.c et libsrc/read_xram.c.
 */
#include <loci.h>
#include <loci_stream.h>
#include <fcntl.h>

#define LOCI_BANK_EN  0x80 /* MIA_OP_SET_BANK : bit7 = overlay actif */
#define LOCI_BANK_SEL 0x0F /* bits3:0 = index de banque */

int __fastcall__ loci_stream_open(loci_stream_t *s, const char *path,
                                  unsigned char sel)
{
    s->fd = open(path, O_RDONLY);
    if (s->fd < 0)
        return -1;
    s->sel = sel & LOCI_BANK_SEL;
    s->xbase = (unsigned int)s->sel << 14; /* sel * 0x4000 */
    return 0;
}

int __fastcall__ loci_stream_load(loci_stream_t *s, unsigned long off,
                                  unsigned int dst, unsigned int len)
{
    /* Ordre de push = miroir du dépilage LIFO firmware (std_api_stream_bank) :
     * fd poussé en premier (dépilé en dernier, _end), len poussé en dernier. */
    mia_push_char(s->fd);
    mia_push_long(off);
    mia_push_int(dst);
    mia_push_int(len);
    mia_set_ax(s->sel & LOCI_BANK_SEL); /* A = MAP(0) | SEL : charge sans mapper */
    return mia_call_int_errno(MIA_OP_STREAM_BANK);
}

void __fastcall__ loci_stream_show(loci_stream_t *s)
{
    mia_set_ax(LOCI_BANK_EN | (s->sel & LOCI_BANK_SEL));
    mia_call_int(MIA_OP_SET_BANK);
}

void __fastcall__ loci_stream_hide(void)
{
    mia_set_ax(0x00); /* EN=0 : passthrough ROM interne */
    mia_call_int(MIA_OP_SET_BANK);
}

void __fastcall__ loci_stream_close(loci_stream_t *s)
{
    if (s->fd >= 0) {
        close(s->fd);
        s->fd = -1;
    }
}
