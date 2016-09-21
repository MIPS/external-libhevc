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
#include "ihevc_recon.h"
#include "ihevc_macros.h"
#include "ihevc_macros_msa.h"

static void hevc_addblk_zerocol_4x4_msa(WORD16 *coeffs, UWORD8 *dst,
                                        WORD32 dst_stride, WORD32 zero_cols)
{
    UWORD32 dst0, dst1, dst2, dst3;
    v8i16 dst0_r, dst0_l, in0, in1;
    v16u8 zeros = { 0 };
    v4i32 dst_vec = { 0 };

    LD_SH2(coeffs, 8, in0, in1);

    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 0, 0);
        in0 = __msa_insert_h(in0, 4, 0);
        in1 = __msa_insert_h(in1, 0, 0);
        in1 = __msa_insert_h(in1, 4, 0);
    }

    zero_cols >>= 1;
    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 1, 0);
        in0 = __msa_insert_h(in0, 5, 0);
        in1 = __msa_insert_h(in1, 1, 0);
        in1 = __msa_insert_h(in1, 5, 0);
    }

    zero_cols >>= 1;
    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 2, 0);
        in0 = __msa_insert_h(in0, 6, 0);
        in1 = __msa_insert_h(in1, 2, 0);
        in1 = __msa_insert_h(in1, 6, 0);
    }

    zero_cols >>= 1;
    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 3, 0);
        in0 = __msa_insert_h(in0, 7, 0);
        in1 = __msa_insert_h(in1, 3, 0);
        in1 = __msa_insert_h(in1, 7, 0);
    }

    LW4(dst, dst_stride, dst0, dst1, dst2, dst3);
    INSERT_W4_SW(dst0, dst1, dst2, dst3, dst_vec);
    ILVRL_B2_SH(zeros, dst_vec, dst0_r, dst0_l);
    ADD2(dst0_r, in0, dst0_l, in1, dst0_r, dst0_l);
    CLIP_SH2_0_255(dst0_r, dst0_l);
    dst_vec = (v4i32) __msa_pckev_b((v16i8) dst0_l, (v16i8) dst0_r);
    ST4x4_UB(dst_vec, dst_vec, 0, 1, 2, 3, dst, dst_stride);
}

