/******************************************************************************
 *
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************
 * Originally developed and contributed by Imagination Technologies
*/

#include "ihevc_typedefs.h"
#include "ihevc_itrans_recon.h"
#include "ihevc_macros.h"
#include "ihevc_macros_msa.h"

static WORD16 gt8x8_cnst[16] =
{
    64, 64, 83, 36, 89, 50, 18, 75, 64, -64, 36, -83, 75, -89, -50, -18
};

static WORD16 gt16x16_cnst[64] =
{
    64, 83, 64, 36, 89, 75, 50, 18, 90, 80, 57, 25, 70, 87, 9, 43,
    64, 36, -64, -83, 75, -18, -89, -50, 87, 9, -80, -70, -43, 57, -25, -90,
    64, -36, -64, 83, 50, -89, 18, 75, 80, -70, -25, 90, -87, 9, 43, 57,
    64, -83, 64, -36, 18, -50, 75, -89, 70, -87, 90, -80, 9, -43, -57, 25
};

static WORD16 gt32x32_cnst0[256] =
{
    90, 90, 88, 85, 82, 78, 73, 67, 61, 54, 46, 38, 31, 22, 13, 4,
    90, 82, 67, 46, 22, -4, -31, -54, -73, -85, -90, -88, -78, -61, -38, -13,
    88, 67, 31, -13, -54, -82, -90, -78, -46, -4, 38, 73, 90, 85, 61, 22,
    85, 46, -13, -67, -90, -73, -22, 38, 82, 88, 54, -4, -61, -90, -78, -31,
    82, 22, -54, -90, -61, 13, 78, 85, 31, -46, -90, -67, 4, 73, 88, 38,
    78, -4, -82, -73, 13, 85, 67, -22, -88, -61, 31, 90, 54, -38, -90, -46,
    73, -31, -90, -22, 78, 67, -38, -90, -13, 82, 61, -46, -88, -4, 85, 54,
    67, -54, -78, 38, 85, -22, -90, 4, 90, 13, -88, -31, 82, 46, -73, -61,
    61, -73, -46, 82, 31, -88, -13, 90, -4, -90, 22, 85, -38, -78, 54, 67,
    54, -85, -4, 88, -46, -61, 82, 13, -90, 38, 67, -78, -22, 90, -31, -73,
    46, -90, 38, 54, -90, 31, 61, -88, 22, 67, -85, 13, 73, -82, 4, 78,
    38, -88, 73, -4, -67, 90, -46, -31, 85, -78, 13, 61, -90, 54, 22, -82,
    31, -78, 90, -61, 4, 54, -88, 82, -38, -22, 73, -90, 67, -13, -46, 85,
    22, -61, 85, -90, 73, -38, -4, 46, -78, 90, -82, 54, -13, -31, 67, -88,
    13, -38, 61, -78, 88, -90, 85, -73, 54, -31, 4, 22, -46, 67, -82, 90,
    4, -13, 22, -31, 38, -46, 54, -61, 67, -73, 78, -82, 85, -88, 90, -90
};

static WORD16 gt32x32_cnst1[64] =
{
    90, 87, 80, 70, 57, 43, 25, 9, 87, 57, 9, -43, -80, -90, -70, -25,
    80, 9, -70, -87, -25, 57, 90, 43, 70, -43, -87, 9, 90, 25, -80, -57,
    57, -80, -25, 90, -9, -87, 43, 70, 43, -90, 57, 25, -87, 70, 9, -80,
    25, -70, 90, -80, 43, 9, -57, 87, 9, -25, 43, -57, 70, -80, 87, -90
};

static WORD16 gt32x32_cnst2[16] =
{
    89, 75, 50, 18, 75, -18, -89, -50, 50, -89, 18, 75, 18, -50, 75, -89
};

static WORD16 gt32x32_cnst3[16] =
{
    64, 64, 64, 64, 83, 36, -36, -83, 64, -64, -64, 64, 36, -83, 83, -36
};

#define HEVC_IDCT4x4_COL(in0_r, in0_l, in1_r, in1_l,          \
                         sum0, sum1, sum2, sum3, shift)       \
{                                                             \
    v4i32 vec0, vec1, vec2, vec3, vec4, vec5;                 \
    v4i32 cnst64 = __msa_ldi_w(64);                           \
    v4i32 cnst83 = __msa_ldi_w(83);                           \
    v4i32 cnst36 = __msa_ldi_w(36);                           \
                                                              \
    DOTP_SH4_SW(in0_r, in1_r, in0_l, in1_l, cnst64, cnst64,   \
                cnst83, cnst36, vec0, vec2, vec1, vec3);      \
    DOTP_SH2_SW(in0_l, in1_l, cnst36, cnst83, vec4, vec5);    \
                                                              \
    sum0 = vec0 + vec2;                                       \
    sum1 = vec0 - vec2;                                       \
    sum3 = sum0;                                              \
    sum2 = sum1;                                              \
                                                              \
    vec1 += vec3;                                             \
    vec4 -= vec5;                                             \
                                                              \
    sum0 += vec1;                                             \
    sum1 += vec4;                                             \
    sum2 -= vec4;                                             \
    sum3 -= vec1;                                             \
                                                              \
    SRARI_W4_SW(sum0, sum1, sum2, sum3, shift);               \
    SAT_SW4_SW(sum0, sum1, sum2, sum3, 15);                   \
}

#define HEVC_IDCT8x8_COL(in0, in1, in2, in3, in4, in5, in6, in7,         \
                         shift, filt)                                    \
{                                                                        \
    v8i16 src0_r, src1_r, src2_r, src3_r;                                \
    v8i16 src0_l, src1_l, src2_l, src3_l;                                \
    v8i16 filt0, filter0, filter1, filter2, filter3;                     \
    v4i32 temp0_r, temp1_r, temp2_r, temp3_r, temp4_r, temp5_r;          \
    v4i32 temp0_l, temp1_l, temp2_l, temp3_l, temp4_l, temp5_l;          \
    v4i32 sum0_r, sum1_r, sum2_r, sum3_r;                                \
    v4i32 sum0_l, sum1_l, sum2_l, sum3_l;                                \
                                                                         \
    ILVR_H4_SH(in4, in0, in6, in2, in5, in1, in3, in7,                   \
               src0_r, src1_r, src2_r, src3_r);                          \
    ILVL_H4_SH(in4, in0, in6, in2, in5, in1, in3, in7,                   \
               src0_l, src1_l, src2_l, src3_l);                          \
                                                                         \
    filt0 = LD_SH(filt);                                                 \
    SPLATI_W4_SH(filt0, filter0, filter1, filter2, filter3);             \
    DOTP_SH4_SW(src0_r, src0_l, src1_r, src1_l, filter0, filter0,        \
                filter1, filter1, temp0_r, temp0_l, temp1_r, temp1_l);   \
                                                                         \
    BUTTERFLY_4(temp0_r, temp0_l, temp1_l, temp1_r, sum0_r, sum0_l,      \
                sum1_l, sum1_r);                                         \
    sum2_r = sum1_r;                                                     \
    sum2_l = sum1_l;                                                     \
    sum3_r = sum0_r;                                                     \
    sum3_l = sum0_l;                                                     \
                                                                         \
    DOTP_SH4_SW(src2_r, src2_l, src3_r, src3_l,  filter2, filter2,       \
                filter3, filter3, temp2_r, temp2_l, temp3_r, temp3_l);   \
                                                                         \
    temp2_r += temp3_r;                                                  \
    temp2_l += temp3_l;                                                  \
    sum0_r += temp2_r;                                                   \
    sum0_l += temp2_l;                                                   \
    sum3_r -= temp2_r;                                                   \
    sum3_l -= temp2_l;                                                   \
                                                                         \
    SRARI_W4_SW(sum0_r, sum0_l, sum3_r, sum3_l, shift);                  \
    SAT_SW4_SW(sum0_r, sum0_l, sum3_r, sum3_l, 15);                      \
    PCKEV_H2_SH(sum0_l, sum0_r, sum3_l, sum3_r, in0, in7);               \
    DOTP_SH4_SW(src2_r, src2_l, src3_r, src3_l,  filter3, filter3,       \
                filter2, filter2, temp4_r, temp4_l, temp5_r, temp5_l);   \
                                                                         \
    temp4_r -= temp5_r;                                                  \
    temp4_l -= temp5_l;                                                  \
    sum1_r += temp4_r;                                                   \
    sum1_l += temp4_l;                                                   \
    sum2_r -= temp4_r;                                                   \
    sum2_l -= temp4_l;                                                   \
                                                                         \
    SRARI_W4_SW(sum1_r, sum1_l, sum2_r, sum2_l, shift);                  \
    SAT_SW4_SW(sum1_r, sum1_l, sum2_r, sum2_l, 15);                      \
    PCKEV_H2_SH(sum1_l, sum1_r, sum2_l, sum2_r, in3, in4);               \
                                                                         \
    filt0 = LD_SH(filt + 8);                                             \
    SPLATI_W4_SH(filt0, filter0, filter1, filter2, filter3);             \
    DOTP_SH4_SW(src0_r, src0_l, src1_r, src1_l,  filter0, filter0,       \
                filter1, filter1, temp0_r, temp0_l, temp1_r, temp1_l);   \
                                                                         \
    BUTTERFLY_4(temp0_r, temp0_l, temp1_l, temp1_r, sum0_r, sum0_l,      \
                sum1_l, sum1_r);                                         \
    sum2_r = sum1_r;                                                     \
    sum2_l = sum1_l;                                                     \
    sum3_r = sum0_r;                                                     \
    sum3_l = sum0_l;                                                     \
                                                                         \
    DOTP_SH4_SW(src2_r, src2_l, src3_r, src3_l, filter2, filter2,        \
                filter3, filter3, temp2_r, temp2_l, temp3_r, temp3_l);   \
                                                                         \
    temp2_r += temp3_r;                                                  \
    temp2_l += temp3_l;                                                  \
    sum0_r += temp2_r;                                                   \
    sum0_l += temp2_l;                                                   \
    sum3_r -= temp2_r;                                                   \
    sum3_l -= temp2_l;                                                   \
                                                                         \
    SRARI_W4_SW(sum0_r, sum0_l, sum3_r, sum3_l, shift);                  \
    SAT_SW4_SW(sum0_r, sum0_l, sum3_r, sum3_l, 15);                      \
    PCKEV_H2_SH(sum0_l, sum0_r, sum3_l, sum3_r, in1, in6);               \
    DOTP_SH4_SW(src2_r, src2_l, src3_r, src3_l, filter3, filter3,        \
                filter2, filter2, temp4_r, temp4_l, temp5_r, temp5_l);   \
                                                                         \
    temp4_r -= temp5_r;                                                  \
    temp4_l -= temp5_l;                                                  \
    sum1_r -= temp4_r;                                                   \
    sum1_l -= temp4_l;                                                   \
    sum2_r += temp4_r;                                                   \
    sum2_l += temp4_l;                                                   \
                                                                         \
    SRARI_W4_SW(sum1_r, sum1_l, sum2_r, sum2_l, shift);                  \
    SAT_SW4_SW(sum1_r, sum1_l, sum2_r, sum2_l, 15);                      \
    PCKEV_H2_SH(sum1_l, sum1_r, sum2_l, sum2_r, in2, in5);               \
}

