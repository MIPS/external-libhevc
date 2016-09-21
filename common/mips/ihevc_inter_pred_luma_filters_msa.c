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

static const UWORD8 mc_filt_mask_arr[16 * 3] =
{
    /* 8 width cases */
    0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8,
    /* 4 width cases */
    0, 1, 1, 2, 2, 3, 3, 4, 16, 17, 17, 18, 18, 19, 19, 20,
    /* 4 width cases */
    8, 9, 9, 10, 10, 11, 11, 12, 24, 25, 25, 26, 26, 27, 27, 28
};

#define HEVC_FILT_8TAP(in0, in1, in2, in3,                       \
                       filt0, filt1, filt2, filt3)               \
( {                                                              \
    v4i32 out_m;                                                 \
                                                                 \
    out_m = __msa_dotp_s_w((v8i16) in0, (v8i16) filt0);          \
    out_m = __msa_dpadd_s_w(out_m, (v8i16) in1, (v8i16) filt1);  \
    DPADD_SH2_SW(in2, in3, filt2, filt3, out_m, out_m);          \
    out_m;                                                       \
} )

#define FILT_8TAP_DPADD_S_H(vec0, vec1, vec2, vec3,             \
                            filt0, filt1, filt2, filt3)         \
( {                                                             \
    v8i16 tmp0, tmp1;                                           \
                                                                \
    tmp0 = __msa_dotp_s_h((v16i8) vec0, (v16i8) filt0);         \
    tmp0 = __msa_dpadd_s_h(tmp0, (v16i8) vec1, (v16i8) filt1);  \
    tmp1 = __msa_dotp_s_h((v16i8) vec2, (v16i8) filt2);         \
    tmp1 = __msa_dpadd_s_h(tmp1, (v16i8) vec3, (v16i8) filt3);  \
    tmp0 = __msa_adds_s_h(tmp0, tmp1);                          \
                                                                \
    tmp0;                                                       \
} )

#define HORIZ_8TAP_4WID_4VECS_FILT(src0, src1, src2, src3,                  \
                                   mask0, mask1, mask2, mask3,              \
                                   filt0, filt1, filt2, filt3,              \
                                   out0, out1)                              \
{                                                                           \
    v16i8 vec0_m, vec1_m, vec2_m, vec3_m,  vec4_m, vec5_m, vec6_m, vec7_m;  \
    v8i16 res0_m, res1_m, res2_m, res3_m;                                   \
                                                                            \
    VSHF_B2_SB(src0, src1, src2, src3, mask0, mask0, vec0_m, vec1_m);       \
    DOTP_SB2_SH(vec0_m, vec1_m, filt0, filt0, res0_m, res1_m);              \
    VSHF_B2_SB(src0, src1, src2, src3, mask1, mask1, vec2_m, vec3_m);       \
    DPADD_SB2_SH(vec2_m, vec3_m, filt1, filt1, res0_m, res1_m);             \
    VSHF_B2_SB(src0, src1, src2, src3, mask2, mask2, vec4_m, vec5_m);       \
    DOTP_SB2_SH(vec4_m, vec5_m, filt2, filt2, res2_m, res3_m);              \
    VSHF_B2_SB(src0, src1, src2, src3, mask3, mask3, vec6_m, vec7_m);       \
    DPADD_SB2_SH(vec6_m, vec7_m, filt3, filt3, res2_m, res3_m);             \
    ADDS_SH2_SH(res0_m, res2_m, res1_m, res3_m, out0, out1);                \
}

#define HORIZ_8TAP_8WID_4VECS_FILT(src0, src1, src2, src3,                    \
                                   mask0, mask1, mask2, mask3,                \
                                   filt0, filt1, filt2, filt3,                \
                                   out0, out1, out2, out3)                    \
{                                                                             \
    v16i8 vec0_m, vec1_m, vec2_m, vec3_m, vec4_m, vec5_m, vec6_m, vec7_m;     \
    v8i16 res0_m, res1_m, res2_m, res3_m, res4_m, res5_m, res6_m, res7_m;     \
                                                                              \
    VSHF_B2_SB(src0, src0, src1, src1, mask0, mask0, vec0_m, vec1_m);         \
    VSHF_B2_SB(src2, src2, src3, src3, mask0, mask0, vec2_m, vec3_m);         \
    DOTP_SB4_SH(vec0_m, vec1_m, vec2_m, vec3_m, filt0, filt0, filt0, filt0,   \
                res0_m, res1_m, res2_m, res3_m);                              \
    VSHF_B2_SB(src0, src0, src1, src1, mask2, mask2, vec0_m, vec1_m);         \
    VSHF_B2_SB(src2, src2, src3, src3, mask2, mask2, vec2_m, vec3_m);         \
    DOTP_SB4_SH(vec0_m, vec1_m, vec2_m, vec3_m, filt2, filt2, filt2, filt2,   \
                res4_m, res5_m, res6_m, res7_m);                              \
    VSHF_B2_SB(src0, src0, src1, src1, mask1, mask1, vec4_m, vec5_m);         \
    VSHF_B2_SB(src2, src2, src3, src3, mask1, mask1, vec6_m, vec7_m);         \
    DPADD_SB4_SH(vec4_m, vec5_m, vec6_m, vec7_m, filt1, filt1, filt1, filt1,  \
                 res0_m, res1_m, res2_m, res3_m);                             \
    VSHF_B2_SB(src0, src0, src1, src1, mask3, mask3, vec4_m, vec5_m);         \
    VSHF_B2_SB(src2, src2, src3, src3, mask3, mask3, vec6_m, vec7_m);         \
    DPADD_SB4_SH(vec4_m, vec5_m, vec6_m, vec7_m, filt3, filt3, filt3, filt3,  \
                 res4_m, res5_m, res6_m, res7_m);                             \
    ADDS_SH4_SH(res0_m, res4_m, res1_m, res5_m, res2_m, res6_m, res3_m,       \
                res7_m, out0, out1, out2, out3);                              \
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

static void hevc_hz_8t_4w_msa(UWORD8 *src, WORD32 src_stride,
                              WORD16 *dst, WORD32 dst_stride,
                              WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    uint64_t out;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 mask1, mask2, mask3, vec0, vec1, vec2, vec3;
    v8i16 filt0, filt1, filt2, filt3, dst0, dst1, dst2, dst3;
    v8i16 filter_vec, const_vec;
    v16i8 mask0 = { 0, 1, 1, 2, 2, 3, 3, 4, 16, 17, 17, 18, 18, 19, 19, 20 };

    src -= 3;
    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    filter_vec = LD_SH(filter);
    SPLATI_H4_SH(filter_vec, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    mask1 = ADDVI_B(mask0, 2);
    mask2 = ADDVI_B(mask0, 4);
    mask3 = ADDVI_B(mask0, 6);

    for(loop_cnt = (height >> 3); loop_cnt--;)
    {
        LD_SB8(src, src_stride, src0, src1, src2, src3, src4, src5, src6, src7);
        src += (8 * src_stride);
        XORI_B8_128_SB(src0, src1, src2, src3, src4, src5, src6, src7);

        VSHF_B4_SB(src0, src1, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst0 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst0, dst0, dst0, dst0);
        VSHF_B4_SB(src2, src3, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst1 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst1, dst1, dst1, dst1);
        VSHF_B4_SB(src4, src5, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst2 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst2, dst2, dst2, dst2);
        VSHF_B4_SB(src6, src7, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst3 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst3, dst3, dst3, dst3);

        ST8x8_UB(dst0, dst1, dst2, dst3, dst, 2 * dst_stride);
        dst += (8 * dst_stride);
    }

    if(0 != (height % 8))
    {
        LD_SB7(src, src_stride, src0, src1, src2, src3, src4, src5, src6);
        XORI_B7_128_SB(src0, src1, src2, src3, src4, src5, src6);

        VSHF_B4_SB(src0, src1, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst0 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst0, dst0, dst0, dst0);
        VSHF_B4_SB(src2, src3, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst1 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst1, dst1, dst1, dst1);
        VSHF_B4_SB(src4, src5, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst2 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst2, dst2, dst2, dst2);

        VSHF_B4_SB(src6, src6, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst3 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst3, dst3, dst3, dst3);

        ST8x4_UB(dst0, dst1, dst, 2 * dst_stride);
        ST8x2_UB(dst2, dst + 4 * dst_stride, 2 * dst_stride);
        out = __msa_copy_u_d((v2i64) dst3, 0);
        SD(out, dst + 6 * dst_stride);
    }
}

static void hevc_hz_8t_8w_msa(UWORD8 *src, WORD32 src_stride,
                              WORD16 *dst, WORD32 dst_stride,
                              WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, mask1, mask2, mask3, vec0, vec1, vec2, vec3;
    v8i16 filt0, filt1, filt2, filt3, dst0, dst1, dst2, dst3;
    v8i16 filter_vec, const_vec;
    v16i8 mask0 = { 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8 };

    src -= 3;
    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    filter_vec = LD_SH(filter);
    SPLATI_H4_SH(filter_vec, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    mask1 = ADDVI_B(mask0, 2);
    mask2 = ADDVI_B(mask0, 4);
    mask3 = ADDVI_B(mask0, 6);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src0, src1, src2, src3);
        src += (4 * src_stride);
        XORI_B4_128_SB(src0, src1, src2, src3);

        VSHF_B4_SB(src0, src0, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst0 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst0, dst0, dst0, dst0);
        VSHF_B4_SB(src1, src1, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst1 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst1, dst1, dst1, dst1);
        VSHF_B4_SB(src2, src2, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst2 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst2, dst2, dst2, dst2);
        VSHF_B4_SB(src3, src3, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst3 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst3, dst3, dst3, dst3);

        ST_SH4(dst0, dst1, dst2, dst3, dst, dst_stride);
        dst += (4 * dst_stride);
    }

    if(0 != (height % 4))
    {
        LD_SB3(src, src_stride, src0, src1, src2);
        XORI_B3_128_SB(src0, src1, src2);

        VSHF_B4_SB(src0, src0, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst0 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst0, dst0, dst0, dst0);
        VSHF_B4_SB(src1, src1, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst1 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst1, dst1, dst1, dst1);
        VSHF_B4_SB(src2, src2, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst2 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst2, dst2, dst2, dst2);

        ST_SH2(dst0, dst1, dst, dst_stride);
        ST_SH(dst2, dst + 2 * dst_stride);
    }
}

static void hevc_hz_8t_12w_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride,
                               WORD8 *filter, WORD32 height)
{
    hevc_hz_8t_8w_msa(src, src_stride, dst, dst_stride, filter, height);
    hevc_hz_8t_4w_msa(src + 8, src_stride, dst + 8, dst_stride, filter, height);
}

static void hevc_hz_8t_16w_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride,
                               WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, mask1, mask2, mask3;
    v16i8 vec0, vec1, vec2, vec3;
    v8i16 filt0, filt1, filt2, filt3, filter_vec, const_vec;
    v8i16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v16i8 mask0 = { 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8 };

    src -= 3;
    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    filter_vec = LD_SH(filter);
    SPLATI_H4_SH(filter_vec, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    mask1 = ADDVI_B(mask0, 2);
    mask2 = ADDVI_B(mask0, 4);
    mask3 = ADDVI_B(mask0, 6);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src0, src2, src4, src6);
        LD_SB4(src + 8, src_stride, src1, src3, src5, src7);
        src += (4 * src_stride);
        XORI_B8_128_SB(src0, src1, src2, src3, src4, src5, src6, src7);

        VSHF_B4_SB(src0, src0, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst0 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst0, dst0, dst0, dst0);
        VSHF_B4_SB(src1, src1, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst1 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst1, dst1, dst1, dst1);
        VSHF_B4_SB(src2, src2, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst2 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst2, dst2, dst2, dst2);
        VSHF_B4_SB(src3, src3, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst3 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst3, dst3, dst3, dst3);
        VSHF_B4_SB(src4, src4, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst4 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst4, dst4, dst4, dst4);
        VSHF_B4_SB(src5, src5, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst5 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst5, dst5, dst5, dst5);
        VSHF_B4_SB(src6, src6, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst6 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst6, dst6, dst6, dst6);
        VSHF_B4_SB(src7, src7, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst7 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst7, dst7, dst7, dst7);

        ST_SH4(dst0, dst2, dst4, dst6, dst, dst_stride);
        ST_SH4(dst1, dst3, dst5, dst7, dst + 8, dst_stride);
        dst += (4 * dst_stride);
    }

    if(0 != (height % 4))
    {
        LD_SB3(src, src_stride, src0, src2, src4);
        LD_SB3(src + 8, src_stride, src1, src3, src5);
        XORI_B6_128_SB(src0, src1, src2, src3, src4, src5);

        VSHF_B4_SB(src0, src0, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst0 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst0, dst0, dst0, dst0);
        VSHF_B4_SB(src1, src1, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst1 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst1, dst1, dst1, dst1);
        VSHF_B4_SB(src2, src2, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst2 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst2, dst2, dst2, dst2);
        VSHF_B4_SB(src3, src3, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst3 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst3, dst3, dst3, dst3);
        VSHF_B4_SB(src4, src4, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst4 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst4, dst4, dst4, dst4);
        VSHF_B4_SB(src5, src5, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst5 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst5, dst5, dst5, dst5);

        ST_SH2(dst0, dst2, dst, dst_stride);
        ST_SH(dst4, dst + 2 * dst_stride);
        ST_SH2(dst1, dst3, dst + 8, dst_stride);
        ST_SH(dst5, dst + 2 * dst_stride + 8);
    }
}

