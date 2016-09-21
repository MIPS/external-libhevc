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
#include "ihevc_defs.h"
#include "ihevc_macros.h"
#include "ihevc_inter_pred.h"
#include "ihevc_macros_msa.h"

#define FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1)           \
( {                                                             \
    v8i16 tmp0;                                                 \
                                                                \
    tmp0 = __msa_dotp_s_h((v16i8) vec0, (v16i8) filt0);         \
    tmp0 = __msa_dpadd_s_h(tmp0, (v16i8) vec1, (v16i8) filt1);  \
                                                                \
    tmp0;                                                       \
} )

#define HEVC_FILT_4TAP(in0, in1, filt0, filt1)           \
( {                                                      \
    v4i32 out_m;                                         \
                                                         \
    out_m = __msa_dotp_s_w(in0, (v8i16) filt0);          \
    out_m = __msa_dpadd_s_w(out_m, in1, (v8i16) filt1);  \
    out_m;                                               \
} )

#define HORIZ_4TAP_4WID_4VECS_FILT(src0, src1, src2, src3, mask0,      \
                                   mask1, filt0, filt1, out0, out1)    \
{                                                                      \
    v16i8 vec0_m, vec1_m, vec2_m, vec3_m;                              \
                                                                       \
    VSHF_B2_SB(src0, src1, src2, src3, mask0, mask0, vec0_m, vec1_m);  \
    DOTP_SB2_SH(vec0_m, vec1_m, filt0, filt0, out0, out1);             \
    VSHF_B2_SB(src0, src1, src2, src3, mask1, mask1, vec2_m, vec3_m);  \
    DPADD_SB2_SH(vec2_m, vec3_m, filt1, filt1, out0, out1);            \
}

#define HORIZ_4TAP_8WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1,  \
                                   filt0, filt1, out0, out1, out2, out3)  \
{                                                                         \
    v16i8 vec0_m, vec1_m, vec2_m, vec3_m;                                 \
                                                                          \
    VSHF_B2_SB(src0, src0, src1, src1, mask0, mask0, vec0_m, vec1_m);     \
    VSHF_B2_SB(src2, src2, src3, src3, mask0, mask0, vec2_m, vec3_m);     \
    DOTP_SB4_SH(vec0_m, vec1_m, vec2_m, vec3_m, filt0, filt0, filt0,      \
                filt0, out0, out1, out2, out3);                           \
    VSHF_B2_SB(src0, src0, src1, src1, mask1, mask1, vec0_m, vec1_m);     \
    VSHF_B2_SB(src2, src2, src3, src3, mask1, mask1, vec2_m, vec3_m);     \
    DPADD_SB4_SH(vec0_m, vec1_m, vec2_m, vec3_m, filt1, filt1, filt1,     \
                 filt1, out0, out1, out2, out3);                          \
}

static void hevc_copy_4w_msa(UWORD8 *src, WORD32 src_stride,
                             WORD16 *dst, WORD32 dst_stride,
                             WORD32 height)
{
    v16i8 zero = { 0 };

    if(2 == height)
    {
        v16i8 src0, src1;
        v8i16 in0;

        LD_SB2(src, src_stride, src0, src1);

        src0 = (v16i8) __msa_ilvr_w((v4i32) src1, (v4i32) src0);
        in0 = (v8i16) __msa_ilvr_b(zero, src0);
        in0 = SLLI_H(in0, 6);
        ST8x2_UB(in0, dst, 2 * dst_stride);
    }
    else if(4 == height)
    {
        v16i8 src0, src1, src2, src3;
        v8i16 in0, in1;

        LD_SB4(src, src_stride, src0, src1, src2, src3);

        ILVR_W2_SB(src1, src0, src3, src2, src0, src1);
        ILVR_B2_SH(zero, src0, zero, src1, in0, in1);
        SLLI_H2_SH(in0, in1, 6);
        ST8x4_UB(in0, in1, dst, 2 * dst_stride);
    }
    else if(0 == height % 8)
    {
        v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
        v8i16 in0, in1, in2, in3;
        UWORD32 loop_cnt;

        for(loop_cnt = (height >> 3); loop_cnt--;)
        {
            LD_SB8(src, src_stride,
                   src0, src1, src2, src3, src4, src5, src6, src7);
            src += (8 * src_stride);

            ILVR_W4_SB(src1, src0, src3, src2, src5, src4, src7, src6,
                       src0, src1, src2, src3);
            ILVR_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3,
                       in0, in1, in2, in3);
            SLLI_H4_SH(in0, in1, in2, in3, 6);
            ST8x8_UB(in0, in1, in2, in3, dst, 2 * dst_stride);
            dst += (8 * dst_stride);
        }
    }
}

static void hevc_copy_8w_msa(UWORD8 *src, WORD32 src_stride,
                             WORD16 *dst, WORD32 dst_stride,
                             WORD32 height)
{
    v16i8 zero = { 0 };

    if(2 == height)
    {
        v16i8 src0, src1;
        v8i16 in0, in1;

        LD_SB2(src, src_stride, src0, src1);
        ILVR_B2_SH(zero, src0, zero, src1, in0, in1);
        SLLI_H2_SH(in0, in1, 6);
        ST_SH2(in0, in1, dst, dst_stride);
    }
    else if(4 == height)
    {
        v16i8 src0, src1, src2, src3;
        v8i16 in0, in1, in2, in3;

        LD_SB4(src, src_stride, src0, src1, src2, src3);
        ILVR_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3,
                   in0, in1, in2, in3);
        SLLI_H4_SH(in0, in1, in2, in3, 6);
        ST_SH4(in0, in1, in2, in3, dst, dst_stride);
    }
    else if(6 == height)
    {
        v16i8 src0, src1, src2, src3, src4, src5;
        v8i16 in0, in1, in2, in3, in4, in5;

        LD_SB6(src, src_stride, src0, src1, src2, src3, src4, src5);
        ILVR_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3,
                   in0, in1, in2, in3);
        ILVR_B2_SH(zero, src4, zero, src5, in4, in5);
        SLLI_H4_SH(in0, in1, in2, in3, 6);
        SLLI_H2_SH(in4, in5, 6);
        ST_SH6(in0, in1, in2, in3, in4, in5, dst, dst_stride);
    }
    else if(0 == height % 8)
    {
        UWORD32 loop_cnt;
        v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
        v8i16 in0, in1, in2, in3, in4, in5, in6, in7;

        for(loop_cnt = (height >> 3); loop_cnt--;)
        {
            LD_SB8(src, src_stride,
                   src0, src1, src2, src3, src4, src5, src6, src7);
            src += (8 * src_stride);

            ILVR_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3,
                       in0, in1, in2, in3);
            ILVR_B4_SH(zero, src4, zero, src5, zero, src6, zero, src7,
                       in4, in5, in6, in7);
            SLLI_H4_SH(in0, in1, in2, in3, 6);
            SLLI_H4_SH(in4, in5, in6, in7, 6);
            ST_SH8(in0, in1, in2, in3, in4, in5, in6, in7, dst, dst_stride);
            dst += (8 * dst_stride);
        }
    }
}

static void hevc_copy_12w_msa(UWORD8 *src, WORD32 src_stride,
                              WORD16 *dst, WORD32 dst_stride,
                              WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 zero = { 0 };
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v8i16 in0, in1, in0_r, in1_r, in2_r, in3_r;

    for(loop_cnt = (height >> 3); loop_cnt--;)
    {
        LD_SB8(src, src_stride, src0, src1, src2, src3, src4, src5, src6, src7);
        src += (8 * src_stride);

        ILVR_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3,
                   in0_r, in1_r, in2_r, in3_r);
        SLLI_H4_SH(in0_r, in1_r, in2_r, in3_r, 6);
        ILVL_W2_SB(src1, src0, src3, src2, src0, src1);
        ILVR_B2_SH(zero, src0, zero, src1, in0, in1);
        SLLI_H2_SH(in0, in1, 6);
        ST_SH4(in0_r, in1_r, in2_r, in3_r, dst, dst_stride);
        ST8x4_UB(in0, in1, dst + 8, 2 * dst_stride);
        dst += (4 * dst_stride);

        ILVR_B4_SH(zero, src4, zero, src5, zero, src6, zero, src7,
                   in0_r, in1_r, in2_r, in3_r);
        SLLI_H4_SH(in0_r, in1_r, in2_r, in3_r, 6);
        ILVL_W2_SB(src5, src4, src7, src6, src0, src1);
        ILVR_B2_SH(zero, src0, zero, src1, in0, in1);
        SLLI_H2_SH(in0, in1, 6);
        ST_SH4(in0_r, in1_r, in2_r, in3_r, dst, dst_stride);
        ST8x4_UB(in0, in1, dst + 8, 2 * dst_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_copy_16x2_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride)
{
    v16i8 zero = { 0 };
    v16i8 src0, src1;
    v8i16 in0_r, in1_r, in0_l, in1_l;

    LD_SB2(src, src_stride, src0, src1);
    ILVR_B2_SH(zero, src0, zero, src1, in0_r, in1_r);
    ILVL_B2_SH(zero, src0, zero, src1, in0_l, in1_l);
    SLLI_H4_SH(in0_r, in1_r, in0_l, in1_l, 6);
    ST_SH2(in0_r, in1_r, dst, dst_stride);
    ST_SH2(in0_l, in1_l, (dst + 8), dst_stride);
}

static void hevc_copy_16x4_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride)
{
    v16i8 zero = { 0 };
    v16i8 src0, src1, src2, src3;
    v8i16 in0_r, in1_r, in2_r, in3_r, in0_l, in1_l, in2_l, in3_l;

    LD_SB4(src, src_stride, src0, src1, src2, src3);
    ILVR_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3,
               in0_r, in1_r, in2_r, in3_r);
    ILVL_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3,
               in0_l, in1_l, in2_l, in3_l);
    SLLI_H4_SH(in0_r, in1_r, in2_r, in3_r, 6);
    SLLI_H4_SH(in0_l, in1_l, in2_l, in3_l, 6);
    ST_SH4(in0_r, in1_r, in2_r, in3_r, dst, dst_stride);
    ST_SH4(in0_l, in1_l, in2_l, in3_l, (dst + 8), dst_stride);
}

static void hevc_copy_16x6_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride)
{
    v16i8 zero = { 0 };
    v16i8 src0, src1, src2, src3, src4, src5;
    v8i16 in0_r, in1_r, in2_r, in3_r, in4_r, in5_r;
    v8i16 in0_l, in1_l, in2_l, in3_l, in4_l, in5_l;

    LD_SB6(src, src_stride, src0, src1, src2, src3, src4, src5);
    ILVR_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3,
               in0_r, in1_r, in2_r, in3_r);
    ILVL_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3,
               in0_l, in1_l, in2_l, in3_l);
    ILVR_B2_SH(zero, src4, zero, src5, in4_r, in5_r);
    ILVL_B2_SH(zero, src4, zero, src5, in4_l, in5_l);
    SLLI_H4_SH(in0_r, in1_r, in2_r, in3_r, 6);
    SLLI_H4_SH(in0_l, in1_l, in2_l, in3_l, 6);
    SLLI_H4_SH(in4_l, in5_l, in4_r, in5_r, 6);
    ST_SH6(in0_r, in1_r, in2_r, in3_r, in4_r, in5_r, dst, dst_stride);
    ST_SH6(in0_l, in1_l, in2_l, in3_l, in4_l, in5_l, (dst + 8), dst_stride);
}

static void hevc_copy_16x12_msa(UWORD8 *src, WORD32 src_stride,
                                WORD16 *dst, WORD32 dst_stride)
{
    v16i8 zero = { 0 };
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 src8, src9, src10, src11;
    v8i16 in0_r, in1_r, in2_r, in3_r, in0_l, in1_l, in2_l, in3_l;

    LD_SB8(src, src_stride, src0, src1, src2, src3, src4, src5, src6, src7);
    src += (8 * src_stride);
    LD_SB4(src, src_stride, src8, src9, src10, src11);

    ILVR_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3,
               in0_r, in1_r, in2_r, in3_r);
    ILVL_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3,
               in0_l, in1_l, in2_l, in3_l);
    SLLI_H4_SH(in0_r, in1_r, in2_r, in3_r, 6);
    SLLI_H4_SH(in0_l, in1_l, in2_l, in3_l, 6);
    ST_SH4(in0_r, in1_r, in2_r, in3_r, dst, dst_stride);
    ST_SH4(in0_l, in1_l, in2_l, in3_l, (dst + 8), dst_stride);
    dst += (4 * dst_stride);

    ILVR_B4_SH(zero, src4, zero, src5, zero, src6, zero, src7,
               in0_r, in1_r, in2_r, in3_r);
    ILVL_B4_SH(zero, src4, zero, src5, zero, src6, zero, src7,
               in0_l, in1_l, in2_l, in3_l);
    SLLI_H4_SH(in0_r, in1_r, in2_r, in3_r, 6);
    SLLI_H4_SH(in0_l, in1_l, in2_l, in3_l, 6);
    ST_SH4(in0_r, in1_r, in2_r, in3_r, dst, dst_stride);
    ST_SH4(in0_l, in1_l, in2_l, in3_l, (dst + 8), dst_stride);
    dst += (4 * dst_stride);

    ILVR_B4_SH(zero, src8, zero, src9, zero, src10, zero, src11,
               in0_r, in1_r, in2_r, in3_r);
    ILVL_B4_SH(zero, src8, zero, src9, zero, src10, zero, src11,
               in0_l, in1_l, in2_l, in3_l);
    SLLI_H4_SH(in0_r, in1_r, in2_r, in3_r, 6);
    SLLI_H4_SH(in0_l, in1_l, in2_l, in3_l, 6);
    ST_SH4(in0_r, in1_r, in2_r, in3_r, dst, dst_stride);
    ST_SH4(in0_l, in1_l, in2_l, in3_l, (dst + 8), dst_stride);
}

static void hevc_copy_16multx8mult_msa(UWORD8 *src, WORD32 src_stride,
                                       WORD16 *dst, WORD32 dst_stride,
                                       WORD32 height, WORD32 width)
{
    UWORD8 *src_tmp;
    WORD16 *dst_tmp;
    UWORD32 loop_cnt, cnt;
    v16i8 zero = { 0 };
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v8i16 in0_r, in1_r, in2_r, in3_r, in0_l, in1_l, in2_l, in3_l;

    for(cnt = (width >> 4); cnt--;)
    {
        src_tmp = src;
        dst_tmp = dst;

        for(loop_cnt = (height >> 3); loop_cnt--;)
        {
            LD_SB8(src_tmp, src_stride,
                   src0, src1, src2, src3, src4, src5, src6, src7);
            src_tmp += (8 * src_stride);

            ILVR_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3,
                       in0_r, in1_r, in2_r, in3_r);
            ILVL_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3,
                       in0_l, in1_l, in2_l, in3_l);
            SLLI_H4_SH(in0_r, in1_r, in2_r, in3_r, 6);
            SLLI_H4_SH(in0_l, in1_l, in2_l, in3_l, 6);
            ST_SH4(in0_r, in1_r, in2_r, in3_r, dst_tmp, dst_stride);
            ST_SH4(in0_l, in1_l, in2_l, in3_l, (dst_tmp + 8), dst_stride);
            dst_tmp += (4 * dst_stride);

            ILVR_B4_SH(zero, src4, zero, src5, zero, src6, zero, src7,
                       in0_r, in1_r, in2_r, in3_r);
            ILVL_B4_SH(zero, src4, zero, src5, zero, src6, zero, src7,
                       in0_l, in1_l, in2_l, in3_l);
            SLLI_H4_SH(in0_r, in1_r, in2_r, in3_r, 6);
            SLLI_H4_SH(in0_l, in1_l, in2_l, in3_l, 6);
            ST_SH4(in0_r, in1_r, in2_r, in3_r, dst_tmp, dst_stride);
            ST_SH4(in0_l, in1_l, in2_l, in3_l, (dst_tmp + 8), dst_stride);
            dst_tmp += (4 * dst_stride);
        }

        src += 16;
        dst += 16;
    }
}
static void hevc_copy_16w_msa(UWORD8 *src, WORD32 src_stride,
                              WORD16 *dst, WORD32 dst_stride,
                              WORD32 height)
{
    hevc_copy_16multx8mult_msa(src, src_stride, dst, dst_stride, height, 16);
}

static void hevc_copy_24w_msa(UWORD8 *src, WORD32 src_stride,
                              WORD16 *dst, WORD32 dst_stride,
                              WORD32 height)
{
    hevc_copy_16multx8mult_msa(src, src_stride, dst, dst_stride, height, 16);
    hevc_copy_8w_msa(src + 16, src_stride, dst + 16, dst_stride, height);
}

static void hevc_copy_32w_msa(UWORD8 *src, WORD32 src_stride,
                              WORD16 *dst, WORD32 dst_stride,
                              WORD32 height)
{
    hevc_copy_16multx8mult_msa(src, src_stride, dst, dst_stride, height, 32);
}

static void hevc_copy_48w_msa(UWORD8 *src, WORD32 src_stride,
                              WORD16 *dst, WORD32 dst_stride,
                              WORD32 height)
{
    hevc_copy_16multx8mult_msa(src, src_stride, dst, dst_stride, height, 48);
}

static void hevc_copy_64w_msa(UWORD8 *src, WORD32 src_stride,
                              WORD16 *dst, WORD32 dst_stride,
                              WORD32 height)
{
    hevc_copy_16multx8mult_msa(src, src_stride, dst, dst_stride, height, 64);
}

static void hevc_ilv_hz_4t_2x4_msa(UWORD8 *src, WORD32 src_stride,
                                   WORD16 *dst, WORD32 dst_stride,
                                   WORD8 *filter, WORD32 height)
{
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 16, 18, 17, 19, 18, 20, 19, 21};
    v16i8 filt0, filt1, src0, src1, src2, src3, vec0, vec1, mask1;
    v8i16 filt, const_vec, dst0, dst1;

    src -= 2;
    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);
    mask1 = ADDVI_B(mask0, 4);

    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);
    LD_SB4(src, src_stride, src0, src1, src2, src3);
    XORI_B4_128_SB(src0, src1, src2, src3);
    VSHF_B2_SB(src0, src1, src0, src1, mask0, mask1, vec0, vec1);
    dst0 = const_vec;
    DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst0, dst0);
    VSHF_B2_SB(src2, src3, src2, src3, mask0, mask1, vec0, vec1);
    dst1 = const_vec;
    DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst1, dst1);
    ST8x4_UB(dst0, dst1, dst, 2 * dst_stride);

    if(0 != (height % 2))
    {
        LD_SB3(src + 4 * src_stride, src_stride, src0, src1, src2);
        XORI_B3_128_SB(src0, src1, src2);

        VSHF_B2_SB(src0, src1, src0, src1, mask0, mask1, vec0, vec1);
        dst0 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst0, dst0);
        VSHF_B2_SB(src2, src2, src2, src2, mask0, mask1, vec0, vec1);
        dst1 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst1, dst1);

        ST8x2_UB(dst0, dst + 4 * dst_stride, 2 * dst_stride);
        ST8x1_UB(dst1, dst + 6 * dst_stride);
    }
}

static void hevc_ilv_hz_4t_2x8_msa(UWORD8 *src, WORD32 src_stride,
                                   WORD16 *dst, WORD32 dst_stride,
                                   WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, filt0, filt1;
    v16i8 mask1, vec0, vec1;
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 16, 18, 17, 19, 18, 20, 19, 21};
    v8i16 filt, const_vec, dst0, dst1, dst2, dst3;

    src -= 2;
    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);
    mask1 = ADDVI_B(mask0, 4);

    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    for(loop_cnt = (height >> 3); loop_cnt--;)
    {
        LD_SB8(src, src_stride, src0, src1, src2, src3, src4, src5, src6, src7);
        src += (8 * src_stride);

        XORI_B8_128_SB(src0, src1, src2, src3, src4, src5, src6, src7);

        VSHF_B2_SB(src0, src1, src0, src1, mask0, mask1, vec0, vec1);
        dst0 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst0, dst0);
        VSHF_B2_SB(src2, src3, src2, src3, mask0, mask1, vec0, vec1);
        dst1 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst1, dst1);
        VSHF_B2_SB(src4, src5, src4, src5, mask0, mask1, vec0, vec1);
        dst2 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst2, dst2);
        VSHF_B2_SB(src6, src7, src6, src7, mask0, mask1, vec0, vec1);
        dst3 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst3, dst3);

        ST8x8_UB(dst0, dst1, dst2, dst3, dst, 2 * dst_stride);
        dst += (8 * dst_stride);
    }

    if(0 != (height % 2))
    {
        LD_SB3(src, src_stride, src0, src1, src2);
        XORI_B3_128_SB(src0, src1, src2);

        VSHF_B2_SB(src0, src1, src0, src1, mask0, mask1, vec0, vec1);
        dst0 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst0, dst0);
        VSHF_B2_SB(src2, src2, src2, src2, mask0, mask1, vec0, vec1);
        dst1 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst1, dst1);

        ST8x2_UB(dst0, dst, 2 * dst_stride);
        ST8x1_UB(dst1, dst + 2 * dst_stride);
    }
}

static void hevc_ilv_hz_4t_4x2_msa(UWORD8 *src, WORD32 src_stride,
                                   WORD16 *dst, WORD32 dst_stride,
                                   WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9};
    v16i8 filt0, filt1, src0, src1, src2, vec0, vec1, mask1;
    v8i16 dst0, dst1, dst2, filt, const_vec;

    src -= 2;
    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);
    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);
    mask1 = ADDVI_B(mask0, 4);

    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    for(loop_cnt = (height >> 1); loop_cnt--;)
    {
        LD_SB2(src, src_stride, src0, src1);
        src += (2 * src_stride);
        XORI_B2_128_SB(src0, src1);
        VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
        dst0 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst0, dst0);
        VSHF_B2_SB(src1, src1, src1, src1, mask0, mask1, vec0, vec1);
        dst1 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst1, dst1);
        ST_SH2(dst0, dst1, dst, dst_stride);
        dst += (2 * dst_stride);
    }

    if(0 != (height % 2))
    {
        LD_SB3(src, src_stride, src0, src1, src2);
        XORI_B3_128_SB(src0, src1, src2);

        VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
        dst0 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst0, dst0);
        VSHF_B2_SB(src1, src1, src1, src1, mask0, mask1, vec0, vec1);
        dst1 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst1, dst1);
        VSHF_B2_SB(src2, src2, src2, src2, mask0, mask1, vec0, vec1);
        dst2 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst2, dst2);

        ST_SH3(dst0, dst1, dst2, dst, dst_stride);
    }
}

static void hevc_ilv_hz_4t_4x4mult_msa(UWORD8 *src, WORD32 src_stride,
                                       WORD16 *dst, WORD32 dst_stride,
                                       WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9};
    v16i8 filt0, filt1, src0, src1, src2, src3, vec0, vec1, mask1;
    v8i16 filt, const_vec, dst0, dst1, dst2, dst3;

    src -= 2;
    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);
    mask1 = ADDVI_B(mask0, 4);

    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src0, src1, src2, src3);
        src += (4 * src_stride);

        XORI_B4_128_SB(src0, src1, src2, src3);

        VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
        dst0 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst0, dst0);

        VSHF_B2_SB(src1, src1, src1, src1, mask0, mask1, vec0, vec1);
        dst1 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst1, dst1);

        VSHF_B2_SB(src2, src2, src2, src2, mask0, mask1, vec0, vec1);
        dst2 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst2, dst2);

        VSHF_B2_SB(src3, src3, src3, src3, mask0, mask1, vec0, vec1);
        dst3 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst3, dst3);

        ST_SH4(dst0, dst1, dst2, dst3, dst, dst_stride);
        dst += (4 * dst_stride);
    }

    if(0 != (height % 2))
    {
        LD_SB3(src, src_stride, src0, src1, src2);
        XORI_B3_128_SB(src0, src1, src2);

        VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
        dst0 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst0, dst0);
        VSHF_B2_SB(src1, src1, src1, src1, mask0, mask1, vec0, vec1);
        dst1 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst1, dst1);
        VSHF_B2_SB(src2, src2, src2, src2, mask0, mask1, vec0, vec1);
        dst2 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst2, dst2);

        ST_SH3(dst0, dst1, dst2, dst, dst_stride);
    }
}

static void hevc_ilv_hz_4t_6w_msa(UWORD8 *src, WORD32 src_stride,
                                  WORD16 *dst, WORD32 dst_stride,
                                  WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    UWORD16 tmp0, tmp1, tmp2, tmp3;
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9};
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, filt0, filt1;
    v16i8 mask1, mask2, mask3, vec0, vec1, vec2, vec3;
    v8i16 dst0, dst1, dst2, dst3, dst4, dst6, filt, const_vec;

    src -= 2;
    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);
    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    mask1 = ADDVI_B(mask0, 4);
    mask2 = ADDVI_B(mask0, 8);
    mask3 = ADDVI_B(mask0, 12);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src0, src2, src4, src6);
        tmp0 = LH(src + 16);
        tmp1 = LH(src + 16 + src_stride);
        tmp2 = LH(src + 16 + 2 * src_stride);
        tmp3 = LH(src + 16 + 3 * src_stride);
        src1 = (v16i8) __msa_fill_h(tmp0);
        src3 = (v16i8) __msa_fill_h(tmp1);
        src5 = (v16i8) __msa_fill_h(tmp2);
        src7 = (v16i8) __msa_fill_h(tmp3);
        src += 4 * src_stride;
        XORI_B8_128_SB(src0, src1, src2, src3, src4, src5, src6, src7);

        VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
        dst0 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst0, dst0);
        VSHF_B2_SB(src2, src2, src2, src2, mask0, mask1, vec0, vec1);
        dst2 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst2, dst2);
        VSHF_B2_SB(src4, src4, src4, src4, mask0, mask1, vec0, vec1);
        dst4 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst4, dst4);
        VSHF_B2_SB(src6, src6, src6, src6, mask0, mask1, vec0, vec1);
        dst6 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst6, dst6);
        VSHF_B2_SB(src0, src1, src0, src1, mask2, mask3, vec0, vec1);
        VSHF_B2_SB(src2, src3, src2, src3, mask2, mask3, vec2, vec3);
        ILVR_D2_SB(vec2, vec0, vec3, vec1, vec0, vec1);
        dst1 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst1, dst1);
        VSHF_B2_SB(src4, src5, src4, src5, mask2, mask3, vec0, vec1);
        VSHF_B2_SB(src6, src7, src6, src7, mask2, mask3, vec2, vec3);
        ILVR_D2_SB(vec2, vec0, vec3, vec1, vec0, vec1);
        dst3 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst3, dst3);

        ST_SH4(dst0, dst2, dst4, dst6, dst, dst_stride);
        ST8x4_UB(dst1, dst3, dst + 8, 2 * dst_stride);
        dst += 4 * dst_stride;
    }

    if(0 != (height % 2))
    {
        LD_SB3(src, src_stride, src0, src2, src4);
        tmp0 = LH(src + 16);
        tmp1 = LH(src + 16 + src_stride);
        tmp2 = LH(src + 16 + 2 * src_stride);
        src1 = (v16i8) __msa_fill_h(tmp0);
        src3 = (v16i8) __msa_fill_h(tmp1);
        src5 = (v16i8) __msa_fill_h(tmp2);
        XORI_B6_128_SB(src0, src1, src2, src3, src4, src5);

        VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
        dst0 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst0, dst0);
        VSHF_B2_SB(src2, src2, src2, src2, mask0, mask1, vec0, vec1);
        dst2 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst2, dst2);
        VSHF_B2_SB(src4, src4, src4, src4, mask0, mask1, vec0, vec1);
        dst4 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst4, dst4);
        VSHF_B2_SB(src0, src1, src0, src1, mask2, mask3, vec0, vec1);
        VSHF_B2_SB(src2, src3, src2, src3, mask2, mask3, vec2, vec3);
        ILVR_D2_SB(vec2, vec0, vec3, vec1, vec0, vec1);
        dst1 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst1, dst1);
        VSHF_B2_SB(src4, src5, src4, src5, mask2, mask3, vec0, vec1);
        dst3 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst3, dst3);

        ST_SH3(dst0, dst2, dst4, dst, dst_stride);
        ST8x2_UB(dst1, dst + 8, 2 * dst_stride);
        ST8x1_UB(dst3, dst + 2 * dst_stride + 8);
    }
}