static void hevc_addblk_zerocol_8x8_msa(WORD16 *coeffs, UWORD8 *dst,
                                        WORD32 dst_stride, WORD32 zero_cols)
{
    ULWORD64 dst0, dst1, dst2, dst3;
    v8i16 dst0_r, dst0_l, dst1_r, dst1_l;
    v8i16 in0, in1, in2, in3, in4, in5, in6, in7;
    v16u8 zeros = { 0 };
    v2i64 dst_vec0 = { 0 };
    v2i64 dst_vec1 = { 0 };

    LD_SH8(coeffs, 8, in0, in1, in2, in3, in4, in5, in6, in7);

    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 0, 0);
        in1 = __msa_insert_h(in1, 0, 0);
        in2 = __msa_insert_h(in2, 0, 0);
        in3 = __msa_insert_h(in3, 0, 0);
        in4 = __msa_insert_h(in4, 0, 0);
        in5 = __msa_insert_h(in5, 0, 0);
        in6 = __msa_insert_h(in6, 0, 0);
        in7 = __msa_insert_h(in7, 0, 0);
    }

    zero_cols >>= 1;
    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 1, 0);
        in1 = __msa_insert_h(in1, 1, 0);
        in2 = __msa_insert_h(in2, 1, 0);
        in3 = __msa_insert_h(in3, 1, 0);
        in4 = __msa_insert_h(in4, 1, 0);
        in5 = __msa_insert_h(in5, 1, 0);
        in6 = __msa_insert_h(in6, 1, 0);
        in7 = __msa_insert_h(in7, 1, 0);
    }

    zero_cols >>= 1;
    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 2, 0);
        in1 = __msa_insert_h(in1, 2, 0);
        in2 = __msa_insert_h(in2, 2, 0);
        in3 = __msa_insert_h(in3, 2, 0);
        in4 = __msa_insert_h(in4, 2, 0);
        in5 = __msa_insert_h(in5, 2, 0);
        in6 = __msa_insert_h(in6, 2, 0);
        in7 = __msa_insert_h(in7, 2, 0);
    }

    zero_cols >>= 1;
    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 3, 0);
        in1 = __msa_insert_h(in1, 3, 0);
        in2 = __msa_insert_h(in2, 3, 0);
        in3 = __msa_insert_h(in3, 3, 0);
        in4 = __msa_insert_h(in4, 3, 0);
        in5 = __msa_insert_h(in5, 3, 0);
        in6 = __msa_insert_h(in6, 3, 0);
        in7 = __msa_insert_h(in7, 3, 0);
    }

    zero_cols >>= 1;
    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 4, 0);
        in1 = __msa_insert_h(in1, 4, 0);
        in2 = __msa_insert_h(in2, 4, 0);
        in3 = __msa_insert_h(in3, 4, 0);
        in4 = __msa_insert_h(in4, 4, 0);
        in5 = __msa_insert_h(in5, 4, 0);
        in6 = __msa_insert_h(in6, 4, 0);
        in7 = __msa_insert_h(in7, 4, 0);
    }

    zero_cols >>= 1;
    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 5, 0);
        in1 = __msa_insert_h(in1, 5, 0);
        in2 = __msa_insert_h(in2, 5, 0);
        in3 = __msa_insert_h(in3, 5, 0);
        in4 = __msa_insert_h(in4, 5, 0);
        in5 = __msa_insert_h(in5, 5, 0);
        in6 = __msa_insert_h(in6, 5, 0);
        in7 = __msa_insert_h(in7, 5, 0);
    }

    zero_cols >>= 1;
    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 6, 0);
        in1 = __msa_insert_h(in1, 6, 0);
        in2 = __msa_insert_h(in2, 6, 0);
        in3 = __msa_insert_h(in3, 6, 0);
        in4 = __msa_insert_h(in4, 6, 0);
        in5 = __msa_insert_h(in5, 6, 0);
        in6 = __msa_insert_h(in6, 6, 0);
        in7 = __msa_insert_h(in7, 6, 0);
    }

    zero_cols >>= 1;
    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 7, 0);
        in1 = __msa_insert_h(in1, 7, 0);
        in2 = __msa_insert_h(in2, 7, 0);
        in3 = __msa_insert_h(in3, 7, 0);
        in4 = __msa_insert_h(in4, 7, 0);
        in5 = __msa_insert_h(in5, 7, 0);
        in6 = __msa_insert_h(in6, 7, 0);
        in7 = __msa_insert_h(in7, 7, 0);
    }

    LD4(dst, dst_stride, dst0, dst1, dst2, dst3);
    INSERT_D2_SD(dst0, dst1, dst_vec0);
    INSERT_D2_SD(dst2, dst3, dst_vec1);
    ILVRL_B2_SH(zeros, dst_vec0, dst0_r, dst0_l);
    ILVRL_B2_SH(zeros, dst_vec1, dst1_r, dst1_l);
    ADD4(dst0_r, in0, dst0_l, in1, dst1_r, in2, dst1_l, in3, dst0_r, dst0_l,
         dst1_r, dst1_l);
    CLIP_SH4_0_255(dst0_r, dst0_l, dst1_r, dst1_l);
    PCKEV_B2_SH(dst0_l, dst0_r, dst1_l, dst1_r, dst0_r, dst1_r);
    ST8x4_UB(dst0_r, dst1_r, dst, dst_stride);
    dst += (4 * dst_stride);

    LD4(dst, dst_stride, dst0, dst1, dst2, dst3);
    INSERT_D2_SD(dst0, dst1, dst_vec0);
    INSERT_D2_SD(dst2, dst3, dst_vec1);
    UNPCK_UB_SH(dst_vec0, dst0_r, dst0_l);
    UNPCK_UB_SH(dst_vec1, dst1_r, dst1_l);
    ADD4(dst0_r, in4, dst0_l, in5, dst1_r, in6, dst1_l, in7, dst0_r, dst0_l,
         dst1_r, dst1_l);
    CLIP_SH4_0_255(dst0_r, dst0_l, dst1_r, dst1_l);
    PCKEV_B2_SH(dst0_l, dst0_r, dst1_l, dst1_r, dst0_r, dst1_r);
    ST8x4_UB(dst0_r, dst1_r, dst, dst_stride);
}