static void hevc_hz_8t_24w_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride,
                               WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, vec0, vec1, vec2, vec3;
    v16i8 mask1, mask2, mask3, mask4, mask5, mask6, mask7;
    v8i16 filt0, filt1, filt2, filt3, dst0, dst1, dst2, dst3, dst4, dst5;
    v8i16 filter_vec, const_vec;
    v16i8 mask0 = { 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8 };

    src -= 3;
    filter_vec = LD_SH(filter);
    SPLATI_H4_SH(filter_vec, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    mask1 = ADDVI_B(mask0, 2);
    mask2 = ADDVI_B(mask0, 4);
    mask3 = ADDVI_B(mask0, 6);
    mask4 = ADDVI_B(mask0, 8);
    mask5 = ADDVI_B(mask0, 10);
    mask6 = ADDVI_B(mask0, 12);
    mask7 = ADDVI_B(mask0, 14);

    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    for(loop_cnt = (height >> 1); loop_cnt--;)
    {
        LD_SB2(src, 16, src0, src1);
        src += src_stride;
        LD_SB2(src, 16, src2, src3);
        src += src_stride;
        XORI_B4_128_SB(src0, src1, src2, src3);

        VSHF_B4_SB(src0, src0, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst0 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst0, dst0, dst0, dst0);
        VSHF_B4_SB(src0, src1, mask4, mask5, mask6, mask7,
                   vec0, vec1, vec2, vec3);
        dst1 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst1, dst1, dst1, dst1);
        VSHF_B4_SB(src1, src1, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst2 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst2, dst2, dst2, dst2);
        VSHF_B4_SB(src2, src2, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst3 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst3, dst3, dst3, dst3);
        VSHF_B4_SB(src2, src3, mask4, mask5, mask6, mask7,
                   vec0, vec1, vec2, vec3);
        dst4 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst4, dst4, dst4, dst4);
        VSHF_B4_SB(src3, src3, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst5 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst5, dst5, dst5, dst5);

        ST_SH2(dst0, dst1, dst, 8);
        ST_SH(dst2, dst + 16);
        dst += dst_stride;
        ST_SH2(dst3, dst4, dst, 8);
        ST_SH(dst5, dst + 16);
        dst += dst_stride;
    }

    if(0 != (height % 2))
    {
        LD_SB2(src, 16, src0, src1);
        XORI_B2_128_SB(src0, src1);

        VSHF_B4_SB(src0, src0, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst0 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst0, dst0, dst0, dst0);
        VSHF_B4_SB(src0, src1, mask4, mask5, mask6, mask7,
                   vec0, vec1, vec2, vec3);
        dst1 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst1, dst1, dst1, dst1);
        VSHF_B4_SB(src1, src1, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst2 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst2, dst2, dst2, dst2);

        ST_SH2(dst0, dst1, dst, 8);
        ST_SH(dst2, dst + 16);
    }
}

static void hevc_hz_8t_32w_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride,
                               WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, vec0, vec1, vec2, vec3;
    v16i8 mask1, mask2, mask3, mask4, mask5, mask6, mask7;
    v8i16 filt0, filt1, filt2, filt3, dst0, dst1, dst2, dst3;
    v8i16 filter_vec, const_vec;
    v16i8 mask0 = { 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8 };

    src -= 3;
    filter_vec = LD_SH(filter);
    SPLATI_H4_SH(filter_vec, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    mask1 = ADDVI_B(mask0, 2);
    mask2 = ADDVI_B(mask0, 4);
    mask3 = ADDVI_B(mask0, 6);
    mask4 = ADDVI_B(mask0, 8);
    mask5 = ADDVI_B(mask0, 10);
    mask6 = ADDVI_B(mask0, 12);
    mask7 = ADDVI_B(mask0, 14);

    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    for(loop_cnt = height; loop_cnt--;)
    {
        LD_SB2(src, 16, src0, src1);
        src2 = LD_SB(src + 24);
        src += src_stride;
        XORI_B3_128_SB(src0, src1, src2);

        VSHF_B4_SB(src0, src0, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst0 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst0, dst0, dst0, dst0);
        VSHF_B4_SB(src0, src1, mask4, mask5, mask6, mask7,
                   vec0, vec1, vec2, vec3);
        dst1 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst1, dst1, dst1, dst1);
        VSHF_B4_SB(src1, src1, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst2 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst2, dst2, dst2, dst2);
        VSHF_B4_SB(src2, src2, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst3 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst3, dst3, dst3, dst3);

        ST_SH4(dst0, dst1, dst2, dst3, dst, 8);
        dst += dst_stride;
    }
}

static void hevc_hz_8t_48w_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride,
                               WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, vec0, vec1, vec2, vec3;
    v16i8 mask1, mask2, mask3, mask4, mask5, mask6, mask7;
    v8i16 filt0, filt1, filt2, filt3, dst0, dst1, dst2, dst3, dst4, dst5;
    v8i16 filter_vec, const_vec;
    v16i8 mask0 = { 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8 };

    src -= 3;
    filter_vec = LD_SH(filter);
    SPLATI_H4_SH(filter_vec, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    mask1 = ADDVI_B(mask0, 2);
    mask2 = ADDVI_B(mask0, 4);
    mask3 = ADDVI_B(mask0, 6);
    mask4 = ADDVI_B(mask0, 8);
    mask5 = ADDVI_B(mask0, 10);
    mask6 = ADDVI_B(mask0, 12);
    mask7 = ADDVI_B(mask0, 14);

    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    for(loop_cnt = height; loop_cnt--;)
    {
        LD_SB3(src, 16, src0, src1, src2);
        src3 = LD_SB(src + 40);
        src += src_stride;
        XORI_B4_128_SB(src0, src1, src2, src3);

        VSHF_B4_SB(src0, src0, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst0 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst0, dst0, dst0, dst0);
        VSHF_B4_SB(src0, src1, mask4, mask5, mask6, mask7,
                   vec0, vec1, vec2, vec3);
        dst1 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst1, dst1, dst1, dst1);
        VSHF_B4_SB(src1, src1, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst2 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst2, dst2, dst2, dst2);
        VSHF_B4_SB(src1, src2, mask4, mask5, mask6, mask7,
                   vec0, vec1, vec2, vec3);
        dst3 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst3, dst3, dst3, dst3);
        VSHF_B4_SB(src2, src2, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst4 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst4, dst4, dst4, dst4);
        VSHF_B4_SB(src3, src3, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst5 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst5, dst5, dst5, dst5);

        ST_SH6(dst0, dst1, dst2, dst3, dst4, dst5, dst, 8);
        dst += dst_stride;
    }
}

static void hevc_hz_8t_64w_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride,
                               WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, vec0, vec1, vec2, vec3;
    v16i8 mask1, mask2, mask3, mask4, mask5, mask6, mask7;
    v8i16 filt0, filt1, filt2, filt3, filter_vec, const_vec;
    v8i16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v16i8 mask0 = { 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8 };

    src -= 3;

    filter_vec = LD_SH(filter);
    SPLATI_H4_SH(filter_vec, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    mask1 = ADDVI_B(mask0, 2);
    mask2 = ADDVI_B(mask0, 4);
    mask3 = ADDVI_B(mask0, 6);
    mask4 = ADDVI_B(mask0, 8);
    mask5 = ADDVI_B(mask0, 10);
    mask6 = ADDVI_B(mask0, 12);
    mask7 = ADDVI_B(mask0, 14);

    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    for(loop_cnt = height; loop_cnt--;)
    {
        LD_SB4(src, 16, src0, src1, src2, src3);
        src4 = LD_SB(src + 56);
        src += src_stride;
        XORI_B5_128_SB(src0, src1, src2, src3, src4);

        VSHF_B4_SB(src0, src0, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst0 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst0, dst0, dst0, dst0);
        ST_SH(dst0, dst);

        VSHF_B4_SB(src0, src1, mask4, mask5, mask6, mask7,
                   vec0, vec1, vec2, vec3);
        dst1 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst1, dst1, dst1, dst1);
        ST_SH(dst1, dst + 8);

        VSHF_B4_SB(src1, src1, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst2 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst2, dst2, dst2, dst2);
        ST_SH(dst2, dst + 16);

        VSHF_B4_SB(src1, src2, mask4, mask5, mask6, mask7,
                   vec0, vec1, vec2, vec3);
        dst3 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst3, dst3, dst3, dst3);
        ST_SH(dst3, dst + 24);

        VSHF_B4_SB(src2, src2, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst4 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst4, dst4, dst4, dst4);
        ST_SH(dst4, dst + 32);

        VSHF_B4_SB(src2, src3, mask4, mask5, mask6, mask7,
                   vec0, vec1, vec2, vec3);
        dst5 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst5, dst5, dst5, dst5);
        ST_SH(dst5, dst + 40);

        VSHF_B4_SB(src3, src3, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst6 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst6, dst6, dst6, dst6);
        ST_SH(dst6, dst + 48);

        VSHF_B4_SB(src4, src4, mask0, mask1, mask2, mask3,
                   vec0, vec1, vec2, vec3);
        dst7 = const_vec;
        DPADD_SB4_SH(vec0, vec1, vec2, vec3, filt0, filt1, filt2, filt3,
                     dst7, dst7, dst7, dst7);
        ST_SH(dst7, dst + 56);
        dst += dst_stride;
    }
}

static void hevc_vt_8t_4w_msa(UWORD8 *src, WORD32 src_stride,
                              WORD16 *dst, WORD32 dst_stride,
                              WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v16i8 src11, src12, src13, src14, src10_r, src32_r, src54_r, src76_r;
    v16i8 src98_r, src21_r, src43_r, src65_r, src87_r, src109_r, src1110_r;
    v16i8 src1211_r, src1312_r, src1413_r, src2110, src4332, src6554, src8776;
    v16i8 src10998, src12111110, src14131312;
    v8i16 dst10, dst32, dst54, dst76, filt0, filt1, filt2, filt3;
    v8i16 filter_vec, const_vec;

    src -= (3 * src_stride);

    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    filter_vec = LD_SH(filter);
    SPLATI_H4_SH(filter_vec, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    LD_SB7(src, src_stride, src0, src1, src2, src3, src4, src5, src6);
    src += (7 * src_stride);
    ILVR_B4_SB(src1, src0, src3, src2, src5, src4, src2, src1,
               src10_r, src32_r, src54_r, src21_r);
    ILVR_B2_SB(src4, src3, src6, src5, src43_r, src65_r);
    ILVR_D3_SB(src21_r, src10_r, src43_r, src32_r, src65_r, src54_r,
               src2110, src4332, src6554);
    XORI_B3_128_SB(src2110, src4332, src6554);

    for(loop_cnt = (height >> 3); loop_cnt--;)
    {
        LD_SB8(src, src_stride,
               src7, src8, src9, src10, src11, src12, src13, src14);
        src += (8 * src_stride);

        ILVR_B4_SB(src7, src6, src8, src7, src9, src8, src10, src9,
                   src76_r, src87_r, src98_r, src109_r);
        ILVR_B4_SB(src11, src10, src12, src11, src13, src12, src14, src13,
                   src1110_r, src1211_r, src1312_r, src1413_r);
        ILVR_D4_SB(src87_r, src76_r, src109_r, src98_r, src1211_r, src1110_r,
                   src1413_r, src1312_r, src8776, src10998, src12111110,
                   src14131312);
        XORI_B4_128_SB(src8776, src10998, src12111110, src14131312);

        dst10 = const_vec;
        DPADD_SB4_SH(src2110, src4332, src6554, src8776, filt0, filt1, filt2,
                     filt3, dst10, dst10, dst10, dst10);
        dst32 = const_vec;
        DPADD_SB4_SH(src4332, src6554, src8776, src10998, filt0, filt1, filt2,
                     filt3, dst32, dst32, dst32, dst32);
        dst54 = const_vec;
        DPADD_SB4_SH(src6554, src8776, src10998, src12111110, filt0, filt1,
                     filt2, filt3, dst54, dst54, dst54, dst54);
        dst76 = const_vec;
        DPADD_SB4_SH(src8776, src10998, src12111110, src14131312, filt0, filt1,
                     filt2, filt3, dst76, dst76, dst76, dst76);

        ST8x8_UB(dst10, dst32, dst54, dst76, dst, 2 * dst_stride);
        dst += (8 * dst_stride);

        src2110 = src10998;
        src4332 = src12111110;
        src6554 = src14131312;
        src6 = src14;
    }
}

static void hevc_vt_8t_8w_msa(UWORD8 *src, WORD32 src_stride,
                              WORD16 *dst, WORD32 dst_stride,
                              WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v16i8 src10_r, src32_r, src54_r, src76_r, src98_r;
    v16i8 src21_r, src43_r, src65_r, src87_r, src109_r;
    v8i16 dst0_r, dst1_r, dst2_r, dst3_r, filt0, filt1, filt2, filt3;
    v8i16 filter_vec, const_vec;

    src -= (3 * src_stride);
    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    filter_vec = LD_SH(filter);
    SPLATI_H4_SH(filter_vec, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    LD_SB7(src, src_stride, src0, src1, src2, src3, src4, src5, src6);
    src += (7 * src_stride);
    XORI_B7_128_SB(src0, src1, src2, src3, src4, src5, src6);
    ILVR_B4_SB(src1, src0, src3, src2, src5, src4, src2, src1,
               src10_r, src32_r, src54_r, src21_r);
    ILVR_B2_SB(src4, src3, src6, src5, src43_r, src65_r);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src7, src8, src9, src10);
        src += (4 * src_stride);
        XORI_B4_128_SB(src7, src8, src9, src10);
        ILVR_B4_SB(src7, src6, src8, src7, src9, src8, src10, src9,
                   src76_r, src87_r, src98_r, src109_r);

        dst0_r = const_vec;
        DPADD_SB4_SH(src10_r, src32_r, src54_r, src76_r, filt0, filt1, filt2,
                     filt3, dst0_r, dst0_r, dst0_r, dst0_r);
        dst1_r = const_vec;
        DPADD_SB4_SH(src21_r, src43_r, src65_r, src87_r, filt0, filt1, filt2,
                     filt3, dst1_r, dst1_r, dst1_r, dst1_r);
        dst2_r = const_vec;
        DPADD_SB4_SH(src32_r, src54_r, src76_r, src98_r, filt0, filt1, filt2,
                     filt3, dst2_r, dst2_r, dst2_r, dst2_r);
        dst3_r = const_vec;
        DPADD_SB4_SH(src43_r, src65_r, src87_r, src109_r, filt0, filt1, filt2,
                     filt3, dst3_r, dst3_r, dst3_r, dst3_r);

        ST_SH4(dst0_r, dst1_r, dst2_r, dst3_r, dst, dst_stride);
        dst += (4 * dst_stride);

        src10_r = src54_r;
        src32_r = src76_r;
        src54_r = src98_r;
        src21_r = src65_r;
        src43_r = src87_r;
        src65_r = src109_r;
        src6 = src10;
    }
}