static void hevc_ilv_hz_4t_8x2mult_msa(UWORD8 *src, WORD32 src_stride,
                                       WORD16 *dst, WORD32 dst_stride,
                                       WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    uint64_t tmp0, tmp1;
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9};
    v16i8 src0, src1, src2, src3, filt0, filt1, mask1, mask2, mask3, vec0, vec1;
    v8i16 out0, out1, out2, out3, filt, const_vec;

    src -= 2;
    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);
    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    mask1 = ADDVI_B(mask0, 4);
    mask2 = ADDVI_B(mask0, 8);
    mask3 = ADDVI_B(mask0, 12);

    for(loop_cnt = (height >> 1); loop_cnt--;)
    {
        LD_SB2(src, src_stride, src0, src2);
        LD2(src + 16, src_stride, tmp0, tmp1);
        src1 = (v16i8) __msa_fill_d(tmp0);
        src3 = (v16i8) __msa_fill_d(tmp1);
        src += 2 * src_stride;
        XORI_B4_128_SB(src0, src1, src2, src3);

        VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
        out0 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out0, out0);
        VSHF_B2_SB(src0, src1, src0, src1, mask2, mask3, vec0, vec1);
        out1 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out1, out1);
        VSHF_B2_SB(src2, src2, src2, src2, mask0, mask1, vec0, vec1);
        out2 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out2, out2);
        VSHF_B2_SB(src2, src3, src2, src3, mask2, mask3, vec0, vec1);
        out3 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out3, out3);

        ST_SH2(out0, out1, dst, 8);
        ST_SH2(out2, out3, dst + dst_stride, 8);
        dst += 2 * dst_stride;
    }

    if(0 != (height % 2))
    {
        src0 = LD_SB(src);
        tmp0 = LD(src + 16);
        src1 = (v16i8) __msa_fill_d(tmp0);
        XORI_B2_128_SB(src0, src1);

        VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
        out0 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out0, out0);
        VSHF_B2_SB(src0, src1, src0, src1, mask2, mask3, vec0, vec1);
        out1 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out1, out1);

        ST_SH2(out0, out1, dst, 8);
    }
}

static void hevc_ilv_hz_4t_8x4mult_msa(UWORD8 *src, WORD32 src_stride,
                                       WORD16 *dst, WORD32 dst_stride,
                                       WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    uint64_t tmp0, tmp1, tmp2, tmp3;
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9};
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, filt0, filt1;
    v16i8 mask1, mask2, mask3, vec0, vec1;
    v8i16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7, filt, const_vec;

    src -= 2;
    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);
    mask1 = ADDVI_B(mask0, 4);
    mask2 = ADDVI_B(mask0, 8);
    mask3 = ADDVI_B(mask0, 12);

    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);
    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src0, src2, src4, src6);
        LD4(src + 16, src_stride, tmp0, tmp1, tmp2, tmp3);
        src1 = (v16i8) __msa_fill_d(tmp0);
        src3 = (v16i8) __msa_fill_d(tmp1);
        src5 = (v16i8) __msa_fill_d(tmp2);
        src7 = (v16i8) __msa_fill_d(tmp3);
        src += 4 * src_stride;
        XORI_B8_128_SB(src0, src1, src2, src3, src4, src5, src6, src7);

        VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
        dst0 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst0, dst0);
        VSHF_B2_SB(src0, src1, src0, src1, mask2, mask3, vec0, vec1);
        dst1 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst1, dst1);
        VSHF_B2_SB(src2, src2, src2, src2, mask0, mask1, vec0, vec1);
        dst2 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst2, dst2);
        VSHF_B2_SB(src2, src3, src2, src3, mask2, mask3, vec0, vec1);
        dst3 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst3, dst3);
        VSHF_B2_SB(src4, src4, src4, src4, mask0, mask1, vec0, vec1);
        dst4 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst4, dst4);
        VSHF_B2_SB(src4, src5, src4, src5, mask2, mask3, vec0, vec1);
        dst5 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst5, dst5);
        VSHF_B2_SB(src6, src6, src6, src6, mask0, mask1, vec0, vec1);
        dst6 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst6, dst6);
        VSHF_B2_SB(src6, src7, src6, src7, mask2, mask3, vec0, vec1);
        dst7 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst7, dst7);

        ST_SH2(dst0, dst1, dst, 8);
        ST_SH2(dst2, dst3, dst + dst_stride, 8);
        ST_SH2(dst4, dst5, dst + 2 * dst_stride, 8);
        ST_SH2(dst6, dst7, dst + 3 * dst_stride, 8);
        dst += 4 * dst_stride;
    }

    if(0 != (height % 2))
    {
        LD_SB3(src, src_stride, src0, src2, src4);
        LD2(src + 16, src_stride, tmp0, tmp1);
        tmp2 = LD(src + 2 * src_stride + 16);
        src1 = (v16i8) __msa_fill_d(tmp0);
        src3 = (v16i8) __msa_fill_d(tmp1);
        src5 = (v16i8) __msa_fill_d(tmp2);
        XORI_B6_128_SB(src0, src1, src2, src3, src4, src5);

        VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
        dst0 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst0, dst0);
        VSHF_B2_SB(src0, src1, src0, src1, mask2, mask3, vec0, vec1);
        dst1 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst1, dst1);
        VSHF_B2_SB(src2, src2, src2, src2, mask0, mask1, vec0, vec1);
        dst2 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst2, dst2);
        VSHF_B2_SB(src2, src3, src2, src3, mask2, mask3, vec0, vec1);
        dst3 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst3, dst3);
        VSHF_B2_SB(src4, src4, src4, src4, mask0, mask1, vec0, vec1);
        dst4 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst4, dst4);
        VSHF_B2_SB(src4, src5, src4, src5, mask2, mask3, vec0, vec1);
        dst5 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst5, dst5);

        ST_SH2(dst0, dst1, dst, 8);
        ST_SH2(dst2, dst3, dst + dst_stride, 8);
        ST_SH2(dst4, dst5, dst + 2 * dst_stride, 8);
    }
}

static void hevc_ilv_hz_4t_12w_msa(UWORD8 *src, WORD32 src_stride,
                                   WORD16 *dst, WORD32 dst_stride,
                                   WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9};
    v16i8 filt0, filt1, mask1, mask2, mask3, vec0, vec1;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v8i16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7, dst8, dst9;
    v8i16 filt, const_vec, dst10, dst11;

    src -= 2;
    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    mask1 = ADDVI_B(mask0, 4);
    mask2 = ADDVI_B(mask0, 8);
    mask3 = ADDVI_B(mask0, 12);

    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src0, src2, src4, src6);
        LD_SB4(src + 16, src_stride, src1, src3, src5, src7);
        src += 4 * src_stride;
        XORI_B8_128_SB(src0, src1, src2, src3, src4, src5, src6, src7);

        VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
        dst0 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst0, dst0);
        VSHF_B2_SB(src0, src1, src0, src1, mask2, mask3, vec0, vec1);
        dst1 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst1, dst1);
        VSHF_B2_SB(src1, src1, src1, src1, mask0, mask1, vec0, vec1);
        dst2 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst2, dst2);
        VSHF_B2_SB(src2, src2, src2, src2, mask0, mask1, vec0, vec1);
        dst3 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst3, dst3);
        VSHF_B2_SB(src2, src3, src2, src3, mask2, mask3, vec0, vec1);
        dst4 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst4, dst4);
        VSHF_B2_SB(src3, src3, src3, src3, mask0, mask1, vec0, vec1);
        dst5 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst5, dst5);
        VSHF_B2_SB(src4, src4, src4, src4, mask0, mask1, vec0, vec1);
        dst6 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst6, dst6);
        VSHF_B2_SB(src4, src5, src4, src5, mask2, mask3, vec0, vec1);
        dst7 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst7, dst7);
        VSHF_B2_SB(src5, src5, src5, src5, mask0, mask1, vec0, vec1);
        dst8 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst8, dst8);
        VSHF_B2_SB(src6, src6, src6, src6, mask0, mask1, vec0, vec1);
        dst9 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst9, dst9);
        VSHF_B2_SB(src6, src7, src6, src7, mask2, mask3, vec0, vec1);
        dst10 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst10, dst10);
        VSHF_B2_SB(src7, src7, src7, src7, mask0, mask1, vec0, vec1);
        dst11 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst11, dst11);

        ST_SH3(dst0, dst1, dst2, dst, 8);
        ST_SH3(dst3, dst4, dst5, dst + dst_stride, 8);
        ST_SH3(dst6, dst7, dst8, dst +  2 * dst_stride, 8);
        ST_SH3(dst9, dst10, dst11, dst +  3 * dst_stride, 8);
        dst += 4 * dst_stride;
    }

    if(0 != (height % 2))
    {
        LD_SB3(src, src_stride, src0, src2, src4);
        LD_SB3(src + 16, src_stride, src1, src3, src5);
        XORI_B6_128_SB(src0, src1, src2, src3, src4, src5);

        VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
        dst0 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst0, dst0);
        VSHF_B2_SB(src0, src1, src0, src1, mask2, mask3, vec0, vec1);
        dst1 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst1, dst1);
        VSHF_B2_SB(src1, src1, src1, src1, mask0, mask1, vec0, vec1);
        dst2 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst2, dst2);
        VSHF_B2_SB(src2, src2, src2, src2, mask0, mask1, vec0, vec1);
        dst3 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst3, dst3);
        VSHF_B2_SB(src2, src3, src2, src3, mask2, mask3, vec0, vec1);
        dst4 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst4, dst4);
        VSHF_B2_SB(src3, src3, src3, src3, mask0, mask1, vec0, vec1);
        dst5 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst5, dst5);
        VSHF_B2_SB(src4, src4, src4, src4, mask0, mask1, vec0, vec1);
        dst6 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst6, dst6);
        VSHF_B2_SB(src4, src5, src4, src5, mask2, mask3, vec0, vec1);
        dst7 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst7, dst7);
        VSHF_B2_SB(src5, src5, src5, src5, mask0, mask1, vec0, vec1);
        dst8 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst8, dst8);

        ST_SH3(dst0, dst1, dst2, dst, 8);
        ST_SH3(dst3, dst4, dst5, dst + dst_stride, 8);
        ST_SH3(dst6, dst7, dst8, dst +  2 * dst_stride, 8);
    }
}

static void hevc_ilv_hz_4t_16w_msa(UWORD8 *src, WORD32 src_stride,
                                   WORD16 *dst, WORD32 dst_stride,
                                   WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    uint64_t tmp0, tmp1, tmp2, tmp3;
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9};
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v16i8 src11, filt0, filt1, mask1, mask2, mask3, vec0, vec1;
    v8i16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v8i16 dst8, dst9, dst10, dst11, dst12, dst13, dst14, dst15;
    v8i16 filt, const_vec;

    src -= 2;
    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);
    mask1 = ADDVI_B(mask0, 4);
    mask2 = ADDVI_B(mask0, 8);
    mask3 = ADDVI_B(mask0, 12);

    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src0, src3, src6, src9);
        LD_SB4(src + 16, src_stride, src1, src4, src7, src10);
        LD4(src + 32, src_stride, tmp0, tmp1, tmp2, tmp3);
        src2 = (v16i8) __msa_fill_d(tmp0);
        src5 = (v16i8) __msa_fill_d(tmp1);
        src8 = (v16i8) __msa_fill_d(tmp2);
        src11 = (v16i8) __msa_fill_d(tmp3);
        src += 4 * src_stride;
        XORI_B8_128_SB(src0, src1, src2, src3, src4, src5, src6, src7);
        XORI_B4_128_SB(src8, src9, src10, src11);

        VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
        dst0 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst0, dst0);
        VSHF_B2_SB(src0, src1, src0, src1, mask2, mask3, vec0, vec1);
        dst1 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst1, dst1);
        VSHF_B2_SB(src1, src1, src1, src1, mask0, mask1, vec0, vec1);
        dst2 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst2, dst2);
        VSHF_B2_SB(src1, src2, src1, src2, mask2, mask3, vec0, vec1);
        dst3 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst3, dst3);
        VSHF_B2_SB(src3, src3, src3, src3, mask0, mask1, vec0, vec1);
        dst4 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst4, dst4);
        VSHF_B2_SB(src3, src4, src3, src4, mask2, mask3, vec0, vec1);
        dst5 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst5, dst5);
        VSHF_B2_SB(src4, src4, src4, src4, mask0, mask1, vec0, vec1);
        dst6 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst6, dst6);
        VSHF_B2_SB(src4, src5, src4, src5, mask2, mask3, vec0, vec1);
        dst7 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst7, dst7);
        VSHF_B2_SB(src6, src6, src6, src6, mask0, mask1, vec0, vec1);
        dst8 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst8, dst8);
        VSHF_B2_SB(src6, src7, src6, src7, mask2, mask3, vec0, vec1);
        dst9 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst9, dst9);
        VSHF_B2_SB(src7, src7, src7, src7, mask0, mask1, vec0, vec1);
        dst10 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst10, dst10);
        VSHF_B2_SB(src7, src8, src7, src8, mask2, mask3, vec0, vec1);
        dst11 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst11, dst11);
        VSHF_B2_SB(src9, src9, src9, src9, mask0, mask1, vec0, vec1);
        dst12 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst12, dst12);
        VSHF_B2_SB(src9, src10, src9, src10, mask2, mask3, vec0, vec1);
        dst13 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst13, dst13);
        VSHF_B2_SB(src10, src10, src10, src10, mask0, mask1, vec0, vec1);
        dst14 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst14, dst14);
        VSHF_B2_SB(src10, src11, src10, src11, mask2, mask3, vec0, vec1);
        dst15 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst15, dst15);

        ST_SH4(dst0, dst1, dst2, dst3, dst, 8);
        ST_SH4(dst4, dst5, dst6, dst7, dst + dst_stride, 8);
        ST_SH4(dst8, dst9, dst10, dst11, dst + 2 * dst_stride, 8);
        ST_SH4(dst12, dst13, dst14, dst15, dst + 3 * dst_stride, 8);
        dst += 4 * dst_stride;
    }

    if(0 != (height % 2))
    {
        LD_SB3(src, src_stride, src0, src3, src6);
        LD_SB3(src + 16, src_stride, src1, src4, src7);
        LD2(src + 32, src_stride, tmp0, tmp1);
        tmp2 = LD(src + 32 + 2 * src_stride);
        src2 = (v16i8) __msa_fill_d(tmp0);
        src5 = (v16i8) __msa_fill_d(tmp1);
        src8 = (v16i8) __msa_fill_d(tmp2);
        XORI_B7_128_SB(src0, src1, src2, src3, src4, src5, src6);
        XORI_B2_128_SB(src7, src8);

        VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
        dst0 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst0, dst0);
        VSHF_B2_SB(src0, src1, src0, src1, mask2, mask3, vec0, vec1);
        dst1 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst1, dst1);
        VSHF_B2_SB(src1, src1, src1, src1, mask0, mask1, vec0, vec1);
        dst2 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst2, dst2);
        VSHF_B2_SB(src1, src2, src1, src2, mask2, mask3, vec0, vec1);
        dst3 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst3, dst3);
        VSHF_B2_SB(src3, src3, src3, src3, mask0, mask1, vec0, vec1);
        dst4 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst4, dst4);
        VSHF_B2_SB(src3, src4, src3, src4, mask2, mask3, vec0, vec1);
        dst5 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst5, dst5);
        VSHF_B2_SB(src4, src4, src4, src4, mask0, mask1, vec0, vec1);
        dst6 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst6, dst6);
        VSHF_B2_SB(src4, src5, src4, src5, mask2, mask3, vec0, vec1);
        dst7 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst7, dst7);
        VSHF_B2_SB(src6, src6, src6, src6, mask0, mask1, vec0, vec1);
        dst8 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst8, dst8);
        VSHF_B2_SB(src6, src7, src6, src7, mask2, mask3, vec0, vec1);
        dst9 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst9, dst9);
        VSHF_B2_SB(src7, src7, src7, src7, mask0, mask1, vec0, vec1);
        dst10 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst10, dst10);
        VSHF_B2_SB(src7, src8, src7, src8, mask2, mask3, vec0, vec1);
        dst11 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, dst11, dst11);

        ST_SH4(dst0, dst1, dst2, dst3, dst, 8);
        ST_SH4(dst4, dst5, dst6, dst7, dst + dst_stride, 8);
        ST_SH4(dst8, dst9, dst10, dst11, dst + 2 * dst_stride, 8);
    }
}

static void hevc_ilv_hz_4t_24w_msa(UWORD8 *src, WORD32 src_stride,
                                   WORD16 *dst, WORD32 dst_stride,
                                   WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    uint64_t tmp0, tmp1;
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9};
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 filt0, filt1, mask1, mask2, mask3, vec0, vec1;
    v8i16 out0, out1, out2, out3, out4, out5, out6, out7, out8, out9, out10;
    v8i16 out11, filt, const_vec;

    src -= 2;
    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);
    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    mask1 = ADDVI_B(mask0, 4);
    mask2 = ADDVI_B(mask0, 8);
    mask3 = ADDVI_B(mask0, 12);

    for(loop_cnt = (height >> 1); loop_cnt--;)
    {
        LD_SB2(src, src_stride, src0, src4);
        LD_SB2(src + 16, src_stride, src1, src5);
        LD_SB2(src + 32, src_stride, src2, src6);
        LD2(src + 48, src_stride, tmp0, tmp1);
        src3 = (v16i8) __msa_fill_d(tmp0);
        src7 = (v16i8) __msa_fill_d(tmp1);
        src += 2 * src_stride;
        XORI_B8_128_SB(src0, src1, src2, src3, src4, src5, src6, src7);

        VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
        out0 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out0, out0);
        VSHF_B2_SB(src0, src1, src0, src1, mask2, mask3, vec0, vec1);
        out1 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out1, out1);
        VSHF_B2_SB(src1, src1, src1, src1, mask0, mask1, vec0, vec1);
        out2 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out2, out2);
        VSHF_B2_SB(src1, src2, src1, src2, mask2, mask3, vec0, vec1);
        out3 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out3, out3);
        VSHF_B2_SB(src2, src2, src2, src2, mask0, mask1, vec0, vec1);
        out4 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out4, out4);
        VSHF_B2_SB(src2, src3, src2, src3, mask2, mask3, vec0, vec1);
        out5 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out5, out5);
        VSHF_B2_SB(src4, src4, src4, src4, mask0, mask1, vec0, vec1);
        out6 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out6, out6);
        VSHF_B2_SB(src4, src5, src4, src5, mask2, mask3, vec0, vec1);
        out7 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out7, out7);
        VSHF_B2_SB(src5, src5, src5, src5, mask0, mask1, vec0, vec1);
        out8 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out8, out8);
        VSHF_B2_SB(src5, src6, src5, src6, mask2, mask3, vec0, vec1);
        out9 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out9, out9);
        VSHF_B2_SB(src6, src6, src6, src6, mask0, mask1, vec0, vec1);
        out10 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out10, out10);
        VSHF_B2_SB(src6, src7, src6, src7, mask2, mask3, vec0, vec1);
        out11 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out11, out11);

        ST_SH6(out0, out1, out2, out3, out4, out5, dst, 8);
        ST_SH6(out6, out7, out8, out9, out10, out11, dst + dst_stride, 8);
        dst += 2 * dst_stride;
    }

    if(0 != (height % 2))
    {
        LD_SB3(src, 16, src0, src1, src2);
        tmp0 = LD(src + 48);
        src3 = (v16i8) __msa_fill_d(tmp0);
        XORI_B4_128_SB(src0, src1, src2, src3);

        VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
        out0 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out0, out0);
        VSHF_B2_SB(src0, src1, src0, src1, mask2, mask3, vec0, vec1);
        out1 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out1, out1);
        VSHF_B2_SB(src1, src1, src1, src1, mask0, mask1, vec0, vec1);
        out2 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out2, out2);
        VSHF_B2_SB(src1, src2, src1, src2, mask2, mask3, vec0, vec1);
        out3 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out3, out3);
        VSHF_B2_SB(src2, src2, src2, src2, mask0, mask1, vec0, vec1);
        out4 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out4, out4);
        VSHF_B2_SB(src2, src3, src2, src3, mask2, mask3, vec0, vec1);
        out5 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out5, out5);

        ST_SH6(out0, out1, out2, out3, out4, out5, dst, 8);
    }
}

static void hevc_ilv_hz_4t_32w_msa(UWORD8 *src, WORD32 src_stride,
                                   WORD16 *dst, WORD32 dst_stride,
                                   WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    uint64_t tmp0, tmp1;
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9};
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9;
    v16i8 filt0, filt1, mask1, mask2, mask3, vec0, vec1;
    v8i16 out0, out1, out2, out3, out4, out5, out6, out7, filt, const_vec;
    v8i16 out8, out9, out10, out11, out12, out13, out14, out15;

    src -= 2;
    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);
    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    mask1 = ADDVI_B(mask0, 4);
    mask2 = ADDVI_B(mask0, 8);
    mask3 = ADDVI_B(mask0, 12);

    for(loop_cnt = height >> 1; loop_cnt--;)
    {
        LD_SB4(src, 16, src0, src1, src2, src3);
        LD_SB4(src + src_stride, 16, src5, src6, src7, src8);
        tmp0 = LD(src + 64);
        tmp1 = LD(src + src_stride + 64);
        src4 = (v16i8) __msa_fill_d(tmp0);
        src9 = (v16i8) __msa_fill_d(tmp1);
        src += 2 * src_stride;
        XORI_B5_128_SB(src0, src1, src2, src3, src4);
        XORI_B5_128_SB(src5, src6, src7, src8, src9);

        VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
        out0 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out0, out0);
        VSHF_B2_SB(src0, src1, src0, src1, mask2, mask3, vec0, vec1);
        out1 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out1, out1);
        VSHF_B2_SB(src1, src1, src1, src1, mask0, mask1, vec0, vec1);
        out2 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out2, out2);
        VSHF_B2_SB(src1, src2, src1, src2, mask2, mask3, vec0, vec1);
        out3 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out3, out3);
        VSHF_B2_SB(src2, src2, src2, src2, mask0, mask1, vec0, vec1);
        out4 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out4, out4);
        VSHF_B2_SB(src2, src3, src2, src3, mask2, mask3, vec0, vec1);
        out5 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out5, out5);
        VSHF_B2_SB(src3, src3, src3, src3, mask0, mask1, vec0, vec1);
        out6 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out6, out6);
        VSHF_B2_SB(src3, src4, src3, src4, mask2, mask3, vec0, vec1);
        out7 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out7, out7);

        VSHF_B2_SB(src5, src5, src5, src5, mask0, mask1, vec0, vec1);
        out8 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out8, out8);
        VSHF_B2_SB(src5, src6, src5, src6, mask2, mask3, vec0, vec1);
        out9 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out9, out9);
        VSHF_B2_SB(src6, src6, src6, src6, mask0, mask1, vec0, vec1);
        out10 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out10, out10);
        VSHF_B2_SB(src6, src7, src6, src7, mask2, mask3, vec0, vec1);
        out11 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out11, out11);
        VSHF_B2_SB(src7, src7, src7, src7, mask0, mask1, vec0, vec1);
        out12 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out12, out12);
        VSHF_B2_SB(src7, src8, src7, src8, mask2, mask3, vec0, vec1);
        out13 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out13, out13);
        VSHF_B2_SB(src8, src8, src8, src8, mask0, mask1, vec0, vec1);
        out14 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out14, out14);
        VSHF_B2_SB(src8, src9, src8, src9, mask2, mask3, vec0, vec1);
        out15 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out15, out15);

        ST_SH8(out0, out1, out2, out3, out4, out5, out6, out7, dst, 8);
        ST_SH8(out8, out9, out10, out11, out12, out13, out14, out15,
               dst + dst_stride, 8);
        dst += 2 * dst_stride;
    }

    if(0 != (height % 2))
    {
        LD_SB4(src, 16, src0, src1, src2, src3);
        tmp0 = LD(src + 64);
        src4 = (v16i8) __msa_fill_d(tmp0);
        XORI_B5_128_SB(src0, src1, src2, src3, src4);

        VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
        out0 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out0, out0);
        VSHF_B2_SB(src0, src1, src0, src1, mask2, mask3, vec0, vec1);
        out1 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out1, out1);
        VSHF_B2_SB(src1, src1, src1, src1, mask0, mask1, vec0, vec1);
        out2 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out2, out2);
        VSHF_B2_SB(src1, src2, src1, src2, mask2, mask3, vec0, vec1);
        out3 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out3, out3);
        VSHF_B2_SB(src2, src2, src2, src2, mask0, mask1, vec0, vec1);
        out4 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out4, out4);
        VSHF_B2_SB(src2, src3, src2, src3, mask2, mask3, vec0, vec1);
        out5 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out5, out5);
        VSHF_B2_SB(src3, src3, src3, src3, mask0, mask1, vec0, vec1);
        out6 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out6, out6);
        VSHF_B2_SB(src3, src4, src3, src4, mask2, mask3, vec0, vec1);
        out7 = const_vec;
        DPADD_SB2_SH(vec0, vec1, filt0, filt1, out7, out7);

        ST_SH8(out0, out1, out2, out3, out4, out5, out6, out7, dst, 8);
    }
}

static void hevc_vt_4t_4x2_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride,
                               WORD8 *filter)
{
    v16i8 src0, src1, src2, src3, src4;
    v16i8 src10_r, src32_r, src21_r, src43_r, src2110, src4332;
    v8i16 dst10, filt0, filt1, filter_vec, const_vec;

    src -= src_stride;

    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    filter_vec = LD_SH(filter);
    SPLATI_H2_SH(filter_vec, 0, 1, filt0, filt1);

    LD_SB5(src, src_stride, src0, src1, src2, src3, src4);
    ILVR_B4_SB(src1, src0, src2, src1, src3, src2, src4, src3,
               src10_r, src21_r, src32_r, src43_r);

    ILVR_D2_SB(src21_r, src10_r, src43_r, src32_r, src2110, src4332);
    XORI_B2_128_SB(src2110, src4332);
    dst10 = const_vec;
    DPADD_SB2_SH(src2110, src4332, filt0, filt1, dst10, dst10);

    ST8x2_UB(dst10, dst, 2 * dst_stride);
}

