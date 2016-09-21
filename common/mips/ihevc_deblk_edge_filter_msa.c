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

#include <assert.h>
#include <stdlib.h>
#include "ihevc_typedefs.h"
#include "ihevc_macros.h"
#include "ihevc_deblk.h"
#include "ihevc_deblk_tables.h"
#include "ihevc_debug.h"
#include "ihevc_macros_msa.h"

#define CONST_PAIR_H(RTYPE, flag0_m, flag1_m, out_m)  \
{                                                     \
    v2i64 temp0_m, temp1_m;                           \
                                                      \
    temp0_m = (v2i64) __msa_fill_h(flag0_m);          \
    temp1_m = (v2i64) __msa_fill_h(flag1_m);          \
    out_m = (RTYPE) __msa_ilvev_d(temp1_m, temp0_m);  \
}
#define CONST_PAIR_H_SH(...) CONST_PAIR_H(v8i16, __VA_ARGS__)

#define FILTER_CALC(RTYPE, in0, in1, in2, in3, in4, in5,   \
                    tc_neg_m, tc_pos_m, out0, out1, out2)  \
{                                                          \
    v8u16 temp0_m, temp1_m;                                \
    v8i16 temp2_m;                                         \
                                                           \
    temp0_m = in2 + in3 + in4;                             \
    temp1_m = (v8u16) SLLI_H((in0 + in1), 1);              \
    temp1_m += in1 + temp0_m;                              \
    temp1_m = (v8u16) __msa_srari_h((v8i16) temp1_m, 3);   \
    temp2_m = (v8i16) (temp1_m - in1);                     \
    temp2_m = CLIP_SH(temp2_m, tc_neg_m, tc_pos_m);        \
    out0 = (RTYPE) (temp2_m + (v8i16) in1);                \
                                                           \
    temp1_m = temp0_m + in1;                               \
    temp1_m = (v8u16) __msa_srari_h((v8i16) temp1_m, 2);   \
    temp2_m = (v8i16) (temp1_m - in2);                     \
    temp2_m = CLIP_SH(temp2_m, tc_neg_m, tc_pos_m);        \
    out1 = (RTYPE) (temp2_m + (v8i16) in2);                \
                                                           \
    temp1_m = (v8u16) SLLI_H(temp0_m, 1) + in1 + in5;      \
    temp1_m = (v8u16) __msa_srari_h((v8i16) temp1_m, 3);   \
    temp2_m = (v8i16) (temp1_m - in3);                     \
    temp2_m = CLIP_SH(temp2_m, tc_neg_m, tc_pos_m);        \
    out2 = (RTYPE) (temp2_m + (v8i16) in3);                \
}
#define FILTER_CALC_UB(...) FILTER_CALC(v16u8, __VA_ARGS__)

#define DELTA_CALC(RTYPE, in0, in1, in2, in3, delta_m)  \
{                                                       \
    v8i16 diff0_m, diff1_m;                             \
                                                        \
    diff0_m = (v8i16) (in2 - in1);                      \
    diff1_m = (v8i16) (in3 - in0);                      \
    diff0_m += SLLI_H(diff0_m, 3);                      \
    diff1_m += SLLI_H(diff1_m, 1);                      \
    diff0_m -= diff1_m;                                 \
    delta_m = (RTYPE) __msa_srari_h(diff0_m, 4);        \
}
#define DELTA_CALC_SH(...) DELTA_CALC(v8i16, __VA_ARGS__)