#define HEVC_IDCT16x16_COL(src0_r, src1_r, src2_r, src3_r,                \
                           src4_r, src5_r, src6_r, src7_r,                \
                           src0_l, src1_l, src2_l, src3_l,                \
                           src4_l, src5_l, src6_l, src7_l,                \
                           shift, buf_p)                                  \
{                                                                         \
    WORD16 *ptr0, *ptr1;                                                  \
    v8i16 filt0, filt1, dst0, dst1;                                       \
    v8i16 filter0, filter1, filter2, filter3;                             \
    v4i32 temp0_r, temp1_r, temp0_l, temp1_l;                             \
    v4i32 sum0_r, sum1_r, sum2_r, sum3_r, sum0_l, sum1_l, sum2_l;         \
    v4i32 sum3_l, res0_r, res1_r, res0_l, res1_l;                         \
                                                                          \
    ptr0 = (buf_p + 112);                                                 \
    ptr1 = (buf_p + 128);                                                 \
    k = -1;                                                               \
                                                                          \
    for(j = 0; j < 4; j++)                                                \
    {                                                                     \
        LD_SH2(filter, 8, filt0, filt1)                                   \
        filter += 16;                                                     \
        SPLATI_W2_SH(filt0, 0, filter0, filter1);                         \
        SPLATI_W2_SH(filt1, 0, filter2, filter3);                         \
        DOTP_SH4_SW(src0_r, src0_l, src4_r, src4_l,  filter0, filter0,    \
                    filter2, filter2, sum0_r, sum0_l, sum2_r, sum2_l);    \
        DOTP_SH2_SW(src7_r, src7_l, filter2, filter2, sum3_r, sum3_l);    \
        DPADD_SH4_SW(src1_r, src1_l, src5_r, src5_l,  filter1, filter1,   \
                     filter3, filter3, sum0_r, sum0_l, sum2_r, sum2_l);   \
        DPADD_SH2_SW(src6_r, src6_l, filter3, filter3, sum3_r, sum3_l);   \
                                                                          \
        sum1_r = sum0_r;                                                  \
        sum1_l = sum0_l;                                                  \
                                                                          \
        SPLATI_W2_SH(filt0, 2, filter0, filter1);                         \
        SPLATI_W2_SH(filt1, 2, filter2, filter3);                         \
        DOTP_SH2_SW(src2_r, src2_l, filter0, filter0, temp0_r, temp0_l);  \
        DPADD_SH2_SW(src6_r, src6_l, filter2, filter2, sum2_r, sum2_l);   \
        DOTP_SH2_SW(src5_r, src5_l, filter2, filter2, temp1_r, temp1_l);  \
                                                                          \
        sum0_r += temp0_r;                                                \
        sum0_l += temp0_l;                                                \
        sum1_r -= temp0_r;                                                \
        sum1_l -= temp0_l;                                                \
                                                                          \
        sum3_r = temp1_r - sum3_r;                                        \
        sum3_l = temp1_l - sum3_l;                                        \
                                                                          \
        DOTP_SH2_SW(src3_r, src3_l, filter1, filter1, temp0_r, temp0_l);  \
        DPADD_SH4_SW(src7_r, src7_l, src4_r, src4_l, filter3, filter3,    \
                     filter3, filter3, sum2_r, sum2_l, sum3_r, sum3_l);   \
                                                                          \
        sum0_r += temp0_r;                                                \
        sum0_l += temp0_l;                                                \
        sum1_r -= temp0_r;                                                \
        sum1_l -= temp0_l;                                                \
                                                                          \
        BUTTERFLY_4(sum0_r, sum0_l, sum2_l, sum2_r, res0_r, res0_l,       \
                    res1_l, res1_r);                                      \
        SRARI_W4_SW(res0_r, res0_l, res1_r, res1_l, shift);               \
        SAT_SW4_SW(res0_r, res0_l, res1_r, res1_l, 15);                   \
        PCKEV_H2_SH(res0_l, res0_r, res1_l, res1_r, dst0, dst1);          \
        ST_SH(dst0, buf_p);                                               \
        ST_SH(dst1, (buf_p + ((15 - (j * 2)) * 16)));                     \
                                                                          \
        BUTTERFLY_4(sum1_r, sum1_l, sum3_l, sum3_r, res0_r, res0_l,       \
                    res1_l, res1_r);                                      \
        SRARI_W4_SW(res0_r, res0_l, res1_r, res1_l, shift);               \
        SAT_SW4_SW(res0_r, res0_l, res1_r, res1_l, 15);                   \
        PCKEV_H2_SH(res0_l, res0_r, res1_l, res1_r, dst0, dst1);          \
        ST_SH(dst0, (ptr0 + (((j / 2 + j % 2) * 2 * k) * 16)));           \
        ST_SH(dst1, (ptr1 - (((j / 2 + j % 2) * 2 * k) * 16)));           \
                                                                          \
        k *= -1;                                                          \
        buf_p += 16;                                                      \
    }                                                                     \
}

#define HEVC_EVEN16_CALC(input, sum0_r, sum0_l, load_idx, store_idx)  \
{                                                                     \
    LD_SW2(input + load_idx * 8, 4, tmp0_r, tmp0_l);                  \
    tmp1_r = sum0_r;                                                  \
    tmp1_l = sum0_l;                                                  \
    sum0_r += tmp0_r;                                                 \
    sum0_l += tmp0_l;                                                 \
    ST_SW2(sum0_r, sum0_l, (input + load_idx * 8), 4);                \
    tmp1_r -= tmp0_r;                                                 \
    tmp1_l -= tmp0_l;                                                 \
    ST_SW2(tmp1_r, tmp1_l, (input + store_idx * 8), 4);               \
}

#define HEVC_IDCT_LUMA4x4_COL(in0_r, in0_l, in1_r, in1_l,     \
                              res0, res1, res2, res3, shift)  \
{                                                             \
    v4i32 vec0, vec1, vec2, vec3;                             \
    v4i32 cnst74 = __msa_ldi_w(74);                           \
    v4i32 cnst55 = __msa_ldi_w(55);                           \
    v4i32 cnst29 = __msa_ldi_w(29);                           \
                                                              \
    vec0 = in0_r + in1_r;                                     \
    vec2 = in0_r - in1_l;                                     \
    res0 = vec0 * cnst29;                                     \
    res1 = vec2 * cnst55;                                     \
    res2 = in0_r - in1_r;                                     \
    vec1 = in1_r + in1_l;                                     \
    res2 += in1_l;                                            \
    vec3 = in0_l * cnst74;                                    \
    res3 = vec0 * cnst55;                                     \
                                                              \
    res0 += vec1 * cnst55;                                    \
    res1 -= vec1 * cnst29;                                    \
    res2 *= cnst74;                                           \
    res3 += vec2 * cnst29;                                    \
                                                              \
    res0 += vec3;                                             \
    res1 += vec3;                                             \
    res3 -= vec3;                                             \
                                                              \
    SRARI_W4_SW(res0, res1, res2, res3, shift);               \
    SAT_SW4_SW(res0, res1, res2, res3, 15);                   \
}