static void hevc_vt_4t_4x4_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride,
                               WORD8 *filter)
{
    v16i8 src0, src1, src2, src3, src4, src5, src6, src2110, src4332, src6554;
    v16i8 src10_r, src32_r, src54_r, src21_r, src43_r, src65_r;
    v8i16 dst10, dst32, filt0, filt1, filter_vec, const_vec;

    src -= src_stride;

    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    filter_vec = LD_SH(filter);
    SPLATI_H2_SH(filter_vec, 0, 1, filt0, filt1);

    LD_SB7(src, src_stride, src0, src1, src2, src3, src4, src5, src6);
    ILVR_B4_SB(src1, src0, src2, src1, src3, src2, src4, src3,
               src10_r, src21_r, src32_r, src43_r);
    ILVR_B2_SB(src5, src4, src6, src5, src54_r, src65_r);
    ILVR_D3_SB(src21_r, src10_r, src43_r, src32_r, src65_r, src54_r,
               src2110, src4332, src6554);
    XORI_B3_128_SB(src2110, src4332, src6554);
    dst10 = const_vec;
    DPADD_SB2_SH(src2110, src4332, filt0, filt1, dst10, dst10);
    dst32 = const_vec;
    DPADD_SB2_SH(src4332, src6554, filt0, filt1, dst32, dst32);

    ST8x4_UB(dst10, dst32, dst, 2 * dst_stride);
}

static void hevc_vt_4t_4x8multiple_msa(UWORD8 *src, WORD32 src_stride,
                                       WORD16 *dst, WORD32 dst_stride,
                                       WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9;
    v16i8 src10_r, src32_r, src54_r, src76_r, src98_r, src21_r, src43_r;
    v16i8 src65_r, src87_r, src109_r, src2110, src4332, src6554, src8776;
    v8i16 dst10, dst32, dst54, dst76, filt0, filt1, filter_vec, const_vec;

    src -= src_stride;
    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    filter_vec = LD_SH(filter);
    SPLATI_H2_SH(filter_vec, 0, 1, filt0, filt1);

    LD_SB3(src, src_stride, src0, src1, src2);
    src += (3 * src_stride);

    ILVR_B2_SB(src1, src0, src2, src1, src10_r, src21_r);
    src2110 = (v16i8) __msa_ilvr_d((v2i64) src21_r, (v2i64) src10_r);
    src2110 = (v16i8) XORI_B(src2110, 128);

    for(loop_cnt = (height >> 3); loop_cnt--;)
    {
        LD_SB6(src, src_stride, src3, src4, src5, src6, src7, src8);
        src += (6 * src_stride);

        ILVR_B4_SB(src3, src2, src4, src3, src5, src4, src6, src5,
                   src32_r, src43_r, src54_r, src65_r);
        ILVR_B2_SB(src7, src6, src8, src7, src76_r, src87_r);
        ILVR_D3_SB(src43_r, src32_r, src65_r, src54_r, src87_r, src76_r,
                   src4332, src6554, src8776);
        XORI_B3_128_SB(src4332, src6554, src8776);

        dst10 = const_vec;
        DPADD_SB2_SH(src2110, src4332,  filt0, filt1, dst10, dst10);
        dst32 = const_vec;
        DPADD_SB2_SH(src4332, src6554, filt0, filt1, dst32, dst32);
        dst54 = const_vec;
        DPADD_SB2_SH(src6554, src8776, filt0, filt1, dst54, dst54);

        LD_SB2(src, src_stride, src9, src2);
        src += (2 * src_stride);
        ILVR_B2_SB(src9, src8, src2, src9, src98_r, src109_r);
        src2110 = (v16i8) __msa_ilvr_d((v2i64) src109_r, (v2i64) src98_r);
        src2110 = (v16i8) XORI_B(src2110, 128);
        dst76 = const_vec;
        DPADD_SB2_SH(src8776, src2110, filt0, filt1, dst76, dst76);

        ST8x8_UB(dst10, dst32, dst54, dst76, dst, 2 * dst_stride);
        dst += (8 * dst_stride);
    }
}

static void hevc_vt_4t_4w_msa(UWORD8 *src, WORD32 src_stride,
                              WORD16 *dst, WORD32 dst_stride,
                              WORD8 *filter, WORD32 height)
{
    if(2 == height)
    {
        hevc_vt_4t_4x2_msa(src, src_stride, dst, dst_stride, filter);
    }
    else if(4 == height)
    {
        hevc_vt_4t_4x4_msa(src, src_stride, dst, dst_stride, filter);
    }
    else if(0 == (height % 8))
    {
        hevc_vt_4t_4x8multiple_msa(src, src_stride, dst, dst_stride, filter,
                                   height);
    }
}

static void hevc_vt_4t_8x2_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride,
                               WORD8 *filter)
{
    v16i8 src0, src1, src2, src3, src4, src10_r, src32_r, src21_r, src43_r;
    v8i16 dst0_r, dst1_r, filt0, filt1, filter_vec, const_vec;

    src -= src_stride;
    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    filter_vec = LD_SH(filter);
    SPLATI_H2_SH(filter_vec, 0, 1, filt0, filt1);

    LD_SB3(src, src_stride, src0, src1, src2);
    src += (3 * src_stride);
    XORI_B3_128_SB(src0, src1, src2);
    ILVR_B2_SB(src1, src0, src2, src1, src10_r, src21_r);

    LD_SB2(src, src_stride, src3, src4);
    XORI_B2_128_SB(src3, src4);
    ILVR_B2_SB(src3, src2, src4, src3, src32_r, src43_r);
    dst0_r = const_vec;
    DPADD_SB2_SH(src10_r, src32_r, filt0, filt1, dst0_r, dst0_r);
    dst1_r = const_vec;
    DPADD_SB2_SH(src21_r, src43_r, filt0, filt1, dst1_r, dst1_r);

    ST_SH2(dst0_r, dst1_r, dst, dst_stride);
}

static void hevc_vt_4t_8x6_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride,
                               WORD8 *filter)
{
    v16i8 src0, src1, src2, src3, src4, src10_r, src32_r, src21_r, src43_r;
    v8i16 dst0_r, dst1_r, filt0, filt1, filter_vec, const_vec;

    src -= src_stride;
    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    filter_vec = LD_SH(filter);
    SPLATI_H2_SH(filter_vec, 0, 1, filt0, filt1);

    LD_SB3(src, src_stride, src0, src1, src2);
    src += (3 * src_stride);
    XORI_B3_128_SB(src0, src1, src2);
    ILVR_B2_SB(src1, src0, src2, src1, src10_r, src21_r);

    LD_SB2(src, src_stride, src3, src4);
    src += (2 * src_stride);
    XORI_B2_128_SB(src3, src4);

    ILVR_B2_SB(src3, src2, src4, src3, src32_r, src43_r);
    dst0_r = const_vec;
    DPADD_SB2_SH(src10_r, src32_r, filt0, filt1, dst0_r, dst0_r);
    dst1_r = const_vec;
    DPADD_SB2_SH(src21_r, src43_r, filt0, filt1, dst1_r, dst1_r);

    ST_SH2(dst0_r, dst1_r, dst, dst_stride);
    dst += (2 * dst_stride);

    LD_SB2(src, src_stride, src1, src2);
    src += (2 * src_stride);
    XORI_B2_128_SB(src1, src2);

    ILVR_B2_SB(src1, src4, src2, src1, src10_r, src21_r);
    dst0_r = const_vec;
    DPADD_SB2_SH(src32_r, src10_r, filt0, filt1, dst0_r, dst0_r);
    dst1_r = const_vec;
    DPADD_SB2_SH(src43_r, src21_r, filt0, filt1, dst1_r, dst1_r);

    ST_SH2(dst0_r, dst1_r, dst, dst_stride);
    dst += (2 * dst_stride);

    LD_SB2(src, src_stride, src3, src4);
    XORI_B2_128_SB(src3, src4);

    ILVR_B2_SB(src3, src2, src4, src3, src32_r, src43_r);
    dst0_r = const_vec;
    DPADD_SB2_SH(src10_r, src32_r, filt0, filt1, dst0_r, dst0_r);
    dst1_r = const_vec;
    DPADD_SB2_SH(src21_r, src43_r, filt0, filt1, dst1_r, dst1_r);

    ST_SH2(dst0_r, dst1_r, dst, dst_stride);
}

static void hevc_vt_4t_8x4multiple_msa(UWORD8 *src, WORD32 src_stride,
                                       WORD16 *dst, WORD32 dst_stride,
                                       WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, src5;
    v16i8 src10_r, src32_r, src21_r, src43_r;
    v8i16 dst0_r, dst1_r, filt0, filt1, filter_vec, const_vec;

    src -= src_stride;
    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    filter_vec = LD_SH(filter);
    SPLATI_H2_SH(filter_vec, 0, 1, filt0, filt1);

    LD_SB3(src, src_stride, src0, src1, src2);
    src += (3 * src_stride);
    XORI_B3_128_SB(src0, src1, src2);
    ILVR_B2_SB(src1, src0, src2, src1, src10_r, src21_r);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB2(src, src_stride, src3, src4);
        src += (2 * src_stride);
        XORI_B2_128_SB(src3, src4);
        ILVR_B2_SB(src3, src2, src4, src3, src32_r, src43_r);
        dst0_r = const_vec;
        DPADD_SB2_SH(src10_r, src32_r, filt0, filt1, dst0_r, dst0_r);
        dst1_r = const_vec;
        DPADD_SB2_SH(src21_r, src43_r, filt0, filt1, dst1_r, dst1_r);

        ST_SH2(dst0_r, dst1_r, dst, dst_stride);
        dst += (2 * dst_stride);

        LD_SB2(src, src_stride, src5, src2);
        src += (2 * src_stride);
        XORI_B2_128_SB(src5, src2);
        ILVR_B2_SB(src5, src4, src2, src5, src10_r, src21_r);
        dst0_r = const_vec;
        DPADD_SB2_SH(src32_r, src10_r, filt0, filt1, dst0_r, dst0_r);
        dst1_r = const_vec;
        DPADD_SB2_SH(src43_r, src21_r, filt0, filt1, dst1_r, dst1_r);

        ST_SH2(dst0_r, dst1_r, dst, dst_stride);
        dst += (2 * dst_stride);
    }
}

static void hevc_vt_4t_8w_msa(UWORD8 *src, WORD32 src_stride,
                              WORD16 *dst, WORD32 dst_stride,
                              WORD8 *filter, WORD32 height)
{
    if(2 == height)
    {
        hevc_vt_4t_8x2_msa(src, src_stride, dst, dst_stride, filter);
    }
    else if(6 == height)
    {
        hevc_vt_4t_8x6_msa(src, src_stride, dst, dst_stride, filter);
    }
    else
    {
        hevc_vt_4t_8x4multiple_msa(src, src_stride, dst, dst_stride,
                                   filter, height);
    }
}

static void hevc_vt_4t_12w_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride,
                               WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, src5, src10_r, src32_r, src21_r;
    v16i8 src10_l, src32_l, src54_l, src21_l, src43_l, src65_l, src43_r;
    v16i8 src2110, src4332;
    v8i16 dst0_r, dst1_r, dst2_r, dst3_r, dst0_l, dst1_l, filt0, filt1;
    v8i16 filter_vec, const_vec;

    src -= (1 * src_stride);
    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    filter_vec = LD_SH(filter);
    SPLATI_H2_SH(filter_vec, 0, 1, filt0, filt1);

    LD_SB3(src, src_stride, src0, src1, src2);
    src += (3 * src_stride);
    XORI_B3_128_SB(src0, src1, src2);
    ILVR_B2_SB(src1, src0, src2, src1, src10_r, src21_r);
    ILVL_B2_SB(src1, src0, src2, src1, src10_l, src21_l);
    src2110 = (v16i8) __msa_ilvr_d((v2i64) src21_l, (v2i64) src10_l);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB2(src, src_stride, src3, src4);
        src += (2 * src_stride);
        XORI_B2_128_SB(src3, src4);
        ILVR_B2_SB(src3, src2, src4, src3, src32_r, src43_r);
        ILVL_B2_SB(src3, src2, src4, src3, src32_l, src43_l);
        src4332 = (v16i8) __msa_ilvr_d((v2i64) src43_l, (v2i64) src32_l);
        dst0_r = const_vec;
        DPADD_SB2_SH(src10_r, src32_r, filt0, filt1, dst0_r, dst0_r);
        dst1_r = const_vec;
        DPADD_SB2_SH(src21_r, src43_r, filt0, filt1, dst1_r, dst1_r);
        dst0_l = const_vec;
        DPADD_SB2_SH(src2110, src4332, filt0, filt1, dst0_l, dst0_l);

        LD_SB2(src, src_stride, src5, src2);
        src += (2 * src_stride);
        XORI_B2_128_SB(src5, src2);
        ILVR_B2_SB(src5, src4, src2, src5, src10_r, src21_r);
        ILVL_B2_SB(src5, src4, src2, src5, src54_l, src65_l);
        src2110 = (v16i8) __msa_ilvr_d((v2i64) src65_l, (v2i64) src54_l);
        dst2_r = const_vec;
        DPADD_SB2_SH(src32_r, src10_r, filt0, filt1, dst2_r, dst2_r);
        dst3_r = const_vec;
        DPADD_SB2_SH(src43_r, src21_r, filt0, filt1, dst3_r, dst3_r);
        dst1_l = const_vec;
        DPADD_SB2_SH(src4332, src2110, filt0, filt1, dst1_l, dst1_l);

        ST_SH4(dst0_r, dst1_r, dst2_r, dst3_r, dst, dst_stride);
        ST8x4_UB(dst0_l, dst1_l, dst + 8, (2 * dst_stride));
        dst += (4 * dst_stride);
    }
}

static void hevc_vt_4t_16x2mult_msa(UWORD8 *src, WORD32 src_stride,
                                    WORD16 *dst, WORD32 dst_stride,
                                    WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, src10_r, src32_r;
    v16i8 src21_r, src43_r, src10_l, src32_l, src21_l, src43_l;
    v8i16 dst0_r, dst1_r, dst0_l, dst1_l, filt0, filt1, filter_vec, const_vec;

    src -= src_stride;
    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    filter_vec = LD_SH(filter);
    SPLATI_H2_SH(filter_vec, 0, 1, filt0, filt1);

    LD_SB3(src, src_stride, src0, src1, src2);
    src += (3 * src_stride);
    XORI_B3_128_SB(src0, src1, src2);
    ILVR_B2_SB(src1, src0, src2, src1, src10_r, src21_r);
    ILVL_B2_SB(src1, src0, src2, src1, src10_l, src21_l);

    for(loop_cnt = (height >> 1); loop_cnt--;)
    {
        LD_SB2(src, src_stride, src3, src4);
        src += (2 * src_stride);
        XORI_B2_128_SB(src3, src4);
        ILVR_B2_SB(src3, src2, src4, src3, src32_r, src43_r);
        ILVL_B2_SB(src3, src2, src4, src3, src32_l, src43_l);
        dst0_r = const_vec;
        DPADD_SB2_SH(src10_r, src32_r, filt0, filt1, dst0_r, dst0_r);
        dst0_l = const_vec;
        DPADD_SB2_SH(src10_l, src32_l, filt0, filt1, dst0_l, dst0_l);
        dst1_r = const_vec;
        DPADD_SB2_SH(src21_r, src43_r, filt0, filt1, dst1_r, dst1_r);
        dst1_l = const_vec;
        DPADD_SB2_SH(src21_l, src43_l, filt0, filt1, dst1_l, dst1_l);
        ST_SH2(dst0_r, dst0_l, dst, 8);
        dst += dst_stride;
        ST_SH2(dst1_r, dst1_l, dst, 8);
        dst += dst_stride;

        src10_r = src32_r;
        src21_r = src43_r;
        src10_l = src32_l;
        src21_l = src43_l;
        src2 = src4;
    }
}

static void hevc_vt_4t_16x4mult_msa(UWORD8 *src, WORD32 src_stride,
                                    WORD16 *dst, WORD32 dst_stride,
                                    WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, src5, src10_r, src32_r, src21_r;
    v16i8 src43_r, src10_l, src32_l, src21_l, src43_l;
    v8i16 dst0_r, dst1_r, dst0_l, dst1_l, filt0, filt1, filter_vec, const_vec;

    src -= src_stride;
    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    filter_vec = LD_SH(filter);
    SPLATI_H2_SH(filter_vec, 0, 1, filt0, filt1);

    LD_SB3(src, src_stride, src0, src1, src2);
    src += (3 * src_stride);
    XORI_B3_128_SB(src0, src1, src2);
    ILVR_B2_SB(src1, src0, src2, src1, src10_r, src21_r);
    ILVL_B2_SB(src1, src0, src2, src1, src10_l, src21_l);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB2(src, src_stride, src3, src4);
        src += (2 * src_stride);
        XORI_B2_128_SB(src3, src4);
        ILVR_B2_SB(src3, src2, src4, src3, src32_r, src43_r);
        ILVL_B2_SB(src3, src2, src4, src3, src32_l, src43_l);
        dst0_r = const_vec;
        DPADD_SB2_SH(src10_r, src32_r, filt0, filt1, dst0_r, dst0_r);
        dst0_l = const_vec;
        DPADD_SB2_SH(src10_l, src32_l, filt0, filt1, dst0_l, dst0_l);
        dst1_r = const_vec;
        DPADD_SB2_SH(src21_r, src43_r, filt0, filt1, dst1_r, dst1_r);
        dst1_l = const_vec;
        DPADD_SB2_SH(src21_l, src43_l, filt0, filt1, dst1_l, dst1_l);
        ST_SH2(dst0_r, dst0_l, dst, 8);
        dst += dst_stride;
        ST_SH2(dst1_r, dst1_l, dst, 8);
        dst += dst_stride;

        LD_SB2(src, src_stride, src5, src2);
        src += (2 * src_stride);
        XORI_B2_128_SB(src5, src2);
        ILVR_B2_SB(src5, src4, src2, src5, src10_r, src21_r);
        ILVL_B2_SB(src5, src4, src2, src5, src10_l, src21_l);
        dst0_r = const_vec;
        DPADD_SB2_SH(src32_r, src10_r, filt0, filt1, dst0_r, dst0_r);
        dst0_l = const_vec;
        DPADD_SB2_SH(src32_l, src10_l, filt0, filt1, dst0_l, dst0_l);
        dst1_r = const_vec;
        DPADD_SB2_SH(src43_r, src21_r, filt0, filt1, dst1_r, dst1_r);
        dst1_l = const_vec;
        DPADD_SB2_SH(src43_l, src21_l, filt0, filt1, dst1_l, dst1_l);
        ST_SH2(dst0_r, dst0_l, dst, 8);
        dst += dst_stride;
        ST_SH2(dst1_r, dst1_l, dst, 8);
        dst += dst_stride;
    }
}

static void hevc_vt_4t_16w_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride,
                               WORD8 *filter, WORD32 height)
{
    if(height % 4 == 0)
    {
        hevc_vt_4t_16x4mult_msa(src, src_stride, dst, dst_stride, filter,
                                height);
    }
    else
    {
        hevc_vt_4t_16x2mult_msa(src, src_stride, dst, dst_stride, filter,
                                height);
    }
}

static void hevc_vt_4t_24w_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride,
                               WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v16i8 src11, src10_r, src32_r, src76_r, src98_r, src21_r, src43_r, src87_r;
    v16i8 src109_r, src10_l, src32_l, src21_l, src43_l;
    v8i16 dst0_r, dst1_r, dst2_r, dst3_r, dst0_l, dst1_l, filt0, filt1;
    v8i16 filter_vec, const_vec;

    src -= src_stride;
    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    filter_vec = LD_SH(filter);
    SPLATI_H2_SH(filter_vec, 0, 1, filt0, filt1);

    LD_SB3(src, src_stride, src0, src1, src2);
    XORI_B3_128_SB(src0, src1, src2);
    ILVR_B2_SB(src1, src0, src2, src1, src10_r, src21_r);
    ILVL_B2_SB(src1, src0, src2, src1, src10_l, src21_l);

    LD_SB3(src + 16, src_stride, src6, src7, src8);
    src += (3 * src_stride);
    XORI_B3_128_SB(src6, src7, src8);
    ILVR_B2_SB(src7, src6, src8, src7, src76_r, src87_r);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB2(src, src_stride, src3, src4);
        XORI_B2_128_SB(src3, src4);
        ILVR_B2_SB(src3, src2, src4, src3, src32_r, src43_r);
        ILVL_B2_SB(src3, src2, src4, src3, src32_l, src43_l);

        LD_SB2(src + 16, src_stride, src9, src10);
        src += (2 * src_stride);
        XORI_B2_128_SB(src9, src10);
        ILVR_B2_SB(src9, src8, src10, src9, src98_r, src109_r);

        dst0_r = const_vec;
        DPADD_SB2_SH(src10_r, src32_r, filt0, filt1, dst0_r, dst0_r);
        dst0_l = const_vec;
        DPADD_SB2_SH(src10_l, src32_l, filt0, filt1, dst0_l, dst0_l);
        dst1_r = const_vec;
        DPADD_SB2_SH(src21_r, src43_r, filt0, filt1, dst1_r, dst1_r);
        dst1_l = const_vec;
        DPADD_SB2_SH(src21_l, src43_l, filt0, filt1, dst1_l, dst1_l);
        dst2_r = const_vec;
        DPADD_SB2_SH(src76_r, src98_r, filt0, filt1, dst2_r, dst2_r);
        dst3_r = const_vec;
        DPADD_SB2_SH(src87_r, src109_r, filt0, filt1, dst3_r, dst3_r);

        ST_SH2(dst0_r, dst0_l, dst, 8);
        ST_SH(dst2_r, dst + 16);
        dst += dst_stride;
        ST_SH2(dst1_r, dst1_l, dst, 8);
        ST_SH(dst3_r, dst + 16);
        dst += dst_stride;

        LD_SB2(src, src_stride, src5, src2);
        XORI_B2_128_SB(src5, src2);
        ILVR_B2_SB(src5, src4, src2, src5, src10_r, src21_r);
        ILVL_B2_SB(src5, src4, src2, src5, src10_l, src21_l);

        LD_SB2(src + 16, src_stride, src11, src8);
        src += (2 * src_stride);
        XORI_B2_128_SB(src11, src8);
        ILVR_B2_SB(src11, src10, src8, src11, src76_r, src87_r);

        dst0_r = const_vec;
        DPADD_SB2_SH(src32_r, src10_r, filt0, filt1, dst0_r, dst0_r);
        dst0_l = const_vec;
        DPADD_SB2_SH(src32_l, src10_l, filt0, filt1, dst0_l, dst0_l);
        dst1_r = const_vec;
        DPADD_SB2_SH(src43_r, src21_r, filt0, filt1, dst1_r, dst1_r);
        dst1_l = const_vec;
        DPADD_SB2_SH(src43_l, src21_l, filt0, filt1, dst1_l, dst1_l);
        dst2_r = const_vec;
        DPADD_SB2_SH(src98_r, src76_r, filt0, filt1, dst2_r, dst2_r);
        dst3_r = const_vec;
        DPADD_SB2_SH(src109_r, src87_r, filt0, filt1, dst3_r, dst3_r);

        ST_SH2(dst0_r, dst0_l, dst, 8);
        ST_SH(dst2_r, dst + 16);
        dst += dst_stride;
        ST_SH2(dst1_r, dst1_l, dst, 8);
        ST_SH(dst3_r, dst + 16);
        dst += dst_stride;
    }
}

static void hevc_vt_4t_32w_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride,
                               WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v16i8 src11, src10_r, src32_r, src76_r, src98_r, src21_r, src43_r;
    v16i8 src87_r, src109_r, src10_l, src32_l, src76_l, src98_l, src21_l;
    v16i8 src43_l, src87_l, src109_l;
    v8i16 dst0_r, dst1_r, dst2_r, dst3_r, dst0_l, dst1_l, dst2_l, dst3_l;
    v8i16 filt0, filt1, filter_vec, const_vec;

    src -= src_stride;
    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    filter_vec = LD_SH(filter);
    SPLATI_H2_SH(filter_vec, 0, 1, filt0, filt1);

    LD_SB3(src, src_stride, src0, src1, src2);
    XORI_B3_128_SB(src0, src1, src2);
    ILVR_B2_SB(src1, src0, src2, src1, src10_r, src21_r);
    ILVL_B2_SB(src1, src0, src2, src1, src10_l, src21_l);

    LD_SB3(src + 16, src_stride, src6, src7, src8);
    src += (3 * src_stride);
    XORI_B3_128_SB(src6, src7, src8);
    ILVR_B2_SB(src7, src6, src8, src7, src76_r, src87_r);
    ILVL_B2_SB(src7, src6, src8, src7, src76_l, src87_l);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB2(src, src_stride, src3, src4);
        XORI_B2_128_SB(src3, src4);
        ILVR_B2_SB(src3, src2, src4, src3, src32_r, src43_r);
        ILVL_B2_SB(src3, src2, src4, src3, src32_l, src43_l);

        LD_SB2(src + 16, src_stride, src9, src10);
        src += (2 * src_stride);
        XORI_B2_128_SB(src9, src10);
        ILVR_B2_SB(src9, src8, src10, src9, src98_r, src109_r);
        ILVL_B2_SB(src9, src8, src10, src9, src98_l, src109_l);

        dst0_r = const_vec;
        DPADD_SB2_SH(src10_r, src32_r, filt0, filt1, dst0_r, dst0_r);
        dst0_l = const_vec;
        DPADD_SB2_SH(src10_l, src32_l, filt0, filt1, dst0_l, dst0_l);
        dst1_r = const_vec;
        DPADD_SB2_SH(src21_r, src43_r, filt0, filt1, dst1_r, dst1_r);
        dst1_l = const_vec;
        DPADD_SB2_SH(src21_l, src43_l, filt0, filt1, dst1_l, dst1_l);
        dst2_r = const_vec;
        DPADD_SB2_SH(src76_r, src98_r, filt0, filt1, dst2_r, dst2_r);
        dst2_l = const_vec;
        DPADD_SB2_SH(src76_l, src98_l, filt0, filt1, dst2_l, dst2_l);
        dst3_r = const_vec;
        DPADD_SB2_SH(src87_r, src109_r, filt0, filt1, dst3_r, dst3_r);
        dst3_l = const_vec;
        DPADD_SB2_SH(src87_l, src109_l, filt0, filt1, dst3_l, dst3_l);

        ST_SH4(dst0_r, dst0_l, dst2_r, dst2_l, dst, 8);
        dst += dst_stride;
        ST_SH4(dst1_r, dst1_l, dst3_r, dst3_l, dst, 8);
        dst += dst_stride;

        LD_SB2(src, src_stride, src5, src2);
        XORI_B2_128_SB(src5, src2);
        ILVR_B2_SB(src5, src4, src2, src5, src10_r, src21_r);
        ILVL_B2_SB(src5, src4, src2, src5, src10_l, src21_l);

        LD_SB2(src + 16, src_stride, src11, src8);
        src += (2 * src_stride);
        XORI_B2_128_SB(src11, src8);
        ILVR_B2_SB(src11, src10, src8, src11, src76_r, src87_r);
        ILVL_B2_SB(src11, src10, src8, src11, src76_l, src87_l);

        dst0_r = const_vec;
        DPADD_SB2_SH(src32_r, src10_r, filt0, filt1, dst0_r, dst0_r);
        dst0_l = const_vec;
        DPADD_SB2_SH(src32_l, src10_l, filt0, filt1, dst0_l, dst0_l);
        dst1_r = const_vec;
        DPADD_SB2_SH(src43_r, src21_r, filt0, filt1, dst1_r, dst1_r);
        dst1_l = const_vec;
        DPADD_SB2_SH(src43_l, src21_l, filt0, filt1, dst1_l, dst1_l);
        dst2_r = const_vec;
        DPADD_SB2_SH(src98_r, src76_r, filt0, filt1, dst2_r, dst2_r);
        dst2_l = const_vec;
        DPADD_SB2_SH(src98_l, src76_l, filt0, filt1, dst2_l, dst2_l);
        dst3_r = const_vec;
        DPADD_SB2_SH(src109_r, src87_r, filt0, filt1, dst3_r, dst3_r);
        dst3_l = const_vec;
        DPADD_SB2_SH(src109_l, src87_l, filt0, filt1, dst3_l, dst3_l);

        ST_SH4(dst0_r, dst0_l, dst2_r, dst2_l, dst, 8);
        dst += dst_stride;
        ST_SH4(dst1_r, dst1_l, dst3_r, dst3_l, dst, 8);
        dst += dst_stride;
    }
}