#define DELTA_ADD_BLOCK(RTYPE, in0, in1, in2, delta_m, tc_neg_m,  \
                        tc_pos_m, p_is_pcm_vec_m, out_m)          \
{                                                                 \
    v8i16 temp_m;                                                 \
                                                                  \
    temp_m = (v8i16) __msa_aver_u_h(in0, in2);                    \
    temp_m -= (v8i16) in1;                                        \
    temp_m += delta_m;                                            \
    temp_m = SRAI_H(temp_m, 1);                                   \
    temp_m = CLIP_SH(temp_m, tc_neg_m, tc_pos_m);                 \
    temp_m = (v8i16) in1 + (v8i16) temp_m;                        \
    temp_m = CLIP_SH_0_255(temp_m);                               \
    out_m = (RTYPE) __msa_bmnz_v((v16u8) temp_m, (v16u8) in1,     \
                                  (v16u8) p_is_pcm_vec_m);        \
}
#define DELTA_ADD_BLOCK_SH(...) DELTA_ADD_BLOCK(v8i16, __VA_ARGS__)

#define ST6x1_UB(in, pdst)                    \
{                                             \
    UWORD32 dst_w_m;                          \
    UWORD16 dst_h_m;                          \
                                              \
    dst_w_m = __msa_copy_u_w((v4i32) in, 0);  \
    dst_h_m = __msa_copy_u_h((v8i16) in, 2);  \
    SW(dst_w_m, src);                         \
    SH(dst_h_m, src + 4);                     \
}

#define BMZ_V2(RTYPE, in0, in1, in2, in3, in4, in5, out0, out1)         \
{                                                                       \
    out0 = (RTYPE) __msa_bmz_v((v16u8) in0, (v16u8) in1, (v16u8) in2);  \
    out1 = (RTYPE) __msa_bmz_v((v16u8) in3, (v16u8) in4, (v16u8) in5);  \
}
#define BMZ_V2_UB(...) BMZ_V2(v16u8, __VA_ARGS__)

#define BMZ_V3(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7, in8,      \
               out0, out1, out2)                                        \
{                                                                       \
    BMZ_V2(RTYPE, in0, in1, in2, in3, in4, in5, out0, out1);            \
    out2 = (RTYPE) __msa_bmz_v((v16u8) in6, (v16u8) in7, (v16u8) in8);  \
}
#define BMZ_V3_UB(...) BMZ_V3(v16u8, __VA_ARGS__)

#define BMZ_V4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7, in8,  \
               in9, in10, in11, out0, out1, out2, out3)             \
{                                                                   \
    BMZ_V2(RTYPE, in0, in1, in2, in3, in4, in5, out0, out1);        \
    BMZ_V2(RTYPE, in6, in7, in8, in9, in10, in11, out2, out3);      \
}
#define BMZ_V4_UB(...) BMZ_V4(v16u8, __VA_ARGS__)

