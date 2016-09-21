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
#include "ihevc_macros.h"
#include "ihevc_inter_pred.h"
#include "ihevc_macros_msa.h"

#define HEVC_PCK_SW_SB4(in0, in1, in2, in3, out)                  \
{                                                                 \
    v8i16 tmp0_m, tmp1_m;                                         \
                                                                  \
    PCKEV_H2_SH(in0, in1, in2, in3, tmp0_m, tmp1_m);              \
    out = (v4i32) __msa_pckev_b((v16i8) tmp1_m, (v16i8) tmp0_m);  \
}

#define HEVC_PCK_SW_SB8(in0, in1, in2, in3, in4, in5, in6, in7,  \
                        out0, out1)                              \
{                                                                \
    v8i16 tmp0_m, tmp1_m, tmp2_m, tmp3_m;                        \
                                                                 \
    PCKEV_H4_SH(in0, in1, in2, in3, in4, in5, in6, in7,          \
                tmp0_m, tmp1_m, tmp2_m, tmp3_m);                 \
    PCKEV_B2_SW(tmp1_m, tmp0_m, tmp3_m, tmp2_m, out0, out1);     \
}

#define HEVC_PCK_SW_SB12(in0, in1, in2, in3, in4, in5, in6, in7,   \
                         in8, in9, in10, in11, out0, out1, out2)   \
{                                                                  \
    v8i16 tmp0_m, tmp1_m, tmp2_m, tmp3_m, tmp4_m, tmp5_m;          \
                                                                   \
    PCKEV_H4_SH(in0, in1, in2, in3, in4, in5, in6, in7,            \
                tmp0_m, tmp1_m, tmp2_m, tmp3_m);                   \
    PCKEV_H2_SH(in8, in9, in10, in11, tmp4_m, tmp5_m);             \
    PCKEV_B2_SW(tmp1_m, tmp0_m, tmp3_m, tmp2_m, out0, out1);       \
    out2 = (v4i32) __msa_pckev_b((v16i8) tmp5_m, (v16i8) tmp4_m);  \
}

#define HEVC_UNIW_RND_CLIP4_R(in0, in1, in2, in3, in4, in5, in6, in7,  \
                              wgt, offset, rnd, out0_r, out1_r,        \
                              out2_r, out3_r)                          \
{                                                                      \
    ILVR_H4_SW(in0, in1, in2, in3, in4, in5, in6, in7,                 \
               out0_r, out1_r, out2_r, out3_r);                        \
    DOTP_SH4_SW(out0_r, out1_r, out2_r, out3_r, wgt, wgt, wgt, wgt,    \
                out0_r, out1_r, out2_r, out3_r);                       \
    SRAR_W4_SW(out0_r, out1_r, out2_r, out3_r, rnd);                   \
    ADD4(out0_r, offset, out1_r, offset, out2_r, offset, out3_r,       \
         offset, out0_r, out1_r, out2_r, out3_r);                      \
    CLIP_SW4_0_255(out0_r, out1_r, out2_r, out3_r);                    \
}

#define HEVC_UNIW_RND_CLIP2(in0, in1, in2, in3, wgt, offset, rnd,    \
                            out0_r, out1_r, out0_l, out1_l)          \
{                                                                    \
    ILVR_H2_SW(in0, in1, in2, in3, out0_r, out1_r);                  \
    ILVL_H2_SW(in0, in1, in2, in3, out0_l, out1_l);                  \
    DOTP_SH4_SW(out0_r, out1_r, out0_l, out1_l, wgt, wgt, wgt, wgt,  \
                out0_r, out1_r, out0_l, out1_l);                     \
    SRAR_W4_SW(out0_r, out1_r, out0_l, out1_l, rnd);                 \
    ADD4(out0_r, offset, out1_r, offset,                             \
         out0_l, offset, out1_l, offset,                             \
         out0_r, out1_r, out0_l, out1_l);                            \
    CLIP_SW4_0_255(out0_r, out1_r, out0_l, out1_l);                  \
}

#define HEVC_UNIW_RND_CLIP4(in0, in1, in2, in3, in4, in5, in6, in7,  \
                            wgt, offset, rnd,                        \
                            out0_r, out1_r, out2_r, out3_r,          \
                            out0_l, out1_l, out2_l, out3_l)          \
{                                                                    \
    HEVC_UNIW_RND_CLIP2(in0, in1, in2, in3, wgt, offset, rnd,        \
                        out0_r, out1_r, out0_l, out1_l);             \
    HEVC_UNIW_RND_CLIP2(in4, in5, in6, in7, wgt, offset, rnd,        \
                        out2_r, out3_r, out2_l, out3_l);             \
}

#define HEVC_BIW_RND_CLIP4_R(in0, in1, in2, in3, vec0, vec1,        \
                             vec2, vec3, wgt, rnd, offset,          \
                             out0_r, out1_r, out2_r, out3_r)        \
{                                                                   \
    ILVR_H4_SW(in0, vec0, in1, vec1, in2, vec2, in3, vec3,          \
               out0_r, out1_r, out2_r, out3_r);                     \
                                                                    \
    out0_r = __msa_dpadd_s_w(offset, (v8i16) out0_r, (v8i16) wgt);  \
    out1_r = __msa_dpadd_s_w(offset, (v8i16) out1_r, (v8i16) wgt);  \
    out2_r = __msa_dpadd_s_w(offset, (v8i16) out2_r, (v8i16) wgt);  \
    out3_r = __msa_dpadd_s_w(offset, (v8i16) out3_r, (v8i16) wgt);  \
                                                                    \
    SRAR_W4_SW(out0_r, out1_r, out2_r, out3_r, rnd);                \
    CLIP_SW4_0_255(out0_r, out1_r, out2_r, out3_r);                 \
}

#define HEVC_BIW_RND_CLIP2(in0, in1, vec0, vec1, wgt, rnd, offset,  \
                           out0_r, out1_r, out0_l, out1_l)          \
{                                                                   \
    ILVR_H2_SW(in0, vec0, in1, vec1, out0_r, out1_r);               \
    ILVL_H2_SW(in0, vec0, in1, vec1, out0_l, out1_l);               \
                                                                    \
    out0_r = __msa_dpadd_s_w(offset, (v8i16) out0_r, (v8i16) wgt);  \
    out1_r = __msa_dpadd_s_w(offset, (v8i16) out1_r, (v8i16) wgt);  \
    out0_l = __msa_dpadd_s_w(offset, (v8i16) out0_l, (v8i16) wgt);  \
    out1_l = __msa_dpadd_s_w(offset, (v8i16) out1_l, (v8i16) wgt);  \
                                                                    \
    SRAR_W4_SW(out0_r, out1_r, out0_l, out1_l, rnd);                \
    CLIP_SW4_0_255(out0_r, out1_r, out0_l, out1_l);                 \
}

#define HEVC_BIW_RND_CLIP4(in0, in1, in2, in3, vec0, vec1,      \
                           vec2, vec3, wgt, rnd, offset,        \
                           out0_r, out1_r, out2_r, out3_r,      \
                           out0_l, out1_l, out2_l, out3_l)      \
{                                                               \
    HEVC_BIW_RND_CLIP2(in0, in1, vec0, vec1, wgt, rnd, offset,  \
                       out0_r, out1_r, out0_l, out1_l)          \
    HEVC_BIW_RND_CLIP2(in2, in3, vec2, vec3, wgt, rnd, offset,  \
                       out2_r, out3_r, out2_l, out3_l)          \
}

#define HEVC_BI_RND_CLIP2(in0, in1, vec0, vec1,     \
                          rnd_val, out0, out1)      \
{                                                   \
    ADDS_SH2_SH(vec0, in0, vec1, in1, out0, out1);  \
    SRARI_H2_SH(out0, out1, rnd_val);               \
    CLIP_SH2_0_255(out0, out1);                     \
}

#define HEVC_BI_RND_CLIP4(in0, in1, in2, in3,                      \
                          vec0, vec1, vec2, vec3, rnd_val,         \
                          out0, out1, out2, out3)                  \
{                                                                  \
    HEVC_BI_RND_CLIP2(in0, in1, vec0, vec1, rnd_val, out0, out1);  \
    HEVC_BI_RND_CLIP2(in2, in3, vec2, vec3, rnd_val, out2, out3);  \
}

static void hevc_chroma_uniwgt_4x4_msa(WORD16 *src, WORD32 src_stride,
                                       UWORD8 *dst, WORD32 dst_stride,
                                       WORD32 wgt0_cb, WORD32 wgt0_cr,
                                       WORD32 off0_cb, WORD32 off0_cr,
                                       WORD32 shift, WORD32 lvl_shift)
{
    v8i16 src0, src1, src2, src3, weight, shift_vec, tmp1;
    v4i32 src0_r, src1_r, src2_r, src3_r, round, offset, tmp0;

    offset = __msa_fill_w(off0_cb);
    tmp0 = __msa_fill_w(off0_cr);
    offset = __msa_ilvr_w(tmp0, offset);
    weight = __msa_fill_h(wgt0_cb);
    tmp1 = __msa_fill_h(wgt0_cr);
    weight = (v8i16) __msa_ilvr_w((v4i32) tmp1, (v4i32) weight);
    shift_vec = __msa_fill_h(lvl_shift);
    round = __msa_fill_w(shift);

    LD_SH4(src, src_stride, src0, src1, src2, src3);
    HEVC_UNIW_RND_CLIP4_R(shift_vec, src0, shift_vec, src1, shift_vec, src2,
                          shift_vec, src3, weight, offset, round,
                          src0_r, src1_r, src2_r, src3_r);
    HEVC_PCK_SW_SB4(src1_r, src0_r, src3_r, src2_r, src0_r);
    ST4x4_UB(src0_r, src0_r, 0, 1, 2, 3, dst, dst_stride);
}

static void hevc_uniwgt_4x8mult_msa(WORD16 *src, WORD32 src_stride,
                                    UWORD8 *dst, WORD32 dst_stride,
                                    WORD32 wgt, WORD32 off, WORD32 shift,
                                    WORD32 lvl_shift, WORD32 height)
{
    WORD32 loop_cnt;
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7, weight, shift_vec;
    v4i32 src0_r, src1_r, src2_r, src3_r, src4_r, src5_r, src6_r, src7_r;
    v4i32 round, offset;

    offset = __msa_fill_w(off);
    weight = __msa_fill_h(wgt);
    shift_vec = __msa_fill_h(lvl_shift);
    round = __msa_fill_w(shift);

    for(loop_cnt = (height >> 3); loop_cnt--;)
    {
        LD_SH8(src, src_stride, src0, src1, src2, src3, src4, src5, src6, src7);
        src += (8 * src_stride);
        HEVC_UNIW_RND_CLIP4_R(shift_vec, src0, shift_vec, src1, shift_vec, src2,
                              shift_vec, src3, weight, offset, round,
                              src0_r, src1_r, src2_r, src3_r);
        HEVC_UNIW_RND_CLIP4_R(shift_vec, src4, shift_vec, src5, shift_vec, src6,
                              shift_vec, src7, weight, offset, round,
                              src4_r, src5_r, src6_r, src7_r);
        HEVC_PCK_SW_SB8(src1_r, src0_r, src3_r, src2_r, src5_r, src4_r, src7_r,
                        src6_r, src0_r, src1_r);
        ST4x8_UB(src0_r, src1_r, dst, dst_stride);
        dst += (8 * dst_stride);
    }
}