static void hevc_addblk_zerocol_16x16_msa(WORD16 *coeffs, UWORD8 *dst,
                                          WORD32 dst_stride, WORD32 zero_cols)
{
    UWORD8 loop_cnt;
    WORD32 zero_cols_temp;
    v16u8 dst0, dst1, dst2, dst3;
    v8i16 dst0_r, dst0_l, dst1_r, dst1_l, dst2_r, dst2_l, dst3_r, dst3_l;
    v8i16 in0, in1, in2, in3, in4, in5, in6, in7;

    for(loop_cnt = 4; loop_cnt--;)
    {
        LD_SH4(coeffs, 16, in0, in2, in4, in6);
        LD_SH4((coeffs + 8), 16, in1, in3, in5, in7);
        coeffs += 64;

        zero_cols_temp = zero_cols;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 0, 0);
            in2 = __msa_insert_h(in2, 0, 0);
            in4 = __msa_insert_h(in4, 0, 0);
            in6 = __msa_insert_h(in6, 0, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 1, 0);
            in2 = __msa_insert_h(in2, 1, 0);
            in4 = __msa_insert_h(in4, 1, 0);
            in6 = __msa_insert_h(in6, 1, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 2, 0);
            in2 = __msa_insert_h(in2, 2, 0);
            in4 = __msa_insert_h(in4, 2, 0);
            in6 = __msa_insert_h(in6, 2, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 3, 0);
            in2 = __msa_insert_h(in2, 3, 0);
            in4 = __msa_insert_h(in4, 3, 0);
            in6 = __msa_insert_h(in6, 3, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 4, 0);
            in2 = __msa_insert_h(in2, 4, 0);
            in4 = __msa_insert_h(in4, 4, 0);
            in6 = __msa_insert_h(in6, 4, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 5, 0);
            in2 = __msa_insert_h(in2, 5, 0);
            in4 = __msa_insert_h(in4, 5, 0);
            in6 = __msa_insert_h(in6, 5, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 6, 0);
            in2 = __msa_insert_h(in2, 6, 0);
            in4 = __msa_insert_h(in4, 6, 0);
            in6 = __msa_insert_h(in6, 6, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 7, 0);
            in2 = __msa_insert_h(in2, 7, 0);
            in4 = __msa_insert_h(in4, 7, 0);
            in6 = __msa_insert_h(in6, 7, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 0, 0);
            in3 = __msa_insert_h(in3, 0, 0);
            in5 = __msa_insert_h(in5, 0, 0);
            in7 = __msa_insert_h(in7, 0, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 1, 0);
            in3 = __msa_insert_h(in3, 1, 0);
            in5 = __msa_insert_h(in5, 1, 0);
            in7 = __msa_insert_h(in7, 1, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 2, 0);
            in3 = __msa_insert_h(in3, 2, 0);
            in5 = __msa_insert_h(in5, 2, 0);
            in7 = __msa_insert_h(in7, 2, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 3, 0);
            in3 = __msa_insert_h(in3, 3, 0);
            in5 = __msa_insert_h(in5, 3, 0);
            in7 = __msa_insert_h(in7, 3, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 4, 0);
            in3 = __msa_insert_h(in3, 4, 0);
            in5 = __msa_insert_h(in5, 4, 0);
            in7 = __msa_insert_h(in7, 4, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 5, 0);
            in3 = __msa_insert_h(in3, 5, 0);
            in5 = __msa_insert_h(in5, 5, 0);
            in7 = __msa_insert_h(in7, 5, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 6, 0);
            in3 = __msa_insert_h(in3, 6, 0);
            in5 = __msa_insert_h(in5, 6, 0);
            in7 = __msa_insert_h(in7, 6, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 7, 0);
            in3 = __msa_insert_h(in3, 7, 0);
            in5 = __msa_insert_h(in5, 7, 0);
            in7 = __msa_insert_h(in7, 7, 0);
        }

        LD_UB4(dst, dst_stride, dst0, dst1, dst2, dst3);
        UNPCK_UB_SH(dst0, dst0_r, dst0_l);
        UNPCK_UB_SH(dst1, dst1_r, dst1_l);
        UNPCK_UB_SH(dst2, dst2_r, dst2_l);
        UNPCK_UB_SH(dst3, dst3_r, dst3_l);
        ADD4(dst0_r, in0, dst0_l, in1, dst1_r, in2, dst1_l, in3, dst0_r, dst0_l,
             dst1_r, dst1_l);
        ADD4(dst2_r, in4, dst2_l, in5, dst3_r, in6, dst3_l, in7, dst2_r, dst2_l,
             dst3_r, dst3_l);
        CLIP_SH4_0_255(dst0_r, dst0_l, dst1_r, dst1_l);
        CLIP_SH4_0_255(dst2_r, dst2_l, dst3_r, dst3_l);
        PCKEV_B4_UB(dst0_l, dst0_r, dst1_l, dst1_r, dst2_l, dst2_r, dst3_l,
                    dst3_r, dst0, dst1, dst2, dst3);
        ST_UB4(dst0, dst1, dst2, dst3, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_addblk_zerocol_32x32_msa(WORD16 *coeffs, UWORD8 *dst,
                                          WORD32 dst_stride, WORD32 zero_cols)
{
    UWORD8 loop_cnt;
    WORD32 zero_cols_temp;
    v16u8 dst0, dst1, dst2, dst3;
    v8i16 dst0_r, dst0_l, dst1_r, dst1_l, dst2_r, dst2_l, dst3_r, dst3_l;
    v8i16 in0, in1, in2, in3, in4, in5, in6, in7;

    for(loop_cnt = 8; loop_cnt--;)
    {
        LD_SH4(coeffs, 32, in0, in2, in4, in6);
        LD_SH4((coeffs + 8), 32, in1, in3, in5, in7);

        zero_cols_temp = zero_cols;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 0, 0);
            in2 = __msa_insert_h(in2, 0, 0);
            in4 = __msa_insert_h(in4, 0, 0);
            in6 = __msa_insert_h(in6, 0, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 1, 0);
            in2 = __msa_insert_h(in2, 1, 0);
            in4 = __msa_insert_h(in4, 1, 0);
            in6 = __msa_insert_h(in6, 1, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 2, 0);
            in2 = __msa_insert_h(in2, 2, 0);
            in4 = __msa_insert_h(in4, 2, 0);
            in6 = __msa_insert_h(in6, 2, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 3, 0);
            in2 = __msa_insert_h(in2, 3, 0);
            in4 = __msa_insert_h(in4, 3, 0);
            in6 = __msa_insert_h(in6, 3, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 4, 0);
            in2 = __msa_insert_h(in2, 4, 0);
            in4 = __msa_insert_h(in4, 4, 0);
            in6 = __msa_insert_h(in6, 4, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 5, 0);
            in2 = __msa_insert_h(in2, 5, 0);
            in4 = __msa_insert_h(in4, 5, 0);
            in6 = __msa_insert_h(in6, 5, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 6, 0);
            in2 = __msa_insert_h(in2, 6, 0);
            in4 = __msa_insert_h(in4, 6, 0);
            in6 = __msa_insert_h(in6, 6, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 7, 0);
            in2 = __msa_insert_h(in2, 7, 0);
            in4 = __msa_insert_h(in4, 7, 0);
            in6 = __msa_insert_h(in6, 7, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 0, 0);
            in3 = __msa_insert_h(in3, 0, 0);
            in5 = __msa_insert_h(in5, 0, 0);
            in7 = __msa_insert_h(in7, 0, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 1, 0);
            in3 = __msa_insert_h(in3, 1, 0);
            in5 = __msa_insert_h(in5, 1, 0);
            in7 = __msa_insert_h(in7, 1, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 2, 0);
            in3 = __msa_insert_h(in3, 2, 0);
            in5 = __msa_insert_h(in5, 2, 0);
            in7 = __msa_insert_h(in7, 2, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 3, 0);
            in3 = __msa_insert_h(in3, 3, 0);
            in5 = __msa_insert_h(in5, 3, 0);
            in7 = __msa_insert_h(in7, 3, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 4, 0);
            in3 = __msa_insert_h(in3, 4, 0);
            in5 = __msa_insert_h(in5, 4, 0);
            in7 = __msa_insert_h(in7, 4, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 5, 0);
            in3 = __msa_insert_h(in3, 5, 0);
            in5 = __msa_insert_h(in5, 5, 0);
            in7 = __msa_insert_h(in7, 5, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 6, 0);
            in3 = __msa_insert_h(in3, 6, 0);
            in5 = __msa_insert_h(in5, 6, 0);
            in7 = __msa_insert_h(in7, 6, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 7, 0);
            in3 = __msa_insert_h(in3, 7, 0);
            in5 = __msa_insert_h(in5, 7, 0);
            in7 = __msa_insert_h(in7, 7, 0);
        }
        zero_cols_temp >>= 1;

        LD_UB4(dst, dst_stride, dst0, dst1, dst2, dst3);
        UNPCK_UB_SH(dst0, dst0_r, dst0_l);
        UNPCK_UB_SH(dst1, dst1_r, dst1_l);
        UNPCK_UB_SH(dst2, dst2_r, dst2_l);
        UNPCK_UB_SH(dst3, dst3_r, dst3_l);
        ADD4(dst0_r, in0, dst0_l, in1, dst1_r, in2, dst1_l, in3, dst0_r, dst0_l,
             dst1_r, dst1_l);
        ADD4(dst2_r, in4, dst2_l, in5, dst3_r, in6, dst3_l, in7, dst2_r, dst2_l,
             dst3_r, dst3_l);
        CLIP_SH4_0_255(dst0_r, dst0_l, dst1_r, dst1_l);
        CLIP_SH4_0_255(dst2_r, dst2_l, dst3_r, dst3_l);
        PCKEV_B4_UB(dst0_l, dst0_r, dst1_l, dst1_r, dst2_l, dst2_r, dst3_l,
                    dst3_r, dst0, dst1, dst2, dst3);
        ST_UB4(dst0, dst1, dst2, dst3, dst, dst_stride);

        LD_SH4((coeffs + 16), 32, in0, in2, in4, in6);
        LD_SH4((coeffs + 24), 32, in1, in3, in5, in7);

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 0, 0);
            in2 = __msa_insert_h(in2, 0, 0);
            in4 = __msa_insert_h(in4, 0, 0);
            in6 = __msa_insert_h(in6, 0, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 1, 0);
            in2 = __msa_insert_h(in2, 1, 0);
            in4 = __msa_insert_h(in4, 1, 0);
            in6 = __msa_insert_h(in6, 1, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 2, 0);
            in2 = __msa_insert_h(in2, 2, 0);
            in4 = __msa_insert_h(in4, 2, 0);
            in6 = __msa_insert_h(in6, 2, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 3, 0);
            in2 = __msa_insert_h(in2, 3, 0);
            in4 = __msa_insert_h(in4, 3, 0);
            in6 = __msa_insert_h(in6, 3, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 4, 0);
            in2 = __msa_insert_h(in2, 4, 0);
            in4 = __msa_insert_h(in4, 4, 0);
            in6 = __msa_insert_h(in6, 4, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 5, 0);
            in2 = __msa_insert_h(in2, 5, 0);
            in4 = __msa_insert_h(in4, 5, 0);
            in6 = __msa_insert_h(in6, 5, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 6, 0);
            in2 = __msa_insert_h(in2, 6, 0);
            in4 = __msa_insert_h(in4, 6, 0);
            in6 = __msa_insert_h(in6, 6, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 7, 0);
            in2 = __msa_insert_h(in2, 7, 0);
            in4 = __msa_insert_h(in4, 7, 0);
            in6 = __msa_insert_h(in6, 7, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 0, 0);
            in3 = __msa_insert_h(in3, 0, 0);
            in5 = __msa_insert_h(in5, 0, 0);
            in7 = __msa_insert_h(in7, 0, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 1, 0);
            in3 = __msa_insert_h(in3, 1, 0);
            in5 = __msa_insert_h(in5, 1, 0);
            in7 = __msa_insert_h(in7, 1, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 2, 0);
            in3 = __msa_insert_h(in3, 2, 0);
            in5 = __msa_insert_h(in5, 2, 0);
            in7 = __msa_insert_h(in7, 2, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 3, 0);
            in3 = __msa_insert_h(in3, 3, 0);
            in5 = __msa_insert_h(in5, 3, 0);
            in7 = __msa_insert_h(in7, 3, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 4, 0);
            in3 = __msa_insert_h(in3, 4, 0);
            in5 = __msa_insert_h(in5, 4, 0);
            in7 = __msa_insert_h(in7, 4, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 5, 0);
            in3 = __msa_insert_h(in3, 5, 0);
            in5 = __msa_insert_h(in5, 5, 0);
            in7 = __msa_insert_h(in7, 5, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 6, 0);
            in3 = __msa_insert_h(in3, 6, 0);
            in5 = __msa_insert_h(in5, 6, 0);
            in7 = __msa_insert_h(in7, 6, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 7, 0);
            in3 = __msa_insert_h(in3, 7, 0);
            in5 = __msa_insert_h(in5, 7, 0);
            in7 = __msa_insert_h(in7, 7, 0);
        }
        zero_cols_temp >>= 1;

        coeffs += 128;

        LD_UB4((dst + 16), dst_stride, dst0, dst1, dst2, dst3);
        UNPCK_UB_SH(dst0, dst0_r, dst0_l);
        UNPCK_UB_SH(dst1, dst1_r, dst1_l);
        UNPCK_UB_SH(dst2, dst2_r, dst2_l);
        UNPCK_UB_SH(dst3, dst3_r, dst3_l);
        ADD4(dst0_r, in0, dst0_l, in1, dst1_r, in2, dst1_l, in3, dst0_r, dst0_l,
             dst1_r, dst1_l);
        ADD4(dst2_r, in4, dst2_l, in5, dst3_r, in6, dst3_l, in7, dst2_r, dst2_l,
             dst3_r, dst3_l);
        CLIP_SH4_0_255(dst0_r, dst0_l, dst1_r, dst1_l);
        CLIP_SH4_0_255(dst2_r, dst2_l, dst3_r, dst3_l);
        PCKEV_B4_UB(dst0_l, dst0_r, dst1_l, dst1_r, dst2_l, dst2_r, dst3_l,
                    dst3_r, dst0, dst1, dst2, dst3);
        ST_UB4(dst0, dst1, dst2, dst3, (dst + 16), dst_stride);
        dst += (4 * dst_stride);
    }
}