static void hevc_lpf_luma_ver_4w_msa(UWORD8 *src, WORD32 stride, WORD32 beta,
                                     WORD32 tc, WORD32 flag_p, WORD32 flag_q)
{
    UWORD8 *p3 = src;
    UWORD8 *p2 = src + 3 * stride;
    WORD32 dp00, dq00, dp30, dq30, d00, d30;
    WORD32 p_is_pcm0, q_is_pcm0, beta30, beta20, tc250, tmp;
    v16u8 dst0, dst1, dst2, dst3, dst4, dst5, dst6 = { 0 };
    v16u8 dst7 = { 0 };
    v16i8 zero = { 0 };
    v8u16 p3_src, p2_src, p1_src, p0_src, q0_src, q1_src, q2_src, q3_src;
    v8u16 temp0, temp1;
    v8i16 temp2, tc_pos, tc_neg, delta0, delta1, delta2, abs_delta0;
    v2i64 p_is_pcm_vec, q_is_pcm_vec;

    if(0 == tc)
    {
        return;
    }

    dp00 = abs(p3[-3] - (p3[-2] << 1) + p3[-1]);
    dq00 = abs(p3[2] - (p3[1] << 1) + p3[0]);
    dp30 = abs(p2[-3] - (p2[-2] << 1) + p2[-1]);
    dq30 = abs(p2[2] - (p2[1] << 1) + p2[0]);
    d00 = dp00 + dq00;
    d30 = dp30 + dq30;
    p_is_pcm0 = flag_p;
    q_is_pcm0 = flag_q;
    q_is_pcm_vec = __msa_fill_d(!q_is_pcm0);
    q_is_pcm_vec = CEQI_D(q_is_pcm_vec, 0);
    p_is_pcm_vec = __msa_fill_d(!p_is_pcm0);
    p_is_pcm_vec = CEQI_D(p_is_pcm_vec, 0);

    if(p_is_pcm0 || q_is_pcm0)
    {
        if(!(d00 + d30 >= beta))
        {
            src -= 4;
            LD_UH4(src, stride, p3_src, p2_src, p1_src, p0_src);
            beta30 = beta >> 3;
            beta20 = beta >> 2;
            tc250 = ((tc * 5 + 1) >> 1);

            TRANSPOSE4x8_UB_UH(p3_src, p2_src, p1_src, p0_src,
                               p3_src, p2_src, p1_src, p0_src,
                               q0_src, q1_src, q2_src, q3_src);
            ILVR_B8_UH(zero, p3_src, zero, p2_src, zero, p1_src, zero, p0_src,
                       zero, q0_src, zero, q1_src, zero, q2_src, zero, q3_src,
                       p3_src, p2_src, p1_src, p0_src, q0_src, q1_src, q2_src,
                       q3_src);
            tc_pos = __msa_fill_h(tc << 1);
            tc_neg = - tc_pos;

            if(abs(p3[-4] - p3[-1]) + abs(p3[3] - p3[0]) < beta30 &&
               abs(p3[-1] - p3[0]) < tc250 &&
               abs(p2[-4] - p2[-1]) + abs(p2[3] - p2[0]) < beta30 &&
               abs(p2[-1] - p2[0]) < tc250 &&
               (d00 << 1) < beta20 &&
               (d30 << 1) < beta20
               )
            {
                FILTER_CALC_UB(p3_src, p2_src, p1_src, p0_src, q0_src, q1_src,
                               tc_neg, tc_pos, dst0, dst1, dst2);
                BMZ_V3_UB(dst0, p2_src, p_is_pcm_vec, dst1, p1_src,
                          p_is_pcm_vec, dst2, p0_src, p_is_pcm_vec,
                          dst0, dst1, dst2);
                FILTER_CALC_UB(q3_src, q2_src, q1_src, q0_src, p0_src, p1_src,
                               tc_neg, tc_pos, dst5, dst4, dst3);
                BMZ_V3_UB(dst3, q0_src, q_is_pcm_vec, dst4, q1_src,
                          q_is_pcm_vec, dst5, q2_src, q_is_pcm_vec,
                          dst3, dst4, dst5);
            }
            else
            {
                tc_pos = SRAI_H(tc_pos, 1);
                tc_neg = - tc_pos;
                DELTA_CALC_SH(p1_src, p0_src, q0_src, q1_src, delta0);
                temp1 = (v8u16)(SLLI_H(tc_pos, 3) + SLLI_H(tc_pos, 1));
                abs_delta0 = __msa_add_a_h(delta0, (v8i16) zero);
                abs_delta0 = (v8u16) abs_delta0 < temp1;
                delta0 = CLIP_SH(delta0, tc_neg, tc_pos);
                temp0 = (v8u16) delta0 + p0_src;
                temp0 = (v8u16) CLIP_SH_0_255(temp0);
                temp0 = (v8u16) __msa_bmz_v((v16u8) temp0, (v16u8) p0_src,
                                            (v16u8) p_is_pcm_vec);
                temp2 = (v8i16) q0_src - delta0;
                temp2 = CLIP_SH_0_255(temp2);
                temp2 = (v8i16) __msa_bmz_v((v16u8) temp2, (v16u8) q0_src,
                                            (v16u8) q_is_pcm_vec);

                tmp = ((beta + (beta >> 1)) >> 3);
                p_is_pcm_vec = __msa_fill_d(p_is_pcm0 && (dp00 + dp30 < tmp));
                p_is_pcm_vec = CEQI_D(p_is_pcm_vec, 0);
                q_is_pcm_vec = (v2i64) __msa_fill_h((q_is_pcm0) &&
                                                    (dq00 + dq30 < tmp));
                q_is_pcm_vec = CEQI_D(q_is_pcm_vec, 0);

                tc_pos = SRAI_H(tc_pos, 1);
                tc_neg = -tc_pos;
                DELTA_ADD_BLOCK_SH(p2_src, p1_src, p0_src, delta0, tc_neg,
                                   tc_pos, p_is_pcm_vec, delta1);
                DELTA_ADD_BLOCK_SH(q2_src, q1_src, q0_src, delta0, tc_neg,
                                   tc_pos, q_is_pcm_vec, delta2);

                BMZ_V4_UB(delta1, p1_src, abs_delta0, temp0, p0_src, abs_delta0,
                          temp2, q0_src, abs_delta0, delta2, q1_src, abs_delta0,
                          dst1, dst2, dst3, dst4);
                dst0 = (v16u8) p2_src;
                dst5 = (v16u8) q2_src;
            }

            PCKEV_B4_UB(dst0, dst0, dst1, dst1, dst2, dst2, dst3, dst3,
                        dst0, dst1, dst2, dst3);
            PCKEV_B2_UB(dst4, dst4, dst5, dst5, dst4, dst5);
            TRANSPOSE8x4_UB_UB(dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7,
                               dst0, dst1, dst2, dst3);
            src += 1;
            ST6x1_UB(dst0, src);
            src += stride;
            ST6x1_UB(dst1, src);
            src += stride;
            ST6x1_UB(dst2, src);
            src += stride;
            ST6x1_UB(dst3, src);
            src += stride;
        }
    }
}