static void hevc_ilv_hz_4t_2w_msa(UWORD8 *src, WORD32 src_stride,
                                  WORD16 *dst, WORD32 dst_stride,
                                  WORD8 *filter, WORD32 height)
{
    if(4 == height || 7 == height)
    {
        hevc_ilv_hz_4t_2x4_msa(src, src_stride, dst, dst_stride, filter,
                               height);
    }
    else if(8 == height || 11 == height)
    {
        hevc_ilv_hz_4t_2x8_msa(src, src_stride, dst, dst_stride, filter,
                               height);
    }
}

static void hevc_ilv_hz_4t_4w_msa(UWORD8 *src, WORD32 src_stride,
                                  WORD16 *dst, WORD32 dst_stride,
                                  WORD8 *filter, WORD32 height)
{
    if(2 == height || 5 == height)
    {
        hevc_ilv_hz_4t_4x2_msa(src, src_stride, dst, dst_stride, filter,
                               height);
    }
    else if(4 == height || 7 == height)
    {
        hevc_ilv_hz_4t_4x4mult_msa(src, src_stride, dst, dst_stride, filter,
                                   height);
    }
    else if(8 == height || 11 == height)
    {
        hevc_ilv_hz_4t_4x4mult_msa(src, src_stride, dst, dst_stride, filter,
                                   height);
    }
    else if(16 == height || 19 == height)
    {
        hevc_ilv_hz_4t_4x4mult_msa(src, src_stride, dst, dst_stride, filter,
                                   height);
    }
}

static void hevc_ilv_hz_4t_8w_msa(UWORD8 *src, WORD32 src_stride,
                                  WORD16 *dst, WORD32 dst_stride,
                                  WORD8 *filter, WORD32 height)
{
    if ((2 == height) || (5 == height) || (6 == height) || (9 == height))
    {
        hevc_ilv_hz_4t_8x2mult_msa(src, src_stride, dst, dst_stride, filter,
                                   height);
    }
    else
    {
        hevc_ilv_hz_4t_8x4mult_msa(src, src_stride, dst, dst_stride, filter,
                                   height);
    }
}

static void hevc_vt_uni_4t_4x2_w16inp_msa(WORD16 *src, WORD32 src_stride,
                                          UWORD8 *dst, WORD32 dst_stride,
                                          WORD8 *filter)
{
    v16i8 vec0;
    v8i16 src0, src1, src2, src3, src4, filter_vec;
    v8i16 dst10_r, dst32_r, dst21_r, dst43_r;
    v4i32 filt0, filt1, dst0_r, dst1_r;

    src -= src_stride;
    filter_vec = LD_SH(filter);
    vec0 = CLTI_S_B(filter_vec, 0);
    filter_vec = (v8i16) __msa_ilvr_b(vec0, (v16i8) filter_vec);
    SPLATI_W2_SW(filter_vec, 0, filt0, filt1);

    LD_SH2(src, src_stride, src0, src1);
    src2 = LD_SH(src + 2 * src_stride);
    src += (3 * src_stride);
    ILVR_H2_SH(src1, src0, src2, src1, dst10_r, dst21_r);
    LD_SH2(src, src_stride, src3, src4);
    dst32_r = __msa_ilvr_h(src3, src2);
    dst0_r = HEVC_FILT_4TAP(dst10_r, dst32_r, filt0, filt1);
    dst0_r = SRAI_W(dst0_r, 6);
    dst43_r = __msa_ilvr_h(src4, src3);
    dst1_r = HEVC_FILT_4TAP(dst21_r, dst43_r, filt0, filt1);
    dst1_r = SRAI_W(dst1_r, 6);

    dst0_r = (v4i32) __msa_pckev_h((v8i16) dst1_r, (v8i16) dst0_r);
    dst0_r = (v4i32) __msa_srari_h((v8i16) dst0_r, 6);
    dst0_r = (v4i32) CLIP_SH_0_255(dst0_r);
    dst0_r = (v4i32) __msa_pckev_b((v16i8) dst0_r, (v16i8) dst0_r);
    ST4x2_UB(dst0_r, dst, dst_stride);
}

static void hevc_vt_uni_4t_4x4_w16inp_msa(WORD16 *src, WORD32 src_stride,
                                          UWORD8 *dst, WORD32 dst_stride,
                                          WORD8 *filter)
{
    v16i8 vec0;
    v8i16 src0, src1, src2, src3, src4, src5, src6, out0_r, out1_r;
    v8i16 filter_vec, dst10_r, dst32_r, dst21_r, dst43_r;
    v4i32 dst0_r, dst1_r, dst2_r, dst3_r, filt0, filt1;

    src -= src_stride;
    filter_vec = LD_SH(filter);
    vec0 = CLTI_S_B(filter_vec, 0);
    filter_vec = (v8i16) __msa_ilvr_b(vec0, (v16i8) filter_vec);
    SPLATI_W2_SW(filter_vec, 0, filt0, filt1);

    LD_SH2(src, src_stride, src0, src1);
    src2 = LD_SH(src + 2 * src_stride);
    src += (3 * src_stride);
    ILVR_H2_SH(src1, src0, src2, src1, dst10_r, dst21_r);
    LD_SH4(src, src_stride, src3, src4, src5, src6);
    dst32_r = __msa_ilvr_h(src3, src2);
    dst0_r = HEVC_FILT_4TAP(dst10_r, dst32_r, filt0, filt1);
    dst0_r = SRAI_W(dst0_r, 6);
    dst43_r = __msa_ilvr_h(src4, src3);
    dst1_r = HEVC_FILT_4TAP(dst21_r, dst43_r, filt0, filt1);
    dst1_r = SRAI_W(dst1_r, 6);
    dst10_r = __msa_ilvr_h(src5, src4);
    dst2_r = HEVC_FILT_4TAP(dst32_r, dst10_r, filt0, filt1);
    dst2_r = SRAI_W(dst2_r, 6);
    dst21_r = __msa_ilvr_h(src6, src5);
    dst3_r = HEVC_FILT_4TAP(dst43_r, dst21_r, filt0, filt1);
    dst3_r = SRAI_W(dst3_r, 6);

    PCKEV_H2_SH(dst1_r, dst0_r, dst3_r, dst2_r, out0_r, out1_r);
    SRARI_H2_SH(out0_r, out1_r, 6);
    CLIP_SH2_0_255(out0_r, out1_r);
    out0_r = (v8i16) __msa_pckev_b((v16i8) out1_r, (v16i8) out0_r);
    ST4x4_UB(out0_r, out0_r, 0, 1, 2, 3, dst, dst_stride);
}

static void hevc_vt_uni_4t_4x8mult_w16inp_msa(WORD16 *src, WORD32 src_stride,
                                              UWORD8 *dst, WORD32 dst_stride,
                                              WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    v16i8 vec0;
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9;
    v8i16 filter_vec, dst10_r, dst32_r, dst54_r, dst76_r, dst21_r;
    v8i16 dst43_r, dst65_r, dst87_r, out0_r, out1_r, out2_r, out3_r;
    v4i32 dst0_r, dst1_r, dst2_r, dst3_r, dst4_r, dst5_r, dst6_r, dst7_r;
    v4i32 filt0, filt1;

    src -= src_stride;
    filter_vec = LD_SH(filter);
    vec0 = CLTI_S_B(filter_vec, 0);
    filter_vec = (v8i16) __msa_ilvr_b(vec0, (v16i8) filter_vec);
    SPLATI_W2_SW(filter_vec, 0, filt0, filt1);

    LD_SH2(src, src_stride, src0, src1);
    src2 = LD_SH(src + 2 * src_stride);
    src += (3 * src_stride);
    ILVR_H2_SH(src1, src0, src2, src1, dst10_r, dst21_r);

    for(loop_cnt = height >> 3; loop_cnt--;)
    {
        LD_SH4(src, src_stride, src3, src4, src5, src6);
        src += (4 * src_stride);
        dst32_r = __msa_ilvr_h(src3, src2);
        dst0_r = HEVC_FILT_4TAP(dst10_r, dst32_r, filt0, filt1);
        dst0_r = SRAI_W(dst0_r, 6);
        dst43_r = __msa_ilvr_h(src4, src3);
        dst1_r = HEVC_FILT_4TAP(dst21_r, dst43_r, filt0, filt1);
        dst1_r = SRAI_W(dst1_r, 6);
        dst54_r = __msa_ilvr_h(src5, src4);
        dst2_r = HEVC_FILT_4TAP(dst32_r, dst54_r, filt0, filt1);
        dst2_r = SRAI_W(dst2_r, 6);
        dst65_r = __msa_ilvr_h(src6, src5);
        dst3_r = HEVC_FILT_4TAP(dst43_r, dst65_r, filt0, filt1);
        dst3_r = SRAI_W(dst3_r, 6);

        LD_SH4(src, src_stride, src7, src8, src9, src2);
        src += (4 * src_stride);
        dst76_r = __msa_ilvr_h(src7, src6);
        dst4_r = HEVC_FILT_4TAP(dst54_r, dst76_r, filt0, filt1);
        dst4_r = SRAI_W(dst4_r, 6);
        dst87_r = __msa_ilvr_h(src8, src7);
        dst5_r = HEVC_FILT_4TAP(dst65_r, dst87_r, filt0, filt1);
        dst5_r = SRAI_W(dst5_r, 6);
        dst10_r = __msa_ilvr_h(src9, src8);
        dst6_r = HEVC_FILT_4TAP(dst76_r, dst10_r, filt0, filt1);
        dst6_r = SRAI_W(dst6_r, 6);
        dst21_r = __msa_ilvr_h(src2, src9);
        dst7_r = HEVC_FILT_4TAP(dst87_r, dst21_r, filt0, filt1);
        dst7_r = SRAI_W(dst7_r, 6);

        PCKEV_H4_SH(dst1_r, dst0_r, dst3_r, dst2_r, dst5_r, dst4_r, dst7_r,
                    dst6_r, out0_r, out1_r, out2_r, out3_r);
        SRARI_H4_SH(out0_r, out1_r, out2_r, out3_r, 6);
        CLIP_SH4_0_255(out0_r, out1_r, out2_r, out3_r);
        PCKEV_B2_SH(out1_r, out0_r, out3_r, out2_r, out0_r, out1_r);
        ST4x8_UB(out0_r, out1_r, dst, dst_stride);
        dst += (8 * dst_stride);
    }
}

static void hevc_vt_uni_4t_8x2_w16inp_msa(WORD16 *src, WORD32 src_stride,
                                          UWORD8 *dst, WORD32 dst_stride,
                                          WORD8 *filter)
{
    v16i8 vec0;
    v8i16 src0, src1, src2, src3, src4, filter_vec, out0_r, out1_r;
    v8i16 dst10_r, dst32_r, dst21_r, dst43_r;
    v8i16 dst10_l, dst32_l, dst21_l, dst43_l;
    v4i32 filt0, filt1, dst0_r, dst0_l, dst1_r, dst1_l;

    src -= src_stride;
    filter_vec = LD_SH(filter);
    vec0 = CLTI_S_B(filter_vec, 0);
    filter_vec = (v8i16) __msa_ilvr_b(vec0, (v16i8) filter_vec);
    SPLATI_W2_SW(filter_vec, 0, filt0, filt1);

    LD_SH2(src, src_stride, src0, src1);
    src2 = LD_SH(src + 2 * src_stride);
    src += (3 * src_stride);
    ILVRL_H2_SH(src1, src0, dst10_r, dst10_l);
    ILVRL_H2_SH(src2, src1, dst21_r, dst21_l);
    LD_SH2(src, src_stride, src3, src4);
    ILVRL_H2_SH(src3, src2, dst32_r, dst32_l);
    dst0_r = HEVC_FILT_4TAP(dst10_r, dst32_r, filt0, filt1);
    dst0_l = HEVC_FILT_4TAP(dst10_l, dst32_l, filt0, filt1);
    SRAI_W2_SW(dst0_r, dst0_l, 6);
    ILVRL_H2_SH(src4, src3, dst43_r, dst43_l);
    dst1_r = HEVC_FILT_4TAP(dst21_r, dst43_r, filt0, filt1);
    dst1_l = HEVC_FILT_4TAP(dst21_l, dst43_l, filt0, filt1);
    SRAI_W2_SW(dst1_r, dst1_l, 6);

    PCKEV_H2_SH(dst0_l, dst0_r, dst1_l, dst1_r, out0_r, out1_r);
    SRARI_H2_SH(out0_r, out1_r, 6);
    CLIP_SH2_0_255(out0_r, out1_r);
    out0_r = (v8i16) __msa_pckev_b((v16i8) out1_r, (v16i8) out0_r);
    ST8x2_UB(out0_r, dst, dst_stride);
}