static void hevc_vt_8t_12w_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride,
                               WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v16i8 src10_r, src32_r, src54_r, src76_r, src98_r, src21_r, src43_r;
    v16i8 src65_r, src87_r, src109_r, src10_l, src32_l, src54_l, src76_l;
    v16i8 src98_l, src21_l, src43_l, src65_l, src87_l, src109_l;
    v16i8 src2110, src4332, src6554, src8776, src10998;
    v8i16 dst0_r, dst1_r, dst2_r, dst3_r, dst0_l, dst1_l, filter_vec, const_vec;
    v8i16 filt0, filt1, filt2, filt3;

    src -= (3 * src_stride);
    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    filter_vec = LD_SH(filter);
    SPLATI_H4_SH(filter_vec, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    LD_SB7(src, src_stride, src0, src1, src2, src3, src4, src5, src6);
    src += (7 * src_stride);
    XORI_B7_128_SB(src0, src1, src2, src3, src4, src5, src6);
    ILVR_B4_SB(src1, src0, src3, src2, src5, src4, src2, src1,
               src10_r, src32_r, src54_r, src21_r);
    ILVR_B2_SB(src4, src3, src6, src5, src43_r, src65_r);
    ILVL_B4_SB(src1, src0, src3, src2, src5, src4, src2, src1,
               src10_l, src32_l, src54_l, src21_l);
    ILVL_B2_SB(src4, src3, src6, src5, src43_l, src65_l);
    ILVR_D3_SB(src21_l, src10_l, src43_l, src32_l, src65_l, src54_l,
               src2110, src4332, src6554);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src7, src8, src9, src10);
        src += (4 * src_stride);
        XORI_B4_128_SB(src7, src8, src9, src10);
        ILVR_B4_SB(src7, src6, src8, src7, src9, src8, src10, src9,
                   src76_r, src87_r, src98_r, src109_r);
        ILVL_B4_SB(src7, src6, src8, src7, src9, src8, src10, src9,
                   src76_l, src87_l, src98_l, src109_l);
        ILVR_D2_SB(src87_l, src76_l, src109_l, src98_l, src8776, src10998);

        dst0_r = const_vec;
        DPADD_SB4_SH(src10_r, src32_r, src54_r, src76_r, filt0, filt1, filt2,
                     filt3, dst0_r, dst0_r, dst0_r, dst0_r);
        dst1_r = const_vec;
        DPADD_SB4_SH(src21_r, src43_r, src65_r, src87_r, filt0, filt1, filt2,
                     filt3, dst1_r, dst1_r, dst1_r, dst1_r);
        dst2_r = const_vec;
        DPADD_SB4_SH(src32_r, src54_r, src76_r, src98_r, filt0, filt1, filt2,
                     filt3, dst2_r, dst2_r, dst2_r, dst2_r);
        dst3_r = const_vec;
        DPADD_SB4_SH(src43_r, src65_r, src87_r, src109_r, filt0, filt1, filt2,
                     filt3, dst3_r, dst3_r, dst3_r, dst3_r);
        dst0_l = const_vec;
        DPADD_SB4_SH(src2110, src4332, src6554, src8776, filt0, filt1, filt2,
                     filt3, dst0_l, dst0_l, dst0_l, dst0_l);
        dst1_l = const_vec;
        DPADD_SB4_SH(src4332, src6554, src8776, src10998, filt0, filt1, filt2,
                     filt3, dst1_l, dst1_l, dst1_l, dst1_l);

        ST_SH4(dst0_r, dst1_r, dst2_r, dst3_r, dst, dst_stride);
        ST8x4_UB(dst0_l, dst1_l, dst + 8, 2 * dst_stride);
        dst += (4 * dst_stride);

        src10_r = src54_r;
        src32_r = src76_r;
        src54_r = src98_r;
        src21_r = src65_r;
        src43_r = src87_r;
        src65_r = src109_r;
        src2110 = src6554;
        src4332 = src8776;
        src6554 = src10998;
        src6 = src10;
    }
}

static void hevc_vt_8t_16multx4mult_msa(UWORD8 *src, WORD32 src_stride,
                                        WORD16 *dst, WORD32 dst_stride,
                                        WORD8 *filter, WORD32 height,
                                        WORD32 width)
{
    UWORD8 *src_tmp;
    WORD16 *dst_tmp;
    WORD32 loop_cnt, cnt;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v16i8 src10_r, src32_r, src54_r, src76_r, src98_r,  src21_r, src43_r;
    v16i8 src65_r, src87_r, src109_r, src10_l, src32_l, src54_l, src76_l;
    v16i8 src98_l, src21_l, src43_l, src65_l, src87_l, src109_l;
    v8i16 dst0_r, dst1_r, dst2_r, dst3_r, dst0_l, dst1_l, dst2_l, dst3_l;
    v8i16 filter_vec, const_vec, filt0, filt1, filt2, filt3;

    src -= (3 * src_stride);
    const_vec = __msa_ldi_h(128);
    const_vec = SLLI_H(const_vec, 6);

    filter_vec = LD_SH(filter);
    SPLATI_H4_SH(filter_vec, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    for(cnt = width >> 4; cnt--;)
    {
        src_tmp = src;
        dst_tmp = dst;

        LD_SB7(src_tmp, src_stride, src0, src1, src2, src3, src4, src5, src6);
        src_tmp += (7 * src_stride);
        XORI_B7_128_SB(src0, src1, src2, src3, src4, src5, src6);
        ILVR_B4_SB(src1, src0, src3, src2, src5, src4, src2, src1,
                   src10_r, src32_r, src54_r, src21_r);
        ILVR_B2_SB(src4, src3, src6, src5, src43_r, src65_r);
        ILVL_B4_SB(src1, src0, src3, src2, src5, src4, src2, src1,
                   src10_l, src32_l, src54_l, src21_l);
        ILVL_B2_SB(src4, src3, src6, src5, src43_l, src65_l);

        for(loop_cnt = (height >> 2); loop_cnt--;)
        {
            LD_SB4(src_tmp, src_stride, src7, src8, src9, src10);
            src_tmp += (4 * src_stride);
            XORI_B4_128_SB(src7, src8, src9, src10);
            ILVR_B4_SB(src7, src6, src8, src7, src9, src8, src10, src9,
                       src76_r, src87_r, src98_r, src109_r);
            ILVL_B4_SB(src7, src6, src8, src7, src9, src8, src10, src9,
                       src76_l, src87_l, src98_l, src109_l);

            dst0_r = const_vec;
            DPADD_SB4_SH(src10_r, src32_r, src54_r, src76_r, filt0, filt1,
                         filt2, filt3, dst0_r, dst0_r, dst0_r, dst0_r);
            dst1_r = const_vec;
            DPADD_SB4_SH(src21_r, src43_r, src65_r, src87_r, filt0, filt1,
                         filt2, filt3, dst1_r, dst1_r, dst1_r, dst1_r);
            dst2_r = const_vec;
            DPADD_SB4_SH(src32_r, src54_r, src76_r, src98_r, filt0, filt1,
                         filt2, filt3, dst2_r, dst2_r, dst2_r, dst2_r);
            dst3_r = const_vec;
            DPADD_SB4_SH(src43_r, src65_r, src87_r, src109_r, filt0, filt1,
                         filt2, filt3, dst3_r, dst3_r, dst3_r, dst3_r);
            dst0_l = const_vec;
            DPADD_SB4_SH(src10_l, src32_l, src54_l, src76_l, filt0, filt1,
                         filt2, filt3, dst0_l, dst0_l, dst0_l, dst0_l);
            dst1_l = const_vec;
            DPADD_SB4_SH(src21_l, src43_l, src65_l, src87_l, filt0, filt1,
                         filt2, filt3, dst1_l, dst1_l, dst1_l, dst1_l);
            dst2_l = const_vec;
            DPADD_SB4_SH(src32_l, src54_l, src76_l, src98_l, filt0, filt1,
                         filt2, filt3, dst2_l, dst2_l, dst2_l, dst2_l);
            dst3_l = const_vec;
            DPADD_SB4_SH(src43_l, src65_l, src87_l, src109_l, filt0, filt1,
                         filt2, filt3, dst3_l, dst3_l, dst3_l, dst3_l);

            ST_SH4(dst0_r, dst1_r, dst2_r, dst3_r, dst_tmp, dst_stride);
            ST_SH4(dst0_l, dst1_l, dst2_l, dst3_l, dst_tmp + 8, dst_stride);
            dst_tmp += (4 * dst_stride);

            src10_r = src54_r;
            src32_r = src76_r;
            src54_r = src98_r;
            src21_r = src65_r;
            src43_r = src87_r;
            src65_r = src109_r;
            src10_l = src54_l;
            src32_l = src76_l;
            src54_l = src98_l;
            src21_l = src65_l;
            src43_l = src87_l;
            src65_l = src109_l;
            src6 = src10;
        }

        src += 16;
        dst += 16;
    }
}

static void hevc_vt_8t_16w_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride,
                               WORD8 *filter, WORD32 height)
{
    hevc_vt_8t_16multx4mult_msa(src, src_stride, dst, dst_stride,
                                filter, height, 16);
}

static void hevc_vt_8t_24w_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride,
                               WORD8 *filter, WORD32 height)
{
    hevc_vt_8t_16multx4mult_msa(src, src_stride, dst, dst_stride,
                                filter, height, 16);
    hevc_vt_8t_8w_msa(src + 16, src_stride, dst + 16, dst_stride,
                      filter, height);
}

static void hevc_vt_8t_32w_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride,
                               WORD8 *filter, WORD32 height)
{
    hevc_vt_8t_16multx4mult_msa(src, src_stride, dst, dst_stride,
                                filter, height, 32);
}

static void hevc_vt_8t_48w_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride,
                               WORD8 *filter, WORD32 height)
{
    hevc_vt_8t_16multx4mult_msa(src, src_stride, dst, dst_stride,
                                filter, height, 48);
}

static void hevc_vt_8t_64w_msa(UWORD8 *src, WORD32 src_stride,
                               WORD16 *dst, WORD32 dst_stride,
                               WORD8 *filter, WORD32 height)
{
    hevc_vt_8t_16multx4mult_msa(src, src_stride, dst, dst_stride,
                                filter, height, 64);
}

static void hevc_vt_uni_8t_4x8mult_w16inp_msa(WORD16 *src, WORD32 src_stride,
                                              UWORD8 *dst, WORD32 dst_stride,
                                              WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 vec0;
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v8i16 src11, src12, src13, filter_vec;
    v8i16 src10_r, src32_r, src54_r, src76_r, src98_r, src1110_r, src1312_r;
    v8i16 src21_r, src43_r, src65_r, src87_r, src109_r, src1211_r, src1413_r;
    v4i32 filt0, filt1, filt2, filt3, dst0_r, dst1_r, dst2_r, dst3_r;

    src -= (3 * src_stride);
    filter_vec = LD_SH(filter);
    vec0 = CLTI_S_B(filter_vec, 0);
    filter_vec = (v8i16) __msa_ilvr_b(vec0, (v16i8) filter_vec);
    SPLATI_W4_SW(filter_vec, filt0, filt1, filt2, filt3);

    LD_SH6(src, src_stride, src0, src1, src2, src3, src4, src5);
    src += (6 * src_stride);
    src6 = LD_SH(src);
    src += src_stride;
    ILVR_H4_SH(src1, src0, src3, src2, src5, src4, src2, src1, src10_r, src32_r,
               src54_r, src21_r);
    ILVR_H2_SH(src4, src3, src6, src5, src43_r, src65_r);

    for(loop_cnt = height >> 3; loop_cnt--;)
    {
        LD_SH4(src, src_stride, src7, src8, src9, src10);
        src += 4 * src_stride;
        ILVR_H4_SH(src7, src6, src8, src7, src9, src8, src10, src9, src76_r,
                   src87_r, src98_r, src109_r);

        dst0_r = HEVC_FILT_8TAP(src10_r, src32_r, src54_r, src76_r,
                                filt0, filt1, filt2, filt3);
        dst1_r = HEVC_FILT_8TAP(src21_r, src43_r, src65_r, src87_r,
                                filt0, filt1, filt2, filt3);
        dst2_r = HEVC_FILT_8TAP(src32_r, src54_r, src76_r, src98_r,
                                filt0, filt1, filt2, filt3);
        dst3_r = HEVC_FILT_8TAP(src43_r, src65_r, src87_r, src109_r,
                                filt0, filt1, filt2, filt3);

        SRAI_W4_SW(dst0_r, dst1_r, dst2_r, dst3_r, 6);
        SRARI_W4_SW(dst0_r, dst1_r, dst2_r, dst3_r, 6);
        CLIP_SW4_0_255(dst0_r, dst1_r, dst2_r, dst3_r);
        PCKEV_H2_SW(dst1_r, dst0_r, dst3_r, dst2_r, dst0_r, dst1_r);
        dst0_r = (v4i32) __msa_pckev_b((v16i8) dst1_r, (v16i8) dst0_r);
        ST4x4_UB(dst0_r, dst0_r, 0, 1, 2, 3, dst, dst_stride);
        dst += 4 * dst_stride;

        LD_SH4(src, src_stride, src11, src12, src13, src6);
        src += 4 * src_stride;
        ILVR_H4_SH(src11, src10, src12, src11, src13, src12, src6, src13,
                   src1110_r, src1211_r, src1312_r, src1413_r);

        dst0_r = HEVC_FILT_8TAP(src54_r, src76_r, src98_r, src1110_r,
                                filt0, filt1, filt2, filt3);
        dst1_r = HEVC_FILT_8TAP(src65_r, src87_r, src109_r, src1211_r,
                                filt0, filt1, filt2, filt3);
        dst2_r = HEVC_FILT_8TAP(src76_r, src98_r, src1110_r, src1312_r,
                                filt0, filt1, filt2, filt3);
        dst3_r = HEVC_FILT_8TAP(src87_r, src109_r, src1211_r, src1413_r,
                                filt0, filt1, filt2, filt3);

        SRAI_W4_SW(dst0_r, dst1_r, dst2_r, dst3_r, 6);
        SRARI_W4_SW(dst0_r, dst1_r, dst2_r, dst3_r, 6);
        CLIP_SW4_0_255(dst0_r, dst1_r, dst2_r, dst3_r);
        PCKEV_H2_SW(dst1_r, dst0_r, dst3_r, dst2_r, dst0_r, dst1_r);
        dst0_r = (v4i32) __msa_pckev_b((v16i8) dst1_r, (v16i8) dst0_r);
        ST4x4_UB(dst0_r, dst0_r, 0, 1, 2, 3, dst, dst_stride);
        dst += 4 * dst_stride;

        src10_r = src98_r;
        src32_r = src1110_r;
        src54_r = src1312_r;
        src21_r = src109_r;
        src43_r = src1211_r;
        src65_r = src1413_r;
    }
}