static void hevc_lpf_luma_hor_4w_msa(UWORD8 *src, WORD32 stride, WORD32 beta,
                                     WORD32 tc, WORD32 flag_p, WORD32 flag_q)
{
    UWORD8 *p3 = src - (stride << 2);
    UWORD8 *p2 = src - ((stride << 1) + stride);
    UWORD8 *p1 = src - (stride << 1);
    UWORD8 *p0 = src - stride;
    UWORD8 *q0 = src;
    UWORD8 *q1 = src + stride;
    UWORD8 *q2 = src + (stride << 1);
    UWORD8 *q3 = src + (stride << 1) + stride;
    WORD32 dp00, dq00, dp30, dq30, d00, d30;
    WORD32 tc0, p_is_pcm0, q_is_pcm0, beta30, beta20, tc250, tmp;
    UWORD32 dst_val0, dst_val1;
    v16u8 dst0, dst1, dst2, dst3, dst4, dst5;
    v16i8 zero = { 0 };
    v8u16 temp0, temp1;
    v8i16 temp2, tc_pos, tc_neg, delta0, delta1, delta2, abs_delta0;
    v8u16 p3_src, p2_src, p1_src, p0_src, q0_src, q1_src, q2_src, q3_src;
    v2i64 p_is_pcm_vec, q_is_pcm_vec;

    dp00 = abs(p2[0] - (p1[0] << 1) + p0[0]);
    dq00 = abs(q2[0] - (q1[0] << 1) + q0[0]);
    dp30 = abs(p2[3] - (p1[3] << 1) + p0[3]);
    dq30 = abs(q2[3] - (q1[3] << 1) + q0[3]);
    d00 = dp00 + dq00;
    d30 = dp30 + dq30;
    p_is_pcm0 = flag_p;
    q_is_pcm0 = flag_q;

    if(p_is_pcm0 || q_is_pcm0)
    {
        if(!(d00 + d30 >= beta))
        {
            p3_src = LD_UH(p3);
            p2_src = LD_UH(p2);
            p1_src = LD_UH(p1);
            p0_src = LD_UH(p0);
            q0_src = LD_UH(q0);
            q1_src = LD_UH(q1);
            q2_src = LD_UH(q2);
            q3_src = LD_UH(q3);

            tc0 = tc;
            beta30 = beta >> 3;
            beta20 = beta >> 2;
            tc250 = ((tc0 * 5 + 1) >> 1);

            ILVR_B8_UH(zero, p3_src, zero, p2_src, zero, p1_src, zero, p0_src,
                       zero, q0_src, zero, q1_src, zero, q2_src, zero, q3_src,
                       p3_src, p2_src, p1_src, p0_src, q0_src, q1_src, q2_src,
                       q3_src);
            tc_pos = __msa_fill_h(tc0);
            tc_pos = SLLI_H(tc_pos, 1);
            tc_neg = -tc_pos;

            p_is_pcm_vec = __msa_fill_d(!p_is_pcm0);
            p_is_pcm_vec = CEQI_D(p_is_pcm_vec, 0);

            q_is_pcm_vec = __msa_fill_d(!q_is_pcm0);
            q_is_pcm_vec = CEQI_D(q_is_pcm_vec, 0);

            if(abs(p3[0] - p0[0]) + abs(q3[0] - q0[0]) < beta30 &&
               abs(p0[0] - q0[0]) < tc250 &&
               abs(p3[3] - p0[3]) + abs(q3[3] - q0[3]) < beta30 &&
               abs(p0[3] - q0[3]) < tc250 &&
               (d00 << 1) < beta20 &&
               (d30 << 1) < beta20
              )
            {
                FILTER_CALC_UB(p3_src, p2_src, p1_src, p0_src, q0_src, q1_src,
                               tc_neg, tc_pos, dst0, dst1, dst2);
                BMZ_V3_UB(dst0, p2_src, p_is_pcm_vec, dst1, p1_src, p_is_pcm_vec,
                          dst2, p0_src, p_is_pcm_vec, dst0, dst1, dst2);
                FILTER_CALC_UB(q3_src, q2_src, q1_src, q0_src, p0_src, p1_src,
                               tc_neg, tc_pos, dst5, dst4, dst3);
                BMZ_V3_UB(dst3, q0_src, q_is_pcm_vec, dst4, q1_src, q_is_pcm_vec,
                          dst5, q2_src, q_is_pcm_vec, dst3, dst4, dst5);
            }
            else
            {
                tc_pos = SRAI_H(tc_pos, 1);
                tc_neg = -tc_pos;

                DELTA_CALC_SH(p1_src, p0_src, q0_src, q1_src, delta0);
                temp1 = (v8u16) (SLLI_H(tc_pos, 3) + SLLI_H(tc_pos, 1));
                abs_delta0 = __msa_add_a_h(delta0, (v8i16) zero);
                abs_delta0 = (v8u16) abs_delta0 < temp1;
                delta0 = CLIP_SH(delta0, tc_neg, tc_pos);

                temp0 = (v8u16) (delta0 + p0_src);
                temp0 = (v8u16) CLIP_SH_0_255(temp0);
                temp0 = (v8u16) __msa_bmz_v((v16u8) temp0, (v16u8) p0_src,
                                            (v16u8) p_is_pcm_vec);
                temp2 = (v8i16) (q0_src - delta0);
                temp2 = CLIP_SH_0_255(temp2);
                temp2 = (v8i16) __msa_bmz_v((v16u8) temp2, (v16u8) q0_src,
                                            (v16u8) q_is_pcm_vec);
                tmp = (beta + (beta >> 1)) >> 3;
                p_is_pcm_vec = __msa_fill_d(p_is_pcm0 && ((dp00 + dp30) < tmp));
                p_is_pcm_vec = CEQI_D(p_is_pcm_vec, 0);
                q_is_pcm_vec = (v2i64) __msa_fill_h(q_is_pcm0 &&
                                                    (dq00 + dq30 < tmp));
                q_is_pcm_vec = CEQI_D(q_is_pcm_vec, 0);

                tc_pos = SRAI_H(tc_pos, 1);
                tc_neg = -tc_pos;
                DELTA_ADD_BLOCK_SH(p2_src, p1_src, p0_src, delta0, tc_neg,
                                   tc_pos, p_is_pcm_vec, delta1);
                DELTA_ADD_BLOCK_SH(q2_src, q1_src, q0_src, delta0, tc_neg,
                                   tc_pos, q_is_pcm_vec, delta2);
                BMZ_V4_UB(delta1, p1_src, abs_delta0, temp0, p0_src, abs_delta0,
                          temp2, q0_src, abs_delta0, delta2, q1_src, abs_delta0,
                          dst1, dst2, dst3, dst4);
                dst0 = (v16u8) p2_src;
                dst5 = (v16u8) q2_src;
            }

            PCKEV_B2_UB(dst1, dst0, dst3, dst2, dst0, dst1);
            dst2 = (v16u8) __msa_pckev_b((v16i8) dst5, (v16i8) dst4);
            dst_val0 = __msa_copy_u_w((v4i32) dst2, 0);
            dst_val1 = __msa_copy_u_w((v4i32) dst2, 2);
            ST4x4_UB(dst0, dst1, 0, 2, 0, 2, p2, stride);
            p2 += (4 * stride);
            SW(dst_val0, p2);
            p2 += stride;
            SW(dst_val1, p2);
        }
    }
}