static void hevc_idct_8x32_column_msa(WORD16 *coeffs, UWORD8 buf_pitch,
                                      UWORD8 round)
{
    UWORD8 i;
    WORD16 *filter_ptr0 = &gt32x32_cnst0[0];
    WORD16 *filter_ptr1 = &gt32x32_cnst1[0];
    WORD16 *filter_ptr2 = &gt32x32_cnst2[0];
    WORD16 *filter_ptr3 = &gt32x32_cnst3[0];
    WORD16 *src0 = (coeffs + buf_pitch);
    WORD16 *src1 = (coeffs + 2 * buf_pitch);
    WORD16 *src2 = (coeffs + 4 * buf_pitch);
    WORD16 *src3 = (coeffs);
    WORD32 cnst0, cnst1;
    WORD32 tmp_buf[8 * 32];
    WORD32 *tmp_buf_ptr = &tmp_buf[0];
    v8i16 in0, in1, in2, in3, in4, in5, in6, in7;
    v8i16 src0_r, src1_r, src2_r, src3_r, src4_r, src5_r, src6_r, src7_r;
    v8i16 src0_l, src1_l, src2_l, src3_l, src4_l, src5_l, src6_l, src7_l;
    v8i16 filt0, filter0, filter1, filter2, filter3;
    v4i32 sum0_r, sum0_l, sum1_r, sum1_l, tmp0_r, tmp0_l, tmp1_r, tmp1_l;

    /* process coeff 4, 12, 20, 28 */
    LD_SH4(src2, 8 * buf_pitch, in0, in1, in2, in3);
    ILVR_H2_SH(in1, in0, in3, in2, src0_r, src1_r);
    ILVL_H2_SH(in1, in0, in3, in2, src0_l, src1_l);

    /* loop for all columns of constants */
    for(i = 0; i < 4; i++)
    {
        /* processing single column of constants */
        cnst0 = LW(filter_ptr2);
        cnst1 = LW(filter_ptr2 + 2);

        filter0 = (v8i16) __msa_fill_w(cnst0);
        filter1 = (v8i16) __msa_fill_w(cnst1);

        DOTP_SH2_SW(src0_r, src0_l, filter0, filter0, sum0_r, sum0_l);
        DPADD_SH2_SW(src1_r, src1_l, filter1, filter1, sum0_r, sum0_l);
        ST_SW2(sum0_r, sum0_l, (tmp_buf_ptr + i * 8), 4);

        filter_ptr2 += 4;
    }

    /* process coeff 0, 8, 16, 24 */
    LD_SH2(src3, 16 * buf_pitch, in0, in2);
    LD_SH2((src3 + 8 * buf_pitch), 16 * buf_pitch, in1, in3);

    ILVR_H2_SH(in2, in0, in3, in1, src0_r, src1_r);
    ILVL_H2_SH(in2, in0, in3, in1, src0_l, src1_l);

    /* loop for all columns of constants */
    for(i = 0; i < 2; i++)
    {
        /* processing first column of filter constants */
        cnst0 = LW(filter_ptr3);
        cnst1 = LW(filter_ptr3 + 4);

        filter0 = (v8i16) __msa_fill_w(cnst0);
        filter1 = (v8i16) __msa_fill_w(cnst1);

        DOTP_SH4_SW(src0_r, src0_l, src1_r, src1_l, filter0, filter0, filter1,
                    filter1, sum0_r, sum0_l, tmp1_r, tmp1_l);

        sum1_r = sum0_r;
        sum1_l = sum0_l;
        sum0_r += tmp1_r;
        sum0_l += tmp1_l;

        sum1_r -= tmp1_r;
        sum1_l -= tmp1_l;

        HEVC_EVEN16_CALC(tmp_buf_ptr, sum0_r, sum0_l, i, (7 - i));
        HEVC_EVEN16_CALC(tmp_buf_ptr, sum1_r, sum1_l, (3 - i), (4 + i));

        filter_ptr3 += 8;
    }

    /* process coeff 2 6 10 14 18 22 26 30 */
    LD_SH8(src1, 4 * buf_pitch, in0, in1, in2, in3, in4, in5, in6, in7);
    ILVR_H4_SH(in1, in0, in3, in2, in5, in4, in7, in6, src0_r, src1_r, src2_r,
               src3_r);
    ILVL_H4_SH(in1, in0, in3, in2, in5, in4, in7, in6, src0_l, src1_l, src2_l,
               src3_l);

    /* loop for all columns of constants */
    for(i = 0; i < 8; i++)
    {
        /* processing single column of constants */
        filt0 = LD_SH(filter_ptr1);
        SPLATI_W4_SH(filt0, filter0, filter1, filter2, filter3);
        DOTP_SH2_SW(src0_r, src0_l, filter0, filter0, sum0_r, sum0_l);
        DPADD_SH4_SW(src1_r, src1_l, src2_r, src2_l, filter1, filter1, filter2,
                     filter2, sum0_r, sum0_l, sum0_r, sum0_l);
        DPADD_SH2_SW(src3_r, src3_l, filter3, filter3, sum0_r, sum0_l);

        LD_SW2(tmp_buf_ptr + i * 8, 4, tmp0_r, tmp0_l);
        tmp1_r = tmp0_r;
        tmp1_l = tmp0_l;
        tmp0_r += sum0_r;
        tmp0_l += sum0_l;
        ST_SW2(tmp0_r, tmp0_l, (tmp_buf_ptr + i * 8), 4);
        tmp1_r -= sum0_r;
        tmp1_l -= sum0_l;
        ST_SW2(tmp1_r, tmp1_l, (tmp_buf_ptr + (15 - i) * 8), 4);

        filter_ptr1 += 8;
    }

    /* process coeff 1 3 5 7 9 11 13 15 17 19 21 23 25 27 29 31 */
    LD_SH8(src0, 2 * buf_pitch, in0, in1, in2, in3, in4, in5, in6, in7);
    src0 += 16 * buf_pitch;
    ILVR_H4_SH(in1, in0, in3, in2, in5, in4, in7, in6, src0_r, src1_r, src2_r,
               src3_r);
    ILVL_H4_SH(in1, in0, in3, in2, in5, in4, in7, in6, src0_l, src1_l, src2_l,
               src3_l);

    LD_SH8(src0, 2 * buf_pitch, in0, in1, in2, in3, in4, in5, in6, in7);
    ILVR_H4_SH(in1, in0, in3, in2, in5, in4, in7, in6, src4_r, src5_r, src6_r,
               src7_r);
    ILVL_H4_SH(in1, in0, in3, in2, in5, in4, in7, in6, src4_l, src5_l, src6_l,
               src7_l);

    /* loop for all columns of filter constants */
    for(i = 0; i < 16; i++)
    {
        /* processing single column of constants */
        filt0 = LD_SH(filter_ptr0);
        SPLATI_W4_SH(filt0, filter0, filter1, filter2, filter3);
        DOTP_SH2_SW(src0_r, src0_l, filter0, filter0, sum0_r, sum0_l);
        DPADD_SH4_SW(src1_r, src1_l, src2_r, src2_l, filter1, filter1, filter2,
                     filter2, sum0_r, sum0_l, sum0_r, sum0_l);
        DPADD_SH2_SW(src3_r, src3_l, filter3, filter3, sum0_r, sum0_l);

        tmp1_r = sum0_r;
        tmp1_l = sum0_l;

        filt0 = LD_SH(filter_ptr0 + 8);
        SPLATI_W4_SH(filt0, filter0, filter1, filter2, filter3);
        DOTP_SH2_SW(src4_r, src4_l, filter0, filter0, sum0_r, sum0_l);
        DPADD_SH4_SW(src5_r, src5_l, src6_r, src6_l, filter1, filter1, filter2,
                     filter2, sum0_r, sum0_l, sum0_r, sum0_l);
        DPADD_SH2_SW(src7_r, src7_l, filter3, filter3, sum0_r, sum0_l);
        ADD2(sum0_r, tmp1_r, sum0_l, tmp1_l, sum0_r, sum0_l);

        LD_SW2(tmp_buf_ptr + i * 8, 4, tmp0_r, tmp0_l);
        tmp1_r = tmp0_r;
        tmp1_l = tmp0_l;
        tmp0_r += sum0_r;
        tmp0_l += sum0_l;
        sum1_r = __msa_fill_w(round);
        SRAR_W2_SW(tmp0_r, tmp0_l, sum1_r);
        SAT_SW2_SW(tmp0_r, tmp0_l, 15);
        in0 = __msa_pckev_h((v8i16) tmp0_l, (v8i16) tmp0_r);
        ST_SH(in0, (coeffs + i * buf_pitch));
        tmp1_r -= sum0_r;
        tmp1_l -= sum0_l;
        SRAR_W2_SW(tmp1_r, tmp1_l, sum1_r);
        SAT_SW2_SW(tmp1_r, tmp1_l, 15);
        in0 = __msa_pckev_h((v8i16) tmp1_l, (v8i16) tmp1_r);
        ST_SH(in0, (coeffs + (31 - i) * buf_pitch));

        filter_ptr0 += 16;
    }
}