static void hevc_vt_uni_8t_8multx4mult_w16inp_msa(WORD16 *src,
                                                  WORD32 src_stride,
                                                  UWORD8 *dst,
                                                  WORD32 dst_stride,
                                                  WORD8 *filter,
                                                  WORD32 height,
                                                  WORD32 width)
{
    UWORD32 loop_cnt, cnt;
    WORD16 *src_tmp;
    UWORD8 *dst_tmp;
    v16i8 vec0;
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v8i16 src10_r, src32_r, src54_r, src76_r, src98_r, src21_r, src43_r;
    v8i16 src65_r, src87_r, src109_r, src10_l, src32_l, src54_l, src76_l;
    v8i16 src98_l, src21_l, src43_l, src65_l, src87_l, src109_l, filter_vec;
    v4i32 filt0, filt1, filt2, filt3;
    v4i32 dst0_r, dst1_r, dst2_r, dst3_r, dst0_l, dst1_l, dst2_l, dst3_l;

    src -= (3 * src_stride);
    filter_vec = LD_SH(filter);
    vec0 = CLTI_S_B(filter_vec, 0);
    filter_vec = (v8i16) __msa_ilvr_b(vec0, (v16i8) filter_vec);
    SPLATI_W4_SW(filter_vec, filt0, filt1, filt2, filt3);

    for(cnt = width >> 3; cnt--;)
    {
        src_tmp = src;
        dst_tmp = dst;

        LD_SH6(src_tmp, src_stride, src0, src1, src2, src3, src4, src5);
        src_tmp += (6 * src_stride);
        src6 = LD_SH(src_tmp);
        src_tmp += src_stride;
        ILVR_H4_SH(src1, src0, src3, src2, src5, src4, src2, src1,
                src10_r, src32_r, src54_r, src21_r);
        ILVL_H4_SH(src1, src0, src3, src2, src5, src4, src2, src1,
                src10_l, src32_l, src54_l, src21_l);
        ILVR_H2_SH(src4, src3, src6, src5, src43_r, src65_r);
        ILVL_H2_SH(src4, src3, src6, src5, src43_l, src65_l);

        for(loop_cnt = height >> 2; loop_cnt--;)
        {
            LD_SH4(src_tmp, src_stride, src7, src8, src9, src10);
            src_tmp += 4 * src_stride;
            ILVR_H4_SH(src7, src6, src8, src7, src9, src8, src10, src9,
                    src76_r, src87_r, src98_r, src109_r);
            ILVL_H4_SH(src7, src6, src8, src7, src9, src8, src10, src9,
                    src76_l, src87_l, src98_l, src109_l);

            dst0_r = HEVC_FILT_8TAP(src10_r, src32_r, src54_r, src76_r,
                                    filt0, filt1, filt2, filt3);
            dst1_r = HEVC_FILT_8TAP(src21_r, src43_r, src65_r, src87_r,
                                    filt0, filt1, filt2, filt3);
            dst2_r = HEVC_FILT_8TAP(src32_r, src54_r, src76_r, src98_r,
                                    filt0, filt1, filt2, filt3);
            dst3_r = HEVC_FILT_8TAP(src43_r, src65_r, src87_r, src109_r,
                                    filt0, filt1, filt2, filt3);
            dst0_l = HEVC_FILT_8TAP(src10_l, src32_l, src54_l, src76_l,
                                    filt0, filt1, filt2, filt3);
            dst1_l = HEVC_FILT_8TAP(src21_l, src43_l, src65_l, src87_l,
                                    filt0, filt1, filt2, filt3);
            dst2_l = HEVC_FILT_8TAP(src32_l, src54_l, src76_l, src98_l,
                                    filt0, filt1, filt2, filt3);
            dst3_l = HEVC_FILT_8TAP(src43_l, src65_l, src87_l, src109_l,
                                    filt0, filt1, filt2, filt3);

            SRAI_W4_SW(dst0_r, dst1_r, dst2_r, dst3_r, 6);
            SRAI_W4_SW(dst0_l, dst1_l, dst2_l, dst3_l, 6);
            SRARI_W4_SW(dst0_r, dst1_r, dst2_r, dst3_r, 6);
            SRARI_W4_SW(dst0_l, dst1_l, dst2_l, dst3_l, 6);
            CLIP_SW4_0_255(dst0_r, dst1_r, dst2_r, dst3_r);
            CLIP_SW4_0_255(dst0_l, dst1_l, dst2_l, dst3_l);
            PCKEV_H4_SW(dst0_l, dst0_r, dst1_l, dst1_r, dst2_l, dst2_r, dst3_l,
                        dst3_r, dst0_r, dst1_r, dst2_r, dst3_r);
            PCKEV_B2_SW(dst1_r, dst0_r, dst3_r, dst2_r, dst0_r, dst1_r);
            ST8x4_UB(dst0_r, dst1_r, dst_tmp, dst_stride);
            dst_tmp += 4 * dst_stride;

            src6 = src10;
            src10_r = src54_r;
            src32_r = src76_r;
            src54_r = src98_r;
            src21_r = src65_r;
            src43_r = src87_r;
            src65_r = src109_r;
            src10_l = src54_l;
            src32_l = src76_l;
            src54_l = src98_l;
            src21_l = src65_l;
            src43_l = src87_l;
            src65_l = src109_l;
        }

        src += 8;
        dst += 8;
    }
}

static void hevc_vt_uni_8t_12x4mult_w16inp_msa(WORD16 *src, WORD32 src_stride,
                                               UWORD8 *dst, WORD32 dst_stride,
                                               WORD8 *filter, WORD32 height)
{
    hevc_vt_uni_8t_8multx4mult_w16inp_msa(src, src_stride, dst, dst_stride,
                                          filter, height, 8);
    hevc_vt_uni_8t_4x8mult_w16inp_msa(src + 8, src_stride, dst + 8, dst_stride,
                                      filter, height);
}

static void hevc_vt_8t_4x8mult_w16inp_msa(WORD16 *src, WORD32 src_stride,
                                          WORD16 *dst, WORD32 dst_stride,
                                          WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 vec0;
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v8i16 src11, src12, src13, filter_vec;
    v4i32 filt0, filt1, filt2, filt3, const_vec, dst0_r, dst1_r, dst2_r, dst3_r;
    v8i16 src10_r, src32_r, src54_r, src76_r, src98_r, src1110_r, src1312_r;
    v8i16 src21_r, src43_r, src65_r, src87_r, src109_r, src1211_r, src1413_r;

    src -= (3 * src_stride);
    filter_vec = LD_SH(filter);
    vec0 = CLTI_S_B(filter_vec, 0);
    filter_vec = (v8i16) __msa_ilvr_b(vec0, (v16i8) filter_vec);
    SPLATI_W4_SW(filter_vec, filt0, filt1, filt2, filt3);
    const_vec = __msa_ldi_w(1);
    const_vec = SLLI_W(const_vec, 13);

    LD_SH6(src, src_stride, src0, src1, src2, src3, src4, src5);
    src += (6 * src_stride);
    src6 = LD_SH(src);
    src += src_stride;
    ILVR_H4_SH(src1, src0, src3, src2, src5, src4, src2, src1,
               src10_r, src32_r, src54_r, src21_r);
    ILVR_H2_SH(src4, src3, src6, src5, src43_r, src65_r);

    for(loop_cnt = height >> 3; loop_cnt--;)
    {
        LD_SH4(src, src_stride, src7, src8, src9, src10);
        src += 4 * src_stride;
        ILVR_H4_SH(src7, src6, src8, src7, src9, src8, src10, src9,
                   src76_r, src87_r, src98_r, src109_r);

        dst0_r = HEVC_FILT_8TAP(src10_r, src32_r, src54_r, src76_r,
                                filt0, filt1, filt2, filt3);
        dst1_r = HEVC_FILT_8TAP(src21_r, src43_r, src65_r, src87_r,
                                filt0, filt1, filt2, filt3);
        dst2_r = HEVC_FILT_8TAP(src32_r, src54_r, src76_r, src98_r,
                                filt0, filt1, filt2, filt3);
        dst3_r = HEVC_FILT_8TAP(src43_r, src65_r, src87_r, src109_r,
                                filt0, filt1, filt2, filt3);

        SRAI_W4_SW(dst0_r, dst1_r, dst2_r, dst3_r, 6);
        SUB4(dst0_r, const_vec, dst1_r, const_vec,
             dst2_r, const_vec, dst3_r, const_vec,
             dst0_r, dst1_r, dst2_r, dst3_r);
        PCKEV_H2_SW(dst1_r, dst0_r, dst3_r, dst2_r, dst0_r, dst1_r);
        ST8x4_UB(dst0_r, dst1_r, dst, 2 * dst_stride);
        dst += 4 * dst_stride;

        LD_SH4(src, src_stride, src11, src12, src13, src6);
        src += 4 * src_stride;
        ILVR_H4_SH(src11, src10, src12, src11, src13, src12, src6, src13,
                   src1110_r, src1211_r, src1312_r, src1413_r);

        dst0_r = HEVC_FILT_8TAP(src54_r, src76_r, src98_r, src1110_r,
                                filt0, filt1, filt2, filt3);
        dst1_r = HEVC_FILT_8TAP(src65_r, src87_r, src109_r, src1211_r,
                                filt0, filt1, filt2, filt3);
        dst2_r = HEVC_FILT_8TAP(src76_r, src98_r, src1110_r, src1312_r,
                                filt0, filt1, filt2, filt3);
        dst3_r = HEVC_FILT_8TAP(src87_r, src109_r, src1211_r, src1413_r,
                                filt0, filt1, filt2, filt3);

        SRAI_W4_SW(dst0_r, dst1_r, dst2_r, dst3_r, 6);
        SUB4(dst0_r, const_vec, dst1_r, const_vec, dst2_r, const_vec,
             dst3_r, const_vec, dst0_r, dst1_r, dst2_r, dst3_r);
        PCKEV_H2_SW(dst1_r, dst0_r, dst3_r, dst2_r, dst0_r, dst1_r);
        ST8x4_UB(dst0_r, dst1_r, dst, 2 * dst_stride);
        dst += 4 * dst_stride;

        src10_r = src98_r;
        src32_r = src1110_r;
        src54_r = src1312_r;
        src21_r = src109_r;
        src43_r = src1211_r;
        src65_r = src1413_r;
    }
}

static void hevc_vt_8t_8multx4mult_w16inp_msa(WORD16 *src, WORD32 src_stride,
                                              WORD16 *dst, WORD32 dst_stride,
                                              WORD8 *filter, WORD32 height,
                                              WORD32 width)
{
    UWORD32 loop_cnt, cnt;
    WORD16 *src_tmp;
    WORD16 *dst_tmp;
    v16i8 vec0;
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v8i16 src10_r, src32_r, src54_r, src76_r, src98_r, src21_r, src43_r;
    v8i16 src65_r, src87_r, src109_r, src10_l, src32_l, src54_l, src76_l;
    v8i16 src98_l, src21_l, src43_l, src65_l, src87_l, src109_l, filter_vec;
    v4i32 filt0, filt1, filt2, filt3, const_vec, dst0_r, dst1_r;
    v4i32 dst2_r, dst3_r, dst0_l, dst1_l, dst2_l, dst3_l;

    src -= (3 * src_stride);
    filter_vec = LD_SH(filter);
    vec0 = CLTI_S_B(filter_vec, 0);
    filter_vec = (v8i16) __msa_ilvr_b(vec0, (v16i8) filter_vec);
    SPLATI_W4_SW(filter_vec, filt0, filt1, filt2, filt3);
    const_vec = __msa_ldi_w(1);
    const_vec = SLLI_W(const_vec, 13);

    for(cnt = width >> 3; cnt--;)
    {
        src_tmp = src;
        dst_tmp = dst;

        LD_SH6(src_tmp, src_stride, src0, src1, src2, src3, src4, src5);
        src_tmp += (6 * src_stride);
        src6 = LD_SH(src_tmp);
        src_tmp += src_stride;
        ILVR_H4_SH(src1, src0, src3, src2, src5, src4, src2, src1, src10_r,
                   src32_r, src54_r, src21_r);
        ILVL_H4_SH(src1, src0, src3, src2, src5, src4, src2, src1, src10_l,
                   src32_l, src54_l, src21_l);
        ILVR_H2_SH(src4, src3, src6, src5, src43_r, src65_r);
        ILVL_H2_SH(src4, src3, src6, src5, src43_l, src65_l);

        for(loop_cnt = height >> 2; loop_cnt--;)
        {
            LD_SH4(src_tmp, src_stride, src7, src8, src9, src10);
            src_tmp += 4 * src_stride;
            ILVR_H4_SH(src7, src6, src8, src7, src9, src8, src10, src9,
                       src76_r, src87_r, src98_r, src109_r);
            ILVL_H4_SH(src7, src6, src8, src7, src9, src8, src10, src9,
                       src76_l, src87_l, src98_l, src109_l);

            dst0_r = HEVC_FILT_8TAP(src10_r, src32_r, src54_r, src76_r,
                                    filt0, filt1, filt2, filt3);
            dst1_r = HEVC_FILT_8TAP(src21_r, src43_r, src65_r, src87_r,
                                    filt0, filt1, filt2, filt3);
            dst2_r = HEVC_FILT_8TAP(src32_r, src54_r, src76_r, src98_r,
                                    filt0, filt1, filt2, filt3);
            dst3_r = HEVC_FILT_8TAP(src43_r, src65_r, src87_r, src109_r,
                                    filt0, filt1, filt2, filt3);
            dst0_l = HEVC_FILT_8TAP(src10_l, src32_l, src54_l, src76_l,
                                    filt0, filt1, filt2, filt3);
            dst1_l = HEVC_FILT_8TAP(src21_l, src43_l, src65_l, src87_l,
                                    filt0, filt1, filt2, filt3);
            dst2_l = HEVC_FILT_8TAP(src32_l, src54_l, src76_l, src98_l,
                                    filt0, filt1, filt2, filt3);
            dst3_l = HEVC_FILT_8TAP(src43_l, src65_l, src87_l, src109_l,
                                    filt0, filt1, filt2, filt3);

            SRAI_W4_SW(dst0_r, dst1_r, dst2_r, dst3_r, 6);
            SRAI_W4_SW(dst0_l, dst1_l, dst2_l, dst3_l, 6);
            SUB4(dst0_r, const_vec, dst1_r, const_vec, dst2_r, const_vec,
                 dst3_r, const_vec, dst0_r, dst1_r, dst2_r, dst3_r);
            SUB4(dst0_l, const_vec, dst1_l, const_vec, dst2_l, const_vec,
                 dst3_l, const_vec, dst0_l, dst1_l, dst2_l, dst3_l);

            PCKEV_H4_SW(dst0_l, dst0_r, dst1_l, dst1_r, dst2_l, dst2_r,
                        dst3_l, dst3_r, dst0_r, dst1_r, dst2_r, dst3_r);
            ST_SW2(dst0_r, dst1_r, dst_tmp, dst_stride);
            ST_SW2(dst2_r, dst3_r, dst_tmp + 2 * dst_stride, dst_stride);
            dst_tmp += 4 * dst_stride;

            src6 = src10;
            src10_r = src54_r;
            src32_r = src76_r;
            src54_r = src98_r;
            src21_r = src65_r;
            src43_r = src87_r;
            src65_r = src109_r;
            src10_l = src54_l;
            src32_l = src76_l;
            src54_l = src98_l;
            src21_l = src65_l;
            src43_l = src87_l;
            src65_l = src109_l;
        }

        src += 8;
        dst += 8;
    }
}