static void hevc_lpf_chroma_ver_4w_msa(UWORD8 *src, WORD32 src_stride,
                                       WORD32 tc_u, WORD32 tc_v,
                                       WORD32 flag_p, WORD32 flag_q)
{
    UWORD8 *src_left, *src_st;
    WORD32 val0, val1, val2, val3;
    v16u8 src0, src1, src2, src3, dst0, dst1, dst2, dst3;
    v16i8 flag0, flag1;
    v16i8 zero = { 0 };
    v8i16 temp0, temp1, temp2, temp3, diff0, diff1, delta, tc0, tc1;

    if((0 == tc_u) && (0 == tc_v))
    {
        return;
    }

    src_left = src - 4;
    src_st = src - 2;

    LD_UB4(src_left, src_stride, src0, src1, src2, src3);

    CONST_PAIR_H_SH(tc_u, tc_v, tc0);
    flag0 = __msa_fill_b(flag_p);
    flag1 = __msa_fill_b(flag_q);

    flag0 = CEQI_B(flag0, 0);
    flag1 = CEQI_B(flag1, 0);

    flag0 = (v16i8) __msa_ilvr_h((v8i16) flag1, (v8i16) flag0);
    tc1 = - tc0;
    ILVR_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3,
               temp0, temp1, temp2, temp3);
    SLDI_B2_UB(src0, src1, src0, src1, src0, src1, 2);
    SLDI_B2_UB(src2, src3, src2, src3, src2, src3, 2);
    TRANSPOSE8X4_SH_SH(temp0, temp1, temp2, temp3, temp0, temp1, temp2, temp3);

    diff0 = SLLI_H((temp2 - temp1), 2);
    diff1 = temp0 - temp3;
    diff0 += diff1;
    diff0 = __msa_srari_h(diff0, 3);
    delta = CLIP_SH(diff0, tc1, tc0);
    temp1 += delta;
    temp2 -= delta;

    CLIP_SH2_0_255(temp1, temp2);
    PCKEV_B2_UB(temp1, temp1, temp2, temp2, dst0, dst2);
    dst1 = (v16u8) __msa_ilvod_w((v4i32) dst0, (v4i32) dst0);
    dst3 = (v16u8) __msa_ilvod_w((v4i32) dst2, (v4i32) dst2);
    TRANSPOSE4x4_UB_UB(dst0, dst1, dst2, dst3, dst0, dst1, dst2, dst3);
    BMZ_V2_UB(src0, dst0, flag0, src1, dst1, flag0, dst0, dst1);
    BMZ_V2_UB(src2, dst2, flag0, src3, dst3, flag0, dst2, dst3);
    val0 = __msa_copy_u_w((v4i32) dst0, 0);
    val1 = __msa_copy_u_w((v4i32) dst1, 0);
    val2 = __msa_copy_u_w((v4i32) dst2, 0);
    val3 = __msa_copy_u_w((v4i32) dst3, 0);
    SW4(val0, val1, val2, val3, src_st, src_stride);
}

