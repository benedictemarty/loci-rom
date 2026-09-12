/*
 * loci_fs — implémentation (voir include/loci_fs.h).
 * Style calqué sur cc65 libsrc/rp6502/f_stat.c et f_getfree.c (même ABI) et sur
 * libsrc/sysmkdir.c (chaîne poussée à l'envers).
 */
#include <loci.h>
#include <loci_fs.h>
#include <string.h>

static void push_str(const char *s)
{
    size_t n = strlen(s);
    while (n)
        mia_push_char(((const char *)s)[--n]);
}

int __fastcall__ loci_stat(const char *path, loci_stat_t *st)
{
    int i, ax;
    push_str(path);
    ax = mia_call_int_errno(MIA_OP_STAT);
    if (ax < 0)
        return -1;
    for (i = 0; i < (int)sizeof(loci_stat_t); i++)
        ((char *)st)[i] = mia_pop_char();
    return ax;
}

int __fastcall__ loci_chdir(const char *path)
{
    push_str(path);
    return mia_call_int_errno(MIA_OP_CHDIR);
}

int __fastcall__ loci_getfree(const char *volume, unsigned long *nfree, unsigned long *total)
{
    int ax;
    push_str(volume);
    ax = mia_call_int_errno(MIA_OP_GETFREE);
    if (ax < 0)
        return -1;
    *nfree = (unsigned long)mia_pop_long();
    *total = (unsigned long)mia_pop_long();
    return ax;
}

int __fastcall__ loci_syncfs(int fd)
{
    mia_set_ax(fd < 0 ? 0xFF : (unsigned int)fd);
    return mia_call_int_errno(MIA_OP_SYNCFS);
}