static void hevc_vt_8t_12x4mult_w16inp_msa(WORD16 *src, WORD32 src_stride,
                                           WORD16 *dst, WORD32 dst_stride,
                                           WORD8 *filter, WORD32 height)
{
    hevc_vt_8t_8multx4mult_w16inp_msa(src, src_stride, dst, dst_stride,
                                      filter, height, 8);
    hevc_vt_8t_4x8mult_w16inp_msa(src + 8, src_stride, dst + 8, dst_stride,
                                  filter, height);
}

static void common_hz_8t_4x4_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter)
{
    v16u8 mask0, mask1, mask2, mask3, out;
    v16i8 src0, src1, src2, src3, filt0, filt1, filt2, filt3;
    v8i16 filt, out0, out1;

    mask0 = LD_UB(&mc_filt_mask_arr[16]);
    src -= 3;

    filt = LD_SH(filter);
    SPLATI_H4_SB(filt, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    mask1 = (v16u8) ADDVI_B(mask0, 2);
    mask2 = (v16u8) ADDVI_B(mask0, 4);
    mask3 = (v16u8) ADDVI_B(mask0, 6);

    LD_SB4(src, src_stride, src0, src1, src2, src3);
    XORI_B4_128_SB(src0, src1, src2, src3);
    HORIZ_8TAP_4WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1, mask2,
                               mask3, filt0, filt1, filt2, filt3, out0, out1);
    SRARI_H2_SH(out0, out1, 6);
    SAT_SH2_SH(out0, out1, 7);
    out = PCKEV_XORI128_UB(out0, out1);
    ST4x4_UB(out, out, 0, 1, 2, 3, dst, dst_stride);
}

static void common_hz_8t_4x8_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter)
{
    v16i8 filt0, filt1, filt2, filt3, src0, src1, src2, src3;
    v16u8 mask0, mask1, mask2, mask3, out;
    v8i16 filt, out0, out1, out2, out3;

    mask0 = LD_UB(&mc_filt_mask_arr[16]);
    src -= 3;

    filt = LD_SH(filter);
    SPLATI_H4_SB(filt, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    mask1 = (v16u8) ADDVI_B(mask0, 2);
    mask2 = (v16u8) ADDVI_B(mask0, 4);
    mask3 = (v16u8) ADDVI_B(mask0, 6);

    LD_SB4(src, src_stride, src0, src1, src2, src3);
    XORI_B4_128_SB(src0, src1, src2, src3);
    src += (4 * src_stride);
    HORIZ_8TAP_4WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1, mask2,
                               mask3, filt0, filt1, filt2, filt3, out0, out1);
    LD_SB4(src, src_stride, src0, src1, src2, src3);
    XORI_B4_128_SB(src0, src1, src2, src3);
    HORIZ_8TAP_4WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1, mask2,
                               mask3, filt0, filt1, filt2, filt3, out2, out3);
    SRARI_H4_SH(out0, out1, out2, out3, 6);
    SAT_SH4_SH(out0, out1, out2, out3, 7);
    out = PCKEV_XORI128_UB(out0, out1);
    ST4x4_UB(out, out, 0, 1, 2, 3, dst, dst_stride);
    dst += (4 * dst_stride);
    out = PCKEV_XORI128_UB(out2, out3);
    ST4x4_UB(out, out, 0, 1, 2, 3, dst, dst_stride);
}

static void common_hz_8t_4x16_msa(UWORD8 *src, WORD32 src_stride,
                                  UWORD8 *dst, WORD32 dst_stride,
                                  WORD8 *filter)
{
    v16u8 mask0, mask1, mask2, mask3, out;
    v16i8 src0, src1, src2, src3, filt0, filt1, filt2, filt3;
    v8i16 filt, out0, out1, out2, out3;

    mask0 = LD_UB(&mc_filt_mask_arr[16]);
    src -= 3;

    filt = LD_SH(filter);
    SPLATI_H4_SB(filt, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    mask1 = (v16u8) ADDVI_B(mask0, 2);
    mask2 = (v16u8) ADDVI_B(mask0, 4);
    mask3 = (v16u8) ADDVI_B(mask0, 6);

    LD_SB4(src, src_stride, src0, src1, src2, src3);
    XORI_B4_128_SB(src0, src1, src2, src3);
    src += (4 * src_stride);
    HORIZ_8TAP_4WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1, mask2,
                               mask3, filt0, filt1, filt2, filt3, out0, out1);
    LD_SB4(src, src_stride, src0, src1, src2, src3);
    XORI_B4_128_SB(src0, src1, src2, src3);
    src += (4 * src_stride);
    HORIZ_8TAP_4WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1, mask2,
                               mask3, filt0, filt1, filt2, filt3, out2, out3);
    SRARI_H4_SH(out0, out1, out2, out3, 6);
    SAT_SH4_SH(out0, out1, out2, out3, 7);
    out = PCKEV_XORI128_UB(out0, out1);
    ST4x4_UB(out, out, 0, 1, 2, 3, dst, dst_stride);
    dst += (4 * dst_stride);
    out = PCKEV_XORI128_UB(out2, out3);
    ST4x4_UB(out, out, 0, 1, 2, 3, dst, dst_stride);
    dst += (4 * dst_stride);

    LD_SB4(src, src_stride, src0, src1, src2, src3);
    XORI_B4_128_SB(src0, src1, src2, src3);
    src += (4 * src_stride);
    HORIZ_8TAP_4WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1, mask2,
                               mask3, filt0, filt1, filt2, filt3, out0, out1);
    LD_SB4(src, src_stride, src0, src1, src2, src3);
    XORI_B4_128_SB(src0, src1, src2, src3);
    src += (4 * src_stride);
    HORIZ_8TAP_4WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1, mask2,
                               mask3, filt0, filt1, filt2, filt3, out2, out3);
    SRARI_H4_SH(out0, out1, out2, out3, 6);
    SAT_SH4_SH(out0, out1, out2, out3, 7);
    out = PCKEV_XORI128_UB(out0, out1);
    ST4x4_UB(out, out, 0, 1, 2, 3, dst, dst_stride);
    dst += (4 * dst_stride);
    out = PCKEV_XORI128_UB(out2, out3);
    ST4x4_UB(out, out, 0, 1, 2, 3, dst, dst_stride);
}

static void common_hz_8t_4w_msa(UWORD8 *src, WORD32 src_stride,
                                UWORD8 *dst, WORD32 dst_stride,
                                WORD8 *filter, WORD32 height)
{
    if(4 == height)
    {
        common_hz_8t_4x4_msa(src, src_stride, dst, dst_stride, filter);
    }
    else if(8 == height)
    {
        common_hz_8t_4x8_msa(src, src_stride, dst, dst_stride, filter);
    }
    else if(16 == height)
    {
        common_hz_8t_4x16_msa(src, src_stride, dst, dst_stride, filter);
    }
}

static void common_hz_8t_8x4_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter)
{
    v16i8 src0, src1, src2, src3, filt0, filt1, filt2, filt3;
    v16u8 mask0, mask1, mask2, mask3, tmp0, tmp1;
    v8i16 filt, out0, out1, out2, out3;

    mask0 = LD_UB(&mc_filt_mask_arr[0]);
    src -= 3;

    filt = LD_SH(filter);
    SPLATI_H4_SB(filt, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    mask1 = (v16u8) ADDVI_B(mask0, 2);
    mask2 = (v16u8) ADDVI_B(mask0, 4);
    mask3 = (v16u8) ADDVI_B(mask0, 6);

    LD_SB4(src, src_stride, src0, src1, src2, src3);
    XORI_B4_128_SB(src0, src1, src2, src3);
    HORIZ_8TAP_8WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1, mask2,
                               mask3, filt0, filt1, filt2, filt3, out0, out1,
                               out2, out3);
    SRARI_H4_SH(out0, out1, out2, out3, 6);
    SAT_SH4_SH(out0, out1, out2, out3, 7);
    tmp0 = PCKEV_XORI128_UB(out0, out1);
    tmp1 = PCKEV_XORI128_UB(out2, out3);
    ST8x4_UB(tmp0, tmp1, dst, dst_stride);
}

static void common_hz_8t_8x8mult_msa(UWORD8 *src, WORD32 src_stride,
                                     UWORD8 *dst, WORD32 dst_stride,
                                     WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, filt0, filt1, filt2, filt3;
    v16u8 mask0, mask1, mask2, mask3, tmp0, tmp1;
    v8i16 filt, out0, out1, out2, out3;

    mask0 = LD_UB(&mc_filt_mask_arr[0]);
    src -= 3;

    filt = LD_SH(filter);
    SPLATI_H4_SB(filt, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    mask1 = (v16u8) ADDVI_B(mask0, 2);
    mask2 = (v16u8) ADDVI_B(mask0, 4);
    mask3 = (v16u8) ADDVI_B(mask0, 6);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src0, src1, src2, src3);
        XORI_B4_128_SB(src0, src1, src2, src3);
        src += (4 * src_stride);
        HORIZ_8TAP_8WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1, mask2,
                                   mask3, filt0, filt1, filt2, filt3, out0,
                                   out1, out2, out3);
        SRARI_H4_SH(out0, out1, out2, out3, 6);
        SAT_SH4_SH(out0, out1, out2, out3, 7);
        tmp0 = PCKEV_XORI128_UB(out0, out1);
        tmp1 = PCKEV_XORI128_UB(out2, out3);
        ST8x4_UB(tmp0, tmp1, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void common_hz_8t_8w_msa(UWORD8 *src, WORD32 src_stride,
                                UWORD8 *dst, WORD32 dst_stride,
                                WORD8 *filter, WORD32 height)
{
    if(4 == height)
    {
        common_hz_8t_8x4_msa(src, src_stride, dst, dst_stride, filter);
    }
    else
    {
        common_hz_8t_8x8mult_msa(src, src_stride, dst, dst_stride, filter,
                                 height);
    }
}

static void common_hz_8t_12w_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter, WORD32 height)
{
    UWORD8 *src1_ptr, *dst1;
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, filt0, filt1, filt2, filt3;
    v8i16 filt, out0, out1, out2, out3;
    v16u8 mask0, mask1, mask2, mask3, mask4, mask5, mask6, mask00, tmp0, tmp1;

    mask00 = LD_UB(&mc_filt_mask_arr[0]);
    mask0 = LD_UB(&mc_filt_mask_arr[16]);

    src1_ptr = src - 3;
    dst1 = dst;

    dst = dst1 + 8;
    src = src1_ptr + 8;

    filt = LD_SH(filter);
    SPLATI_H4_SB(filt, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    mask1 = (v16u8) ADDVI_B(mask00, 2);
    mask2 = (v16u8) ADDVI_B(mask00, 4);
    mask3 = (v16u8) ADDVI_B(mask00, 6);
    mask4 = (v16u8) ADDVI_B(mask0, 2);
    mask5 = (v16u8) ADDVI_B(mask0, 4);
    mask6 = (v16u8) ADDVI_B(mask0, 6);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src1_ptr, src_stride, src0, src1, src2, src3);
        XORI_B4_128_SB(src0, src1, src2, src3);
        src1_ptr += (4 * src_stride);
        HORIZ_8TAP_8WID_4VECS_FILT(src0, src1, src2, src3, mask00, mask1, mask2,
                                   mask3, filt0, filt1, filt2, filt3, out0,
                                   out1, out2, out3);
        SRARI_H4_SH(out0, out1, out2, out3, 6);
        SAT_SH4_SH(out0, out1, out2, out3, 7);
        tmp0 = PCKEV_XORI128_UB(out0, out1);
        tmp1 = PCKEV_XORI128_UB(out2, out3);
        ST8x4_UB(tmp0, tmp1, dst1, dst_stride);
        dst1 += (4 * dst_stride);

        LD_SB4(src, src_stride, src0, src1, src2, src3);
        XORI_B4_128_SB(src0, src1, src2, src3);
        src += (4 * src_stride);
        HORIZ_8TAP_4WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask4, mask5,
                                   mask6, filt0, filt1, filt2, filt3, out0,
                                   out1);
        SRARI_H2_SH(out0, out1, 6);
        SAT_SH2_SH(out0, out1, 7);
        tmp0 = PCKEV_XORI128_UB(out0, out1);
        ST4x4_UB(tmp0, tmp0, 0, 1, 2, 3, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void common_hz_8t_16w_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, filt0, filt1, filt2, filt3;
    v16u8 mask0, mask1, mask2, mask3, out;
    v8i16 filt, out0, out1, out2, out3;

    mask0 = LD_UB(&mc_filt_mask_arr[0]);
    src -= 3;

    filt = LD_SH(filter);
    SPLATI_H4_SB(filt, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    mask1 = (v16u8) ADDVI_B(mask0, 2);
    mask2 = (v16u8) ADDVI_B(mask0, 4);
    mask3 = (v16u8) ADDVI_B(mask0, 6);

    for(loop_cnt = (height >> 1); loop_cnt--;)
    {
        LD_SB2(src, src_stride, src0, src2);
        LD_SB2(src + 8, src_stride, src1, src3);
        XORI_B4_128_SB(src0, src1, src2, src3);
        src += (2 * src_stride);
        HORIZ_8TAP_8WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1, mask2,
                                   mask3, filt0, filt1, filt2, filt3, out0,
                                   out1, out2, out3);
        SRARI_H4_SH(out0, out1, out2, out3, 6);
        SAT_SH4_SH(out0, out1, out2, out3, 7);
        out = PCKEV_XORI128_UB(out0, out1);
        ST_UB(out, dst);
        dst += dst_stride;
        out = PCKEV_XORI128_UB(out2, out3);
        ST_UB(out, dst);
        dst += dst_stride;
    }
}