static void hevc_lpf_chroma_hor_4w_msa(UWORD8 *src, WORD32 src_stride,
                                       WORD32 tc_u, WORD32 tc_v,
                                       WORD32 flag_p, WORD32 flag_q)
{
    UWORD8 *src_top = src - 2 * src_stride;
    uint64_t dst0, dst1;
    v16u8 src0, src1, src2, src3;
    v16i8 zero = { 0 };
    v8i16 temp0, temp1, temp2, temp3, diff0, diff1, delta, tc0, tc1;

    if((0 == tc_u) && (0 == tc_v))
    {
        return;
    }

    LD_UB4(src_top, src_stride, src0, src1, src2, src3);
    tc0 = __msa_fill_h(tc_u);
    tc1 = __msa_fill_h(tc_v);
    tc0 = __msa_ilvr_h(tc1, tc0);
    tc1 = - tc0;
    ILVR_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3,
               temp0, temp1, temp2, temp3);

    diff0 = SLLI_H((temp2 - temp1), 2);
    diff1 = temp0 - temp3;
    diff0 += diff1;
    delta = __msa_srari_h(diff0, 3);
    delta = CLIP_SH(delta, tc1, tc0);
    temp1 += delta;
    temp2 -= delta;
    CLIP_SH2_0_255(temp1, temp2);
    PCKEV_B2_SH(temp1, temp1, temp2, temp2, temp1, temp2);
    dst0 = __msa_copy_u_d((v2i64) temp1, 0);
    dst1 = __msa_copy_u_d((v2i64) temp2, 0);

    if(0 != flag_p)
    {
        SD(dst0, src - src_stride);
    }

    if(0 != flag_q)
    {
        SD(dst1, src);
    }
}