static void hevc_chroma_uniwgt_4x8mult_msa(WORD16 *src, WORD32 src_stride,
                                           UWORD8 *dst, WORD32 dst_stride,
                                           WORD32 wgt0_cb, WORD32 wgt0_cr,
                                           WORD32 off0_cb, WORD32 off0_cr,
                                           WORD32 shift, WORD32 lvl_shift,
                                           WORD32 height)
{
    WORD32 loop_cnt;
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7, weight, shift_vec;
    v8i16 tmp1;
    v4i32 src0_r, src1_r, src2_r, src3_r, src4_r, src5_r, src6_r, src7_r;
    v4i32 round, offset, tmp0;

    offset = __msa_fill_w(off0_cb);
    tmp0 = __msa_fill_w(off0_cr);
    offset = __msa_ilvr_w(tmp0, offset);
    weight = __msa_fill_h(wgt0_cb);
    tmp1 = __msa_fill_h(wgt0_cr);
    weight = (v8i16) __msa_ilvr_w((v4i32) tmp1, (v4i32) weight);
    shift_vec = __msa_fill_h(lvl_shift);
    round = __msa_fill_w(shift);

    for(loop_cnt = (height >> 3); loop_cnt--;)
    {
        LD_SH8(src, src_stride, src0, src1, src2, src3, src4, src5, src6, src7);
        src += (8 * src_stride);
        HEVC_UNIW_RND_CLIP4_R(shift_vec, src0, shift_vec, src1, shift_vec, src2,
                              shift_vec, src3, weight, offset, round,
                              src0_r, src1_r, src2_r, src3_r);
        HEVC_UNIW_RND_CLIP4_R(shift_vec, src4, shift_vec, src5, shift_vec, src6,
                              shift_vec, src7, weight, offset, round,
                              src4_r, src5_r, src6_r, src7_r);
        HEVC_PCK_SW_SB8(src1_r, src0_r, src3_r, src2_r, src5_r, src4_r, src7_r,
                        src6_r, src0_r, src1_r);
        ST4x8_UB(src0_r, src1_r, dst, dst_stride);
        dst += (8 * dst_stride);
    }
}

static void hevc_chroma_uniwgt_8x2_msa(WORD16 *src, WORD32 src_stride,
                                       UWORD8 *dst, WORD32 dst_stride,
                                       WORD32 wgt0_cb, WORD32 wgt0_cr,
                                       WORD32 off0_cb, WORD32 off0_cr,
                                       WORD32 shift, WORD32 lvl_shift)
{
    v8i16 src0, src1, weight, shift_vec, tmp1;
    v4i32 src0_r, src1_r, src0_l, src1_l, round, offset, tmp0;

    offset = __msa_fill_w(off0_cb);
    tmp0 = __msa_fill_w(off0_cr);
    offset = __msa_ilvr_w(tmp0, offset);
    weight = __msa_fill_h(wgt0_cb);
    tmp1 = __msa_fill_h(wgt0_cr);
    weight = (v8i16) __msa_ilvr_w((v4i32) tmp1, (v4i32) weight);
    shift_vec = __msa_fill_h(lvl_shift);
    round = __msa_fill_w(shift);

    LD_SH2(src, src_stride, src0, src1);
    HEVC_UNIW_RND_CLIP2(shift_vec, src0, shift_vec, src1, weight, offset, round,
                        src0_r, src1_r, src0_l, src1_l);
    HEVC_PCK_SW_SB4(src0_l, src0_r, src1_l, src1_r, src0_r);
    ST8x2_UB(src0_r, dst, dst_stride);
}