static void hevc_idct_transpose_32x8_to_8x32(WORD16 *coeffs, WORD16 *tmp_buf)
{
    UWORD8 i;
    v8i16 in0, in1, in2, in3, in4, in5, in6, in7;

    for(i = 0; i < 4; i++)
    {
        LD_SH8(coeffs + i * 8, 32, in0, in1, in2, in3, in4, in5, in6, in7);
        TRANSPOSE8x8_SH_SH(in0, in1, in2, in3, in4, in5, in6, in7,
                           in0, in1, in2, in3, in4, in5, in6, in7);
        ST_SH8(in0, in1, in2, in3, in4, in5, in6, in7, tmp_buf + i * 8 * 8, 8);
    }
}

static void hevc_idct_addpred_4x4_msa(WORD16 *src, UWORD8 *dst,
                                      WORD32 dst_stride)
{
    UWORD32 pred0, pred1, pred2, pred3;
    v8i16 in0, in1, pred_r, pred_l;
    v8i16 zeros = { 0 };
    v4i32 in0_r, in0_l, in1_r, in1_l, sum0, sum1, sum2, sum3;
    v4i32 vec = { 0 };

    LD_SH2(src, 8, in0, in1);
    ILVRL_H2_SW(zeros, in0, in0_r, in0_l);
    ILVRL_H2_SW(zeros, in1, in1_r, in1_l);

    HEVC_IDCT4x4_COL(in0_r, in0_l, in1_r, in1_l, sum0, sum1, sum2, sum3, 7);
    TRANSPOSE4x4_SW_SW(sum0, sum1, sum2, sum3, in0_r, in0_l, in1_r, in1_l);
    HEVC_IDCT4x4_COL(in0_r, in0_l, in1_r, in1_l, sum0, sum1, sum2, sum3, 12);
    TRANSPOSE4x4_SW_SW(sum0, sum1, sum2, sum3, sum0, sum1, sum2, sum3);
    PCKEV_H2_SH(sum1, sum0, sum3, sum2, in0, in1);

    LW4(dst, dst_stride, pred0, pred1, pred2, pred3);
    INSERT_W4_SW(pred0, pred1, pred2, pred3, vec);
    ILVRL_B2_SH(zeros, vec, pred_r, pred_l);
    ADD2(pred_r, in0, pred_l, in1, pred_r, pred_l);
    CLIP_SH2_0_255(pred_r, pred_l);
    vec = (v4i32) __msa_pckev_b((v16i8) pred_l, (v16i8) pred_r);
    ST4x4_UB(vec, vec, 0, 1, 2, 3, dst, dst_stride);
}

static void hevc_idct_addpred_8x8_msa(WORD16 *src, UWORD8 *dst,
                                      WORD32 dst_stride)
{
    WORD16 *filter = &gt8x8_cnst[0];
    ULWORD64 pred0, pred1, pred2, pred3;
    v8i16 pred0_r, pred0_l, pred1_r, pred1_l;
    v8i16 in0, in1, in2, in3, in4, in5, in6, in7;
    v8i16 zeros = { 0 };
    v2i64 vec0 = { 0 };
    v2i64 vec1 = { 0 };

    LD_SH8(src, 8, in0, in1, in2, in3, in4, in5, in6, in7);
    HEVC_IDCT8x8_COL(in0, in1, in2, in3, in4, in5, in6, in7, 7, filter);
    TRANSPOSE8x8_SH_SH(in0, in1, in2, in3, in4, in5, in6, in7,
                       in0, in1, in2, in3, in4, in5, in6, in7);
    HEVC_IDCT8x8_COL(in0, in1, in2, in3, in4, in5, in6, in7, 12, filter);
    TRANSPOSE8x8_SH_SH(in0, in1, in2, in3, in4, in5, in6, in7,
                       in0, in1, in2, in3, in4, in5, in6, in7);

    LD4(dst, dst_stride, pred0, pred1, pred2, pred3);
    INSERT_D2_SD(pred0, pred1, vec0);
    INSERT_D2_SD(pred2, pred3, vec1);
    ILVRL_B2_SH(zeros, vec0, pred0_r, pred0_l);
    ILVRL_B2_SH(zeros, vec1, pred1_r, pred1_l);
    ADD4(pred0_r, in0, pred0_l, in1, pred1_r, in2, pred1_l, in3,
         pred0_r, pred0_l, pred1_r, pred1_l);
    CLIP_SH4_0_255(pred0_r, pred0_l, pred1_r, pred1_l);
    PCKEV_B2_SH(pred0_l, pred0_r, pred1_l, pred1_r, pred0_r, pred1_r);
    ST8x4_UB(pred0_r, pred1_r, dst, dst_stride);
    dst += (4 * dst_stride);

    LD4(dst, dst_stride, pred0, pred1, pred2, pred3);
    INSERT_D2_SD(pred0, pred1, vec0);
    INSERT_D2_SD(pred2, pred3, vec1);
    ILVRL_B2_SH(zeros, vec0, pred0_r, pred0_l);
    ILVRL_B2_SH(zeros, vec1, pred1_r, pred1_l);
    ADD4(pred0_r, in4, pred0_l, in5, pred1_r, in6, pred1_l, in7,
         pred0_r, pred0_l, pred1_r, pred1_l);
    CLIP_SH4_0_255(pred0_r, pred0_l, pred1_r, pred1_l);
    PCKEV_B2_SH(pred0_l, pred0_r, pred1_l, pred1_r, pred0_r, pred1_r);
    ST8x4_UB(pred0_r, pred1_r, dst, dst_stride);
}

