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

#include <string.h>
#include "ihevc_typedefs.h"
#include "ihevc_macros.h"
#include "ihevc_defs.h"
#include "ihevc_sao.h"
#include "ihevc_macros_msa.h"

#define NUM_BAND_TABLE  32

const WORD32 gi4_ihevc_table_edge_idx_msa[5] = { 1, 2, 0, 3, 4 };

#define BMNZ_V2(RTYPE, in0, in1, in2, in3, in4, in5, out0, out1)         \
{                                                                        \
    out0 = (RTYPE) __msa_bmnz_v((v16u8) in0, (v16u8) in1, (v16u8) in2);  \
    out1 = (RTYPE) __msa_bmnz_v((v16u8) in3, (v16u8) in4, (v16u8) in5);  \
}
#define BMNZ_V2_UB(...) BMNZ_V2(v16u8, __VA_ARGS__)

#define EDGE_OFFSET_CAL2(src_ilv_m0, src_zero0_m, src_ilv_m1, src_zero1_m,  \
                         out0_m, out1_m)                                    \
{                                                                           \
    v16u8 const1_m = (v16u8) __msa_ldi_b(1);                                \
    v16u8 cmp_temp0, cmp_temp1;                                             \
                                                                            \
    cmp_temp0 = ((v16u8) src_zero0_m == (v16u8) src_ilv_m0);                \
    out0_m = __msa_nor_v(cmp_temp0, cmp_temp0);                             \
    cmp_temp0 = ((v16u8) src_ilv_m0 < (v16u8) src_zero0_m);                 \
                                                                            \
    cmp_temp1 = ((v16u8) src_zero1_m == (v16u8) src_ilv_m1);                \
    out1_m = __msa_nor_v(cmp_temp1, cmp_temp1);                             \
    cmp_temp1 = ((v16u8) src_ilv_m1 < (v16u8) src_zero1_m);                 \
                                                                            \
    BMNZ_V2_UB(out0_m, const1_m, cmp_temp0, out1_m, const1_m, cmp_temp1,    \
               out0_m, out1_m);                                             \
}

#define EDGE_OFFSET_CAL4(src_ilv_m0, src_zero0_m, src_ilv_m1, src_zero1_m,  \
                         src_ilv_m2, src_zero2_m, src_ilv_m3, src_zero3_m,  \
                         out0_m, out1_m, out2_m, out3_m)                    \
{                                                                           \
    EDGE_OFFSET_CAL2(src_ilv_m0, src_zero0_m, src_ilv_m1, src_zero1_m,      \
                     out0_m, out1_m);                                       \
    EDGE_OFFSET_CAL2(src_ilv_m2, src_zero2_m, src_ilv_m3, src_zero3_m,      \
                     out2_m, out3_m);                                       \
}

static void hevc_sao_band_filter_4width_msa(UWORD8 *src, WORD32 src_stride,
                                            WORD32 sao_left_class,
                                            WORD8 *sao_offset_val,
                                            WORD32 height)
{
    WORD32 h_cnt, offset_val;
    v16u8 src0, src1, src2, src3;
    v16i8 src0_r, src1_r, offset, mask;
    v16i8 offset0 = { 0 };
    v16i8 offset1 = { 0 };
    v16i8 zero = { 0 };
    v8i16 temp0, temp1, dst0, dst1;

#ifdef CLANG_BUILD
    asm volatile (
    #if (__mips == 64)
        "daddiu  %[sao_offset_val],  %[sao_offset_val],  2  \n\t"
    #else
        "addiu   %[sao_offset_val],  %[sao_offset_val],  2  \n\t"
    #endif

        : [sao_offset_val] "+r" (sao_offset_val)
        :
    );
#else
    sao_offset_val += 1;
#endif

    offset_val = LW(sao_offset_val);
    offset1 = (v16i8) __msa_insert_w((v4i32) offset1, 3, offset_val);
    offset0 = __msa_sld_b(offset1, offset0, 28 - ((sao_left_class) & 31));
    offset1 = __msa_sld_b(zero, offset1, 28 - ((sao_left_class) & 31));

    if(!((sao_left_class > 12) & (sao_left_class < 29)))
    {
        SWAP(offset0, offset1);
    }

    for(h_cnt = height >> 2; h_cnt--;)
    {
        LD_UB4(src, src_stride, src0, src1, src2, src3);
        src += (4 * src_stride);

        ILVEV_D2_SB(src0, src1, src2, src3, src0_r, src1_r);

        src0_r = (v16i8) __msa_pckev_w((v4i32) src1_r, (v4i32) src0_r);
        mask = (v16i8) SRLI_B(src0_r, 3);
        offset = __msa_vshf_b(mask, offset1, offset0);

        UNPCK_SB_SH(offset, temp0, temp1);
        ILVRL_B2_SH(zero, src0_r, dst0, dst1);
        ADD2(dst0, temp0, dst1, temp1, dst0, dst1);
        CLIP_SH2_0_255(dst0, dst1);
        dst0 = (v8i16) __msa_pckev_b((v16i8) dst1, (v16i8) dst0);
        ST4x4_UB(dst0, dst0, 0, 1, 2, 3, src, src_stride);
        src += (4 * src_stride);
    }
}

static void hevc_sao_band_filter_8width_msa(UWORD8 *src, WORD32 src_stride,
                                            WORD32 sao_left_class,
                                            WORD8 *sao_offset_val,
                                            WORD32 height)
{
    WORD32 h_cnt, offset_val;
    v16u8 src0, src1, src2, src3;
    v16i8 src0_r, src1_r, mask0, mask1, offset;
    v16i8 offset0 = { 0 };
    v16i8 offset1 = { 0 };
    v16i8 zero = { 0 };
    v8i16 dst0, dst1, dst2, dst3, temp0, temp1, temp2, temp3;

#ifdef CLANG_BUILD
    asm volatile (
    #if (__mips == 64)
        "daddiu  %[sao_offset_val],  %[sao_offset_val],  2  \n\t"
    #else
        "addiu   %[sao_offset_val],  %[sao_offset_val],  2  \n\t"
    #endif

        : [sao_offset_val] "+r" (sao_offset_val)
        :
    );
#else
    sao_offset_val += 1;
#endif

    offset_val = LW(sao_offset_val);
    offset1 = (v16i8) __msa_insert_w((v4i32) offset1, 3, offset_val);
    offset0 = __msa_sld_b(offset1, offset0, 28 - ((sao_left_class) & 31));
    offset1 = __msa_sld_b(zero, offset1, 28 - ((sao_left_class) & 31));

    if(!((sao_left_class > 12) & (sao_left_class < 29)))
    {
        SWAP(offset0, offset1);
    }

    for(h_cnt = height >> 2; h_cnt--;)
    {
        LD_UB4(src, src_stride, src0, src1, src2, src3);
        src += (4 * src_stride);

        ILVR_D2_SB(src1, src0, src3, src2, src0_r, src1_r);
        mask0 = (v16i8) SRLI_B(src0_r, 3);
        mask1 = (v16i8) SRLI_B(src1_r, 3);
        offset = __msa_vshf_b(mask0, offset1, offset0);
        UNPCK_SB_SH(offset, temp0, temp1);
        offset = __msa_vshf_b(mask1, offset1, offset0);
        UNPCK_SB_SH(offset, temp2, temp3);
        UNPCK_UB_SH(src0_r, dst0, dst1);
        UNPCK_UB_SH(src1_r, dst2, dst3);
        ADD4(dst0, temp0, dst1, temp1, dst2, temp2, dst3, temp3,
             dst0, dst1, dst2, dst3);
        CLIP_SH4_0_255(dst0, dst1, dst2, dst3);
        PCKEV_B2_SH(dst1, dst0, dst3, dst2, dst0, dst2);
        ST8x4_UB(dst0, dst2, src, src_stride);
        src += (4 * src_stride);
    }
}

static void hevc_sao_band_filter_16multiple_msa(UWORD8 *src, WORD32 src_stride,
                                                WORD32 sao_left_class,
                                                WORD8 *sao_offset_val,
                                                WORD32 width, WORD32 height)
{
    WORD32 h_cnt, w_cnt, offset_val, wd_mod16;
    UWORD8 *src_orig = src;
    v16u8 src0, src1, src2, src3;
    v8i16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v16i8 out0, out1, out2, out3, mask0, mask1, mask2, mask3;
    v16i8 tmp0, tmp1, tmp2, tmp3, src0_r, src1_r, offset;
    v16i8 offset0 = { 0 };
    v16i8 offset1 = { 0 };
    v16i8 zero = { 0 };
    v8i16 temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;

#ifdef CLANG_BUILD
    asm volatile (
    #if (__mips == 64)
        "daddiu  %[sao_offset_val],  %[sao_offset_val],  2  \n\t"
    #else
        "addiu   %[sao_offset_val],  %[sao_offset_val],  2  \n\t"
    #endif

        : [sao_offset_val] "+r" (sao_offset_val)
        :
    );
#else
    sao_offset_val += 1;
#endif

    wd_mod16 = width - ((width >> 4) << 4);
    offset_val = LW(sao_offset_val);
    offset1 = (v16i8) __msa_insert_w((v4i32) offset1, 3, offset_val);
    offset0 = __msa_sld_b(offset1, offset0, 28 - ((sao_left_class) & 31));
    offset1 = __msa_sld_b(zero, offset1, 28 - ((sao_left_class) & 31));

    if(!((sao_left_class > 12) & (sao_left_class < 29)))
    {
        SWAP(offset0, offset1);
    }

    for(h_cnt = height >> 2; h_cnt--;)
    {
        src = src_orig;

        for(w_cnt = 0; w_cnt < (width >> 4); w_cnt++)
        {
            LD_UB4(src, src_stride, src0, src1, src2, src3);

            mask0 = (v16i8) SRLI_B(src0, 3);
            mask1 = (v16i8) SRLI_B(src1, 3);
            mask2 = (v16i8) SRLI_B(src2, 3);
            mask3 = (v16i8) SRLI_B(src3, 3);

            VSHF_B2_SB(offset0, offset1, offset0, offset1, mask0, mask1,
                      tmp0, tmp1);
            VSHF_B2_SB(offset0, offset1, offset0, offset1, mask2, mask3,
                      tmp2, tmp3);
            UNPCK_SB_SH(tmp0, temp0, temp1);
            UNPCK_SB_SH(tmp1, temp2, temp3);
            UNPCK_SB_SH(tmp2, temp4, temp5);
            UNPCK_SB_SH(tmp3, temp6, temp7);
            ILVRL_B2_SH(zero, src0, dst0, dst1);
            ILVRL_B2_SH(zero, src1, dst2, dst3);
            ILVRL_B2_SH(zero, src2, dst4, dst5);
            ILVRL_B2_SH(zero, src3, dst6, dst7);
            ADD4(dst0, temp0, dst1, temp1, dst2, temp2, dst3, temp3,
                 dst0, dst1, dst2, dst3);
            ADD4(dst4, temp4, dst5, temp5, dst6, temp6, dst7, temp7,
                 dst4, dst5, dst6, dst7);
            CLIP_SH4_0_255(dst0, dst1, dst2, dst3);
            CLIP_SH4_0_255(dst4, dst5, dst6, dst7);
            PCKEV_B4_SB(dst1, dst0, dst3, dst2, dst5, dst4, dst7, dst6,
                        out0, out1, out2, out3);
            ST_SB4(out0, out1, out2, out3, src, src_stride);
            src += 16;
        }

        if(wd_mod16 % 8)
        {
            LD_UB4(src, src_stride, src0, src1, src2, src3);

            ILVR_D2_SB(src1, src0, src3, src2, src0_r, src1_r);
            mask0 = (v16i8) SRLI_B(src0_r, 3);
            mask1 = (v16i8) SRLI_B(src1_r, 3);
            offset = __msa_vshf_b(mask0, offset1, offset0);
            UNPCK_SB_SH(offset, temp0, temp1);
            offset = __msa_vshf_b(mask1, offset1, offset0);
            UNPCK_SB_SH(offset, temp2, temp3);

            UNPCK_UB_SH(src0_r, dst0, dst1);
            UNPCK_UB_SH(src1_r, dst2, dst3);
            ADD4(dst0, temp0, dst1, temp1, dst2, temp2, dst3, temp3,
                dst0, dst1, dst2, dst3);
            CLIP_SH4_0_255(dst0, dst1, dst2, dst3);
            PCKEV_B2_SH(dst1, dst0, dst3, dst2, dst0, dst2);
            ST8x4_UB(dst0, dst2, src, src_stride);
            src += 8;
            wd_mod16 -= 8;
        }

        if(wd_mod16 % 4)
        {
            LD_UB4(src, src_stride, src0, src1, src2, src3);

            ILVR_D2_SB(src1, src0, src3, src2, src0_r, src1_r);
            mask0 = (v16i8) SRLI_B(src0_r, 3);
            mask1 = (v16i8) SRLI_B(src1_r, 3);
            offset = __msa_vshf_b(mask0, offset1, offset0);
            UNPCK_SB_SH(offset, temp0, temp1);
            offset = __msa_vshf_b(mask1, offset1, offset0);
            UNPCK_SB_SH(offset, temp2, temp3);

            UNPCK_UB_SH(src0_r, dst0, dst1);
            UNPCK_UB_SH(src1_r, dst2, dst3);
            ADD4(dst0, temp0, dst1, temp1, dst2, temp2, dst3, temp3,
                dst0, dst1, dst2, dst3);
            CLIP_SH4_0_255(dst0, dst1, dst2, dst3);
            PCKEV_B2_SH(dst1, dst0, dst3, dst2, dst0, dst2);
            ST8x4_UB(dst0, dst2, src, src_stride);
        }

        src_orig += src_stride << 2;
    }
}


static void hevc_sao_band_chroma_4width_msa(UWORD8 *src, WORD32 src_stride,
                                            WORD32 sao_band_pos_u,
                                            WORD32 sao_band_pos_v,
                                            WORD8 *sao_offset_u,
                                            WORD8 *sao_offset_v,
                                            WORD32 height)
{
    WORD32 h_cnt, val0, val1, val2, val3;
    v16u8 src0, src1, dst0, src0_r, src0_div3;
    v16i8 mask0, mask1, mask2, mask3, offset0;
    v16i8 const1 = __msa_fill_b(1);
    v16i8 const2 = __msa_fill_b(2);
    v16i8 const3 = __msa_fill_b(3);
    v16i8 const4 = __msa_fill_b(4);
    v16i8 edge_idx = { sao_offset_u[0], sao_offset_u[1], sao_offset_u[2],
                       sao_offset_u[3], sao_offset_u[4], sao_offset_v[0],
                       sao_offset_v[1], sao_offset_v[2], sao_offset_v[3],
                       sao_offset_v[4], 0, 0, 0, 0, 0, 0 };
    v16i8 add_coeff = {0, 5, 0, 5, 0, 5, 0, 5, 0, 5, 0, 5, 0, 5, 0, 5};
    v16i8 sao_pair0, sao_pair1, sao_pair2, sao_pair3;
    v8i16 temp0, temp1, temp2, temp3;

    src0 = (v16u8) __msa_fill_b(sao_band_pos_u);
    src1 = (v16u8) __msa_fill_b(sao_band_pos_v);
    sao_pair0 = __msa_ilvev_b((v16i8) src1, (v16i8) src0);
    sao_pair1 = ADDVI_B(sao_pair0, 1);
    sao_pair2 = ADDVI_B(sao_pair0, 2);
    sao_pair3 = ADDVI_B(sao_pair0, 3);

    for(h_cnt = height >> 2; h_cnt--;)
    {
        LW4(src, src_stride, val0, val1, val2, val3);

        offset0 = __msa_ldi_b(0);
        src0_r = (v16u8) __msa_ldi_b(0);
        INSERT_W4_UB(val0, val1, val2, val3, src0_r);
        src0_div3 = (v16u8) SRLI_B(src0_r, 3);

        mask0 = (src0_div3 == sao_pair0);
        mask1 = (src0_div3 == sao_pair1);
        mask2 = (src0_div3 == sao_pair2);
        mask3 = (src0_div3 == sao_pair3);

        offset0 = (v16i8) __msa_bmnz_v((v16u8) offset0, (v16u8) const1,
                                       (v16u8) mask0);
        offset0 = (v16i8) __msa_bmnz_v((v16u8) offset0, (v16u8) const2,
                                       (v16u8) mask1);
        offset0 = (v16i8) __msa_bmnz_v((v16u8) offset0, (v16u8) const3,
                                       (v16u8) mask2);
        offset0 = (v16i8) __msa_bmnz_v((v16u8) offset0, (v16u8) const4,
                                       (v16u8) mask3);
        offset0 += add_coeff;
        offset0 = __msa_vshf_b(offset0, edge_idx, edge_idx);
        UNPCK_UB_SH(src0_r, temp0, temp1);
        UNPCK_SB_SH(offset0, temp2, temp3);
        temp0 += temp2;
        temp1 += temp3;
        CLIP_SH2_0_255(temp0, temp1);
        dst0 = (v16u8) __msa_pckev_b((v16i8) temp1, (v16i8) temp0);

        ST4x4_UB(dst0, dst0, 0, 1, 2, 3, src, src_stride);
        src += (4 * src_stride);
    }
}

static void hevc_sao_band_chroma_8width_msa(UWORD8 *src, WORD32 src_stride,
                                            WORD32 sao_band_pos_u,
                                            WORD32 sao_band_pos_v,
                                            WORD8 *sao_offset_u,
                                            WORD8 *sao_offset_v,
                                            WORD32 height)
{
    WORD32 h_cnt;
    v16u8 src0, src1, src2, src3, dst0, dst1, src0_r, src1_r;
    v16u8 src0_div3, src1_div3;
    v16i8 mask0, mask1, mask2, mask3, offset0, offset1;
    v16i8 const1 = __msa_fill_b(1);
    v16i8 const2 = __msa_fill_b(2);
    v16i8 const3 = __msa_fill_b(3);
    v16i8 const4 = __msa_fill_b(4);
    v16i8 edge_idx = { sao_offset_u[0], sao_offset_u[1], sao_offset_u[2],
                       sao_offset_u[3], sao_offset_u[4], sao_offset_v[0],
                       sao_offset_v[1], sao_offset_v[2], sao_offset_v[3],
                       sao_offset_v[4], 0, 0, 0, 0, 0, 0 };
    v16i8 add_coeff = { 0, 5, 0, 5, 0, 5, 0, 5, 0, 5, 0, 5, 0, 5, 0, 5 };
    v16i8 sao_pair0, sao_pair1, sao_pair2, sao_pair3;
    v8i16 temp0, temp1, temp2, temp3;

    src0 = (v16u8) __msa_fill_b(sao_band_pos_u);
    src1 = (v16u8) __msa_fill_b(sao_band_pos_v);
    sao_pair0 = __msa_ilvev_b((v16i8) src1, (v16i8) src0);
    sao_pair1 = ADDVI_B(sao_pair0, 1);
    sao_pair2 = ADDVI_B(sao_pair0, 2);
    sao_pair3 = ADDVI_B(sao_pair0, 3);

    for(h_cnt = height >> 2; h_cnt--;)
    {
        LD_UB4(src, src_stride, src0, src1, src2, src3);

        offset0 = __msa_ldi_b(0);
        offset1 = __msa_ldi_b(0);
        ILVR_D2_UB(src1, src0, src3, src2, src0_r, src1_r);
        src0_div3 = (v16u8) SRLI_B(src0_r, 3);
        src1_div3 = (v16u8) SRLI_B(src1_r, 3);

        mask0 = (src0_div3 == sao_pair0);
        mask1 = (src0_div3 == sao_pair1);
        mask2 = (src0_div3 == sao_pair2);
        mask3 = (src0_div3 == sao_pair3);

        offset0 = (v16i8) __msa_bmnz_v((v16u8) offset0, (v16u8) const1,
                                       (v16u8) mask0);
        offset0 = (v16i8) __msa_bmnz_v((v16u8) offset0, (v16u8) const2,
                                       (v16u8) mask1);
        offset0 = (v16i8) __msa_bmnz_v((v16u8) offset0, (v16u8) const3,
                                       (v16u8) mask2);
        offset0 = (v16i8) __msa_bmnz_v((v16u8) offset0, (v16u8) const4,
                                       (v16u8) mask3);
        offset0 += add_coeff;
        offset0 = __msa_vshf_b(offset0, edge_idx, edge_idx);
        UNPCK_UB_SH(src0_r, temp0, temp1);
        UNPCK_SB_SH(offset0, temp2, temp3);
        temp0 += temp2;
        temp1 += temp3;
        CLIP_SH2_0_255(temp0, temp1);
        dst0 = (v16u8) __msa_pckev_b((v16i8) temp1, (v16i8) temp0);

        mask0 = (src1_div3 == sao_pair0);
        mask1 = (src1_div3 == sao_pair1);
        mask2 = (src1_div3 == sao_pair2);
        mask3 = (src1_div3 == sao_pair3);

        offset1 = (v16i8) __msa_bmnz_v((v16u8) offset1, (v16u8) const1,
                                       (v16u8) mask0);
        offset1 = (v16i8) __msa_bmnz_v((v16u8) offset1, (v16u8) const2,
                                       (v16u8) mask1);
        offset1 = (v16i8) __msa_bmnz_v((v16u8) offset1, (v16u8) const3,
                                       (v16u8) mask2);
        offset1 = (v16i8) __msa_bmnz_v((v16u8) offset1, (v16u8) const4,
                                       (v16u8) mask3);
        offset1 += add_coeff;
        offset1 = __msa_vshf_b(offset1, edge_idx, edge_idx);
        UNPCK_UB_SH(src1_r, temp0, temp1);
        UNPCK_SB_SH(offset1, temp2, temp3);
        temp0 += temp2;
        temp1 += temp3;
        CLIP_SH2_0_255(temp0, temp1);
        dst1 = (v16u8) __msa_pckev_b((v16i8) temp1, (v16i8) temp0);

        ST8x4_UB(dst0, dst1, src, src_stride);
        src += (4 * src_stride);
    }
}