static void common_hz_8t_24w_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, filt0, filt1, filt2, filt3;
    v16u8 mask0, mask1, mask2, mask3, mask4, mask5, mask6, mask7, out;
    v16i8 vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7, vec8, vec9, vec10;
    v16i8 vec11;
    v8i16 out0, out1, out2, out3, out4, out5, out6, out7, out8, out9, out10;
    v8i16 out11, filt;

    mask0 = LD_UB(&mc_filt_mask_arr[0]);
    src -= 3;

    filt = LD_SH(filter);
    SPLATI_H4_SB(filt, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    mask1 = (v16u8) ADDVI_B(mask0, 2);
    mask2 = (v16u8) ADDVI_B(mask0, 4);
    mask3 = (v16u8) ADDVI_B(mask0, 6);
    mask4 = (v16u8) ADDVI_B(mask0, 8);
    mask5 = (v16u8) ADDVI_B(mask0, 10);
    mask6 = (v16u8) ADDVI_B(mask0, 12);
    mask7 = (v16u8) ADDVI_B(mask0, 14);

    for(loop_cnt = (height >> 1); loop_cnt--;)
    {
        LD_SB2(src, src_stride, src0, src2);
        LD_SB2(src + 16, src_stride, src1, src3);
        XORI_B4_128_SB(src0, src1, src2, src3);
        src += (2 * src_stride);
        VSHF_B2_SB(src0, src0, src1, src1, mask0, mask0, vec0, vec8);
        VSHF_B2_SB(src2, src2, src3, src3, mask0, mask0, vec2, vec9);
        VSHF_B2_SB(src0, src1, src2, src3, mask4, mask4, vec1, vec3);
        DOTP_SB4_SH(vec0, vec8, vec2, vec9, filt0, filt0, filt0, filt0, out0,
                    out8, out2, out9);
        DOTP_SB2_SH(vec1, vec3, filt0, filt0, out1, out3);
        VSHF_B2_SB(src0, src0, src1, src1, mask2, mask2, vec0, vec8);
        VSHF_B2_SB(src2, src2, src3, src3, mask2, mask2, vec2, vec9);
        VSHF_B2_SB(src0, src1, src2, src3, mask6, mask6, vec1, vec3);
        DOTP_SB4_SH(vec0, vec8, vec2, vec9, filt2, filt2, filt2, filt2, out4,
                    out10, out6, out11);
        DOTP_SB2_SH(vec1, vec3, filt2, filt2, out5, out7);
        VSHF_B2_SB(src0, src0, src1, src1, mask1, mask1, vec4, vec10);
        VSHF_B2_SB(src2, src2, src3, src3, mask1, mask1, vec6, vec11);
        VSHF_B2_SB(src0, src1, src2, src3, mask5, mask5, vec5, vec7);
        DPADD_SB4_SH(vec4, vec10, vec6, vec11, filt1, filt1, filt1, filt1,
                     out0, out8, out2, out9);
        DPADD_SB2_SH(vec5, vec7, filt1, filt1, out1, out3);
        VSHF_B2_SB(src0, src0, src1, src1, mask3, mask3, vec4, vec10);
        VSHF_B2_SB(src2, src2, src3, src3, mask3, mask3, vec6, vec11);
        VSHF_B2_SB(src0, src1, src2, src3, mask7, mask7, vec5, vec7);
        DPADD_SB4_SH(vec4, vec10, vec6, vec11, filt3, filt3, filt3, filt3,
                     out4, out10, out6, out11);
        DPADD_SB2_SH(vec5, vec7, filt3, filt3, out5, out7);
        ADDS_SH4_SH(out0, out4, out8, out10, out2, out6, out9, out11, out0,
                    out8, out2, out9);
        ADDS_SH2_SH(out1, out5, out3, out7, out1, out3);
        SRARI_H4_SH(out0, out8, out2, out9, 6);
        SRARI_H2_SH(out1, out3, 6);
        SAT_SH4_SH(out0, out8, out2, out9, 7);
        SAT_SH2_SH(out1, out3, 7);
        out = PCKEV_XORI128_UB(out8, out9);
        ST8x2_UB(out, dst + 16, dst_stride)
        out = PCKEV_XORI128_UB(out0, out1);
        ST_UB(out, dst);
        dst += dst_stride;
        out = PCKEV_XORI128_UB(out2, out3);
        ST_UB(out, dst);
        dst += dst_stride;
    }
}

static void common_hz_8t_32w_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, filt0, filt1, filt2, filt3;
    v16u8 mask0, mask1, mask2, mask3, out;
    v8i16 filt, out0, out1, out2, out3;

    mask0 = LD_UB(&mc_filt_mask_arr[0]);
    src -= 3;

    filt = LD_SH(filter);
    SPLATI_H4_SB(filt, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    mask1 = (v16u8) ADDVI_B(mask0, 2);
    mask2 = (v16u8) ADDVI_B(mask0, 4);
    mask3 = (v16u8) ADDVI_B(mask0, 6);

    for(loop_cnt = (height >> 1); loop_cnt--;)
    {
        src0 = LD_SB(src);
        src2 = LD_SB(src + 16);
        src3 = LD_SB(src + 24);
        src1 = __msa_sldi_b(src2, src0, 8);
        src += src_stride;
        XORI_B4_128_SB(src0, src1, src2, src3);
        HORIZ_8TAP_8WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1, mask2,
                                   mask3, filt0, filt1, filt2, filt3, out0,
                                   out1, out2, out3);
        SRARI_H4_SH(out0, out1, out2, out3, 6);
        SAT_SH4_SH(out0, out1, out2, out3, 7);

        src0 = LD_SB(src);
        src2 = LD_SB(src + 16);
        src3 = LD_SB(src + 24);
        src1 = __msa_sldi_b(src2, src0, 8);
        src += src_stride;

        out = PCKEV_XORI128_UB(out0, out1);
        ST_UB(out, dst);
        out = PCKEV_XORI128_UB(out2, out3);
        ST_UB(out, dst + 16);
        dst += dst_stride;

        XORI_B4_128_SB(src0, src1, src2, src3);
        HORIZ_8TAP_8WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1, mask2,
                                   mask3, filt0, filt1, filt2, filt3, out0,
                                   out1, out2, out3);
        SRARI_H4_SH(out0, out1, out2, out3, 6);
        SAT_SH4_SH(out0, out1, out2, out3, 7);
        out = PCKEV_XORI128_UB(out0, out1);
        ST_UB(out, dst);
        out = PCKEV_XORI128_UB(out2, out3);
        ST_UB(out, dst + 16);
        dst += dst_stride;
    }
}

static void common_hz_8t_48w_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, filt0, filt1, filt2, filt3, vec0, vec1, vec2;
    v16u8 mask0, mask1, mask2, mask3, mask4, mask5, mask6, mask7, out;
    v8i16 filt, out0, out1, out2, out3, out4, out5, out6;

    mask0 = LD_UB(&mc_filt_mask_arr[0]);
    src -= 3;

    filt = LD_SH(filter);
    SPLATI_H4_SB(filt, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    mask1 = (v16u8) ADDVI_B(mask0, 2);
    mask2 = (v16u8) ADDVI_B(mask0, 4);
    mask3 = (v16u8) ADDVI_B(mask0, 6);
    mask4 = (v16u8) ADDVI_B(mask0, 8);
    mask5 = (v16u8) ADDVI_B(mask0, 10);
    mask6 = (v16u8) ADDVI_B(mask0, 12);
    mask7 = (v16u8) ADDVI_B(mask0, 14);

    for(loop_cnt = height; loop_cnt--;)
    {
        LD_SB3(src, 16, src0, src2, src3);
        src1 = __msa_sldi_b(src2, src0, 8);

        XORI_B4_128_SB(src0, src1, src2, src3);
        VSHF_B3_SB(src0, src0, src1, src1, src2, src2, mask0, mask0, mask0,
                   vec0, vec1, vec2);
        DOTP_SB3_SH(vec0, vec1, vec2, filt0, filt0, filt0, out0, out1, out2);
        VSHF_B3_SB(src0, src0, src1, src1, src2, src2, mask1, mask1, mask1,
                   vec0, vec1, vec2);
        DPADD_SB2_SH(vec0, vec1, filt1, filt1, out0, out1);
        out2 = __msa_dpadd_s_h(out2, vec2, filt1);
        VSHF_B3_SB(src0, src0, src1, src1, src2, src2, mask2, mask2, mask2,
                   vec0, vec1, vec2);
        DOTP_SB3_SH(vec0, vec1, vec2, filt2, filt2, filt2, out3, out4, out5);
        VSHF_B3_SB(src0, src0, src1, src1, src2, src2, mask3, mask3, mask3,
                   vec0, vec1, vec2);
        DPADD_SB2_SH(vec0, vec1, filt3, filt3, out3, out4);
        out5 = __msa_dpadd_s_h(out5, vec2, filt3);
        ADDS_SH2_SH(out0, out3, out1, out4, out0, out1);
        out2 = __msa_adds_s_h(out2, out5);
        SRARI_H2_SH(out0, out1, 6);
        out6 = __msa_srari_h(out2, 6);
        SAT_SH3_SH(out0, out1, out6, 7);
        out = PCKEV_XORI128_UB(out0, out1);
        ST_UB(out, dst);

        src1 = LD_SB(src + 40);
        src += src_stride;
        src1 = (v16i8) XORI_B(src1, 128);

        VSHF_B3_SB(src2, src3, src3, src3, src1, src1, mask4, mask0, mask0,
                   vec0, vec1, vec2);
        DOTP_SB3_SH(vec0, vec1, vec2, filt0, filt0, filt0, out0, out1, out2);
        VSHF_B3_SB(src2, src3, src3, src3, src1, src1, mask5, mask1, mask1,
                   vec0, vec1, vec2);
        DPADD_SB2_SH(vec0, vec1, filt1, filt1, out0, out1);
        out2 = __msa_dpadd_s_h(out2, vec2, filt1);
        VSHF_B3_SB(src2, src3, src3, src3, src1, src1, mask6, mask2, mask2,
                   vec0, vec1, vec2);
        DOTP_SB3_SH(vec0, vec1, vec2, filt2, filt2, filt2, out3, out4, out5);
        VSHF_B3_SB(src2, src3, src3, src3, src1, src1, mask7, mask3, mask3,
                   vec0, vec1, vec2);
        DPADD_SB2_SH(vec0, vec1, filt3, filt3, out3, out4);
        out5 = __msa_dpadd_s_h(out5, vec2, filt3);
        ADDS_SH2_SH(out0, out3, out1, out4, out3, out4);
        out5 = __msa_adds_s_h(out2, out5);
        SRARI_H3_SH(out3, out4, out5, 6);
        SAT_SH3_SH(out3, out4, out5, 7);
        out = PCKEV_XORI128_UB(out6, out3);
        ST_UB(out, dst + 16);
        out = PCKEV_XORI128_UB(out4, out5);
        ST_UB(out, dst + 32);
        dst += dst_stride;
    }
}

static void common_hz_8t_64w_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, filt0, filt1, filt2, filt3;
    v16u8 mask0, mask1, mask2, mask3, out;
    v8i16 filt, out0, out1, out2, out3;

    mask0 = LD_UB(&mc_filt_mask_arr[0]);
    src -= 3;

    filt = LD_SH(filter);
    SPLATI_H4_SB(filt, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    mask1 = (v16u8) ADDVI_B(mask0, 2);
    mask2 = (v16u8) ADDVI_B(mask0, 4);
    mask3 = (v16u8) ADDVI_B(mask0, 6);

    for(loop_cnt = height; loop_cnt--;)
    {
        src0 = LD_SB(src);
        src2 = LD_SB(src + 16);
        src3 = LD_SB(src + 24);
        src1 = __msa_sldi_b(src2, src0, 8);

        XORI_B4_128_SB(src0, src1, src2, src3);
        HORIZ_8TAP_8WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1, mask2,
                                   mask3, filt0, filt1, filt2, filt3, out0,
                                   out1, out2, out3);
        SRARI_H4_SH(out0, out1, out2, out3, 6);
        SAT_SH4_SH(out0, out1, out2, out3, 7);
        out = PCKEV_XORI128_UB(out0, out1);
        ST_UB(out, dst);
        out = PCKEV_XORI128_UB(out2, out3);
        ST_UB(out, dst + 16);

        src0 = LD_SB(src + 32);
        src2 = LD_SB(src + 48);
        src3 = LD_SB(src + 56);
        src1 = __msa_sldi_b(src2, src0, 8);
        src += src_stride;

        XORI_B4_128_SB(src0, src1, src2, src3);
        HORIZ_8TAP_8WID_4VECS_FILT(src0, src1, src2, src3, mask0, mask1, mask2,
                                   mask3, filt0, filt1, filt2, filt3, out0,
                                   out1, out2, out3);
        SRARI_H4_SH(out0, out1, out2, out3, 6);
        SAT_SH4_SH(out0, out1, out2, out3, 7);
        out = PCKEV_XORI128_UB(out0, out1);
        ST_UB(out, dst + 32);
        out = PCKEV_XORI128_UB(out2, out3);
        ST_UB(out, dst + 48);
        dst += dst_stride;
    }
}