static void hevc_idct_addpred_16x16_msa(WORD16 *src, UWORD8 *dst,
                                        WORD32 dst_stride)
{
    WORD16 i, j, k;
    WORD16 buf[256];
    WORD16 *buf_ptr = &buf[0];
    WORD16 *src_tmp = src;
    WORD16 *filter = &gt16x16_cnst[0];
    v16u8 pred0, pred1;
    v8i16 in0, in1, in2, in3, in4, in5, in6, in7, in8, in9, in10, in11, in12;
    v8i16 in13, in14, in15, vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7;
    v8i16 out0, out1, out2, out3, out4, out5, out6, out7;
    v8i16 src0_r, src1_r, src2_r, src3_r, src4_r, src5_r, src6_r, src7_r;
    v8i16 src0_l, src1_l, src2_l, src3_l, src4_l, src5_l, src6_l, src7_l;
    v8i16 zeros = { 0 };

    for(i = 2; i--;)
    {
        LD_SH16(src_tmp, 16, in0, in1, in2, in3, in4, in5, in6, in7, in8, in9,
                in10, in11, in12, in13, in14, in15);

        ILVR_H4_SH(in4, in0, in12, in8, in6, in2, in14, in10, src0_r, src1_r,
                   src2_r, src3_r);
        ILVR_H4_SH(in5, in1, in13, in9, in3, in7, in11, in15, src4_r, src5_r,
                   src6_r, src7_r);
        ILVL_H4_SH(in4, in0, in12, in8, in6, in2, in14, in10, src0_l, src1_l,
                   src2_l, src3_l);
        ILVL_H4_SH(in5, in1, in13, in9, in3, in7, in11, in15, src4_l, src5_l,
                   src6_l, src7_l);
        HEVC_IDCT16x16_COL(src0_r, src1_r, src2_r, src3_r, src4_r, src5_r,
                           src6_r, src7_r, src0_l, src1_l, src2_l, src3_l,
                           src4_l, src5_l, src6_l, src7_l, 7, buf_ptr);

        src_tmp += 8;
        buf_ptr = (&buf[0] + 8);
        filter = &gt16x16_cnst[0];
    }

    src_tmp = &buf[0];
    buf_ptr = src;
    filter = &gt16x16_cnst[0];

    for(i = 2; i--;)
    {
        LD_SH16(src_tmp, 8, in0, in8, in1, in9, in2, in10, in3, in11,
                in4, in12, in5, in13, in6, in14, in7, in15);
        TRANSPOSE8x8_SH_SH(in0, in1, in2, in3, in4, in5, in6, in7,
                           in0, in1, in2, in3, in4, in5, in6, in7);
        TRANSPOSE8x8_SH_SH(in8, in9, in10, in11, in12, in13, in14, in15,
                           in8, in9, in10, in11, in12, in13, in14, in15);
        ILVR_H4_SH(in4, in0, in12, in8, in6, in2, in14, in10, src0_r, src1_r,
                   src2_r, src3_r);
        ILVR_H4_SH(in5, in1, in13, in9, in3, in7, in11, in15, src4_r, src5_r,
                   src6_r, src7_r);
        ILVL_H4_SH(in4, in0, in12, in8, in6, in2, in14, in10, src0_l, src1_l,
                   src2_l, src3_l);
        ILVL_H4_SH(in5, in1, in13, in9, in3, in7, in11, in15, src4_l, src5_l,
                   src6_l, src7_l);
        HEVC_IDCT16x16_COL(src0_r, src1_r, src2_r, src3_r, src4_r, src5_r,
                           src6_r, src7_r, src0_l, src1_l, src2_l, src3_l,
                           src4_l, src5_l, src6_l, src7_l, 12, buf_ptr);

        src_tmp += 128;
        buf_ptr = src + 8;
        filter = &gt16x16_cnst[0];
    }

    LD_SH8(src, 16, vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7);
    TRANSPOSE8x8_SH_SH(vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7,
                       vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7);
    LD_SH8((src + 128), 16, out0, out1, out2, out3, out4, out5, out6, out7);
    TRANSPOSE8x8_SH_SH(out0, out1, out2, out3, out4, out5, out6, out7,
                       out0, out1, out2, out3, out4, out5, out6, out7);

    LD_UB2(dst, dst_stride, pred0, pred1);
    ILVR_B2_SH(zeros, pred0, zeros, pred1, in0, in1);
    ILVL_B2_SH(zeros, pred0, zeros, pred1, in2, in3);
    ADD4(vec0, in0, vec1, in1, out0, in2, out1, in3, vec0, vec1, out0, out1);
    CLIP_SH4_0_255(vec0, vec1, out0, out1);
    PCKEV_B2_UB(out0, vec0, out1, vec1, pred0, pred1);
    ST_UB2(pred0, pred1, dst, dst_stride);

    LD_UB2(dst + 2 * dst_stride, dst_stride, pred0, pred1);
    ILVR_B2_SH(zeros, pred0, zeros, pred1, in0, in1);
    ILVL_B2_SH(zeros, pred0, zeros, pred1, in2, in3);
    ADD4(vec2, in0, vec3, in1, out2, in2, out3, in3, vec0, vec1, out0, out1);
    CLIP_SH4_0_255(vec0, vec1, out0, out1);
    PCKEV_B2_UB(out0, vec0, out1, vec1, pred0, pred1);
    ST_UB2(pred0, pred1, dst + 2 * dst_stride, dst_stride);

    LD_UB2(dst + 4 * dst_stride, dst_stride, pred0, pred1);
    ILVR_B2_SH(zeros, pred0, zeros, pred1, in0, in1);
    ILVL_B2_SH(zeros, pred0, zeros, pred1, in2, in3);
    ADD4(vec4, in0, vec5, in1, out4, in2, out5, in3, vec0, vec1, out0, out1);
    CLIP_SH4_0_255(vec0, vec1, out0, out1);
    PCKEV_B2_UB(out0, vec0, out1, vec1, pred0, pred1);
    ST_UB2(pred0, pred1, dst + 4 * dst_stride, dst_stride);

    LD_UB2(dst + 6 * dst_stride, dst_stride, pred0, pred1);
    ILVR_B2_SH(zeros, pred0, zeros, pred1, in0, in1);
    ILVL_B2_SH(zeros, pred0, zeros, pred1, in2, in3);
    ADD4(vec6, in0, vec7, in1, out6, in2, out7, in3, vec0, vec1, out0, out1);
    CLIP_SH4_0_255(vec0, vec1, out0, out1);
    PCKEV_B2_UB(out0, vec0, out1, vec1, pred0, pred1);
    ST_UB2(pred0, pred1, dst + 6 * dst_stride, dst_stride);

    dst += 8 * dst_stride;

    LD_SH8((src + 8), 16, vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7);
    TRANSPOSE8x8_SH_SH(vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7,
                       vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7);
    LD_SH8((src + 136), 16, out0, out1, out2, out3, out4, out5, out6, out7);
    TRANSPOSE8x8_SH_SH(out0, out1, out2, out3, out4, out5, out6, out7,
                       out0, out1, out2, out3, out4, out5, out6, out7);

    LD_UB2(dst, dst_stride, pred0, pred1);
    ILVR_B2_SH(zeros, pred0, zeros, pred1, in0, in1);
    ILVL_B2_SH(zeros, pred0, zeros, pred1, in2, in3);
    ADD4(vec0, in0, vec1, in1, out0, in2, out1, in3, vec0, vec1, out0, out1);
    CLIP_SH4_0_255(vec0, vec1, out0, out1);
    PCKEV_B2_UB(out0, vec0, out1, vec1, pred0, pred1);
    ST_UB2(pred0, pred1, dst, dst_stride);

    LD_UB2(dst + 2 * dst_stride, dst_stride, pred0, pred1);
    ILVR_B2_SH(zeros, pred0, zeros, pred1, in0, in1);
    ILVL_B2_SH(zeros, pred0, zeros, pred1, in2, in3);
    ADD4(vec2, in0, vec3, in1, out2, in2, out3, in3, vec0, vec1, out0, out1);
    CLIP_SH4_0_255(vec0, vec1, out0, out1);
    PCKEV_B2_UB(out0, vec0, out1, vec1, pred0, pred1);
    ST_UB2(pred0, pred1, dst + 2 * dst_stride, dst_stride);

    LD_UB2(dst + 4 * dst_stride, dst_stride, pred0, pred1);
    ILVR_B2_SH(zeros, pred0, zeros, pred1, in0, in1);
    ILVL_B2_SH(zeros, pred0, zeros, pred1, in2, in3);
    ADD4(vec4, in0, vec5, in1, out4, in2, out5, in3, vec0, vec1, out0, out1);
    CLIP_SH4_0_255(vec0, vec1, out0, out1);
    PCKEV_B2_UB(out0, vec0, out1, vec1, pred0, pred1);
    ST_UB2(pred0, pred1, dst + 4 * dst_stride, dst_stride);

    LD_UB2(dst + 6 * dst_stride, dst_stride, pred0, pred1);
    ILVR_B2_SH(zeros, pred0, zeros, pred1, in0, in1);
    ILVL_B2_SH(zeros, pred0, zeros, pred1, in2, in3);
    ADD4(vec6, in0, vec7, in1, out6, in2, out7, in3, vec0, vec1, out0, out1);
    CLIP_SH4_0_255(vec0, vec1, out0, out1);
    PCKEV_B2_UB(out0, vec0, out1, vec1, pred0, pred1);
    ST_UB2(pred0, pred1, dst + 6 * dst_stride, dst_stride);
}

static void hevc_idct_transpose_8x32_to_32x8_addpred(WORD16 *tmp_buf,
                                                     UWORD8 *dst,
                                                     WORD32 dst_stride)
{
    UWORD8 i;
    v16u8 pred0, pred1;
    v8i16 in0, in1, in2, in3, out0, out1, out2, out3, out4, out5, out6, out7;
    v8i16 vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7;
    v8i16 zeros = { 0 };

    for(i = 0; i < 2; i++)
    {
        LD_SH8(tmp_buf + i * 128, 8, vec0, vec1, vec2, vec3, vec4, vec5, vec6,
               vec7);
        TRANSPOSE8x8_SH_SH(vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7,
                           vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7);

        LD_SH8(tmp_buf + (i * 2 + 1) * 64, 8, out0, out1, out2, out3, out4,
               out5, out6, out7);
        TRANSPOSE8x8_SH_SH(out0, out1, out2, out3, out4, out5, out6, out7,
                           out0, out1, out2, out3, out4, out5, out6, out7);

        LD_UB2(dst, dst_stride, pred0, pred1);
        ILVR_B2_SH(zeros, pred0, zeros, pred1, in0, in1);
        ILVL_B2_SH(zeros, pred0, zeros, pred1, in2, in3);
        ADD2(vec0, in0, vec1, in1, vec0, vec1);
        ADD2(out0, in2, out1, in3, out0, out1);
        CLIP_SH4_0_255(vec0, vec1, out0, out1);
        PCKEV_B2_UB(out0, vec0, out1, vec1, pred0, pred1);
        ST_UB2(pred0, pred1, dst, dst_stride);

        LD_UB2(dst + 2 * dst_stride, dst_stride, pred0, pred1);
        ILVR_B2_SH(zeros, pred0, zeros, pred1, in0, in1);
        ILVL_B2_SH(zeros, pred0, zeros, pred1, in2, in3);
        ADD2(vec2, in0, vec3, in1, vec0, vec1);
        ADD2(out2, in2, out3, in3, out0, out1);
        CLIP_SH4_0_255(vec0, vec1, out0, out1);
        PCKEV_B2_UB(out0, vec0, out1, vec1, pred0, pred1);
        ST_UB2(pred0, pred1, dst + 2 * dst_stride, dst_stride);

        LD_UB2(dst + 4 * dst_stride, dst_stride, pred0, pred1);
        ILVR_B2_SH(zeros, pred0, zeros, pred1, in0, in1);
        ILVL_B2_SH(zeros, pred0, zeros, pred1, in2, in3);
        ADD2(vec4, in0, vec5, in1, vec0, vec1);
        ADD2(out4, in2, out5, in3, out0, out1);
        CLIP_SH4_0_255(vec0, vec1, out0, out1);
        PCKEV_B2_UB(out0, vec0, out1, vec1, pred0, pred1);
        ST_UB2(pred0, pred1, dst + 4 * dst_stride, dst_stride);

        LD_UB2(dst + 6 * dst_stride, dst_stride, pred0, pred1);
        ILVR_B2_SH(zeros, pred0, zeros, pred1, in0, in1);
        ILVL_B2_SH(zeros, pred0, zeros, pred1, in2, in3);
        ADD2(vec6, in0, vec7, in1, vec0, vec1);
        ADD2(out6, in2, out7, in3, out0, out1);
        CLIP_SH4_0_255(vec0, vec1, out0, out1);
        PCKEV_B2_UB(out0, vec0, out1, vec1, pred0, pred1);
        ST_UB2(pred0, pred1, dst + 6 * dst_stride, dst_stride);

        dst += 16;
    }
}