static void hevc_vt_uni_4t_8x4mult_w16inp_msa(WORD16 *src, WORD32 src_stride,
                                              UWORD8 *dst, WORD32 dst_stride,
                                              WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    v16i8 vec0;
    v8i16 src0, src1, src2, src3, src4, src5, filter_vec, dst10_r, dst32_r;
    v8i16 dst21_r, dst43_r, dst10_l, dst32_l, dst21_l, dst43_l;
    v8i16 out0_r, out1_r, out2_r, out3_r;
    v4i32 dst0_r, dst0_l, dst1_r, dst1_l, dst2_r, dst2_l, dst3_r, dst3_l;
    v4i32 filt0, filt1;

    src -= src_stride;
    filter_vec = LD_SH(filter);
    vec0 = CLTI_S_B(filter_vec, 0);
    filter_vec = (v8i16) __msa_ilvr_b(vec0, (v16i8) filter_vec);
    SPLATI_W2_SW(filter_vec, 0, filt0, filt1);
    LD_SH2(src, src_stride, src0, src1);
    src2 = LD_SH(src + 2 * src_stride);
    src += (3 * src_stride);
    ILVRL_H2_SH(src1, src0, dst10_r, dst10_l);
    ILVRL_H2_SH(src2, src1, dst21_r, dst21_l);

    for(loop_cnt = height >> 2; loop_cnt--;)
    {
        LD_SH2(src, src_stride, src3, src4);
        src += (2 * src_stride);
        ILVRL_H2_SH(src3, src2, dst32_r, dst32_l);
        dst0_r = HEVC_FILT_4TAP(dst10_r, dst32_r, filt0, filt1);
        dst0_l = HEVC_FILT_4TAP(dst10_l, dst32_l, filt0, filt1);
        SRAI_W2_SW(dst0_r, dst0_l, 6);
        ILVRL_H2_SH(src4, src3, dst43_r, dst43_l);
        dst1_r = HEVC_FILT_4TAP(dst21_r, dst43_r, filt0, filt1);
        dst1_l = HEVC_FILT_4TAP(dst21_l, dst43_l, filt0, filt1);
        SRAI_W2_SW(dst1_r, dst1_l, 6);

        LD_SH2(src, src_stride, src5, src2);
        src += (2 * src_stride);
        ILVRL_H2_SH(src5, src4, dst10_r, dst10_l);
        dst2_r = HEVC_FILT_4TAP(dst32_r, dst10_r, filt0, filt1);
        dst2_l = HEVC_FILT_4TAP(dst32_l, dst10_l, filt0, filt1);
        SRAI_W2_SW(dst2_r, dst2_l, 6);
        ILVRL_H2_SH(src2, src5, dst21_r, dst21_l);
        dst3_r = HEVC_FILT_4TAP(dst43_r, dst21_r, filt0, filt1);
        dst3_l = HEVC_FILT_4TAP(dst43_l, dst21_l, filt0, filt1);
        SRAI_W2_SW(dst3_r, dst3_l, 6);

        PCKEV_H4_SH(dst0_l, dst0_r, dst1_l, dst1_r, dst2_l, dst2_r, dst3_l,
                    dst3_r, out0_r, out1_r, out2_r, out3_r);
        SRARI_H4_SH(out0_r, out1_r, out2_r, out3_r, 6);
        CLIP_SH4_0_255(out0_r, out1_r, out2_r, out3_r);
        PCKEV_B2_SH(out1_r, out0_r, out3_r, out2_r, out0_r, out1_r);
        ST8x4_UB(out0_r, out1_r, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_vt_uni_4t_12x4mult_w16inp_msa(WORD16 *src, WORD32 src_stride,
                                               UWORD8 *dst, WORD32 dst_stride,
                                               WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 vec0;
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v8i16 src11, filter_vec, dst10_r, dst32_r, dst21_r, dst43_r, dst10_l;
    v8i16 dst32_l, dst21_l, dst43_l, dst76_r, dst98_r, dst87_r, dst109_r;
    v8i16 out0_r, out1_r, out2_r, out3_r, out4_r, out5_r;
    v4i32 dst0_r, dst0_l, dst1_r, dst1_l, dst2_r, dst2_l, dst3_r, dst3_l;
    v4i32 dst4_r, dst5_r, dst6_r, dst7_r, filt0, filt1;

    src -= src_stride;
    filter_vec = LD_SH(filter);
    vec0 = CLTI_S_B(filter_vec, 0);
    filter_vec = (v8i16) __msa_ilvr_b(vec0, (v16i8) filter_vec);
    SPLATI_W2_SW(filter_vec, 0, filt0, filt1);

    LD_SH2(src, src_stride, src0, src1);
    LD_SH2(src + 8, src_stride, src6, src7);
    src2 = LD_SH(src + 2 * src_stride);
    src8 = LD_SH(src + 2 * src_stride + 8);
    src += (3 * src_stride);
    ILVRL_H2_SH(src1, src0, dst10_r, dst10_l);
    ILVRL_H2_SH(src2, src1, dst21_r, dst21_l);
    ILVR_H2_SH(src7, src6, src8, src7, dst76_r, dst87_r);

    for(loop_cnt = height >> 2; loop_cnt--;)
    {
        LD_SH2(src, src_stride, src3, src4);
        LD_SH2(src + 8, src_stride, src9, src10);
        src += (2 * src_stride);
        ILVRL_H2_SH(src3, src2, dst32_r, dst32_l);
        ILVRL_H2_SH(src4, src3, dst43_r, dst43_l);
        ILVR_H2_SH(src9, src8, src10, src9, dst98_r, dst109_r);
        dst0_r = HEVC_FILT_4TAP(dst10_r, dst32_r, filt0, filt1);
        dst0_l = HEVC_FILT_4TAP(dst10_l, dst32_l, filt0, filt1);
        dst1_r = HEVC_FILT_4TAP(dst21_r, dst43_r, filt0, filt1);
        dst1_l = HEVC_FILT_4TAP(dst21_l, dst43_l, filt0, filt1);
        dst4_r = HEVC_FILT_4TAP(dst76_r, dst98_r, filt0, filt1);
        dst5_r = HEVC_FILT_4TAP(dst87_r, dst109_r, filt0, filt1);
        SRAI_W4_SW(dst0_r, dst0_l, dst1_r, dst1_l, 6);
        SRAI_W2_SW(dst4_r, dst5_r, 6);

        LD_SH2(src, src_stride, src5, src2);
        LD_SH2(src + 8, src_stride, src11, src8);
        src += (2 * src_stride);
        ILVRL_H2_SH(src5, src4, dst10_r, dst10_l);
        ILVRL_H2_SH(src2, src5, dst21_r, dst21_l);
        ILVR_H2_SH(src11, src10, src8, src11, dst76_r, dst87_r);
        dst2_r = HEVC_FILT_4TAP(dst32_r, dst10_r, filt0, filt1);
        dst2_l = HEVC_FILT_4TAP(dst32_l, dst10_l, filt0, filt1);
        dst3_r = HEVC_FILT_4TAP(dst43_r, dst21_r, filt0, filt1);
        dst3_l = HEVC_FILT_4TAP(dst43_l, dst21_l, filt0, filt1);
        dst6_r = HEVC_FILT_4TAP(dst98_r, dst76_r, filt0, filt1);
        dst7_r = HEVC_FILT_4TAP(dst109_r, dst87_r, filt0, filt1);
        SRAI_W4_SW(dst2_r, dst2_l, dst3_r, dst3_l, 6);
        SRAI_W2_SW(dst6_r, dst7_r, 6);

        PCKEV_H4_SH(dst0_l, dst0_r, dst1_l, dst1_r, dst2_l, dst2_r, dst3_l,
                    dst3_r, out0_r, out1_r, out2_r, out3_r);
        PCKEV_H2_SH(dst5_r, dst4_r, dst7_r, dst6_r, out4_r, out5_r);
        SRARI_H4_SH(out0_r, out1_r, out2_r, out3_r, 6);
        SRARI_H2_SH(out4_r, out5_r, 6);
        CLIP_SH4_0_255(out0_r, out1_r, out2_r, out3_r);
        CLIP_SH2_0_255(out4_r, out5_r);
        PCKEV_B2_SH(out1_r, out0_r, out3_r, out2_r, out0_r, out1_r);
        out2_r = (v8i16) __msa_pckev_b((v16i8) out5_r, (v16i8) out4_r);
        ST12x4_UB(out0_r, out1_r, out2_r, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_vt_uni_4t_16x2mult_w16inp_msa(WORD16 *src, WORD32 src_stride,
                                               UWORD8 *dst, WORD32 dst_stride,
                                               WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 vec0;
    v8i16 src0, src1, src2, src3, src4, src6, src7, src8, src9, src10;
    v8i16 filter_vec, dst10_r, dst32_r, dst21_r, dst43_r, dst10_l, dst32_l;
    v8i16 dst21_l, dst43_l, dst76_r, dst98_r, dst87_r, dst109_r;
    v8i16 dst76_l, dst98_l, dst87_l, dst109_l, out0_r, out1_r, out2_r, out3_r;
    v4i32 dst0_r, dst0_l, dst1_r, dst1_l, dst2_r, dst2_l, dst3_r, dst3_l;
    v4i32 filt0, filt1;

    src -= src_stride;
    filter_vec = LD_SH(filter);
    vec0 = CLTI_S_B(filter_vec, 0);
    filter_vec = (v8i16) __msa_ilvr_b(vec0, (v16i8) filter_vec);
    SPLATI_W2_SW(filter_vec, 0, filt0, filt1);
    LD_SH2(src, src_stride, src0, src1);
    LD_SH2(src + 8, src_stride, src6, src7);
    src2 = LD_SH(src + 2 * src_stride);
    src8 = LD_SH(src + 2 * src_stride + 8);
    src += (3 * src_stride);
    ILVRL_H2_SH(src1, src0, dst10_r, dst10_l);
    ILVRL_H2_SH(src2, src1, dst21_r, dst21_l);
    ILVRL_H2_SH(src7, src6, dst76_r, dst76_l);
    ILVRL_H2_SH(src8, src7, dst87_r, dst87_l);

    for(loop_cnt = height >> 1; loop_cnt--;)
    {
        LD_SH2(src, src_stride, src3, src4);
        LD_SH2(src + 8, src_stride, src9, src10);
        src += (2 * src_stride);

        ILVRL_H2_SH(src3, src2, dst32_r, dst32_l);
        ILVRL_H2_SH(src4, src3, dst43_r, dst43_l);
        ILVRL_H2_SH(src9, src8, dst98_r, dst98_l);
        ILVRL_H2_SH(src10, src9, dst109_r, dst109_l);
        dst0_r = HEVC_FILT_4TAP(dst10_r, dst32_r, filt0, filt1);
        dst0_l = HEVC_FILT_4TAP(dst10_l, dst32_l, filt0, filt1);
        dst1_r = HEVC_FILT_4TAP(dst21_r, dst43_r, filt0, filt1);
        dst1_l = HEVC_FILT_4TAP(dst21_l, dst43_l, filt0, filt1);
        dst2_r = HEVC_FILT_4TAP(dst76_r, dst98_r, filt0, filt1);
        dst2_l = HEVC_FILT_4TAP(dst76_l, dst98_l, filt0, filt1);
        dst3_r = HEVC_FILT_4TAP(dst87_r, dst109_r, filt0, filt1);
        dst3_l = HEVC_FILT_4TAP(dst87_l, dst109_l, filt0, filt1);
        SRAI_W4_SW(dst0_r, dst0_l, dst1_r, dst1_l, 6);
        SRAI_W4_SW(dst2_r, dst2_l, dst3_r, dst3_l, 6);
        PCKEV_H2_SH(dst0_l, dst0_r, dst1_l, dst1_r, out0_r, out1_r);
        PCKEV_H2_SH(dst2_l, dst2_r, dst3_l, dst3_r, out2_r, out3_r);
        SRARI_H4_SH(out0_r, out1_r, out2_r, out3_r, 6);
        CLIP_SH4_0_255(out0_r, out1_r, out2_r, out3_r);
        PCKEV_B2_SH(out2_r, out0_r, out3_r, out1_r, out0_r, out1_r);
        ST_SH2(out0_r, out1_r, dst, dst_stride);
        dst += (2 * dst_stride);

        src2 = src4;
        src8 = src10;
        dst10_r = dst32_r;
        dst10_l = dst32_l;
        dst21_r = dst43_r;
        dst21_l = dst43_l;
        dst76_r = dst98_r;
        dst76_l = dst98_l;
        dst87_r = dst109_r;
        dst87_l = dst109_l;
    }
}

static void hevc_vt_uni_4t_16multx4mult_w16inp_msa(WORD16 *src,
                                                   WORD32 src_stride,
                                                   UWORD8 *dst,
                                                   WORD32 dst_stride,
                                                   WORD8 *filter,
                                                   WORD32 height,
                                                   WORD32 width)
{
    WORD32 loop_cnt, cnt;
    WORD16 *src_tmp;
    UWORD8 *dst_tmp;
    v16i8 vec0;
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v8i16 src11, filter_vec, dst10_r, dst32_r, dst21_r, dst43_r;
    v8i16 dst10_l, dst32_l, dst21_l, dst43_l, dst76_r, dst98_r, dst87_r;
    v8i16 dst109_r, dst76_l, dst98_l, dst87_l, dst109_l;
    v8i16 out0_r, out1_r, out2_r, out3_r, out4_r, out5_r, out6_r, out7_r;
    v4i32 filt0, filt1;
    v4i32 dst0_r, dst0_l, dst1_r, dst1_l, dst2_r, dst2_l, dst3_r, dst3_l;
    v4i32 dst4_r, dst4_l, dst5_r, dst5_l, dst6_r, dst6_l, dst7_r, dst7_l;

    src -= src_stride;
    filter_vec = LD_SH(filter);
    vec0 = CLTI_S_B(filter_vec, 0);
    filter_vec = (v8i16) __msa_ilvr_b(vec0, (v16i8) filter_vec);
    SPLATI_W2_SW(filter_vec, 0, filt0, filt1);

    for(cnt = width >> 4; cnt--;)
    {
        src_tmp = src;
        dst_tmp = dst;

        LD_SH2(src_tmp, src_stride, src0, src1);
        LD_SH2(src_tmp + 8, src_stride, src6, src7);
        src2 = LD_SH(src_tmp + 2 * src_stride);
        src8 = LD_SH(src_tmp + 2 * src_stride + 8);
        src_tmp += (3 * src_stride);
        ILVRL_H2_SH(src1, src0, dst10_r, dst10_l);
        ILVRL_H2_SH(src2, src1, dst21_r, dst21_l);
        ILVRL_H2_SH(src7, src6, dst76_r, dst76_l);
        ILVRL_H2_SH(src8, src7, dst87_r, dst87_l);

        for(loop_cnt = height >> 2; loop_cnt--;)
        {
            LD_SH2(src_tmp, src_stride, src3, src4);
            LD_SH2(src_tmp + 8, src_stride, src9, src10);
            src_tmp += (2 * src_stride);
            ILVRL_H2_SH(src3, src2, dst32_r, dst32_l);
            ILVRL_H2_SH(src4, src3, dst43_r, dst43_l);
            ILVRL_H2_SH(src9, src8, dst98_r, dst98_l);
            ILVRL_H2_SH(src10, src9, dst109_r, dst109_l);
            dst0_r = HEVC_FILT_4TAP(dst10_r, dst32_r, filt0, filt1);
            dst0_l = HEVC_FILT_4TAP(dst10_l, dst32_l, filt0, filt1);
            dst1_r = HEVC_FILT_4TAP(dst21_r, dst43_r, filt0, filt1);
            dst1_l = HEVC_FILT_4TAP(dst21_l, dst43_l, filt0, filt1);
            dst4_r = HEVC_FILT_4TAP(dst76_r, dst98_r, filt0, filt1);
            dst4_l = HEVC_FILT_4TAP(dst76_l, dst98_l, filt0, filt1);
            dst5_r = HEVC_FILT_4TAP(dst87_r, dst109_r, filt0, filt1);
            dst5_l = HEVC_FILT_4TAP(dst87_l, dst109_l, filt0, filt1);
            SRAI_W4_SW(dst0_r, dst0_l, dst1_r, dst1_l, 6);
            SRAI_W4_SW(dst4_r, dst4_l, dst5_r, dst5_l, 6);

            LD_SH2(src_tmp, src_stride, src5, src2);
            LD_SH2(src_tmp + 8, src_stride, src11, src8);
            src_tmp += (2 * src_stride);
            ILVRL_H2_SH(src5, src4, dst10_r, dst10_l);
            ILVRL_H2_SH(src2, src5, dst21_r, dst21_l);
            ILVRL_H2_SH(src11, src10, dst76_r, dst76_l);
            ILVRL_H2_SH(src8, src11, dst87_r, dst87_l);
            dst2_r = HEVC_FILT_4TAP(dst32_r, dst10_r, filt0, filt1);
            dst2_l = HEVC_FILT_4TAP(dst32_l, dst10_l, filt0, filt1);
            dst3_r = HEVC_FILT_4TAP(dst43_r, dst21_r, filt0, filt1);
            dst3_l = HEVC_FILT_4TAP(dst43_l, dst21_l, filt0, filt1);
            dst6_r = HEVC_FILT_4TAP(dst98_r, dst76_r, filt0, filt1);
            dst6_l = HEVC_FILT_4TAP(dst98_l, dst76_l, filt0, filt1);
            dst7_r = HEVC_FILT_4TAP(dst109_r, dst87_r, filt0, filt1);
            dst7_l = HEVC_FILT_4TAP(dst109_l, dst87_l, filt0, filt1);
            SRAI_W4_SW(dst2_r, dst2_l, dst3_r, dst3_l, 6);
            SRAI_W4_SW(dst6_r, dst6_l, dst7_r, dst7_l, 6);

            PCKEV_H4_SH(dst0_l, dst0_r, dst1_l, dst1_r, dst2_l, dst2_r, dst3_l,
                        dst3_r, out0_r, out1_r, out2_r, out3_r);
            PCKEV_H4_SH(dst4_l, dst4_r, dst5_l, dst5_r, dst6_l, dst6_r, dst7_l,
                        dst7_r, out4_r, out5_r, out6_r, out7_r);

            SRARI_H4_SH(out0_r, out1_r, out2_r, out3_r, 6);
            SRARI_H4_SH(out4_r, out5_r, out6_r, out7_r, 6);
            CLIP_SH4_0_255(out0_r, out1_r, out2_r, out3_r);
            CLIP_SH4_0_255(out4_r, out5_r, out6_r, out7_r);
            PCKEV_B4_SH(out4_r, out0_r, out5_r, out1_r, out6_r, out2_r, out7_r,
                        out3_r, out0_r, out1_r, out2_r, out3_r);
            ST_SH4(out0_r, out1_r, out2_r, out3_r, dst_tmp, dst_stride);
            dst_tmp += (4 * dst_stride);
        }

        src += 16;
        dst += 16;
    }
}

static void hevc_vt_4t_4x2_w16inp_msa(WORD16 *src, WORD32 src_stride,
                                      WORD16 *dst, WORD32 dst_stride,
                                      WORD8 *filter)
{
    v16i8 vec0;
    v8i16 src0, src1, src2, src3, src4, filter_vec;
    v8i16 dst10_r, dst32_r, dst21_r, dst43_r;
    v4i32 dst0_r, dst1_r, filt0, filt1;

    src -= src_stride;
    filter_vec = LD_SH(filter);
    vec0 = CLTI_S_B(filter_vec, 0);
    filter_vec = (v8i16) __msa_ilvr_b(vec0, (v16i8) filter_vec);
    SPLATI_W2_SW(filter_vec, 0, filt0, filt1);

    LD_SH2(src, src_stride, src0, src1);
    src2 = LD_SH(src + 2 * src_stride);
    src += (3 * src_stride);
    ILVR_H2_SH(src1, src0, src2, src1, dst10_r, dst21_r);
    LD_SH2(src, src_stride, src3, src4);
    dst32_r = __msa_ilvr_h(src3, src2);
    dst0_r = HEVC_FILT_4TAP(dst10_r, dst32_r, filt0, filt1);
    dst0_r = SRAI_W(dst0_r, 6);
    dst43_r = __msa_ilvr_h(src4, src3);
    dst1_r = HEVC_FILT_4TAP(dst21_r, dst43_r, filt0, filt1);
    dst1_r = SRAI_W(dst1_r, 6);

    dst0_r = (v4i32) __msa_pckev_h((v8i16) dst1_r, (v8i16) dst0_r);
    ST8x2_UB(dst0_r, dst, 2 * dst_stride);
}

static void hevc_vt_4t_4x4_w16inp_msa(WORD16 *src, WORD32 src_stride,
                                      WORD16 *dst, WORD32 dst_stride,
                                      WORD8 *filter)
{
    v16i8 vec0;
    v8i16 src0, src1, src2, src3, src4, src5, src6, filter_vec, out0_r, out1_r;
    v8i16 dst10_r, dst32_r, dst21_r, dst43_r;
    v4i32 dst0_r, dst1_r, dst2_r, dst3_r, filt0, filt1;

    src -= src_stride;
    filter_vec = LD_SH(filter);
    vec0 = CLTI_S_B(filter_vec, 0);
    filter_vec = (v8i16) __msa_ilvr_b(vec0, (v16i8) filter_vec);
    SPLATI_W2_SW(filter_vec, 0, filt0, filt1);

    LD_SH2(src, src_stride, src0, src1);
    src2 = LD_SH(src + 2 * src_stride);
    src += (3 * src_stride);
    ILVR_H2_SH(src1, src0, src2, src1, dst10_r, dst21_r);
    LD_SH4(src, src_stride, src3, src4, src5, src6);
    dst32_r = __msa_ilvr_h(src3, src2);
    dst0_r = HEVC_FILT_4TAP(dst10_r, dst32_r, filt0, filt1);
    dst0_r = SRAI_W(dst0_r, 6);
    dst43_r = __msa_ilvr_h(src4, src3);
    dst1_r = HEVC_FILT_4TAP(dst21_r, dst43_r, filt0, filt1);
    dst1_r = SRAI_W(dst1_r, 6);
    dst10_r = __msa_ilvr_h(src5, src4);
    dst2_r = HEVC_FILT_4TAP(dst32_r, dst10_r, filt0, filt1);
    dst2_r = SRAI_W(dst2_r, 6);
    dst21_r = __msa_ilvr_h(src6, src5);
    dst3_r = HEVC_FILT_4TAP(dst43_r, dst21_r, filt0, filt1);
    dst3_r = SRAI_W(dst3_r, 6);

    PCKEV_H2_SH(dst1_r, dst0_r, dst3_r, dst2_r, out0_r, out1_r);
    ST8x4_UB(out0_r, out1_r, dst, 2 * dst_stride);
}

static void hevc_vt_4t_4x8mult_w16inp_msa(WORD16 *src, WORD32 src_stride,
                                          WORD16 *dst, WORD32 dst_stride,
                                          WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    v16i8 vec0;
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9;
    v8i16 dst10_r, dst32_r, dst54_r, dst76_r, dst21_r, dst43_r, dst65_r;
    v8i16 out0_r, out1_r, out2_r, out3_r, filter_vec, dst87_r;
    v4i32 filt0, filt1;
    v4i32 dst0_r, dst1_r, dst2_r, dst3_r, dst4_r, dst5_r, dst6_r, dst7_r;

    src -= src_stride;
    filter_vec = LD_SH(filter);
    vec0 = CLTI_S_B(filter_vec, 0);
    filter_vec = (v8i16) __msa_ilvr_b(vec0, (v16i8) filter_vec);
    SPLATI_W2_SW(filter_vec, 0, filt0, filt1);

    LD_SH2(src, src_stride, src0, src1);
    src2 = LD_SH(src + 2 * src_stride);
    src += (3 * src_stride);
    ILVR_H2_SH(src1, src0, src2, src1, dst10_r, dst21_r);

    for(loop_cnt = height >> 3; loop_cnt--;)
    {
        LD_SH4(src, src_stride, src3, src4, src5, src6);
        src += (4 * src_stride);
        dst32_r = __msa_ilvr_h(src3, src2);
        dst0_r = HEVC_FILT_4TAP(dst10_r, dst32_r, filt0, filt1);
        dst0_r = SRAI_W(dst0_r, 6);
        dst43_r = __msa_ilvr_h(src4, src3);
        dst1_r = HEVC_FILT_4TAP(dst21_r, dst43_r, filt0, filt1);
        dst1_r = SRAI_W(dst1_r, 6);
        dst54_r = __msa_ilvr_h(src5, src4);
        dst2_r = HEVC_FILT_4TAP(dst32_r, dst54_r, filt0, filt1);
        dst2_r = SRAI_W(dst2_r, 6);
        dst65_r = __msa_ilvr_h(src6, src5);
        dst3_r = HEVC_FILT_4TAP(dst43_r, dst65_r, filt0, filt1);
        dst3_r = SRAI_W(dst3_r, 6);

        LD_SH4(src, src_stride, src7, src8, src9, src2);
        src += (4 * src_stride);
        dst76_r = __msa_ilvr_h(src7, src6);
        dst4_r = HEVC_FILT_4TAP(dst54_r, dst76_r, filt0, filt1);
        dst4_r = SRAI_W(dst4_r, 6);
        dst87_r = __msa_ilvr_h(src8, src7);
        dst5_r = HEVC_FILT_4TAP(dst65_r, dst87_r, filt0, filt1);
        dst5_r = SRAI_W(dst5_r, 6);
        dst10_r = __msa_ilvr_h(src9, src8);
        dst6_r = HEVC_FILT_4TAP(dst76_r, dst10_r, filt0, filt1);
        dst6_r = SRAI_W(dst6_r, 6);
        dst21_r = __msa_ilvr_h(src2, src9);
        dst7_r = HEVC_FILT_4TAP(dst87_r, dst21_r, filt0, filt1);
        dst7_r = SRAI_W(dst7_r, 6);

        PCKEV_H4_SH(dst1_r, dst0_r, dst3_r, dst2_r, dst5_r, dst4_r, dst7_r,
                    dst6_r, out0_r, out1_r, out2_r, out3_r);
        ST8x8_UB(out0_r, out1_r, out2_r, out3_r, dst, 2 * dst_stride);
        dst += (8 * dst_stride);
    }
}

static void hevc_vt_4t_8x2_w16inp_msa(WORD16 *src, WORD32 src_stride,
                                      WORD16 *dst, WORD32 dst_stride,
                                      WORD8 *filter)
{
    v16i8 vec0;
    v8i16 src0, src1, src2, src3, src4, filter_vec, dst43_l, out0_r, out1_r;
    v8i16 dst10_r, dst32_r, dst21_r, dst43_r, dst10_l, dst32_l, dst21_l;
    v4i32 dst0_r, dst0_l, dst1_r, dst1_l, filt0, filt1;

    src -= src_stride;
    filter_vec = LD_SH(filter);
    vec0 = CLTI_S_B(filter_vec, 0);
    filter_vec = (v8i16) __msa_ilvr_b(vec0, (v16i8) filter_vec);
    SPLATI_W2_SW(filter_vec, 0, filt0, filt1);

    LD_SH2(src, src_stride, src0, src1);
    src2 = LD_SH(src + 2 * src_stride);
    src += (3 * src_stride);
    ILVRL_H2_SH(src1, src0, dst10_r, dst10_l);
    ILVRL_H2_SH(src2, src1, dst21_r, dst21_l);
    LD_SH2(src, src_stride, src3, src4);
    ILVRL_H2_SH(src3, src2, dst32_r, dst32_l);
    dst0_r = HEVC_FILT_4TAP(dst10_r, dst32_r, filt0, filt1);
    dst0_l = HEVC_FILT_4TAP(dst10_l, dst32_l, filt0, filt1);
    SRAI_W2_SW(dst0_r, dst0_l, 6);
    ILVRL_H2_SH(src4, src3, dst43_r, dst43_l);
    dst1_r = HEVC_FILT_4TAP(dst21_r, dst43_r, filt0, filt1);
    dst1_l = HEVC_FILT_4TAP(dst21_l, dst43_l, filt0, filt1);
    SRAI_W2_SW(dst1_r, dst1_l, 6);

    PCKEV_H2_SH(dst0_l, dst0_r, dst1_l, dst1_r, out0_r, out1_r);
    ST_SH2(out0_r, out1_r, dst, dst_stride);
}

static void hevc_vt_4t_8x4mult_w16inp_msa(WORD16 *src, WORD32 src_stride,
                                          WORD16 *dst, WORD32 dst_stride,
                                          WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 vec0;
    v8i16 src0, src1, src2, src3, src4, src5, filter_vec, dst43_l;
    v8i16 dst10_r, dst32_r, dst21_r, dst43_r, dst10_l, dst32_l, dst21_l;
    v8i16 out0_r, out1_r, out2_r, out3_r;
    v4i32 dst0_r, dst0_l, dst1_r, dst1_l, dst2_r, dst2_l, dst3_r, dst3_l;
    v4i32 filt0, filt1;

    src -= src_stride;
    filter_vec = LD_SH(filter);
    vec0 = CLTI_S_B(filter_vec, 0);
    filter_vec = (v8i16) __msa_ilvr_b(vec0, (v16i8) filter_vec);
    SPLATI_W2_SW(filter_vec, 0, filt0, filt1);
    LD_SH2(src, src_stride, src0, src1);
    src2 = LD_SH(src + 2 * src_stride);
    src += (3 * src_stride);
    ILVRL_H2_SH(src1, src0, dst10_r, dst10_l);
    ILVRL_H2_SH(src2, src1, dst21_r, dst21_l);

    for(loop_cnt = height >> 2; loop_cnt--;)
    {
        LD_SH2(src, src_stride, src3, src4);
        src += (2 * src_stride);
        ILVRL_H2_SH(src3, src2, dst32_r, dst32_l);
        dst0_r = HEVC_FILT_4TAP(dst10_r, dst32_r, filt0, filt1);
        dst0_l = HEVC_FILT_4TAP(dst10_l, dst32_l, filt0, filt1);
        SRAI_W2_SW(dst0_r, dst0_l, 6);
        ILVRL_H2_SH(src4, src3, dst43_r, dst43_l);
        dst1_r = HEVC_FILT_4TAP(dst21_r, dst43_r, filt0, filt1);
        dst1_l = HEVC_FILT_4TAP(dst21_l, dst43_l, filt0, filt1);
        SRAI_W2_SW(dst1_r, dst1_l, 6);

        LD_SH2(src, src_stride, src5, src2);
        src += (2 * src_stride);
        ILVRL_H2_SH(src5, src4, dst10_r, dst10_l);
        dst2_r = HEVC_FILT_4TAP(dst32_r, dst10_r, filt0, filt1);
        dst2_l = HEVC_FILT_4TAP(dst32_l, dst10_l, filt0, filt1);
        SRAI_W2_SW(dst2_r, dst2_l, 6);
        ILVRL_H2_SH(src2, src5, dst21_r, dst21_l);
        dst3_r = HEVC_FILT_4TAP(dst43_r, dst21_r, filt0, filt1);
        dst3_l = HEVC_FILT_4TAP(dst43_l, dst21_l, filt0, filt1);
        SRAI_W2_SW(dst3_r, dst3_l, 6);

        PCKEV_H4_SH(dst0_l, dst0_r, dst1_l, dst1_r, dst2_l, dst2_r, dst3_l,
                    dst3_r, out0_r, out1_r, out2_r, out3_r);
        ST_SH4(out0_r, out1_r, out2_r, out3_r, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_vt_4t_12x4mult_w16inp_msa(WORD16 *src, WORD32 src_stride,
                                           WORD16 *dst, WORD32 dst_stride,
                                           WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    v16i8 vec0;
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v8i16 dst10_r, dst32_r, dst21_r, dst43_r, dst10_l, dst32_l, dst21_l;
    v8i16 dst43_l, dst76_r, dst98_r, dst87_r, dst109_r, filter_vec, src11;
    v8i16 out0_r, out1_r, out2_r, out3_r, out4_r, out5_r;
    v4i32 dst0_r, dst0_l, dst1_r, dst1_l, dst2_r, dst2_l, dst3_r, dst3_l;
    v4i32 dst4_r, dst5_r, dst6_r, dst7_r, filt0, filt1;

    src -= src_stride;
    filter_vec = LD_SH(filter);
    vec0 = CLTI_S_B(filter_vec, 0);
    filter_vec = (v8i16) __msa_ilvr_b(vec0, (v16i8) filter_vec);
    SPLATI_W2_SW(filter_vec, 0, filt0, filt1);

    LD_SH2(src, src_stride, src0, src1);
    LD_SH2(src + 8, src_stride, src6, src7);
    src2 = LD_SH(src + 2 * src_stride);
    src8 = LD_SH(src + 2 * src_stride + 8);
    src += (3 * src_stride);
    ILVRL_H2_SH(src1, src0, dst10_r, dst10_l);
    ILVRL_H2_SH(src2, src1, dst21_r, dst21_l);
    ILVR_H2_SH(src7, src6, src8, src7, dst76_r, dst87_r);

    for(loop_cnt = height >> 2; loop_cnt--;)
    {
        LD_SH2(src, src_stride, src3, src4);
        LD_SH2(src + 8, src_stride, src9, src10);
        src += (2 * src_stride);
        ILVRL_H2_SH(src3, src2, dst32_r, dst32_l);
        ILVRL_H2_SH(src4, src3, dst43_r, dst43_l);
        ILVR_H2_SH(src9, src8, src10, src9, dst98_r, dst109_r);
        dst0_r = HEVC_FILT_4TAP(dst10_r, dst32_r, filt0, filt1);
        dst0_l = HEVC_FILT_4TAP(dst10_l, dst32_l, filt0, filt1);
        dst1_r = HEVC_FILT_4TAP(dst21_r, dst43_r, filt0, filt1);
        dst1_l = HEVC_FILT_4TAP(dst21_l, dst43_l, filt0, filt1);
        dst4_r = HEVC_FILT_4TAP(dst76_r, dst98_r, filt0, filt1);
        dst5_r = HEVC_FILT_4TAP(dst87_r, dst109_r, filt0, filt1);
        SRAI_W4_SW(dst0_r, dst0_l, dst1_r, dst1_l, 6);
        SRAI_W2_SW(dst4_r, dst5_r, 6);

        LD_SH2(src, src_stride, src5, src2);
        LD_SH2(src + 8, src_stride, src11, src8);
        src += (2 * src_stride);
        ILVRL_H2_SH(src5, src4, dst10_r, dst10_l);
        ILVRL_H2_SH(src2, src5, dst21_r, dst21_l);
        ILVR_H2_SH(src11, src10, src8, src11, dst76_r, dst87_r);
        dst2_r = HEVC_FILT_4TAP(dst32_r, dst10_r, filt0, filt1);
        dst2_l = HEVC_FILT_4TAP(dst32_l, dst10_l, filt0, filt1);
        dst3_r = HEVC_FILT_4TAP(dst43_r, dst21_r, filt0, filt1);
        dst3_l = HEVC_FILT_4TAP(dst43_l, dst21_l, filt0, filt1);
        dst6_r = HEVC_FILT_4TAP(dst98_r, dst76_r, filt0, filt1);
        dst7_r = HEVC_FILT_4TAP(dst109_r, dst87_r, filt0, filt1);
        SRAI_W4_SW(dst2_r, dst2_l, dst3_r, dst3_l, 6);
        SRAI_W2_SW(dst6_r, dst7_r, 6);

        PCKEV_H4_SH(dst0_l, dst0_r, dst1_l, dst1_r, dst2_l, dst2_r, dst3_l,
                    dst3_r, out0_r, out1_r, out2_r, out3_r);
        PCKEV_H2_SH(dst5_r, dst4_r, dst7_r, dst6_r, out4_r, out5_r);
        ST_SH4(out0_r, out1_r, out2_r, out3_r, dst, dst_stride);
        ST8x4_UB(out4_r, out5_r, dst + 8, 2 * dst_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_vt_4t_16x2mult_w16inp_msa(WORD16 *src, WORD32 src_stride,
                                           WORD16 *dst, WORD32 dst_stride,
                                           WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    v16i8 vec0;
    v8i16 src0, src1, src2, src3, src4, src6, src7, src8, src9, src10;
    v8i16 dst10_r, dst32_r, dst21_r, dst43_r, dst10_l, dst32_l, dst21_l;
    v8i16 dst43_l, dst76_r, dst98_r, dst87_r, dst109_r, dst76_l, dst98_l;
    v8i16 dst87_l, dst109_l, filter_vec, out0_r, out1_r, out2_r, out3_r;
    v4i32 dst0_r, dst0_l, dst1_r, dst1_l, dst2_r, dst2_l, dst3_r, dst3_l;
    v4i32 filt0, filt1;

    src -= src_stride;
    filter_vec = LD_SH(filter);
    vec0 = CLTI_S_B(filter_vec, 0);
    filter_vec = (v8i16) __msa_ilvr_b(vec0, (v16i8) filter_vec);
    SPLATI_W2_SW(filter_vec, 0, filt0, filt1);
    LD_SH2(src, src_stride, src0, src1);
    LD_SH2(src + 8, src_stride, src6, src7);
    src2 = LD_SH(src + 2 * src_stride);
    src8 = LD_SH(src + 2 * src_stride + 8);
    src += (3 * src_stride);
    ILVRL_H2_SH(src1, src0, dst10_r, dst10_l);
    ILVRL_H2_SH(src2, src1, dst21_r, dst21_l);
    ILVRL_H2_SH(src7, src6, dst76_r, dst76_l);
    ILVRL_H2_SH(src8, src7, dst87_r, dst87_l);

    for(loop_cnt = height >> 1; loop_cnt--;)
    {
        LD_SH2(src, src_stride, src3, src4);
        LD_SH2(src + 8, src_stride, src9, src10);
        src += (2 * src_stride);
        ILVRL_H2_SH(src3, src2, dst32_r, dst32_l);
        ILVRL_H2_SH(src4, src3, dst43_r, dst43_l);
        ILVRL_H2_SH(src9, src8, dst98_r, dst98_l);
        ILVRL_H2_SH(src10, src9, dst109_r, dst109_l);
        dst0_r = HEVC_FILT_4TAP(dst10_r, dst32_r, filt0, filt1);
        dst0_l = HEVC_FILT_4TAP(dst10_l, dst32_l, filt0, filt1);
        dst1_r = HEVC_FILT_4TAP(dst21_r, dst43_r, filt0, filt1);
        dst1_l = HEVC_FILT_4TAP(dst21_l, dst43_l, filt0, filt1);
        dst2_r = HEVC_FILT_4TAP(dst76_r, dst98_r, filt0, filt1);
        dst2_l = HEVC_FILT_4TAP(dst76_l, dst98_l, filt0, filt1);
        dst3_r = HEVC_FILT_4TAP(dst87_r, dst109_r, filt0, filt1);
        dst3_l = HEVC_FILT_4TAP(dst87_l, dst109_l, filt0, filt1);
        SRAI_W4_SW(dst0_r, dst0_l, dst1_r, dst1_l, 6);
        SRAI_W4_SW(dst2_r, dst2_l, dst3_r, dst3_l, 6);

        PCKEV_H2_SH(dst0_l, dst0_r, dst1_l, dst1_r, out0_r, out1_r);
        PCKEV_H2_SH(dst2_l, dst2_r, dst3_l, dst3_r, out2_r, out3_r);
        ST_SH2(out0_r, out2_r, dst, 8);
        ST_SH2(out1_r, out3_r, dst + dst_stride, 8);
        dst += (2 * dst_stride);

        src2 = src4;
        src8 = src10;
        dst10_r = dst32_r;
        dst10_l = dst32_l;
        dst21_r = dst43_r;
        dst21_l = dst43_l;
        dst76_r = dst98_r;
        dst76_l = dst98_l;
        dst87_r = dst109_r;
        dst87_l = dst109_l;
    }
}

static void hevc_vt_4t_16multx4mult_w16inp_msa(WORD16 *src, WORD32 src_stride,
                                               WORD16 *dst, WORD32 dst_stride,
                                               WORD8 *filter, WORD32 height,
                                               WORD32 width)
{
    WORD32 loop_cnt, cnt;
    WORD16 *src_tmp, *dst_tmp;
    v16i8 vec0;
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v8i16 dst10_r, dst32_r, dst21_r, dst43_r, dst10_l, dst32_l, dst21_l;
    v8i16 dst43_l, dst76_r, dst98_r, dst87_r, dst109_r, dst76_l, dst98_l;
    v8i16 out0_r, out1_r, out2_r, out3_r, out4_r, out5_r, out6_r, out7_r;
    v8i16 src11, filter_vec, dst87_l, dst109_l;
    v4i32 filt0, filt1;
    v4i32 dst0_r, dst0_l, dst1_r, dst1_l, dst2_r, dst2_l, dst3_r, dst3_l;
    v4i32 dst4_r, dst4_l, dst5_r, dst5_l, dst6_r, dst6_l, dst7_r, dst7_l;

    src -= src_stride;
    filter_vec = LD_SH(filter);
    vec0 = CLTI_S_B(filter_vec, 0);
    filter_vec = (v8i16) __msa_ilvr_b(vec0, (v16i8) filter_vec);
    SPLATI_W2_SW(filter_vec, 0, filt0, filt1);

    for(cnt = width >> 4; cnt--;)
    {
        src_tmp = src;
        dst_tmp = dst;

        LD_SH2(src_tmp, src_stride, src0, src1);
        LD_SH2(src_tmp + 8, src_stride, src6, src7);
        src2 = LD_SH(src_tmp + 2 * src_stride);
        src8 = LD_SH(src_tmp + 2 * src_stride + 8);
        src_tmp += (3 * src_stride);
        ILVRL_H2_SH(src1, src0, dst10_r, dst10_l);
        ILVRL_H2_SH(src2, src1, dst21_r, dst21_l);
        ILVRL_H2_SH(src7, src6, dst76_r, dst76_l);
        ILVRL_H2_SH(src8, src7, dst87_r, dst87_l);

        for(loop_cnt = height >> 2; loop_cnt--;)
        {
            LD_SH2(src_tmp, src_stride, src3, src4);
            LD_SH2(src_tmp + 8, src_stride, src9, src10);
            src_tmp += (2 * src_stride);
            ILVRL_H2_SH(src3, src2, dst32_r, dst32_l);
            ILVRL_H2_SH(src4, src3, dst43_r, dst43_l);
            ILVRL_H2_SH(src9, src8, dst98_r, dst98_l);
            ILVRL_H2_SH(src10, src9, dst109_r, dst109_l);
            dst0_r = HEVC_FILT_4TAP(dst10_r, dst32_r, filt0, filt1);
            dst0_l = HEVC_FILT_4TAP(dst10_l, dst32_l, filt0, filt1);
            dst1_r = HEVC_FILT_4TAP(dst21_r, dst43_r, filt0, filt1);
            dst1_l = HEVC_FILT_4TAP(dst21_l, dst43_l, filt0, filt1);
            dst4_r = HEVC_FILT_4TAP(dst76_r, dst98_r, filt0, filt1);
            dst4_l = HEVC_FILT_4TAP(dst76_l, dst98_l, filt0, filt1);
            dst5_r = HEVC_FILT_4TAP(dst87_r, dst109_r, filt0, filt1);
            dst5_l = HEVC_FILT_4TAP(dst87_l, dst109_l, filt0, filt1);
            SRAI_W4_SW(dst0_r, dst0_l, dst1_r, dst1_l, 6);
            SRAI_W4_SW(dst4_r, dst4_l, dst5_r, dst5_l, 6);

            LD_SH2(src_tmp, src_stride, src5, src2);
            LD_SH2(src_tmp + 8, src_stride, src11, src8);
            src_tmp += (2 * src_stride);
            ILVRL_H2_SH(src5, src4, dst10_r, dst10_l);
            ILVRL_H2_SH(src2, src5, dst21_r, dst21_l);
            ILVRL_H2_SH(src11, src10, dst76_r, dst76_l);
            ILVRL_H2_SH(src8, src11, dst87_r, dst87_l);
            dst2_r = HEVC_FILT_4TAP(dst32_r, dst10_r, filt0, filt1);
            dst2_l = HEVC_FILT_4TAP(dst32_l, dst10_l, filt0, filt1);
            dst3_r = HEVC_FILT_4TAP(dst43_r, dst21_r, filt0, filt1);
            dst3_l = HEVC_FILT_4TAP(dst43_l, dst21_l, filt0, filt1);
            dst6_r = HEVC_FILT_4TAP(dst98_r, dst76_r, filt0, filt1);
            dst6_l = HEVC_FILT_4TAP(dst98_l, dst76_l, filt0, filt1);
            dst7_r = HEVC_FILT_4TAP(dst109_r, dst87_r, filt0, filt1);
            dst7_l = HEVC_FILT_4TAP(dst109_l, dst87_l, filt0, filt1);
            SRAI_W4_SW(dst2_r, dst2_l, dst3_r, dst3_l, 6);
            SRAI_W4_SW(dst6_r, dst6_l, dst7_r, dst7_l, 6);

            PCKEV_H4_SH(dst0_l, dst0_r, dst1_l, dst1_r, dst2_l, dst2_r, dst3_l,
                        dst3_r, out0_r, out1_r, out2_r, out3_r);
            PCKEV_H4_SH(dst4_l, dst4_r, dst5_l, dst5_r, dst6_l, dst6_r, dst7_l,
                        dst7_r, out4_r, out5_r, out6_r, out7_r);
            ST_SH2(out0_r, out4_r, dst_tmp, 8);
            ST_SH2(out1_r, out5_r, dst_tmp + dst_stride, 8);
            ST_SH2(out2_r, out6_r, dst_tmp + 2 * dst_stride, 8);
            ST_SH2(out3_r, out7_r, dst_tmp + 3 * dst_stride, 8);
            dst_tmp += (4 * dst_stride);
        }

        src += 16;
        dst += 16;
    }
}

static void common_ilv_hz_4t_2x4_msa(UWORD8 *src, WORD32 src_stride,
                                     UWORD8 *dst, WORD32 dst_stride,
                                     WORD8 *filter)
{
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 16, 18, 17, 19, 18, 20, 19, 21};
    v16i8 src0, src1, src2, src3, filt0, filt1, mask1;
    v16u8 out;
    v8i16 filt, out0, out1;

    src -= 2;
    mask1 = ADDVI_B(mask0, 4);
    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);
    LD_SB4(src, src_stride, src0, src1, src2, src3);
    XORI_B4_128_SB(src0, src1, src2, src3);
    HORIZ_4TAP_4WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1, filt0,
                               filt1, out0, out1);
    SRARI_H2_SH(out0, out1, 6);
    SAT_SH2_SH(out0, out1, 7);
    out = PCKEV_XORI128_UB(out0, out1);
    ST4x4_UB(out, out, 0, 1, 2, 3, dst, dst_stride);
}

static void common_ilv_hz_4t_2x8_msa(UWORD8 *src, WORD32 src_stride,
                                     UWORD8 *dst, WORD32 dst_stride,
                                     WORD8 *filter)
{
    v16i8 src0, src1, src2, src3, filt0, filt1, mask1;
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 16, 18, 17, 19, 18, 20, 19, 21};
    v16u8 out;
    v8i16 filt, out0, out1, out2, out3;

    src -= 2;
    mask1 = ADDVI_B(mask0, 4);

    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    LD_SB4(src, src_stride, src0, src1, src2, src3);
    src += (4 * src_stride);

    XORI_B4_128_SB(src0, src1, src2, src3);
    HORIZ_4TAP_4WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1, filt0,
                               filt1, out0, out1);
    LD_SB4(src, src_stride, src0, src1, src2, src3);
    XORI_B4_128_SB(src0, src1, src2, src3);
    HORIZ_4TAP_4WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1, filt0,
                               filt1, out2, out3);
    SRARI_H4_SH(out0, out1, out2, out3, 6);
    SAT_SH4_SH(out0, out1, out2, out3, 7);
    out = PCKEV_XORI128_UB(out0, out1);
    ST4x4_UB(out, out, 0, 1, 2, 3, dst, dst_stride);
    dst += (4 * dst_stride);
    out = PCKEV_XORI128_UB(out2, out3);
    ST4x4_UB(out, out, 0, 1, 2, 3, dst, dst_stride);
}

static void common_ilv_hz_4t_4x2mult_msa(UWORD8 *src, WORD32 src_stride,
                                         UWORD8 *dst, WORD32 dst_stride,
                                         WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, filt0, filt1, mask1;
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9};
    v16u8 out;
    v8i16 filt, vec0, vec1, vec2, vec3;

    src -= 2;
    mask1 = ADDVI_B(mask0, 4);

    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    for(loop_cnt = (height >> 1); loop_cnt--;)
    {
        LD_SB2(src, src_stride, src0, src1);
        src += (2 * src_stride);

        XORI_B2_128_SB(src0, src1);
        VSHF_B2_SH(src0, src0, src1, src1, mask0, mask0, vec0, vec1);
        DOTP_SB2_SH(vec0, vec1, filt0, filt0, vec0, vec1);
        VSHF_B2_SH(src0, src0, src1, src1, mask1, mask1, vec2, vec3);
        DPADD_SB2_SH(vec2, vec3, filt1, filt1, vec0, vec1);
        SRARI_H2_SH(vec0, vec1, 6);
        SAT_SH2_SH(vec0, vec1, 7);
        out = PCKEV_XORI128_UB(vec0, vec1);
        ST8x2_UB(out, dst, dst_stride)
        dst += (2 * dst_stride);
    }
}

static void common_ilv_hz_4t_4x4mult_msa(UWORD8 *src, WORD32 src_stride,
                                         UWORD8 *dst, WORD32 dst_stride,
                                         WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, filt0, filt1, mask1;
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9};
    v16u8 tmp0, tmp1;
    v8i16 filt, out0, out1, out2, out3;

    src -= 2;
    mask1 = ADDVI_B(mask0, 4);

    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src0, src1, src2, src3);
        src += (4 * src_stride);

        XORI_B4_128_SB(src0, src1, src2, src3);
        HORIZ_4TAP_8WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1,
                                   filt0, filt1, out0, out1, out2, out3);
        SRARI_H4_SH(out0, out1, out2, out3, 6);
        SAT_SH4_SH(out0, out1, out2, out3, 7);
        tmp0 = PCKEV_XORI128_UB(out0, out1);
        tmp1 = PCKEV_XORI128_UB(out2, out3);
        ST8x4_UB(tmp0, tmp1, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void common_ilv_hz_4t_6w_msa(UWORD8 *src, WORD32 src_stride,
                                    UWORD8 *dst, WORD32 dst_stride,
                                    WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    UWORD16 tmp0, tmp1, tmp2, tmp3;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, filt0, filt1;
    v16i8 op0, op1, op2, mask1, mask2, mask3, vec0, vec1, vec2, vec3;
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9};
    v8i16 filt, out0, out1, out2, out3, out4, out6;

    src -= 2;
    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    mask1 = ADDVI_B(mask0, 4);
    mask2 = ADDVI_B(mask0, 8);
    mask3 = ADDVI_B(mask0, 12);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src0, src2, src4, src6);
        tmp0 = LH(src + 16);
        tmp1 = LH(src + 16 + src_stride);
        tmp2 = LH(src + 16 + 2 * src_stride);
        tmp3 = LH(src + 16 + 3 * src_stride);
        src1 = (v16i8) __msa_fill_h(tmp0);
        src3 = (v16i8) __msa_fill_h(tmp1);
        src5 = (v16i8) __msa_fill_h(tmp2);
        src7 = (v16i8) __msa_fill_h(tmp3);
        src += 4 * src_stride;
        XORI_B8_128_SB(src0, src1, src2, src3, src4, src5, src6, src7);

        VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
        out0 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);
        VSHF_B2_SB(src2, src2, src2, src2, mask0, mask1, vec0, vec1);
        out2 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);
        VSHF_B2_SB(src4, src4, src4, src4, mask0, mask1, vec0, vec1);
        out4 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);
        VSHF_B2_SB(src6, src6, src6, src6, mask0, mask1, vec0, vec1);
        out6 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);
        VSHF_B2_SB(src0, src1, src0, src1, mask2, mask3, vec0, vec1);
        VSHF_B2_SB(src2, src3, src2, src3, mask2, mask3, vec2, vec3);
        vec0 = (v16i8) __msa_ilvr_d((v2i64) vec2, (v2i64) vec0);
        vec1 = (v16i8) __msa_ilvr_d((v2i64) vec3, (v2i64) vec1);
        out1 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);
        VSHF_B2_SB(src4, src5, src4, src5, mask2, mask3, vec0, vec1);
        VSHF_B2_SB(src6, src7, src6, src7, mask2, mask3, vec2, vec3);
        vec0 = (v16i8) __msa_ilvr_d((v2i64) vec2, (v2i64) vec0);
        vec1 = (v16i8) __msa_ilvr_d((v2i64) vec3, (v2i64) vec1);
        out3 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);

        SRARI_H4_SH(out0, out2, out4, out6, 6);
        SRARI_H2_SH(out1, out3, 6);
        SAT_SH4_SH(out0, out2, out4, out6, 7);
        SAT_SH2_SH(out1, out3, 7);

        PCKEV_B3_SB(out2, out0, out6, out4, out3, out1, op0, op1, op2);
        XORI_B3_128_SB(op0, op1, op2);
        ST8x4_UB(op0, op1, dst, dst_stride);
        ST4x4_UB(op2, op2, 0, 1, 2, 3, dst + 8, dst_stride);
        dst += 4 * dst_stride;
    }
}