static void hevc_sao_band_chroma_16width_msa(UWORD8 *src, WORD32 src_stride,
                                             WORD32 sao_band_pos_u,
                                             WORD32 sao_band_pos_v,
                                             WORD8 *sao_offset_u,
                                             WORD8 *sao_offset_v,
                                             WORD32 width, WORD32 height)
{
    UWORD8 *src_orig = src;
    WORD32 w_cnt, h_cnt;
    v16u8 src0, src1, dst0, dst1, src0_div3, src1_div3;
    v16i8 mask0, mask1, mask2, mask3, offset0, offset1;
    v16i8 sao_pair0, sao_pair1, sao_pair2, sao_pair3;
    v16i8 const1 = __msa_fill_b(1);
    v16i8 const2 = __msa_fill_b(2);
    v16i8 const3 = __msa_fill_b(3);
    v16i8 const4 = __msa_fill_b(4);
    v16i8 edge_idx = { sao_offset_u[0], sao_offset_u[1], sao_offset_u[2],
                       sao_offset_u[3], sao_offset_u[4], sao_offset_v[0],
                       sao_offset_v[1], sao_offset_v[2], sao_offset_v[3],
                       sao_offset_v[4], 0, 0, 0, 0, 0, 0 };
    v16i8 add_coeff = { 0, 5, 0, 5, 0, 5, 0, 5, 0, 5, 0, 5, 0, 5, 0, 5 };
    v8i16 temp0, temp1, temp2, temp3;

    src0 = (v16u8) __msa_fill_b(sao_band_pos_u);
    src1 = (v16u8) __msa_fill_b(sao_band_pos_v);
    sao_pair0 = __msa_ilvev_b((v16i8) src1, (v16i8) src0);
    sao_pair1 = ADDVI_B(sao_pair0, 1);
    sao_pair2 = ADDVI_B(sao_pair0, 2);
    sao_pair3 = ADDVI_B(sao_pair0, 3);

    sao_pair0 = (v16i8) ANDI_B(sao_pair0, 31);
    sao_pair1 = (v16i8) ANDI_B(sao_pair1, 31);
    sao_pair2 = (v16i8) ANDI_B(sao_pair2, 31);
    sao_pair3 = (v16i8) ANDI_B(sao_pair3, 31);

    for(w_cnt = 0; w_cnt < width; w_cnt+=16)
    {
        src = src_orig + w_cnt;

        for(h_cnt = height >> 1; h_cnt--;)
        {
            LD_UB2(src, src_stride, src0, src1);

            offset0 = __msa_ldi_b(0);
            offset1 = __msa_ldi_b(0);
            src0_div3 = (v16u8) SRLI_B(src0, 3);
            src1_div3 = (v16u8) SRLI_B(src1, 3);

            mask0 = (src0_div3 == sao_pair0);
            mask1 = (src0_div3 == sao_pair1);
            mask2 = (src0_div3 == sao_pair2);
            mask3 = (src0_div3 == sao_pair3);

            offset0 = (v16i8) __msa_bmnz_v((v16u8) offset0, (v16u8) const1,
                                           (v16u8) mask0);
            offset0 = (v16i8) __msa_bmnz_v((v16u8) offset0, (v16u8) const2,
                                           (v16u8) mask1);
            offset0 = (v16i8) __msa_bmnz_v((v16u8) offset0, (v16u8) const3,
                                           (v16u8) mask2);
            offset0 = (v16i8) __msa_bmnz_v((v16u8) offset0, (v16u8) const4,
                                           (v16u8) mask3);
            offset0 += add_coeff;
            offset0 = __msa_vshf_b(offset0, edge_idx, edge_idx);
            UNPCK_UB_SH(src0, temp0, temp1);
            UNPCK_SB_SH(offset0, temp2, temp3);
            temp0 += temp2;
            temp1 += temp3;
            CLIP_SH2_0_255(temp0, temp1);
            dst0 = (v16u8) __msa_pckev_b((v16i8) temp1, (v16i8) temp0);

            mask0 = (src1_div3 == sao_pair0);
            mask1 = (src1_div3 == sao_pair1);
            mask2 = (src1_div3 == sao_pair2);
            mask3 = (src1_div3 == sao_pair3);

            offset1 = (v16i8) __msa_bmnz_v((v16u8) offset1, (v16u8) const1,
                                           (v16u8) mask0);
            offset1 = (v16i8) __msa_bmnz_v((v16u8) offset1, (v16u8) const2,
                                           (v16u8) mask1);
            offset1 = (v16i8) __msa_bmnz_v((v16u8) offset1, (v16u8) const3,
                                           (v16u8) mask2);
            offset1 = (v16i8) __msa_bmnz_v((v16u8) offset1, (v16u8) const4,
                                           (v16u8) mask3);
            offset1 += add_coeff;
            offset1 = __msa_vshf_b(offset1, edge_idx, edge_idx);
            UNPCK_UB_SH(src1, temp0, temp1);
            UNPCK_SB_SH(offset1, temp2, temp3);
            temp0 += temp2;
            temp1 += temp3;
            CLIP_SH2_0_255(temp0, temp1);
            dst1 = (v16u8) __msa_pckev_b((v16i8) temp1, (v16i8) temp0);

            ST_UB2(dst0, dst1, src, src_stride);
            src += (2 * src_stride);
        }
    }
}

static void hevc_sao_edge_0deg_16w_in_place_msa(UWORD8 *src, WORD32 src_stride,
                                                UWORD8 *left, UWORD8 *mask,
                                                WORD8 *sao_offset_val,
                                                WORD32 width, WORD32 height)
{
    UWORD8 *dst = src;
    WORD32 dst_stride = src_stride;
    UWORD8 *dst_ptr, *src_minus1;
    WORD32 h_cnt, v_cnt;
    v16i8 edge_idx = { sao_offset_val[1], sao_offset_val[2], 0,
                       sao_offset_val[3], sao_offset_val[4] };
    v16u8 diff_minus10, diff_plus10, diff_minus11, diff_plus11;
    v16u8 diff_minus12, diff_plus12, diff_minus13, diff_plus13;
    v16u8 src10, src11, src12, src13, dst0, dst1, dst2, dst3;
    v16u8 src_minus10, src_minus11, src_minus12, src_minus13;
    v16i8 offset_mask0, offset_mask1, offset_mask2, offset_mask3;
    v16i8 src_zero0, src_zero1, src_zero2, src_zero3;
    v16i8 src_plus10, src_plus11, src_plus12, src_plus13;
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7;
    v8i16 temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;

    /* Find multiple of 16 width count */
    /* 12 width case is also handelled in multiple of 16 loop */
    WORD32 iSimd16PxlCount = ((width + 4) >> 4) << 4;
    if(iSimd16PxlCount > width)
    {
        /* Set 4 masks after width to zero */
        /*This is to handle multiple of 12 case*/
        *((WORD32 *) (mask + width)) = 0;
    }

    for(h_cnt = 0 ; h_cnt < (height >> 2) << 2; h_cnt += 4)
    {
        src_minus1 = src - 1;
        LD_UB4(src_minus1, src_stride,
               src_minus10, src_minus11, src_minus12, src_minus13);
        src_minus10 = (v16u8) __msa_insert_b((v16i8) src_minus10, 0,
                                             left[h_cnt + 0]);
        src_minus11 = (v16u8) __msa_insert_b((v16i8) src_minus11, 0,
                                             left[h_cnt + 1]);
        src_minus12 = (v16u8) __msa_insert_b((v16i8) src_minus12, 0,
                                             left[h_cnt + 2]);
        src_minus13 = (v16u8) __msa_insert_b((v16i8) src_minus13, 0,
                                             left[h_cnt + 3]);

        for(v_cnt = 0; v_cnt < iSimd16PxlCount; v_cnt += 16)
        {
            src_minus1 += 16;
            dst_ptr = dst + v_cnt;
            LD_UB4(src_minus1, src_stride, src10, src11, src12, src13);
            SLDI_B2_SB(src10, src11, src_minus10, src_minus11, src_zero0,
                       src_zero1, 1);
            SLDI_B2_SB(src12, src13, src_minus12, src_minus13, src_zero2,
                       src_zero3, 1);
            SLDI_B2_SB(src10, src11, src_minus10, src_minus11, src_plus10,
                       src_plus11, 2);
            SLDI_B2_SB(src12, src13, src_minus12, src_minus13, src_plus12,
                       src_plus13, 2);
            EDGE_OFFSET_CAL4(src_minus10, src_zero0, src_minus11, src_zero1,
                             src_minus12, src_zero2, src_minus13, src_zero3,
                             diff_minus10, diff_minus11, diff_minus12,
                             diff_minus13);
            EDGE_OFFSET_CAL4(src_plus10, src_zero0, src_plus11, src_zero1,
                             src_plus12, src_zero2, src_plus13, src_zero3,
                             diff_plus10, diff_plus11, diff_plus12,
                             diff_plus13);

            offset_mask0 = (v16i8) diff_minus10 + (v16i8) diff_plus10;
            offset_mask1 = (v16i8) diff_minus11 + (v16i8) diff_plus11;
            offset_mask2 = (v16i8) diff_minus12 + (v16i8) diff_plus12;
            offset_mask3 = (v16i8) diff_minus13 + (v16i8) diff_plus13;
            offset_mask0 = ADDVI_B(offset_mask0, 2);
            offset_mask1 = ADDVI_B(offset_mask1, 2);
            offset_mask2 = ADDVI_B(offset_mask2, 2);
            offset_mask3 = ADDVI_B(offset_mask3, 2);
            VSHF_B4_SB(edge_idx, edge_idx, offset_mask0, offset_mask1,
                       offset_mask2, offset_mask3, offset_mask0, offset_mask1,
                       offset_mask2, offset_mask3);
            offset_mask0 = offset_mask0 & LD_SB(mask + v_cnt);
            offset_mask1 = offset_mask1 & LD_SB(mask + v_cnt);
            offset_mask2 = offset_mask2 & LD_SB(mask + v_cnt);
            offset_mask3 = offset_mask3 & LD_SB(mask + v_cnt);

            UNPCK_UB_SH(src_zero0, src0, src1);
            UNPCK_SB_SH(offset_mask0, temp0, temp1);
            UNPCK_UB_SH(src_zero1, src2, src3);
            UNPCK_SB_SH(offset_mask1, temp2, temp3);
            UNPCK_UB_SH(src_zero2, src4, src5);
            UNPCK_SB_SH(offset_mask2, temp4, temp5);
            UNPCK_UB_SH(src_zero3, src6, src7);
            UNPCK_SB_SH(offset_mask3, temp6, temp7);
            ADD4(temp0, src0, temp1, src1, temp2, src2, temp3, src3, temp0,
                 temp1, temp2, temp3);
            ADD4(temp4, src4, temp5, src5, temp6, src6, temp7, src7, temp4,
                 temp5, temp6, temp7);
            CLIP_SH4_0_255(temp0, temp1, temp2, temp3);
            CLIP_SH4_0_255(temp4, temp5, temp6, temp7);
            PCKEV_B4_UB(temp1, temp0, temp3, temp2, temp5, temp4, temp7, temp6,
                        dst0, dst1, dst2, dst3);

            src_minus10 = src10;
            ST_UB(dst0, dst_ptr);
            src_minus11 = src11;
            ST_UB(dst1, dst_ptr + dst_stride);
            src_minus12 = src12;
            ST_UB(dst2, dst_ptr + (dst_stride << 1));
            src_minus13 = src13;
            ST_UB(dst3, dst_ptr + (dst_stride * 3));
        }

        /* Handle if remaining width = 8 */
        if(8 == (width - iSimd16PxlCount))
        {
            /* We have already loaded data in src_minus10, src_minus11,
               src_minus12, src_minus13 registers */
            dst_ptr = dst + v_cnt;
            v16u8 mask_reg = LD_UB(mask + v_cnt);

            SLDI_B2_SB(src_minus10, src_minus11, src_minus10, src_minus11,
                       src_zero0, src_zero1, 1);
            SLDI_B2_SB(src_minus12, src_minus13, src_minus12, src_minus13,
                       src_zero2, src_zero3, 1);
            SLDI_B2_SB(src_minus10, src_minus11, src_minus10, src_minus11,
                       src_plus10, src_plus11, 2);
            SLDI_B2_SB(src_minus12, src_minus13, src_minus12, src_minus13,
                       src_plus12, src_plus13, 2);
            ILVR_D2_UB(src_minus11, src_minus10, src_minus13, src_minus12,
                       src_minus10, src_minus12);
            ILVR_D2_SB(src_zero1, src_zero0, src_zero3, src_zero2,
                       src_zero0, src_zero2);
            ILVR_D2_SB(src_plus11, src_plus10, src_plus13, src_plus12,
                       src_plus10, src_plus12);
            mask_reg = (v16u8) __msa_ilvr_d((v2i64) mask_reg, (v2i64) mask_reg);

            EDGE_OFFSET_CAL4(src_minus10, src_zero0, src_plus10, src_zero0,
                             src_minus12, src_zero2, src_plus12, src_zero2,
                             diff_minus10, diff_plus10, diff_minus12,
                             diff_plus12);

            offset_mask0 = (v16i8) diff_minus10 + (v16i8) diff_plus10;
            offset_mask2 = (v16i8) diff_minus12 + (v16i8) diff_plus12;
            offset_mask0 = ADDVI_B(offset_mask0, 2);
            offset_mask2 = ADDVI_B(offset_mask2, 2);
            offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
            offset_mask2 = __msa_vshf_b(offset_mask2, edge_idx, edge_idx);
            offset_mask0 &= mask_reg;
            offset_mask2 = offset_mask2 & mask_reg;

            UNPCK_UB_SH(src_zero0, src0, src1);
            UNPCK_SB_SH(offset_mask0, temp0, temp1);
            UNPCK_UB_SH(src_zero2, src4, src5);
            UNPCK_SB_SH(offset_mask2, temp4, temp5);
            ADD4(temp0, src0, temp1, src1, temp4, src4, temp5, src5, temp0,
                 temp1, temp4, temp5);
            CLIP_SH4_0_255(temp0, temp1, temp4, temp5);
            PCKEV_B2_UB(temp1, temp0, temp5, temp4, dst0, dst2);
            ST8x4_UB(dst0, dst2, dst_ptr, dst_stride);
        }
        else if(4 == (width - iSimd16PxlCount)) /* if remaining width = 4 */
        {
            /* We have already loaded data in src_minus10, src_minus11,
               src_minus12, src_minus13 registers */
            dst_ptr = dst + v_cnt;
            v16u8 mask_reg = LD_UB(mask + v_cnt);
            SLDI_B2_SB(src_minus10, src_minus11, src_minus10, src_minus11,
                       src_zero0, src_zero1, 1);
            SLDI_B2_SB(src_minus12, src_minus13, src_minus12, src_minus13,
                       src_zero2, src_zero3, 1);
            SLDI_B2_SB(src_minus10, src_minus11, src_minus10, src_minus11,
                       src_plus10, src_plus11, 2);
            SLDI_B2_SB(src_minus12, src_minus13, src_minus12, src_minus13,
                       src_plus12, src_plus13, 2);

            ILVR_W2_UB(src_minus11, src_minus10, src_minus13, src_minus12,
                       src_minus10, src_minus12);
            ILVR_W2_SB(src_zero1, src_zero0, src_zero3, src_zero2,
                       src_zero0, src_zero2);
            ILVR_W2_SB(src_plus11, src_plus10, src_plus13, src_plus12,
                       src_plus10, src_plus12);
            mask_reg = (v16u8) __msa_ilvr_w((v4i32) mask_reg, (v4i32) mask_reg);
            ILVR_D2_SB(src_plus12, src_plus10, src_zero2, src_zero0,
                       src_plus10, src_zero0);
            ILVR_D2_UB(src_minus12, src_minus10, mask_reg, mask_reg,
                       src_minus10, mask_reg);
            EDGE_OFFSET_CAL2(src_minus10, src_zero0, src_plus10, src_zero0,
                             diff_minus10, diff_plus10);

            offset_mask0 = (v16i8) diff_minus10 + (v16i8) diff_plus10;
            offset_mask0 = ADDVI_B(offset_mask0, 2);
            offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
            offset_mask0 &= mask_reg;
            UNPCK_UB_SH(src_zero0, src0, src1);
            UNPCK_SB_SH(offset_mask0, temp0, temp1);
            ADD2(temp0, src0, temp1, src1, temp0, temp1);
            CLIP_SH2_0_255(temp0, temp1);
            dst0 = (v16u8) __msa_pckev_b((v16i8) temp1, (v16i8) temp0);
            ST4x4_UB(dst0, dst0, 0, 1, 2, 3, dst_ptr, dst_stride);
        }

        src += (src_stride << 2);
        dst += (dst_stride << 2);
    }
}

static void hevc_sao_edge_0deg_8w_in_place_msa(UWORD8 *src, WORD32 src_stride,
                                               UWORD8 *left, UWORD8 *mask,
                                               WORD8 *sao_offset_val,
                                               WORD32 height)
{
    UWORD8 *dst = src;
    WORD32 dst_stride = src_stride;
    WORD32 h_cnt;
    v16i8 edge_idx = { sao_offset_val[1], sao_offset_val[2], 0,
                       sao_offset_val[3], sao_offset_val[4] };
    v16u8 diff_minus10, diff_plus10, diff_minus12, diff_plus12;
    v16u8 dst0, dst2;
    v16u8 src_minus10, src_minus11, src_minus12, src_minus13;
    v16i8 offset_mask0, offset_mask2;
    v16i8 src_zero0, src_zero1, src_zero2, src_zero3;
    v16i8 src_plus10, src_plus11, src_plus12, src_plus13;
    v8i16 src0, src1, src2, src3, temp0, temp1, temp2, temp3;
    v16u8 mask_reg;

    mask_reg = LD_UB(mask);
    mask_reg = (v16u8) __msa_ilvr_d((v2i64) mask_reg, (v2i64) mask_reg);

    for(h_cnt = 0; h_cnt < height; h_cnt += 4)
    {
        LD_UB4(src - 1, src_stride,
               src_minus10, src_minus11, src_minus12, src_minus13);

        src_minus10 = (v16u8) __msa_insert_b((v16i8) src_minus10, 0,
                                             left[h_cnt+0]);
        src_minus11 = (v16u8) __msa_insert_b((v16i8) src_minus11, 0,
                                             left[h_cnt+1]);
        src_minus12 = (v16u8) __msa_insert_b((v16i8) src_minus12, 0,
                                             left[h_cnt+2]);
        src_minus13 = (v16u8) __msa_insert_b((v16i8) src_minus13, 0,
                                             left[h_cnt+3]);

        SLDI_B2_SB(src_minus10, src_minus11, src_minus10, src_minus11,
                   src_zero0, src_zero1, 1);
        SLDI_B2_SB(src_minus12, src_minus13, src_minus12, src_minus13,
                   src_zero2, src_zero3, 1);
        SLDI_B2_SB(src_minus10, src_minus11, src_minus10, src_minus11,
                   src_plus10, src_plus11, 2);
        SLDI_B2_SB(src_minus12, src_minus13, src_minus12, src_minus13,
                   src_plus12, src_plus13, 2);
        ILVR_D2_UB(src_minus11, src_minus10, src_minus13, src_minus12,
                   src_minus10, src_minus12);
        ILVR_D2_SB(src_zero1, src_zero0, src_zero3, src_zero2,
                   src_zero0, src_zero2);
        ILVR_D2_SB(src_plus11, src_plus10, src_plus13, src_plus12,
                   src_plus10, src_plus12);
        mask_reg = (v16u8) __msa_ilvr_d((v2i64) mask_reg, (v2i64) mask_reg);
        EDGE_OFFSET_CAL4(src_minus10, src_zero0, src_plus10, src_zero0,
                         src_minus12, src_zero2, src_plus12, src_zero2,
                         diff_minus10, diff_plus10, diff_minus12,
                         diff_plus12);

        offset_mask0 = (v16i8) diff_minus10 + (v16i8) diff_plus10;
        offset_mask2 = (v16i8) diff_minus12 + (v16i8) diff_plus12;
        offset_mask0 = ADDVI_B(offset_mask0, 2);
        offset_mask2 = ADDVI_B(offset_mask2, 2);
        offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
        offset_mask2 = __msa_vshf_b(offset_mask2, edge_idx, edge_idx);
        offset_mask0 &= mask_reg;
        offset_mask2 = offset_mask2 & mask_reg;

        UNPCK_UB_SH(src_zero0, src0, src1);
        UNPCK_SB_SH(offset_mask0, temp0, temp1);
        UNPCK_UB_SH(src_zero2, src2, src3);
        UNPCK_SB_SH(offset_mask2, temp2, temp3);
        ADD4(temp0, src0, temp1, src1, temp2, src2, temp3, src3, temp0,
             temp1, temp2, temp3);
        CLIP_SH4_0_255(temp0, temp1, temp2, temp3);
        PCKEV_B2_UB(temp1, temp0, temp3, temp2, dst0, dst2);
        ST8x4_UB(dst0, dst2, dst, dst_stride);
        src += (src_stride << 2);
        dst += (dst_stride << 2);
    }
}

static void hevc_sao_edge_0deg_4w_in_place_msa(UWORD8 *src, WORD32 src_stride,
                                               UWORD8 *left, UWORD8 *mask,
                                               WORD8 *sao_offset_val,
                                               WORD32 height)
{
    UWORD8 *dst = src;
    WORD32 dst_stride = src_stride;
    WORD32 h_cnt;
    v16i8 edge_idx = { sao_offset_val[1], sao_offset_val[2], 0,
                       sao_offset_val[3], sao_offset_val[4] };
    v16u8 diff_minus10, diff_plus10;
    v16u8 src_minus10, src_minus11, src_minus12, src_minus13, dst0, mask_reg;
    v16i8 src_zero0, src_zero1, src_zero2, src_zero3, offset_mask;
    v16i8 src_plus10, src_plus11, src_plus12, src_plus13;
    v8i16 src0, src1, temp0, temp1;

    mask_reg = LD_UB(mask);
    mask_reg = (v16u8) __msa_ilvr_w((v4i32) mask_reg, (v4i32) mask_reg);
    for(h_cnt = 0; h_cnt < height; h_cnt += 4)
    {
        LD_UB4(src - 1, src_stride,
               src_minus10, src_minus11, src_minus12, src_minus13);
        src_minus10 = (v16u8) __msa_insert_b((v16i8) src_minus10, 0,
                                             left[h_cnt+0]);
        src_minus11 = (v16u8) __msa_insert_b((v16i8) src_minus11, 0,
                                             left[h_cnt+1]);
        src_minus12 = (v16u8) __msa_insert_b((v16i8) src_minus12, 0,
                                             left[h_cnt+2]);
        src_minus13 = (v16u8) __msa_insert_b((v16i8) src_minus13, 0,
                                             left[h_cnt+3]);

        SLDI_B2_SB(src_minus10, src_minus11, src_minus10, src_minus11,
                   src_zero0, src_zero1, 1);
        SLDI_B2_SB(src_minus12, src_minus13, src_minus12, src_minus13,
                   src_zero2, src_zero3, 1);
        SLDI_B2_SB(src_minus10, src_minus11, src_minus10, src_minus11,
                   src_plus10, src_plus11, 2);
        SLDI_B2_SB(src_minus12, src_minus13, src_minus12, src_minus13,
                   src_plus12, src_plus13, 2);
        ILVR_W2_UB(src_minus11, src_minus10, src_minus13, src_minus12,
                   src_minus10, src_minus12);
        ILVR_W2_SB(src_zero1, src_zero0, src_zero3, src_zero2,
                   src_zero0, src_zero2);
        ILVR_W2_SB(src_plus11, src_plus10, src_plus13, src_plus12,
                   src_plus10, src_plus12);

        ILVR_D2_SB(src_plus12, src_plus10, src_zero2, src_zero0,
                   src_plus10, src_zero0);
        src_minus10 = (v16u8) __msa_ilvr_d((v2i64) src_minus12,
                                           (v2i64) src_minus10);
        EDGE_OFFSET_CAL2(src_minus10, src_zero0, src_plus10, src_zero0,
                         diff_minus10, diff_plus10);

        offset_mask = (v16i8) diff_minus10 + (v16i8) diff_plus10;
        offset_mask = ADDVI_B(offset_mask, 2);
        offset_mask = __msa_vshf_b(offset_mask, edge_idx, edge_idx);
        offset_mask &= mask_reg;
        UNPCK_UB_SH(src_zero0, src0, src1);
        UNPCK_SB_SH(offset_mask, temp0, temp1);
        ADD2(temp0, src0, temp1, src1, temp0, temp1);
        CLIP_SH2_0_255(temp0, temp1);
        dst0 = (v16u8) __msa_pckev_b((v16i8) temp1, (v16i8) temp0);
        ST4x4_UB(dst0, dst0, 0, 1, 2, 3, dst, dst_stride);
        src += (src_stride << 2);
        dst += (dst_stride << 2);
    }
}