static void hevc_idct_addpred_32x32_msa(WORD16 *src, UWORD8 *dst,
                                        WORD32 dst_stride)
{
    UWORD8 row, col;
    WORD16 *src_tmp = src;
    WORD16 tmp_buf[8 * 32];
    WORD16 *tmp_buf_ptr = &tmp_buf[0];

    /* column transform */
    for(col = 0; col < 4; col++)
    {
        /* process 8x32 blocks */
        hevc_idct_8x32_column_msa((src + col * 8), 32, 7);
    }

    /* row transform */
    for(row = 0; row < 4; row++)
    {
        /* process 32x8 blocks */
        src_tmp = (src + 256 * row);

        hevc_idct_transpose_32x8_to_8x32(src_tmp, tmp_buf_ptr);
        hevc_idct_8x32_column_msa(tmp_buf_ptr, 8, 12);
        hevc_idct_transpose_8x32_to_32x8_addpred(tmp_buf_ptr,
                                                 dst + 8 * row * dst_stride,
                                                 dst_stride);
    }
}

static void hevc_idct_addpred_luma_4x4_msa(WORD16 *src, UWORD8 *dst,
                                           WORD32 dst_stride)
{
    UWORD32 pred0, pred1, pred2, pred3;
    v8i16 pred_r, pred_l, in0, in1, dst0, dst1, zeros = { 0 };
    v4i32 in0_r, in0_l, in1_r, in1_l, res0, res1, res2, res3, vec = { 0 };

    LD_SH2(src, 8, in0, in1);
    UNPCK_SH_SW(in0, in0_r, in0_l);
    UNPCK_SH_SW(in1, in1_r, in1_l);
    HEVC_IDCT_LUMA4x4_COL(in0_r, in0_l, in1_r, in1_l, res0, res1, res2, res3,
                          7);
    TRANSPOSE4x4_SW_SW(res0, res1, res2, res3, in0_r, in0_l, in1_r, in1_l);
    HEVC_IDCT_LUMA4x4_COL(in0_r, in0_l, in1_r, in1_l, res0, res1, res2, res3,
                          12);
    TRANSPOSE4x4_SW_SW(res0, res1, res2, res3, res0, res1, res2, res3);
    PCKEV_H2_SH(res1, res0, res3, res2, dst0, dst1);

    LW4(dst, dst_stride, pred0, pred1, pred2, pred3);
    INSERT_W4_SW(pred0, pred1, pred2, pred3, vec);
    ILVRL_B2_SH(zeros, vec, pred_r, pred_l);
    ADD2(pred_r, dst0, pred_l, dst1, pred_r, pred_l);
    CLIP_SH2_0_255(pred_r, pred_l);
    vec = (v4i32) __msa_pckev_b((v16i8) pred_l, (v16i8) pred_r);
    ST4x4_UB(vec, vec, 0, 1, 2, 3, dst, dst_stride);

}

static void hevc_chroma_idct_addpred_4x4_msa(WORD16 *src, UWORD8 *dst,
                                             WORD32 dst_stride)

{
    ULWORD64 pred0, pred1, pred2, pred3;
    v4i32 in0_r, in0_l, in1_r, in1_l, sum0, sum1, sum2, sum3;
    v8i16 in0, in1, dst0, dst1;
    v8i16 zeros = { 0 };
    v16i8 vec0 = { 0 };
    v16i8 vec1 = { 0 };
    v16i8 mask = { 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0,
                   0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0 };
    v16i8 mask1 = { 0, 17, 2, 19, 4, 21, 6, 23, 8, 25, 10, 27, 12, 29, 14, 31 };

    LD_SH2(src, 8, in0, in1);
    ILVRL_H2_SW(zeros, in0, in0_r, in0_l);
    ILVRL_H2_SW(zeros, in1, in1_r, in1_l);

    HEVC_IDCT4x4_COL(in0_r, in0_l, in1_r, in1_l, sum0, sum1, sum2, sum3, 7);
    TRANSPOSE4x4_SW_SW(sum0, sum1, sum2, sum3, in0_r, in0_l, in1_r, in1_l);
    HEVC_IDCT4x4_COL(in0_r, in0_l, in1_r, in1_l, sum0, sum1, sum2, sum3, 12);
    TRANSPOSE4x4_SW_SW(sum0, sum1, sum2, sum3, sum0, sum1, sum2, sum3);
    PCKEV_H2_SH(sum1, sum0, sum3, sum2, in0, in1);

    LD4(dst, dst_stride, pred0, pred1, pred2, pred3);
    INSERT_D2_SB(pred0, pred1, vec0);
    INSERT_D2_SB(pred2, pred3, vec1);
    dst0 = (v8i16) (vec0 & mask);
    dst1 = (v8i16) (vec1 & mask);
    ADD2(dst0, in0, dst1, in1, dst0, dst1);
    CLIP_SH2_0_255(dst0, dst1);
    VSHF_B2_SB(dst0, vec0, dst1, vec1, mask1, mask1, vec0, vec1);
    ST8x4_UB(vec0, vec1, dst, dst_stride);
}

static void hevc_chroma_idct_addpred_8x8_msa(WORD16 *src,
                                             UWORD8 *dst, WORD32 dst_stride)
{
    WORD16 *filter = &gt8x8_cnst[0];
    v16i8 pred0, pred1, pred2, pred3, pred4, pred5, pred6, pred7;
    v8i16 in0, in1, in2, in3, in4, in5, in6, in7;
    v8i16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v16i8 mask = { 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0,
                   0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0 };
    v16i8 mask1 = { 0, 17, 2, 19, 4, 21, 6, 23, 8, 25, 10, 27, 12, 29, 14, 31 };

    LD_SH8(src, 8, in0, in1, in2, in3, in4, in5, in6, in7);
    HEVC_IDCT8x8_COL(in0, in1, in2, in3, in4, in5, in6, in7, 7, filter);
    TRANSPOSE8x8_SH_SH(in0, in1, in2, in3, in4, in5, in6, in7,
                       in0, in1, in2, in3, in4, in5, in6, in7);
    HEVC_IDCT8x8_COL(in0, in1, in2, in3, in4, in5, in6, in7, 12, filter);
    TRANSPOSE8x8_SH_SH(in0, in1, in2, in3, in4, in5, in6, in7,
                       in0, in1, in2, in3, in4, in5, in6, in7);
    LD_SB8(dst, dst_stride, pred0, pred1, pred2, pred3, pred4, pred5, pred6,
           pred7);

    dst0 = (v8i16) (pred0 & mask);
    dst1 = (v8i16) (pred1 & mask);
    dst2 = (v8i16) (pred2 & mask);
    dst3 = (v8i16) (pred3 & mask);
    dst4 = (v8i16) (pred4 & mask);
    dst5 = (v8i16) (pred5 & mask);
    dst6 = (v8i16) (pred6 & mask);
    dst7 = (v8i16) (pred7 & mask);

    ADD4(dst0, in0, dst1, in1, dst2, in2, dst3, in3, dst0, dst1, dst2, dst3);
    ADD4(dst4, in4, dst5, in5, dst6, in6, dst7, in7, dst4, dst5, dst6, dst7);
    CLIP_SH4_0_255(dst0, dst1, dst2, dst3);
    CLIP_SH4_0_255(dst4, dst5, dst6, dst7);
    VSHF_B2_SB(dst0, pred0, dst1, pred1, mask1, mask1, pred0, pred1);
    VSHF_B2_SB(dst2, pred2, dst3, pred3, mask1, mask1, pred2, pred3);
    VSHF_B2_SB(dst4, pred4, dst5, pred5, mask1, mask1, pred4, pred5);
    VSHF_B2_SB(dst6, pred6, dst7, pred7, mask1, mask1, pred6, pred7);
    ST_SB4(pred0, pred1, pred2, pred3, dst, dst_stride);
    ST_SB4(pred4, pred5, pred6, pred7, dst + 4 * dst_stride, dst_stride);
}

