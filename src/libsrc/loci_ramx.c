/* loci_ramx — implémentation (voir include/loci_ramx.h). */
#include <loci.h>
#include <loci_ramx.h>
#include <string.h>

unsigned int __fastcall__ loci_ramx_pages(void)
{
    mia_set_ax(0);
    return (unsigned int)mia_call_int(MIA_OP_RAMX);
}

void __fastcall__ loci_ramx_read(unsigned int page, void *dst, unsigned char len)
{
    loci_ramx_select(page);
    memcpy(dst, LOCI_RAMX_WINDOW, len);
}

void __fastcall__ loci_ramx_write(unsigned int page, const void *src, unsigned char len)
{
    loci_ramx_select(page);
    memcpy(LOCI_RAMX_WINDOW, src, len);
}