static void hevc_chroma_addblk_zerocol_4x4_msa(WORD16 *coeffs, UWORD8 *dst,
                                               WORD32 dst_stride,
                                               WORD32 zero_cols)
{
    ULWORD64 pred0, pred1, pred2, pred3;
    v16i8 vec0 = { 0 };
    v16i8 vec1 = { 0 };
    v16i8 mask = { 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0,
                   0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0 };
    v16i8 mask1 = { 0, 17, 2, 19, 4, 21, 6, 23, 8, 25, 10, 27, 12, 29, 14, 31 };
    v8i16 dst0, dst1, in0, in1;

    LD_SH2(coeffs, 8, in0, in1);

    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 0, 0);
        in0 = __msa_insert_h(in0, 4, 0);
        in1 = __msa_insert_h(in1, 0, 0);
        in1 = __msa_insert_h(in1, 4, 0);
    }

    zero_cols >>= 1;
    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 1, 0);
        in0 = __msa_insert_h(in0, 5, 0);
        in1 = __msa_insert_h(in1, 1, 0);
        in1 = __msa_insert_h(in1, 5, 0);
    }

    zero_cols >>= 1;
    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 2, 0);
        in0 = __msa_insert_h(in0, 6, 0);
        in1 = __msa_insert_h(in1, 2, 0);
        in1 = __msa_insert_h(in1, 6, 0);
    }

    zero_cols >>= 1;
    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 3, 0);
        in0 = __msa_insert_h(in0, 7, 0);
        in1 = __msa_insert_h(in1, 3, 0);
        in1 = __msa_insert_h(in1, 7, 0);
    }

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

