/* loci_math.h — wrappers cc65 du coprocesseur arithmétique LOCI (opcode $A9).
 *
 * Délègue au RP2040 les calculs absents du 6502 (mul/div, flottant IEEE754,
 * transcendantes). Sous-code d'opération dans A (API_A), opérandes sur le
 * xstack. Convention (alignée firmware/Phosphoric) : pour op(a,b) on pousse
 * a PUIS b (b au sommet) ; le coprocesseur dépile b puis a et calcule op(a,b).
 *
 * Le flottant est manipulé en bits IEEE754 (unsigned long) — cc65 n'a pas de
 * float natif ; l'appelant fournit/lit les motifs binaires (ex. 1.0f =
 * 0x3F800000UL). Voir extensions/coprocessor-A9/spec-coprocesseur-math.md.
 *
 * Détectable : sur un firmware sans coproc, $A9 renvoie une erreur (opcode
 * inconnu) — tester via les variantes *_errno si besoin.
 */
#ifndef _LOCI_MATH_H
#define _LOCI_MATH_H

#include "loci.h"

/* ---- Sous-codes (API_A) ---- */
#define MATH_MUL_U16    0x00
#define MATH_MUL_I16    0x01
#define MATH_DIVMOD_U16 0x02
#define MATH_DIVMOD_I16 0x03
#define MATH_MUL_U32    0x04
#define MATH_DIVMOD_U32 0x05
#define MATH_DIVMOD_I32 0x06
#define MATH_FADD       0x10
#define MATH_FSUB       0x11
#define MATH_FMUL       0x12
#define MATH_FDIV       0x13
#define MATH_FCMP       0x14
#define MATH_ITOF       0x15
#define MATH_FTOI       0x16
#define MATH_FSQRT      0x20
#define MATH_FSIN       0x21
#define MATH_FCOS       0x22
#define MATH_FTAN       0x23
#define MATH_FATAN2     0x24
#define MATH_FLOG       0x25
#define MATH_FEXP       0x26
#define MATH_FPOW       0x27
#define MATH_MBF_TO_IEEE 0x30
#define MATH_IEEE_TO_MBF 0x31
#define MATH_VEC_DOT     0x40
#define MATH_VEC_SCALE   0x41
#define MATH_POLY_EVAL   0x42

/* ---- Entiers : produit (retour scalaire AX:SREG) ---- */
static unsigned long loci_mul_u16(unsigned a, unsigned b) {
    mia_push_int(a); mia_push_int(b);
    mia_set_a(MATH_MUL_U16);
    return (unsigned long)mia_call_long(MIA_OP_MATH);
}
static long loci_mul_i16(int a, int b) {
    mia_push_int((unsigned)a); mia_push_int((unsigned)b);
    mia_set_a(MATH_MUL_I16);
    return mia_call_long(MIA_OP_MATH);
}
static unsigned long loci_mul_u32(unsigned long a, unsigned long b) {
    mia_push_long(a); mia_push_long(b);
    mia_set_a(MATH_MUL_U32);
    return (unsigned long)mia_call_long(MIA_OP_MATH);
}

/* ---- Entiers : divmod (retour : quotient ; reste écrit via *rem) ----
 * Multi-valeurs sur le xstack : quotient au sommet, reste dessous.
 * Division par zéro : le coproc renvoie une erreur (résultats indéfinis) —
 * l'appelant doit garantir divisor != 0 (ou vérifier en amont). */
static unsigned loci_divmod_u16(unsigned dividend, unsigned divisor, unsigned *rem) {
    mia_push_int(dividend); mia_push_int(divisor);
    mia_set_a(MATH_DIVMOD_U16);
    mia_call_void(MIA_OP_MATH);
    { unsigned q = (unsigned)mia_pop_int(); *rem = (unsigned)mia_pop_int(); return q; }
}
static unsigned long loci_divmod_u32(unsigned long dividend, unsigned long divisor,
                                     unsigned long *rem) {
    mia_push_long(dividend); mia_push_long(divisor);
    mia_set_a(MATH_DIVMOD_U32);
    mia_call_void(MIA_OP_MATH);
    { unsigned long q = (unsigned long)mia_pop_long(); *rem = (unsigned long)mia_pop_long(); return q; }
}