static void common_vt_8t_4w_msa(UWORD8 *src, WORD32 src_stride,
                                UWORD8 *dst, WORD32 dst_stride,
                                WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v16i8 src10_r, src32_r, src54_r, src76_r, src98_r, src21_r, src43_r;
    v16i8 src65_r, src87_r, src109_r, src2110, src4332, src6554, src8776;
    v16i8 src10998, filt0, filt1, filt2, filt3;
    v16u8 out;
    v8i16 filt, out10, out32;

    src -= (3 * src_stride);

    filt = LD_SH(filter);
    SPLATI_H4_SB(filt, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    LD_SB7(src, src_stride, src0, src1, src2, src3, src4, src5, src6);
    src += (7 * src_stride);

    ILVR_B4_SB(src1, src0, src3, src2, src5, src4, src2, src1, src10_r, src32_r,
               src54_r, src21_r);
    ILVR_B2_SB(src4, src3, src6, src5, src43_r, src65_r);
    ILVR_D3_SB(src21_r, src10_r, src43_r, src32_r, src65_r, src54_r, src2110,
               src4332, src6554);
    XORI_B3_128_SB(src2110, src4332, src6554);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src7, src8, src9, src10);
        src += (4 * src_stride);

        ILVR_B4_SB(src7, src6, src8, src7, src9, src8, src10, src9, src76_r,
                   src87_r, src98_r, src109_r);
        ILVR_D2_SB(src87_r, src76_r, src109_r, src98_r, src8776, src10998);
        XORI_B2_128_SB(src8776, src10998);
        out10 = FILT_8TAP_DPADD_S_H(src2110, src4332, src6554, src8776, filt0,
                                    filt1, filt2, filt3);
        out32 = FILT_8TAP_DPADD_S_H(src4332, src6554, src8776, src10998, filt0,
                                    filt1, filt2, filt3);
        SRARI_H2_SH(out10, out32, 6);
        SAT_SH2_SH(out10, out32, 7);
        out = PCKEV_XORI128_UB(out10, out32);
        ST4x4_UB(out, out, 0, 1, 2, 3, dst, dst_stride);
        dst += (4 * dst_stride);

        src2110 = src6554;
        src4332 = src8776;
        src6554 = src10998;
        src6 = src10;
    }
}

static void common_vt_8t_8w_msa(UWORD8 *src, WORD32 src_stride,
                                UWORD8 *dst, WORD32 dst_stride,
                                WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v16i8 src10_r, src32_r, src54_r, src76_r, src98_r, src21_r, src43_r;
    v16i8 src65_r, src87_r, src109_r, filt0, filt1, filt2, filt3;
    v16u8 tmp0, tmp1;
    v8i16 filt, out0_r, out1_r, out2_r, out3_r;

    src -= (3 * src_stride);

    filt = LD_SH(filter);
    SPLATI_H4_SB(filt, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    LD_SB7(src, src_stride, src0, src1, src2, src3, src4, src5, src6);
    XORI_B7_128_SB(src0, src1, src2, src3, src4, src5, src6);
    src += (7 * src_stride);
    ILVR_B4_SB(src1, src0, src3, src2, src5, src4, src2, src1, src10_r, src32_r,
               src54_r, src21_r);
    ILVR_B2_SB(src4, src3, src6, src5, src43_r, src65_r);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src7, src8, src9, src10);
        XORI_B4_128_SB(src7, src8, src9, src10);
        src += (4 * src_stride);

        ILVR_B4_SB(src7, src6, src8, src7, src9, src8, src10, src9, src76_r,
                   src87_r, src98_r, src109_r);
        out0_r = FILT_8TAP_DPADD_S_H(src10_r, src32_r, src54_r, src76_r, filt0,
                                     filt1, filt2, filt3);
        out1_r = FILT_8TAP_DPADD_S_H(src21_r, src43_r, src65_r, src87_r, filt0,
                                     filt1, filt2, filt3);
        out2_r = FILT_8TAP_DPADD_S_H(src32_r, src54_r, src76_r, src98_r, filt0,
                                     filt1, filt2, filt3);
        out3_r = FILT_8TAP_DPADD_S_H(src43_r, src65_r, src87_r, src109_r, filt0,
                                     filt1, filt2, filt3);
        SRARI_H4_SH(out0_r, out1_r, out2_r, out3_r, 6);
        SAT_SH4_SH(out0_r, out1_r, out2_r, out3_r, 7);
        tmp0 = PCKEV_XORI128_UB(out0_r, out1_r);
        tmp1 = PCKEV_XORI128_UB(out2_r, out3_r);
        ST8x4_UB(tmp0, tmp1, dst, dst_stride);
        dst += (4 * dst_stride);

        src10_r = src54_r;
        src32_r = src76_r;
        src54_r = src98_r;
        src21_r = src65_r;
        src43_r = src87_r;
        src65_r = src109_r;
        src6 = src10;
    }
}

static void common_vt_8t_12w_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter, WORD32 height)
{
    WORD32 loop_cnt;
    UWORD32 out2, out3;
    uint64_t out0, out1;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8, res0, res1;
    v16i8 res2, vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7;
    v8i16 vec01, vec23, vec45, vec67, tmp0, tmp1, tmp2;
    v8i16 filt, filt0, filt1, filt2, filt3;
    v4i32 mask = { 2, 6, 2, 6 };

    src -= (3 * src_stride);

    filt = LD_SH(filter);
    SPLATI_H4_SH(filt, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    LD_SB7(src, src_stride, src0, src1, src2, src3, src4, src5, src6);
    src += (7 * src_stride);

    XORI_B7_128_SB(src0, src1, src2, src3, src4, src5, src6);

    VSHF_W2_SB(src0, src1, src1, src2, mask, mask, vec0, vec1);
    VSHF_W2_SB(src2, src3, src3, src4, mask, mask, vec2, vec3);
    VSHF_W2_SB(src4, src5, src5, src6, mask, mask, vec4, vec5);

    for(loop_cnt = (height >> 1); loop_cnt--;)
    {
        LD_SB2(src, src_stride, src7, src8);
        XORI_B2_128_SB(src7, src8);
        src += (2 * src_stride);

        ILVR_B4_SH(src1, src0, src3, src2, src5, src4, src7, src6,
                   vec01, vec23, vec45, vec67);
        tmp0 = FILT_8TAP_DPADD_S_H(vec01, vec23, vec45, vec67, filt0, filt1,
                                   filt2, filt3);
        ILVR_B4_SH(src2, src1, src4, src3, src6, src5, src8, src7, vec01, vec23,
                   vec45, vec67);
        tmp1 = FILT_8TAP_DPADD_S_H(vec01, vec23, vec45, vec67, filt0, filt1,
                                   filt2, filt3);

        VSHF_W2_SB(src6, src7, src7, src8, mask, mask, vec6, vec7);
        ILVR_B4_SH(vec1, vec0, vec3, vec2, vec5, vec4, vec7, vec6, vec01, vec23,
                   vec45, vec67);
        tmp2 = FILT_8TAP_DPADD_S_H(vec01, vec23, vec45, vec67, filt0, filt1,
                                   filt2, filt3);
        SRARI_H3_SH(tmp0, tmp1, tmp2, 6);
        SAT_SH3_SH(tmp0, tmp1, tmp2, 7);
        PCKEV_B3_SB(tmp0, tmp0, tmp1, tmp1, tmp2, tmp2, res0, res1, res2);
        XORI_B3_128_SB(res0, res1, res2);

        out0 = __msa_copy_u_d((v2i64) res0, 0);
        out1 = __msa_copy_u_d((v2i64) res1, 0);
        out2 = __msa_copy_u_w((v4i32) res2, 0);
        out3 = __msa_copy_u_w((v4i32) res2, 1);
        SD(out0, dst);
        SW(out2, (dst + 8));
        dst += dst_stride;
        SD(out1, dst);
        SW(out3, (dst + 8));
        dst += dst_stride;

        src0 = src2;
        src1 = src3;
        src2 = src4;
        src3 = src5;
        src4 = src6;
        src5 = src7;
        src6 = src8;
        vec0 = vec2;
        vec1 = vec3;
        vec2 = vec4;
        vec3 = vec5;
        vec4 = vec6;
        vec5 = vec7;
    }
}

static void common_vt_8t_16w_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter, WORD32 height)
{
    UWORD32 loop_cnt;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v16i8 filt0, filt1, filt2, filt3;
    v16i8 src10_r, src32_r, src54_r, src76_r, src98_r, src21_r, src43_r;
    v16i8 src65_r, src87_r, src109_r, src10_l, src32_l, src54_l, src76_l;
    v16i8 src98_l, src21_l, src43_l, src65_l, src87_l, src109_l;
    v16u8 tmp0, tmp1, tmp2, tmp3;
    v8i16 filt, out0_r, out1_r, out2_r, out3_r, out0_l, out1_l, out2_l, out3_l;

    src -= (3 * src_stride);

    filt = LD_SH(filter);
    SPLATI_H4_SB(filt, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    LD_SB7(src, src_stride, src0, src1, src2, src3, src4, src5, src6);
    XORI_B7_128_SB(src0, src1, src2, src3, src4, src5, src6);
    src += (7 * src_stride);
    ILVR_B4_SB(src1, src0, src3, src2, src5, src4, src2, src1, src10_r, src32_r,
               src54_r, src21_r);
    ILVR_B2_SB(src4, src3, src6, src5, src43_r, src65_r);
    ILVL_B4_SB(src1, src0, src3, src2, src5, src4, src2, src1, src10_l, src32_l,
               src54_l, src21_l);
    ILVL_B2_SB(src4, src3, src6, src5, src43_l, src65_l);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SB4(src, src_stride, src7, src8, src9, src10);
        XORI_B4_128_SB(src7, src8, src9, src10);
        src += (4 * src_stride);

        ILVR_B4_SB(src7, src6, src8, src7, src9, src8, src10, src9, src76_r,
                   src87_r, src98_r, src109_r);
        ILVL_B4_SB(src7, src6, src8, src7, src9, src8, src10, src9, src76_l,
                   src87_l, src98_l, src109_l);
        out0_r = FILT_8TAP_DPADD_S_H(src10_r, src32_r, src54_r, src76_r, filt0,
                                     filt1, filt2, filt3);
        out1_r = FILT_8TAP_DPADD_S_H(src21_r, src43_r, src65_r, src87_r, filt0,
                                     filt1, filt2, filt3);
        out2_r = FILT_8TAP_DPADD_S_H(src32_r, src54_r, src76_r, src98_r, filt0,
                                     filt1, filt2, filt3);
        out3_r = FILT_8TAP_DPADD_S_H(src43_r, src65_r, src87_r, src109_r, filt0,
                                     filt1, filt2, filt3);
        out0_l = FILT_8TAP_DPADD_S_H(src10_l, src32_l, src54_l, src76_l, filt0,
                                     filt1, filt2, filt3);
        out1_l = FILT_8TAP_DPADD_S_H(src21_l, src43_l, src65_l, src87_l, filt0,
                                     filt1, filt2, filt3);
        out2_l = FILT_8TAP_DPADD_S_H(src32_l, src54_l, src76_l, src98_l, filt0,
                                     filt1, filt2, filt3);
        out3_l = FILT_8TAP_DPADD_S_H(src43_l, src65_l, src87_l, src109_l, filt0,
                                     filt1, filt2, filt3);
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
        src32_r = src76_r;
        src54_r = src98_r;
        src21_r = src65_r;
        src43_r = src87_r;
        src65_r = src109_r;
        src10_l = src54_l;
        src32_l = src76_l;
        src54_l = src98_l;
        src21_l = src65_l;
        src43_l = src87_l;
        src65_l = src109_l;
        src6 = src10;
    }
}

static void common_vt_8t_16w_mult_msa(UWORD8 *src, WORD32 src_stride,
                                      UWORD8 *dst, WORD32 dst_stride,
                                      WORD8 *filter, WORD32 height,
                                      WORD32 width)
{
    UWORD8 *src_tmp;
    UWORD8 *dst_tmp;
    UWORD32 loop_cnt, cnt;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v16i8 filt0, filt1, filt2, filt3;
    v16i8 src10_r, src32_r, src54_r, src76_r, src98_r, src21_r, src43_r;
    v16i8 src65_r, src87_r, src109_r, src10_l, src32_l, src54_l, src76_l;
    v16i8 src98_l, src21_l, src43_l, src65_l, src87_l, src109_l;
    v16u8 tmp0, tmp1, tmp2, tmp3;
    v8i16 filt, out0_r, out1_r, out2_r, out3_r, out0_l, out1_l, out2_l, out3_l;

    src -= (3 * src_stride);

    filt = LD_SH(filter);
    SPLATI_H4_SB(filt, 0, 1, 2, 3, filt0, filt1, filt2, filt3);

    for(cnt = (width >> 4); cnt--;)
    {
        src_tmp = src;
        dst_tmp = dst;

        LD_SB7(src_tmp, src_stride, src0, src1, src2, src3, src4, src5, src6);
        XORI_B7_128_SB(src0, src1, src2, src3, src4, src5, src6);
        src_tmp += (7 * src_stride);
        ILVR_B4_SB(src1, src0, src3, src2, src5, src4, src2, src1, src10_r,
                   src32_r, src54_r, src21_r);
        ILVR_B2_SB(src4, src3, src6, src5, src43_r, src65_r);
        ILVL_B4_SB(src1, src0, src3, src2, src5, src4, src2, src1, src10_l,
                   src32_l, src54_l, src21_l);
        ILVL_B2_SB(src4, src3, src6, src5, src43_l, src65_l);

        for(loop_cnt = (height >> 2); loop_cnt--;)
        {
            LD_SB4(src_tmp, src_stride, src7, src8, src9, src10);
            XORI_B4_128_SB(src7, src8, src9, src10);
            src_tmp += (4 * src_stride);
            ILVR_B4_SB(src7, src6, src8, src7, src9, src8, src10, src9, src76_r,
                       src87_r, src98_r, src109_r);
            ILVL_B4_SB(src7, src6, src8, src7, src9, src8, src10, src9, src76_l,
                       src87_l, src98_l, src109_l);
            out0_r = FILT_8TAP_DPADD_S_H(src10_r, src32_r, src54_r, src76_r,
                                         filt0, filt1, filt2, filt3);
            out1_r = FILT_8TAP_DPADD_S_H(src21_r, src43_r, src65_r, src87_r,
                                         filt0, filt1, filt2, filt3);
            out2_r = FILT_8TAP_DPADD_S_H(src32_r, src54_r, src76_r, src98_r,
                                         filt0, filt1, filt2, filt3);
            out3_r = FILT_8TAP_DPADD_S_H(src43_r, src65_r, src87_r, src109_r,
                                         filt0, filt1, filt2, filt3);
            out0_l = FILT_8TAP_DPADD_S_H(src10_l, src32_l, src54_l, src76_l,
                                         filt0, filt1, filt2, filt3);
            out1_l = FILT_8TAP_DPADD_S_H(src21_l, src43_l, src65_l, src87_l,
                                         filt0, filt1, filt2, filt3);
            out2_l = FILT_8TAP_DPADD_S_H(src32_l, src54_l, src76_l, src98_l,
                                         filt0, filt1, filt2, filt3);
            out3_l = FILT_8TAP_DPADD_S_H(src43_l, src65_l, src87_l, src109_l,
                                         filt0, filt1, filt2, filt3);
            SRARI_H4_SH(out0_r, out1_r, out2_r, out3_r, 6);
            SRARI_H4_SH(out0_l, out1_l, out2_l, out3_l, 6);
            SAT_SH4_SH(out0_r, out1_r, out2_r, out3_r, 7);
            SAT_SH4_SH(out0_l, out1_l, out2_l, out3_l, 7);
            PCKEV_B4_UB(out0_l, out0_r, out1_l, out1_r, out2_l, out2_r, out3_l,
                        out3_r, tmp0, tmp1, tmp2, tmp3);
            XORI_B4_128_UB(tmp0, tmp1, tmp2, tmp3);
            ST_UB4(tmp0, tmp1, tmp2, tmp3, dst_tmp, dst_stride);
            dst_tmp += (4 * dst_stride);

            src10_r = src54_r;
            src32_r = src76_r;
            src54_r = src98_r;
            src21_r = src65_r;
            src43_r = src87_r;
            src65_r = src109_r;
            src10_l = src54_l;
            src32_l = src76_l;
            src54_l = src98_l;
            src21_l = src65_l;
            src43_l = src87_l;
            src65_l = src109_l;
            src6 = src10;
        }

        src += 16;
        dst += 16;
    }
}