static void hevc_chroma_addblk_zerocol_8x8_msa(WORD16 *coeffs, UWORD8 *dst,
                                               WORD32 dst_stride,
                                               WORD32 zero_cols)
{
    v16i8 pred0, pred1, pred2, pred3, pred4, pred5, pred6, pred7;
    v8i16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v8i16 in0, in1, in2, in3, in4, in5, in6, in7;
    v16i8 mask = { 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0,
                   0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0 };
    v16i8 mask1 = { 0, 17, 2, 19, 4, 21, 6, 23, 8, 25, 10, 27, 12, 29, 14, 31 };

    LD_SH8(coeffs, 8, in0, in1, in2, in3, in4, in5, in6, in7);

    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 0, 0);
        in1 = __msa_insert_h(in1, 0, 0);
        in2 = __msa_insert_h(in2, 0, 0);
        in3 = __msa_insert_h(in3, 0, 0);
        in4 = __msa_insert_h(in4, 0, 0);
        in5 = __msa_insert_h(in5, 0, 0);
        in6 = __msa_insert_h(in6, 0, 0);
        in7 = __msa_insert_h(in7, 0, 0);
    }

    zero_cols >>= 1;
    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 1, 0);
        in1 = __msa_insert_h(in1, 1, 0);
        in2 = __msa_insert_h(in2, 1, 0);
        in3 = __msa_insert_h(in3, 1, 0);
        in4 = __msa_insert_h(in4, 1, 0);
        in5 = __msa_insert_h(in5, 1, 0);
        in6 = __msa_insert_h(in6, 1, 0);
        in7 = __msa_insert_h(in7, 1, 0);
    }

    zero_cols >>= 1;
    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 2, 0);
        in1 = __msa_insert_h(in1, 2, 0);
        in2 = __msa_insert_h(in2, 2, 0);
        in3 = __msa_insert_h(in3, 2, 0);
        in4 = __msa_insert_h(in4, 2, 0);
        in5 = __msa_insert_h(in5, 2, 0);
        in6 = __msa_insert_h(in6, 2, 0);
        in7 = __msa_insert_h(in7, 2, 0);
    }

    zero_cols >>= 1;
    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 3, 0);
        in1 = __msa_insert_h(in1, 3, 0);
        in2 = __msa_insert_h(in2, 3, 0);
        in3 = __msa_insert_h(in3, 3, 0);
        in4 = __msa_insert_h(in4, 3, 0);
        in5 = __msa_insert_h(in5, 3, 0);
        in6 = __msa_insert_h(in6, 3, 0);
        in7 = __msa_insert_h(in7, 3, 0);
    }

    zero_cols >>= 1;
    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 4, 0);
        in1 = __msa_insert_h(in1, 4, 0);
        in2 = __msa_insert_h(in2, 4, 0);
        in3 = __msa_insert_h(in3, 4, 0);
        in4 = __msa_insert_h(in4, 4, 0);
        in5 = __msa_insert_h(in5, 4, 0);
        in6 = __msa_insert_h(in6, 4, 0);
        in7 = __msa_insert_h(in7, 4, 0);
    }

    zero_cols >>= 1;
    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 5, 0);
        in1 = __msa_insert_h(in1, 5, 0);
        in2 = __msa_insert_h(in2, 5, 0);
        in3 = __msa_insert_h(in3, 5, 0);
        in4 = __msa_insert_h(in4, 5, 0);
        in5 = __msa_insert_h(in5, 5, 0);
        in6 = __msa_insert_h(in6, 5, 0);
        in7 = __msa_insert_h(in7, 5, 0);
    }

    zero_cols >>= 1;
    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 6, 0);
        in1 = __msa_insert_h(in1, 6, 0);
        in2 = __msa_insert_h(in2, 6, 0);
        in3 = __msa_insert_h(in3, 6, 0);
        in4 = __msa_insert_h(in4, 6, 0);
        in5 = __msa_insert_h(in5, 6, 0);
        in6 = __msa_insert_h(in6, 6, 0);
        in7 = __msa_insert_h(in7, 6, 0);
    }

    zero_cols >>= 1;
    if(1 == (zero_cols & 1))
    {
        in0 = __msa_insert_h(in0, 7, 0);
        in1 = __msa_insert_h(in1, 7, 0);
        in2 = __msa_insert_h(in2, 7, 0);
        in3 = __msa_insert_h(in3, 7, 0);
        in4 = __msa_insert_h(in4, 7, 0);
        in5 = __msa_insert_h(in5, 7, 0);
        in6 = __msa_insert_h(in6, 7, 0);
        in7 = __msa_insert_h(in7, 7, 0);
    }

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