/* ---- Flottant IEEE754 (bits u32 in/out) ---- */
static unsigned long loci_f_binop(unsigned char sub, unsigned long a, unsigned long b) {
    mia_push_long(a); mia_push_long(b);
    mia_set_a(sub);
    return (unsigned long)mia_call_long(MIA_OP_MATH);
}
static unsigned long loci_f_unop(unsigned char sub, unsigned long a) {
    mia_push_long(a);
    mia_set_a(sub);
    return (unsigned long)mia_call_long(MIA_OP_MATH);
}
#define loci_fadd(a,b)  loci_f_binop(MATH_FADD,(a),(b))
#define loci_fsub(a,b)  loci_f_binop(MATH_FSUB,(a),(b))
#define loci_fmul(a,b)  loci_f_binop(MATH_FMUL,(a),(b))
#define loci_fdiv(a,b)  loci_f_binop(MATH_FDIV,(a),(b))
#define loci_fatan2(a,b) loci_f_binop(MATH_FATAN2,(a),(b))
#define loci_fpow(a,b)  loci_f_binop(MATH_FPOW,(a),(b))
#define loci_fsqrt(a)   loci_f_unop(MATH_FSQRT,(a))
#define loci_fsin(a)    loci_f_unop(MATH_FSIN,(a))
#define loci_fcos(a)    loci_f_unop(MATH_FCOS,(a))
#define loci_ftan(a)    loci_f_unop(MATH_FTAN,(a))
#define loci_flog(a)    loci_f_unop(MATH_FLOG,(a))
#define loci_fexp(a)    loci_f_unop(MATH_FEXP,(a))

/* fcmp : renvoie -1/0/1 (dans A). itof : i32 -> bits f32. ftoi : bits f32 -> i32. */
static int loci_fcmp(unsigned long a, unsigned long b) {
    mia_push_long(a); mia_push_long(b);
    mia_set_a(MATH_FCMP);
    return mia_call_int(MIA_OP_MATH);
}
static unsigned long loci_itof(long i) {
    mia_push_long((unsigned long)i);
    mia_set_a(MATH_ITOF);
    return (unsigned long)mia_call_long(MIA_OP_MATH);
}
static long loci_ftoi(unsigned long fbits) {
    mia_push_long(fbits);
    mia_set_a(MATH_FTOI);
    return mia_call_long(MIA_OP_MATH);
}

/* ---- Pont MBF (5 octets) <-> IEEE754 (bits u32) ----
 * MBF : out[0]=exposant (au sommet du xstack), out[1..4]=mantisse. */
static unsigned long loci_mbf_to_ieee(const unsigned char mbf[5]) {
    /* pousser mbf[4]..mbf[0] pour que mbf[0] (exp) soit au sommet */
    mia_push_char(mbf[4]); mia_push_char(mbf[3]); mia_push_char(mbf[2]);
    mia_push_char(mbf[1]); mia_push_char(mbf[0]);
    mia_set_a(MATH_MBF_TO_IEEE);
    return (unsigned long)mia_call_long(MIA_OP_MATH);
}
static void loci_ieee_to_mbf(unsigned long fbits, unsigned char out[5]) {
    mia_push_long(fbits);
    mia_set_a(MATH_IEEE_TO_MBF);
    mia_call_void(MIA_OP_MATH);
    /* 5 octets sur le xstack, exposant (out[0]) au sommet -> dépilé en 1er */
    out[0] = mia_pop_char(); out[1] = mia_pop_char(); out[2] = mia_pop_char();
    out[3] = mia_pop_char(); out[4] = mia_pop_char();
}

/* ---- Ops par bloc sur vecteurs f32 en XRAM (pointeurs = offsets XRAM) ---- */
static unsigned long loci_vec_dot(unsigned ptrA, unsigned ptrB, unsigned count) {
    mia_push_int(ptrA); mia_push_int(ptrB); mia_push_int(count);
    mia_set_a(MATH_VEC_DOT);
    return (unsigned long)mia_call_long(MIA_OP_MATH);
}
static void loci_vec_scale(unsigned ptr, unsigned count, unsigned long a_bits) {
    mia_push_int(ptr); mia_push_int(count); mia_push_long(a_bits);
    mia_set_a(MATH_VEC_SCALE);
    mia_call_void(MIA_OP_MATH);
}
static unsigned long loci_poly_eval(unsigned ptr, unsigned degree, unsigned long x_bits) {
    mia_push_int(ptr); mia_push_int(degree); mia_push_long(x_bits);
    mia_set_a(MATH_POLY_EVAL);
    return (unsigned long)mia_call_long(MIA_OP_MATH);
}

#endif /* _LOCI_MATH_H */