void ihevc_deblk_luma_vert_msa(UWORD8 *src, WORD32 src_strd, WORD32 bs,
                               WORD32 quant_param_p, WORD32 quant_param_q,
                               WORD32 beta_offset_div2, WORD32 tc_offset_div2,
                               WORD32 filter_flag_p, WORD32 filter_flag_q)
{
    WORD32 qp_luma, beta_indx, tc_indx, beta, tc;

    ASSERT((bs > 0) && (bs <= 3));
    ASSERT(filter_flag_p || filter_flag_q);

    qp_luma = (quant_param_p + quant_param_q + 1) >> 1;
    beta_indx = CLIP3(qp_luma + (beta_offset_div2 << 1), 0, 51);

    tc_indx = CLIP3(qp_luma + (2 * (bs >> 1)) + (tc_offset_div2 << 1), 0, 53);

    beta = gai4_ihevc_beta_table[beta_indx];
    tc = gai4_ihevc_tc_table[tc_indx];

    if(0 == tc)
    {
        return;
    }

    hevc_lpf_luma_ver_4w_msa(src, src_strd, beta, tc, filter_flag_p,
                             filter_flag_q);
}

void ihevc_deblk_luma_horz_msa(UWORD8 *src, WORD32 src_strd,
                               WORD32 bs, WORD32 quant_param_p,
                               WORD32 quant_param_q, WORD32 beta_offset_div2,
                               WORD32 tc_offset_div2, WORD32 filter_flag_p,
                               WORD32 filter_flag_q)
{
    WORD32 qp_luma, beta_indx, tc_indx, beta, tc;

    ASSERT((bs > 0));
    ASSERT(filter_flag_p || filter_flag_q);

    qp_luma = (quant_param_p + quant_param_q + 1) >> 1;
    beta_indx = CLIP3(qp_luma + (beta_offset_div2 << 1), 0, 51);
    tc_indx = CLIP3(qp_luma + 2 * (bs >> 1) + (tc_offset_div2 << 1), 0, 53);
    beta = gai4_ihevc_beta_table[beta_indx];
    tc = gai4_ihevc_tc_table[tc_indx];

    if(0 == tc)
    {
        return;
    }

    hevc_lpf_luma_hor_4w_msa(src, src_strd, beta, tc, filter_flag_p,
                             filter_flag_q);
}