static void hevc_chroma_addblk_zerocol_16x16_msa(WORD16 *coeffs, UWORD8 *dst,
                                                 WORD32 dst_stride,
                                                 WORD32 zero_cols)
{
    UWORD32 loop_cnt;
    WORD32 zero_cols_temp = zero_cols;
    v16i8 pred0, pred1, pred2, pred3, pred4, pred5, pred6, pred7;
    v8i16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v8i16 in0, in1, in2, in3, in4, in5, in6, in7;
    v16i8 mask = { 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0,
                   0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0 };
    v16i8 mask1 = { 0, 17, 2, 19, 4, 21, 6, 23, 8, 25, 10, 27, 12, 29, 14, 31 };

    for(loop_cnt = 4; loop_cnt--;)
    {
        LD_SH4(coeffs, 16, in0, in2, in4, in6);
        LD_SH4((coeffs + 8), 16, in1, in3, in5, in7);
        coeffs += 64;

        zero_cols_temp = zero_cols;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 0, 0);
            in2 = __msa_insert_h(in2, 0, 0);
            in4 = __msa_insert_h(in4, 0, 0);
            in6 = __msa_insert_h(in6, 0, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 1, 0);
            in2 = __msa_insert_h(in2, 1, 0);
            in4 = __msa_insert_h(in4, 1, 0);
            in6 = __msa_insert_h(in6, 1, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 2, 0);
            in2 = __msa_insert_h(in2, 2, 0);
            in4 = __msa_insert_h(in4, 2, 0);
            in6 = __msa_insert_h(in6, 2, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 3, 0);
            in2 = __msa_insert_h(in2, 3, 0);
            in4 = __msa_insert_h(in4, 3, 0);
            in6 = __msa_insert_h(in6, 3, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 4, 0);
            in2 = __msa_insert_h(in2, 4, 0);
            in4 = __msa_insert_h(in4, 4, 0);
            in6 = __msa_insert_h(in6, 4, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 5, 0);
            in2 = __msa_insert_h(in2, 5, 0);
            in4 = __msa_insert_h(in4, 5, 0);
            in6 = __msa_insert_h(in6, 5, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 6, 0);
            in2 = __msa_insert_h(in2, 6, 0);
            in4 = __msa_insert_h(in4, 6, 0);
            in6 = __msa_insert_h(in6, 6, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in0 = __msa_insert_h(in0, 7, 0);
            in2 = __msa_insert_h(in2, 7, 0);
            in4 = __msa_insert_h(in4, 7, 0);
            in6 = __msa_insert_h(in6, 7, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 0, 0);
            in3 = __msa_insert_h(in3, 0, 0);
            in5 = __msa_insert_h(in5, 0, 0);
            in7 = __msa_insert_h(in7, 0, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 1, 0);
            in3 = __msa_insert_h(in3, 1, 0);
            in5 = __msa_insert_h(in5, 1, 0);
            in7 = __msa_insert_h(in7, 1, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 2, 0);
            in3 = __msa_insert_h(in3, 2, 0);
            in5 = __msa_insert_h(in5, 2, 0);
            in7 = __msa_insert_h(in7, 2, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 3, 0);
            in3 = __msa_insert_h(in3, 3, 0);
            in5 = __msa_insert_h(in5, 3, 0);
            in7 = __msa_insert_h(in7, 3, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 4, 0);
            in3 = __msa_insert_h(in3, 4, 0);
            in5 = __msa_insert_h(in5, 4, 0);
            in7 = __msa_insert_h(in7, 4, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 5, 0);
            in3 = __msa_insert_h(in3, 5, 0);
            in5 = __msa_insert_h(in5, 5, 0);
            in7 = __msa_insert_h(in7, 5, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 6, 0);
            in3 = __msa_insert_h(in3, 6, 0);
            in5 = __msa_insert_h(in5, 6, 0);
            in7 = __msa_insert_h(in7, 6, 0);
        }
        zero_cols_temp >>= 1;

        if(1 == (zero_cols_temp & 1))
        {
            in1 = __msa_insert_h(in1, 7, 0);
            in3 = __msa_insert_h(in3, 7, 0);
            in5 = __msa_insert_h(in5, 7, 0);
            in7 = __msa_insert_h(in7, 7, 0);
        }

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

        ADD4(dst0, in0, dst1, in1, dst2, in2, dst3, in3, dst0, dst1, dst2,
             dst3);
        ADD4(dst4, in4, dst5, in5, dst6, in6, dst7, in7, dst4, dst5, dst6,
             dst7);
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

void ihevc_recon_4x4_msa(WORD16 *pi2_src, UWORD8 *pu1_pred,
                         UWORD8 *pu1_dst, WORD32 src_strd,
                         WORD32 pred_strd, WORD32 dst_strd,
                         WORD32 zero_cols)
{
    UNUSED(pu1_pred);
    UNUSED(src_strd);
    UNUSED(pred_strd);

    hevc_addblk_zerocol_4x4_msa(pi2_src, pu1_dst, dst_strd, zero_cols);
}

void ihevc_recon_8x8_msa(WORD16 *pi2_src, UWORD8 *pu1_pred,
                         UWORD8 *pu1_dst, WORD32 src_strd,
                         WORD32 pred_strd, WORD32 dst_strd,
                         WORD32 zero_cols)
{
    UNUSED(pu1_pred);
    UNUSED(src_strd);
    UNUSED(pred_strd);

    hevc_addblk_zerocol_8x8_msa(pi2_src, pu1_dst, dst_strd, zero_cols);
}

void ihevc_recon_16x16_msa(WORD16 *pi2_src, UWORD8 *pu1_pred,
                           UWORD8 *pu1_dst, WORD32 src_strd,
                           WORD32 pred_strd, WORD32 dst_strd,
                           WORD32 zero_cols)
{
    UNUSED(pu1_pred);
    UNUSED(src_strd);
    UNUSED(pred_strd);

    hevc_addblk_zerocol_16x16_msa(pi2_src, pu1_dst, dst_strd, zero_cols);
}

void ihevc_recon_32x32_msa(WORD16 *pi2_src, UWORD8 *pu1_pred,
                           UWORD8 *pu1_dst, WORD32 src_strd,
                           WORD32 pred_strd, WORD32 dst_strd,
                           WORD32 zero_cols)
{
    UNUSED(pu1_pred);
    UNUSED(src_strd);
    UNUSED(pred_strd);

    hevc_addblk_zerocol_32x32_msa(pi2_src, pu1_dst, dst_strd, zero_cols);
}

void ihevc_chroma_recon_4x4_msa(WORD16 *pi2_src, UWORD8 *pu1_pred,
                                UWORD8 *pu1_dst, WORD32 src_strd,
                                WORD32 pred_strd, WORD32 dst_strd,
                                WORD32 zero_cols)
{
    UNUSED(pu1_pred);
    UNUSED(src_strd);
    UNUSED(pred_strd);

    hevc_chroma_addblk_zerocol_4x4_msa(pi2_src, pu1_dst, dst_strd, zero_cols);
}

void ihevc_chroma_recon_8x8_msa(WORD16 *pi2_src, UWORD8 *pu1_pred,
                                UWORD8 *pu1_dst, WORD32 src_strd,
                                WORD32 pred_strd, WORD32 dst_strd,
                                WORD32 zero_cols)
{
    UNUSED(pu1_pred);
    UNUSED(src_strd);
    UNUSED(pred_strd);

    hevc_chroma_addblk_zerocol_8x8_msa(pi2_src, pu1_dst, dst_strd, zero_cols);
}

void ihevc_chroma_recon_16x16_msa(WORD16 *pi2_src, UWORD8 *pu1_pred,
                                  UWORD8 *pu1_dst, WORD32 src_strd,
                                  WORD32 pred_strd, WORD32 dst_strd,
                                  WORD32 zero_cols)
{
    UNUSED(pu1_pred);
    UNUSED(src_strd);
    UNUSED(pred_strd);

    hevc_chroma_addblk_zerocol_16x16_msa(pi2_src, pu1_dst, dst_strd, zero_cols);
}