static void hevc_chroma_idct_addpred_16x16_msa(WORD16 *src, UWORD8 *dst,
                                               WORD32 dst_stride)
{
    WORD16 i, j, k;
    WORD16 buf[256];
    WORD16 *buf_ptr = &buf[0];
    WORD16 *src_tmp = src;
    WORD16 *filter = &gt16x16_cnst[0];
    v16i8 pred0, pred1, pred2, pred3;
    v8i16 in0, in1, in2, in3, in4, in5, in6, in7, in8, in9, in10, in11, in12;
    v8i16 in13, in14, in15, vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7;
    v8i16 out0, out1, out2, out3, out4, out5, out6, out7;
    v8i16 src0_r, src1_r, src2_r, src3_r, src4_r, src5_r, src6_r, src7_r;
    v8i16 src0_l, src1_l, src2_l, src3_l, src4_l, src5_l, src6_l, src7_l;
    v8i16 dst0, dst1, dst2, dst3;
    v16i8 mask = { 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0,
                   0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0 };
    v16i8 mask1 = { 0, 17, 2, 19, 4, 21, 6, 23, 8, 25, 10, 27, 12, 29, 14, 31 };

    for(i = 2; i--;)
    {
        LD_SH16(src_tmp, 16, in0, in1, in2, in3, in4, in5, in6, in7,
                in8, in9, in10, in11, in12, in13, in14, in15);

        ILVR_H4_SH(in4, in0, in12, in8, in6, in2, in14, in10, src0_r, src1_r,
                   src2_r, src3_r);
        ILVR_H4_SH(in5, in1, in13, in9, in3, in7, in11, in15, src4_r, src5_r,
                   src6_r, src7_r);
        ILVL_H4_SH(in4, in0, in12, in8, in6, in2, in14, in10, src0_l, src1_l,
                   src2_l, src3_l);
        ILVL_H4_SH(in5, in1, in13, in9, in3, in7, in11, in15, src4_l, src5_l,
                   src6_l, src7_l);
        HEVC_IDCT16x16_COL(src0_r, src1_r, src2_r, src3_r, src4_r, src5_r,
                           src6_r, src7_r, src0_l, src1_l, src2_l, src3_l,
                           src4_l, src5_l, src6_l, src7_l, 7, buf_ptr);

        src_tmp += 8;
        buf_ptr = (&buf[0] + 8);
        filter = &gt16x16_cnst[0];
    }

    src_tmp = &buf[0];
    buf_ptr = src;
    filter = &gt16x16_cnst[0];

    for(i = 2; i--;)
    {
        LD_SH16(src_tmp, 8, in0, in8, in1, in9, in2, in10, in3, in11,
                in4, in12, in5, in13, in6, in14, in7, in15);
        TRANSPOSE8x8_SH_SH(in0, in1, in2, in3, in4, in5, in6, in7,
                           in0, in1, in2, in3, in4, in5, in6, in7);
        TRANSPOSE8x8_SH_SH(in8, in9, in10, in11, in12, in13, in14, in15,
                           in8, in9, in10, in11, in12, in13, in14, in15);
        ILVR_H4_SH(in4, in0, in12, in8, in6, in2, in14, in10, src0_r, src1_r,
                   src2_r, src3_r);
        ILVR_H4_SH(in5, in1, in13, in9, in3, in7, in11, in15, src4_r, src5_r,
                   src6_r, src7_r);
        ILVL_H4_SH(in4, in0, in12, in8, in6, in2, in14, in10, src0_l, src1_l,
                   src2_l, src3_l);
        ILVL_H4_SH(in5, in1, in13, in9, in3, in7, in11, in15, src4_l, src5_l,
                   src6_l, src7_l);
        HEVC_IDCT16x16_COL(src0_r, src1_r, src2_r, src3_r, src4_r, src5_r,
                           src6_r, src7_r, src0_l, src1_l, src2_l, src3_l,
                           src4_l, src5_l, src6_l, src7_l, 12, buf_ptr);

        src_tmp += 128;
        buf_ptr = src + 8;
        filter = &gt16x16_cnst[0];
    }

    LD_SH8(src, 16, vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7);
    TRANSPOSE8x8_SH_SH(vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7,
                       vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7);
    LD_SH8((src + 128), 16, out0, out1, out2, out3, out4, out5, out6, out7);
    TRANSPOSE8x8_SH_SH(out0, out1, out2, out3, out4, out5, out6, out7,
                       out0, out1, out2, out3, out4, out5, out6, out7);

    LD_SB2(dst, 16, pred0, pred1);
    LD_SB2(dst + dst_stride, 16, pred2, pred3);
    dst0 = (v8i16) (pred0 & mask);
    dst1 = (v8i16) (pred1 & mask);
    dst2 = (v8i16) (pred2 & mask);
    dst3 = (v8i16) (pred3 & mask);
    ADD4(vec0, dst0, out0, dst1, vec1, dst2, out1, dst3, dst0, dst1, dst2,
         dst3);
    CLIP_SH4_0_255(dst0, dst1, dst2, dst3);
    VSHF_B2_SB(dst0, pred0, dst1, pred1, mask1, mask1, pred0, pred1);
    VSHF_B2_SB(dst2, pred2, dst3, pred3, mask1, mask1, pred2, pred3);
    ST_SB2(pred0, pred1, dst, 16);
    ST_SB2(pred2, pred3, dst + dst_stride, 16);

    LD_SB2(dst + 2 * dst_stride, 16, pred0, pred1);
    LD_SB2(dst + 3 * dst_stride, 16, pred2, pred3);
    dst0 = (v8i16) (pred0 & mask);
    dst1 = (v8i16) (pred1 & mask);
    dst2 = (v8i16) (pred2 & mask);
    dst3 = (v8i16) (pred3 & mask);
    ADD4(vec2, dst0, out2, dst1, vec3, dst2, out3, dst3, dst0, dst1, dst2,
         dst3);
    CLIP_SH4_0_255(dst0, dst1, dst2, dst3);
    VSHF_B2_SB(dst0, pred0, dst1, pred1, mask1, mask1, pred0, pred1);
    VSHF_B2_SB(dst2, pred2, dst3, pred3, mask1, mask1, pred2, pred3);
    ST_SB2(pred0, pred1, dst + 2 * dst_stride, 16);
    ST_SB2(pred2, pred3, dst + 3 * dst_stride, 16);

    LD_SB2(dst + 4 * dst_stride, 16, pred0, pred1);
    LD_SB2(dst + 5 * dst_stride, 16, pred2, pred3);
    dst0 = (v8i16) (pred0 & mask);
    dst1 = (v8i16) (pred1 & mask);
    dst2 = (v8i16) (pred2 & mask);
    dst3 = (v8i16) (pred3 & mask);
    ADD2(vec4, dst0, out4, dst1, dst0, dst1);
    ADD2(vec5, dst2, out5, dst3, dst2, dst3);
    CLIP_SH4_0_255(dst0, dst1, dst2, dst3);
    VSHF_B2_SB(dst0, pred0, dst1, pred1, mask1, mask1, pred0, pred1);
    VSHF_B2_SB(dst2, pred2, dst3, pred3, mask1, mask1, pred2, pred3);
    ST_SB2(pred0, pred1, dst + 4 * dst_stride, 16);
    ST_SB2(pred2, pred3, dst + 5 * dst_stride, 16);

    LD_SB2(dst + 6 * dst_stride, 16, pred0, pred1);
    LD_SB2(dst + 7 * dst_stride, 16, pred2, pred3);
    dst0 = (v8i16) (pred0 & mask);
    dst1 = (v8i16) (pred1 & mask);
    dst2 = (v8i16) (pred2 & mask);
    dst3 = (v8i16) (pred3 & mask);
    ADD2(vec6, dst0, out6, dst1, dst0, dst1);
    ADD2(vec7, dst2, out7, dst3, dst2, dst3);
    CLIP_SH4_0_255(dst0, dst1, dst2, dst3);
    VSHF_B2_SB(dst0, pred0, dst1, pred1, mask1, mask1, pred0, pred1);
    VSHF_B2_SB(dst2, pred2, dst3, pred3, mask1, mask1, pred2, pred3);
    ST_SB2(pred0, pred1, dst + 6 * dst_stride, 16);
    ST_SB2(pred2, pred3, dst + 7 * dst_stride, 16);

    dst += 8 * dst_stride;

    LD_SH8((src + 8), 16, vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7);
    TRANSPOSE8x8_SH_SH(vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7,
                       vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7);
    LD_SH8((src + 136), 16, out0, out1, out2, out3, out4, out5, out6, out7);
    TRANSPOSE8x8_SH_SH(out0, out1, out2, out3, out4, out5, out6, out7,
                       out0, out1, out2, out3, out4, out5, out6, out7);

    LD_SB2(dst, 16, pred0, pred1);
    LD_SB2(dst + dst_stride, 16, pred2, pred3);
    dst0 = (v8i16) (pred0 & mask);
    dst1 = (v8i16) (pred1 & mask);
    dst2 = (v8i16) (pred2 & mask);
    dst3 = (v8i16) (pred3 & mask);
    ADD2(vec0, dst0, out0, dst1, dst0, dst1);
    ADD2(vec1, dst2, out1, dst3, dst2, dst3);
    CLIP_SH4_0_255(dst0, dst1, dst2, dst3);
    VSHF_B2_SB(dst0, pred0, dst1, pred1, mask1, mask1, pred0, pred1);
    VSHF_B2_SB(dst2, pred2, dst3, pred3, mask1, mask1, pred2, pred3);
    ST_SB2(pred0, pred1, dst, 16);
    ST_SB2(pred2, pred3, dst + dst_stride, 16);

    LD_SB2(dst + 2 * dst_stride, 16, pred0, pred1);
    LD_SB2(dst + 3 * dst_stride, 16, pred2, pred3);
    dst0 = (v8i16) (pred0 & mask);
    dst1 = (v8i16) (pred1 & mask);
    dst2 = (v8i16) (pred2 & mask);
    dst3 = (v8i16) (pred3 & mask);
    ADD2(vec2, dst0, out2, dst1, dst0, dst1);
    ADD2(vec3, dst2, out3, dst3, dst2, dst3);
    CLIP_SH4_0_255(dst0, dst1, dst2, dst3);
    VSHF_B2_SB(dst0, pred0, dst1, pred1, mask1, mask1, pred0, pred1);
    VSHF_B2_SB(dst2, pred2, dst3, pred3, mask1, mask1, pred2, pred3);
    ST_SB2(pred0, pred1, dst + 2 * dst_stride, 16);
    ST_SB2(pred2, pred3, dst + 3 * dst_stride, 16);

    LD_SB2(dst + 4 * dst_stride, 16, pred0, pred1);
    LD_SB2(dst + 5 * dst_stride, 16, pred2, pred3);
    dst0 = (v8i16) (pred0 & mask);
    dst1 = (v8i16) (pred1 & mask);
    dst2 = (v8i16) (pred2 & mask);
    dst3 = (v8i16) (pred3 & mask);
    ADD2(vec4, dst0, out4, dst1, dst0, dst1);
    ADD2(vec5, dst2, out5, dst3, dst2, dst3);
    CLIP_SH4_0_255(dst0, dst1, dst2, dst3);
    VSHF_B2_SB(dst0, pred0, dst1, pred1, mask1, mask1, pred0, pred1);
    VSHF_B2_SB(dst2, pred2, dst3, pred3, mask1, mask1, pred2, pred3);
    ST_SB2(pred0, pred1, dst + 4 * dst_stride, 16);
    ST_SB2(pred2, pred3, dst + 5 * dst_stride, 16);

    LD_SB2(dst + 6 * dst_stride, 16, pred0, pred1);
    LD_SB2(dst + 7 * dst_stride, 16, pred2, pred3);
    dst0 = (v8i16) (pred0 & mask);
    dst1 = (v8i16) (pred1 & mask);
    dst2 = (v8i16) (pred2 & mask);
    dst3 = (v8i16) (pred3 & mask);
    ADD2(vec6, dst0, out6, dst1, dst0, dst1);
    ADD2(vec7, dst2, out7, dst3, dst2, dst3);
    CLIP_SH4_0_255(dst0, dst1, dst2, dst3);
    VSHF_B2_SB(dst0, pred0, dst1, pred1, mask1, mask1, pred0, pred1);
    VSHF_B2_SB(dst2, pred2, dst3, pred3, mask1, mask1, pred2, pred3);
    ST_SB2(pred0, pred1, dst + 6 * dst_stride, 16);
    ST_SB2(pred2, pred3, dst + 7 * dst_stride, 16);
}