static void common_ilv_hz_4t_8x2mult_msa(UWORD8 *src, WORD32 src_stride,
                                         UWORD8 *dst, WORD32 dst_stride,
                                         WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    uint64_t tmp0, tmp1;
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9};
    v16i8 src0, src1, src2, src3, filt0, filt1, op0, op1, mask1, mask2, mask3;
    v16i8 vec0, vec1;
    v8i16 filt, out0, out1, out2, out3;

    src -= 2;
    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    mask1 = ADDVI_B(mask0, 4);
    mask2 = ADDVI_B(mask0, 8);
    mask3 = ADDVI_B(mask0, 12);

    for(loop_cnt = (height >> 1); loop_cnt--;)
    {
        LD_SB2(src, src_stride, src0, src2);
        LD2(src + 16, src_stride, tmp0, tmp1);
        src1 = (v16i8) __msa_fill_d(tmp0);
        src3 = (v16i8) __msa_fill_d(tmp1);
        src += 2 * src_stride;
        XORI_B4_128_SB(src0, src1, src2, src3);

        VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
        out0 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);
        VSHF_B2_SB(src0, src1, src0, src1, mask2, mask3, vec0, vec1);
        out1 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);
        VSHF_B2_SB(src2, src2, src2, src2, mask0, mask1, vec0, vec1);
        out2 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);
        VSHF_B2_SB(src2, src3, src2, src3, mask2, mask3, vec0, vec1);
        out3 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);

        SRARI_H4_SH(out0, out1, out2, out3, 6);
        SAT_SH4_SH(out0, out1, out2, out3, 7);

        PCKEV_B2_SB(out1, out0, out3, out2, op0, op1);
        XORI_B2_128_SB(op0, op1);
        ST_SB2(op0, op1, dst, dst_stride);
        dst += 2 * dst_stride;
    }
}

static void common_ilv_hz_4t_8x4mult_msa(UWORD8 *src, WORD32 src_stride,
                                         UWORD8 *dst, WORD32 dst_stride,
                                         WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9};
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, filt0, filt1, mask1;
    v16u8 out;
    v8i16 filt, out0, out1, out2, out3, out4, out5, out6, out7;

    src -= 2;
    mask1 = ADDVI_B(mask0, 4);

    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src0, src2, src4, src6);
        LD_SB4(src + 8, src_stride, src1, src3, src5, src7);
        src += (4 * src_stride);

        XORI_B8_128_SB(src0, src1, src2, src3, src4, src5, src6, src7);
        HORIZ_4TAP_8WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1,
                                   filt0, filt1, out0, out1, out2, out3);
        HORIZ_4TAP_8WID_4VECS_FILT(src4, src5, src6, src7, mask0, mask1,
                                   filt0, filt1, out4, out5, out6, out7);
        SRARI_H4_SH(out0, out1, out2, out3, 6);
        SRARI_H4_SH(out4, out5, out6, out7, 6);
        SAT_SH4_SH(out0, out1, out2, out3, 7);
        SAT_SH4_SH(out4, out5, out6, out7, 7);
        out = PCKEV_XORI128_UB(out0, out1);
        ST_UB(out, dst);
        dst += dst_stride;
        out = PCKEV_XORI128_UB(out2, out3);
        ST_UB(out, dst);
        dst += dst_stride;
        out = PCKEV_XORI128_UB(out4, out5);
        ST_UB(out, dst);
        dst += dst_stride;
        out = PCKEV_XORI128_UB(out6, out7);
        ST_UB(out, dst);
        dst += dst_stride;
    }
}

static void common_ilv_hz_4t_12w_msa(UWORD8 *src, WORD32 src_stride,
                                     UWORD8 *dst, WORD32 dst_stride,
                                     WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    UWORD8 *dst1 = dst + 16;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7;
    v16i8 filt0, filt1, mask1, mask2, mask3;
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9};
    v16u8 tmp0, tmp1;
    v8i16 filt, out0, out1, out2, out3;

    src -= 2;

    mask1 = ADDVI_B(mask0, 4);
    mask2 = ADDVI_B(mask0, 8);
    mask3 = ADDVI_B(mask0, 12);

    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src0, src2, src4, src6);
        LD_SB4(src + 16, src_stride, src1, src3, src5, src7);
        src += (4 * src_stride);

        XORI_B8_128_SB(src0, src1, src2, src3, src4, src5, src6, src7);
        VSHF_B2_SB(src0, src0, src0, src1, mask0, mask2, vec0, vec1);
        VSHF_B2_SB(src2, src2, src2, src3, mask0, mask2, vec2, vec3);
        VSHF_B2_SB(src0, src0, src0, src1, mask1, mask3, vec4, vec5);
        VSHF_B2_SB(src2, src2, src2, src3, mask1, mask3, vec6, vec7);
        DOTP_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt0, filt0, filt0,
                    out0, out1, out2, out3);
        DPADD_SB4_SH(vec4, vec5, vec6, vec7, filt1, filt1, filt1, filt1,
                     out0, out1, out2, out3);
        SRARI_H4_SH(out0, out1, out2, out3, 6);
        SAT_SH4_SH(out0, out1, out2, out3, 7);
        tmp0 = PCKEV_XORI128_UB(out0, out1);
        ST_UB(tmp0, dst);
        dst += dst_stride;
        tmp0 = PCKEV_XORI128_UB(out2, out3);
        ST_UB(tmp0, dst);
        dst += dst_stride;

        VSHF_B2_SB(src4, src4, src4, src5, mask0, mask2, vec0, vec1);
        VSHF_B2_SB(src6, src6, src6, src7, mask0, mask2, vec2, vec3);
        VSHF_B2_SB(src4, src4, src4, src5, mask1, mask3, vec4, vec5);
        VSHF_B2_SB(src6, src6, src6, src7, mask1, mask3, vec6, vec7);
        DOTP_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt0, filt0, filt0,
                    out0, out1, out2, out3);
        DPADD_SB4_SH(vec4, vec5, vec6, vec7, filt1, filt1, filt1, filt1,
                     out0, out1, out2, out3);
        SRARI_H4_SH(out0, out1, out2, out3, 6);
        SAT_SH4_SH(out0, out1, out2, out3, 7);
        tmp0 = PCKEV_XORI128_UB(out0, out1);
        ST_UB(tmp0, dst);
        dst += dst_stride;
        tmp0 = PCKEV_XORI128_UB(out2, out3);
        ST_UB(tmp0, dst);
        dst += dst_stride;

        VSHF_B2_SB(src1, src1, src3, src3, mask0, mask0, vec0, vec1);
        VSHF_B2_SB(src5, src5, src7, src7, mask0, mask0, vec2, vec3);
        VSHF_B2_SB(src1, src1, src3, src3, mask1, mask1, vec4, vec5);
        VSHF_B2_SB(src5, src5, src7, src7, mask1, mask1, vec6, vec7);

        DOTP_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt0, filt0, filt0,
                    out0, out1, out2, out3);
        DPADD_SB4_SH(vec4, vec5, vec6, vec7, filt1, filt1, filt1, filt1,
                     out0, out1, out2, out3);

        SRARI_H4_SH(out0, out1, out2, out3, 6);
        SAT_SH4_SH(out0, out1, out2, out3, 7);
        tmp0 = PCKEV_XORI128_UB(out0, out1);
        tmp1 = PCKEV_XORI128_UB(out2, out3);
        ST8x4_UB(tmp0, tmp1, dst1, dst_stride);
        dst1 += (4 * dst_stride);
    }
}

static void common_ilv_hz_4t_16w_msa(UWORD8 *src, WORD32 src_stride,
                                     UWORD8 *dst, WORD32 dst_stride,
                                     WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, filt0, filt1, mask1;
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9};
    v16u8 out;
    v8i16 filt, out0, out1, out2, out3, out4, out5, out6, out7;

    src -= 2;
    mask1 = ADDVI_B(mask0, 4);

    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    for(loop_cnt = (height >> 1); loop_cnt--;)
    {
        src0 = LD_SB(src);
        src2 = LD_SB(src + 16);
        src3 = LD_SB(src + 24);
        src += src_stride;
        src4 = LD_SB(src);
        src6 = LD_SB(src + 16);
        src7 = LD_SB(src + 24);
        SLDI_B2_SB(src2, src6, src0, src4, src1, src5, 8);
        src += src_stride;

        XORI_B8_128_SB(src0, src1, src2, src3, src4, src5, src6, src7);
        HORIZ_4TAP_8WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1,
                                   filt0, filt1, out0, out1, out2, out3);
        HORIZ_4TAP_8WID_4VECS_FILT(src4, src5, src6, src7, mask0, mask1,
                                   filt0, filt1, out4, out5, out6, out7);
        SRARI_H4_SH(out0, out1, out2, out3, 6);
        SRARI_H4_SH(out4, out5, out6, out7, 6);
        SAT_SH4_SH(out0, out1, out2, out3, 7);
        SAT_SH4_SH(out4, out5, out6, out7, 7);
        out = PCKEV_XORI128_UB(out0, out1);
        ST_UB(out, dst);
        out = PCKEV_XORI128_UB(out2, out3);
        ST_UB(out, dst + 16);
        dst += dst_stride;
        out = PCKEV_XORI128_UB(out4, out5);
        ST_UB(out, dst);
        out = PCKEV_XORI128_UB(out6, out7);
        ST_UB(out, dst + 16);
        dst += dst_stride;
    }
}

static void common_ilv_hz_4t_24w_msa(UWORD8 *src, WORD32 src_stride,
                                     UWORD8 *dst, WORD32 dst_stride,
                                     WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    uint64_t tmp0, tmp1;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, filt0, filt1;
    v16i8 op0, op1, op2, mask1, mask2, mask3, vec0, vec1;
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9};
    v8i16 filt, out0, out1, out2, out3, out4, out5;

    src -= 2;
    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    mask1 = ADDVI_B(mask0, 4);
    mask2 = ADDVI_B(mask0, 8);
    mask3 = ADDVI_B(mask0, 12);

    for(loop_cnt = (height >> 1); loop_cnt--;)
    {
        LD_SB2(src, src_stride, src0, src4);
        LD_SB2(src + 16, src_stride, src1, src5);
        LD_SB2(src + 32, src_stride, src2, src6);
        LD2(src + 48, src_stride, tmp0, tmp1);
        src3 = (v16i8) __msa_fill_d(tmp0);
        src7 = (v16i8) __msa_fill_d(tmp1);
        src += 2 * src_stride;
        XORI_B8_128_SB(src0, src1, src2, src3, src4, src5, src6, src7);

        VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
        out0 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);
        VSHF_B2_SB(src0, src1, src0, src1, mask2, mask3, vec0, vec1);
        out1 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);
        VSHF_B2_SB(src1, src1, src1, src1, mask0, mask1, vec0, vec1);
        out2 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);
        VSHF_B2_SB(src1, src2, src1, src2, mask2, mask3, vec0, vec1);
        out3 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);
        VSHF_B2_SB(src2, src2, src2, src2, mask0, mask1, vec0, vec1);
        out4 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);
        VSHF_B2_SB(src2, src3, src2, src3, mask2, mask3, vec0, vec1);
        out5 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);

        SRARI_H4_SH(out0, out1, out2, out3, 6);
        SRARI_H2_SH(out4, out5, 6);
        SAT_SH4_SH(out0, out1, out2, out3, 7);
        SAT_SH2_SH(out4, out5, 7);

        PCKEV_B3_SB(out1, out0, out3, out2, out5, out4, op0, op1, op2);
        XORI_B3_128_SB(op0, op1, op2);
        ST_SB2(op0, op1, dst, 16);
        ST_SB(op2, dst + 32);

        VSHF_B2_SB(src4, src4, src4, src4, mask0, mask1, vec0, vec1);
        out0 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);
        VSHF_B2_SB(src4, src5, src4, src5, mask2, mask3, vec0, vec1);
        out1 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);
        VSHF_B2_SB(src5, src5, src5, src5, mask0, mask1, vec0, vec1);
        out2 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);
        VSHF_B2_SB(src5, src6, src5, src6, mask2, mask3, vec0, vec1);
        out3 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);
        VSHF_B2_SB(src6, src6, src6, src6, mask0, mask1, vec0, vec1);
        out4 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);
        VSHF_B2_SB(src6, src7, src6, src7, mask2, mask3, vec0, vec1);
        out5 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);

        SRARI_H4_SH(out0, out1, out2, out3, 6);
        SRARI_H2_SH(out4, out5, 6);
        SAT_SH4_SH(out0, out1, out2, out3, 7);
        SAT_SH2_SH(out4, out5, 7);

        PCKEV_B3_SB(out1, out0, out3, out2, out5, out4, op0, op1, op2);
        XORI_B3_128_SB(op0, op1, op2);
        ST_SB2(op0, op1, dst + dst_stride, 16);
        ST_SB(op2, dst + dst_stride + 32);
        dst += 2 * dst_stride;
    }
}

static void common_ilv_hz_4t_32w_msa(UWORD8 *src, WORD32 src_stride,
                                     UWORD8 *dst, WORD32 dst_stride,
                                     WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, filt0, filt1, mask1;
    v16i8 mask0 = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9};
    v16u8 out;
    v8i16 filt, out0, out1, out2, out3, out4, out5, out6, out7;

    src -= 2;
    mask1 = ADDVI_B(mask0, 4);

    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    for(loop_cnt = height; loop_cnt--;)
    {
        LD_SB4(src, 16, src0, src2, src4, src6);
        src7 = LD_SB(src + 56);
        SLDI_B3_SB(src2, src4, src6, src0, src2, src4, src1, src3, src5, 8);
        src += src_stride;

        XORI_B8_128_SB(src0, src1, src2, src3, src4, src5, src6, src7);
        HORIZ_4TAP_8WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1,
                                   filt0, filt1, out0, out1, out2, out3);
        HORIZ_4TAP_8WID_4VECS_FILT(src4, src5, src6, src7, mask0, mask1,
                                   filt0, filt1, out4, out5, out6, out7);
        SRARI_H4_SH(out0, out1, out2, out3, 6);
        SRARI_H4_SH(out4, out5, out6, out7, 6);
        SAT_SH4_SH(out0, out1, out2, out3, 7);
        SAT_SH4_SH(out4, out5, out6, out7, 7);
        out = PCKEV_XORI128_UB(out0, out1);
        ST_UB(out, dst);
        out = PCKEV_XORI128_UB(out2, out3);
        ST_UB(out, dst + 16);
        out = PCKEV_XORI128_UB(out4, out5);
        ST_UB(out, dst + 32);
        out = PCKEV_XORI128_UB(out6, out7);
        ST_UB(out, dst + 48);
        dst += dst_stride;
    }
}