static void hevc_sao_edge_90deg_16w_in_place_msa(UWORD8 *src, WORD32 src_stride,
                                                 UWORD8 *top,
                                                 WORD8 *sao_offset_val,
                                                 WORD32 width, WORD32 height)
{
    UWORD8 *dst = src;
    WORD32 dst_stride = src_stride;
    UWORD8 *src_orig = src;
    UWORD8 *dst_orig = dst;
    WORD32 h_cnt, v_cnt;
    v16i8 edge_idx = { sao_offset_val[1], sao_offset_val[2], 0,
                       sao_offset_val[3], sao_offset_val[4],
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    v16u8 const1 = (v16u8) __msa_ldi_b(1);
    v16u8 cmp_minus10, cmp_plus10, diff_minus10, diff_plus10, diff_minus11;
    v16u8 diff_plus11, diff_minus12, diff_plus12, diff_minus13, diff_plus13;
    v16u8 src10, src_minus10, dst0, src11, src_minus11, dst1;
    v16u8 src12, dst2, src13, dst3;
    v16i8 offset_mask0, offset_mask1, offset_mask2, offset_mask3;
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7;
    v8i16 temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;

    for(v_cnt = 0; v_cnt < ((width >> 4) << 4); v_cnt += 16)
    {
        src = src_orig + v_cnt;
        dst = dst_orig + v_cnt;
        /* Load from top pointer and first row */
        src_minus10 = LD_UB(top + v_cnt);
        src_minus11 = LD_UB(src);
        /* Calculate signUp in advance */
        cmp_minus10 = (src_minus11 == src_minus10);
        diff_minus10 = __msa_nor_v(cmp_minus10, cmp_minus10);
        cmp_minus10 = (src_minus10 < src_minus11);
        diff_minus10 = __msa_bmnz_v(diff_minus10, const1, cmp_minus10);

        /* Process 4 rows at a time. */
        for(h_cnt = 0 ; h_cnt < (height >> 2) << 2; h_cnt += 4)
        {
            LD_UB4(src + src_stride, src_stride, src10, src11, src12, src13);
            EDGE_OFFSET_CAL4(src_minus10, src_minus11, src10, src_minus11,
                             src_minus11, src10, src11, src10, diff_minus10,
                             diff_plus10, diff_minus11, diff_plus11);
            EDGE_OFFSET_CAL4(src10, src11, src12, src11, src11, src12,
                             src13, src12, diff_minus12, diff_plus12,
                             diff_minus13, diff_plus13);
            offset_mask0 = (v16i8) diff_minus10 + (v16i8) diff_plus10;
            offset_mask1 = (v16i8) diff_minus11 + (v16i8) diff_plus11;
            offset_mask2 = (v16i8) diff_minus12 + (v16i8) diff_plus12;
            offset_mask3 = (v16i8) diff_minus13 + (v16i8) diff_plus13;
            offset_mask0 = ADDVI_B(offset_mask0, 2);
            offset_mask1 = ADDVI_B(offset_mask1, 2);
            offset_mask2 = ADDVI_B(offset_mask2, 2);
            offset_mask3 = ADDVI_B(offset_mask3, 2);
            VSHF_B4_SB(edge_idx, edge_idx, offset_mask0, offset_mask1,
                       offset_mask2, offset_mask3, offset_mask0, offset_mask1,
                       offset_mask2, offset_mask3);
            UNPCK_UB_SH(src_minus11, src0, src1);
            UNPCK_SB_SH(offset_mask0, temp0, temp1);
            UNPCK_UB_SH(src10, src2, src3);
            UNPCK_SB_SH(offset_mask1, temp2, temp3);
            UNPCK_UB_SH(src11, src4, src5);
            UNPCK_SB_SH(offset_mask2, temp4, temp5);
            UNPCK_UB_SH(src12, src6, src7);
            UNPCK_SB_SH(offset_mask3, temp6, temp7);
            ADD4(temp0, src0, temp1, src1, temp2, src2, temp3, src3, temp0,
                 temp1, temp2, temp3);
            ADD4(temp4, src4, temp5, src5, temp6, src6, temp7, src7, temp4,
                 temp5, temp6, temp7);
            CLIP_SH4_0_255(temp0, temp1, temp2, temp3);
            CLIP_SH4_0_255(temp4, temp5, temp6, temp7);
            PCKEV_B4_UB(temp1, temp0, temp3, temp2, temp5, temp4, temp7, temp6,
                        dst0, dst1, dst2, dst3);

            src_minus10 = src12;
            src_minus11 = src13;
            /* Reuse sign for next iteration*/
            diff_minus10 = -diff_plus13;
            ST_UB4(dst0, dst1, dst2, dst3, dst, dst_stride);
            src += (src_stride << 2);
            dst += (dst_stride << 2);
        }

        /* Process remaining rows, 1 row at a time */
        for( ; h_cnt < height; h_cnt++)
        {
            src10 = LD_UB(src + src_stride);
            cmp_plus10 = (src_minus11 == src10);
            diff_plus10 = __msa_nor_v(cmp_plus10, cmp_plus10);
            cmp_plus10 = (src10 < src_minus11);
            diff_plus10 = __msa_bmnz_v(diff_plus10, const1, cmp_plus10);
            offset_mask0 = (v16i8) diff_minus10 + (v16i8) diff_plus10;
            offset_mask0 = ADDVI_B(offset_mask0, 2);
            offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
            UNPCK_UB_SH(src_minus11, src0, src1);
            UNPCK_SB_SH(offset_mask0, temp0, temp1);

            ADD2(temp0, src0, temp1, src1, temp0, temp1);
            CLIP_SH2_0_255(temp0, temp1);
            dst0 = (v16u8) __msa_pckev_b((v16i8) temp1, (v16i8) temp0);
            src_minus10 = src_minus11;
            src_minus11 = src10;

            /* Reuse sign for next iteration*/
            diff_minus10 = -diff_plus10;
            ST_UB(dst0, dst);
            src += src_stride;
            dst += dst_stride;
        }
    }
}

static void hevc_sao_edge_90deg_8w_in_place_msa(UWORD8 *src, WORD32 src_stride,
                                                UWORD8 *top,
                                                WORD8 *sao_offset_val,
                                                WORD32 height)
{
    UWORD8 *dst = src;
    WORD32 dst_stride = src_stride;
    WORD32 h_cnt;
    uint64_t dst_val0, dst_val1;
    v8i16 edge_idx = { sao_offset_val[1], sao_offset_val[2], 0,
                       sao_offset_val[3], sao_offset_val[4], 0, 0, 0 };
    v16u8 const1 = (v16u8) __msa_ldi_b(1);
    v16i8 zero = { 0 };
    v16i8 src_zero0, src_zero1, dst0, dst1;
    v16u8 cmp_minus10, diff_minus10, diff_minus11;
    v16u8 src_minus10, src_minus11, src10, src11;
    v8i16 src00, offset_mask0, src01, offset_mask1;

    src_minus10 = LD_UB(top);
    src_minus11 = LD_UB(src);
    /* Process 2 rows at a time */
    for(h_cnt = 0; h_cnt < ((height >> 1) << 1); h_cnt += 2)
    {
        LD_UB2(src + src_stride, src_stride, src10, src11);
        src_minus10 = (v16u8) __msa_ilvr_b((v16i8) src10, (v16i8) src_minus10);
        src_zero0 = __msa_ilvr_b((v16i8) src_minus11, (v16i8) src_minus11);
        src_minus11 = (v16u8) __msa_ilvr_b((v16i8) src11, (v16i8) src_minus11);
        src_zero1 = __msa_ilvr_b((v16i8) src10, (v16i8) src10);
        EDGE_OFFSET_CAL2(src_minus10, src_zero0, src_minus11, src_zero1,
                         diff_minus10, diff_minus11);

        offset_mask0 = (v8i16) __msa_hadd_u_h(diff_minus10, diff_minus10);
        offset_mask1 = (v8i16) __msa_hadd_u_h(diff_minus11, diff_minus11);
        offset_mask0 = ADDVI_H(offset_mask0, 2);
        offset_mask1 = ADDVI_H(offset_mask1, 2);
        offset_mask0 = __msa_vshf_h(offset_mask0, edge_idx, edge_idx);
        offset_mask1 = __msa_vshf_h(offset_mask1, edge_idx, edge_idx);
        ILVEV_B2_SH(src_zero0, zero, src_zero1, zero, src00, src01);
        ADD2(offset_mask0, src00, offset_mask1, src01, offset_mask0,
             offset_mask1);
        CLIP_SH2_0_255(offset_mask0, offset_mask1);
        PCKEV_B2_SB(offset_mask0, offset_mask0, offset_mask1, offset_mask1,
                    dst0, dst1);

        src_minus10 = src10;
        src_minus11 = src11;
        dst_val0 = __msa_copy_u_d((v2i64) dst0, 0);
        dst_val1 = __msa_copy_u_d((v2i64) dst1, 0);
        SD(dst_val0, dst);
        dst += dst_stride;
        SD(dst_val1, dst);
        dst += dst_stride;
        src += (src_stride << 1);
    }

    /* Process remaining row if there */
    if(h_cnt < height)
    {
        src10 = LD_UB(src + src_stride);
        src_minus10 = (v16u8) __msa_ilvr_b((v16i8) src10, (v16i8) src_minus10);
        src_zero0 = __msa_ilvr_b((v16i8) src_minus11, (v16i8) src_minus11);

        cmp_minus10 = ((v16u8) src_zero0 == src_minus10);
        diff_minus10 = __msa_nor_v(cmp_minus10, cmp_minus10);
        cmp_minus10 = (src_minus10 < (v16u8) src_zero0);
        diff_minus10 = __msa_bmnz_v(diff_minus10, const1, cmp_minus10);
        offset_mask0 = (v8i16) __msa_hadd_u_h(diff_minus10, diff_minus10);
        offset_mask0 = ADDVI_H(offset_mask0, 2);
        offset_mask0 = __msa_vshf_h(offset_mask0, edge_idx, edge_idx);
        src00 = (v8i16) __msa_ilvev_b((v16i8) zero, (v16i8) src_zero0);

        offset_mask0 += src00;
        offset_mask0 = CLIP_SH_0_255(offset_mask0);
        dst0 = __msa_pckev_b((v16i8) offset_mask0, (v16i8) offset_mask0);
        dst_val0 = __msa_copy_u_d((v2i64) dst0, 0);
        SD(dst_val0, dst);
    }
}

static void hevc_sao_edge_90deg_4w_in_place_msa(UWORD8 *src, WORD32 src_stride,
                                                UWORD8 *top,
                                                WORD8 *sao_offset_val,
                                                WORD32 height)
{
    WORD32 h_cnt;
    UWORD8 *dst = src;
    WORD32 dst_stride = src_stride;
    UWORD32 dst_val0, dst_val1;
    v8i16 edge_idx = { sao_offset_val[1], sao_offset_val[2], 0,
                       sao_offset_val[3], sao_offset_val[4], 0, 0, 0 };
    v16u8 const1 = (v16u8) __msa_ldi_b(1);
    v16i8 src_zero0, src_zero1, dst0;
    v16i8 zero = { 0 };
    v16u8 cmp_minus10, diff_minus10, diff_minus11;
    v16u8 src_minus10, src_minus11, src10, src11;
    v8i16 sao_offset, src00, src01, offset_mask0, offset_mask1;

    sao_offset = LD_SH(sao_offset_val);
    sao_offset = (v8i16) __msa_ilvr_b((v16i8) zero, (v16i8) sao_offset);
    src_minus10 = LD_UB(top);
    src_minus11 = LD_UB(src);
    for(h_cnt = 0; h_cnt < ((height >> 1) << 1); h_cnt += 2)
    {
        LD_UB2(src + src_stride, src_stride, src10, src11);
        src_minus10 = (v16u8) __msa_ilvr_b((v16i8) src10, (v16i8) src_minus10);
        src_zero0 = __msa_ilvr_b((v16i8) src_minus11, (v16i8) src_minus11);
        src_minus11 = (v16u8) __msa_ilvr_b((v16i8) src11, (v16i8) src_minus11);
        src_zero1 = __msa_ilvr_b((v16i8) src10, (v16i8) src10);

        EDGE_OFFSET_CAL2(src_minus10, src_zero0, src_minus11, src_zero1,
                         diff_minus10, diff_minus11);
        offset_mask0 = (v8i16) __msa_hadd_u_h(diff_minus10, diff_minus10);
        offset_mask1 = (v8i16) __msa_hadd_u_h(diff_minus11, diff_minus11);
        offset_mask0 = ADDVI_H(offset_mask0, 2);
        offset_mask1 = ADDVI_H(offset_mask1, 2);
        offset_mask0 = __msa_vshf_h(offset_mask0, edge_idx, edge_idx);
        offset_mask1 = __msa_vshf_h(offset_mask1, edge_idx, edge_idx);
        ILVEV_B2_SH(src_zero0, zero, src_zero1, zero, src00, src01);
        ADD2(offset_mask0, src00, offset_mask1, src01, offset_mask0,
             offset_mask1);
        CLIP_SH2_0_255(offset_mask0, offset_mask1);
        dst0 = __msa_pckev_b((v16i8) offset_mask1, (v16i8) offset_mask0);

        src_minus10 = src10;
        src_minus11 = src11;
        dst_val0 = __msa_copy_u_w((v4i32) dst0, 0);
        dst_val1 = __msa_copy_u_w((v4i32) dst0, 2);
        SW(dst_val0, dst);
        dst += dst_stride;
        SW(dst_val1, dst);
        dst += dst_stride;
        src += (src_stride << 1);
    }

    if(h_cnt < height)
    {
        src10 = LD_UB(src + src_stride);
        src_minus10 = (v16u8) __msa_ilvr_b((v16i8) src10, (v16i8) src_minus10);
        src_zero0 = __msa_ilvr_b((v16i8) src_minus11, (v16i8) src_minus11);

        cmp_minus10 = ((v16u8) src_zero0 == src_minus10);
        diff_minus10 = __msa_nor_v(cmp_minus10, cmp_minus10);
        cmp_minus10 = (src_minus10 < (v16u8) src_zero0);
        diff_minus10 = __msa_bmnz_v(diff_minus10, const1, cmp_minus10);
        offset_mask0 = (v8i16) __msa_hadd_u_h(diff_minus10, diff_minus10);
        offset_mask0 = ADDVI_H(offset_mask0, 2);
        offset_mask0 = __msa_vshf_h(offset_mask0, edge_idx, edge_idx);
        src00 = (v8i16) __msa_ilvev_b((v16i8) zero, (v16i8) src_zero0);
        offset_mask0 += src00;
        offset_mask0 = CLIP_SH_0_255(offset_mask0);
        dst0 = __msa_pckev_b((v16i8) offset_mask0, (v16i8) offset_mask0);
        dst_val0 = __msa_copy_u_w((v4i32) dst0, 0);
        SW(dst_val0, dst);
    }
}

static void hevc_sao_edge_135deg_8w_in_place_msa(UWORD8 *src, WORD32 src_stride,
                                                 WORD8 *sign_up, UWORD8 *left,
                                                 UWORD8 *mask,
                                                 WORD8 *sao_offset_val,
                                                 WORD32 width, WORD32 height)
{
    UWORD8 src_arr0[height], src_arr1[height];
    UWORD8 *src_temp0 = src_arr0;
    UWORD8 *src_temp1 = src_arr1;
    UWORD8 *src_swap;
    WORD32 h_cnt, w_cnt, val0, val1;
    UWORD8 *src_orig = src;
    v16i8 edge_idx = { sao_offset_val[1], sao_offset_val[2], 0,
                       sao_offset_val[3], sao_offset_val[4],
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    v16u8 const1 = (v16u8) __msa_ldi_b(1);
    v16i8 zero = { 0 };
    v16u8 src_zero0, src_zero1, src_zero2, src_plus0, src_plus1;
    v16u8 mask_reg,  diff_plus0, diff_plus1;
    v16i8 sign_up_vec0, sign_up_vec1, offset_mask0, offset_mask1;
    v8i16 src0, src1, temp0, temp1;

    for(w_cnt = 0; w_cnt < width; w_cnt+=8)
    {
        src = src_orig + w_cnt;
        src_zero0 = LD_UB(src);
        sign_up_vec0 = LD_SB(sign_up + w_cnt);
        mask_reg = LD_UB(mask + w_cnt);

        for(h_cnt = 0; h_cnt < (height >> 1); h_cnt++)
        {
            LD_UB2(src + src_stride, src_stride, src_zero1, src_zero2);
            SLDI_B2_UB(src_zero1, src_zero2, src_zero1, src_zero2,
                    src_plus0, src_plus1, 1);
            EDGE_OFFSET_CAL2(src_plus0, src_zero0, src_plus1, src_zero1,
                            diff_plus0, diff_plus1);
            sign_up_vec1 = __msa_sldi_b((v16i8) diff_plus0, (v16i8) diff_plus0,
                                        15);
            sign_up_vec1 = - (v16i8) sign_up_vec1;

            if(0 == w_cnt)
            {
                val0 = src[0] - left[2 * h_cnt - 1];
                sign_up_vec0[0] = (WORD8) (val0 > 0) ? 1 : (val0 < 0 ? -1 : 0);
                val1 = src[src_stride] - left[2 * h_cnt];
                sign_up_vec1[0] = (WORD8) (val1 > 0) ? 1 : (val1 < 0 ? -1 : 0);
                src_temp1[2 * h_cnt] = src_zero0[7];
                src_temp1[2 * h_cnt + 1] = src_zero1[7];
            }
            else if(0 == h_cnt)
            {
                val1 = (src_temp0[2 * h_cnt] - src[src_stride]);
                sign_up_vec1[0] = - ((WORD8) (val1 > 0) ? 1 : (val1 < 0 ? -1 : 0));
                src_temp1[2 * h_cnt] = src_zero0[7];
                src_temp1[2 * h_cnt + 1] = src_zero1[7];
            }
            else
            {
                val0 = (src_temp0[2 * h_cnt - 1] - src[0]);
                sign_up_vec0[0] = - ((WORD8) (val0 > 0) ? 1 : (val0 < 0 ? -1 : 0));
                val1 = (src_temp0[2 * h_cnt] - src[src_stride]);
                sign_up_vec1[0] = - ((WORD8) (val1 > 0) ? 1 : (val1 < 0 ? -1 : 0));
                src_temp1[2 * h_cnt] = src_zero0[7];
                src_temp1[2 * h_cnt + 1] = src_zero1[7];
            }

            offset_mask0 = sign_up_vec0 + (v16i8) diff_plus0;
            offset_mask1 = sign_up_vec1 + (v16i8) diff_plus1;
            offset_mask0 = ADDVI_B(offset_mask0, 2);
            offset_mask1 = ADDVI_B(offset_mask1, 2);
            offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
            offset_mask1 = __msa_vshf_b(offset_mask1, edge_idx, edge_idx);
            offset_mask0 &= mask_reg;
            offset_mask1 &= mask_reg;

            ILVR_B2_SH(zero, src_zero0, zero, src_zero1, src0, src1);
            UNPCK_R_SB_SH(offset_mask0, temp0);
            UNPCK_R_SB_SH(offset_mask1, temp1);
            ADD2(temp0, src0, temp1, src1, temp0, temp1);
            CLIP_SH2_0_255(temp0, temp1);
            src_zero0 = (v16u8) __msa_pckev_b((v16i8) temp1, (v16i8) temp0);

            ST8x2_UB(src_zero0, src, src_stride);
            src += (src_stride << 1);
            sign_up_vec0 = __msa_sldi_b((v16i8) diff_plus1, (v16i8) diff_plus1,
                                        15);
            sign_up_vec0 = - (v16i8) sign_up_vec0;
            src_zero0 = src_zero2;
        }

        if(height % 2)
        {
            v16u8 cmp_temp0;
            int64_t dst0;

            src_zero1 = LD_UB(src + src_stride);
            src_plus0 = (v16u8) __msa_sldi_b((v16i8) src_zero1,
                                             (v16i8) src_zero1, 1);

            cmp_temp0 = (src_zero0 == src_plus0);
            diff_plus0 = __msa_nor_v(cmp_temp0, cmp_temp0);
            cmp_temp0 = (src_plus0 < src_zero0);
            diff_plus0 = __msa_bmnz_v(diff_plus0, const1, cmp_temp0);

            if(0 == w_cnt)
            {
                val0 = src[0] - left[2 * h_cnt - 1];
                sign_up_vec0[0] = (UWORD8) (val0 > 0) ? 1 :
                                            (val0 < 0 ? -1 : 0);
            }
            else
            {
                val0 = (src_temp0[2 * h_cnt - 1] - src[0]);
                sign_up_vec0[0] = - ((WORD8) (val0 > 0) ? 1 :
                                            (val0 < 0 ? -1 : 0));
            }

            offset_mask0 = sign_up_vec0 + (v16i8) diff_plus0;
            offset_mask0 = ADDVI_B(offset_mask0, 2);
            offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
            offset_mask0 &= mask_reg;

            src0 = (v8i16) __msa_ilvr_b((v16i8) zero, (v16i8) src_zero0);
            UNPCK_R_SB_SH(offset_mask0, temp0);
            temp0 += src0;
            temp0 = CLIP_SH_0_255(temp0);
            src_zero0 = (v16u8) __msa_pckev_b((v16i8) temp0, (v16i8) temp0);

            dst0 = __msa_copy_u_d((v2i64) src_zero0, 0);
            SD(dst0, src);
        }

        src_swap = src_temp0;
        src_temp0 = src_temp1;
        src_temp1 = src_swap;
    }
}

static void hevc_sao_edge_135deg_4w_in_place_msa(UWORD8 *src, WORD32 src_stride,
                                                 WORD8 *sign_up, UWORD8 *left,
                                                 UWORD8 *mask,
                                                 WORD8 *sao_offset_val,
                                                 WORD32 height)
{
    WORD32 h_cnt, val0, val1;
    v16i8 edge_idx = { sao_offset_val[1], sao_offset_val[2], 0,
                       sao_offset_val[3], sao_offset_val[4],
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    v16u8 const1 = (v16u8) __msa_ldi_b(1);
    v16i8 zero = { 0 };
    v16u8 src_zero0, src_zero1, src_zero2, src_plus0, src_plus1;
    v16u8 mask_reg,  diff_plus0, diff_plus1;
    v16i8 sign_up_vec0, sign_up_vec1, offset_mask0, offset_mask1;
    v8i16 src0, src1, temp0, temp1;

    src_zero0 = LD_UB(src);
    sign_up_vec0 = LD_SB(sign_up);
    mask_reg = LD_UB(mask);

    for(h_cnt = 0; h_cnt < (height >> 1); h_cnt++)
    {
        LD_UB2(src + src_stride, src_stride, src_zero1, src_zero2);
        SLDI_B2_UB(src_zero1, src_zero2, src_zero1, src_zero2,
                   src_plus0, src_plus1, 1);
        EDGE_OFFSET_CAL2(src_plus0, src_zero0, src_plus1, src_zero1,
                         diff_plus0, diff_plus1);
        sign_up_vec1 = __msa_sldi_b((v16i8) diff_plus0, (v16i8) diff_plus0, 15);
        sign_up_vec1 = - (v16i8) sign_up_vec1;

        val0 = src[0] - left[2 * h_cnt - 1];
        sign_up_vec0[0] = (WORD8) (val0 > 0) ? 1 : (val0 < 0 ? -1 : 0);
        val1 = src[src_stride] - left[2 * h_cnt];
        sign_up_vec1[0] = (WORD8) (val1 > 0) ? 1 : (val1 < 0 ? -1 : 0);

        offset_mask0 = sign_up_vec0 + (v16i8) diff_plus0;
        offset_mask1 = sign_up_vec1 + (v16i8) diff_plus1;
        offset_mask0 = ADDVI_B(offset_mask0, 2);
        offset_mask1 = ADDVI_B(offset_mask1, 2);
        offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
        offset_mask1 = __msa_vshf_b(offset_mask1, edge_idx, edge_idx);
        offset_mask0 &= mask_reg;
        offset_mask1 &= mask_reg;

        ILVR_B2_SH(zero, src_zero0, zero, src_zero1, src0, src1);
        UNPCK_R_SB_SH(offset_mask0, temp0);
        UNPCK_R_SB_SH(offset_mask1, temp1);
        ADD2(temp0, src0, temp1, src1, temp0, temp1);
        CLIP_SH2_0_255(temp0, temp1);
        src_zero0 = (v16u8) __msa_pckev_b((v16i8) temp1, (v16i8) temp0);

        val0 = __msa_copy_u_w((v4i32) src_zero0, 0);
        val1 = __msa_copy_u_w((v4i32) src_zero0, 2);
        SW(val0, src);
        SW(val1, src + src_stride);
        src += (src_stride << 1);
        sign_up_vec0 = __msa_sldi_b((v16i8) diff_plus1, (v16i8) diff_plus1, 15);
        sign_up_vec0 = - (v16i8) sign_up_vec0;
        src_zero0 = src_zero2;
    }

    if(height % 2)
    {
        v16u8 cmp_temp0;
        WORD32 dst0;

        src_plus0 = LD_UB(src + src_stride + 1);

        cmp_temp0 = (src_zero0 == src_plus0);
        diff_plus0 = __msa_nor_v(cmp_temp0, cmp_temp0);
        cmp_temp0 = (src_plus0 < src_zero0);
        diff_plus0 = __msa_bmnz_v(diff_plus0, const1, cmp_temp0);

        val0 = src[0] - left[2 * h_cnt - 1];
        sign_up_vec0[0] = (UWORD8) (val0 > 0) ? 1 : (val0 < 0 ? -1 : 0);

        offset_mask0 = sign_up_vec0 + (v16i8) diff_plus0;
        offset_mask0 = ADDVI_B(offset_mask0, 2);
        offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
        offset_mask0 &= mask_reg;

        src0 = (v8i16) __msa_ilvr_b((v16i8) zero, (v16i8) src_zero0);
        UNPCK_R_SB_SH(offset_mask0, temp0);
        temp0 += src0;
        temp0 = CLIP_SH_0_255(temp0);
        src_zero0 = (v16u8) __msa_pckev_b((v16i8) temp0, (v16i8) temp0);
        dst0 = __msa_copy_u_w((v4i32) src_zero0, 0);
        SW(dst0, src);
    }
}

static void hevc_sao_edge_45deg_8w_in_place_msa(UWORD8 *src, WORD32 src_stride,
                                                WORD8 *sign_up, UWORD8 *left,
                                                UWORD8 *mask,
                                                WORD8 *sao_offset_val,
                                                WORD32 width, WORD32 height)
{
    UWORD8 src_arr0[height + 1];
    UWORD8 src_arr1[height + 1];
    UWORD8 *src_temp0 = src_arr0;
    UWORD8 *src_temp1 = src_arr1;
    UWORD8 *src_swap;
    WORD32 h_cnt, w_cnt = 0, val0, val1;
    UWORD8 *src_orig = src;
    v16i8 edge_idx = { sao_offset_val[1], sao_offset_val[2], 0,
                       sao_offset_val[3], sao_offset_val[4],
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    v16u8 const1 = (v16u8) __msa_ldi_b(1);
    v16i8 zero = { 0 };
    v16u8 src_minus1, src_minus2, src_zero0, src_zero1, src_zero2;
    v16u8 mask_reg,  diff_plus0, diff_plus1;
    v16i8 sign_up_vec0, sign_up_vec1, offset_mask0, offset_mask1;
    v8i16 src0, src1, temp0, temp1;

    for(w_cnt = 0; w_cnt < width; w_cnt+=8)
    {
        src = src_orig + w_cnt;
        src_zero0 = LD_UB(src);
        sign_up_vec0 = LD_SB(sign_up + w_cnt);
        mask_reg = LD_UB(mask + w_cnt);

        for(h_cnt = 0; h_cnt < (height >> 1); h_cnt++)
        {
            LD_UB2((src + src_stride - 1), src_stride, src_minus1, src_minus2);
            SLDI_B2_UB(src_minus1, src_minus2, src_minus1, src_minus2,
                       src_zero1, src_zero2, 1);

            if(0 == w_cnt)
            {
                src_minus1[0] = left[2 * h_cnt + 1];
                src_minus2[0] = left[2 * h_cnt + 2];
                src_temp1[2 * h_cnt] = src[7 + src_stride];
                src_temp1[2 * h_cnt + 1] = src[7 + 2 * src_stride];
            }
            else
            {
                src_minus1[0] = src_temp0[2 * h_cnt];
                src_minus2[0] = src_temp0[2 * h_cnt + 1];
                src_temp1[2 * h_cnt] = src[7 + src_stride];
                src_temp1[2 * h_cnt + 1] = src[7 + 2 * src_stride];
            }

            EDGE_OFFSET_CAL2(src_minus1, src_zero0, src_minus2, src_zero1,
                             diff_plus0, diff_plus1);
            sign_up_vec1 = __msa_sldi_b((v16i8) diff_plus0, (v16i8) diff_plus0, 1);
            sign_up_vec1 = - (v16i8) sign_up_vec1;

            if((width - 8) == w_cnt)
            {
                val0 = src[7] - src[8 - src_stride];
                sign_up_vec0[7] = (WORD8) (val0 > 0) ? 1 : (val0 < 0 ? -1 : 0);
                val1 = src[src_stride + 7] - src[8];
                sign_up_vec1[7] = (WORD8) (val1 > 0) ? 1 : (val1 < 0 ? -1 : 0);
            }

            offset_mask0 = sign_up_vec0 + (v16i8) diff_plus0;
            offset_mask1 = sign_up_vec1 + (v16i8) diff_plus1;
            offset_mask0 = ADDVI_B(offset_mask0, 2);
            offset_mask1 = ADDVI_B(offset_mask1, 2);
            offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
            offset_mask1 = __msa_vshf_b(offset_mask1, edge_idx, edge_idx);
            offset_mask0 &= mask_reg;
            offset_mask1 &= mask_reg;

            ILVR_B2_SH(zero, src_zero0, zero, src_zero1, src0, src1);
            UNPCK_R_SB_SH(offset_mask0, temp0);
            UNPCK_R_SB_SH(offset_mask1, temp1);
            ADD2(temp0, src0, temp1, src1, temp0, temp1);
            CLIP_SH2_0_255(temp0, temp1);
            src_zero0 = (v16u8) __msa_pckev_b((v16i8) temp1, (v16i8) temp0);

            ST8x2_UB(src_zero0, src, src_stride);
            src += (src_stride << 1);
            sign_up_vec0 = __msa_sldi_b((v16i8) diff_plus1, (v16i8) diff_plus1, 1);
            sign_up_vec0 = - (v16i8) sign_up_vec0;
            src_zero0 = src_zero2;
        }

        if(height % 2)
        {
            v16u8 cmp_temp0;
            int64_t dst0;

            src_minus1 = LD_UB(src + src_stride - 1);
            if(0 == w_cnt)
            {
                src_minus1[0] = left[2 * h_cnt + 1];
            }

            cmp_temp0 = (src_zero0 == src_minus1);
            diff_plus0 = __msa_nor_v(cmp_temp0, cmp_temp0);
            cmp_temp0 = (src_minus1 < src_zero0);
            diff_plus0 = __msa_bmnz_v(diff_plus0, const1, cmp_temp0);

            if((width - 8) == w_cnt)
            {
                val0 = src[7] - src[8 - src_stride];
                sign_up_vec0[7] = (WORD8) (val0 > 0) ? 1 : (val0 < 0 ? -1 : 0);
            }

            offset_mask0 = sign_up_vec0 + (v16i8) diff_plus0;
            offset_mask0 = ADDVI_B(offset_mask0, 2);
            offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
            offset_mask0 &= mask_reg;

            src0 = (v8i16) __msa_ilvr_b((v16i8) zero, (v16i8) src_zero0);
            UNPCK_R_SB_SH(offset_mask0, temp0);
            temp0 += src0;
            temp0 = CLIP_SH_0_255(temp0);
            src_zero0 = (v16u8) __msa_pckev_b((v16i8) temp0, (v16i8) temp0);
            dst0 = __msa_copy_u_d((v2i64) src_zero0, 0);

            SD(dst0, src);
        }

        src_swap = src_temp0;
        src_temp0 = src_temp1;
        src_temp1 = src_swap;
    }
}

static void hevc_sao_edge_45deg_4w_in_place_msa(UWORD8 *src, WORD32 src_stride,
                                                WORD8 *sign_up, UWORD8 *left,
                                                UWORD8 *mask,
                                                WORD8 *sao_offset_val,
                                                WORD32 height)
{
    WORD32 h_cnt, val0, val1;
    UWORD8 *src_orig = src;
    v16i8 edge_idx = { sao_offset_val[1], sao_offset_val[2], 0,
                       sao_offset_val[3], sao_offset_val[4],
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    v16u8 const1 = (v16u8) __msa_ldi_b(1);
    v16i8 zero = { 0 };
    v16u8 src_minus1, src_minus2, src_zero0, src_zero1, src_zero2;
    v16u8 mask_reg,  diff_plus0, diff_plus1;
    v16i8 sign_up_vec0, sign_up_vec1, offset_mask0, offset_mask1;
    v8i16 src0, src1, temp0, temp1;

    src = src_orig;
    src_zero0 = LD_UB(src);
    sign_up_vec0 = LD_SB(sign_up);
    mask_reg = LD_UB(mask);

    for(h_cnt = 0; h_cnt < (height >> 1); h_cnt++)
    {
        LD_UB2((src + src_stride - 1), src_stride, src_minus1, src_minus2);
        SLDI_B2_UB(src_minus1, src_minus2, src_minus1, src_minus2,
                   src_zero1, src_zero2, 1);

        src_minus1[0] = left[2 * h_cnt + 1];
        src_minus2[0] = left[2 * h_cnt + 2];

        EDGE_OFFSET_CAL2(src_minus1, src_zero0, src_minus2, src_zero1,
                         diff_plus0, diff_plus1);
        sign_up_vec1 = __msa_sldi_b((v16i8) diff_plus0, (v16i8) diff_plus0, 1);
        sign_up_vec1 = - (v16i8) sign_up_vec1;

        val0 = src[3] - src[4 - src_stride];
        sign_up_vec0[3] = (WORD8) (val0 > 0) ? 1 : (val0 < 0 ? -1 : 0);
        val1 = src[src_stride + 3] - src[4];
        sign_up_vec1[3] = (WORD8) (val1 > 0) ? 1 : (val1 < 0 ? -1 : 0);

        offset_mask0 = sign_up_vec0 + (v16i8) diff_plus0;
        offset_mask1 = sign_up_vec1 + (v16i8) diff_plus1;
        offset_mask0 = ADDVI_B(offset_mask0, 2);
        offset_mask1 = ADDVI_B(offset_mask1, 2);
        offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
        offset_mask1 = __msa_vshf_b(offset_mask1, edge_idx, edge_idx);
        offset_mask0 &= mask_reg;
        offset_mask1 &= mask_reg;

        ILVR_B2_SH(zero, src_zero0, zero, src_zero1, src0, src1);
        UNPCK_R_SB_SH(offset_mask0, temp0);
        UNPCK_R_SB_SH(offset_mask1, temp1);
        ADD2(temp0, src0, temp1, src1, temp0, temp1);
        CLIP_SH2_0_255(temp0, temp1);
        src_zero0 = (v16u8) __msa_pckev_b((v16i8) temp1, (v16i8) temp0);

        val0 = __msa_copy_u_w((v4i32) src_zero0, 0);
        val1 = __msa_copy_u_w((v4i32) src_zero0, 2);
        SW(val0, src);
        SW(val1, src + src_stride);
        src += (src_stride << 1);
        sign_up_vec0 = __msa_sldi_b((v16i8) diff_plus1, (v16i8) diff_plus1, 1);
        sign_up_vec0 = - (v16i8) sign_up_vec0;
        src_zero0 = src_zero2;
    }

    if(height % 2)
    {
        v16u8 cmp_temp0;
        int64_t dst0;

        src_minus1 = LD_UB(src + src_stride - 1);
        src_minus1[0] = left[2 * h_cnt + 1];

        cmp_temp0 = (src_zero0 == src_minus1);
        diff_plus0 = __msa_nor_v(cmp_temp0, cmp_temp0);
        cmp_temp0 = (src_minus1 < src_zero0);
        diff_plus0 = __msa_bmnz_v(diff_plus0, const1, cmp_temp0);

        val0 = src[3] - src[4 - src_stride];
        sign_up_vec0[3] = (WORD8) (val0 > 0) ? 1 : (val0 < 0 ? -1 : 0);

        offset_mask0 = sign_up_vec0 + (v16i8) diff_plus0;
        offset_mask0 = ADDVI_B(offset_mask0, 2);
        offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
        offset_mask0 &= mask_reg;

        src0 = (v8i16) __msa_ilvr_b((v16i8) zero, (v16i8) src_zero0);
        UNPCK_R_SB_SH(offset_mask0, temp0);
        temp0 += src0;
        temp0 = CLIP_SH_0_255(temp0);
        src_zero0 = (v16u8) __msa_pckev_b((v16i8) temp0, (v16i8) temp0);
        dst0 = __msa_copy_u_w((v4i32) src_zero0, 0);

        SW(dst0, src);
    }
}

static void hevc_sao_edge_chroma_0deg_16w_in_place_msa(UWORD8 *src,
                                                       WORD32 src_stride,
                                                       UWORD8 *left,
                                                       UWORD8 *mask,
                                                       WORD8 *sao_offset_u,
                                                       WORD8 *sao_offset_v,
                                                       WORD32 width,
                                                       WORD32 height)
{
    UWORD8 *dst = src;
    WORD32 dst_stride = src_stride;
    UWORD8 *dst_ptr, *src_minus1;
    WORD32 h_cnt, v_cnt;
    v16i8 edge_idx = { sao_offset_u[1], sao_offset_u[2], 0,
                       sao_offset_u[3], sao_offset_u[4],
                       sao_offset_v[1], sao_offset_v[2], 0,
                       sao_offset_v[3], sao_offset_v[4] };
    v16i8 add_coeff = { 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7 };
    v16u8 diff_minus10, diff_plus10, diff_minus11, diff_plus11;
    v16u8 diff_minus12, diff_plus12, diff_minus13, diff_plus13;
    v16u8 src10, src11, src12, src13, dst0, dst1, dst2, dst3;
    v16u8 src_minus10, src_minus11, src_minus12, src_minus13;
    v16i8 offset_mask0, offset_mask1, offset_mask2, offset_mask3;
    v16i8 src_zero0, src_zero1, src_zero2, src_zero3;
    v16i8 src_plus10, src_plus11, src_plus12, src_plus13;
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7;
    v8i16 temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;

    /* Find multiple of 16 width count */
    /* 12 width case is also handelled in multiple of 16 loop */
    WORD32 iSimd16PxlCount = ((width + 4) >> 4) << 4;
    if(iSimd16PxlCount > width)
    {
        /* Set 4 masks after width to zero */
        /*This is to handle multiple of 12 case*/
        *((WORD32 *) (mask + width)) = 0;
    }

    for(h_cnt = 0 ; h_cnt < (height >> 2) << 2; h_cnt += 4)
    {
        src_minus1 = src - 2;
        LD_UB4(src_minus1, src_stride,
               src_minus10, src_minus11, src_minus12, src_minus13);
        src_minus10 =
            (v16u8) __msa_insert_h((v8i16) src_minus10, 0,
                                   *((WORD16 *) (left + 2 * h_cnt)));
        src_minus11 =
            (v16u8) __msa_insert_h((v8i16) src_minus11, 0,
                                   *((WORD16 *) (left + 2 * h_cnt + 2)));
        src_minus12 =
            (v16u8) __msa_insert_h((v8i16) src_minus12, 0,
                                   *((WORD16 *) (left + 2 * h_cnt + 4)));
        src_minus13 =
            (v16u8) __msa_insert_h((v8i16) src_minus13, 0,
                                   *((WORD16 *) (left + 2 * h_cnt + 6)));

        for(v_cnt = 0; v_cnt < iSimd16PxlCount; v_cnt += 16)
        {
            src_minus1 += 16;
            dst_ptr = dst + v_cnt;
            LD_UB4(src_minus1, src_stride, src10, src11, src12, src13);
            SLDI_B2_SB(src10, src11, src_minus10, src_minus11, src_zero0,
                       src_zero1, 2);
            SLDI_B2_SB(src12, src13, src_minus12, src_minus13, src_zero2,
                       src_zero3, 2);
            SLDI_B2_SB(src10, src11, src_minus10, src_minus11, src_plus10,
                       src_plus11, 4);
            SLDI_B2_SB(src12, src13, src_minus12, src_minus13, src_plus12,
                       src_plus13, 4);
            EDGE_OFFSET_CAL4(src_minus10, src_zero0, src_minus11, src_zero1,
                             src_minus12, src_zero2, src_minus13, src_zero3,
                             diff_minus10, diff_minus11, diff_minus12,
                             diff_minus13);
            EDGE_OFFSET_CAL4(src_plus10, src_zero0, src_plus11, src_zero1,
                             src_plus12, src_zero2, src_plus13, src_zero3,
                             diff_plus10, diff_plus11, diff_plus12,
                             diff_plus13);

            offset_mask0 = add_coeff + (v16i8) diff_minus10 +
                           (v16i8) diff_plus10;
            offset_mask1 = add_coeff + (v16i8) diff_minus11 +
                           (v16i8) diff_plus11;
            offset_mask2 = add_coeff + (v16i8) diff_minus12 +
                           (v16i8) diff_plus12;
            offset_mask3 = add_coeff + (v16i8) diff_minus13 +
                           (v16i8) diff_plus13;
            VSHF_B4_SB(edge_idx, edge_idx, offset_mask0, offset_mask1,
                       offset_mask2, offset_mask3, offset_mask0, offset_mask1,
                       offset_mask2, offset_mask3);
            offset_mask0 = offset_mask0 & LD_SB(mask + v_cnt);
            offset_mask1 = offset_mask1 & LD_SB(mask + v_cnt);
            offset_mask2 = offset_mask2 & LD_SB(mask + v_cnt);
            offset_mask3 = offset_mask3 & LD_SB(mask + v_cnt);

            UNPCK_UB_SH(src_zero0, src0, src1);
            UNPCK_SB_SH(offset_mask0, temp0, temp1);
            UNPCK_UB_SH(src_zero1, src2, src3);
            UNPCK_SB_SH(offset_mask1, temp2, temp3);
            UNPCK_UB_SH(src_zero2, src4, src5);
            UNPCK_SB_SH(offset_mask2, temp4, temp5);
            UNPCK_UB_SH(src_zero3, src6, src7);
            UNPCK_SB_SH(offset_mask3, temp6, temp7);
            ADD4(temp0, src0, temp1, src1, temp2, src2, temp3, src3, temp0,
                 temp1, temp2, temp3);
            ADD4(temp4, src4, temp5, src5, temp6, src6, temp7, src7, temp4,
                 temp5, temp6, temp7);
            CLIP_SH4_0_255(temp0, temp1, temp2, temp3);
            CLIP_SH4_0_255(temp4, temp5, temp6, temp7);
            PCKEV_B4_UB(temp1, temp0, temp3, temp2, temp5, temp4, temp7, temp6,
                        dst0, dst1, dst2, dst3);

            src_minus10 = src10;
            ST_UB(dst0, dst_ptr);
            src_minus11 = src11;
            ST_UB(dst1, dst_ptr + dst_stride);
            src_minus12 = src12;
            ST_UB(dst2, dst_ptr + (dst_stride << 1));
            src_minus13 = src13;
            ST_UB(dst3, dst_ptr + (dst_stride * 3));
        }

        /* Handle if remaining width = 8 */
        if(8 == (width - iSimd16PxlCount))
        {
            /* We have already loaded data in src_minus10, src_minus11,
               src_minus12, src_minus13 registers */
            dst_ptr = dst + v_cnt;
            v16u8 mask_reg = LD_UB(mask + v_cnt);

            SLDI_B2_SB(src_minus10, src_minus11, src_minus10, src_minus11,
                       src_zero0, src_zero1, 2);
            SLDI_B2_SB(src_minus12, src_minus13, src_minus12, src_minus13,
                       src_zero2, src_zero3, 2);
            SLDI_B2_SB(src_minus10, src_minus11, src_minus10, src_minus11,
                       src_plus10, src_plus11, 4);
            SLDI_B2_SB(src_minus12, src_minus13, src_minus12, src_minus13,
                       src_plus12, src_plus13, 4);
            ILVR_D2_UB(src_minus11, src_minus10, src_minus13, src_minus12,
                       src_minus10, src_minus12);
            ILVR_D2_SB(src_zero1, src_zero0, src_zero3, src_zero2,
                       src_zero0, src_zero2);
            ILVR_D2_SB(src_plus11, src_plus10, src_plus13, src_plus12,
                       src_plus10, src_plus12);
            EDGE_OFFSET_CAL4(src_minus10, src_zero0, src_plus10, src_zero0,
                             src_minus12, src_zero2, src_plus12, src_zero2,
                             diff_minus10, diff_plus10, diff_minus12,
                             diff_plus12);

            offset_mask0 = add_coeff + (v16i8) diff_minus10 +
                           (v16i8) diff_plus10;
            offset_mask2 = add_coeff + (v16i8) diff_minus12 +
                           (v16i8) diff_plus12;
            offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
            offset_mask2 = __msa_vshf_b(offset_mask2, edge_idx, edge_idx);
            offset_mask0 &= mask_reg;
            offset_mask2 &= mask_reg;

            UNPCK_UB_SH(src_zero0, src0, src1);
            UNPCK_SB_SH(offset_mask0, temp0, temp1);
            UNPCK_UB_SH(src_zero2, src4, src5);
            UNPCK_SB_SH(offset_mask2, temp4, temp5);
            ADD4(temp0, src0, temp1, src1, temp4, src4, temp5, src5, temp0,
                 temp1, temp4, temp5);
            CLIP_SH4_0_255(temp0, temp1, temp4, temp5);
            PCKEV_B2_UB(temp1, temp0, temp5, temp4, dst0, dst2);
            ST8x4_UB(dst0, dst2, dst_ptr, dst_stride);
        }
        else if(4 == (width - iSimd16PxlCount))
        {
            /* We have already loaded data in src_minus10, src_minus11,
               src_minus12, src_minus13 registers */
            dst_ptr = dst + v_cnt;
            v16u8 mask_reg = LD_UB(mask + v_cnt);
            SLDI_B2_SB(src_minus10, src_minus11, src_minus10, src_minus11,
                       src_zero0, src_zero1, 2);
            SLDI_B2_SB(src_minus12, src_minus13, src_minus12, src_minus13,
                       src_zero2, src_zero3, 2);
            SLDI_B2_SB(src_minus10, src_minus11, src_minus10, src_minus11,
                       src_plus10, src_plus11, 4);
            SLDI_B2_SB(src_minus12, src_minus13, src_minus12, src_minus13,
                       src_plus12, src_plus13, 4);

            ILVR_W2_UB(src_minus11, src_minus10, src_minus13, src_minus12,
                       src_minus10, src_minus12);
            ILVR_W2_SB(src_zero1, src_zero0, src_zero3, src_zero2,
                       src_zero0, src_zero2);
            ILVR_W2_SB(src_plus11, src_plus10, src_plus13, src_plus12,
                       src_plus10, src_plus12);
            ILVR_D2_SB(src_plus12, src_plus10, src_zero2, src_zero0,
                       src_plus10, src_zero0);
            ILVR_D2_UB(src_minus12, src_minus10, mask_reg, mask_reg,
                       src_minus10, mask_reg);
            EDGE_OFFSET_CAL2(src_minus10, src_zero0, src_plus10, src_zero0,
                             diff_minus10, diff_plus10);

            offset_mask0 = add_coeff + (v16i8) diff_minus10 +
                           (v16i8) diff_plus10;
            offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
            offset_mask0 &= mask_reg;
            UNPCK_UB_SH(src_zero0, src0, src1);
            UNPCK_SB_SH(offset_mask0, temp0, temp1);
            ADD2(temp0, src0, temp1, src1, temp0, temp1);
            CLIP_SH2_0_255(temp0, temp1);
            dst0 = (v16u8) __msa_pckev_b((v16i8) temp1, (v16i8) temp0);
            ST4x4_UB(dst0, dst0, 0, 1, 2, 3, dst_ptr, dst_stride);
        }

        src += (src_stride << 2);
        dst += (dst_stride << 2);
    }
}

static void hevc_sao_edge_chroma_0deg_8w_in_place_msa(UWORD8 *src,
                                                      WORD32 src_stride,
                                                      UWORD8 *left,
                                                      UWORD8 *mask,
                                                      WORD8 *sao_offset_u,
                                                      WORD8 *sao_offset_v,
                                                      WORD32 height)
{
    UWORD8 *dst = src;
    WORD32 dst_stride = src_stride;
    UWORD8 *src_minus1;
    WORD32 h_cnt, h_mod4;
    v16i8 edge_idx = { sao_offset_u[1], sao_offset_u[2], 0,
                       sao_offset_u[3], sao_offset_u[4],
                       sao_offset_v[1], sao_offset_v[2], 0,
                       sao_offset_v[3], sao_offset_v[4]};
    v16i8 add_coeff = { 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7 };
    v16i8 zero = { 0 };
    v16u8 diff_minus10, diff_plus10, diff_minus12, diff_plus12, dst0, dst2;
    v16u8 src_minus10, src_minus11, src_minus12, src_minus13;
    v16i8 offset_mask0, offset_mask2, mask_reg;
    v16i8 src_zero0, src_zero1, src_zero2, src_zero3;
    v16i8 src_plus10, src_plus11, src_plus12, src_plus13;
    v8i16 src0, src1, src2, src3, temp0, temp1, temp2, temp3;

    h_mod4 = height - (height % 4);
    mask_reg = LD_SB(mask);
    mask_reg = __msa_ilvr_b(mask_reg, mask_reg);

    for(h_cnt = 0; h_cnt < h_mod4; h_cnt += 4)
    {
        src_minus1 = src - 2;
        LD_UB4(src_minus1, src_stride, src_minus10, src_minus11,
               src_minus12, src_minus13);
        src_minus10 =
            (v16u8) __msa_insert_h((v8i16) src_minus10, 0,
                                   *((UWORD16 *) (left + 2 * h_cnt)));
        src_minus11 =
            (v16u8) __msa_insert_h((v8i16) src_minus11, 0,
                                   *((UWORD16 *) (left + 2 * h_cnt + 2)));
        src_minus12 =
            (v16u8) __msa_insert_h((v8i16) src_minus12, 0,
                                   *((UWORD16 *) (left + 2 * h_cnt + 4)));
        src_minus13 =
            (v16u8) __msa_insert_h((v8i16) src_minus13, 0,
                                   *((UWORD16 *) (left + 2 * h_cnt + 6)));

        SLDI_B2_SB(src_minus10, src_minus11, src_minus10, src_minus11,
                   src_zero0, src_zero1, 2);
        SLDI_B2_SB(src_minus12, src_minus13, src_minus12, src_minus13,
                   src_zero2, src_zero3, 2);
        SLDI_B2_SB(src_minus10, src_minus11, src_minus10, src_minus11,
                   src_plus10, src_plus11, 4);
        SLDI_B2_SB(src_minus12, src_minus13, src_minus12, src_minus13,
                   src_plus12, src_plus13, 4);
        ILVR_D2_UB(src_minus11, src_minus10, src_minus13, src_minus12,
                   src_minus10, src_minus12);
        ILVR_D2_SB(src_zero1, src_zero0, src_zero3, src_zero2,
                   src_zero0, src_zero2);
        ILVR_D2_SB(src_plus11, src_plus10, src_plus13, src_plus12,
                   src_plus10, src_plus12);
        EDGE_OFFSET_CAL4(src_minus10, src_zero0, src_plus10, src_zero0,
                         src_minus12, src_zero2, src_plus12, src_zero2,
                         diff_minus10, diff_plus10, diff_minus12,
                         diff_plus12);
        offset_mask0 = add_coeff + (v16i8) diff_minus10 + (v16i8) diff_plus10;
        offset_mask2 = add_coeff + (v16i8) diff_minus12 + (v16i8) diff_plus12;
        offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
        offset_mask2 = __msa_vshf_b(offset_mask2, edge_idx, edge_idx);
        offset_mask0 &= mask_reg;
        offset_mask2 &= mask_reg;

        UNPCK_UB_SH(src_zero0, src0, src1);
        UNPCK_SB_SH(offset_mask0, temp0, temp1);
        UNPCK_UB_SH(src_zero2, src2, src3);
        UNPCK_SB_SH(offset_mask2, temp2, temp3);
        ADD4(temp0, src0, temp1, src1, temp2, src2, temp3, src3,
             temp0, temp1, temp2, temp3);
        CLIP_SH4_0_255(temp0, temp1, temp2, temp3);
        PCKEV_B2_UB(temp1, temp0, temp3, temp2, dst0, dst2);
        ST8x4_UB(dst0, dst2, dst, dst_stride);
        src += (src_stride << 2);
        dst += (dst_stride << 2);
    }

    for(h_cnt = h_mod4; h_cnt < height; h_cnt++)
    {
        int64_t dst_val;

        src_minus1 = src - 2;
        src_minus10 = LD_UB(src_minus1);
        src_minus10 =
            (v16u8) __msa_insert_h((v8i16) src_minus10, 0,
                                   *((UWORD16 *) (left + 2 * h_cnt)));
        src_zero0 = __msa_sldi_b((v16i8) src_minus10, (v16i8) src_minus10, 2);
        src_plus10 = __msa_sldi_b((v16i8) src_minus10, (v16i8) src_minus10, 4);

        EDGE_OFFSET_CAL2(src_minus10, src_zero0, src_plus10, src_zero0,
                         diff_minus10, diff_plus10);
        offset_mask0 = add_coeff + (v16i8) diff_minus10 + (v16i8) diff_plus10;
        offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
        offset_mask0 &= mask_reg;

        src0 = (v8i16) __msa_ilvr_b(zero, src_zero0);
        UNPCK_R_SB_SH(offset_mask0, temp0);
        temp0 += src0;
        temp0 = CLIP_SH_0_255(temp0);
        dst0 = (v16u8) __msa_pckev_b((v16i8) temp0, (v16i8) temp0);
        dst_val = __msa_copy_u_d((v2i64) dst0, 0);
        SD(dst_val, dst);
        src += src_stride;
        dst += dst_stride;
    }
}

static void hevc_sao_edge_chroma_0deg_4w_in_place_msa(UWORD8 *src,
                                                      WORD32 src_stride,
                                                      UWORD8 *left,
                                                      UWORD8 *mask,
                                                      WORD8 *sao_offset_u,
                                                      WORD8 *sao_offset_v,
                                                      WORD32 height)
{
    UWORD8 *dst = src;
    WORD32 dst_stride = src_stride;
    UWORD8 *src_minus1;
    WORD32 h_cnt, h_mod4;
    v16i8 edge_idx = { sao_offset_u[1], sao_offset_u[2], 0,
                       sao_offset_u[3], sao_offset_u[4],
                       sao_offset_v[1], sao_offset_v[2], 0,
                       sao_offset_v[3], sao_offset_v[4]};
    v16i8 add_coeff = { 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7 };
    v16i8 zero = { 0 };
    v16u8 diff_minus10, diff_plus10, diff_minus12, diff_plus12, dst0, dst2;
    v16u8 src_minus10, src_minus11, src_minus12, src_minus13;
    v16i8 offset_mask0, offset_mask2, mask_reg;
    v16i8 src_zero0, src_zero1, src_zero2, src_zero3;
    v16i8 src_plus10, src_plus11, src_plus12, src_plus13;
    v8i16 src0, src1, src2, src3, temp0, temp1, temp2, temp3;

    h_mod4 = height - (height % 4);
    mask_reg = LD_SB(mask);
    mask_reg = __msa_ilvr_b(mask_reg, mask_reg);

    for(h_cnt = 0; h_cnt < h_mod4; h_cnt += 4)
    {
        src_minus1 = src - 2;
        LD_UB4(src_minus1, src_stride, src_minus10, src_minus11,
               src_minus12, src_minus13);
        src_minus10 =
            (v16u8) __msa_insert_h((v8i16) src_minus10, 0,
                                   *((UWORD16 *) (left + 2 * h_cnt)));
        src_minus11 =
            (v16u8) __msa_insert_h((v8i16) src_minus11, 0,
                                   *((UWORD16 *) (left + 2 * h_cnt + 2)));
        src_minus12 =
            (v16u8) __msa_insert_h((v8i16) src_minus12, 0,
                                   *((UWORD16 *) (left + 2 * h_cnt + 4)));
        src_minus13 =
            (v16u8) __msa_insert_h((v8i16) src_minus13, 0,
                                   *((UWORD16 *) (left + 2 * h_cnt + 6)));

        SLDI_B2_SB(src_minus10, src_minus11, src_minus10, src_minus11,
                   src_zero0, src_zero1, 2);
        SLDI_B2_SB(src_minus12, src_minus13, src_minus12, src_minus13,
                   src_zero2, src_zero3, 2);
        SLDI_B2_SB(src_minus10, src_minus11, src_minus10, src_minus11,
                   src_plus10, src_plus11, 4);
        SLDI_B2_SB(src_minus12, src_minus13, src_minus12, src_minus13,
                   src_plus12, src_plus13, 4);
        ILVR_D2_UB(src_minus11, src_minus10, src_minus13, src_minus12,
                   src_minus10, src_minus12);
        ILVR_D2_SB(src_zero1, src_zero0, src_zero3, src_zero2,
                   src_zero0, src_zero2);
        ILVR_D2_SB(src_plus11, src_plus10, src_plus13, src_plus12,
                   src_plus10, src_plus12);
        EDGE_OFFSET_CAL4(src_minus10, src_zero0, src_plus10, src_zero0,
                         src_minus12, src_zero2, src_plus12, src_zero2,
                         diff_minus10, diff_plus10, diff_minus12,
                         diff_plus12);
        offset_mask0 = add_coeff + (v16i8) diff_minus10 + (v16i8) diff_plus10;
        offset_mask2 = add_coeff + (v16i8) diff_minus12 + (v16i8) diff_plus12;
        offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
        offset_mask2 = __msa_vshf_b(offset_mask2, edge_idx, edge_idx);
        offset_mask0 &= mask_reg;
        offset_mask2 = offset_mask2 & mask_reg;

        UNPCK_UB_SH(src_zero0, src0, src1);
        UNPCK_SB_SH(offset_mask0, temp0, temp1);
        UNPCK_UB_SH(src_zero2, src2, src3);
        UNPCK_SB_SH(offset_mask2, temp2, temp3);
        ADD4(temp0, src0, temp1, src1, temp2, src2, temp3, src3,
             temp0, temp1, temp2, temp3);
        CLIP_SH4_0_255(temp0, temp1, temp2, temp3);
        PCKEV_B2_UB(temp1, temp0, temp3, temp2, dst0, dst2);
        ST4x4_UB(dst0, dst2, 0, 2, 0, 2, dst, dst_stride);
        src += (src_stride << 2);
        dst += (dst_stride << 2);
    }

    for(h_cnt = h_mod4; h_cnt < height; h_cnt++)
    {
        WORD32 dst_val;

        src_minus1 = src - 2;
        src_minus10 = LD_UB(src_minus1);
        src_minus10 = (v16u8) __msa_insert_h((v8i16) src_minus10, 0,
                                           *((UWORD16 *) (left + 2 * h_cnt)));
        src_zero0 = __msa_sldi_b((v16i8) src_minus10, (v16i8) src_minus10, 2);
        src_plus10 = __msa_sldi_b((v16i8) src_minus10, (v16i8) src_minus10, 4);

        EDGE_OFFSET_CAL2(src_minus10, src_zero0, src_plus10, src_zero0,
                         diff_minus10, diff_plus10);
        offset_mask0 = add_coeff + (v16i8) diff_minus10 + (v16i8) diff_plus10;
        offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
        offset_mask0 &= mask_reg;

        src0 = (v8i16) __msa_ilvr_b(zero, src_zero0);
        UNPCK_R_SB_SH(offset_mask0, temp0);
        temp0 += src0;
        temp0 = CLIP_SH_0_255(temp0);
        dst0 = (v16u8) __msa_pckev_b((v16i8) temp0, (v16i8) temp0);
        dst_val = __msa_copy_u_w((v4i32) dst0, 0);
        SW(dst_val, dst);
        src += src_stride;
        dst += dst_stride;
    }
}

static void hevc_sao_edge_chroma_90deg_16w_in_place_msa(UWORD8 *src,
                                                        WORD32 src_stride,
                                                        WORD8 *sign_up,
                                                        UWORD8 *mask,
                                                        WORD8 *sao_offset_u,
                                                        WORD8 *sao_offset_v,
                                                        WORD32 width,
                                                        WORD32 height)
{
    UWORD8 *dst = src;
    WORD32 dst_stride = src_stride;
    UWORD8 *src_orig = src;
    UWORD8 *dst_orig = dst;
    WORD32 h_cnt, v_cnt;
    v16i8 edge_idx = { sao_offset_u[1], sao_offset_u[2], 0,
                       sao_offset_u[3], sao_offset_u[4],
                       sao_offset_v[1], sao_offset_v[2], 0,
                       sao_offset_v[3], sao_offset_v[4] };
    v16i8 add_coeff = { 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7 };
    v16u8 const1 = (v16u8) __msa_ldi_b(1);
    v16u8 diff_plus10, diff_plus12, diff_plus13, diff_plus11, dst1, cmp_plus10;
    v16u8 src10, dst0, src11, src_minus11, src12, dst2, src13, dst3;
    v16i8 offset_mask0, offset_mask1, offset_mask2, offset_mask3, sign_up_vec;
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7;
    v8i16 temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;
    v16i8 mask_reg;

    for(v_cnt = 0; v_cnt < ((width >> 4) << 4); v_cnt += 16)
    {
        src = src_orig + v_cnt;
        dst = dst_orig + v_cnt;
        /* Load from top pointer and first row */
        sign_up_vec = LD_SB(sign_up + v_cnt);
        src_minus11 = LD_UB(src);

        mask_reg = LD_SB(mask + v_cnt);
        mask_reg = __msa_ilvr_b(mask_reg, mask_reg);
        /* Calculate signUp in advance */

        /* Process 4 rows at a time. */
        for(h_cnt = 0 ; h_cnt < (height >> 2) << 2; h_cnt += 4)
        {
            LD_UB4(src + src_stride, src_stride, src10, src11, src12, src13);
            EDGE_OFFSET_CAL4(src10, src_minus11, src11, src10,
                             src12, src11, src13, src12,
                             diff_plus10, diff_plus11, diff_plus12, diff_plus13);

            offset_mask0 = add_coeff + (v16i8) sign_up_vec + (v16i8) diff_plus10;
            offset_mask1 = add_coeff - (v16i8) diff_plus10 + (v16i8) diff_plus11;
            offset_mask2 = add_coeff - (v16i8) diff_plus11 + (v16i8) diff_plus12;
            offset_mask3 = add_coeff - (v16i8) diff_plus12 + (v16i8) diff_plus13;
            VSHF_B4_SB(edge_idx, edge_idx, offset_mask0, offset_mask1,
                       offset_mask2, offset_mask3, offset_mask0, offset_mask1,
                       offset_mask2, offset_mask3);
            UNPCK_UB_SH(src_minus11, src0, src1);
            UNPCK_SB_SH(offset_mask0, temp0, temp1);
            UNPCK_UB_SH(src10, src2, src3);
            UNPCK_SB_SH(offset_mask1, temp2, temp3);
            UNPCK_UB_SH(src11, src4, src5);
            UNPCK_SB_SH(offset_mask2, temp4, temp5);
            UNPCK_UB_SH(src12, src6, src7);
            UNPCK_SB_SH(offset_mask3, temp6, temp7);
            ADD4(temp0, src0, temp1, src1, temp2, src2, temp3, src3, temp0,
                 temp1, temp2, temp3);
            ADD4(temp4, src4, temp5, src5, temp6, src6, temp7, src7, temp4,
                 temp5, temp6, temp7);
            CLIP_SH4_0_255(temp0, temp1, temp2, temp3);
            CLIP_SH4_0_255(temp4, temp5, temp6, temp7);
            PCKEV_B4_UB(temp1, temp0, temp3, temp2, temp5, temp4, temp7, temp6,
                        dst0, dst1, dst2, dst3);

            sign_up_vec = - (v16i8) diff_plus13;
            src_minus11 = src13;
            ST_UB4(dst0, dst1, dst2, dst3, dst, dst_stride);
            src += (src_stride << 2);
            dst += (dst_stride << 2);
        }

        /* Process remaining rows, 1 row at a time */
        for( ; h_cnt < height; h_cnt++)
        {
            src10 = LD_UB(src + src_stride);

            cmp_plus10 = (src_minus11 == src10);
            diff_plus10 = __msa_nor_v(cmp_plus10, cmp_plus10);
            cmp_plus10 = (src10 < src_minus11);
            diff_plus10 = __msa_bmnz_v(diff_plus10, const1, cmp_plus10);

            offset_mask0 = add_coeff + sign_up_vec + (v16i8) diff_plus10;
            offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
            UNPCK_UB_SH(src_minus11, src0, src1);
            UNPCK_SB_SH(offset_mask0, temp0, temp1);

            ADD2(temp0, src0, temp1, src1, temp0, temp1);
            CLIP_SH2_0_255(temp0, temp1);
            dst0 = (v16u8) __msa_pckev_b((v16i8) temp1, (v16i8) temp0);
            src_minus11 = src10;

            /* Reuse sign for next iteration*/
            sign_up_vec = - (v16i8) diff_plus10;
            ST_UB(dst0, dst);
            src += src_stride;
            dst += dst_stride;
        }
    }
}

static void hevc_sao_edge_chroma_90deg_8w_in_place_msa(UWORD8 *src,
                                                       WORD32 src_stride,
                                                       WORD8 *sign_up,
                                                       UWORD8 *mask,
                                                       WORD8 *sao_offset_u,
                                                       WORD8 *sao_offset_v,
                                                       WORD32 height)
{
    UWORD8 *dst = src;
    WORD32 dst_stride = src_stride;
    WORD32 h_cnt;
    uint64_t dst_val0;
    v16i8 edge_idx = { sao_offset_u[1], sao_offset_u[2], 0,
                       sao_offset_u[3], sao_offset_u[4],
                       sao_offset_v[1], sao_offset_v[2], 0,
                       sao_offset_v[3], sao_offset_v[4] };
    v16i8 add_coeff = { 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7 };
    v16u8 const1 = (v16u8) __msa_ldi_b(1);
    v16i8 zero = { 0 };
    v16u8 cmp_minus10, diff_minus10, diff_minus11, dst0;
    v16u8 src_minus11, src10, src11;
    v16i8 offset_mask0, offset_mask1, sign_up_vec, mask_reg;
    v8i16 temp0, temp1, src00, src01;

    mask_reg = LD_SB(mask);
    mask_reg = __msa_ilvr_b(mask_reg, mask_reg);
    sign_up_vec = LD_SB(sign_up);
    src_minus11 = LD_UB(src);
    /* Process 2 rows at a time */
    for(h_cnt = 0; h_cnt < ((height >> 1) << 1); h_cnt += 2)
    {
        LD_UB2(src + src_stride, src_stride, src10, src11);

        EDGE_OFFSET_CAL2(src10, src_minus11, src11, src10, diff_minus10,
                         diff_minus11);

        offset_mask0 = add_coeff + (v16i8) diff_minus10 + sign_up_vec;
        offset_mask1 = add_coeff - (v16i8) diff_minus10 + (v16i8) diff_minus11;
        offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
        offset_mask1 = __msa_vshf_b(offset_mask1, edge_idx, edge_idx);
        offset_mask0 &= mask_reg;
        offset_mask1 &= mask_reg;
        ILVR_B2_SH(zero, src_minus11, zero, src10, src00, src01);
        UNPCK_R_SB_SH(offset_mask0, temp0);
        UNPCK_R_SB_SH(offset_mask1, temp1);
        ADD2(temp0, src00, temp1, src01, temp0, temp1);
        CLIP_SH2_0_255(temp0, temp1);

        dst0 = (v16u8) __msa_pckev_b((v16i8) temp1, (v16i8) temp0);
        sign_up_vec = - (v16i8) diff_minus11;
        src_minus11 = src11;
        ST8x2_UB(dst0, dst, dst_stride);
        dst += (dst_stride << 1);
        src += (src_stride << 1);
    }

    /* Process remaining row if there */
    if(h_cnt < height)
    {
        src10 = LD_UB(src + src_stride);

        cmp_minus10 = ((v16u8) src_minus11 == src10);
        diff_minus10 = __msa_nor_v(cmp_minus10, cmp_minus10);
        cmp_minus10 = (src10 < (v16u8) src_minus11);
        diff_minus10 = __msa_bmnz_v(diff_minus10, const1, cmp_minus10);

        offset_mask0 = add_coeff + (v16i8) diff_minus10 + (v16i8) sign_up_vec;
        offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
        offset_mask0 &= mask_reg;
        UNPCK_R_SB_SH(offset_mask0, temp0);
        src00 = (v8i16) __msa_ilvr_b(zero, (v16i8) src_minus11);

        temp0 += src00;
        temp0 = CLIP_SH_0_255(temp0);
        dst0 = (v16u8) __msa_pckev_b((v16i8) temp0, (v16i8) temp0);
        dst_val0 = __msa_copy_u_d((v2i64) dst0, 0);
        SD(dst_val0, dst);
    }
}

static void hevc_sao_edge_chroma_90deg_4w_in_place_msa(UWORD8 *src,
                                                       WORD32 src_stride,
                                                       WORD8 *sign_up,
                                                       UWORD8 *mask,
                                                       WORD8 *sao_offset_u,
                                                       WORD8 *sao_offset_v,
                                                       WORD32 height)
{
    UWORD8 *dst = src;
    WORD32 dst_stride = src_stride;
    WORD32 h_cnt, val0, val1;
    v16i8 edge_idx = { sao_offset_u[1], sao_offset_u[2], 0,
                       sao_offset_u[3], sao_offset_u[4],
                       sao_offset_v[1], sao_offset_v[2], 0,
                       sao_offset_v[3], sao_offset_v[4] };
    v16i8 add_coeff = { 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7 };
    v16u8 const1 = (v16u8) __msa_ldi_b(1);
    v16i8 zero = { 0 };
    v16u8 cmp_minus10, diff_minus10, diff_minus11, dst0;
    v16u8 src_minus11, src10, src11;
    v16i8 offset_mask0, offset_mask1, sign_up_vec, mask_reg;
    v8i16 temp0, temp1, src00, src01;

    mask_reg = LD_SB(mask);
    mask_reg = __msa_ilvr_b(mask_reg, mask_reg);
    sign_up_vec = LD_SB(sign_up);
    src_minus11 = LD_UB(src);
    /* Process 2 rows at a time */
    for(h_cnt = 0; h_cnt < ((height >> 1) << 1); h_cnt += 2)
    {
        LD_UB2(src + src_stride, src_stride, src10, src11);

        EDGE_OFFSET_CAL2(src10, src_minus11, src11, src10,
                         diff_minus10, diff_minus11);

        offset_mask0 = add_coeff + (v16i8) diff_minus10 + sign_up_vec;
        offset_mask1 = add_coeff - (v16i8) diff_minus10 + (v16i8) diff_minus11;
        offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
        offset_mask1 = __msa_vshf_b(offset_mask1, edge_idx, edge_idx);
        offset_mask0 &= mask_reg;
        offset_mask1 &= mask_reg;
        ILVR_B2_SH(zero, src_minus11, zero, src10, src00, src01);
        UNPCK_R_SB_SH(offset_mask0, temp0);
        UNPCK_R_SB_SH(offset_mask1, temp1);
        ADD2(temp0, src00, temp1, src01, temp0, temp1);
        CLIP_SH2_0_255(temp0, temp1);

        dst0 = (v16u8) __msa_pckev_b((v16i8) temp1, (v16i8) temp0);
        sign_up_vec = - (v16i8) diff_minus11;
        src_minus11 = src11;
        val0 = __msa_copy_u_w((v4i32) dst0, 0);
        val1 = __msa_copy_u_w((v4i32) dst0, 2);
        SW(val0, dst);
        SW(val1, dst + dst_stride);
        dst += (dst_stride << 1);
        src += (src_stride << 1);
    }

    /* Process remaining row if there */
    if(h_cnt < height)
    {
        src10 = LD_UB(src + src_stride);

        cmp_minus10 = ((v16u8) src_minus11 == src10);
        diff_minus10 = __msa_nor_v(cmp_minus10, cmp_minus10);
        cmp_minus10 = (src10 < (v16u8) src_minus11);
        diff_minus10 = __msa_bmnz_v(diff_minus10, const1, cmp_minus10);

        offset_mask0 = add_coeff + (v16i8) diff_minus10 + (v16i8) sign_up_vec;
        offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
        offset_mask0 &= mask_reg;
        UNPCK_R_SB_SH(offset_mask0, temp0);
        src00 = (v8i16) __msa_ilvr_b(zero, (v16i8) src_minus11);

        temp0 += src00;
        temp0 = CLIP_SH_0_255(temp0);
        dst0 = (v16u8) __msa_pckev_b((v16i8) temp0, (v16i8) temp0);
        val0 = __msa_copy_u_w((v4i32) dst0, 0);
        SW(val0, dst);
    }
}

static void hevc_sao_edge_chroma_135deg_8w_in_place_msa(UWORD8 *src,
                                                        WORD32 src_stride,
                                                        WORD8 *sign_up,
                                                        UWORD8 *left,
                                                        UWORD8 *mask,
                                                        WORD8 *sao_offset_u,
                                                        WORD8 *sao_offset_v,
                                                        WORD32 width,
                                                        WORD32 height)
{
    UWORD8 src_arr0[2 * height], src_arr1[2 * height];
    UWORD8 *src_temp0 = src_arr0;
    UWORD8 *src_temp1 = src_arr1;
    UWORD8 *src_swap;
    WORD32 h_cnt, w_cnt, val0, val1, val2, val3;
    UWORD8 *src_orig = src;
    v16i8 edge_idx = { sao_offset_u[1], sao_offset_u[2], 0,
                       sao_offset_u[3], sao_offset_u[4],
                       sao_offset_v[1], sao_offset_v[2], 0,
                       sao_offset_v[3], sao_offset_v[4] };
    v16i8 add_coeff = { 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7 };
    v16u8 const1 = (v16u8) __msa_ldi_b(1);
    v16i8 zero = { 0 };
    v16u8 src_zero0, src_zero1, src_zero2, src_plus0, src_plus1;
    v16u8 diff_plus0, diff_plus1;
    v16i8 sign_up_vec0, sign_up_vec1, offset_mask0, offset_mask1, mask_reg;
    v8i16 src0, src1, temp0, temp1;

    for(w_cnt = 0; w_cnt < width; w_cnt+=8)
    {
        src = src_orig + w_cnt;
        src_zero0 = LD_UB(src);
        sign_up_vec0 = LD_SB(sign_up + w_cnt);
        mask_reg = LD_SB(mask + (w_cnt >> 1));
        mask_reg = __msa_ilvr_b(mask_reg, mask_reg);

        for(h_cnt = 0; h_cnt < (height >> 1); h_cnt++)
        {
            LD_UB2(src + src_stride, src_stride, src_zero1, src_zero2);
            SLDI_B2_UB(src_zero1, src_zero2, src_zero1, src_zero2,
                       src_plus0, src_plus1, 2);
            EDGE_OFFSET_CAL2(src_plus0, src_zero0, src_plus1, src_zero1,
                             diff_plus0, diff_plus1);

            sign_up_vec1 = __msa_sldi_b((v16i8) diff_plus0,
                                        (v16i8) diff_plus0, 14);
            sign_up_vec1 = - (v16i8) sign_up_vec1;

            if(0 == w_cnt)
            {
                val0 = src[0] - left[-2 + 4 * h_cnt];
                val1 = src[1] - left[-1 + 4 * h_cnt];
                val2 = src[src_stride] - left[4 * h_cnt];
                val3 = src[src_stride + 1] - left[4 * h_cnt + 1];
                sign_up_vec0[0] = (WORD8) (val0 > 0) ? 1 : (val0 < 0 ? -1 : 0);
                sign_up_vec0[1] = (WORD8) (val1 > 0) ? 1 : (val1 < 0 ? -1 : 0);
                sign_up_vec1[0] = (WORD8) (val2 > 0) ? 1 : (val2 < 0 ? -1 : 0);
                sign_up_vec1[1] = (WORD8) (val3 > 0) ? 1 : (val3 < 0 ? -1 : 0);
                src_temp1[4 * h_cnt] = src_zero0[6];
                src_temp1[4 * h_cnt + 1] = src_zero0[7];
                src_temp1[4 * h_cnt + 2] = src_zero1[6];
                src_temp1[4 * h_cnt + 3] = src_zero1[7];
            }
            else if(0 == h_cnt)
            {
                val0 = (src_temp0[4 * h_cnt] - src[src_stride]);
                val1 = (src_temp0[4 * h_cnt + 1] - src[src_stride + 1]);
                sign_up_vec1[0] = - ((WORD8) (val0 > 0) ? 1 : (val0 < 0 ? -1 : 0));
                sign_up_vec1[1] = - ((WORD8) (val1 > 0) ? 1 : (val1 < 0 ? -1 : 0));
                src_temp1[4 * h_cnt] = src_zero0[6];
                src_temp1[4 * h_cnt + 1] = src_zero0[7];
                src_temp1[4 * h_cnt + 2] = src_zero1[6];
                src_temp1[4 * h_cnt + 3] = src_zero1[7];
            }
            else
            {
                val0 = (src_temp0[4 * h_cnt - 2] - src[0]);
                val1 = (src_temp0[4 * h_cnt - 1] - src[1]);
                val2 = (src_temp0[4 * h_cnt] - src[src_stride]);
                val3 = (src_temp0[4 * h_cnt + 1] - src[src_stride + 1]);
                sign_up_vec0[0] = - ((WORD8) (val0 > 0) ? 1 : (val0 < 0 ? -1 : 0));
                sign_up_vec0[1] = - ((WORD8) (val1 > 0) ? 1 : (val1 < 0 ? -1 : 0));
                sign_up_vec1[0] = - ((WORD8) (val2 > 0) ? 1 : (val2 < 0 ? -1 : 0));
                sign_up_vec1[1] = - ((WORD8) (val3 > 0) ? 1 : (val3 < 0 ? -1 : 0));
                src_temp1[4 * h_cnt] = src_zero0[6];
                src_temp1[4 * h_cnt + 1] = src_zero0[7];
                src_temp1[4 * h_cnt + 2] = src_zero1[6];
                src_temp1[4 * h_cnt + 3] = src_zero1[7];
            }

            offset_mask0 = add_coeff + sign_up_vec0 + (v16i8) diff_plus0;
            offset_mask1 = add_coeff + sign_up_vec1 + (v16i8) diff_plus1;
            offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
            offset_mask1 = __msa_vshf_b(offset_mask1, edge_idx, edge_idx);
            offset_mask0 &= mask_reg;
            offset_mask1 &= mask_reg;

            ILVR_B2_SH(zero, src_zero0, zero, src_zero1, src0, src1);
            UNPCK_R_SB_SH(offset_mask0, temp0);
            UNPCK_R_SB_SH(offset_mask1, temp1);

            ADD2(temp0, src0, temp1, src1, temp0, temp1);
            CLIP_SH2_0_255(temp0, temp1);
            src_zero0 = (v16u8) __msa_pckev_b((v16i8) temp1, (v16i8) temp0);

            ST8x2_UB(src_zero0, src, src_stride);
            src += (src_stride << 1);
            sign_up_vec0 = __msa_sldi_b((v16i8) diff_plus1, (v16i8) diff_plus1, 14);
            sign_up_vec0 = - (v16i8) sign_up_vec0;
            src_zero0 = src_zero2;
        }

        if(height % 2)
        {
            v16u8 cmp_temp0;
            int64_t dst0;

            src_plus0 = LD_UB(src + src_stride + 2);

            cmp_temp0 = (src_zero0 == src_plus0);
            diff_plus0 = __msa_nor_v(cmp_temp0, cmp_temp0);
            cmp_temp0 = (src_plus0 < src_zero0);
            diff_plus0 = __msa_bmnz_v(diff_plus0, const1, cmp_temp0);

            if(0 == w_cnt)
            {
                val0 = src[0] - left[-2 + 4 * h_cnt];
                val1 = src[1] - left[-1 + 4 * h_cnt];
                sign_up_vec0[0] = (WORD8) (val0 > 0) ? 1 : (val0 < 0 ? -1 : 0);
                sign_up_vec0[1] = (WORD8) (val1 > 0) ? 1 : (val1 < 0 ? -1 : 0);
                src_temp1[4 * h_cnt] = src_zero0[6];
                src_temp1[4 * h_cnt + 1] = src_zero0[7];
            }
            else
            {
                val0 = (src_temp0[4 * h_cnt - 2] - src[0]);
                val1 = (src_temp0[4 * h_cnt - 1] - src[1]);
                sign_up_vec0[0] = - ((WORD8) (val0 > 0) ? 1 : (val0 < 0 ? -1 : 0));
                sign_up_vec0[1] = - ((WORD8) (val1 > 0) ? 1 : (val1 < 0 ? -1 : 0));
                src_temp1[4 * h_cnt] = src_zero0[6];
                src_temp1[4 * h_cnt + 1] = src_zero0[7];
            }

            offset_mask0 = add_coeff + sign_up_vec0 + (v16i8) diff_plus0;
            offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
            offset_mask0 &= mask_reg;

            src0 = (v8i16) __msa_ilvr_b((v16i8) zero, (v16i8) src_zero0);
            UNPCK_R_SB_SH(offset_mask0, temp0);
            temp0 += src0;
            temp0 = CLIP_SH_0_255(temp0);
            src_zero0 = (v16u8) __msa_pckev_b((v16i8) temp0, (v16i8) temp0);
            dst0 = __msa_copy_u_d((v2i64) src_zero0, 0);

            SD(dst0, src);
        }

        src_swap = src_temp0;
        src_temp0 = src_temp1;
        src_temp1 = src_swap;
    }
}

static void hevc_sao_edge_chroma_135deg_4w_in_place_msa(UWORD8 *src,
                                                        WORD32 src_stride,
                                                        WORD8 *sign_up,
                                                        UWORD8 *left,
                                                        UWORD8 *mask,
                                                        WORD8 *sao_offset_u,
                                                        WORD8 *sao_offset_v,
                                                        WORD32 height)
{
    WORD32 h_cnt, val0, val1, val2, val3;
    v16i8 edge_idx = { sao_offset_u[1], sao_offset_u[2], 0,
                       sao_offset_u[3], sao_offset_u[4],
                       sao_offset_v[1], sao_offset_v[2], 0,
                       sao_offset_v[3], sao_offset_v[4] };
    v16i8 add_coeff = { 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7 };
    v16u8 const1 = (v16u8) __msa_ldi_b(1);
    v16i8 zero = { 0 };
    v16u8 src_zero0, src_zero1, src_zero2, src_plus0, src_plus1;
    v16u8 diff_plus0, diff_plus1;
    v16i8 sign_up_vec0, sign_up_vec1, offset_mask0, offset_mask1, mask_reg;
    v8i16 src0, src1, temp0, temp1;

    src_zero0 = LD_UB(src);
    sign_up_vec0 = LD_SB(sign_up);
    mask_reg = LD_SB(mask);
    mask_reg = __msa_ilvr_b(mask_reg, mask_reg);

    for(h_cnt = 0; h_cnt < (height >> 1); h_cnt++)
    {
        LD_UB2(src + src_stride, src_stride, src_zero1, src_zero2);
        SLDI_B2_UB(src_zero1, src_zero2, src_zero1, src_zero2,
                   src_plus0, src_plus1, 2);
        EDGE_OFFSET_CAL2(src_plus0, src_zero0, src_plus1, src_zero1,
                         diff_plus0, diff_plus1);

        sign_up_vec1 = __msa_sldi_b((v16i8) diff_plus0,
                                    (v16i8) diff_plus0, 14);
        sign_up_vec1 = - (v16i8) sign_up_vec1;

        val0 = src[0] - left[-2 + 4 * h_cnt];
        val1 = src[1] - left[-1 + 4 * h_cnt];
        val2 = src[src_stride] - left[4 * h_cnt];
        val3 = src[src_stride + 1] - left[4 * h_cnt + 1];
        sign_up_vec0[0] = (WORD8) (val0 > 0) ? 1 : (val0 < 0 ? -1 : 0);
        sign_up_vec0[1] = (WORD8) (val1 > 0) ? 1 : (val1 < 0 ? -1 : 0);
        sign_up_vec1[0] = (WORD8) (val2 > 0) ? 1 : (val2 < 0 ? -1 : 0);
        sign_up_vec1[1] = (WORD8) (val3 > 0) ? 1 : (val3 < 0 ? -1 : 0);

        offset_mask0 = add_coeff + sign_up_vec0 + (v16i8) diff_plus0;
        offset_mask1 = add_coeff + sign_up_vec1 + (v16i8) diff_plus1;
        offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
        offset_mask1 = __msa_vshf_b(offset_mask1, edge_idx, edge_idx);
        offset_mask0 &= mask_reg;
        offset_mask1 &= mask_reg;

        ILVR_B2_SH(zero, src_zero0, zero, src_zero1, src0, src1);
        UNPCK_R_SB_SH(offset_mask0, temp0);
        UNPCK_R_SB_SH(offset_mask1, temp1);

        ADD2(temp0, src0, temp1, src1, temp0, temp1);
        CLIP_SH2_0_255(temp0, temp1);
        src_zero0 = (v16u8) __msa_pckev_b((v16i8) temp1, (v16i8) temp0);

        val0 = __msa_copy_u_w((v4i32) src_zero0, 0);
        val1 = __msa_copy_u_w((v4i32) src_zero0, 2);
        SW(val0, src);
        SW(val1, src + src_stride);

        src += (src_stride << 1);
        sign_up_vec0 = __msa_sldi_b((v16i8) diff_plus1, (v16i8) diff_plus1, 14);
        sign_up_vec0 = - (v16i8) sign_up_vec0;
        src_zero0 = src_zero2;
    }

    if(height % 2)
    {
        v16u8 cmp_temp0;

        src_plus0 = LD_UB(src + src_stride + 2);

        cmp_temp0 = (src_zero0 == src_plus0);
        diff_plus0 = __msa_nor_v(cmp_temp0, cmp_temp0);
        cmp_temp0 = (src_plus0 < src_zero0);
        diff_plus0 = __msa_bmnz_v(diff_plus0, const1, cmp_temp0);

        val0 = src[0] - left[-2 + 4 * h_cnt];
        val1 = src[1] - left[-1 + 4 * h_cnt];
        sign_up_vec0[0] = (WORD8) (val0 > 0) ? 1 : (val0 < 0 ? -1 : 0);
        sign_up_vec0[1] = (WORD8) (val1 > 0) ? 1 : (val1 < 0 ? -1 : 0);

        offset_mask0 = add_coeff + sign_up_vec0 + (v16i8) diff_plus0;
        offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
        offset_mask0 &= mask_reg;

        src0 = (v8i16) __msa_ilvr_b((v16i8) zero, (v16i8) src_zero0);
        UNPCK_R_SB_SH(offset_mask0, temp0);
        temp0 += src0;
        temp0 = CLIP_SH_0_255(temp0);
        src_zero0 = (v16u8) __msa_pckev_b((v16i8) temp0, (v16i8) temp0);
        val0 = __msa_copy_u_w((v4i32) src_zero0, 0);

        SW(val0, src);
    }
}

static void hevc_sao_edge_chroma_45deg_8w_in_place_msa(UWORD8 *src,
                                                       WORD32 src_stride,
                                                       WORD8 *sign_up,
                                                       UWORD8 *left,
                                                       UWORD8 *mask,
                                                       WORD8 *sao_offset_u,
                                                       WORD8 *sao_offset_v,
                                                       WORD32 width,
                                                       WORD32 height)
{
    UWORD8 src_arr0[2 * height], src_arr1[2 * height];
    UWORD8 *src_temp0 = src_arr0;
    UWORD8 *src_temp1 = src_arr1;
    UWORD8 *src_swap;
    WORD32 h_cnt, w_cnt = 0, val0, val1, val2, val3;
    UWORD8 *src_orig = src;
    v16i8 edge_idx = { sao_offset_u[1], sao_offset_u[2], 0,
                       sao_offset_u[3], sao_offset_u[4],
                       sao_offset_v[1], sao_offset_v[2], 0,
                       sao_offset_v[3], sao_offset_v[4] };
    v16i8 add_coeff = { 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7 };
    v16u8 const1 = (v16u8) __msa_ldi_b(1);
    v16i8 zero = { 0 };
    v16u8 src_minus1, src_minus2, src_zero0, src_zero1, src_zero2;
    v16u8 diff_plus0, diff_plus1;
    v16i8 sign_up_vec0, sign_up_vec1, offset_mask0, offset_mask1, mask_reg;
    v8i16 src0, src1, temp0, temp1;

    for(w_cnt = 0; w_cnt < width; w_cnt+=8)
    {
        src = src_orig + w_cnt;
        src_zero0 = LD_UB(src);
        sign_up_vec0 = LD_SB(sign_up + w_cnt);
        mask_reg = LD_SB(mask + (w_cnt >> 1));
        mask_reg = __msa_ilvr_b(mask_reg, mask_reg);

        for(h_cnt = 0; h_cnt < (height >> 1); h_cnt++)
        {
            LD_UB2((src + src_stride - 2), src_stride, src_minus1, src_minus2);
            SLDI_B2_UB(src_minus1, src_minus2, src_minus1, src_minus2,
                       src_zero1, src_zero2, 2);

            if(0 == w_cnt)
            {
                src_minus1[0] = left[4 * h_cnt + 2];
                src_minus1[1] = left[4 * h_cnt + 3];
                src_minus2[0] = left[4 * h_cnt + 4];
                src_minus2[1] = left[4 * h_cnt + 5];
                src_temp1[4 * h_cnt] = src[6 + src_stride];
                src_temp1[4 * h_cnt + 1] = src[7 + src_stride];
                src_temp1[4 * h_cnt + 2] = src[6 + 2 * src_stride];
                src_temp1[4 * h_cnt + 3] = src[7 + 2 * src_stride];
            }
            else
            {
                src_minus1[0] = src_temp0[4 * h_cnt];
                src_minus1[1] = src_temp0[4 * h_cnt + 1];
                src_minus2[0] = src_temp0[4 * h_cnt + 2];
                src_minus2[1] = src_temp0[4 * h_cnt + 3];
                src_temp1[4 * h_cnt] = src[6 + src_stride];
                src_temp1[4 * h_cnt + 1] = src[7 + src_stride];
                src_temp1[4 * h_cnt + 2] = src[6 + 2 * src_stride];
                src_temp1[4 * h_cnt + 3] = src[7 + 2 * src_stride];
            }

            EDGE_OFFSET_CAL2(src_minus1, src_zero0, src_minus2, src_zero1,
                             diff_plus0, diff_plus1);
            sign_up_vec1 = __msa_sldi_b((v16i8) diff_plus0, (v16i8) diff_plus0, 2);
            sign_up_vec1 = - (v16i8) sign_up_vec1;

            if((width - 8) == w_cnt)
            {
                val0 = src[6] - src[8 - src_stride];
                val1 = src[7] - src[9 - src_stride];
                sign_up_vec0[6] = (WORD8) (val0 > 0) ? 1 : (val0 < 0 ? -1 : 0);
                sign_up_vec0[7] = (WORD8) (val1 > 0) ? 1 : (val1 < 0 ? -1 : 0);
                val2 = src[src_stride + 6] - src[8];
                val3 = src[src_stride + 7] - src[9];
                sign_up_vec1[6] = (WORD8) (val2 > 0) ? 1 : (val2 < 0 ? -1 : 0);
                sign_up_vec1[7] = (WORD8) (val3 > 0) ? 1 : (val3 < 0 ? -1 : 0);
            }

            offset_mask0 = add_coeff + sign_up_vec0 + (v16i8) diff_plus0;
            offset_mask1 = add_coeff + sign_up_vec1 + (v16i8) diff_plus1;
            offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
            offset_mask1 = __msa_vshf_b(offset_mask1, edge_idx, edge_idx);
            offset_mask0 &= mask_reg;
            offset_mask1 &= mask_reg;

            ILVR_B2_SH(zero, src_zero0, zero, src_zero1, src0, src1);
            UNPCK_R_SB_SH(offset_mask0, temp0);
            UNPCK_R_SB_SH(offset_mask1, temp1);
            ADD2(temp0, src0, temp1, src1, temp0, temp1);
            CLIP_SH2_0_255(temp0, temp1);
            src_zero0 = (v16u8) __msa_pckev_b((v16i8) temp1, (v16i8) temp0);

            ST8x2_UB(src_zero0, src, src_stride);
            src += (src_stride << 1);
            sign_up_vec0 = __msa_sldi_b((v16i8) diff_plus1, (v16i8) diff_plus1,
                                        2);
            sign_up_vec0 = - (v16i8) sign_up_vec0;
            src_zero0 = src_zero2;
        }

        if(height % 2)
        {
            v16u8 cmp_temp0;
            int64_t dst0;

            src_minus1 = LD_UB(src + src_stride - 2);
            if(0 == w_cnt)
            {
                src_minus1[0] = left[4 * h_cnt + 2];
                src_minus1[1] = left[4 * h_cnt + 3];
            }

            cmp_temp0 = (src_zero0 == src_minus1);
            diff_plus0 = __msa_nor_v(cmp_temp0, cmp_temp0);
            cmp_temp0 = (src_minus1 < src_zero0);
            diff_plus0 = __msa_bmnz_v(diff_plus0, const1, cmp_temp0);

            if((width - 8) == w_cnt)
            {
                val0 = src[6] - src[8 - src_stride];
                val1 = src[7] - src[9 - src_stride];
                sign_up_vec0[6] = (WORD8) (val0 > 0) ? 1 : (val0 < 0 ? -1 : 0);
                sign_up_vec0[7] = (WORD8) (val1 > 0) ? 1 : (val1 < 0 ? -1 : 0);
            }

            offset_mask0 = add_coeff + sign_up_vec0 + (v16i8) diff_plus0;
            offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
            offset_mask0 &= mask_reg;

            src0 = (v8i16) __msa_ilvr_b((v16i8) zero, (v16i8) src_zero0);
            UNPCK_R_SB_SH(offset_mask0, temp0);
            temp0 += src0;
            temp0 = CLIP_SH_0_255(temp0);
            src_zero0 = (v16u8) __msa_pckev_b((v16i8) temp0, (v16i8) temp0);
            dst0 = __msa_copy_u_d((v2i64) src_zero0, 0);

            SD(dst0, src);
        }

        src_swap = src_temp0;
        src_temp0 = src_temp1;
        src_temp1 = src_swap;
    }
}

static void hevc_sao_edge_chroma_45deg_4w_in_place_msa(UWORD8 *src,
                                                       WORD32 src_stride,
                                                       WORD8 *sign_up,
                                                       UWORD8 *left,
                                                       UWORD8 *mask,
                                                       WORD8 *sao_offset_u,
                                                       WORD8 *sao_offset_v,
                                                       WORD32 height)
{
    WORD32 h_cnt, val0, val1, val2, val3;
    v16i8 edge_idx = { sao_offset_u[1], sao_offset_u[2], 0,
                       sao_offset_u[3], sao_offset_u[4],
                       sao_offset_v[1], sao_offset_v[2], 0,
                       sao_offset_v[3], sao_offset_v[4] };
    v16i8 add_coeff = { 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7 };
    v16i8 sign_up_vec0, sign_up_vec1, offset_mask0, offset_mask1, mask_reg;
    v16i8 zero = { 0 };
    v16u8 src_minus1, src_minus2, src_zero0, src_zero1, src_zero2;
    v16u8 diff_plus0, diff_plus1;
    v16u8 const1 = (v16u8) __msa_ldi_b(1);
    v8i16 src0, src1, temp0, temp1;

    src_zero0 = LD_UB(src);
    sign_up_vec0 = LD_SB(sign_up);
    mask_reg = LD_SB(mask);
    mask_reg = __msa_ilvr_b(mask_reg, mask_reg);

    for(h_cnt = 0; h_cnt < (height >> 1); h_cnt++)
    {
        LD_UB2((src + src_stride - 2), src_stride, src_minus1, src_minus2);
        SLDI_B2_UB(src_minus1, src_minus2, src_minus1, src_minus2,
                   src_zero1, src_zero2, 2);

        src_minus1[0] = left[4 * h_cnt + 2];
        src_minus1[1] = left[4 * h_cnt + 3];
        src_minus2[0] = left[4 * h_cnt + 4];
        src_minus2[1] = left[4 * h_cnt + 5];

        EDGE_OFFSET_CAL2(src_minus1, src_zero0, src_minus2, src_zero1,
                         diff_plus0, diff_plus1);
        sign_up_vec1 = __msa_sldi_b((v16i8) diff_plus0, (v16i8) diff_plus0, 2);
        sign_up_vec1 = - (v16i8) sign_up_vec1;

        val0 = src[2] - src[4 - src_stride];
        val1 = src[3] - src[5 - src_stride];
        sign_up_vec0[2] = (WORD8) (val0 > 0) ? 1 : (val0 < 0 ? -1 : 0);
        sign_up_vec0[3] = (WORD8) (val1 > 0) ? 1 : (val1 < 0 ? -1 : 0);
        val2 = src[src_stride + 2] - src[4];
        val3 = src[src_stride + 3] - src[5];
        sign_up_vec1[2] = (WORD8) (val2 > 0) ? 1 : (val2 < 0 ? -1 : 0);
        sign_up_vec1[3] = (WORD8) (val3 > 0) ? 1 : (val3 < 0 ? -1 : 0);

        offset_mask0 = add_coeff + sign_up_vec0 + (v16i8) diff_plus0;
        offset_mask1 = add_coeff + sign_up_vec1 + (v16i8) diff_plus1;
        offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
        offset_mask1 = __msa_vshf_b(offset_mask1, edge_idx, edge_idx);
        offset_mask0 &= mask_reg;
        offset_mask1 &= mask_reg;

        ILVR_B2_SH(zero, src_zero0, zero, src_zero1, src0, src1);
        UNPCK_R_SB_SH(offset_mask0, temp0);
        UNPCK_R_SB_SH(offset_mask1, temp1);
        ADD2(temp0, src0, temp1, src1, temp0, temp1);
        CLIP_SH2_0_255(temp0, temp1);
        src_zero0 = (v16u8) __msa_pckev_b((v16i8) temp1, (v16i8) temp0);

        val0 = __msa_copy_u_w((v4i32) src_zero0, 0);
        val1 = __msa_copy_u_w((v4i32) src_zero0, 2);
        SW(val0, src);
        SW(val1, src + src_stride);
        src += (src_stride << 1);
        sign_up_vec0 = __msa_sldi_b((v16i8) diff_plus1, (v16i8) diff_plus1, 2);
        sign_up_vec0 = - (v16i8) sign_up_vec0;
        src_zero0 = src_zero2;
    }

    if(height % 2)
    {
        v16u8 cmp_temp0;
        int64_t dst0;

        src_minus1 = LD_UB(src + src_stride - 2);
        src_minus1[0] = left[4 * h_cnt + 2];
        src_minus1[1] = left[4 * h_cnt + 3];

        cmp_temp0 = (src_zero0 == src_minus1);
        diff_plus0 = __msa_nor_v(cmp_temp0, cmp_temp0);
        cmp_temp0 = (src_minus1 < src_zero0);
        diff_plus0 = __msa_bmnz_v(diff_plus0, const1, cmp_temp0);

        val0 = src[2] - src[4 - src_stride];
        val1 = src[3] - src[5 - src_stride];
        sign_up_vec0[2] = (WORD8) (val0 > 0) ? 1 : (val0 < 0 ? -1 : 0);
        sign_up_vec0[3] = (WORD8) (val1 > 0) ? 1 : (val1 < 0 ? -1 : 0);

        offset_mask0 = add_coeff + sign_up_vec0 + (v16i8) diff_plus0;
        offset_mask0 = __msa_vshf_b(offset_mask0, edge_idx, edge_idx);
        offset_mask0 &= mask_reg;

        src0 = (v8i16) __msa_ilvr_b((v16i8) zero, (v16i8) src_zero0);
        UNPCK_R_SB_SH(offset_mask0, temp0);
        temp0 += src0;
        temp0 = CLIP_SH_0_255(temp0);
        src_zero0 = (v16u8) __msa_pckev_b((v16i8) temp0, (v16i8) temp0);
        dst0 = __msa_copy_u_w((v4i32) src_zero0, 0);

        SW(dst0, src);
    }
}

void ihevc_sao_band_offset_luma_msa(UWORD8 *src, WORD32 src_stride,
                                    UWORD8 *src_left, UWORD8 *src_top,
                                    UWORD8 *src_top_left, WORD32 sao_band_pos,
                                    WORD8 *sao_offset, WORD32 wd, WORD32 ht)
{
    WORD32 band_shift;
    WORD32 band_table[NUM_BAND_TABLE];
    WORD32 i;
    WORD32 row, col;

    for(row = 0; row < ht; row++)
    {
        src_left[row] = src[row * src_stride + (wd - 1)];
    }

    src_top_left[0] = src_top[wd - 1];
    for(col = 0; col < wd; col++)
    {
        src_top[col] = src[(ht - 1) * src_stride + col];
    }

    if(4 == wd)
    {
        hevc_sao_band_filter_4width_msa(src, src_stride, sao_band_pos,
                                        sao_offset, ht);
    }
    else if(8 == wd)
    {
        hevc_sao_band_filter_8width_msa(src, src_stride, sao_band_pos,
                                        sao_offset, ht);
    }
    else if(wd % 4)
    {
        hevc_sao_band_filter_16multiple_msa(src, src_stride, sao_band_pos,
                                            sao_offset, wd, ht);
    }
    else
    {
        band_shift = BIT_DEPTH_LUMA - 5;
        for(i = 0; i < NUM_BAND_TABLE; i++)
        {
            band_table[i] = 0;
        }

        for(i = 0; i < 4; i++)
        {
            band_table[(i + sao_band_pos) & 31] = i + 1;
        }

        for(row = 0; row < ht; row++)
        {
            for(col = 0; col < wd; col++)
            {
                WORD32 band_idx;

                band_idx = band_table[src[col] >> band_shift];
                src[col] = CLIP3(src[col] + sao_offset[band_idx],
                                 0, (1 << (band_shift + 5)) - 1);
            }

            src += src_stride;
        }
    }
}

/* input 'wd' has to be for the interleaved block and
   not for each color component */
void ihevc_sao_band_offset_chroma_msa(UWORD8 *src, WORD32 src_stride,
                                      UWORD8 *src_left, UWORD8 *src_top,
                                      UWORD8 *src_top_left,
                                      WORD32 sao_band_pos_u,
                                      WORD32 sao_band_pos_v,
                                      WORD8 *sao_offset_u,
                                      WORD8 *sao_offset_v,
                                      WORD32 wd, WORD32 ht)
{
    WORD32 band_shift;
    WORD32 band_table_u[NUM_BAND_TABLE];
    WORD32 band_table_v[NUM_BAND_TABLE];
    WORD32 i;
    WORD32 row, col;

    for(row = 0; row < ht; row++)
    {
        src_left[2 * row] = src[row * src_stride + (wd - 2)];
        src_left[2 * row + 1] = src[row * src_stride + (wd - 1)];
    }

    src_top_left[0] = src_top[wd - 2];
    src_top_left[1] = src_top[wd - 1];
    for(col = 0; col < wd; col++)
    {
        src_top[col] = src[(ht - 1) * src_stride + col];
    }

    if(4 == wd)
    {
        hevc_sao_band_chroma_4width_msa(src, src_stride, sao_band_pos_u,
                                        sao_band_pos_v, sao_offset_u,
                                        sao_offset_v, ht);
    }
    else if(8 == wd)
    {
        hevc_sao_band_chroma_8width_msa(src, src_stride, sao_band_pos_u,
                                        sao_band_pos_v, sao_offset_u,
                                        sao_offset_v, ht);
    }
    else if(!(wd % 16))
    {
        hevc_sao_band_chroma_16width_msa(src, src_stride, sao_band_pos_u,
                                         sao_band_pos_v, sao_offset_u,
                                         sao_offset_v, wd, ht);
    }
    else
    {
        band_shift = BIT_DEPTH_CHROMA - 5;
        for(i = 0; i < NUM_BAND_TABLE; i++)
        {
            band_table_u[i] = 0;
            band_table_v[i] = 0;
        }

        for(i = 0; i < 4; i++)
        {
            band_table_u[(i + sao_band_pos_u) & 31] = i + 1;
            band_table_v[(i + sao_band_pos_v) & 31] = i + 1;
        }

        for(row = 0; row < ht; row++)
        {
            for(col = 0; col < wd; col++)
            {
                WORD32 band_idx;
                WORD8 *sao_offset;

                sao_offset = (0 == col % 2) ? sao_offset_u : sao_offset_v;
                band_idx = (0 == col % 2) ?
                           band_table_u[src[col] >> band_shift] :
                           band_table_v[src[col] >> band_shift];
                src[col] = CLIP3(src[col] + sao_offset[band_idx],
                                 0, (1 << (band_shift + 5)) - 1);
            }

            src += src_stride;
        }
    }
}

/* Horizontal filtering */
void ihevc_sao_edge_offset_class0_msa(UWORD8 *src, WORD32 src_stride,
                                      UWORD8 *src_left, UWORD8 *src_top,
                                      UWORD8 *src_top_left,
                                      UWORD8 *src_top_right,
                                      UWORD8 *src_bot_left, UWORD8 *avail,
                                      WORD8 *sao_offset, WORD32 wd, WORD32 ht)
{
    WORD32 row, col;
    UWORD8 amask[MAX_CTB_SIZE];
    UWORD8 asrc_left_tmp[MAX_CTB_SIZE];
    WORD8 sign_left, sign_right;
    WORD32 bit_depth;
    UNUSED(src_top_right);
    UNUSED(src_bot_left);

    bit_depth = BIT_DEPTH_LUMA;

    /* Initialize the mask values */
    memset(amask, 0xFF, MAX_CTB_SIZE);

    /* Update masks based on the availability flags */
    if(0 == avail[0])
    {
        amask[0] = 0;
    }

    if(0 == avail[1])
    {
        amask[wd - 1] = 0;
    }

    /* Update top and top-left arrays */
    *src_top_left = src_top[wd - 1];
    for(col = 0; col < wd; col++)
    {
        src_top[col] = src[(ht - 1) * src_stride + col];
    }

    for(row = 0; row < ht; row++)
    {
        asrc_left_tmp[row] = src[row * src_stride + wd - 1];
    }

    if(4 == wd)
    {
        hevc_sao_edge_0deg_4w_in_place_msa(src, src_stride, src_left, amask,
                                           sao_offset, ht);
    }
    else if(8 == wd)
    {
        hevc_sao_edge_0deg_8w_in_place_msa(src, src_stride, src_left, amask,
                                           sao_offset, ht);
    }
    else if(wd >> 2)
    {
        hevc_sao_edge_0deg_16w_in_place_msa(src, src_stride, src_left, amask,
                                            sao_offset, wd, ht);
    }
    else
    {
        for(row = 0; row < ht; row++)
        {
            sign_left = SIGN(src[0] - src_left[row]);
            for(col = 0; col < wd; col++)
            {
                WORD32 edge_idx;

                sign_right = SIGN(src[col] - src[col + 1]);
                edge_idx = 2 + sign_left + sign_right;
                sign_left = -sign_right;

                edge_idx = gi4_ihevc_table_edge_idx_msa[edge_idx] & amask[col];

                if(0 != edge_idx)
                {
                    src[col] = CLIP3(src[col] + sao_offset[edge_idx],
                                     0, (1 << bit_depth) - 1);
                }
            }

            src += src_stride;
        }
    }

    /* Update left array */
    for(row = 0; row < ht; row++)
    {
        src_left[row] = asrc_left_tmp[row];
    }
}

/* Vertical filtering */
void ihevc_sao_edge_offset_class1_msa(UWORD8 *src, WORD32 src_stride,
                                      UWORD8 *src_left, UWORD8 *src_top,
                                      UWORD8 *src_top_left,
                                      UWORD8 *src_top_right,
                                      UWORD8 *src_bot_left, UWORD8 *avail,
                                      WORD8 *sao_offset, WORD32 wd, WORD32 ht)
{
    WORD32 row, col;
    UWORD8 amask[MAX_CTB_SIZE];
    UWORD8 asrc_top_tmp[MAX_CTB_SIZE];
    WORD8 asign_up[MAX_CTB_SIZE];
    WORD8 sign_down;
    WORD32 bit_depth;
    UNUSED(src_top_right);
    UNUSED(src_bot_left);

    bit_depth = BIT_DEPTH_LUMA;

    /* Initialize the mask values */
    memset(amask, 0xFF, MAX_CTB_SIZE);

    /* Update left, top and top-left arrays */
    *src_top_left = src_top[wd - 1];
    for(row = 0; row < ht; row++)
    {
        src_left[row] = src[row * src_stride + wd - 1];
    }
    for(col = 0; col < wd; col++)
    {
        asrc_top_tmp[col] = src[(ht - 1) * src_stride + col];
    }

    /* Update height and source pointers based on the availability flags */

    if(0 == avail[3])
    {
        ht--;
    }

    if(4 == wd)
    {
        if(0 == avail[2])
        {
            hevc_sao_edge_90deg_4w_in_place_msa(src + src_stride, src_stride,
                                                src, sao_offset, (ht - 1));
        }
        else
        {
            hevc_sao_edge_90deg_4w_in_place_msa(src, src_stride, src_top,
                                                sao_offset, ht);
        }

        src += ht * src_stride;
    }
    else  if(8 == wd)
    {
        if(0 == avail[2])
        {
            hevc_sao_edge_90deg_8w_in_place_msa(src + src_stride, src_stride,
                                                src, sao_offset, (ht - 1));
        }
        else
        {
            hevc_sao_edge_90deg_8w_in_place_msa(src, src_stride, src_top,
                                                sao_offset, ht);
        }
        src += ht * src_stride;
    }
    else if(!(wd % 16))
    {
        if(0 == avail[2])
        {
            hevc_sao_edge_90deg_16w_in_place_msa(src + src_stride, src_stride,
                                                 src, sao_offset, wd, (ht - 1));
        }
        else
        {
            hevc_sao_edge_90deg_16w_in_place_msa(src, src_stride, src_top,
                                                 sao_offset, wd, ht);
        }
        src += ht * src_stride;
    }
    else
    {
        if(0 == avail[2])
        {
            src += src_stride;
            ht--;
            for(col = 0; col < wd; col++)
            {
                asign_up[col] = SIGN(src[col] - src[col - src_stride]);
            }
        }
        else
        {
            for(col = 0; col < wd; col++)
            {
                asign_up[col] = SIGN(src[col] - src_top[col]);
            }
        }

        for(row = 0; row < ht; row++)
        {
            for(col = 0; col < wd; col++)
            {
                WORD32 edge_idx;

                sign_down = SIGN(src[col] - src[col + src_stride]);
                edge_idx = 2 + asign_up[col] + sign_down;
                asign_up[col] = -sign_down;

                edge_idx = gi4_ihevc_table_edge_idx_msa[edge_idx] & amask[col];

                if(0 != edge_idx)
                {
                    src[col] = CLIP3(src[col] + sao_offset[edge_idx],
                                     0, (1 << bit_depth) - 1);
                }
            }

                src += src_stride;
            }
    }

    for(col = 0; col < wd; col++)
    {
        src_top[col] = asrc_top_tmp[col];
    }

}

/* 135 degree filtering */
void ihevc_sao_edge_offset_class2_msa(UWORD8 *src, WORD32 src_stride,
                                      UWORD8 *src_left, UWORD8 *src_top,
                                      UWORD8 *src_top_left,
                                      UWORD8 *src_top_right,
                                      UWORD8 *src_bot_left, UWORD8 *avail,
                                      WORD8 *sao_offset, WORD32 wd,
                                      WORD32 ht)
{
    WORD32 row, col;
    UWORD8 amask[MAX_CTB_SIZE];
    UWORD8 asrc_left_tmp[MAX_CTB_SIZE], asrc_top_tmp[MAX_CTB_SIZE];
    UWORD8 src_top_left_tmp;
    WORD8 asign_up[MAX_CTB_SIZE + 1], asign_up_tmp[MAX_CTB_SIZE + 1];
    WORD8 sign_down;
    WORD8 *sign_up, *sign_up_tmp;
    UWORD8 *src_left_cpy;
    WORD32 bit_depth;
    UWORD8 pos_0_0_tmp, pos_wd_ht_tmp;
    UNUSED(src_top_right);
    UNUSED(src_bot_left);

    bit_depth = BIT_DEPTH_LUMA;
    sign_up = asign_up;
    sign_up_tmp = asign_up_tmp;
    src_left_cpy = src_left;

    /* Initialize the mask values */
    memset(amask, 0xFF, MAX_CTB_SIZE);

    /* Update left, top and top-left arrays */
    src_top_left_tmp = src_top[wd - 1];
    for(row = 0; row < ht; row++)
    {
        asrc_left_tmp[row] = src[row * src_stride + wd - 1];
    }
    for(col = 0; col < wd; col++)
    {
        asrc_top_tmp[col] = src[(ht - 1) * src_stride + col];
    }


    /* If top-left is available, process separately */
    if(0 != avail[4])
    {
        WORD32 edge_idx;

        edge_idx = 2 + SIGN(src[0] - src_top_left[0]) +
                   SIGN(src[0] - src[1 + src_stride]);

        edge_idx = gi4_ihevc_table_edge_idx_msa[edge_idx];

        if(0 != edge_idx)
        {
            pos_0_0_tmp = CLIP3(src[0] + sao_offset[edge_idx],
                                0, (1 << bit_depth) - 1);
        }
        else
        {
            pos_0_0_tmp = src[0];
        }
    }
    else
    {
        pos_0_0_tmp = src[0];
    }

    /* If bottom-right is available, process separately */
    if(0 != avail[7])
    {
        WORD32 edge_idx;

        edge_idx = 2 + SIGN(src[wd - 1 + (ht - 1) * src_stride] -
                            src[wd - 1 + (ht - 1) * src_stride -
                            1 - src_stride]) +
                   SIGN(src[wd - 1 + (ht - 1) * src_stride] -
                        src[wd - 1 + (ht - 1) * src_stride + 1 + src_stride]);

        edge_idx = gi4_ihevc_table_edge_idx_msa[edge_idx];

        if(0 != edge_idx)
        {
            pos_wd_ht_tmp = CLIP3(src[wd - 1 + (ht - 1) * src_stride] +
                                  sao_offset[edge_idx],
                                  0, (1 << bit_depth) - 1);
        }
        else
        {
            pos_wd_ht_tmp = src[wd - 1 + (ht - 1) * src_stride];
        }
    }
    else
    {
        pos_wd_ht_tmp = src[wd - 1 + (ht - 1) * src_stride];
    }

    /* If Left is not available */
    if(0 == avail[0])
    {
        amask[0] = 0;
    }

    /* If Top is not available */
    if(0 == avail[2])
    {
        src += src_stride;
        ht--;
        src_left_cpy += 1;
        for(col = 1; col < wd; col++)
        {
            sign_up[col] = SIGN(src[col] - src[col - 1 - src_stride]);
        }
    }
    else
    {
        for(col = 1; col < wd; col++)
        {
            sign_up[col] = SIGN(src[col] - src_top[col - 1]);
        }
    }

    /* If Right is not available */
    if(0 == avail[1])
    {
        amask[wd - 1] = 0;
    }

    /* If Bottom is not available */
    if(0 == avail[3])
    {
        ht--;
    }

    /* Processing is done on the intermediate buffer and
       the output is written to the source buffer */
    {
        if(4 == wd)
        {
            hevc_sao_edge_135deg_4w_in_place_msa(src, src_stride, sign_up,
                                                 src_left_cpy, amask,
                                                 sao_offset, ht);
            src += ht * src_stride;
        }
        else if(!(wd % 8))
        {
            hevc_sao_edge_135deg_8w_in_place_msa(src, src_stride, sign_up,
                                                 src_left_cpy, amask,
                                                 sao_offset, wd, ht);
            src += ht * src_stride;
        }
        else
        {
            for(row = 0; row < ht; row++)
            {
                sign_up[0] = SIGN(src[0] - src_left_cpy[row - 1]);
                for(col = 0; col < wd; col++)
                {
                    WORD32 edge_idx;

                    sign_down = SIGN(src[col] - src[col + 1 + src_stride]);
                    edge_idx = 2 + sign_up[col] + sign_down;
                    sign_up_tmp[col + 1] = - sign_down;

                    edge_idx =
                        gi4_ihevc_table_edge_idx_msa[edge_idx] & amask[col];

                    if(0 != edge_idx)
                    {
                        src[col] = CLIP3(src[col] + sao_offset[edge_idx],
                                         0, (1 << bit_depth) - 1);
                    }
                }

                /* Swapping sign_up_tmp and sign_up */
                {
                    WORD8 *swap_tmp = sign_up;

                    sign_up = sign_up_tmp;
                    sign_up_tmp = swap_tmp;
                }

                src += src_stride;
            }
        }

        src[-(avail[2] ? ht : ht + 1) * src_stride] = pos_0_0_tmp;
        src[(avail[3] ? wd - 1 - src_stride : wd - 1)] = pos_wd_ht_tmp;
    }

    if(0 == avail[2])
        ht++;

        if(0 == avail[3])
        ht++;

    *src_top_left = src_top_left_tmp;
    for(row = 0; row < ht; row++)
    {
        src_left[row] = asrc_left_tmp[row];
    }

    for(col = 0; col < wd; col++)
    {
        src_top[col] = asrc_top_tmp[col];
    }
}

/* 45 degree filtering */
void ihevc_sao_edge_offset_class3_msa(UWORD8 *src, WORD32 src_stride,
                                      UWORD8 *src_left, UWORD8 *src_top,
                                      UWORD8 *src_top_left,
                                      UWORD8 *src_top_right,
                                      UWORD8 *src_bot_left,
                                      UWORD8 *avail, WORD8 *sao_offset,
                                      WORD32 wd, WORD32 ht)
{
    WORD32 row, col;
    UWORD8 amask[MAX_CTB_SIZE];
    UWORD8 asrc_top_tmp[MAX_CTB_SIZE];
    UWORD8 asrc_left_tmp[MAX_CTB_SIZE];
    UWORD8 src_top_left_tmp;
    WORD8 asign_up[MAX_CTB_SIZE];
    UWORD8 *src_left_cpy;
    WORD8 sign_down;
    WORD32 bit_depth;

    UWORD8 pos_0_ht_tmp;
    UWORD8 pos_wd_0_tmp;

    bit_depth = BIT_DEPTH_LUMA;
    src_left_cpy = src_left;

    /* Initialize the mask values */
    memset(amask, 0xFF, MAX_CTB_SIZE);

    /* Update left, top and top-left arrays */
    src_top_left_tmp = src_top[wd - 1];
    for(row = 0; row < ht; row++)
    {
        asrc_left_tmp[row] = src[row * src_stride + wd - 1];
    }

    for(col = 0; col < wd; col++)
    {
        asrc_top_tmp[col] = src[(ht - 1) * src_stride + col];
    }

    /* If top-right is available, process separately */
    if(0 != avail[5])
    {
        WORD32 edge_idx;

        edge_idx = 2 + SIGN(src[wd - 1] - src_top_right[0]) +
                        SIGN(src[wd - 1] - src[wd - 1 - 1 + src_stride]);

        edge_idx = gi4_ihevc_table_edge_idx_msa[edge_idx];

        if(0 != edge_idx)
        {
            pos_wd_0_tmp = CLIP3(src[wd - 1] + sao_offset[edge_idx],
                                 0, (1 << bit_depth) - 1);
        }
        else
        {
            pos_wd_0_tmp = src[wd - 1];
        }
    }
    else
    {
        pos_wd_0_tmp = src[wd - 1];
    }

    /* If bottom-left is available, process separately */
    if(0 != avail[6])
    {
        WORD32 edge_idx;

        edge_idx = 2 + SIGN(src[(ht - 1) * src_stride] -
                            src[(ht - 1) * src_stride + 1 - src_stride]) +
                   SIGN(src[(ht - 1) * src_stride] - src_bot_left[0]);

        edge_idx = gi4_ihevc_table_edge_idx_msa[edge_idx];

        if(0 != edge_idx)
        {
            pos_0_ht_tmp = CLIP3(src[(ht - 1) * src_stride] +
                                 sao_offset[edge_idx], 0, (1 << bit_depth) - 1);
        }
        else
        {
            pos_0_ht_tmp = src[(ht - 1) * src_stride];
        }
    }
    else
    {
        pos_0_ht_tmp = src[(ht - 1) * src_stride];
    }

    /* If Left is not available */
    if(0 == avail[0])
    {
        amask[0] = 0;
    }

    /* If Top is not available */
    if(0 == avail[2])
    {
        src += src_stride;
        ht--;
        src_left_cpy += 1;
        for(col = 0; col < wd - 1; col++)
        {
            asign_up[col] = SIGN(src[col] - src[col + 1 - src_stride]);
        }
    }
    else
    {
        for(col = 0; col < wd - 1; col++)
        {
            asign_up[col] = SIGN(src[col] - src_top[col + 1]);
        }
    }

    /* If Right is not available */
    if(0 == avail[1])
    {
        amask[wd - 1] = 0;
    }

    /* If Bottom is not available */
    if(0 == avail[3])
    {
        ht--;
    }

    if(4 == wd)
    {
        hevc_sao_edge_45deg_4w_in_place_msa(src, src_stride, asign_up,
                                            src_left_cpy, amask,
                                            sao_offset, ht);
        src += ht * src_stride;
    }
    else if(!(wd % 8))
    {
        hevc_sao_edge_45deg_8w_in_place_msa(src, src_stride, asign_up,
                                            src_left_cpy, amask,
                                            sao_offset, wd, ht);
        src += ht * src_stride;
    }
    else
    {
        for(row = 0; row < ht; row++)
        {
            asign_up[wd - 1] = SIGN(src[wd - 1] -
                                    src[wd - 1 + 1 - src_stride]);
            for(col = 0; col < wd; col++)
            {
                WORD32 edge_idx;

                sign_down = SIGN(src[col] - ((col == 0) ?
                                 src_left_cpy[row + 1] :
                                 src[col - 1 + src_stride]));

                edge_idx = 2 + asign_up[col] + sign_down;

                if(col > 0)
                {
                    asign_up[col - 1] = -sign_down;
                }

                edge_idx = gi4_ihevc_table_edge_idx_msa[edge_idx] &
                           amask[col];

                if(0 != edge_idx)
                {
                    src[col] = CLIP3(src[col] + sao_offset[edge_idx],
                                     0, (1 << bit_depth) - 1);
                }
            }

            src += src_stride;
        }
    }

    src[-(avail[2] ? ht : ht + 1) * src_stride + wd - 1] = pos_wd_0_tmp;
    src[(avail[3] ?  (-src_stride) : 0)] = pos_0_ht_tmp;

    if(0 == avail[2])
    {
        ht++;
    }

    if(0 == avail[3])
    {
        ht++;
    }

    *src_top_left = src_top_left_tmp;
    for(row = 0; row < ht; row++)
    {
        src_left[row] = asrc_left_tmp[row];
    }
    for(col = 0; col < wd; col++)
    {
        src_top[col] = asrc_top_tmp[col];
    }
}

void ihevc_sao_edge_offset_class0_chroma_msa(UWORD8 *src, WORD32 src_stride,
                                             UWORD8 *src_left, UWORD8 *src_top,
                                             UWORD8 *src_top_left,
                                             UWORD8 *src_top_right,
                                             UWORD8 *src_bot_left,
                                             UWORD8 *avail,
                                             WORD8 *sao_offset_u,
                                             WORD8 *sao_offset_v,
                                             WORD32 wd, WORD32 ht)
{
    WORD32 row, col;
    UWORD8 amask[MAX_CTB_SIZE];
    UWORD8 asrc_left_tmp[2 * MAX_CTB_SIZE];
    WORD8 sign_left_u, sign_right_u, sign_left_v, sign_right_v;
    WORD32 bit_depth;
    UNUSED(src_top_right);
    UNUSED(src_bot_left);

    bit_depth = BIT_DEPTH_CHROMA;

    /* Initialize the mask values */
    memset(amask, 0xFF, MAX_CTB_SIZE);

    /* Update left, top and top-left arrays */
    src_top_left[0] = src_top[wd - 2];
    src_top_left[1] = src_top[wd - 1];
    for(row = 0; row < ht; row++)
    {
        asrc_left_tmp[2 * row] = src[row * src_stride + wd - 2];
        asrc_left_tmp[2 * row + 1] = src[row * src_stride + wd - 1];
    }

    for(col = 0; col < wd; col++)
    {
        src_top[col] = src[(ht - 1) * src_stride + col];
    }

    /* Update masks based on the availability flags */
    if(0 == avail[0])
    {
        amask[0] = 0;
    }

    if(0 == avail[1])
    {
        amask[(wd - 1) >> 1] = 0;
    }

    /* Processing is done on the intermediate buffer and
       the output is written to the source buffer */
    if(4 == wd)
    {
        hevc_sao_edge_chroma_0deg_4w_in_place_msa(src, src_stride, src_left,
                                                  amask, sao_offset_u,
                                                  sao_offset_v, ht);
        src += ht * src_stride;
    }
    else if(8 == wd)
    {
        hevc_sao_edge_chroma_0deg_8w_in_place_msa(src, src_stride, src_left,
                                                  amask, sao_offset_u,
                                                  sao_offset_v, ht);
        src += ht * src_stride;
    }
    else if(wd % 4)
    {
        hevc_sao_edge_chroma_0deg_16w_in_place_msa(src, src_stride, src_left,
                                                   amask, sao_offset_u,
                                                   sao_offset_v, wd, ht);
        src += ht * src_stride;
    }
    else
    {
        for(row = 0; row < ht; row++)
        {
            sign_left_u = SIGN(src[0] - src_left[2 * row]);
            sign_left_v = SIGN(src[1] - src_left[2 * row + 1]);

            for(col = 0; col < wd; col++)
            {
                WORD32 edge_idx;
                WORD8 *sao_offset;

                if(0 == col % 2)
                {
                    sao_offset = sao_offset_u;
                    sign_right_u = SIGN(src[col] - src[col + 2]);
                    edge_idx = 2 + sign_left_u + sign_right_u;
                    sign_left_u = - sign_right_u;
                }
                else
                {
                    sao_offset = sao_offset_v;
                    sign_right_v = SIGN(src[col] - src[col + 2]);
                    edge_idx = 2 + sign_left_v + sign_right_v;
                    sign_left_v = - sign_right_v;
                }

                edge_idx = gi4_ihevc_table_edge_idx_msa[edge_idx] &
                           amask[col >> 1];

                if(0 != edge_idx)
                {
                    src[col] = CLIP3(src[col] + sao_offset[edge_idx],
                                     0, (1 << bit_depth) - 1);
                }
            }

            src += src_stride;
        }
    }

    for(row = 0; row < 2 * ht; row++)
    {
        src_left[row] = asrc_left_tmp[row];
    }

}

/* input 'wd' has to be for the interleaved block and not for each color component */
void ihevc_sao_edge_offset_class1_chroma_msa(UWORD8 *src,
                                             WORD32 src_stride,
                                             UWORD8 *src_left,
                                             UWORD8 *src_top,
                                             UWORD8 *src_top_left,
                                             UWORD8 *src_top_right,
                                             UWORD8 *src_bot_left,
                                             UWORD8 *avail,
                                             WORD8 *sao_offset_u,
                                             WORD8 *sao_offset_v,
                                             WORD32 wd,
                                             WORD32 ht)
{
    WORD32 row, col;
    UWORD8 amask[MAX_CTB_SIZE];
    UWORD8 asrc_top_tmp[MAX_CTB_SIZE];
    WORD8 asign_up[MAX_CTB_SIZE];
    WORD8 sign_down;
    WORD32 bit_depth;
    UNUSED(src_top_right);
    UNUSED(src_bot_left);

    bit_depth = BIT_DEPTH_CHROMA;

    /* Initialize the mask values */
    memset(amask, 0xFF, MAX_CTB_SIZE);

    /* Update left, top and top-left arrays */
    src_top_left[0] = src_top[wd - 2];
    src_top_left[1] = src_top[wd - 1];
    for(row = 0; row < ht; row++)
    {
        src_left[2 * row] = src[row * src_stride + wd - 2];
        src_left[2 * row + 1] = src[row * src_stride + wd - 1];
    }

    for(col = 0; col < wd; col++)
    {
        asrc_top_tmp[col] = src[(ht - 1) * src_stride + col];
    }

    /* Update height and source pointers based on the availability flags */
    if(0 == avail[2])
    {
        src += src_stride;
        ht--;
        for(col = 0; col < wd; col++)
        {
            asign_up[col] = SIGN(src[col] - src[col - src_stride]);
        }
    }
    else
    {
        for(col = 0; col < wd; col++)
        {
            asign_up[col] = SIGN(src[col] - src_top[col]);
        }
    }

    if(0 == avail[3])
    {
        ht--;
    }

    /* Processing is done on the intermediate buffer and
       the output is written to the source buffer */
    if(4 == wd)
    {
        hevc_sao_edge_chroma_90deg_4w_in_place_msa(src, src_stride,
                                                   asign_up, amask,
                                                   sao_offset_u,
                                                   sao_offset_v, ht);
    }
    else if(8 == wd)
    {
        hevc_sao_edge_chroma_90deg_8w_in_place_msa(src, src_stride,
                                                   asign_up, amask,
                                                   sao_offset_u,
                                                   sao_offset_v, ht);
    }
    else if(wd >> 4)
    {
        hevc_sao_edge_chroma_90deg_16w_in_place_msa(src, src_stride,
                                                    asign_up, amask,
                                                    sao_offset_u,
                                                    sao_offset_v, wd, ht);
    }
    else
    {
        for(row = 0; row < ht; row++)
        {
            for(col = 0; col < wd; col++)
            {
                WORD32 edge_idx;
                WORD8 *sao_offset;

                sao_offset = (0 == col % 2) ? sao_offset_u : sao_offset_v;

                sign_down = SIGN(src[col] - src[col + src_stride]);
                edge_idx = 2 + asign_up[col] + sign_down;
                asign_up[col] = -sign_down;

                edge_idx = gi4_ihevc_table_edge_idx_msa[edge_idx] &
                           amask[col >> 1];

                if(0 != edge_idx)
                {
                    src[col] = CLIP3(src[col] + sao_offset[edge_idx],
                                     0, (1 << bit_depth) - 1);
                }
            }

            src += src_stride;
        }
    }

    for(col = 0; col < wd; col++)
    {
        src_top[col] = asrc_top_tmp[col];
    }
}

/* 135 degree filtering */
void ihevc_sao_edge_offset_class2_chroma_msa(UWORD8 *src, WORD32 src_stride,
                                             UWORD8 *src_left, UWORD8 *src_top,
                                             UWORD8 *src_top_left,
                                             UWORD8 *src_top_right,
                                             UWORD8 *src_bot_left,
                                             UWORD8 *avail,
                                             WORD8 *sao_offset_u,
                                             WORD8 *sao_offset_v,
                                             WORD32 wd, WORD32 ht)
{
    WORD32 row, col;
    UWORD8 amask[MAX_CTB_SIZE];
    UWORD8 asrc_left_tmp[2 * MAX_CTB_SIZE], asrc_top_tmp[MAX_CTB_SIZE];
    UWORD8 asrc_top_left_tmp[2];
    WORD8 asign_up[MAX_CTB_SIZE + 2], asign_up_tmp[MAX_CTB_SIZE + 2];
    WORD8 sign_down;
    WORD8 *sign_up, *sign_up_tmp;
    UWORD8 *src_left_cpy;
    WORD32 bit_depth;
    UWORD8 pos_0_0_tmp_u;
    UWORD8 pos_0_0_tmp_v;
    UWORD8 pos_wd_ht_tmp_u;
    UWORD8 pos_wd_ht_tmp_v;
    UNUSED(src_top_right);
    UNUSED(src_bot_left);

    bit_depth = BIT_DEPTH_CHROMA;
    sign_up = asign_up;
    sign_up_tmp = asign_up_tmp;
    src_left_cpy = src_left;

    /* Initialize the mask values */
    memset(amask, 0xFF, MAX_CTB_SIZE);

    /* Update left, top and top-left arrays */
    asrc_top_left_tmp[0] = src_top[wd - 2];
    asrc_top_left_tmp[1] = src_top[wd - 1];
    for(row = 0; row < ht; row++)
    {
        asrc_left_tmp[2 * row] = src[row * src_stride + wd - 2];
        asrc_left_tmp[2 * row + 1] = src[row * src_stride + wd - 1];
    }

    for(col = 0; col < wd; col++)
    {
        asrc_top_tmp[col] = src[(ht - 1) * src_stride + col];
    }


    /* If top-left is available, process separately */
    if(0 != avail[4])
    {
        WORD32 edge_idx;

        /* U */
        edge_idx = 2 + SIGN(src[0] - src_top_left[0]) +
                        SIGN(src[0] - src[2 + src_stride]);

        edge_idx = gi4_ihevc_table_edge_idx_msa[edge_idx];

        if(0 != edge_idx)
        {
            pos_0_0_tmp_u = CLIP3(src[0] + sao_offset_u[edge_idx],
                                  0, (1 << bit_depth) - 1);
        }
        else
        {
            pos_0_0_tmp_u = src[0];
        }

        /* V */
        edge_idx = 2 + SIGN(src[1] - src_top_left[1]) +
                        SIGN(src[1] - src[1 + 2 + src_stride]);

        edge_idx = gi4_ihevc_table_edge_idx_msa[edge_idx];

        if(0 != edge_idx)
        {
            pos_0_0_tmp_v = CLIP3(src[1] + sao_offset_v[edge_idx],
                                  0, (1 << bit_depth) - 1);
        }
        else
        {
            pos_0_0_tmp_v = src[1];
        }
    }
    else
    {
        pos_0_0_tmp_u = src[0];
        pos_0_0_tmp_v = src[1];
    }

    /* If bottom-right is available, process separately */
    if(0 != avail[7])
    {
        WORD32 edge_idx;

        /* U */
        edge_idx = 2 + SIGN(src[wd - 2 + (ht - 1) * src_stride] -
                            src[wd - 2 + (ht - 1) * src_stride -
                                2 - src_stride]) +
                   SIGN(src[wd - 2 + (ht - 1) * src_stride] -
                        src[wd - 2 + (ht - 1) * src_stride + 2 + src_stride]);

        edge_idx = gi4_ihevc_table_edge_idx_msa[edge_idx];

        if(0 != edge_idx)
        {
            pos_wd_ht_tmp_u = CLIP3(src[wd - 2 + (ht - 1) * src_stride] +
                                    sao_offset_u[edge_idx],
                                    0, (1 << bit_depth) - 1);
        }
        else
        {
            pos_wd_ht_tmp_u = src[wd - 2 + (ht - 1) * src_stride];
        }

        /* V */
        edge_idx = 2 + SIGN(src[wd - 1 + (ht - 1) * src_stride] -
                            src[wd - 1 + (ht - 1) * src_stride -
                            2 - src_stride]) +
                   SIGN(src[wd - 1 + (ht - 1) * src_stride] -
                        src[wd - 1 + (ht - 1) * src_stride + 2 + src_stride]);

        edge_idx = gi4_ihevc_table_edge_idx_msa[edge_idx];

        if(0 != edge_idx)
        {
            pos_wd_ht_tmp_v = CLIP3(src[wd - 1 + (ht - 1) * src_stride] +
                                    sao_offset_v[edge_idx],
                                    0, (1 << bit_depth) - 1);
        }
        else
        {
            pos_wd_ht_tmp_v = src[wd - 1 + (ht - 1) * src_stride];
        }
    }
    else
    {
        pos_wd_ht_tmp_u = src[wd - 2 + (ht - 1) * src_stride];
        pos_wd_ht_tmp_v = src[wd - 1 + (ht - 1) * src_stride];
    }

    /* If Left is not available */
    if(0 == avail[0])
    {
        amask[0] = 0;
    }

    /* If Top is not available */
    if(0 == avail[2])
    {
        src += src_stride;
        src_left_cpy += 2;
        ht--;
        for(col = 2; col < wd; col++)
        {
            sign_up[col] = SIGN(src[col] - src[col - 2 - src_stride]);
        }
    }
    else
    {
        for(col = 2; col < wd; col++)
        {
            sign_up[col] = SIGN(src[col] - src_top[col - 2]);
        }
    }

    /* If Right is not available */
    if(0 == avail[1])
    {
        amask[(wd - 1) >> 1] = 0;
    }

    /* If Bottom is not available */
    if(0 == avail[3])
    {
        ht--;
    }

    /* Processing is done on the intermediate buffer and
       the output is written to the source buffer */
    {
        if(4 == wd)
        {
            hevc_sao_edge_chroma_135deg_4w_in_place_msa(src, src_stride,
                                                        sign_up, src_left_cpy,
                                                        amask, sao_offset_u,
                                                        sao_offset_v, ht);
            src += ht * src_stride;
        }
        else if(!(wd % 8))
        {
            hevc_sao_edge_chroma_135deg_8w_in_place_msa(src, src_stride,
                                                        sign_up, src_left_cpy,
                                                        amask, sao_offset_u,
                                                        sao_offset_v, wd, ht);
            src += ht * src_stride;
        }
        else
        {
            for(row = 0; row < ht; row++)
            {
                sign_up[0] = SIGN(src[0] - src_left_cpy[2 * (row - 1)]);
                sign_up[1] = SIGN(src[1] - src_left_cpy[2 * (row - 1) + 1]);
                for(col = 0; col < wd; col++)
                {
                    WORD32 edge_idx;
                    WORD8 *sao_offset;

                    sao_offset = (0 == col % 2) ? sao_offset_u : sao_offset_v;

                    sign_down = SIGN(src[col] - src[col + 2 + src_stride]);
                    edge_idx = 2 + sign_up[col] + sign_down;
                    sign_up_tmp[col + 2] = -sign_down;

                    edge_idx = gi4_ihevc_table_edge_idx_msa[edge_idx] &
                               amask[col >> 1];

                    if(0 != edge_idx)
                    {
                        src[col] = CLIP3(src[col] + sao_offset[edge_idx],
                                         0, (1 << bit_depth) - 1);
                    }
                }

                /* Swapping sign_up_tmp and sign_up */
                {
                    WORD8 *swap_tmp = sign_up;
                    sign_up = sign_up_tmp;
                    sign_up_tmp = swap_tmp;
                }

                src += src_stride;
            }
        }

        src[-(avail[2] ? ht : ht + 1) * src_stride] = pos_0_0_tmp_u;
        src[-(avail[2] ? ht : ht + 1) * src_stride + 1] = pos_0_0_tmp_v;
        src[(avail[3] ? wd - 2 - src_stride : wd - 2)] = pos_wd_ht_tmp_u;
        src[(avail[3] ? wd - 1 - src_stride : wd - 1)] = pos_wd_ht_tmp_v;
    }

    if(0 == avail[2])
    {
        ht++;
    }

    if(0 == avail[3])
    {
        ht++;
    }

    src_top_left[0] = asrc_top_left_tmp[0];
    src_top_left[1] = asrc_top_left_tmp[1];
    for(row = 0; row < 2 * ht; row++)
    {
        src_left[row] = asrc_left_tmp[row];
    }

    for(col = 0; col < wd; col++)
    {
        src_top[col] = asrc_top_tmp[col];
    }
}

void ihevc_sao_edge_offset_class3_chroma_msa(UWORD8 *src, WORD32 src_stride,
                                             UWORD8 *src_left, UWORD8 *src_top,
                                             UWORD8 *src_top_left,
                                             UWORD8 *src_top_right,
                                             UWORD8 *src_bot_left,
                                             UWORD8 *avail,
                                             WORD8 *sao_offset_u,
                                             WORD8 *sao_offset_v,
                                             WORD32 wd, WORD32 ht)
{
    WORD32 row, col;
    UWORD8 amask[MAX_CTB_SIZE];
    UWORD8 asrc_left_tmp[2 * MAX_CTB_SIZE], asrc_top_tmp[MAX_CTB_SIZE];
    UWORD8 asrc_top_left_tmp[2];
    WORD8 asign_up[MAX_CTB_SIZE];
    UWORD8 *src_left_cpy;
    WORD8 sign_down;
    WORD32 bit_depth;
    UWORD8 pos_wd_0_tmp_u;
    UWORD8 pos_wd_0_tmp_v;
    UWORD8 pos_0_ht_tmp_u;
    UWORD8 pos_0_ht_tmp_v;

    bit_depth = BIT_DEPTH_CHROMA;
    src_left_cpy = src_left;

    /* Initialize the mask values */
    memset(amask, 0xFF, MAX_CTB_SIZE);

    /* Update left, top and top-left arrays */
    asrc_top_left_tmp[0] = src_top[wd - 2];
    asrc_top_left_tmp[1] = src_top[wd - 1];
    for(row = 0; row < ht; row++)
    {
        asrc_left_tmp[2 * row] = src[row * src_stride + wd - 2];
        asrc_left_tmp[2 * row + 1] = src[row * src_stride + wd - 1];
    }

    for(col = 0; col < wd; col++)
    {
        asrc_top_tmp[col] = src[(ht - 1) * src_stride + col];
    }

    /* If top-right is available, process separately */
    if(0 != avail[5])
    {
        WORD32 edge_idx;

        /* U */
        edge_idx = 2 + SIGN(src[wd - 2] - src_top_right[0]) +
                   SIGN(src[wd - 2] - src[wd - 2 - 2 + src_stride]);

        edge_idx = gi4_ihevc_table_edge_idx_msa[edge_idx];

        if(0 != edge_idx)
        {
            pos_wd_0_tmp_u = CLIP3(src[wd - 2] + sao_offset_u[edge_idx],
                                   0, (1 << bit_depth) - 1);
        }
        else
        {
            pos_wd_0_tmp_u = src[wd - 2];
        }

        /* V */
        edge_idx = 2 + SIGN(src[wd - 1] - src_top_right[1]) +
                   SIGN(src[wd - 1] - src[wd - 1 - 2 + src_stride]);

        edge_idx = gi4_ihevc_table_edge_idx_msa[edge_idx];

        if(0 != edge_idx)
        {
            pos_wd_0_tmp_v = CLIP3(src[wd - 1] + sao_offset_v[edge_idx],
                                   0, (1 << bit_depth) - 1);
        }
        else
        {
            pos_wd_0_tmp_v = src[wd - 1];
        }
    }
    else
    {
        pos_wd_0_tmp_u = src[wd - 2];
        pos_wd_0_tmp_v = src[wd - 1];
    }

    /* If bottom-left is available, process separately */
    if(0 != avail[6])
    {
        WORD32 edge_idx;

        /* U */
        edge_idx = 2 + SIGN(src[(ht - 1) * src_stride] -
                            src[(ht - 1) * src_stride + 2 - src_stride]) +
                   SIGN(src[(ht - 1) * src_stride] - src_bot_left[0]);

        edge_idx = gi4_ihevc_table_edge_idx_msa[edge_idx];

        if(0 != edge_idx)
        {
            pos_0_ht_tmp_u = CLIP3(src[(ht - 1) * src_stride] +
                                   sao_offset_u[edge_idx],
                                   0, (1 << bit_depth) - 1);
        }
        else
        {
            pos_0_ht_tmp_u = src[(ht - 1) * src_stride];
        }

        /* V */
        edge_idx = 2 + SIGN(src[(ht - 1) * src_stride + 1] -
                           src[(ht - 1) * src_stride + 1 + 2 - src_stride]) +
                   SIGN(src[(ht - 1) * src_stride + 1] - src_bot_left[1]);

        edge_idx = gi4_ihevc_table_edge_idx_msa[edge_idx];

        if(0 != edge_idx)
        {
            pos_0_ht_tmp_v = CLIP3(src[(ht - 1) * src_stride + 1] +
                                   sao_offset_v[edge_idx],
                                   0, (1 << bit_depth) - 1);
        }
        else
        {
            pos_0_ht_tmp_v = src[(ht - 1) * src_stride + 1];
        }
    }
    else
    {
        pos_0_ht_tmp_u = src[(ht - 1) * src_stride];
        pos_0_ht_tmp_v = src[(ht - 1) * src_stride + 1];
    }

    /* If Left is not available */
    if(0 == avail[0])
    {
        amask[0] = 0;
    }

    /* If Top is not available */
    if(0 == avail[2])
    {
        src += src_stride;
        ht--;
        src_left_cpy += 2;
        for(col = 0; col < wd - 2; col++)
        {
            asign_up[col] = SIGN(src[col] - src[col + 2 - src_stride]);
        }
    }
    else
    {
        for(col = 0; col < wd - 2; col++)
        {
            asign_up[col] = SIGN(src[col] - src_top[col + 2]);
        }
    }

    /* If Right is not available */
    if(0 == avail[1])
    {
        amask[(wd - 1) >> 1] = 0;
    }

    /* If Bottom is not available */
    if(0 == avail[3])
    {
        ht--;
    }

    /* Processing is done on the intermediate buffer and
       the output is written to the source buffer */
    if(4 == wd)
    {
        hevc_sao_edge_chroma_45deg_4w_in_place_msa(src, src_stride,
                                                   asign_up, src_left_cpy,
                                                   amask, sao_offset_u,
                                                   sao_offset_v, ht);
        src += ht * src_stride;
    }
    else if(!(wd % 8))
    {
        hevc_sao_edge_chroma_45deg_8w_in_place_msa(src, src_stride,
                                                   asign_up, src_left_cpy,
                                                   amask, sao_offset_u,
                                                   sao_offset_v, wd, ht);
        src += ht * src_stride;
    }
    else
    {
        for(row = 0; row < ht; row++)
        {
            asign_up[wd - 2] = SIGN(src[wd - 2] -
                                    src[wd - 2 + 2 - src_stride]);
            asign_up[wd - 1] = SIGN(src[wd - 1] -
                                    src[wd - 1 + 2 - src_stride]);
            for(col = 0; col < wd; col++)
            {
                WORD32 edge_idx;
                WORD8 *sao_offset;

                sao_offset = (0 == col % 2) ? sao_offset_u : sao_offset_v;

                sign_down = SIGN(src[col] - ((col < 2) ?
                                 src_left_cpy[2 * (row + 1) + col] :
                                 src[col - 2 + src_stride]));
                edge_idx = 2 + asign_up[col] + sign_down;
                if(col > 1)
                    asign_up[col - 2] = -sign_down;

                edge_idx = gi4_ihevc_table_edge_idx_msa[edge_idx] &
                           amask[col >> 1];

                if(0 != edge_idx)
                {
                    src[col] = CLIP3(src[col] + sao_offset[edge_idx],
                                     0, (1 << bit_depth) - 1);
                }
            }

            src += src_stride;
        }
    }

    src[-(avail[2] ? ht : ht + 1) * src_stride + wd - 2] = pos_wd_0_tmp_u;
    src[-(avail[2] ? ht : ht + 1) * src_stride + wd - 1] = pos_wd_0_tmp_v;
    src[(avail[3] ?  (-src_stride) : 0)] = pos_0_ht_tmp_u;
    src[(avail[3] ?  (-src_stride) : 0) + 1] = pos_0_ht_tmp_v;

    if(0 == avail[2])
    {
        ht++;
    }

    if(0 == avail[3])
    {
        ht++;
    }

    src_top_left[0] = asrc_top_left_tmp[0];
    src_top_left[1] = asrc_top_left_tmp[1];

    for(row = 0; row < 2 * ht; row++)
    {
        src_left[row] = asrc_left_tmp[row];
    }

    for(col = 0; col < wd; col++)
    {
        src_top[col] = asrc_top_tmp[col];
    }
}