static void hevc_uniwgt_8x4mult_msa(WORD16 *src, WORD32 src_stride,
                                    UWORD8 *dst, WORD32 dst_stride,
                                    WORD32 wgt, WORD32 off, WORD32 shift,
                                    WORD32 lvl_shift, WORD32 height)
{
    WORD32 loop_cnt;
    v8i16 src0, src1, src2, src3, weight, shift_vec;
    v4i32 src0_r, src1_r, src2_r, src3_r, src0_l, src1_l, src2_l, src3_l;
    v4i32 round, offset;

    offset = __msa_fill_w(off);
    weight = __msa_fill_h(wgt);
    shift_vec = __msa_fill_h(lvl_shift);
    round = __msa_fill_w(shift);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SH4(src, src_stride, src0, src1, src2, src3);
        src += (4 * src_stride);
        HEVC_UNIW_RND_CLIP4(shift_vec, src0, shift_vec, src1, shift_vec, src2,
                            shift_vec, src3, weight, offset, round, src0_r,
                            src1_r, src2_r, src3_r, src0_l, src1_l, src2_l,
                            src3_l);
        HEVC_PCK_SW_SB8(src0_l, src0_r, src1_l, src1_r, src2_l, src2_r, src3_l,
                        src3_r, src0_r, src1_r);
        ST8x4_UB(src0_r, src1_r, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_chroma_uniwgt_8x4mult_msa(WORD16 *src, WORD32 src_stride,
                                           UWORD8 *dst, WORD32 dst_stride,
                                           WORD32 wgt0_cb, WORD32 wgt0_cr,
                                           WORD32 off0_cb, WORD32 off0_cr,
                                           WORD32 shift, WORD32 lvl_shift,
                                           WORD32 height)
{
    WORD32 loop_cnt;
    v8i16 src0, src1, src2, src3, weight, shift_vec, tmp1;
    v4i32 src0_r, src1_r, src2_r, src3_r, src0_l, src1_l, src2_l, src3_l;
    v4i32 round, offset, tmp0;

    offset = __msa_fill_w(off0_cb);
    tmp0 = __msa_fill_w(off0_cr);
    offset = __msa_ilvr_w(tmp0, offset);
    weight = __msa_fill_h(wgt0_cb);
    tmp1 = __msa_fill_h(wgt0_cr);
    weight = (v8i16) __msa_ilvr_w((v4i32) tmp1, (v4i32) weight);
    shift_vec = __msa_fill_h(lvl_shift);
    round = __msa_fill_w(shift);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SH4(src, src_stride, src0, src1, src2, src3);
        src += (4 * src_stride);
        HEVC_UNIW_RND_CLIP4(shift_vec, src0, shift_vec, src1, shift_vec, src2,
                            shift_vec, src3, weight, offset, round, src0_r,
                            src1_r, src2_r, src3_r, src0_l, src1_l, src2_l,
                            src3_l);
        HEVC_PCK_SW_SB8(src0_l, src0_r, src1_l, src1_r, src2_l, src2_r, src3_l,
                        src3_r, src0_r, src1_r);
        ST8x4_UB(src0_r, src1_r, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_uniwgt_12x4mult_msa(WORD16 *src, WORD32 src_stride,
                                     UWORD8 *dst, WORD32 dst_stride,
                                     WORD32 wgt, WORD32 off, WORD32 shift,
                                     WORD32 lvl_shift, WORD32 height)
{
    WORD32 loop_cnt;
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7, weight, shift_vec;
    v4i32 src0_r, src1_r, src2_r, src3_r, src4_r, src5_r, src6_r, src7_r;
    v4i32 src0_l, src1_l, src2_l, src3_l, round, offset;

    offset = __msa_fill_w(off);
    weight = __msa_fill_h(wgt);
    shift_vec = __msa_fill_h(lvl_shift);
    round = __msa_fill_w(shift);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SH4(src, src_stride, src0, src1, src2, src3);
        LD_SH4(src + 8, src_stride, src4, src5, src6, src7);
        src += (4 * src_stride);
        HEVC_UNIW_RND_CLIP4(shift_vec, src0, shift_vec, src1, shift_vec, src2,
                            shift_vec, src3, weight, offset, round, src0_r,
                            src1_r, src2_r, src3_r, src0_l, src1_l, src2_l,
                            src3_l);
        HEVC_UNIW_RND_CLIP4_R(shift_vec, src4, shift_vec, src5, shift_vec, src6,
                              shift_vec, src7, weight, offset, round,
                              src4_r, src5_r, src6_r, src7_r);
        HEVC_PCK_SW_SB12(src0_l, src0_r, src1_l, src1_r, src2_l, src2_r, src3_l,
                         src3_r, src5_r, src4_r, src7_r, src6_r, src0_r, src1_r,
                         src2_r);
        ST12x4_UB(src0_r, src1_r, src2_r, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_chroma_uniwgt_12x4mult_msa(WORD16 *src, WORD32 src_stride,
                                            UWORD8 *dst, WORD32 dst_stride,
                                            WORD32 wgt0_cb, WORD32 wgt0_cr,
                                            WORD32 off0_cb, WORD32 off0_cr,
                                            WORD32 shift, WORD32 lvl_shift,
                                            WORD32 height)
{
    WORD32 loop_cnt;
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7, weight, shift_vec;
    v8i16 tmp1;
    v4i32 src0_r, src1_r, src2_r, src3_r, src4_r, src5_r, src6_r, src7_r;
    v4i32 src0_l, src1_l, src2_l, src3_l, round, offset, tmp0;

    offset = __msa_fill_w(off0_cb);
    tmp0 = __msa_fill_w(off0_cr);
    offset = __msa_ilvr_w(tmp0, offset);
    weight = __msa_fill_h(wgt0_cb);
    tmp1 = __msa_fill_h(wgt0_cr);
    weight = (v8i16) __msa_ilvr_w((v4i32) tmp1, (v4i32) weight);
    shift_vec = __msa_fill_h(lvl_shift);
    round = __msa_fill_w(shift);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SH4(src, src_stride, src0, src1, src2, src3);
        LD_SH4(src + 8, src_stride, src4, src5, src6, src7);
        src += (4 * src_stride);
        HEVC_UNIW_RND_CLIP4(shift_vec, src0, shift_vec, src1, shift_vec, src2,
                            shift_vec, src3, weight, offset, round, src0_r,
                            src1_r, src2_r, src3_r, src0_l, src1_l, src2_l,
                            src3_l);
        HEVC_UNIW_RND_CLIP4_R(shift_vec, src4, shift_vec, src5, shift_vec, src6,
                              shift_vec, src7, weight, offset, round,
                              src4_r, src5_r, src6_r, src7_r);
        HEVC_PCK_SW_SB12(src0_l, src0_r, src1_l, src1_r, src2_l, src2_r, src3_l,
                         src3_r, src5_r, src4_r, src7_r, src6_r, src0_r, src1_r,
                         src2_r);
        ST12x4_UB(src0_r, src1_r, src2_r, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_chroma_uniwgt_16x2mult_msa(WORD16 *src, WORD32 src_stride,
                                            UWORD8 *dst, WORD32 dst_stride,
                                            WORD32 wgt0_cb, WORD32 wgt0_cr,
                                            WORD32 off0_cb, WORD32 off0_cr,
                                            WORD32 shift, WORD32 lvl_shift,
                                            WORD32 height)
{
    WORD32 loop_cnt;
    v8i16 src0, src1, src2, src3, weight, shift_vec, tmp1;
    v4i32 src0_r, src1_r, src2_r, src3_r, src0_l, src1_l, src2_l, src3_l;
    v4i32 round, offset, tmp0;

    offset = __msa_fill_w(off0_cb);
    tmp0 = __msa_fill_w(off0_cr);
    offset = __msa_ilvr_w(tmp0, offset);
    weight = __msa_fill_h(wgt0_cb);
    tmp1 = __msa_fill_h(wgt0_cr);
    weight = (v8i16) __msa_ilvr_w((v4i32) tmp1, (v4i32) weight);
    shift_vec = __msa_fill_h(lvl_shift);
    round = __msa_fill_w(shift);

    for(loop_cnt = (height >> 1); loop_cnt--;)
    {
        LD_SH2(src, src_stride, src0, src1);
        LD_SH2(src + 8, src_stride, src2, src3);
        src += (2 * src_stride);
        HEVC_UNIW_RND_CLIP4(shift_vec, src0, shift_vec, src1, shift_vec, src2,
                            shift_vec, src3, weight, offset, round, src0_r,
                            src1_r, src2_r, src3_r, src0_l, src1_l, src2_l,
                            src3_l);
        HEVC_PCK_SW_SB8(src0_l, src0_r, src2_l, src2_r, src1_l, src1_r, src3_l,
                        src3_r, src0_r, src1_r);
        ST_SW2(src0_r, src1_r, dst, dst_stride);
        dst += (2 * dst_stride);
    }
}

static void hevc_uniwgt_16multx4mult_msa(WORD16 *src, WORD32 src_stride,
                                         UWORD8 *dst, WORD32 dst_stride,
                                         WORD32 wgt, WORD32 off,
                                         WORD32 shift, WORD32 lvl_shift,
                                         WORD32 height, WORD32 width)
{
    WORD32 loop_cnt, cnt;
    WORD16 *src_tmp;
    UWORD8 *dst_tmp;
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7, weight, shift_vec;
    v4i32 src0_r, src1_r, src2_r, src3_r, src4_r, src5_r, src6_r, src7_r;
    v4i32 src0_l, src1_l, src2_l, src3_l, src4_l, src5_l, src6_l, src7_l;
    v4i32 round, offset;

    offset = __msa_fill_w(off);
    weight = __msa_fill_h(wgt);
    shift_vec = __msa_fill_h(lvl_shift);
    round = __msa_fill_w(shift);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        src_tmp = src;
        dst_tmp = dst;

        for(cnt = (width >> 4); cnt--;)
        {
            LD_SH4(src_tmp, src_stride, src0, src1, src2, src3);
            LD_SH4(src_tmp + 8, src_stride, src4, src5, src6, src7);
            src_tmp += 16;
            HEVC_UNIW_RND_CLIP4(shift_vec, src0, shift_vec, src1, shift_vec,
                                src2, shift_vec, src3, weight, offset, round,
                                src0_r, src1_r, src2_r, src3_r, src0_l, src1_l,
                                src2_l, src3_l);
            HEVC_UNIW_RND_CLIP4(shift_vec, src4, shift_vec, src5, shift_vec,
                                src6, shift_vec, src7, weight, offset, round,
                                src4_r, src5_r, src6_r, src7_r, src4_l, src5_l,
                                src6_l, src7_l);
            HEVC_PCK_SW_SB8(src0_l, src0_r, src4_l, src4_r, src1_l, src1_r,
                            src5_l, src5_r, src0_r, src1_r);
            HEVC_PCK_SW_SB8(src2_l, src2_r, src6_l, src6_r, src3_l, src3_r,
                            src7_l, src7_r, src2_r, src3_r);
            ST_SW2(src0_r, src1_r, dst_tmp, dst_stride);
            ST_SW2(src2_r, src3_r, dst_tmp + 2 * dst_stride, dst_stride);
            dst_tmp += 16;
        }

        src += (4 * src_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_chroma_uniwgt_16multx4mult_msa(WORD16 *src, WORD32 src_stride,
                                                UWORD8 *dst, WORD32 dst_stride,
                                                WORD32 wgt0_cb, WORD32 wgt0_cr,
                                                WORD32 off0_cb, WORD32 off0_cr,
                                                WORD32 shift, WORD32 lvl_shift,
                                                WORD32 height, WORD32 width)
{
    WORD32 loop_cnt, cnt;
    WORD16 *src_tmp;
    UWORD8 *dst_tmp;
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7, weight, shift_vec;
    v8i16 tmp1;
    v4i32 src0_r, src1_r, src2_r, src3_r, src4_r, src5_r, src6_r, src7_r;
    v4i32 src0_l, src1_l, src2_l, src3_l, src4_l, src5_l, src6_l, src7_l;
    v4i32 round, offset, tmp0;

    offset = __msa_fill_w(off0_cb);
    tmp0 = __msa_fill_w(off0_cr);
    offset = __msa_ilvr_w(tmp0, offset);
    weight = __msa_fill_h(wgt0_cb);
    tmp1 = __msa_fill_h(wgt0_cr);
    weight = (v8i16) __msa_ilvr_w((v4i32) tmp1, (v4i32) weight);
    shift_vec = __msa_fill_h(lvl_shift);
    round = __msa_fill_w(shift);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        src_tmp = src;
        dst_tmp = dst;

        for(cnt = (width >> 4); cnt--;)
        {
            LD_SH4(src_tmp, src_stride, src0, src1, src2, src3);
            LD_SH4(src_tmp + 8, src_stride, src4, src5, src6, src7);
            src_tmp += 16;
            HEVC_UNIW_RND_CLIP4(shift_vec, src0, shift_vec, src1, shift_vec,
                                src2, shift_vec, src3, weight, offset, round,
                                src0_r, src1_r, src2_r, src3_r, src0_l, src1_l,
                                src2_l, src3_l);
            HEVC_UNIW_RND_CLIP4(shift_vec, src4, shift_vec, src5, shift_vec,
                                src6, shift_vec, src7, weight, offset, round,
                                src4_r, src5_r, src6_r, src7_r, src4_l, src5_l,
                                src6_l, src7_l);
            HEVC_PCK_SW_SB8(src0_l, src0_r, src4_l, src4_r, src1_l, src1_r,
                            src5_l, src5_r, src0_r, src1_r);
            HEVC_PCK_SW_SB8(src2_l, src2_r, src6_l, src6_r, src3_l, src3_r,
                            src7_l, src7_r, src2_r, src3_r);
            ST_SW2(src0_r, src1_r, dst_tmp, dst_stride);
            ST_SW2(src2_r, src3_r, dst_tmp + 2 * dst_stride, dst_stride);
            dst_tmp += 16;
        }

        src += (4 * src_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_chroma_biwgt_4x4_msa(WORD16 *src0, WORD32 src0_stride,
                                      WORD16 *src1, WORD32 src1_stride,
                                      UWORD8 *dst, WORD32 dst_stride,
                                      WORD32 wgt0_cb, WORD32 wgt0_cr,
                                      WORD32 wgt1_cb, WORD32 wgt1_cr,
                                      WORD32 off0_cb, WORD32 off0_cr,
                                      WORD32 off1_cb, WORD32 off1_cr,
                                      WORD32 shift, WORD32 lvl_shift0,
                                      WORD32 lvl_shift1)
{
    WORD32 tmp;
    v8i16 src0_0, src0_1, src0_2, src0_3, src1_0, src1_1, src1_2, src1_3;
    v4i32 src0_r, src1_r, src2_r, src3_r, weight, round, offset, tmp_vec;

    tmp = lvl_shift0 * wgt0_cb;
    tmp += (lvl_shift1 * wgt1_cb);
    tmp += (off0_cb + off1_cb) << (shift - 1);
    offset = __msa_fill_w(tmp);
    tmp = lvl_shift0 * wgt0_cr;
    tmp += (lvl_shift1 * wgt1_cr);
    tmp += (off0_cr + off1_cr) << (shift - 1);
    tmp_vec = __msa_fill_w(tmp);
    offset = __msa_ilvr_w(tmp_vec, offset);
    tmp = (wgt0_cb << 16) | (wgt1_cb & 0x0000FFFF);
    weight = __msa_fill_w(tmp);
    tmp = (wgt0_cr << 16) | (wgt1_cr & 0x0000FFFF);
    tmp_vec = __msa_fill_w(tmp);
    weight = __msa_ilvr_w(tmp_vec, weight);
    round = __msa_fill_w(shift);

    LD_SH4(src0, src0_stride, src0_0, src0_1, src0_2, src0_3);
    LD_SH4(src1, src1_stride, src1_0, src1_1, src1_2, src1_3);
    HEVC_BIW_RND_CLIP4_R(src0_0, src0_1, src0_2, src0_3, src1_0, src1_1,
                         src1_2, src1_3, weight, round, offset,
                         src0_r, src1_r, src2_r, src3_r);
    HEVC_PCK_SW_SB4(src1_r, src0_r, src3_r, src2_r, src0_r);
    ST4x4_UB(src0_r, src0_r, 0, 1, 2, 3, dst, dst_stride);
}

static void hevc_biwgt_4x8mult_msa(WORD16 *src0, WORD32 src0_stride,
                                   WORD16 *src1, WORD32 src1_stride,
                                   UWORD8 *dst, WORD32 dst_stride,
                                   WORD32 wgt0, WORD32 wgt1,
                                   WORD32 off0, WORD32 off1,
                                   WORD32 shift, WORD32 lvl_shift0,
                                   WORD32 lvl_shift1, WORD32 height)
{
    WORD32 loop_cnt, tmp;
    v8i16 src0_0, src0_1, src0_2, src0_3, src0_4, src0_5, src0_6, src0_7;
    v8i16 src1_0, src1_1, src1_2, src1_3, src1_4, src1_5, src1_6, src1_7;
    v4i32 src0_r, src1_r, src2_r, src3_r, src4_r, src5_r, src6_r, src7_r;
    v4i32 weight, round, offset;

    tmp = lvl_shift0 * wgt0;
    tmp += (lvl_shift1 * wgt1);
    tmp += (off0 + off1) << (shift - 1);
    offset = __msa_fill_w(tmp);
    wgt0 = (wgt0 << 16) | (wgt1 & 0x0000FFFF);
    weight = __msa_fill_w(wgt0);
    round = __msa_fill_w(shift);

    for(loop_cnt = (height >> 3); loop_cnt--;)
    {
        LD_SH8(src0, src0_stride, src0_0, src0_1, src0_2, src0_3,
               src0_4, src0_5, src0_6, src0_7);
        src0 += (8 * src0_stride);
        LD_SH8(src1, src1_stride, src1_0, src1_1, src1_2, src1_3,
               src1_4, src1_5, src1_6, src1_7);
        src1 += (8 * src1_stride);

        HEVC_BIW_RND_CLIP4_R(src0_0, src0_1, src0_2, src0_3, src1_0, src1_1,
                             src1_2, src1_3, weight, round, offset,
                             src0_r, src1_r, src2_r, src3_r);
        HEVC_BIW_RND_CLIP4_R(src0_4, src0_5, src0_6, src0_7, src1_4, src1_5,
                             src1_6, src1_7, weight, round, offset,
                             src4_r, src5_r, src6_r, src7_r);
        HEVC_PCK_SW_SB8(src1_r, src0_r, src3_r, src2_r, src5_r, src4_r, src7_r,
                        src6_r, src0_r, src1_r);
        ST4x8_UB(src0_r, src1_r, dst, dst_stride);
        dst += (8 * dst_stride);
    }
}

static void hevc_chroma_biwgt_4x8mult_msa(WORD16 *src0, WORD32 src0_stride,
                                          WORD16 *src1, WORD32 src1_stride,
                                          UWORD8 *dst, WORD32 dst_stride,
                                          WORD32 wgt0_cb, WORD32 wgt0_cr,
                                          WORD32 wgt1_cb, WORD32 wgt1_cr,
                                          WORD32 off0_cb, WORD32 off0_cr,
                                          WORD32 off1_cb, WORD32 off1_cr,
                                          WORD32 shift, WORD32 lvl_shift0,
                                          WORD32 lvl_shift1, WORD32 height)
{
    WORD32 loop_cnt, tmp;
    v8i16 src0_0, src0_1, src0_2, src0_3, src0_4, src0_5, src0_6, src0_7;
    v8i16 src1_0, src1_1, src1_2, src1_3, src1_4, src1_5, src1_6, src1_7;
    v4i32 src0_r, src1_r, src2_r, src3_r, src4_r, src5_r, src6_r, src7_r;
    v4i32 weight, round, offset, tmp_vec;

    tmp = lvl_shift0 * wgt0_cb;
    tmp += (lvl_shift1 * wgt1_cb);
    tmp += (off0_cb + off1_cb) << (shift - 1);
    offset = __msa_fill_w(tmp);
    tmp = lvl_shift0 * wgt0_cr;
    tmp += (lvl_shift1 * wgt1_cr);
    tmp += (off0_cr + off1_cr) << (shift - 1);
    tmp_vec = __msa_fill_w(tmp);
    offset = __msa_ilvr_w(tmp_vec, offset);
    tmp = (wgt0_cb << 16) | (wgt1_cb & 0x0000FFFF);
    weight = __msa_fill_w(tmp);
    tmp = (wgt0_cr << 16) | (wgt1_cr & 0x0000FFFF);
    tmp_vec = __msa_fill_w(tmp);
    weight = __msa_ilvr_w(tmp_vec, weight);
    round = __msa_fill_w(shift);

    for(loop_cnt = (height >> 3); loop_cnt--;)
    {
        LD_SH8(src0, src0_stride, src0_0, src0_1, src0_2, src0_3,
               src0_4, src0_5, src0_6, src0_7);
        src0 += (8 * src0_stride);
        LD_SH8(src1, src1_stride, src1_0, src1_1, src1_2, src1_3,
               src1_4, src1_5, src1_6, src1_7);
        src1 += (8 * src1_stride);

        HEVC_BIW_RND_CLIP4_R(src0_0, src0_1, src0_2, src0_3, src1_0, src1_1,
                             src1_2, src1_3, weight, round, offset,
                             src0_r, src1_r, src2_r, src3_r);
        HEVC_BIW_RND_CLIP4_R(src0_4, src0_5, src0_6, src0_7, src1_4, src1_5,
                             src1_6, src1_7, weight, round, offset,
                             src4_r, src5_r, src6_r, src7_r);
        HEVC_PCK_SW_SB8(src1_r, src0_r, src3_r, src2_r, src5_r, src4_r, src7_r,
                        src6_r, src0_r, src1_r);
        ST4x8_UB(src0_r, src1_r, dst, dst_stride);
        dst += (8 * dst_stride);
    }
}

static void hevc_chroma_biwgt_8x2mult_msa(WORD16 *src0, WORD32 src0_stride,
                                          WORD16 *src1, WORD32 src1_stride,
                                          UWORD8 *dst, WORD32 dst_stride,
                                          WORD32 wgt0_cb, WORD32 wgt0_cr,
                                          WORD32 wgt1_cb, WORD32 wgt1_cr,
                                          WORD32 off0_cb, WORD32 off0_cr,
                                          WORD32 off1_cb, WORD32 off1_cr,
                                          WORD32 shift, WORD32 lvl_shift0,
                                          WORD32 lvl_shift1)
{
    WORD32 tmp;
    v8i16 src0_0, src0_1, src1_0, src1_1;
    v4i32 src0_r, src1_r, src0_l, src1_l, weight, round, offset, tmp_vec;

    tmp = lvl_shift0 * wgt0_cb;
    tmp += (lvl_shift1 * wgt1_cb);
    tmp += (off0_cb + off1_cb) << (shift - 1);
    offset = __msa_fill_w(tmp);
    tmp = lvl_shift0 * wgt0_cr;
    tmp += (lvl_shift1 * wgt1_cr);
    tmp += (off0_cr + off1_cr) << (shift - 1);
    tmp_vec = __msa_fill_w(tmp);
    offset = __msa_ilvr_w(tmp_vec, offset);
    tmp = (wgt0_cb << 16) | (wgt1_cb & 0x0000FFFF);
    weight = __msa_fill_w(tmp);
    tmp = (wgt0_cr << 16) | (wgt1_cr & 0x0000FFFF);
    tmp_vec = __msa_fill_w(tmp);
    weight = __msa_ilvr_w(tmp_vec, weight);
    round = __msa_fill_w(shift);

    LD_SH2(src0, src0_stride, src0_0, src0_1);
    LD_SH2(src1, src1_stride, src1_0, src1_1);
    HEVC_BIW_RND_CLIP2(src0_0, src0_1, src1_0, src1_1, weight, round, offset,
                       src0_r, src1_r, src0_l, src1_l);
    HEVC_PCK_SW_SB4(src0_l, src0_r, src1_l, src1_r, src0_r);
    ST8x2_UB(src0_r, dst, dst_stride);
}

static void hevc_biwgt_8x4mult_msa(WORD16 *src0, WORD32 src0_stride,
                                   WORD16 *src1, WORD32 src1_stride,
                                   UWORD8 *dst, WORD32 dst_stride,
                                   WORD32 wgt0, WORD32 wgt1, WORD32 off0,
                                   WORD32 off1, WORD32 shift, WORD32 lvl_shift0,
                                   WORD32 lvl_shift1, WORD32 height)
{
    WORD32 loop_cnt, tmp;
    v8i16 src0_0, src0_1, src0_2, src0_3, src1_0, src1_1, src1_2, src1_3;
    v4i32 src0_r, src1_r, src2_r, src3_r, src0_l, src1_l, src2_l, src3_l;
    v4i32 weight, round, offset;

    tmp = lvl_shift0 * wgt0;
    tmp += (lvl_shift1 * wgt1);
    tmp += (off0 + off1) << (shift - 1);
    offset = __msa_fill_w(tmp);
    wgt0 = (wgt0 << 16) | (wgt1 & 0x0000FFFF);
    weight = __msa_fill_w(wgt0);
    round = __msa_fill_w(shift);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SH4(src0, src0_stride, src0_0, src0_1, src0_2, src0_3);
        src0 += (4 * src0_stride);
        LD_SH4(src1, src1_stride, src1_0, src1_1, src1_2, src1_3);
        src1 += (4 * src1_stride);
        HEVC_BIW_RND_CLIP4(src0_0, src0_1, src0_2, src0_3, src1_0, src1_1,
                           src1_2, src1_3, weight, round, offset, src0_r,
                           src1_r, src2_r, src3_r, src0_l, src1_l, src2_l,
                           src3_l);
        HEVC_PCK_SW_SB8(src0_l, src0_r, src1_l, src1_r, src2_l, src2_r, src3_l,
                        src3_r, src0_r, src1_r);
        ST8x4_UB(src0_r, src1_r, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_chroma_biwgt_8x4mult_msa(WORD16 *src0, WORD32 src0_stride,
                                          WORD16 *src1, WORD32 src1_stride,
                                          UWORD8 *dst, WORD32 dst_stride,
                                          WORD32 wgt0_cb, WORD32 wgt0_cr,
                                          WORD32 wgt1_cb, WORD32 wgt1_cr,
                                          WORD32 off0_cb, WORD32 off0_cr,
                                          WORD32 off1_cb, WORD32 off1_cr,
                                          WORD32 shift, WORD32 lvl_shift0,
                                          WORD32 lvl_shift1, WORD32 height)
{
    WORD32 loop_cnt, tmp;
    v8i16 src0_0, src0_1, src0_2, src0_3, src1_0, src1_1, src1_2, src1_3;
    v4i32 src0_r, src1_r, src2_r, src3_r, src0_l, src1_l, src2_l, src3_l;
    v4i32 weight, round, offset, tmp_vec;

    tmp = lvl_shift0 * wgt0_cb;
    tmp += (lvl_shift1 * wgt1_cb);
    tmp += (off0_cb + off1_cb) << (shift - 1);
    offset = __msa_fill_w(tmp);
    tmp = lvl_shift0 * wgt0_cr;
    tmp += (lvl_shift1 * wgt1_cr);
    tmp += (off0_cr + off1_cr) << (shift - 1);
    tmp_vec = __msa_fill_w(tmp);
    offset = __msa_ilvr_w(tmp_vec, offset);
    tmp = (wgt0_cb << 16) | (wgt1_cb & 0x0000FFFF);
    weight = __msa_fill_w(tmp);
    tmp = (wgt0_cr << 16) | (wgt1_cr & 0x0000FFFF);
    tmp_vec = __msa_fill_w(tmp);
    weight = __msa_ilvr_w(tmp_vec, weight);
    round = __msa_fill_w(shift);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SH4(src0, src0_stride, src0_0, src0_1, src0_2, src0_3);
        src0 += (4 * src0_stride);
        LD_SH4(src1, src1_stride, src1_0, src1_1, src1_2, src1_3);
        src1 += (4 * src1_stride);
        HEVC_BIW_RND_CLIP4(src0_0, src0_1, src0_2, src0_3, src1_0, src1_1,
                           src1_2, src1_3, weight, round, offset, src0_r,
                           src1_r, src2_r, src3_r, src0_l, src1_l, src2_l,
                           src3_l);
        HEVC_PCK_SW_SB8(src0_l, src0_r, src1_l, src1_r, src2_l, src2_r, src3_l,
                        src3_r, src0_r, src1_r);
        ST8x4_UB(src0_r, src1_r, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_biwgt_12x4mult_msa(WORD16 *src0, WORD32 src0_stride,
                                    WORD16 *src1, WORD32 src1_stride,
                                    UWORD8 *dst, WORD32 dst_stride,
                                    WORD32 wgt0, WORD32 wgt1, WORD32 off0,
                                    WORD32 off1, WORD32 shift,
                                    WORD32 lvl_shift0, WORD32 lvl_shift1,
                                    WORD32 height)
{
    WORD32 loop_cnt, tmp;
    v8i16 src0_0, src0_1, src0_2, src0_3, src0_4, src0_5, src0_6, src0_7;
    v8i16 src1_0, src1_1, src1_2, src1_3, src1_4, src1_5, src1_6, src1_7;
    v4i32 src0_r, src1_r, src2_r, src3_r, src4_r, src5_r, src6_r, src7_r;
    v4i32 src0_l, src1_l, src2_l, src3_l, weight, round, offset;

    tmp = lvl_shift0 * wgt0;
    tmp += (lvl_shift1 * wgt1);
    tmp += (off0 + off1) << (shift - 1);
    offset = __msa_fill_w(tmp);
    wgt0 = (wgt0 << 16) | (wgt1 & 0x0000FFFF);
    weight = __msa_fill_w(wgt0);
    round = __msa_fill_w(shift);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SH4(src0, src0_stride, src0_0, src0_1, src0_2, src0_3);
        LD_SH4(src0 + 8, src0_stride, src0_4, src0_5, src0_6, src0_7);
        src0 += (4 * src0_stride);
        LD_SH4(src1, src1_stride, src1_0, src1_1, src1_2, src1_3);
        LD_SH4(src1 + 8, src1_stride, src1_4, src1_5, src1_6, src1_7);
        src1 += (4 * src1_stride);
        HEVC_BIW_RND_CLIP4(src0_0, src0_1, src0_2, src0_3, src1_0, src1_1,
                           src1_2, src1_3, weight, round, offset, src0_r,
                           src1_r, src2_r, src3_r, src0_l, src1_l, src2_l,
                           src3_l);
        HEVC_BIW_RND_CLIP4_R(src0_4, src0_5, src0_6, src0_7, src1_4, src1_5,
                             src1_6, src1_7, weight, round, offset,
                             src4_r, src5_r, src6_r, src7_r);
        HEVC_PCK_SW_SB12(src0_l, src0_r, src1_l, src1_r, src2_l, src2_r,
                         src3_l, src3_r, src5_r, src4_r, src7_r, src6_r,
                         src0_r, src1_r, src2_r);
        ST12x4_UB(src0_r, src1_r, src2_r, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_chroma_biwgt_12x4mult_msa(WORD16 *src0, WORD32 src0_stride,
                                           WORD16 *src1, WORD32 src1_stride,
                                           UWORD8 *dst, WORD32 dst_stride,
                                           WORD32 wgt0_cb, WORD32 wgt0_cr,
                                           WORD32 wgt1_cb, WORD32 wgt1_cr,
                                           WORD32 off0_cb, WORD32 off0_cr,
                                           WORD32 off1_cb, WORD32 off1_cr,
                                           WORD32 shift, WORD32 lvl_shift0,
                                           WORD32 lvl_shift1, WORD32 height)
{
    WORD32 loop_cnt, tmp;
    v8i16 src0_0, src0_1, src0_2, src0_3, src0_4, src0_5, src0_6, src0_7;
    v8i16 src1_0, src1_1, src1_2, src1_3, src1_4, src1_5, src1_6, src1_7;
    v4i32 src0_r, src1_r, src2_r, src3_r, src4_r, src5_r, src6_r, src7_r;
    v4i32 src0_l, src1_l, src2_l, src3_l, weight, round, offset, tmp_vec;

    tmp = lvl_shift0 * wgt0_cb;
    tmp += (lvl_shift1 * wgt1_cb);
    tmp += (off0_cb + off1_cb) << (shift - 1);
    offset = __msa_fill_w(tmp);
    tmp = lvl_shift0 * wgt0_cr;
    tmp += (lvl_shift1 * wgt1_cr);
    tmp += (off0_cr + off1_cr) << (shift - 1);
    tmp_vec = __msa_fill_w(tmp);
    offset = __msa_ilvr_w(tmp_vec, offset);
    tmp = (wgt0_cb << 16) | (wgt1_cb & 0x0000FFFF);
    weight = __msa_fill_w(tmp);
    tmp = (wgt0_cr << 16) | (wgt1_cr & 0x0000FFFF);
    tmp_vec = __msa_fill_w(tmp);
    weight = __msa_ilvr_w(tmp_vec, weight);
    round = __msa_fill_w(shift);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SH4(src0, src0_stride, src0_0, src0_1, src0_2, src0_3);
        LD_SH4(src0 + 8, src0_stride, src0_4, src0_5, src0_6, src0_7);
        src0 += (4 * src0_stride);
        LD_SH4(src1, src1_stride, src1_0, src1_1, src1_2, src1_3);
        LD_SH4(src1 + 8, src1_stride, src1_4, src1_5, src1_6, src1_7);
        src1 += (4 * src1_stride);
        HEVC_BIW_RND_CLIP4(src0_0, src0_1, src0_2, src0_3, src1_0, src1_1,
                           src1_2, src1_3, weight, round, offset, src0_r,
                           src1_r, src2_r, src3_r, src0_l, src1_l, src2_l,
                           src3_l);
        HEVC_BIW_RND_CLIP4_R(src0_4, src0_5, src0_6, src0_7, src1_4, src1_5,
                             src1_6, src1_7, weight, round, offset,
                             src4_r, src5_r, src6_r, src7_r);
        HEVC_PCK_SW_SB12(src0_l, src0_r, src1_l, src1_r, src2_l, src2_r,
                         src3_l, src3_r, src5_r, src4_r, src7_r, src6_r,
                         src0_r, src1_r, src2_r);
        ST12x4_UB(src0_r, src1_r, src2_r, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_chroma_biwgt_16x2mult_msa(WORD16 *src0, WORD32 src0_stride,
                                           WORD16 *src1, WORD32 src1_stride,
                                           UWORD8 *dst, WORD32 dst_stride,
                                           WORD32 wgt0_cb, WORD32 wgt0_cr,
                                           WORD32 wgt1_cb, WORD32 wgt1_cr,
                                           WORD32 off0_cb, WORD32 off0_cr,
                                           WORD32 off1_cb, WORD32 off1_cr,
                                           WORD32 shift, WORD32 lvl_shift0,
                                           WORD32 lvl_shift1, WORD32 height)
{
    WORD32 loop_cnt, tmp;
    v8i16 src0_0, src0_1, src0_2, src0_3, src1_0, src1_1, src1_2, src1_3;
    v4i32 src0_r, src1_r, src2_r, src3_r, src0_l, src1_l, src2_l, src3_l;
    v4i32 weight, round, offset, tmp_vec;

    tmp = lvl_shift0 * wgt0_cb;
    tmp += (lvl_shift1 * wgt1_cb);
    tmp += (off0_cb + off1_cb) << (shift - 1);
    offset = __msa_fill_w(tmp);
    tmp = lvl_shift0 * wgt0_cr;
    tmp += (lvl_shift1 * wgt1_cr);
    tmp += (off0_cr + off1_cr) << (shift - 1);
    tmp_vec = __msa_fill_w(tmp);
    offset = __msa_ilvr_w(tmp_vec, offset);
    tmp = (wgt0_cb << 16) | (wgt1_cb & 0x0000FFFF);
    weight = __msa_fill_w(tmp);
    tmp = (wgt0_cr << 16) | (wgt1_cr & 0x0000FFFF);
    tmp_vec = __msa_fill_w(tmp);
    weight = __msa_ilvr_w(tmp_vec, weight);
    round = __msa_fill_w(shift);

    for(loop_cnt = (height >> 1); loop_cnt--;)
    {
        LD_SH2(src0, src0_stride, src0_0, src0_1);
        LD_SH2(src0 + 8, src0_stride, src0_2, src0_3);
        src0 += (2 * src0_stride);
        LD_SH2(src1, src1_stride, src1_0, src1_1);
        LD_SH2(src1 + 8, src1_stride, src1_2, src1_3);
        src1 += (2 * src1_stride);
        HEVC_BIW_RND_CLIP4(src0_0, src0_1, src0_2, src0_3, src1_0, src1_1,
                           src1_2, src1_3, weight, round, offset, src0_r,
                           src1_r, src2_r, src3_r, src0_l, src1_l, src2_l,
                           src3_l);
        HEVC_PCK_SW_SB8(src0_l, src0_r, src2_l, src2_r, src1_l, src1_r, src3_l,
                        src3_r, src0_r, src1_r);
        ST_SW2(src0_r, src1_r, dst, dst_stride);
        dst += (2 * dst_stride);
    }
}

static void hevc_biwgt_16multx4mult_msa(WORD16 *src0, WORD32 src0_stride,
                                        WORD16 *src1, WORD32 src1_stride,
                                        UWORD8 *dst, WORD32 dst_stride,
                                        WORD32 wgt0, WORD32 wgt1,
                                        WORD32 off0, WORD32 off1,
                                        WORD32 shift, WORD32 lvl_shift0,
                                        WORD32 lvl_shift1, WORD32 height,
                                        WORD32 width)
{
    WORD32 tmp, loop_cnt, cnt;
    WORD16 *src0_tp, *src1_tp;
    UWORD8 *dst_tp;
    v8i16 src0_0, src0_1, src0_2, src0_3, src0_4, src0_5, src0_6, src0_7;
    v8i16 src1_0, src1_1, src1_2, src1_3, src1_4, src1_5, src1_6, src1_7;
    v4i32 src0_r, src1_r, src2_r, src3_r, src4_r, src5_r, src6_r, src7_r;
    v4i32 src0_l, src1_l, src2_l, src3_l, src4_l, src5_l, src6_l, src7_l;
    v4i32 round, weight, offset;

    tmp = lvl_shift0 * wgt0;
    tmp += (lvl_shift1 * wgt1);
    tmp += (off0 + off1) << (shift - 1);
    offset = __msa_fill_w(tmp);
    wgt0 = (wgt0 << 16) | (wgt1 & 0x0000FFFF);
    weight = __msa_fill_w(wgt0);
    round = __msa_fill_w(shift);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        src0_tp = src0;
        src1_tp = src1;
        dst_tp = dst;

        for(cnt = (width >> 4); cnt--;)
        {
            LD_SH4(src0_tp, src0_stride, src0_0, src0_1, src0_2, src0_3);
            LD_SH4(src0_tp + 8, src0_stride, src0_4, src0_5, src0_6, src0_7);
            src0_tp += 16;
            LD_SH4(src1_tp, src1_stride, src1_0, src1_1, src1_2, src1_3);
            LD_SH4(src1_tp + 8, src1_stride, src1_4, src1_5, src1_6, src1_7);
            src1_tp += 16;
            HEVC_BIW_RND_CLIP4(src0_0, src0_1, src0_2, src0_3, src1_0,
                               src1_1, src1_2, src1_3, weight, round,
                               offset, src0_r, src1_r, src2_r, src3_r,
                               src0_l, src1_l, src2_l, src3_l);
            HEVC_BIW_RND_CLIP4(src0_4, src0_5, src0_6, src0_7, src1_4,
                               src1_5, src1_6, src1_7, weight, round,
                               offset, src4_r, src5_r, src6_r, src7_r,
                               src4_l, src5_l, src6_l, src7_l);
            HEVC_PCK_SW_SB8(src0_l, src0_r, src4_l, src4_r, src1_l, src1_r,
                            src5_l, src5_r, src0_r, src1_r);
            HEVC_PCK_SW_SB8(src2_l, src2_r, src6_l, src6_r, src3_l, src3_r,
                            src7_l, src7_r, src2_r, src3_r);
            ST_SW2(src0_r, src1_r, dst_tp, dst_stride);
            ST_SW2(src2_r, src3_r, dst_tp + 2 * dst_stride, dst_stride);
            dst_tp += 16;
        }

        src0 += (4 * src0_stride);
        src1 += (4 * src1_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_chroma_biwgt_16multx4mult_msa(WORD16 *src0, WORD32 src0_stride,
                                               WORD16 *src1, WORD32 src1_stride,
                                               UWORD8 *dst, WORD32 dst_stride,
                                               WORD32 wgt0_cb, WORD32 wgt0_cr,
                                               WORD32 wgt1_cb, WORD32 wgt1_cr,
                                               WORD32 off0_cb, WORD32 off0_cr,
                                               WORD32 off1_cb, WORD32 off1_cr,
                                               WORD32 shift, WORD32 lvl_shift0,
                                               WORD32 lvl_shift1, WORD32 height,
                                               WORD32 width)
{
    WORD32 tmp, loop_cnt, cnt;
    WORD16 *src0_tp, *src1_tp;
    UWORD8 *dst_tp;
    v8i16 src0_0, src0_1, src0_2, src0_3, src0_4, src0_5, src0_6, src0_7;
    v8i16 src1_0, src1_1, src1_2, src1_3, src1_4, src1_5, src1_6, src1_7;
    v4i32 src0_r, src1_r, src2_r, src3_r, src4_r, src5_r, src6_r, src7_r;
    v4i32 src0_l, src1_l, src2_l, src3_l, src4_l, src5_l, src6_l, src7_l;
    v4i32 round, weight, offset, tmp_vec;

    tmp = lvl_shift0 * wgt0_cb;
    tmp += (lvl_shift1 * wgt1_cb);
    tmp += (off0_cb + off1_cb) << (shift - 1);
    offset = __msa_fill_w(tmp);
    tmp = lvl_shift0 * wgt0_cr;
    tmp += (lvl_shift1 * wgt1_cr);
    tmp += (off0_cr + off1_cr) << (shift - 1);
    tmp_vec = __msa_fill_w(tmp);
    offset = __msa_ilvr_w(tmp_vec, offset);
    tmp = (wgt0_cb << 16) | (wgt1_cb & 0x0000FFFF);
    weight = __msa_fill_w(tmp);
    tmp = (wgt0_cr << 16) | (wgt1_cr & 0x0000FFFF);
    tmp_vec = __msa_fill_w(tmp);
    weight = __msa_ilvr_w(tmp_vec, weight);
    round = __msa_fill_w(shift);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        src0_tp = src0;
        src1_tp = src1;
        dst_tp = dst;

        for(cnt = (width >> 4); cnt--;)
        {
            LD_SH4(src0_tp, src0_stride, src0_0, src0_1, src0_2, src0_3);
            LD_SH4(src0_tp + 8, src0_stride, src0_4, src0_5, src0_6, src0_7);
            src0_tp += 16;
            LD_SH4(src1_tp, src1_stride, src1_0, src1_1, src1_2, src1_3);
            LD_SH4(src1_tp + 8, src1_stride, src1_4, src1_5, src1_6, src1_7);
            src1_tp += 16;
            HEVC_BIW_RND_CLIP4(src0_0, src0_1, src0_2, src0_3, src1_0,
                               src1_1, src1_2, src1_3, weight, round,
                               offset, src0_r, src1_r, src2_r, src3_r,
                               src0_l, src1_l, src2_l, src3_l);
            HEVC_BIW_RND_CLIP4(src0_4, src0_5, src0_6, src0_7, src1_4,
                               src1_5, src1_6, src1_7, weight, round,
                               offset, src4_r, src5_r, src6_r, src7_r,
                               src4_l, src5_l, src6_l, src7_l);
            HEVC_PCK_SW_SB8(src0_l, src0_r, src4_l, src4_r, src1_l, src1_r,
                            src5_l, src5_r, src0_r, src1_r);
            HEVC_PCK_SW_SB8(src2_l, src2_r, src6_l, src6_r, src3_l, src3_r,
                            src7_l, src7_r, src2_r, src3_r);
            ST_SW2(src0_r, src1_r, dst_tp, dst_stride);
            ST_SW2(src2_r, src3_r, dst_tp + 2 * dst_stride, dst_stride);
            dst_tp += 16;
        }

        src0 += (4 * src0_stride);
        src1 += (4 * src1_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_bi_4x8mult_msa(WORD16 *src0, WORD32 src0_stride,
                                WORD16 *src1, WORD32 src1_stride,
                                UWORD8 *dst, WORD32 dst_stride,
                                WORD32 lvl_shift0, WORD32 lvl_shift1,
                                WORD32 height)
{
    WORD32 loop_cnt;
    v8i16 src0_0, src0_1, src0_2, src0_3, src0_4, src0_5, src0_6, src0_7;
    v8i16 src1_0, src1_1, src1_2, src1_3, src1_4, src1_5, src1_6, src1_7;
    v8i16 offset;

    lvl_shift0 += lvl_shift1;
    offset = __msa_fill_h(lvl_shift0);

    for(loop_cnt = (height >> 3); loop_cnt--;)
    {
        LD_SH8(src0, src0_stride, src0_0, src0_1, src0_2, src0_3,
               src0_4, src0_5, src0_6, src0_7);
        src0 += (8 * src0_stride);
        LD_SH8(src1, src1_stride, src1_0, src1_1, src1_2, src1_3,
               src1_4, src1_5, src1_6, src1_7);
        src1 += (8 * src1_stride);

        PCKEV_D2_SH(src0_1, src0_0, src0_3, src0_2, src0_0, src0_1);
        PCKEV_D2_SH(src0_5, src0_4, src0_7, src0_6, src0_2, src0_3);
        PCKEV_D2_SH(src1_1, src1_0, src1_3, src1_2, src1_0, src1_1);
        PCKEV_D2_SH(src1_5, src1_4, src1_7, src1_6, src1_2, src1_3);
        ADDS_SH4_SH(src0_0, offset, src0_1, offset, src0_2, offset, src0_3,
                    offset, src0_0, src0_1, src0_2, src0_3);
        HEVC_BI_RND_CLIP4(src0_0, src0_1, src0_2, src0_3, src1_0, src1_1,
                          src1_2, src1_3, 7, src0_0, src0_1, src0_2, src0_3);
        PCKEV_B2_SH(src0_1, src0_0, src0_3, src0_2, src0_0, src0_1);
        ST4x8_UB(src0_0, src0_1, dst, dst_stride);
        dst += (8 * dst_stride);
    }
}

static void hevc_chroma_bi_4x4_msa(WORD16 *src0, WORD32 src0_stride,
                                   WORD16 *src1, WORD32 src1_stride,
                                   UWORD8 *dst, WORD32 dst_stride)
{
    v8i16 src0_0, src0_1, src0_2, src0_3, src1_0, src1_1, src1_2, src1_3;

    LD_SH4(src0, src0_stride, src0_0, src0_1, src0_2, src0_3);
    LD_SH4(src1, src1_stride, src1_0, src1_1, src1_2, src1_3);
    PCKEV_D2_SH(src0_1, src0_0, src0_3, src0_2, src0_0, src0_1);
    PCKEV_D2_SH(src1_1, src1_0, src1_3, src1_2, src1_0, src1_1);
    HEVC_BI_RND_CLIP2(src0_0, src0_1, src1_0, src1_1, 7, src0_0, src0_1);
    src0_0 = (v8i16) __msa_pckev_b((v16i8) src0_1, (v16i8) src0_0);
    ST4x4_UB(src0_0, src0_0, 0, 1, 2, 3, dst, dst_stride);
}

static void hevc_chroma_bi_4x8mult_msa(WORD16 *src0, WORD32 src0_stride,
                                       WORD16 *src1, WORD32 src1_stride,
                                       UWORD8 *dst, WORD32 dst_stride,
                                       WORD32 height)
{
    WORD32 loop_cnt;
    v8i16 src0_0, src0_1, src0_2, src0_3, src0_4, src0_5, src0_6, src0_7;
    v8i16 src1_0, src1_1, src1_2, src1_3, src1_4, src1_5, src1_6, src1_7;

    for(loop_cnt = (height >> 3); loop_cnt--;)
    {
        LD_SH8(src0, src0_stride, src0_0, src0_1, src0_2, src0_3,
               src0_4, src0_5, src0_6, src0_7);
        src0 += (8 * src0_stride);
        LD_SH8(src1, src1_stride, src1_0, src1_1, src1_2, src1_3,
               src1_4, src1_5, src1_6, src1_7);
        src1 += (8 * src1_stride);

        PCKEV_D2_SH(src0_1, src0_0, src0_3, src0_2, src0_0, src0_1);
        PCKEV_D2_SH(src0_5, src0_4, src0_7, src0_6, src0_2, src0_3);
        PCKEV_D2_SH(src1_1, src1_0, src1_3, src1_2, src1_0, src1_1);
        PCKEV_D2_SH(src1_5, src1_4, src1_7, src1_6, src1_2, src1_3);
        HEVC_BI_RND_CLIP4(src0_0, src0_1, src0_2, src0_3, src1_0, src1_1,
                          src1_2, src1_3, 7, src0_0, src0_1, src0_2, src0_3);
        PCKEV_B2_SH(src0_1, src0_0, src0_3, src0_2, src0_0, src0_1);
        ST4x8_UB(src0_0, src0_1, dst, dst_stride);
        dst += (8 * dst_stride);
    }
}

static void hevc_chroma_bi_8x2mult_msa(WORD16 *src0, WORD32 src0_stride,
                                       WORD16 *src1, WORD32 src1_stride,
                                       UWORD8 *dst, WORD32 dst_stride,
                                       WORD32 height)
{
    WORD32 loop_cnt;
    v8i16 src0_0, src0_1, src1_0, src1_1;

    for(loop_cnt = (height >> 1); loop_cnt--;)
    {
        LD_SH2(src0, src0_stride, src0_0, src0_1);
        src0 += (2 * src0_stride);
        LD_SH2(src1, src1_stride, src1_0, src1_1);
        src1 += (2 * src1_stride);
        HEVC_BI_RND_CLIP2(src0_0, src0_1, src1_0, src1_1, 7, src0_0, src0_1);
        src0_0 = (v8i16) __msa_pckev_b((v16i8) src0_1, (v16i8) src0_0);
        ST8x2_UB(src0_0, dst, dst_stride);
        dst += (2 * dst_stride);
    }
}

static void hevc_bi_8x4mult_msa(WORD16 *src0, WORD32 src0_stride,
                                WORD16 *src1, WORD32 src1_stride,
                                UWORD8 *dst, WORD32 dst_stride,
                                WORD32 lvl_shift0, WORD32 lvl_shift1,
                                WORD32 height)
{
    WORD32 loop_cnt;
    v8i16 src0_0, src0_1, src0_2, src0_3, src1_0, src1_1, src1_2, src1_3;
    v8i16 offset;

    lvl_shift0 += lvl_shift1;
    offset = __msa_fill_h(lvl_shift0);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SH4(src0, src0_stride, src0_0, src0_1, src0_2, src0_3);
        src0 += (4 * src0_stride);
        LD_SH4(src1, src1_stride, src1_0, src1_1, src1_2, src1_3);
        src1 += (4 * src1_stride);
        ADDS_SH4_SH(src0_0, offset, src0_1, offset, src0_2, offset, src0_3,
                    offset, src0_0, src0_1, src0_2, src0_3);
        HEVC_BI_RND_CLIP4(src0_0, src0_1, src0_2, src0_3, src1_0, src1_1,
                          src1_2, src1_3, 7, src0_0, src0_1, src0_2, src0_3);
        PCKEV_B2_SH(src0_1, src0_0, src0_3, src0_2, src0_0, src0_1);
        ST8x4_UB(src0_0, src0_1, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_chroma_bi_8x4mult_msa(WORD16 *src0, WORD32 src0_stride,
                                       WORD16 *src1, WORD32 src1_stride,
                                       UWORD8 *dst, WORD32 dst_stride,
                                       WORD32 height)
{
    WORD32 loop_cnt;
    v8i16 src0_0, src0_1, src0_2, src0_3, src1_0, src1_1, src1_2, src1_3;

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SH4(src0, src0_stride, src0_0, src0_1, src0_2, src0_3);
        src0 += (4 * src0_stride);
        LD_SH4(src1, src1_stride, src1_0, src1_1, src1_2, src1_3);
        src1 += (4 * src1_stride);

        HEVC_BI_RND_CLIP4(src0_0, src0_1, src0_2, src0_3, src1_0, src1_1,
                          src1_2, src1_3, 7, src0_0, src0_1, src0_2, src0_3);
        PCKEV_B2_SH(src0_1, src0_0, src0_3, src0_2, src0_0, src0_1);
        ST8x4_UB(src0_0, src0_1, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_bi_12x4mult_msa(WORD16 *src0, WORD32 src0_stride,
                                 WORD16 *src1, WORD32 src1_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD32 lvl_shift0, WORD32 lvl_shift1,
                                 WORD32 height)
{
    WORD32 loop_cnt;
    v8i16 src0_0, src0_1, src0_2, src0_3, src0_4, src0_5, src0_6, src0_7;
    v8i16 src1_0, src1_1, src1_2, src1_3, src1_4, src1_5, src1_6, src1_7;
    v8i16 offset;

    lvl_shift0 += lvl_shift1;
    offset = __msa_fill_h(lvl_shift0);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SH4(src0, src0_stride, src0_0, src0_1, src0_2, src0_3);
        LD_SH4(src0 + 8, src0_stride, src0_4, src0_5, src0_6, src0_7);
        src0 += (4 * src0_stride);
        LD_SH4(src1, src1_stride, src1_0, src1_1, src1_2, src1_3);
        LD_SH4(src1 + 8, src1_stride, src1_4, src1_5, src1_6, src1_7);
        src1 += (4 * src1_stride);

        PCKEV_D2_SH(src0_5, src0_4, src0_7, src0_6, src0_4, src0_5);
        PCKEV_D2_SH(src1_5, src1_4, src1_7, src1_6, src1_4, src1_5);
        ADDS_SH4_SH(src0_0, offset, src0_1, offset, src0_2, offset, src0_3,
                    offset, src0_0, src0_1, src0_2, src0_3);
        HEVC_BI_RND_CLIP4(src0_0, src0_1, src0_2, src0_3, src1_0, src1_1,
                          src1_2, src1_3, 7, src0_0, src0_1, src0_2, src0_3);
        ADDS_SH2_SH(src0_4, offset, src0_5, offset, src0_4, src0_5);
        HEVC_BI_RND_CLIP2(src0_4, src0_5, src1_4, src1_5, 7, src0_4, src0_5);
        PCKEV_B2_SH(src0_1, src0_0, src0_3, src0_2, src0_0, src0_1);
        src0_4 = (v8i16) __msa_pckev_b((v16i8) src0_5, (v16i8) src0_4);
        ST12x4_UB(src0_0, src0_1, src0_4, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_chroma_bi_12x4mult_msa(WORD16 *src0, WORD32 src0_stride,
                                        WORD16 *src1, WORD32 src1_stride,
                                        UWORD8 *dst, WORD32 dst_stride,
                                        WORD32 height)
{
    WORD32 loop_cnt;
    v8i16 src0_0, src0_1, src0_2, src0_3, src0_4, src0_5, src0_6, src0_7;
    v8i16 src1_0, src1_1, src1_2, src1_3, src1_4, src1_5, src1_6, src1_7;

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        LD_SH4(src0, src0_stride, src0_0, src0_1, src0_2, src0_3);
        LD_SH4(src0 + 8, src0_stride, src0_4, src0_5, src0_6, src0_7);
        src0 += (4 * src0_stride);
        LD_SH4(src1, src1_stride, src1_0, src1_1, src1_2, src1_3);
        LD_SH4(src1 + 8, src1_stride, src1_4, src1_5, src1_6, src1_7);
        src1 += (4 * src1_stride);

        PCKEV_D2_SH(src0_5, src0_4, src0_7, src0_6, src0_4, src0_5);
        PCKEV_D2_SH(src1_5, src1_4, src1_7, src1_6, src1_4, src1_5);
        HEVC_BI_RND_CLIP4(src0_0, src0_1, src0_2, src0_3, src1_0, src1_1,
                          src1_2, src1_3, 7, src0_0, src0_1, src0_2, src0_3);
        HEVC_BI_RND_CLIP2(src0_4, src0_5, src1_4, src1_5, 7, src0_4, src0_5);
        PCKEV_B2_SH(src0_1, src0_0, src0_3, src0_2, src0_0, src0_1);
        src0_4 = (v8i16) __msa_pckev_b((v16i8) src0_5, (v16i8) src0_4);
        ST12x4_UB(src0_0, src0_1, src0_4, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_bi_16x2mult_msa(WORD16 *src0, WORD32 src0_stride,
                                 WORD16 *src1, WORD32 src1_stride,
                                 UWORD8 *dst, WORD32 dst_stride,
                                 WORD32 lvl_shift0, WORD32 lvl_shift1,
                                 WORD32 height)
{
    WORD32 loop_cnt;
    v8i16 src0_0, src0_1, src0_2, src0_3, src1_0, src1_1, src1_2, src1_3;
    v8i16 offset;

    lvl_shift0 += lvl_shift1;
    offset = __msa_fill_h(lvl_shift0);

    for(loop_cnt = (height >> 1); loop_cnt--;)
    {
        LD_SH2(src0, src0_stride, src0_0, src0_1);
        LD_SH2(src0 + 8, src0_stride, src0_2, src0_3);
        src0 += (2 * src0_stride);
        LD_SH2(src1, src1_stride, src1_0, src1_1);
        LD_SH2(src1 + 8, src1_stride, src1_2, src1_3);
        src1 += (2 * src1_stride);

        ADDS_SH4_SH(src0_0, offset, src0_1, offset, src0_2, offset, src0_3,
                    offset, src0_0, src0_1, src0_2, src0_3);
        HEVC_BI_RND_CLIP4(src0_0, src0_1, src0_2, src0_3, src1_0, src1_1,
                          src1_2, src1_3, 7, src0_0, src0_1, src0_2, src0_3);
        PCKEV_B2_SH(src0_2, src0_0, src0_3, src0_1, src0_0, src0_1);
        ST_SH2(src0_0, src0_1, dst, dst_stride);
        dst += (2 * dst_stride);
    }
}

static void hevc_chroma_bi_16x2mult_msa(WORD16 *src0, WORD32 src0_stride,
                                        WORD16 *src1, WORD32 src1_stride,
                                        UWORD8 *dst, WORD32 dst_stride,
                                        WORD32 height)
{
    WORD32 loop_cnt;
    v8i16 src0_0, src0_1, src0_2, src0_3, src1_0, src1_1, src1_2, src1_3;

    for(loop_cnt = (height >> 1); loop_cnt--;)
    {
        LD_SH2(src0, src0_stride, src0_0, src0_1);
        LD_SH2(src0 + 8, src0_stride, src0_2, src0_3);
        src0 += (2 * src0_stride);
        LD_SH2(src1, src1_stride, src1_0, src1_1);
        LD_SH2(src1 + 8, src1_stride, src1_2, src1_3);
        src1 += (2 * src1_stride);

        HEVC_BI_RND_CLIP4(src0_0, src0_1, src0_2, src0_3, src1_0, src1_1,
                          src1_2, src1_3, 7, src0_0, src0_1, src0_2, src0_3);
        PCKEV_B2_SH(src0_2, src0_0, src0_3, src0_1, src0_0, src0_1);
        ST_SH2(src0_0, src0_1, dst, dst_stride);
        dst += (2 * dst_stride);
    }
}

static void hevc_bi_16multx4mult_msa(WORD16 *src0, WORD32 src0_stride,
                                     WORD16 *src1, WORD32 src1_stride,
                                     UWORD8 *dst, WORD32 dst_stride,
                                     WORD32 lvl_shift0, WORD32 lvl_shift1,
                                     WORD32 height, WORD32 width)
{
    WORD32 loop_cnt, cnt;
    WORD16 *src0_tmp, *src1_tmp;
    UWORD8 *dst_tmp;
    v8i16 src0_0, src0_1, src0_2, src0_3, src0_4, src0_5, src0_6, src0_7;
    v8i16 src1_0, src1_1, src1_2, src1_3, src1_4, src1_5, src1_6, src1_7;
    v8i16 offset;

    lvl_shift0 += lvl_shift1;
    offset = __msa_fill_h(lvl_shift0);

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        src0_tmp = src0;
        src1_tmp = src1;
        dst_tmp = dst;

        for(cnt = (width >> 4); cnt--;)
        {
            LD_SH4(src0_tmp, src0_stride, src0_0, src0_1, src0_2, src0_3);
            LD_SH4(src0_tmp + 8, src0_stride, src0_4, src0_5, src0_6, src0_7);
            src0_tmp += 16;
            LD_SH4(src1_tmp, src1_stride, src1_0, src1_1, src1_2, src1_3);
            LD_SH4(src1_tmp + 8, src1_stride, src1_4, src1_5, src1_6, src1_7);
            src1_tmp += 16;

            ADDS_SH4_SH(src0_0, offset, src0_1, offset, src0_2, offset, src0_3,
                        offset, src0_0, src0_1, src0_2, src0_3);
            ADDS_SH4_SH(src0_4, offset, src0_5, offset, src0_6, offset, src0_7,
                        offset, src0_4, src0_5, src0_6, src0_7);
            HEVC_BI_RND_CLIP4(src0_0, src0_1, src0_2, src0_3, src1_0, src1_1,
                              src1_2, src1_3, 7, src0_0, src0_1, src0_2,
                              src0_3);
            HEVC_BI_RND_CLIP4(src0_4, src0_5, src0_6, src0_7, src1_4, src1_5,
                              src1_6, src1_7, 7, src0_4, src0_5, src0_6,
                              src0_7);
            PCKEV_B2_SH(src0_4, src0_0, src0_5, src0_1, src0_0, src0_1);
            PCKEV_B2_SH(src0_6, src0_2, src0_7, src0_3, src0_2, src0_3);
            ST_SH4(src0_0, src0_1, src0_2, src0_3, dst_tmp, dst_stride);
            dst_tmp += 16;
        }

        src0 += (4 * src0_stride);
        src1 += (4 * src1_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_chroma_bi_16multx4mult_msa(WORD16 *src0, WORD32 src0_stride,
                                            WORD16 *src1, WORD32 src1_stride,
                                            UWORD8 *dst, WORD32 dst_stride,
                                            WORD32 height, WORD32 width)
{
    WORD32 loop_cnt, cnt;
    WORD16 *src0_tmp, *src1_tmp;
    UWORD8 *dst_tmp;
    v8i16 src0_0, src0_1, src0_2, src0_3, src0_4, src0_5, src0_6, src0_7;
    v8i16 src1_0, src1_1, src1_2, src1_3, src1_4, src1_5, src1_6, src1_7;

    for(loop_cnt = (height >> 2); loop_cnt--;)
    {
        src0_tmp = src0;
        src1_tmp = src1;
        dst_tmp = dst;

        for(cnt = (width >> 4); cnt--;)
        {
            LD_SH4(src0_tmp, src0_stride, src0_0, src0_1, src0_2, src0_3);
            LD_SH4(src0_tmp + 8, src0_stride, src0_4, src0_5, src0_6, src0_7);
            src0_tmp += 16;
            LD_SH4(src1_tmp, src1_stride, src1_0, src1_1, src1_2, src1_3);
            LD_SH4(src1_tmp + 8, src1_stride, src1_4, src1_5, src1_6, src1_7);
            src1_tmp += 16;

            HEVC_BI_RND_CLIP4(src0_0, src0_1, src0_2, src0_3, src1_0, src1_1,
                              src1_2, src1_3, 7, src0_0, src0_1, src0_2,
                              src0_3);
            HEVC_BI_RND_CLIP4(src0_4, src0_5, src0_6, src0_7, src1_4, src1_5,
                              src1_6, src1_7, 7, src0_4, src0_5, src0_6,
                              src0_7);
            PCKEV_B2_SH(src0_4, src0_0, src0_5, src0_1, src0_0, src0_1);
            PCKEV_B2_SH(src0_6, src0_2, src0_7, src0_3, src0_2, src0_3);
            ST_SH4(src0_0, src0_1, src0_2, src0_3, dst_tmp, dst_stride);
            dst_tmp += 16;
        }

        src0 += (4 * src0_stride);
        src1 += (4 * src1_stride);
        dst += (4 * dst_stride);
    }
}

void ihevc_weighted_pred_uni_msa(WORD16 *src, UWORD8 *dst, WORD32 src_stride,
                                 WORD32 dst_stride, WORD32 wgt0, WORD32 off0,
                                 WORD32 shift, WORD32 lvl_shift, WORD32 ht,
                                 WORD32 wd)
{
    switch(wd)
    {
        case 4:
            hevc_uniwgt_4x8mult_msa(src, src_stride, dst, dst_stride,
                                    wgt0, off0, shift, lvl_shift, ht);
            break;
        case 8:
            hevc_uniwgt_8x4mult_msa(src, src_stride, dst, dst_stride,
                                    wgt0, off0, shift, lvl_shift, ht);
            break;
        case 12:
            hevc_uniwgt_12x4mult_msa(src, src_stride, dst, dst_stride,
                                     wgt0, off0, shift, lvl_shift, ht);
            break;
        case 24:
            hevc_uniwgt_16multx4mult_msa(src, src_stride, dst, dst_stride,
                                         wgt0, off0, shift, lvl_shift, ht, 16);
            hevc_uniwgt_8x4mult_msa(src + 16, src_stride, dst + 16, dst_stride,
                                    wgt0, off0, shift, lvl_shift, ht);
            break;
        case 16:
        case 32:
        case 48:
        case 64:
            hevc_uniwgt_16multx4mult_msa(src, src_stride, dst, dst_stride,
                                         wgt0, off0, shift, lvl_shift, ht, wd);
    }
}

void ihevc_weighted_pred_chroma_uni_msa(WORD16 *pi2_src, UWORD8 *pu1_dst,
                                        WORD32 src_strd, WORD32 dst_strd,
                                        WORD32 wgt0_cb, WORD32 wgt0_cr,
                                        WORD32 off0_cb, WORD32 off0_cr,
                                        WORD32 shift, WORD32 lvl_shift,
                                        WORD32 ht, WORD32 wd)
{
    switch(wd)
    {
        case 2:
            if(0 == (ht % 8))
            {
                hevc_chroma_uniwgt_4x8mult_msa(pi2_src, src_strd, pu1_dst,
                                               dst_strd, wgt0_cb, wgt0_cr,
                                               off0_cb, off0_cr, shift,
                                               lvl_shift, ht);
            }
            else
            {
                hevc_chroma_uniwgt_4x4_msa(pi2_src, src_strd, pu1_dst,
                                           dst_strd, wgt0_cb, wgt0_cr,
                                           off0_cb, off0_cr, shift,
                                           lvl_shift);
            }
            break;
        case 4:
            if(0 == (ht % 4))
            {
                hevc_chroma_uniwgt_8x4mult_msa(pi2_src, src_strd, pu1_dst,
                                               dst_strd, wgt0_cb, wgt0_cr,
                                               off0_cb, off0_cr, shift,
                                               lvl_shift, ht);
            }
            else
            {
                hevc_chroma_uniwgt_8x2_msa(pi2_src, src_strd, pu1_dst,
                                           dst_strd, wgt0_cb, wgt0_cr,
                                           off0_cb, off0_cr, shift,
                                           lvl_shift);
            }
            break;
        case 6:
            hevc_chroma_uniwgt_12x4mult_msa(pi2_src, src_strd, pu1_dst,
                                            dst_strd, wgt0_cb, wgt0_cr,
                                            off0_cb, off0_cr, shift,
                                            lvl_shift, ht);
            break;
        case 8:
            if(0 == (ht % 4))
            {
                hevc_chroma_uniwgt_16multx4mult_msa(pi2_src, src_strd, pu1_dst,
                                                    dst_strd, wgt0_cb, wgt0_cr,
                                                    off0_cb, off0_cr, shift,
                                                    lvl_shift, ht, 2 * wd);
            }
            else
            {
                hevc_chroma_uniwgt_16x2mult_msa(pi2_src, src_strd, pu1_dst,
                                                dst_strd, wgt0_cb, wgt0_cr,
                                                off0_cb, off0_cr, shift,
                                                lvl_shift, ht);
            }
            break;
        case 12:
            hevc_chroma_uniwgt_16multx4mult_msa(pi2_src, src_strd, pu1_dst,
                                                dst_strd, wgt0_cb, wgt0_cr,
                                                off0_cb, off0_cr, shift,
                                                lvl_shift, ht, 16);
            hevc_chroma_uniwgt_8x4mult_msa(pi2_src + 16, src_strd, pu1_dst + 16,
                                           dst_strd, wgt0_cb, wgt0_cr,
                                           off0_cb, off0_cr, shift,
                                           lvl_shift, ht);
            break;
        case 16:
        case 24:
        case 32:
            hevc_chroma_uniwgt_16multx4mult_msa(pi2_src, src_strd, pu1_dst,
                                                dst_strd, wgt0_cb, wgt0_cr,
                                                off0_cb, off0_cr, shift,
                                                lvl_shift, ht, 2 * wd);
    }
}

void ihevc_weighted_pred_bi_msa(WORD16 *pi2_src1, WORD16 *pi2_src2,
                                UWORD8 *pu1_dst, WORD32 src_strd1,
                                WORD32 src_strd2, WORD32 dst_strd,
                                WORD32 wgt0, WORD32 off0, WORD32 wgt1,
                                WORD32 off1, WORD32 shift, WORD32 lvl_shift1,
                                WORD32 lvl_shift2, WORD32 ht, WORD32 wd)
{
    switch(wd)
    {
        case 4:
            hevc_biwgt_4x8mult_msa(pi2_src1, src_strd1, pi2_src2, src_strd2,
                                   pu1_dst, dst_strd, wgt0, wgt1, off0, off1,
                                   shift, lvl_shift1, lvl_shift2, ht);
            break;
        case 8:
            hevc_biwgt_8x4mult_msa(pi2_src1, src_strd1, pi2_src2, src_strd2,
                                   pu1_dst, dst_strd, wgt0, wgt1, off0, off1,
                                   shift, lvl_shift1, lvl_shift2, ht);
            break;
        case 12:
            hevc_biwgt_12x4mult_msa(pi2_src1, src_strd1, pi2_src2, src_strd2,
                                    pu1_dst, dst_strd, wgt0, wgt1, off0, off1,
                                    shift, lvl_shift1, lvl_shift2, ht);
            break;
        case 24:
            hevc_biwgt_16multx4mult_msa(pi2_src1, src_strd1, pi2_src2,
                                        src_strd2, pu1_dst, dst_strd, wgt0,
                                        wgt1, off0, off1, shift, lvl_shift1,
                                        lvl_shift2, ht, 16);
            hevc_biwgt_8x4mult_msa(pi2_src1 + 16, src_strd1, pi2_src2 + 16,
                                   src_strd2, pu1_dst + 16, dst_strd, wgt0,
                                   wgt1, off0, off1, shift, lvl_shift1,
                                   lvl_shift2, ht);
            break;
        case 16:
        case 32:
        case 48:
        case 64:
            hevc_biwgt_16multx4mult_msa(pi2_src1, src_strd1, pi2_src2,
                                        src_strd2, pu1_dst, dst_strd, wgt0,
                                        wgt1, off0, off1, shift, lvl_shift1,
                                        lvl_shift2, ht, wd);
    }
}

void ihevc_weighted_pred_chroma_bi_msa(WORD16 *pi2_src1, WORD16 *pi2_src2,
                                       UWORD8 *pu1_dst, WORD32 src_strd1,
                                       WORD32 src_strd2, WORD32 dst_strd,
                                       WORD32 wgt0_cb, WORD32 wgt0_cr,
                                       WORD32 off0_cb, WORD32 off0_cr,
                                       WORD32 wgt1_cb, WORD32 wgt1_cr,
                                       WORD32 off1_cb, WORD32 off1_cr,
                                       WORD32 shift, WORD32 lvl_shift1,
                                       WORD32 lvl_shift2, WORD32 ht,
                                       WORD32 wd)
{
    switch(wd)
    {
        case 2:
            if(0 == (ht % 8))
            {
                hevc_chroma_biwgt_4x8mult_msa(pi2_src1, src_strd1, pi2_src2,
                                              src_strd2, pu1_dst, dst_strd,
                                              wgt0_cb, wgt0_cr, wgt1_cb,
                                              wgt1_cr, off0_cb, off0_cr,
                                              off1_cb, off1_cr, shift,
                                              lvl_shift1, lvl_shift2, ht);
            }
            else
            {
                hevc_chroma_biwgt_4x4_msa(pi2_src1, src_strd1, pi2_src2,
                                          src_strd2, pu1_dst, dst_strd,
                                          wgt0_cb, wgt0_cr, wgt1_cb,
                                          wgt1_cr, off0_cb, off0_cr,
                                          off1_cb, off1_cr, shift,
                                          lvl_shift1, lvl_shift2);
            }
            break;
        case 4:
            if(0 == (ht % 4))
            {
                hevc_chroma_biwgt_8x4mult_msa(pi2_src1, src_strd1, pi2_src2,
                                              src_strd2, pu1_dst, dst_strd,
                                              wgt0_cb, wgt0_cr, wgt1_cb,
                                              wgt1_cr, off0_cb, off0_cr,
                                              off1_cb, off1_cr, shift,
                                              lvl_shift1, lvl_shift2, ht);
            }
            else
            {
                hevc_chroma_biwgt_8x2mult_msa(pi2_src1, src_strd1, pi2_src2,
                                              src_strd2, pu1_dst, dst_strd,
                                              wgt0_cb, wgt0_cr, wgt1_cb,
                                              wgt1_cr, off0_cb, off0_cr,
                                              off1_cb, off1_cr, shift,
                                              lvl_shift1, lvl_shift2);
            }
            break;
        case 6:
            hevc_chroma_biwgt_12x4mult_msa(pi2_src1, src_strd1, pi2_src2,
                                           src_strd2, pu1_dst, dst_strd,
                                           wgt0_cb, wgt0_cr, wgt1_cb,
                                           wgt1_cr, off0_cb, off0_cr,
                                           off1_cb, off1_cr, shift,
                                           lvl_shift1, lvl_shift2, ht);
        case 8:
            if(0 == (ht % 4))
            {
                hevc_chroma_biwgt_16multx4mult_msa(pi2_src1, src_strd1,
                                                   pi2_src2, src_strd2, pu1_dst,
                                                   dst_strd, wgt0_cb, wgt0_cr,
                                                   wgt1_cb, wgt1_cr, off0_cb,
                                                   off0_cr, off1_cb, off1_cr,
                                                   shift, lvl_shift1,
                                                   lvl_shift2, ht, 2 * wd);
            }
            else
            {
                hevc_chroma_biwgt_16x2mult_msa(pi2_src1, src_strd1, pi2_src2,
                                               src_strd2, pu1_dst, dst_strd,
                                               wgt0_cb, wgt0_cr, wgt1_cb,
                                               wgt1_cr, off0_cb, off0_cr,
                                               off1_cb, off1_cr, shift,
                                               lvl_shift1, lvl_shift2, ht);
            }
            break;
        case 12:
            hevc_chroma_biwgt_16multx4mult_msa(pi2_src1, src_strd1, pi2_src2,
                                               src_strd2, pu1_dst, dst_strd,
                                               wgt0_cb, wgt0_cr, wgt1_cb,
                                               wgt1_cr, off0_cb, off0_cr,
                                               off1_cb, off1_cr, shift,
                                               lvl_shift1, lvl_shift2, ht,
                                               16);
            hevc_chroma_biwgt_8x4mult_msa(pi2_src1 + 16, src_strd1,
                                          pi2_src2 + 16, src_strd2,
                                          pu1_dst + 16, dst_strd,
                                          wgt0_cb, wgt0_cr, wgt1_cb,
                                          wgt1_cr, off0_cb, off0_cr,
                                          off1_cb, off1_cr, shift,
                                          lvl_shift1, lvl_shift2, ht);

            break;
        case 16:
        case 24:
        case 32:
            hevc_chroma_biwgt_16multx4mult_msa(pi2_src1, src_strd1, pi2_src2,
                                               src_strd2, pu1_dst, dst_strd,
                                               wgt0_cb, wgt0_cr, wgt1_cb,
                                               wgt1_cr, off0_cb, off0_cr,
                                               off1_cb, off1_cr, shift,
                                               lvl_shift1, lvl_shift2, ht,
                                               2 * wd);
    }
}

void ihevc_weighted_pred_bi_default_msa(WORD16 *pi2_src1, WORD16 *pi2_src2,
                                        UWORD8 *pu1_dst, WORD32 src_strd1,
                                        WORD32 src_strd2, WORD32 dst_strd,
                                        WORD32 lvl_shift1, WORD32 lvl_shift2,
                                        WORD32 ht, WORD32 wd)
{
    switch(wd)
    {
        case 4:
            hevc_bi_4x8mult_msa(pi2_src1, src_strd1, pi2_src2, src_strd2,
                                pu1_dst, dst_strd, lvl_shift1, lvl_shift2, ht);
            break;
        case 8:
            hevc_bi_8x4mult_msa(pi2_src1, src_strd1, pi2_src2, src_strd2,
                                pu1_dst, dst_strd, lvl_shift1, lvl_shift2, ht);
            break;
        case 12:
            hevc_bi_12x4mult_msa(pi2_src1, src_strd1, pi2_src2, src_strd2,
                                 pu1_dst, dst_strd, lvl_shift1, lvl_shift2, ht);
            break;
        case 16:
            if(0 == (ht % 4))
            {
                hevc_bi_16multx4mult_msa(pi2_src1, src_strd1, pi2_src2,
                                         src_strd2, pu1_dst, dst_strd,
                                         lvl_shift1, lvl_shift2, ht, wd);
            }
            else
            {
                hevc_bi_16x2mult_msa(pi2_src1, src_strd1, pi2_src2, src_strd2,
                                     pu1_dst, dst_strd, lvl_shift1, lvl_shift2,
                                     ht);
            }
            break;
        case 24:
            hevc_bi_16multx4mult_msa(pi2_src1, src_strd1, pi2_src2, src_strd2,
                                     pu1_dst, dst_strd, lvl_shift1, lvl_shift2,
                                     ht, 16);
            hevc_bi_8x4mult_msa(pi2_src1 + 16, src_strd1, pi2_src2 + 16,
                                src_strd2, pu1_dst + 16, dst_strd,
                                lvl_shift1, lvl_shift2, ht);
            break;
        case 32:
            hevc_bi_16multx4mult_msa(pi2_src1, src_strd1, pi2_src2, src_strd2,
                                     pu1_dst, dst_strd, lvl_shift1, lvl_shift2,
                                     ht, wd);
            break;
        case 48:
            hevc_bi_16multx4mult_msa(pi2_src1, src_strd1, pi2_src2, src_strd2,
                                     pu1_dst, dst_strd, lvl_shift1, lvl_shift2,
                                     ht, wd);
            break;
        case 64:
            hevc_bi_16multx4mult_msa(pi2_src1, src_strd1, pi2_src2, src_strd2,
                                     pu1_dst, dst_strd, lvl_shift1, lvl_shift2,
                                     ht, wd);
    }
}

void ihevc_weighted_pred_chroma_bi_default_msa(WORD16 *pi2_src1,
                                               WORD16 *pi2_src2,
                                               UWORD8 *pu1_dst,
                                               WORD32 src_strd1,
                                               WORD32 src_strd2,
                                               WORD32 dst_strd,
                                               WORD32 lvl_shift1,
                                               WORD32 lvl_shift2,
                                               WORD32 ht, WORD32 wd)
{
    UNUSED(lvl_shift1);
    UNUSED(lvl_shift2);

    switch(wd)
    {
        case 2:
            if(0 == (ht % 8))
            {
                hevc_chroma_bi_4x8mult_msa(pi2_src1, src_strd1, pi2_src2,
                                           src_strd2, pu1_dst, dst_strd, ht);
            }
            else
            {
                hevc_chroma_bi_4x4_msa(pi2_src1, src_strd1, pi2_src2,
                                       src_strd2, pu1_dst, dst_strd);
            }
            break;
        case 4:
            if(0 == (ht % 4))
            {
                hevc_chroma_bi_8x4mult_msa(pi2_src1, src_strd1, pi2_src2,
                                           src_strd2, pu1_dst, dst_strd, ht);
            }
            else
            {
                hevc_chroma_bi_8x2mult_msa(pi2_src1, src_strd1, pi2_src2,
                                           src_strd2, pu1_dst, dst_strd, ht);
            }
            break;
        case 6:
            hevc_chroma_bi_12x4mult_msa(pi2_src1, src_strd1, pi2_src2,
                                        src_strd2, pu1_dst, dst_strd, ht);
        case 8:
            if(0 == (ht % 4))
            {
                hevc_chroma_bi_16multx4mult_msa(pi2_src1, src_strd1, pi2_src2,
                                                src_strd2, pu1_dst, dst_strd,
                                                ht, 2 * wd);
            }
            else
            {
                hevc_chroma_bi_16x2mult_msa(pi2_src1, src_strd1, pi2_src2,
                                            src_strd2, pu1_dst, dst_strd, ht);
            }
            break;
        case 12:
            hevc_chroma_bi_16multx4mult_msa(pi2_src1, src_strd1, pi2_src2,
                                            src_strd2, pu1_dst, dst_strd,
                                            ht, 16);
            hevc_chroma_bi_8x4mult_msa(pi2_src1, src_strd1, pi2_src2,
                                       src_strd2, pu1_dst, dst_strd, ht);

            break;
        case 16:
        case 24:
        case 32:
            hevc_chroma_bi_16multx4mult_msa(pi2_src1, src_strd1, pi2_src2,
                                            src_strd2, pu1_dst, dst_strd,
                                            ht, 2 * wd);
    }
}