static void common_ilv_hz_4t_2w_msa(UWORD8 *src, WORD32 src_stride,
                                    UWORD8 *dst, WORD32 dst_stride,
                                    WORD8 *filter, WORD32 height)
{
    if(4 == height)
    {
        common_ilv_hz_4t_2x4_msa(src, src_stride, dst, dst_stride, filter);
    }
    else if(8 == height)
    {
        common_ilv_hz_4t_2x8_msa(src, src_stride, dst, dst_stride, filter);
    }
}

static void common_ilv_hz_4t_4w_msa(UWORD8 *src, WORD32 src_stride,
                                    UWORD8 *dst, WORD32 dst_stride,
                                    WORD8 *filter, WORD32 height)
{
    if(2 == height)
    {
        common_ilv_hz_4t_4x2mult_msa(src, src_stride, dst, dst_stride, filter,
                                     height);
    }
    else if(0 == height % 4)
    {
        common_ilv_hz_4t_4x4mult_msa(src, src_stride, dst, dst_stride, filter,
                                     height);
    }
}

static void common_ilv_hz_4t_8w_msa(UWORD8 *src, WORD32 src_stride,
                                    UWORD8 *dst, WORD32 dst_stride,
                                    WORD8 *filter, WORD32 height)
{
    if ((2 == height) || (6 == height))
    {
        common_ilv_hz_4t_8x2mult_msa(src, src_stride, dst, dst_stride, filter,
                                     height);
    }
    else
    {
        common_ilv_hz_4t_8x4mult_msa(src, src_stride, dst, dst_stride, filter,
                                     height);
    }
}

static void common_vt_4t_4x2_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter)
{
    v16i8 src0, src1, src2, src3, src4, src10_r, src32_r, src21_r, src43_r;
    v16i8 src2110, src4332, filt0, filt1;
    v16u8 out;
    v8i16 filt, out10;

    src -= src_stride;

    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    LD_SB3(src, src_stride, src0, src1, src2);
    src += (3 * src_stride);

    ILVR_B2_SB(src1, src0, src2, src1, src10_r, src21_r);
    src2110 = (v16i8) __msa_ilvr_d((v2i64) src21_r, (v2i64) src10_r);
    src2110 = (v16i8) XORI_B(src2110, 128);
    LD_SB2(src, src_stride, src3, src4);
    ILVR_B2_SB(src3, src2, src4, src3, src32_r, src43_r);
    src4332 = (v16i8) __msa_ilvr_d((v2i64) src43_r, (v2i64) src32_r);
    src4332 = (v16i8) XORI_B(src4332, 128);
    out10 = FILT_4TAP_DPADD_S_H(src2110, src4332, filt0, filt1);
    out10 = __msa_srari_h(out10, 6);
    out10 = __msa_sat_s_h(out10, 7);
    out = PCKEV_XORI128_UB(out10, out10);
    ST4x2_UB(out, dst, dst_stride);
}

static void common_vt_4t_4x4multiple_msa(UWORD8 *src, WORD32 src_stride,
                                         UWORD8 *dst, WORD32 dst_stride,
                                         WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, src5, src2110, src4332, filt0, filt1;
    v16i8 src10_r, src32_r, src54_r, src21_r, src43_r, src65_r;
    v16u8 out;
    v8i16 filt, out10, out32;

    src -= src_stride;

    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    LD_SB3(src, src_stride, src0, src1, src2);
    src += (3 * src_stride);

    ILVR_B2_SB(src1, src0, src2, src1, src10_r, src21_r);

    src2110 = (v16i8) __msa_ilvr_d((v2i64) src21_r, (v2i64) src10_r);
    src2110 = (v16i8) XORI_B(src2110, 128);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB3(src, src_stride, src3, src4, src5);
        src += (3 * src_stride);
        ILVR_B2_SB(src3, src2, src4, src3, src32_r, src43_r);
        src4332 = (v16i8) __msa_ilvr_d((v2i64) src43_r, (v2i64) src32_r);
        src4332 = (v16i8) XORI_B(src4332, 128);
        out10 = FILT_4TAP_DPADD_S_H(src2110, src4332, filt0, filt1);

        src2 = LD_SB(src);
        src += (src_stride);
        ILVR_B2_SB(src5, src4, src2, src5, src54_r, src65_r);
        src2110 = (v16i8) __msa_ilvr_d((v2i64) src65_r, (v2i64) src54_r);
        src2110 = (v16i8) XORI_B(src2110, 128);
        out32 = FILT_4TAP_DPADD_S_H(src4332, src2110, filt0, filt1);
        SRARI_H2_SH(out10, out32, 6);
        SAT_SH2_SH(out10, out32, 7);
        out = PCKEV_XORI128_UB(out10, out32);
        ST4x4_UB(out, out, 0, 1, 2, 3, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void common_vt_4t_4w_msa(UWORD8 *src, WORD32 src_stride,
                                UWORD8 *dst, WORD32 dst_stride,
                                WORD8 *filter, WORD32 height)
{
    if(2 == height)
    {
        common_vt_4t_4x2_msa(src, src_stride, dst, dst_stride, filter);
    }
    else
    {
        common_vt_4t_4x4multiple_msa(src, src_stride, dst, dst_stride, filter,
                                     height);
    }
}

static void common_vt_4t_8x2_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter)
{
    v16i8 src0, src1, src2, src3, src4;
    v16u8 out;
    v8i16 src01, src12, src23, src34, tmp0, tmp1, filt, filt0, filt1;

    src -= src_stride;

    filt = LD_SH(filter);
    SPLATI_H2_SH(filt, 0, 1, filt0, filt1);

    LD_SB5(src, src_stride, src0, src1, src2, src3, src4);
    XORI_B5_128_SB(src0, src1, src2, src3, src4);
    ILVR_B2_SH(src1, src0, src3, src2, src01, src23);
    tmp0 = FILT_4TAP_DPADD_S_H(src01, src23, filt0, filt1);
    ILVR_B2_SH(src2, src1, src4, src3, src12, src34);
    tmp1 = FILT_4TAP_DPADD_S_H(src12, src34, filt0, filt1);
    SRARI_H2_SH(tmp0, tmp1, 6);
    SAT_SH2_SH(tmp0, tmp1, 7);
    out = PCKEV_XORI128_UB(tmp0, tmp1);
    ST8x2_UB(out, dst, dst_stride);
}

static void common_vt_4t_8x6_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter)
{
    UWORD32 loop_cnt;
    uint64_t out0, out1, out2;
    v16i8 src0, src1, src2, src3, src4, src5;
    v8i16 vec0, vec1, vec2, vec3, vec4, tmp0, tmp1, tmp2, filt, filt0, filt1;

    src -= src_stride;

    filt = LD_SH(filter);
    SPLATI_H2_SH(filt, 0, 1, filt0, filt1);

    LD_SB3(src, src_stride, src0, src1, src2);
    src += (3 * src_stride);

    XORI_B3_128_SB(src0, src1, src2);
    ILVR_B2_SH(src1, src0, src2, src1, vec0, vec2);

    for(loop_cnt = 2; loop_cnt--;)
    {
        LD_SB3(src, src_stride, src3, src4, src5);
        src += (3 * src_stride);

        XORI_B3_128_SB(src3, src4, src5);
        ILVR_B3_SH(src3, src2, src4, src3, src5, src4, vec1, vec3, vec4);
        tmp0 = FILT_4TAP_DPADD_S_H(vec0, vec1, filt0, filt1);
        tmp1 = FILT_4TAP_DPADD_S_H(vec2, vec3, filt0, filt1);
        tmp2 = FILT_4TAP_DPADD_S_H(vec1, vec4, filt0, filt1);
        SRARI_H3_SH(tmp0, tmp1, tmp2, 6);
        SAT_SH3_SH(tmp0, tmp1, tmp2, 7);
        PCKEV_B2_SH(tmp1, tmp0, tmp2, tmp2, tmp0, tmp2);
        XORI_B2_128_SH(tmp0, tmp2);

        out0 = __msa_copy_u_d((v2i64) tmp0, 0);
        out1 = __msa_copy_u_d((v2i64) tmp0, 1);
        out2 = __msa_copy_u_d((v2i64) tmp2, 0);
        SD(out0, dst);
        dst += dst_stride;
        SD(out1, dst);
        dst += dst_stride;
        SD(out2, dst);
        dst += dst_stride;

        src2 = src5;
        vec0 = vec3;
        vec2 = vec4;
    }
}

static void common_vt_4t_8x4mult_msa(UWORD8 *src, WORD32 src_stride,
                                     UWORD8 *dst, WORD32 dst_stride,
                                     WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src7, src8, src9, src10;
    v16i8 src10_r, src72_r, src98_r, src21_r, src87_r, src109_r, filt0, filt1;
    v16u8 tmp0, tmp1;
    v8i16 filt, out0_r, out1_r, out2_r, out3_r;

    src -= src_stride;

    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    LD_SB3(src, src_stride, src0, src1, src2);
    src += (3 * src_stride);

    XORI_B3_128_SB(src0, src1, src2);
    ILVR_B2_SB(src1, src0, src2, src1, src10_r, src21_r);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src7, src8, src9, src10);
        src += (4 * src_stride);

        XORI_B4_128_SB(src7, src8, src9, src10);
        ILVR_B4_SB(src7, src2, src8, src7, src9, src8, src10, src9, src72_r,
                   src87_r, src98_r, src109_r);
        out0_r = FILT_4TAP_DPADD_S_H(src10_r, src72_r, filt0, filt1);
        out1_r = FILT_4TAP_DPADD_S_H(src21_r, src87_r, filt0, filt1);
        out2_r = FILT_4TAP_DPADD_S_H(src72_r, src98_r, filt0, filt1);
        out3_r = FILT_4TAP_DPADD_S_H(src87_r, src109_r, filt0, filt1);
        SRARI_H4_SH(out0_r, out1_r, out2_r, out3_r, 6);
        SAT_SH4_SH(out0_r, out1_r, out2_r, out3_r, 7);
        tmp0 = PCKEV_XORI128_UB(out0_r, out1_r);
        tmp1 = PCKEV_XORI128_UB(out2_r, out3_r);
        ST8x4_UB(tmp0, tmp1, dst, dst_stride);
        dst += (4 * dst_stride);

        src10_r = src98_r;
        src21_r = src109_r;
        src2 = src10;
    }
}

static void common_vt_4t_8w_msa(UWORD8 *src, WORD32 src_stride,
                                UWORD8 *dst, WORD32 dst_stride,
                                WORD8 *filter, WORD32 height)
{
    if(2 == height)
    {
        common_vt_4t_8x2_msa(src, src_stride, dst, dst_stride, filter);
    }
    else if(6 == height)
    {
        common_vt_4t_8x6_msa(src, src_stride, dst, dst_stride, filter);
    }
    else
    {
        common_vt_4t_8x4mult_msa(src, src_stride, dst, dst_stride,
                                 filter, height);
    }
}

static void common_vt_4t_12w_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, src5, src6;
    v16i8 vec0, vec1, vec2, vec3, vec4, vec5;
    v16u8 out0, out1;
    v8i16 src10, src21, src32, src43, src54, src65, src87, src109, src1211;
    v8i16 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, filt, filt0, filt1;
    v4u32 mask = { 2, 6, 2, 6 };

    filt = LD_SH(filter);
    SPLATI_H2_SH(filt, 0, 1, filt0, filt1);

    src -= src_stride;

    LD_SB3(src, src_stride, src0, src1, src2);
    src += (3 * src_stride);

    XORI_B3_128_SB(src0, src1, src2);
    VSHF_W2_SB(src0, src1, src1, src2, mask, mask, vec0, vec1);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src3, src4, src5, src6);
        src += (4 * src_stride);

        XORI_B4_128_SB(src3, src4, src5, src6);
        ILVR_B2_SH(src1, src0, src3, src2, src10, src32);
        VSHF_W2_SB(src2, src3, src3, src4, mask, mask, vec2, vec3);
        VSHF_W2_SB(src4, src5, src5, src6, mask, mask, vec4, vec5);
        tmp0 = FILT_4TAP_DPADD_S_H(src10, src32, filt0, filt1);
        ILVR_B4_SH(src2, src1, src4, src3, src5, src4, src6, src5, src21, src43,
                   src54, src65);
        tmp1 = FILT_4TAP_DPADD_S_H(src21, src43, filt0, filt1);
        tmp2 = FILT_4TAP_DPADD_S_H(src32, src54, filt0, filt1);
        tmp3 = FILT_4TAP_DPADD_S_H(src43, src65, filt0, filt1);
        ILVR_B3_SH(vec1, vec0, vec3, vec2, vec5, vec4, src87, src109, src1211);
        tmp4 = FILT_4TAP_DPADD_S_H(src87, src109, filt0, filt1);
        tmp5 = FILT_4TAP_DPADD_S_H(src109, src1211, filt0, filt1);
        SRARI_H4_SH(tmp0, tmp1, tmp2, tmp3, 6);
        SRARI_H2_SH(tmp4, tmp5, 6);
        SAT_SH4_SH(tmp0, tmp1, tmp2, tmp3, 7);
        SAT_SH2_SH(tmp4, tmp5, 7);
        out0 = PCKEV_XORI128_UB(tmp0, tmp1);
        out1 = PCKEV_XORI128_UB(tmp2, tmp3);
        ST8x4_UB(out0, out1, dst, dst_stride);
        out0 = PCKEV_XORI128_UB(tmp4, tmp5);
        ST4x4_UB(out0, out0, 0, 1, 2, 3, dst + 8, dst_stride);
        dst += (4 * dst_stride);

        src0 = src4;
        src1 = src5;
        src2 = src6;
        vec0 = vec4;
        vec1 = vec5;
        src2 = src6;
    }
}

static void common_vt_4t_16x2mult_msa(UWORD8 *src, WORD32 src_stride,
                                      UWORD8 *dst, WORD32 dst_stride,
                                      WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, src10_r, src32_r, src21_r, src43_r;
    v16i8 src10_l, src32_l, src21_l, src43_l, filt0, filt1;
    v16u8 tmp0, tmp1;
    v8i16 filt, out0_r, out1_r, out0_l, out1_l;

    src -= src_stride;

    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    LD_SB3(src, src_stride, src0, src1, src2);
    src += (3 * src_stride);

    XORI_B3_128_SB(src0, src1, src2);
    ILVR_B2_SB(src1, src0, src2, src1, src10_r, src21_r);
    ILVL_B2_SB(src1, src0, src2, src1, src10_l, src21_l);

    for(loop_cnt = (height >> 1); loop_cnt--;)
    {
        LD_SB2(src, src_stride, src3, src4);
        src += (2 * src_stride);

        XORI_B2_128_SB(src3, src4);
        ILVR_B2_SB(src3, src2, src4, src3, src32_r, src43_r);
        ILVL_B2_SB(src3, src2, src4, src3, src32_l, src43_l);
        out0_r = FILT_4TAP_DPADD_S_H(src10_r, src32_r, filt0, filt1);
        out1_r = FILT_4TAP_DPADD_S_H(src21_r, src43_r, filt0, filt1);
        out0_l = FILT_4TAP_DPADD_S_H(src10_l, src32_l, filt0, filt1);
        out1_l = FILT_4TAP_DPADD_S_H(src21_l, src43_l, filt0, filt1);
        SRARI_H4_SH(out0_r, out1_r, out0_l, out1_l, 6);
        SAT_SH4_SH(out0_r, out1_r, out0_l, out1_l, 7);
        PCKEV_B2_UB(out0_l, out0_r, out1_l, out1_r, tmp0, tmp1);
        XORI_B2_128_UB(tmp0, tmp1);
        ST_UB2(tmp0, tmp1, dst, dst_stride);
        dst += (2 * dst_stride);

        src10_r = src32_r;
        src21_r = src43_r;
        src10_l = src32_l;
        src21_l = src43_l;
        src2 = src4;
    }
}

static void common_vt_4t_16x4mult_msa(UWORD8 *src, WORD32 src_stride,
                                      UWORD8 *dst, WORD32 dst_stride,
                                      WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, src5, src6;
    v16i8 src10_r, src32_r, src54_r, src21_r, src43_r, src65_r, src10_l;
    v16i8 src32_l, src54_l, src21_l, src43_l, src65_l, filt0, filt1;
    v16u8 tmp0, tmp1, tmp2, tmp3;
    v8i16 filt, out0_r, out1_r, out2_r, out3_r, out0_l, out1_l, out2_l, out3_l;

    src -= src_stride;

    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    LD_SB3(src, src_stride, src0, src1, src2);
    src += (3 * src_stride);

    XORI_B3_128_SB(src0, src1, src2);
    ILVR_B2_SB(src1, src0, src2, src1, src10_r, src21_r);
    ILVL_B2_SB(src1, src0, src2, src1, src10_l, src21_l);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src3, src4, src5, src6);
        src += (4 * src_stride);

        XORI_B4_128_SB(src3, src4, src5, src6);
        ILVR_B4_SB(src3, src2, src4, src3, src5, src4, src6, src5, src32_r,
                   src43_r, src54_r, src65_r);
        ILVL_B4_SB(src3, src2, src4, src3, src5, src4, src6, src5, src32_l,
                   src43_l, src54_l, src65_l);
        out0_r = FILT_4TAP_DPADD_S_H(src10_r, src32_r, filt0, filt1);
        out1_r = FILT_4TAP_DPADD_S_H(src21_r, src43_r, filt0, filt1);
        out2_r = FILT_4TAP_DPADD_S_H(src32_r, src54_r, filt0, filt1);
        out3_r = FILT_4TAP_DPADD_S_H(src43_r, src65_r, filt0, filt1);
        out0_l = FILT_4TAP_DPADD_S_H(src10_l, src32_l, filt0, filt1);
        out1_l = FILT_4TAP_DPADD_S_H(src21_l, src43_l, filt0, filt1);
        out2_l = FILT_4TAP_DPADD_S_H(src32_l, src54_l, filt0, filt1);
        out3_l = FILT_4TAP_DPADD_S_H(src43_l, src65_l, filt0, filt1);
        SRARI_H4_SH(out0_r, out1_r, out2_r, out3_r, 6);
        SRARI_H4_SH(out0_l, out1_l, out2_l, out3_l, 6);
        SAT_SH4_SH(out0_r, out1_r, out2_r, out3_r, 7);
        SAT_SH4_SH(out0_l, out1_l, out2_l, out3_l, 7);
        PCKEV_B4_UB(out0_l, out0_r, out1_l, out1_r, out2_l, out2_r, out3_l,
                    out3_r, tmp0, tmp1, tmp2, tmp3);
        XORI_B4_128_UB(tmp0, tmp1, tmp2, tmp3);
        ST_UB4(tmp0, tmp1, tmp2, tmp3, dst, dst_stride);
        dst += (4 * dst_stride);

        src10_r = src54_r;
        src21_r = src65_r;
        src10_l = src54_l;
        src21_l = src65_l;
        src2 = src6;
    }
}

static void common_vt_4t_16w_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter, WORD32 height)
{
    if(height % 4 == 0)
        common_vt_4t_16x4mult_msa(src, src_stride, dst, dst_stride, filter,
                                  height);
    else
        common_vt_4t_16x2mult_msa(src, src_stride, dst, dst_stride, filter,
                                  height);
}

static void common_vt_4t_24w_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    uint64_t out0, out1;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v16i8 src10_r, src32_r, src76_r, src98_r, src21_r, src43_r, src87_r;
    v16i8 src11, filt0, filt1, src109_r, src10_l, src32_l, src21_l, src43_l;
    v16u8 out;
    v8i16 filt, out0_r, out1_r, out2_r, out3_r, out0_l, out1_l;

    src -= src_stride;

    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    LD_SB3(src, src_stride, src0, src1, src2);
    XORI_B3_128_SB(src0, src1, src2);
    ILVR_B2_SB(src1, src0, src2, src1, src10_r, src21_r);
    ILVL_B2_SB(src1, src0, src2, src1, src10_l, src21_l);

    LD_SB3(src + 16, src_stride, src6, src7, src8);
    src += (3 * src_stride);
    XORI_B3_128_SB(src6, src7, src8);
    ILVR_B2_SB(src7, src6, src8, src7, src76_r, src87_r);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB2(src, src_stride, src3, src4);
        XORI_B2_128_SB(src3, src4);
        ILVR_B2_SB(src3, src2, src4, src3, src32_r, src43_r);
        ILVL_B2_SB(src3, src2, src4, src3, src32_l, src43_l);

        LD_SB2(src + 16, src_stride, src9, src10);
        src += (2 * src_stride);
        XORI_B2_128_SB(src9, src10);
        ILVR_B2_SB(src9, src8, src10, src9, src98_r, src109_r);

        out0_r = FILT_4TAP_DPADD_S_H(src10_r, src32_r, filt0, filt1);
        out0_l = FILT_4TAP_DPADD_S_H(src10_l, src32_l, filt0, filt1);
        out1_r = FILT_4TAP_DPADD_S_H(src21_r, src43_r, filt0, filt1);
        out1_l = FILT_4TAP_DPADD_S_H(src21_l, src43_l, filt0, filt1);
        out2_r = FILT_4TAP_DPADD_S_H(src76_r, src98_r, filt0, filt1);
        out3_r = FILT_4TAP_DPADD_S_H(src87_r, src109_r, filt0, filt1);

        SRARI_H4_SH(out0_r, out1_r, out2_r, out3_r, 6);
        SRARI_H2_SH(out0_l, out1_l, 6);
        SAT_SH4_SH(out0_r, out1_r, out2_r, out3_r, 7);
        SAT_SH2_SH(out0_l, out1_l, 7);
        out = PCKEV_XORI128_UB(out0_r, out0_l);
        ST_UB(out, dst);
        PCKEV_B2_SH(out2_r, out2_r, out3_r, out3_r, out2_r, out3_r);
        XORI_B2_128_SH(out2_r, out3_r);
        out0 = __msa_copy_u_d((v2i64) out2_r, 0);
        out1 = __msa_copy_u_d((v2i64) out3_r, 0);
        SD(out0, dst + 16);
        dst += dst_stride;
        out = PCKEV_XORI128_UB(out1_r, out1_l);
        ST_UB(out, dst);
        SD(out1, dst + 16);
        dst += dst_stride;

        LD_SB2(src, src_stride, src5, src2);
        XORI_B2_128_SB(src5, src2);
        ILVR_B2_SB(src5, src4, src2, src5, src10_r, src21_r);
        ILVL_B2_SB(src5, src4, src2, src5, src10_l, src21_l);

        LD_SB2(src + 16, src_stride, src11, src8);
        src += (2 * src_stride);
        XORI_B2_128_SB(src11, src8);
        ILVR_B2_SB(src11, src10, src8, src11, src76_r, src87_r);

        out0_r = FILT_4TAP_DPADD_S_H(src32_r, src10_r, filt0, filt1);
        out0_l = FILT_4TAP_DPADD_S_H(src32_l, src10_l, filt0, filt1);
        out1_r = FILT_4TAP_DPADD_S_H(src43_r, src21_r, filt0, filt1);
        out1_l = FILT_4TAP_DPADD_S_H(src43_l, src21_l, filt0, filt1);
        out2_r = FILT_4TAP_DPADD_S_H(src98_r, src76_r, filt0, filt1);
        out3_r = FILT_4TAP_DPADD_S_H(src109_r, src87_r, filt0, filt1);

        SRARI_H4_SH(out0_r, out1_r, out2_r, out3_r, 6);
        SRARI_H2_SH(out0_l, out1_l, 6);
        SAT_SH4_SH(out0_r, out1_r, out2_r, out3_r, 7);
        SAT_SH2_SH(out0_l, out1_l, 7);
        out = PCKEV_XORI128_UB(out0_r, out0_l);
        ST_UB(out, dst);
        out = PCKEV_XORI128_UB(out2_r, out2_r);
        ST8x1_UB(out, dst + 16)
        dst += dst_stride;
        out = PCKEV_XORI128_UB(out1_r, out1_l);
        ST_UB(out, dst);
        out = PCKEV_XORI128_UB(out3_r, out3_r);
        ST8x1_UB(out, dst + 16)
        dst += dst_stride;
    }
}

static void common_vt_4t_32w_mult_msa(UWORD8 *src, WORD32 src_stride,
                                      UWORD8 *dst, WORD32 dst_stride,
                                      WORD8 *filter, WORD32 height,
                                      WORD32 width)
{
    UWORD32 loop_cnt, cnt;
    UWORD8 *dst_tmp, *src_tmp;
    v16u8 out;
    v16i8 src0, src1, src2, src3, src4, src6, src7, src8, src9, src10;
    v16i8 src10_r, src32_r, src76_r, src98_r, src21_r, src43_r, src87_r;
    v16i8 src109_r, src10_l, src32_l, src76_l, src98_l, src21_l, src43_l;
    v16i8 src87_l, src109_l, filt0, filt1;
    v8i16 out0_r, out1_r, out2_r, out3_r, out0_l, out1_l, out2_l, out3_l;
    v8i16 filt;

    src -= src_stride;

    filt = LD_SH(filter);
    SPLATI_H2_SB(filt, 0, 1, filt0, filt1);

    for(cnt = (width >> 5); cnt--;)
    {
        dst_tmp = dst;
        src_tmp = src;

        LD_SB3(src_tmp, src_stride, src0, src1, src2);
        XORI_B3_128_SB(src0, src1, src2);

        ILVR_B2_SB(src1, src0, src2, src1, src10_r, src21_r);
        ILVL_B2_SB(src1, src0, src2, src1, src10_l, src21_l);

        LD_SB3(src_tmp + 16, src_stride, src6, src7, src8);
        src_tmp += (3 * src_stride);

        XORI_B3_128_SB(src6, src7, src8);
        ILVR_B2_SB(src7, src6, src8, src7, src76_r, src87_r);
        ILVL_B2_SB(src7, src6, src8, src7, src76_l, src87_l);

        for(loop_cnt = (height >> 1); loop_cnt--;)
        {
            LD_SB2(src_tmp, src_stride, src3, src4);
            XORI_B2_128_SB(src3, src4);
            ILVR_B2_SB(src3, src2, src4, src3, src32_r, src43_r);
            ILVL_B2_SB(src3, src2, src4, src3, src32_l, src43_l);

            out0_r = FILT_4TAP_DPADD_S_H(src10_r, src32_r, filt0, filt1);
            out0_l = FILT_4TAP_DPADD_S_H(src10_l, src32_l, filt0, filt1);
            out1_r = FILT_4TAP_DPADD_S_H(src21_r, src43_r, filt0, filt1);
            out1_l = FILT_4TAP_DPADD_S_H(src21_l, src43_l, filt0, filt1);

            SRARI_H4_SH(out0_r, out1_r, out0_l, out1_l, 6);
            SAT_SH4_SH(out0_r, out1_r, out0_l, out1_l, 7);
            out = PCKEV_XORI128_UB(out0_r, out0_l);
            ST_UB(out, dst_tmp);
            out = PCKEV_XORI128_UB(out1_r, out1_l);
            ST_UB(out, dst_tmp + dst_stride);

            src10_r = src32_r;
            src21_r = src43_r;
            src10_l = src32_l;
            src21_l = src43_l;
            src2 = src4;

            LD_SB2(src_tmp + 16, src_stride, src9, src10);
            src_tmp += (2 * src_stride);
            XORI_B2_128_SB(src9, src10);
            ILVR_B2_SB(src9, src8, src10, src9, src98_r, src109_r);
            ILVL_B2_SB(src9, src8, src10, src9, src98_l, src109_l);

            out2_r = FILT_4TAP_DPADD_S_H(src76_r, src98_r, filt0, filt1);
            out2_l = FILT_4TAP_DPADD_S_H(src76_l, src98_l, filt0, filt1);
            out3_r = FILT_4TAP_DPADD_S_H(src87_r, src109_r, filt0, filt1);
            out3_l = FILT_4TAP_DPADD_S_H(src87_l, src109_l, filt0, filt1);

            SRARI_H4_SH(out2_r, out3_r, out2_l, out3_l, 6);
            SAT_SH4_SH(out2_r, out3_r, out2_l, out3_l, 7);
            out = PCKEV_XORI128_UB(out2_r, out2_l);
            ST_UB(out, dst_tmp + 16);
            out = PCKEV_XORI128_UB(out3_r, out3_l);
            ST_UB(out, dst_tmp + 16 + dst_stride);

            dst_tmp += 2 * dst_stride;

            src76_r = src98_r;
            src87_r = src109_r;
            src76_l = src98_l;
            src87_l = src109_l;
            src8 = src10;
        }

        src += 32;
        dst += 32;
    }
}