void ihevc_itrans_recon_4x4_ttype1_msa(WORD16 *src, WORD16 *tmp, UWORD8 *pred,
                                       UWORD8 *dst, WORD32 src_stride,
                                       WORD32 pred_stride, WORD32 dst_stride,
                                       WORD32 zero_cols, WORD32 zero_rows)

{
    UNUSED(tmp);
    UNUSED(pred);
    UNUSED(src_stride);
    UNUSED(pred_stride);
    UNUSED(zero_cols);
    UNUSED(zero_rows);

    hevc_idct_addpred_luma_4x4_msa(src, dst, dst_stride);
}

void ihevc_itrans_recon_4x4_msa(WORD16 *src, WORD16 *tmp, UWORD8 *pred,
                                UWORD8 *dst, WORD32 src_stride,
                                WORD32 pred_stride, WORD32 dst_stride,
                                WORD32 zero_cols, WORD32 zero_rows)

{
    UNUSED(tmp);
    UNUSED(pred);
    UNUSED(src_stride);
    UNUSED(pred_stride);
    UNUSED(zero_cols);
    UNUSED(zero_rows);

    hevc_idct_addpred_4x4_msa(src, dst, dst_stride);
}

void ihevc_itrans_recon_8x8_msa(WORD16 *src, WORD16 *tmp, UWORD8 *pred,
                                UWORD8 *dst, WORD32 src_stride,
                                WORD32 pred_stride, WORD32 dst_stride,
                                WORD32 zero_cols, WORD32 zero_rows)

{
    UNUSED(tmp);
    UNUSED(pred);
    UNUSED(src_stride);
    UNUSED(pred_stride);
    UNUSED(zero_cols);
    UNUSED(zero_rows);

    hevc_idct_addpred_8x8_msa(src, dst, dst_stride);
}

void ihevc_itrans_recon_16x16_msa(WORD16 *src, WORD16 *tmp, UWORD8 *pred,
                                  UWORD8 *dst, WORD32 src_stride,
                                  WORD32 pred_stride, WORD32 dst_stride,
                                  WORD32 zero_cols, WORD32 zero_rows)

{
    UNUSED(tmp);
    UNUSED(pred);
    UNUSED(src_stride);
    UNUSED(pred_stride);
    UNUSED(zero_cols);
    UNUSED(zero_rows);

    hevc_idct_addpred_16x16_msa(src, dst, dst_stride);
}

void ihevc_itrans_recon_32x32_msa(WORD16 *src, WORD16 *tmp, UWORD8 *pred,
                                  UWORD8 *dst, WORD32 src_stride,
                                  WORD32 pred_stride, WORD32 dst_stride,
                                  WORD32 zero_cols, WORD32 zero_rows)

{
    UNUSED(tmp);
    UNUSED(pred);
    UNUSED(src_stride);
    UNUSED(pred_stride);
    UNUSED(zero_cols);
    UNUSED(zero_rows);

    hevc_idct_addpred_32x32_msa(src, dst, dst_stride);
}

void ihevc_chroma_itrans_recon_4x4_msa(WORD16 *src, WORD16 *tmp, UWORD8 *pred,
                                       UWORD8 *dst, WORD32 src_stride,
                                       WORD32 pred_stride, WORD32 dst_stride,
                                       WORD32 zero_cols, WORD32 zero_rows)
{
    UNUSED(tmp);
    UNUSED(pred);
    UNUSED(src_stride);
    UNUSED(pred_stride);
    UNUSED(zero_cols);
    UNUSED(zero_rows);

    hevc_chroma_idct_addpred_4x4_msa(src, dst, dst_stride);
}

void ihevc_chroma_itrans_recon_8x8_msa(WORD16 *src, WORD16 *tmp, UWORD8 *pred,
                                       UWORD8 *dst, WORD32 src_stride,
                                       WORD32 pred_stride, WORD32 dst_stride,
                                       WORD32 zero_cols, WORD32 zero_rows)
{
    UNUSED(tmp);
    UNUSED(pred);
    UNUSED(src_stride);
    UNUSED(pred_stride);
    UNUSED(zero_cols);
    UNUSED(zero_rows);

    hevc_chroma_idct_addpred_8x8_msa(src, dst, dst_stride);
}

void ihevc_chroma_itrans_recon_16x16_msa(WORD16 *src, WORD16 *tmp, UWORD8 *pred,
                                         UWORD8 *dst, WORD32 src_stride,
                                         WORD32 pred_stride, WORD32 dst_stride,
                                         WORD32 zero_cols, WORD32 zero_rows)
{
    UNUSED(tmp);
    UNUSED(pred);
    UNUSED(src_stride);
    UNUSED(pred_stride);
    UNUSED(zero_cols);
    UNUSED(zero_rows);

    hevc_chroma_idct_addpred_16x16_msa(src, dst, dst_stride);
}