void ihevc_deblk_chroma_vert_msa(UWORD8 *src, WORD32 src_strd,
                                 WORD32 quant_param_p, WORD32 quant_param_q,
                                 WORD32 qp_offset_u, WORD32 qp_offset_v,
                                 WORD32 tc_offset_div2, WORD32 filter_flag_p,
                                 WORD32 filter_flag_q)
{
    WORD32 qp_indx_u, qp_chroma_u, qp_indx_v, qp_chroma_v;
    WORD32 tc_indx_u, tc_u, tc_indx_v, tc_v;

    ASSERT(filter_flag_p || filter_flag_q);

    qp_indx_u = qp_offset_u + ((quant_param_p + quant_param_q + 1) >> 1);
    qp_chroma_u = qp_indx_u < 0 ? qp_indx_u : (qp_indx_u > 57 ? qp_indx_u - 6 :
                    gai4_ihevc_qp_table[qp_indx_u]);

    qp_indx_v = qp_offset_v + ((quant_param_p + quant_param_q + 1) >> 1);
    qp_chroma_v = qp_indx_v < 0 ? qp_indx_v : (qp_indx_v > 57 ? qp_indx_v - 6 :
                    gai4_ihevc_qp_table[qp_indx_v]);

    tc_indx_u = CLIP3(qp_chroma_u + 2 + (tc_offset_div2 << 1), 0, 53);
    tc_u = gai4_ihevc_tc_table[tc_indx_u];

    tc_indx_v = CLIP3(qp_chroma_v + 2 + (tc_offset_div2 << 1), 0, 53);
    tc_v = gai4_ihevc_tc_table[tc_indx_v];

    if(0 == tc_u && 0 == tc_v)
    {
        return;
    }

    hevc_lpf_chroma_ver_4w_msa(src, src_strd, tc_u, tc_v, filter_flag_p,
                               filter_flag_q);
}

void ihevc_deblk_chroma_horz_msa(UWORD8 *src, WORD32 src_strd,
                                 WORD32 quant_param_p, WORD32 quant_param_q,
                                 WORD32 qp_offset_u, WORD32 qp_offset_v,
                                 WORD32 tc_offset_div2, WORD32 filter_flag_p,
                                 WORD32 filter_flag_q)
{
    WORD32 qp_indx_u, qp_chroma_u, qp_indx_v, qp_chroma_v;
    WORD32 tc_indx_u, tc_u, tc_indx_v, tc_v;

    ASSERT(filter_flag_p || filter_flag_q);

    qp_indx_u = qp_offset_u + ((quant_param_p + quant_param_q + 1) >> 1);
    qp_chroma_u = qp_indx_u < 0 ? qp_indx_u : (qp_indx_u > 57 ? qp_indx_u -
                  6 : gai4_ihevc_qp_table[qp_indx_u]);

    qp_indx_v = qp_offset_v + ((quant_param_p + quant_param_q + 1) >> 1);
    qp_chroma_v = qp_indx_v < 0 ? qp_indx_v : (qp_indx_v > 57 ? qp_indx_v -
                  6 : gai4_ihevc_qp_table[qp_indx_v]);

    tc_indx_u = CLIP3(qp_chroma_u + 2 + (tc_offset_div2 << 1), 0, 53);
    tc_u = gai4_ihevc_tc_table[tc_indx_u];

    tc_indx_v = CLIP3(qp_chroma_v + 2 + (tc_offset_div2 << 1), 0, 53);
    tc_v = gai4_ihevc_tc_table[tc_indx_v];

    if(0 == tc_u && 0 == tc_v)
    {
        return;
    }

    hevc_lpf_chroma_hor_4w_msa(src, src_strd, tc_u, tc_v, filter_flag_p,
                               filter_flag_q);
}