static void common_vt_4t_32w_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter, WORD32 height)
{
    common_vt_4t_32w_mult_msa(src, src_stride, dst, dst_stride,
                              filter, height, 32);
}

static void common_vt_4t_48w_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter, WORD32 height)
{
    common_vt_4t_32w_mult_msa(src, src_stride, dst, dst_stride,
                              filter, height, 32);
    common_vt_4t_16w_msa(src + 32, src_stride, dst + 32, dst_stride,
                         filter, height);
}

static void common_vt_4t_64w_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter, WORD32 height)
{
    common_vt_4t_32w_mult_msa(src, src_stride, dst, dst_stride,
                              filter, height, 64);
}

static void copy_12x16_msa(UWORD8 *src, WORD32 src_stride,
                           UWORD8 *dst, WORD32 dst_stride)
{
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;

    LD_UB8(src, src_stride, src0, src1, src2, src3, src4, src5, src6, src7);
    src += (8 * src_stride);
    ST12x8_UB(src0, src1, src2, src3, src4, src5, src6, src7,
              dst, dst_stride);
    dst += (8 * dst_stride);
    LD_UB8(src, src_stride, src0, src1, src2, src3, src4, src5, src6, src7);
    ST12x8_UB(src0, src1, src2, src3, src4, src5, src6, src7,
              dst, dst_stride);
}

static void copy_16x2_msa(UWORD8 *src, WORD32 src_stride,
                          UWORD8 *dst, WORD32 dst_stride)
{
    v16u8 src0, src1;

    LD_UB2(src, src_stride, src0, src1);
    ST_UB2(src0, src1, dst, dst_stride);
}

static void copy_16x6_msa(UWORD8 *src, WORD32 src_stride,
                          UWORD8 *dst, WORD32 dst_stride)
{
    v16u8 src0, src1, src2, src3, src4, src5;

    LD_UB6(src, src_stride, src0, src1, src2, src3, src4, src5);
    ST_UB4(src0, src1, src2, src3, dst, dst_stride);
    ST_UB2(src4, src5, dst + 4 * dst_stride, dst_stride);
}

static void copy_width8_msa(UWORD8 *src, WORD32 src_stride,
                            UWORD8 *dst, WORD32 dst_stride,
                            WORD32 height)
{
    WORD32 cnt;
    uint64_t out0, out1, out2, out3, out4, out5, out6, out7;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;

    if(0 == height % 12)
    {
        for(cnt = (height / 12); cnt--;)
        {
            LD_UB8(src, src_stride,
                   src0, src1, src2, src3, src4, src5, src6, src7);
            src += (8 * src_stride);

            out0 = __msa_copy_u_d((v2i64) src0, 0);
            out1 = __msa_copy_u_d((v2i64) src1, 0);
            out2 = __msa_copy_u_d((v2i64) src2, 0);
            out3 = __msa_copy_u_d((v2i64) src3, 0);
            out4 = __msa_copy_u_d((v2i64) src4, 0);
            out5 = __msa_copy_u_d((v2i64) src5, 0);
            out6 = __msa_copy_u_d((v2i64) src6, 0);
            out7 = __msa_copy_u_d((v2i64) src7, 0);

            SD4(out0, out1, out2, out3, dst, dst_stride);
            dst += (4 * dst_stride);
            SD4(out4, out5, out6, out7, dst, dst_stride);
            dst += (4 * dst_stride);

            LD_UB4(src, src_stride, src0, src1, src2, src3);
            src += (4 * src_stride);

            out0 = __msa_copy_u_d((v2i64) src0, 0);
            out1 = __msa_copy_u_d((v2i64) src1, 0);
            out2 = __msa_copy_u_d((v2i64) src2, 0);
            out3 = __msa_copy_u_d((v2i64) src3, 0);

            SD4(out0, out1, out2, out3, dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
    else if(0 == height % 8)
    {
        for(cnt = height >> 3; cnt--;)
        {
            LD_UB8(src, src_stride,
                   src0, src1, src2, src3, src4, src5, src6, src7);
            src += (8 * src_stride);

            out0 = __msa_copy_u_d((v2i64) src0, 0);
            out1 = __msa_copy_u_d((v2i64) src1, 0);
            out2 = __msa_copy_u_d((v2i64) src2, 0);
            out3 = __msa_copy_u_d((v2i64) src3, 0);
            out4 = __msa_copy_u_d((v2i64) src4, 0);
            out5 = __msa_copy_u_d((v2i64) src5, 0);
            out6 = __msa_copy_u_d((v2i64) src6, 0);
            out7 = __msa_copy_u_d((v2i64) src7, 0);

            SD4(out0, out1, out2, out3, dst, dst_stride);
            dst += (4 * dst_stride);
            SD4(out4, out5, out6, out7, dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
    else if(0 == height % 4)
    {
        for(cnt = (height / 4); cnt--;)
        {
            LD_UB4(src, src_stride, src0, src1, src2, src3);
            src += (4 * src_stride);
            out0 = __msa_copy_u_d((v2i64) src0, 0);
            out1 = __msa_copy_u_d((v2i64) src1, 0);
            out2 = __msa_copy_u_d((v2i64) src2, 0);
            out3 = __msa_copy_u_d((v2i64) src3, 0);

            SD4(out0, out1, out2, out3, dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
    else if(0 == height % 2)
    {
        for(cnt = (height / 2); cnt--;)
        {
            LD_UB2(src, src_stride, src0, src1);
            src += (2 * src_stride);
            out0 = __msa_copy_u_d((v2i64) src0, 0);
            out1 = __msa_copy_u_d((v2i64) src1, 0);

            SD(out0, dst);
            dst += dst_stride;
            SD(out1, dst);
            dst += dst_stride;
        }
    }
}

static void copy_16multx8mult_msa(UWORD8 *src, WORD32 src_stride,
                                  UWORD8 *dst, WORD32 dst_stride,
                                  WORD32 height, WORD32 width)
{
    WORD32 cnt, loop_cnt;
    UWORD8 *src_tmp, *dst_tmp;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;

    for(cnt = (width >> 4); cnt--;)
    {
        src_tmp = src;
        dst_tmp = dst;

        for(loop_cnt = (height >> 3); loop_cnt--;)
        {
            LD_UB8(src_tmp, src_stride,
                   src0, src1, src2, src3, src4, src5, src6, src7);
            src_tmp += (8 * src_stride);

            ST_UB8(src0, src1, src2, src3, src4, src5, src6, src7,
                   dst_tmp, dst_stride);
            dst_tmp += (8 * dst_stride);
        }

        src += 16;
        dst += 16;
    }
}

static void copy_width16_msa(UWORD8 *src, WORD32 src_stride,
                             UWORD8 *dst, WORD32 dst_stride,
                             WORD32 height)
{
    WORD32 cnt;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;

    if(0 == height % 12)
    {
        for(cnt = (height / 12); cnt--;)
        {
            LD_UB8(src, src_stride,
                   src0, src1, src2, src3, src4, src5, src6, src7);
            src += (8 * src_stride);
            ST_UB8(src0, src1, src2, src3, src4, src5, src6, src7,
                   dst, dst_stride);
            dst += (8 * dst_stride);

            LD_UB4(src, src_stride, src0, src1, src2, src3);
            src += (4 * src_stride);
            ST_UB4(src0, src1, src2, src3, dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
    else if(0 == height % 8)
    {
        copy_16multx8mult_msa(src, src_stride, dst, dst_stride, height, 16);
    }
    else if(0 == height % 4)
    {
        for(cnt = (height >> 2); cnt--;)
        {
            LD_UB4(src, src_stride, src0, src1, src2, src3);
            src += (4 * src_stride);

            ST_UB4(src0, src1, src2, src3, dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
}

static void copy_width24_msa(UWORD8 *src, WORD32 src_stride,
                             UWORD8 *dst, WORD32 dst_stride,
                             WORD32 height)
{
    copy_16multx8mult_msa(src, src_stride, dst, dst_stride, height, 16);
    copy_width8_msa(src + 16, src_stride, dst + 16, dst_stride, height);
}

static void copy_width32_msa(UWORD8 *src, WORD32 src_stride,
                             UWORD8 *dst, WORD32 dst_stride,
                             WORD32 height)
{
    WORD32 cnt;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;

    if(0 == height % 12)
    {
        for(cnt = (height / 12); cnt--;)
        {
            LD_UB4(src, src_stride, src0, src1, src2, src3);
            LD_UB4(src + 16, src_stride, src4, src5, src6, src7);
            src += (4 * src_stride);
            ST_UB4(src0, src1, src2, src3, dst, dst_stride);
            ST_UB4(src4, src5, src6, src7, dst + 16, dst_stride);
            dst += (4 * dst_stride);

            LD_UB4(src, src_stride, src0, src1, src2, src3);
            LD_UB4(src + 16, src_stride, src4, src5, src6, src7);
            src += (4 * src_stride);
            ST_UB4(src0, src1, src2, src3, dst, dst_stride);
            ST_UB4(src4, src5, src6, src7, dst + 16, dst_stride);
            dst += (4 * dst_stride);

            LD_UB4(src, src_stride, src0, src1, src2, src3);
            LD_UB4(src + 16, src_stride, src4, src5, src6, src7);
            src += (4 * src_stride);
            ST_UB4(src0, src1, src2, src3, dst, dst_stride);
            ST_UB4(src4, src5, src6, src7, dst + 16, dst_stride);
            dst += (4 * dst_stride);
        }
    }
    else if(0 == height % 8)
    {
        copy_16multx8mult_msa(src, src_stride, dst, dst_stride, height, 32);
    }
    else if(0 == height % 4)
    {
        for(cnt = (height >> 2); cnt--;)
        {
            LD_UB4(src, src_stride, src0, src1, src2, src3);
            LD_UB4(src + 16, src_stride, src4, src5, src6, src7);
            src += (4 * src_stride);
            ST_UB4(src0, src1, src2, src3, dst, dst_stride);
            ST_UB4(src4, src5, src6, src7, dst + 16, dst_stride);
            dst += (4 * dst_stride);
        }
    }
}

static void copy_width48_msa(UWORD8 *src, WORD32 src_stride,
                             UWORD8 *dst, WORD32 dst_stride,
                             WORD32 height)
{
    copy_16multx8mult_msa(src, src_stride, dst, dst_stride, height, 48);
}

static void copy_width64_msa(UWORD8 *src, WORD32 src_stride,
                             UWORD8 *dst, WORD32 dst_stride,
                             WORD32 height)
{
    copy_16multx8mult_msa(src, src_stride, dst, dst_stride, height, 64);
}

void ihevc_inter_pred_chroma_horz_msa(UWORD8 *pu1_src, UWORD8 *pu1_dst,
                                      WORD32 src_strd, WORD32 dst_strd,
                                      WORD8 *pi1_coeff, WORD32 ht, WORD32 wd)
{
    switch(wd)
    {
        case 2:
            common_ilv_hz_4t_2w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                    pi1_coeff, ht);
            break;
        case 4:
            common_ilv_hz_4t_4w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                    pi1_coeff, ht);
            break;
        case 6:
            common_ilv_hz_4t_6w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                    pi1_coeff, ht);
            break;
        case 8:
            common_ilv_hz_4t_8w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                    pi1_coeff, ht);
            break;
        case 12:
            common_ilv_hz_4t_12w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                     pi1_coeff, ht);
            break;
        case 16:
            common_ilv_hz_4t_16w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                     pi1_coeff, ht);
            break;
        case 24:
            common_ilv_hz_4t_24w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                     pi1_coeff, ht);
            break;
        case 32:
            common_ilv_hz_4t_32w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                     pi1_coeff, ht);
    }
}

void ihevc_inter_pred_chroma_horz_w16out_msa(UWORD8 *pu1_src, WORD16 *pi2_dst,
                                             WORD32 src_strd, WORD32 dst_strd,
                                             WORD8 *pi1_coeff, WORD32 ht,
                                             WORD32 wd)
{
    switch(wd)
    {
        case 2:
            hevc_ilv_hz_4t_2w_msa(pu1_src, src_strd, pi2_dst, dst_strd,
                                  pi1_coeff, ht);
            break;
        case 4:
            hevc_ilv_hz_4t_4w_msa(pu1_src, src_strd, pi2_dst, dst_strd,
                                  pi1_coeff, ht);
            break;
        case 6:
            hevc_ilv_hz_4t_6w_msa(pu1_src, src_strd, pi2_dst, dst_strd,
                                  pi1_coeff, ht);
            break;
        case 8:
            hevc_ilv_hz_4t_8w_msa(pu1_src, src_strd, pi2_dst, dst_strd,
                                  pi1_coeff, ht);
            break;
        case 12:
            hevc_ilv_hz_4t_12w_msa(pu1_src, src_strd, pi2_dst, dst_strd,
                                   pi1_coeff, ht);
            break;
        case 16:
            hevc_ilv_hz_4t_16w_msa(pu1_src, src_strd, pi2_dst, dst_strd,
                                   pi1_coeff, ht);
            break;
        case 24:
            hevc_ilv_hz_4t_24w_msa(pu1_src, src_strd, pi2_dst, dst_strd,
                                   pi1_coeff, ht);
            break;
        case 32:
            hevc_ilv_hz_4t_32w_msa(pu1_src, src_strd, pi2_dst, dst_strd,
                                   pi1_coeff, ht);
    }
}

void ihevc_inter_pred_chroma_vert_w16out_msa(UWORD8 *pu1_src, WORD16 *pi2_dst,
                                             WORD32 src_strd, WORD32 dst_strd,
                                             WORD8 *pi1_coeff, WORD32 ht,
                                             WORD32 wd)
{
    switch(wd)
    {
        case 2:
            hevc_vt_4t_4w_msa(pu1_src, src_strd, pi2_dst, dst_strd,
                              pi1_coeff, ht);
            break;
        case 4:
            hevc_vt_4t_8w_msa(pu1_src, src_strd, pi2_dst, dst_strd,
                              pi1_coeff, ht);
            break;
        case 6:
            hevc_vt_4t_12w_msa(pu1_src, src_strd, pi2_dst, dst_strd,
                               pi1_coeff, ht);
            break;
        case 8:
            hevc_vt_4t_16w_msa(pu1_src, src_strd, pi2_dst, dst_strd,
                               pi1_coeff, ht);
            break;
        case 12:
            hevc_vt_4t_24w_msa(pu1_src, src_strd, pi2_dst, dst_strd,
                               pi1_coeff, ht);
            break;
        case 16:
            hevc_vt_4t_32w_msa(pu1_src, src_strd, pi2_dst, dst_strd,
                               pi1_coeff, ht);
            break;
        case 24:
            hevc_vt_4t_32w_msa(pu1_src, src_strd, pi2_dst, dst_strd,
                               pi1_coeff, ht);
            hevc_vt_4t_16w_msa(pu1_src + 32, src_strd, pi2_dst + 32, dst_strd,
                               pi1_coeff, ht);
            break;
        case 32:
            hevc_vt_4t_32w_msa(pu1_src, src_strd, pi2_dst, dst_strd,
                               pi1_coeff, ht);
            hevc_vt_4t_32w_msa(pu1_src + 32, src_strd, pi2_dst + 32, dst_strd,
                               pi1_coeff, ht);
    }
}

void ihevc_inter_pred_chroma_copy_w16out_msa(UWORD8 *pu1_src, WORD16 *pi2_dst,
                                             WORD32 src_strd, WORD32 dst_strd,
                                             WORD8 *pi1_coeff, WORD32 ht,
                                             WORD32 wd)
{
    UNUSED(pi1_coeff);

    switch(wd)
    {
        case 2:
            hevc_copy_4w_msa(pu1_src, src_strd, pi2_dst, dst_strd, ht);
            break;
        case 4:
            hevc_copy_8w_msa(pu1_src, src_strd, pi2_dst, dst_strd, ht);
            break;
        case 6:
            hevc_copy_12w_msa(pu1_src, src_strd, pi2_dst, dst_strd, ht);
            break;
        case 8:
            if(2 == ht)
            {
                hevc_copy_16x2_msa(pu1_src, src_strd, pi2_dst, dst_strd);
            }
            else if(4 == ht)
            {
                hevc_copy_16x4_msa(pu1_src, src_strd, pi2_dst, dst_strd);
            }
            else if(6 == ht)
            {
                hevc_copy_16x6_msa(pu1_src, src_strd, pi2_dst, dst_strd);
            }
            else if(12 == ht)
            {
                hevc_copy_16x12_msa(pu1_src, src_strd, pi2_dst, dst_strd);
            }
            else if(0 == (ht % 8))
            {
                hevc_copy_16w_msa(pu1_src, src_strd, pi2_dst, dst_strd, ht);
            }
            break;
        case 12:
            hevc_copy_24w_msa(pu1_src, src_strd, pi2_dst, dst_strd, ht);
            break;
        case 16:
            if(0 == (ht % 8))
            {
                hevc_copy_32w_msa(pu1_src, src_strd, pi2_dst, dst_strd, ht);
            }
            else if(12 == ht)
            {
                hevc_copy_16x12_msa(pu1_src, src_strd, pi2_dst, dst_strd);
                hevc_copy_16x12_msa(pu1_src + 16, src_strd, pi2_dst + 16,
                                    dst_strd);
            }
            else if(4 == ht)
            {
                hevc_copy_16x4_msa(pu1_src, src_strd, pi2_dst, dst_strd);
                hevc_copy_16x4_msa(pu1_src + 16, src_strd, pi2_dst + 16,
                                   dst_strd);
            }
            break;
        case 24:
            hevc_copy_48w_msa(pu1_src, src_strd, pi2_dst, dst_strd, ht);
            break;
        case 32:
            hevc_copy_64w_msa(pu1_src, src_strd, pi2_dst, dst_strd, ht);
    }
}

void ihevc_inter_pred_chroma_vert_w16inp_msa(WORD16 *pi2_src, UWORD8 *pu1_dst,
                                             WORD32 src_strd, WORD32 dst_strd,
                                             WORD8 *pi1_coeff, WORD32 ht,
                                             WORD32 wd)
{
    switch(wd)
    {
        case 2:
            if(2 == ht)
            {
                hevc_vt_uni_4t_4x2_w16inp_msa(pi2_src, src_strd, pu1_dst,
                                              dst_strd, pi1_coeff);
            }
            else if(4 == ht)
            {
                hevc_vt_uni_4t_4x4_w16inp_msa(pi2_src, src_strd, pu1_dst,
                                              dst_strd, pi1_coeff);
            }
            else if(0 == (ht % 8))
            {
                hevc_vt_uni_4t_4x8mult_w16inp_msa(pi2_src, src_strd, pu1_dst,
                                                  dst_strd, pi1_coeff, ht);
            }
            break;
        case 4:
            if(2 == ht)
            {
                hevc_vt_uni_4t_8x2_w16inp_msa(pi2_src, src_strd, pu1_dst,
                                              dst_strd, pi1_coeff);
            }
            else
            {
                hevc_vt_uni_4t_8x4mult_w16inp_msa(pi2_src, src_strd, pu1_dst,
                                                  dst_strd, pi1_coeff, ht);
            }
            break;
        case 6:
            hevc_vt_uni_4t_12x4mult_w16inp_msa(pi2_src, src_strd, pu1_dst,
                                               dst_strd, pi1_coeff, ht);
            break;
        case 8:
            if(0 == (ht % 4))
            {
                hevc_vt_uni_4t_16multx4mult_w16inp_msa(pi2_src, src_strd,
                                                       pu1_dst, dst_strd,
                                                       pi1_coeff, ht, 16);
            }
            else
            {
                hevc_vt_uni_4t_16x2mult_w16inp_msa(pi2_src, src_strd, pu1_dst,
                                                   dst_strd, pi1_coeff, ht);
            }
            break;
        case 12:
            hevc_vt_uni_4t_16multx4mult_w16inp_msa(pi2_src, src_strd, pu1_dst,
                                                   dst_strd, pi1_coeff, ht, 16);
            hevc_vt_uni_4t_8x4mult_w16inp_msa(pi2_src + 16, src_strd,
                                              pu1_dst + 16, dst_strd,
                                              pi1_coeff, ht);
            break;
        case 16:
            hevc_vt_uni_4t_16multx4mult_w16inp_msa(pi2_src, src_strd, pu1_dst,
                                                   dst_strd, pi1_coeff, ht, 32);
            break;
        case 24:
            hevc_vt_uni_4t_16multx4mult_w16inp_msa(pi2_src, src_strd, pu1_dst,
                                                   dst_strd, pi1_coeff, ht, 48);
            break;
        case 32:
            hevc_vt_uni_4t_16multx4mult_w16inp_msa(pi2_src, src_strd, pu1_dst,
                                                   dst_strd, pi1_coeff, ht, 64);
    }
}

void ihevc_inter_pred_chroma_vert_w16inp_w16out_msa(WORD16 *pi2_src,
                                                    WORD16 *pi2_dst,
                                                    WORD32 src_strd,
                                                    WORD32 dst_strd,
                                                    WORD8 *pi1_coeff,
                                                    WORD32 ht, WORD32 wd)
{
    switch(wd)
    {
        case 2:
            if(2 == ht)
            {
                hevc_vt_4t_4x2_w16inp_msa(pi2_src, src_strd, pi2_dst, dst_strd,
                                          pi1_coeff);
            }
            else if(4 == ht)
            {
                hevc_vt_4t_4x4_w16inp_msa(pi2_src, src_strd, pi2_dst, dst_strd,
                                          pi1_coeff);
            }
            else if(0 == (ht % 8))
            {
                hevc_vt_4t_4x8mult_w16inp_msa(pi2_src, src_strd, pi2_dst,
                                              dst_strd, pi1_coeff, ht);
            }
            break;
        case 4:
            if(2 == ht)
            {
                hevc_vt_4t_8x2_w16inp_msa(pi2_src, src_strd, pi2_dst, dst_strd,
                                          pi1_coeff);
            }
            else
            {
                hevc_vt_4t_8x4mult_w16inp_msa(pi2_src, src_strd, pi2_dst,
                                              dst_strd, pi1_coeff, ht);
            }
            break;
        case 6:
            hevc_vt_4t_12x4mult_w16inp_msa(pi2_src, src_strd, pi2_dst, dst_strd,
                                           pi1_coeff, ht);
            break;
        case 8:
            if(0 == (ht % 4))
            {
                hevc_vt_4t_16multx4mult_w16inp_msa(pi2_src, src_strd, pi2_dst,
                                                   dst_strd, pi1_coeff, ht, 16);
            }
            else
            {
                hevc_vt_4t_16x2mult_w16inp_msa(pi2_src, src_strd, pi2_dst,
                                               dst_strd, pi1_coeff, ht);
            }
            break;
        case 12:
            hevc_vt_4t_16multx4mult_w16inp_msa(pi2_src, src_strd, pi2_dst,
                                               dst_strd, pi1_coeff, ht, 16);
            hevc_vt_4t_8x4mult_w16inp_msa(pi2_src + 16, src_strd, pi2_dst + 16,
                                          dst_strd, pi1_coeff, ht);
            break;
        case 16:
            hevc_vt_4t_16multx4mult_w16inp_msa(pi2_src, src_strd, pi2_dst,
                                               dst_strd, pi1_coeff, ht, 32);
            break;
        case 24:
            hevc_vt_4t_16multx4mult_w16inp_msa(pi2_src, src_strd, pi2_dst,
                                               dst_strd, pi1_coeff, ht, 48);
            break;
        case 32:
            hevc_vt_4t_16multx4mult_w16inp_msa(pi2_src, src_strd, pi2_dst,
                                               dst_strd, pi1_coeff, ht, 64);
            break;
    }
}

void ihevc_inter_pred_chroma_copy_msa(UWORD8 *pu1_src, UWORD8 *pu1_dst,
                                      WORD32 src_strd, WORD32 dst_strd,
                                      WORD8 *pi1_coeff, WORD32 ht, WORD32 wd)
{
    WORD32 row;
    UNUSED(pi1_coeff);

    switch(wd)
    {
        case 2:
            for(row = 0; row < ht; row++)
            {
                UWORD32 tmp;

                tmp = LW(pu1_src);
                SW(tmp, pu1_dst);

                pu1_src += src_strd;
                pu1_dst += dst_strd;
            }
            break;
        case 4:
            copy_width8_msa(pu1_src, src_strd, pu1_dst, dst_strd, ht);
            break;
        case 6:
            copy_12x16_msa(pu1_src, src_strd, pu1_dst, dst_strd);
            break;
        case 8:
            if(2 == ht)
            {
                copy_16x2_msa(pu1_src, src_strd, pu1_dst, dst_strd);
            }
            else if(6 == ht)
            {
                copy_16x6_msa(pu1_src, src_strd, pu1_dst, dst_strd);
            }
            else
            {
                copy_width16_msa(pu1_src, src_strd, pu1_dst, dst_strd, ht);
            }
            break;
        case 12:
           copy_width24_msa(pu1_src, src_strd, pu1_dst, dst_strd, ht);
           break;
        case 16:
           copy_width32_msa(pu1_src, src_strd, pu1_dst, dst_strd, ht);
           break;
        case 24:
           copy_width48_msa(pu1_src, src_strd, pu1_dst, dst_strd, ht);
           break;
        case 32:
           copy_width64_msa(pu1_src, src_strd, pu1_dst, dst_strd, ht);
    }
}

void ihevc_inter_pred_chroma_vert_msa(UWORD8 *pu1_src, UWORD8 *pu1_dst,
                                      WORD32 src_strd, WORD32 dst_strd,
                                      WORD8 *pi1_coeff, WORD32 ht, WORD32 wd)
{
    switch(wd)
    {
        case 2:
            common_vt_4t_4w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                pi1_coeff, ht);
            break;
        case 4:
            common_vt_4t_8w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                pi1_coeff, ht);
            break;
        case 6:
            common_vt_4t_12w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                 pi1_coeff, ht);
            break;
        case 8:
            common_vt_4t_16w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                 pi1_coeff, ht);
            break;
        case 12:
            common_vt_4t_24w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                 pi1_coeff, ht);
            break;
        case 16:
            common_vt_4t_32w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                 pi1_coeff, ht);
            break;
        case 24:
            common_vt_4t_48w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                 pi1_coeff, ht);
            break;
        case 32:
            common_vt_4t_64w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                 pi1_coeff, ht);
    }
}
