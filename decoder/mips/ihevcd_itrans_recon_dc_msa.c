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
#include "ihevc_platform_macros.h"
#include "ihevcd_itrans_recon_dc.h"
#include "ihevc_macros_msa.h"

static void hevc_idct_dc_addpred_4x4_msa(UWORD8 *dst, WORD32 dst_stride,
                                         WORD16 dc_value)
{
    UWORD32 pred0, pred1, pred2, pred3;
    v8i16 pred_r, pred_l, zeros = { 0 };
    v4i32 vec = { 0 };
    v8i16 dc_vec = __msa_fill_h(dc_value);

    LW4(dst, dst_stride, pred0, pred1, pred2, pred3);
    INSERT_W4_SW(pred0, pred1, pred2, pred3, vec);
    ILVRL_B2_SH(zeros, vec, pred_r, pred_l);
    ADD2(pred_r, dc_vec, pred_l, dc_vec, pred_r, pred_l);
    CLIP_SH2_0_255(pred_r, pred_l);
    vec = (v4i32) __msa_pckev_b((v16i8) pred_l, (v16i8) pred_r);
    ST4x4_UB(vec, vec, 0, 1, 2, 3, dst, dst_stride);
}

static void hevc_idct_dc_addpred_8x8_msa(UWORD8 *dst, WORD32 dst_stride,
                                         WORD16 dc_value)
{
    ULWORD64 pred0, pred1, pred2, pred3;
    v8i16 pred0_r, pred0_l, pred1_r, pred1_l;
    v8i16 zeros = { 0 };
    v8i16 dc_vec = __msa_fill_h(dc_value);
    v2i64 vec0 = { 0 };
    v2i64 vec1 = { 0 };

    LD4(dst, dst_stride, pred0, pred1, pred2, pred3);
    INSERT_D2_SD(pred0, pred1, vec0);
    INSERT_D2_SD(pred2, pred3, vec1);
    ILVRL_B2_SH(zeros, vec0, pred0_r, pred0_l);
    ILVRL_B2_SH(zeros, vec1, pred1_r, pred1_l);
    ADD4(pred0_r, dc_vec, pred0_l, dc_vec, pred1_r, dc_vec, pred1_l, dc_vec,
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
    ADD4(pred0_r, dc_vec, pred0_l, dc_vec, pred1_r, dc_vec, pred1_l, dc_vec,
         pred0_r, pred0_l, pred1_r, pred1_l);
    CLIP_SH4_0_255(pred0_r, pred0_l, pred1_r, pred1_l);
    PCKEV_B2_SH(pred0_l, pred0_r, pred1_l, pred1_r, pred0_r, pred1_r);
    ST8x4_UB(pred0_r, pred1_r, dst, dst_stride);
}

static void hevc_idct_dc_addpred_16x16_msa(UWORD8 *dst, WORD32 dst_stride,
                                           WORD16 dc_value)
{
    UWORD8 loop;
    v16u8 pred0, pred1, pred2, pred3, pred4, pred5, pred6, pred7;
    v8i16 pred0_r, pred0_l, pred1_r, pred1_l, pred2_r, pred2_l, pred3_r;
    v8i16 pred3_l;
    v8i16 dc_vec = __msa_fill_h(dc_value);
    v8i16 zeros = { 0 };

    for(loop = 2; loop--;)
    {
        LD_UB8(dst, dst_stride,
               pred0, pred1, pred2, pred3, pred4, pred5, pred6, pred7);
        ILVR_B4_SH(zeros, pred0, zeros, pred1, zeros, pred2, zeros, pred3,
                   pred0_r, pred1_r, pred2_r, pred3_r);
        ILVL_B4_SH(zeros, pred0, zeros, pred1, zeros, pred2, zeros, pred3,
                   pred0_l, pred1_l, pred2_l, pred3_l);
        ADD4(pred0_r, dc_vec, pred1_r, dc_vec, pred2_r, dc_vec, pred3_r, dc_vec,
             pred0_r, pred1_r, pred2_r, pred3_r);
        ADD4(pred0_l, dc_vec, pred1_l, dc_vec, pred2_l, dc_vec, pred3_l, dc_vec,
             pred0_l, pred1_l, pred2_l, pred3_l);
        CLIP_SH4_0_255(pred0_r, pred1_r, pred2_r, pred3_r);
        CLIP_SH4_0_255(pred0_l, pred1_l, pred2_l, pred3_l);
        PCKEV_B4_UB(pred0_l, pred0_r, pred1_l, pred1_r, pred2_l, pred2_r,
                    pred3_l, pred3_r, pred0, pred1, pred2, pred3);
        ST_UB4(pred0, pred1, pred2, pred3, dst, dst_stride);

        ILVR_B4_SH(zeros, pred4, zeros, pred5, zeros, pred6, zeros, pred7,
                   pred0_r, pred1_r, pred2_r, pred3_r);
        ILVL_B4_SH(zeros, pred4, zeros, pred5, zeros, pred6, zeros, pred7,
                   pred0_l, pred1_l, pred2_l, pred3_l);
        ADD4(pred0_r, dc_vec, pred1_r, dc_vec, pred2_r, dc_vec, pred3_r, dc_vec,
             pred0_r, pred1_r, pred2_r, pred3_r);
        ADD4(pred0_l, dc_vec, pred1_l, dc_vec, pred2_l, dc_vec, pred3_l, dc_vec,
             pred0_l, pred1_l, pred2_l, pred3_l);
        CLIP_SH4_0_255(pred0_r, pred1_r, pred2_r, pred3_r);
        CLIP_SH4_0_255(pred0_l, pred1_l, pred2_l, pred3_l);
        PCKEV_B4_UB(pred0_l, pred0_r, pred1_l, pred1_r, pred2_l, pred2_r,
                    pred3_l, pred3_r, pred0, pred1, pred2, pred3);
        ST_UB4(pred0, pred1, pred2, pred3, dst + 4 * dst_stride, dst_stride);

        dst += 8 * dst_stride;
    }
}

static void hevc_idct_dc_addpred_32x32_msa(UWORD8 *dst, WORD32 dst_stride,
                                           WORD16 dc_value)
{
    UWORD8 col, row;
    v16u8 pred0, pred1, pred2, pred3, pred4, pred5, pred6, pred7;
    v8i16 pred0_r, pred0_l, pred1_r, pred1_l, pred2_r, pred2_l, pred3_r;
    v8i16 pred3_l, zeros = { 0 };
    v8i16 dc_vec = __msa_fill_h(dc_value);

    for(row = 4; row--;)
    {
        for(col = 2; col--;)
        {
            LD_UB8(dst + 16 * col, dst_stride, pred0, pred1, pred2, pred3,
                   pred4, pred5, pred6, pred7);
            ILVR_B4_SH(zeros, pred0, zeros, pred1, zeros, pred2, zeros, pred3,
                       pred0_r, pred1_r, pred2_r, pred3_r);
            ILVL_B4_SH(zeros, pred0, zeros, pred1, zeros, pred2, zeros, pred3,
                       pred0_l, pred1_l, pred2_l, pred3_l);
            ADD4(pred0_r, dc_vec, pred1_r, dc_vec, pred2_r, dc_vec, pred3_r,
                 dc_vec, pred0_r, pred1_r, pred2_r, pred3_r);
            ADD4(pred0_l, dc_vec, pred1_l, dc_vec, pred2_l, dc_vec, pred3_l,
                 dc_vec, pred0_l, pred1_l, pred2_l, pred3_l);
            CLIP_SH4_0_255(pred0_r, pred1_r, pred2_r, pred3_r);
            CLIP_SH4_0_255(pred0_l, pred1_l, pred2_l, pred3_l);
            PCKEV_B4_UB(pred0_l, pred0_r, pred1_l, pred1_r, pred2_l, pred2_r,
                        pred3_l, pred3_r, pred0, pred1, pred2, pred3);
            ST_UB4(pred0, pred1, pred2, pred3, dst + 16 * col, dst_stride);

            ILVR_B4_SH(zeros, pred4, zeros, pred5, zeros, pred6, zeros, pred7,
                       pred0_r, pred1_r, pred2_r, pred3_r);
            ILVL_B4_SH(zeros, pred4, zeros, pred5, zeros, pred6, zeros, pred7,
                       pred0_l, pred1_l, pred2_l, pred3_l);
            ADD4(pred0_r, dc_vec, pred1_r, dc_vec, pred2_r, dc_vec, pred3_r,
                 dc_vec, pred0_r, pred1_r, pred2_r, pred3_r);
            ADD4(pred0_l, dc_vec, pred1_l, dc_vec, pred2_l, dc_vec, pred3_l,
                 dc_vec, pred0_l, pred1_l, pred2_l, pred3_l);
            CLIP_SH4_0_255(pred0_r, pred1_r, pred2_r, pred3_r);
            CLIP_SH4_0_255(pred0_l, pred1_l, pred2_l, pred3_l);
            PCKEV_B4_UB(pred0_l, pred0_r, pred1_l, pred1_r, pred2_l, pred2_r,
                        pred3_l, pred3_r, pred0, pred1, pred2, pred3);
            ST_UB4(pred0, pred1, pred2, pred3, dst + 16 * col + 4 * dst_stride,
                   dst_stride);
        }

        dst += 8 * dst_stride;
    }
}

static void hevc_chroma_idct_dc_addpred_4x4_msa(UWORD8 *dst, WORD32 dst_stride,
                                                WORD16 dc_value)
{
    ULWORD64 pred0, pred1, pred2, pred3;
    v16i8 vec0 = { 0 };
    v16i8 vec1 = { 0 };
    v8i16 dst0, dst1, dc_vec;
    v16i8 mask = { 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0,
                   0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0 };
    v16i8 mask1 = { 0, 17, 2, 19, 4, 21, 6, 23, 8, 25, 10, 27, 12, 29, 14, 31 };

    dc_vec = __msa_fill_h(dc_value);

    LD4(dst, dst_stride, pred0, pred1, pred2, pred3);
    INSERT_D2_SB(pred0, pred1, vec0);
    INSERT_D2_SB(pred2, pred3, vec1);

    dst0 = (v8i16) (vec0 & mask);
    dst1 = (v8i16) (vec1 & mask);

    ADD2(dst0, dc_vec, dst1, dc_vec, dst0, dst1);
    CLIP_SH2_0_255(dst0, dst1);
    VSHF_B2_SB(dst0, vec0, dst1, vec1, mask1, mask1, vec0, vec1);
    ST8x4_UB(vec0, vec1, dst, dst_stride);
}

static void hevc_chroma_idct_dc_addpred_8x8_msa(UWORD8 *dst, WORD32 dst_stride,
                                                WORD16 dc_value)
{
    v16i8 pred0, pred1, pred2, pred3, pred4, pred5, pred6, pred7;
    v8i16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7, dc_vec;
    v16i8 mask = { 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0,
                   0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0 };
    v16i8 mask1 = { 0, 17, 2, 19, 4, 21, 6, 23, 8, 25, 10, 27, 12, 29, 14, 31 };

    dc_vec = __msa_fill_h(dc_value);
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

    ADD4(dst0, dc_vec, dst1, dc_vec, dst2, dc_vec, dst3, dc_vec, dst0, dst1,
          dst2, dst3);
    ADD4(dst4, dc_vec, dst5, dc_vec, dst6, dc_vec, dst7, dc_vec, dst4, dst5,
         dst6, dst7);
    CLIP_SH4_0_255(dst0, dst1, dst2, dst3);
    CLIP_SH4_0_255(dst4, dst5, dst6, dst7);
    VSHF_B2_SB(dst0, pred0, dst1, pred1, mask1, mask1, pred0, pred1);
    VSHF_B2_SB(dst2, pred2, dst3, pred3, mask1, mask1, pred2, pred3);
    VSHF_B2_SB(dst4, pred4, dst5, pred5, mask1, mask1, pred4, pred5);
    VSHF_B2_SB(dst6, pred6, dst7, pred7, mask1, mask1, pred6, pred7);
    ST_SB4(pred0, pred1, pred2, pred3, dst, dst_stride);
    ST_SB4(pred4, pred5, pred6, pred7, dst + 4 * dst_stride, dst_stride);
}

static void hevc_chroma_idct_dc_addpred_16x16_msa(UWORD8 *dst,
                                                  WORD32 dst_stride,
                                                  WORD16 dc_value)
{
    UWORD32 loop_cnt;
    v16i8 pred0, pred1, pred2, pred3, pred4, pred5, pred6, pred7;
    v8i16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7, dc_vec;
    v16i8 mask = { 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0,
                   0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0 };
    v16i8 mask1 = { 0, 17, 2, 19, 4, 21, 6, 23, 8, 25, 10, 27, 12, 29, 14, 31 };

    dc_vec = __msa_fill_h(dc_value);

    for(loop_cnt = 4; loop_cnt--;)
    {
        LD_SB4(dst, dst_stride, pred0, pred2, pred4, pred6);
        LD_SB4(dst + 16, dst_stride, pred1, pred3, pred5, pred7);

        dst0 = (v8i16) (pred0 & mask);
        dst1 = (v8i16) (pred1 & mask);
        dst2 = (v8i16) (pred2 & mask);
        dst3 = (v8i16) (pred3 & mask);
        dst4 = (v8i16) (pred4 & mask);
        dst5 = (v8i16) (pred5 & mask);
        dst6 = (v8i16) (pred6 & mask);
        dst7 = (v8i16) (pred7 & mask);

        ADD4(dst0, dc_vec, dst1, dc_vec, dst2, dc_vec, dst3, dc_vec, dst0, dst1,
             dst2, dst3);
        ADD4(dst4, dc_vec, dst5, dc_vec, dst6, dc_vec, dst7, dc_vec, dst4, dst5,
             dst6, dst7);
        CLIP_SH4_0_255(dst0, dst1, dst2, dst3);
        CLIP_SH4_0_255(dst4, dst5, dst6, dst7);
        VSHF_B2_SB(dst0, pred0, dst1, pred1, mask1, mask1, pred0, pred1);
        VSHF_B2_SB(dst2, pred2, dst3, pred3, mask1, mask1, pred2, pred3);
        VSHF_B2_SB(dst4, pred4, dst5, pred5, mask1, mask1, pred4, pred5);
        VSHF_B2_SB(dst6, pred6, dst7, pred7, mask1, mask1, pred6, pred7);
        ST_SB4(pred0, pred2, pred4, pred6, dst, dst_stride);
        ST_SB4(pred1, pred3, pred5, pred7, dst + 16, dst_stride);
        dst += (4 * dst_stride);
    }
}

void ihevcd_itrans_recon_dc_luma_msa(UWORD8 *pred, UWORD8 *dst,
                                     WORD32 pred_stride, WORD32 dst_stride,
                                     WORD32 log2_trans_size,
                                     WORD16 i2_coeff_value)
{
    WORD32 add, shift, dc_value, quant_out;
    WORD32 trans_size;
    UNUSED(pred);
    UNUSED(pred_stride);

    trans_size = (1 << log2_trans_size);

    quant_out = i2_coeff_value;

    shift = IT_SHIFT_STAGE_1;
    add = 1 << (shift - 1);
    dc_value = CLIP_S16((quant_out * 64 + add) >> shift);
    shift = IT_SHIFT_STAGE_2;
    add = 1 << (shift - 1);
    dc_value = CLIP_S16((dc_value * 64 + add) >> shift);

    switch(trans_size)
    {
        case 4:
            hevc_idct_dc_addpred_4x4_msa(dst, dst_stride, dc_value);
            break;
        case 8:
            hevc_idct_dc_addpred_8x8_msa(dst, dst_stride, dc_value);
            break;
        case 16:
            hevc_idct_dc_addpred_16x16_msa(dst, dst_stride, dc_value);
            break;
        case 32:
            hevc_idct_dc_addpred_32x32_msa(dst, dst_stride, dc_value);
    }
}

void ihevcd_itrans_recon_dc_chroma_msa(UWORD8 *pred, UWORD8 *dst,
                                       WORD32 pred_stride, WORD32 dst_stride,
                                       WORD32 log2_trans_size,
                                       WORD16 i2_coeff_value)
{
    WORD32 add, shift;
    WORD32 dc_value, quant_out;
    WORD32 trans_size;
    UNUSED(pred);
    UNUSED(pred_stride);

    trans_size = (1 << log2_trans_size);

    quant_out = i2_coeff_value;

    shift = IT_SHIFT_STAGE_1;
    add = 1 << (shift - 1);
    dc_value = CLIP_S16((quant_out * 64 + add) >> shift);
    shift = IT_SHIFT_STAGE_2;
    add = 1 << (shift - 1);
    dc_value = CLIP_S16((dc_value * 64 + add) >> shift);

    switch(trans_size)
    {
        case 4:
            hevc_chroma_idct_dc_addpred_4x4_msa(dst, dst_stride, dc_value);
            break;
        case 8:
            hevc_chroma_idct_dc_addpred_8x8_msa(dst, dst_stride, dc_value);
            break;
        case 16:
            hevc_chroma_idct_dc_addpred_16x16_msa(dst, dst_stride, dc_value);
    }
}