static void common_vt_8t_24w_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter, WORD32 height)
{
    common_vt_8t_16w_mult_msa(src, src_stride, dst, dst_stride, filter, height,
                              16);

    common_vt_8t_8w_msa(src + 16, src_stride, dst + 16, dst_stride, filter,
                        height);
}

static void common_vt_8t_32w_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter, WORD32 height)
{
    common_vt_8t_16w_mult_msa(src, src_stride, dst, dst_stride, filter, height,
                              32);
}

static void common_vt_8t_48w_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter, WORD32 height)
{
    common_vt_8t_16w_mult_msa(src, src_stride, dst, dst_stride, filter, height,
                              48);
}

static void common_vt_8t_64w_msa(UWORD8 *src, WORD32 src_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD8 *filter, WORD32 height)
{
    common_vt_8t_16w_mult_msa(src, src_stride, dst, dst_stride, filter, height,
                              64);
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

void ihevc_inter_pred_luma_copy_msa(UWORD8 *pu1_src, UWORD8 *pu1_dst,
                                    WORD32 src_strd, WORD32 dst_strd,
                                    WORD8 *pi1_coeff, WORD32 ht, WORD32 wd)
{
    WORD32 row;
    UNUSED(pi1_coeff);

    switch(wd)
    {
        case 4:
            for(row = 0; row < ht; row++)
            {
                UWORD32 tmp;

                tmp = LW(pu1_src);
                SW(tmp, pu1_dst);

                pu1_src += src_strd;
                pu1_dst += dst_strd;
            }
            break;
        case 8:
            copy_width8_msa(pu1_src, src_strd, pu1_dst, dst_strd, ht);
            break;
        case 12:
            copy_12x16_msa(pu1_src, src_strd, pu1_dst, dst_strd);
            break;
        case 16:
            copy_width16_msa(pu1_src, src_strd, pu1_dst, dst_strd, ht);
            break;
        case 24:
            copy_width24_msa(pu1_src, src_strd, pu1_dst, dst_strd, ht);
            break;
        case 32:
            copy_width32_msa(pu1_src, src_strd, pu1_dst, dst_strd, ht);
            break;
        case 48:
            copy_width48_msa(pu1_src, src_strd, pu1_dst, dst_strd, ht);
            break;
        case 64:
            copy_width64_msa(pu1_src, src_strd, pu1_dst, dst_strd, ht);
    }
}

void ihevc_inter_pred_luma_horz_msa(UWORD8 *pu1_src, UWORD8 *pu1_dst,
                                    WORD32 src_strd, WORD32 dst_strd,
                                    WORD8 *pi1_coeff, WORD32 ht, WORD32 wd)
{
    switch(wd)
    {
        case 4:
            common_hz_8t_4w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                pi1_coeff, ht);
            break;
        case 8:
            common_hz_8t_8w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                pi1_coeff, ht);
            break;
        case 12:
            common_hz_8t_12w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                 pi1_coeff, ht);
            break;
        case 16:
            common_hz_8t_16w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                 pi1_coeff, ht);
            break;
        case 24:
            common_hz_8t_24w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                 pi1_coeff, ht);
            break;
        case 32:
            common_hz_8t_32w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                 pi1_coeff, ht);
            break;
        case 48:
            common_hz_8t_48w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                 pi1_coeff, ht);
            break;
        case 64:
            common_hz_8t_64w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                 pi1_coeff, ht);
    }
}

void ihevc_inter_pred_luma_vert_msa(UWORD8 *pu1_src, UWORD8 *pu1_dst,
                                    WORD32 src_strd, WORD32 dst_strd,
                                    WORD8 *pi1_coeff, WORD32 ht, WORD32 wd)
{
    switch(wd)
    {
        case 4:
            common_vt_8t_4w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                pi1_coeff, ht);
            break;
        case 8:
            common_vt_8t_8w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                pi1_coeff, ht);
            break;
        case 12:
            common_vt_8t_12w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                 pi1_coeff, ht);
            break;
        case 16:
            common_vt_8t_16w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                 pi1_coeff, ht);
            break;
        case 24:
            common_vt_8t_24w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                 pi1_coeff, ht);
            break;
        case 32:
            common_vt_8t_32w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                 pi1_coeff, ht);
            break;
        case 48:
            common_vt_8t_48w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                 pi1_coeff, ht);
            break;
        case 64:
            common_vt_8t_64w_msa(pu1_src, src_strd, pu1_dst, dst_strd,
                                 pi1_coeff, ht);
    }
}

void ihevc_inter_pred_luma_copy_w16out_msa(UWORD8 *pu1_src, WORD16 *pi2_dst,
                                           WORD32 src_strd, WORD32 dst_strd,
                                           WORD8 *pi1_coeff, WORD32 ht,
                                           WORD32 wd)
{
    UNUSED(pi1_coeff);

    switch(wd)
    {
        case 4:
            hevc_copy_4w_msa(pu1_src, src_strd, pi2_dst, dst_strd, ht);
            break;
        case 8:
            hevc_copy_8w_msa(pu1_src, src_strd, pi2_dst, dst_strd, ht);
            break;
        case 12:
            hevc_copy_12w_msa(pu1_src, src_strd, pi2_dst, dst_strd, ht);
            break;
        case 16:
            if(4 == ht)
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
            else
            {
                hevc_copy_16w_msa(pu1_src, src_strd, pi2_dst, dst_strd, ht);
            }
            break;
        case 24:
            hevc_copy_24w_msa(pu1_src, src_strd, pi2_dst, dst_strd, ht);
            break;
        case 32:
            hevc_copy_32w_msa(pu1_src, src_strd, pi2_dst, dst_strd, ht);
            break;
        case 48:
            hevc_copy_48w_msa(pu1_src, src_strd, pi2_dst, dst_strd, ht);
            break;
        case 64:
            hevc_copy_64w_msa(pu1_src, src_strd, pi2_dst, dst_strd, ht);
    }
}

void ihevc_inter_pred_luma_horz_w16out_msa(UWORD8 *pu1_src, WORD16 *pi2_dst,
                                           WORD32 src_strd, WORD32 dst_strd,
                                           WORD8 *pi1_coeff, WORD32 ht,
                                           WORD32 wd)
{
    switch(wd)
    {
        case 4:
            hevc_hz_8t_4w_msa(pu1_src, src_strd, pi2_dst, dst_strd, pi1_coeff,
                              ht);
            break;
        case 8:
            hevc_hz_8t_8w_msa(pu1_src, src_strd, pi2_dst, dst_strd, pi1_coeff,
                              ht);
            break;
        case 12:
            hevc_hz_8t_12w_msa(pu1_src, src_strd, pi2_dst, dst_strd, pi1_coeff,
                               ht);
            break;
        case 16:
            hevc_hz_8t_16w_msa(pu1_src, src_strd, pi2_dst, dst_strd, pi1_coeff,
                               ht);
            break;
        case 24:
            hevc_hz_8t_24w_msa(pu1_src, src_strd, pi2_dst, dst_strd, pi1_coeff,
                               ht);
            break;
        case 32:
            hevc_hz_8t_32w_msa(pu1_src, src_strd, pi2_dst, dst_strd, pi1_coeff,
                               ht);
            break;
        case 48:
            hevc_hz_8t_48w_msa(pu1_src, src_strd, pi2_dst, dst_strd, pi1_coeff,
                               ht);
            break;
        case 64:
            hevc_hz_8t_64w_msa(pu1_src, src_strd, pi2_dst, dst_strd, pi1_coeff,
                               ht);
    }
}

void ihevc_inter_pred_luma_vert_w16out_msa(UWORD8 *pu1_src, WORD16 *pi2_dst,
                                           WORD32 src_strd, WORD32 dst_strd,
                                           WORD8 *pi1_coeff, WORD32 ht,
                                           WORD32 wd)
{
    switch(wd)
    {
        case 4:
            hevc_vt_8t_4w_msa(pu1_src, src_strd, pi2_dst, dst_strd, pi1_coeff,
                              ht);
            break;
        case 8:
            hevc_vt_8t_8w_msa(pu1_src, src_strd, pi2_dst, dst_strd, pi1_coeff,
                              ht);
            break;
        case 12:
            hevc_vt_8t_12w_msa(pu1_src, src_strd, pi2_dst, dst_strd, pi1_coeff,
                               ht);
            break;
        case 16:
            hevc_vt_8t_16w_msa(pu1_src, src_strd, pi2_dst, dst_strd, pi1_coeff,
                               ht);
            break;
        case 24:
            hevc_vt_8t_24w_msa(pu1_src, src_strd, pi2_dst, dst_strd, pi1_coeff,
                               ht);
            break;
        case 32:
            hevc_vt_8t_32w_msa(pu1_src, src_strd, pi2_dst, dst_strd, pi1_coeff,
                               ht);
            break;
        case 48:
            hevc_vt_8t_48w_msa(pu1_src, src_strd, pi2_dst, dst_strd, pi1_coeff,
                               ht);
            break;
        case 64:
            hevc_vt_8t_64w_msa(pu1_src, src_strd, pi2_dst, dst_strd, pi1_coeff,
                               ht);
    }
}

void ihevc_inter_pred_luma_vert_w16inp_msa(WORD16 *pi2_src, UWORD8 *pu1_dst,
                                           WORD32 src_strd, WORD32 dst_strd,
                                           WORD8 *pi1_coeff, WORD32 ht,
                                           WORD32 wd)
{
    switch(wd)
    {
        case 4:
           hevc_vt_uni_8t_4x8mult_w16inp_msa(pi2_src, src_strd, pu1_dst,
                                             dst_strd, pi1_coeff, ht);
           break;
        case 8:
           hevc_vt_uni_8t_8multx4mult_w16inp_msa(pi2_src, src_strd, pu1_dst,
                                                 dst_strd, pi1_coeff, ht, 8);
           break;
        case 12:
           hevc_vt_uni_8t_12x4mult_w16inp_msa(pi2_src, src_strd, pu1_dst,
                                              dst_strd, pi1_coeff, ht);
           break;
        case 16:
           hevc_vt_uni_8t_8multx4mult_w16inp_msa(pi2_src, src_strd, pu1_dst,
                                                 dst_strd, pi1_coeff, ht, 16);
           break;
        case 24:
           hevc_vt_uni_8t_8multx4mult_w16inp_msa(pi2_src, src_strd, pu1_dst,
                                                 dst_strd, pi1_coeff, ht, 24);
           break;
        case 32:
           hevc_vt_uni_8t_8multx4mult_w16inp_msa(pi2_src, src_strd, pu1_dst,
                                                 dst_strd, pi1_coeff, ht, 32);
           break;
        case 48:
           hevc_vt_uni_8t_8multx4mult_w16inp_msa(pi2_src, src_strd, pu1_dst,
                                                 dst_strd, pi1_coeff, ht, 48);
           break;
        case 64:
           hevc_vt_uni_8t_8multx4mult_w16inp_msa(pi2_src, src_strd, pu1_dst,
                                                 dst_strd, pi1_coeff, ht, 64);
    }
}

void ihevc_inter_pred_luma_vert_w16inp_w16out_msa(WORD16 *pi2_src,
                                                  WORD16 *pi2_dst,
                                                  WORD32 src_strd,
                                                  WORD32 dst_strd,
                                                  WORD8 *pi1_coeff,
                                                  WORD32 ht, WORD32 wd)
{
    switch(wd)
    {
        case 4:
           hevc_vt_8t_4x8mult_w16inp_msa(pi2_src, src_strd, pi2_dst, dst_strd,
                                         pi1_coeff, ht);
           break;
        case 8:
           hevc_vt_8t_8multx4mult_w16inp_msa(pi2_src, src_strd, pi2_dst,
                                             dst_strd, pi1_coeff, ht, 8);
           break;
        case 12:
           hevc_vt_8t_12x4mult_w16inp_msa(pi2_src, src_strd, pi2_dst, dst_strd,
                                          pi1_coeff, ht);
           break;
        case 16:
           hevc_vt_8t_8multx4mult_w16inp_msa(pi2_src, src_strd, pi2_dst,
                                             dst_strd, pi1_coeff, ht, 16);
           break;
        case 24:
           hevc_vt_8t_8multx4mult_w16inp_msa(pi2_src, src_strd, pi2_dst,
                                             dst_strd, pi1_coeff, ht, 24);
           break;
        case 32:
           hevc_vt_8t_8multx4mult_w16inp_msa(pi2_src, src_strd, pi2_dst,
                                             dst_strd, pi1_coeff, ht, 32);
           break;
        case 48:
           hevc_vt_8t_8multx4mult_w16inp_msa(pi2_src, src_strd, pi2_dst,
                                             dst_strd, pi1_coeff, ht, 48);
           break;
        case 64:
           hevc_vt_8t_8multx4mult_w16inp_msa(pi2_src, src_strd, pi2_dst,
                                             dst_strd, pi1_coeff, ht, 64);
    }
}
