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
#include "ihevcd_cxa.h"
#include "ihevc_defs.h"
#include "ihevc_structs.h"
#include "ihevc_macros.h"
#include "ihevc_platform_macros.h"
#include "ihevc_cabac_tables.h"
#include "ihevcd_defs.h"
#include "ihevcd_function_selector.h"
#include "ihevcd_structs.h"
#include "ihevcd_error.h"
#include "ihevcd_fmt_conv.h"
#include "ihevc_macros_msa.h"

static void hevcd_fmt_conv_420sp_to_rgb565_w16mult_msa(UWORD8 *src_y,
                                                       UWORD8 *src_uv,
                                                       UWORD16 *dst_rgb,
                                                       WORD32 cnt,
                                                       WORD32 src_y_stride,
                                                       WORD32 dst_stride,
                                                       WORD32 is_u_first)
{
    WORD16 j, coeff1, coeff2, coeff3, coeff4;
    UWORD8 *src_y_next;
    UWORD16 *dst_rgb_next;
    v4i32 sub = __msa_ldi_w(128);
    v4i32 and = __msa_fill_w(0xffff);
    v4i32 srcu = { 0 }, srcv = { 0 };
    v16i8 src, src_next, zero = { 0 };
    v8i16 srcy, shift1, shift2, outb, outg, outr, out, out_bb, out_gg, out_rr;
    v4i32 vcoeff1, vcoeff2, vcoeff3, vcoeff4, out_b, out_g, out_r;

    if(is_u_first)
    {
        coeff1 = COEFF1;
        coeff2 = COEFF2;
        coeff3 = COEFF3;
        coeff4 = COEFF4;

        shift1 = __msa_fill_h(11);
        shift2 = __msa_fill_h(0);
    }
    else
    {
        coeff1 = COEFF4;
        coeff2 = COEFF3;
        coeff3 = COEFF2;
        coeff4 = COEFF1;

        shift1 = __msa_fill_h(0);
        shift2 = __msa_fill_h(11);
    }

    src_y_next = src_y + src_y_stride;
    dst_rgb_next = dst_rgb + dst_stride;

    vcoeff1 = __msa_fill_w(coeff1);
    vcoeff2 = __msa_fill_w(coeff2);
    vcoeff3 = __msa_fill_w(coeff3);
    vcoeff4 = __msa_fill_w(coeff4);

    for(j = cnt; j--;)
    {
        INSERT_W4_SW(src_uv[0], src_uv[2], src_uv[4], src_uv[6], srcu);
        INSERT_W4_SW(src_uv[1], src_uv[3], src_uv[5], src_uv[7], srcv);

        srcu -= sub;
        srcv -= sub;

        out_b = SRAI_W((vcoeff4 * srcu), 13);
        out_g = SRAI_W((vcoeff2 * srcu + vcoeff3 * srcv), 13);
        out_r = SRAI_W((vcoeff1 * srcv), 13);

        out_b &= and;
        out_g &= and;
        out_r &= and;

        out_bb = (v8i16) (out_b | SLLI_W(out_b, 16));
        out_gg = (v8i16) (out_g | SLLI_W(out_g, 16));
        out_rr = (v8i16) (out_r | SLLI_W(out_r, 16));

        src = LD_SB(src_y);
        srcy = (v8i16) __msa_ilvr_b(zero, src);

        outb = out_bb + srcy;
        outg = out_gg + srcy;
        outr = out_rr + srcy;

        outb = CLIP_SH_0_255(outb);
        CLIP_SH2_0_255(outg, outr);

        outb = SRAI_H(outb, 3);
        outg = SRAI_H(outg, 2);
        outr = SRAI_H(outr, 3);

        out = (outr << shift1) | SLLI_H(outg, 5) | (outb << shift2);
        ST_SH(out, dst_rgb);

        src_next = LD_SB(src_y_next);
        srcy = (v8i16) __msa_ilvr_b(zero, src_next);

        outb = out_bb + srcy;
        outg = out_gg + srcy;
        outr = out_rr + srcy;

        outb = CLIP_SH_0_255(outb);
        CLIP_SH2_0_255(outg, outr);

        outb = SRAI_H(outb, 3);
        outg = SRAI_H(outg, 2);
        outr = SRAI_H(outr, 3);

        out = (outr << shift1) | SLLI_H(outg, 5) | (outb << shift2);
        ST_SH(out, dst_rgb_next);

        INSERT_W4_SW(src_uv[8], src_uv[10], src_uv[12], src_uv[14], srcu);
        INSERT_W4_SW(src_uv[9], src_uv[11], src_uv[13], src_uv[15], srcv);

        srcu -= sub;
        srcv -= sub;

        out_b = SRAI_W((vcoeff4 * srcu), 13);
        out_g = SRAI_W((vcoeff2 * srcu + vcoeff3 * srcv), 13);
        out_r = SRAI_W((vcoeff1 * srcv), 13);

        out_b &= and;
        out_g &= and;
        out_r &= and;

        out_bb = (v8i16) (out_b | SLLI_W(out_b, 16));
        out_gg = (v8i16) (out_g | SLLI_W(out_g, 16));
        out_rr = (v8i16) (out_r | SLLI_W(out_r, 16));

        srcy = (v8i16) __msa_ilvl_b(zero, src);

        outb = out_bb + srcy;
        outg = out_gg + srcy;
        outr = out_rr + srcy;

        outb = CLIP_SH_0_255(outb);
        CLIP_SH2_0_255(outg, outr);

        outb = SRAI_H(outb, 3);
        outg = SRAI_H(outg, 2);
        outr = SRAI_H(outr, 3);

        out = (outr << shift1) | SLLI_H(outg, 5) | (outb << shift2);
        ST_SH(out, dst_rgb + 8);

        srcy = (v8i16) __msa_ilvl_b(zero, src_next);

        outb = out_bb + srcy;
        outg = out_gg + srcy;
        outr = out_rr + srcy;

        outb = CLIP_SH_0_255(outb);
        CLIP_SH2_0_255(outg, outr);

        outb = SRAI_H(outb, 3);
        outg = SRAI_H(outg, 2);
        outr = SRAI_H(outr, 3);

        out = (outr << shift1) | SLLI_H(outg, 5) | (outb << shift2);
        ST_SH(out, dst_rgb_next + 8);

        src_uv += 16;
        src_y += 16;
        dst_rgb += 16;
        src_y_next += 16;
        dst_rgb_next += 16;
    }
}

static void hevcd_fmt_conv_420sp_to_rgb565_msa(UWORD8 *src_y, UWORD8 *src_uv,
                                               UWORD16 *dst_rgb, WORD32 width,
                                               WORD32 height,
                                               WORD32 src_y_stride,
                                               WORD32 src_uv_stride,
                                               WORD32 dst_stride,
                                               WORD32 is_u_first)
{
    UWORD8 *src_y_next, *u_src, *v_src;
    UWORD16 *dst_rgb_next;
    WORD16 r0, g0, b0, cnt0, cnt1, i, j;
    UWORD32 r1, g1, b1;

    if(is_u_first)
    {
        u_src = (UWORD8 *)src_uv;
        v_src = (UWORD8 *)src_uv + 1;
    }
    else
    {
        u_src = (UWORD8 *)src_uv + 1;
        v_src = (UWORD8 *)src_uv;
    }

    src_y_next = src_y + src_y_stride;
    dst_rgb_next = dst_rgb + dst_stride;

    cnt0 = width / 16;
    cnt1 = width % 16;

    for(i = (height >> 1); i--;)
    {
        if(cnt0)
        {
            hevcd_fmt_conv_420sp_to_rgb565_w16mult_msa(src_y, src_uv, dst_rgb,
                                                       cnt0, src_y_stride,
                                                       dst_stride, is_u_first);
        }

        src_uv += 16 * cnt0;
        u_src += 16 * cnt0;
        v_src += 16 * cnt0;
        src_y += 16 * cnt0;
        src_y_next += 16 * cnt0;
        dst_rgb += 16 * cnt0;
        dst_rgb_next += 16 * cnt0;
        src_uv += cnt1;

        if(cnt1)
        {
            for(j = (cnt1 >> 1); j--;)
            {
                b0 = ((*u_src - 128) * COEFF4 >> 13);
                g0 = ((*u_src - 128) * COEFF2 + (*v_src - 128) * COEFF3) >> 13;
                r0 = ((*v_src - 128) * COEFF1) >> 13;

                u_src += 2;
                v_src += 2;

                /* pixel 0 */
                /* B */
                b1 = CLIP_U8(*src_y + b0);
                b1 >>= 3;
                /* G */
                g1 = CLIP_U8(*src_y + g0);
                g1 >>= 2;
                /* R */
                r1 = CLIP_U8(*src_y + r0);
                r1 >>= 3;

                src_y++;
                *dst_rgb++ = ((r1 << 11) | (g1 << 5) | b1);

                /* pixel 1 */
                /* B */
                b1 = CLIP_U8(*src_y + b0);
                b1 >>= 3;
                /* G */
                g1 = CLIP_U8(*src_y + g0);
                g1 >>= 2;
                /* R */
                r1 = CLIP_U8(*src_y + r0);
                r1 >>= 3;

                src_y++;
                *dst_rgb++ = ((r1 << 11) | (g1 << 5) | b1);

                /* pixel 2 */
                /* B */
                b1 = CLIP_U8(*src_y_next + b0);
                b1 >>= 3;
                /* G */
                g1 = CLIP_U8(*src_y_next + g0);
                g1 >>= 2;
                /* R */
                r1 = CLIP_U8(*src_y_next + r0);
                r1 >>= 3;

                src_y_next++;
                *dst_rgb_next++ = ((r1 << 11) | (g1 << 5) | b1);

                /* pixel 3 */
                /* B */
                b1 = CLIP_U8(*src_y_next + b0);
                b1 >>= 3;
                /* G */
                g1 = CLIP_U8(*src_y_next + g0);
                g1 >>= 2;
                /* R */
                r1 = CLIP_U8(*src_y_next + r0);
                r1 >>= 3;

                src_y_next++;
                *dst_rgb_next++ = ((r1 << 11) | (g1 << 5) | b1);
            }
        }

        u_src += src_uv_stride - width;
        v_src += src_uv_stride - width;
        src_uv += src_uv_stride - width;
        src_y += (src_y_stride << 1) - width;
        src_y_next += (src_y_stride << 1) - width;

        dst_rgb = dst_rgb_next - width + dst_stride;
        dst_rgb_next += (dst_stride << 1) - width;
    }
}

static void memcpy_multiple_of_16byte_msa(UWORD8 *src, UWORD8 *dst, WORD32 cnt)
{
    WORD32 row;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;

    for(row = (cnt >> 3); row--;)
    {
        LD_UB8(src, 16, src0, src1, src2, src3, src4, src5, src6, src7);
        src += (8 * 16);

        ST_UB8(src0, src1, src2, src3, src4, src5, src6, src7, dst, 16);
        dst += (8 * 16);
    }

    switch(cnt % 8)
    {
        case 7:
            LD_UB4(src, 16, src0, src1, src2, src3);
            LD_UB3(src + 16 * 4, 16, src4, src5, src6);
            ST_UB4(src0, src1, src2, src3, dst, 16);
            ST_UB2(src4, src5, dst + 16 * 4, 16);
            ST_UB(src6, dst + 16 * 6);
            break;
        case 6:
            LD_UB4(src, 16, src0, src1, src2, src3);
            LD_UB2(src + 16 * 4, 16, src4, src5);
            ST_UB4(src0, src1, src2, src3, dst, 16);
            ST_UB2(src4, src5, dst + 16 * 4, 16);
            break;
        case 5:
            LD_UB4(src, 16, src0, src1, src2, src3);
            src4 = LD_UB(src + 16 * 4);
            ST_UB4(src0, src1, src2, src3, dst, 16);
            ST_UB(src4, dst + 16 * 4);
            break;
        case 4:
            LD_UB4(src, 16, src0, src1, src2, src3);
            ST_UB4(src0, src1, src2, src3, dst, 16);
            break;
        case 3:
            LD_UB3(src, 16, src0, src1, src2);
            ST_UB2(src0, src1, dst, 16);
            ST_UB(src2, dst + 16 * 2);
            break;
        case 2:
            LD_UB2(src, 16, src0, src1);
            ST_UB2(src0, src1, dst, 16);
            break;
        case 1:
            src0 = LD_UB(src);
            ST_UB(src0, dst);
    }
}

static void memcpy_ilv_multiple_of_16byte_msa(UWORD8 *src, UWORD8 *dst_u,
                                              UWORD8 *dst_v, WORD32 cnt)
{
    WORD32 row;
    v16u8 src0, src1, src2, src3;
    v16u8 out0_u, out1_u, out0_v, out1_v;

    for(row = (cnt / 4); row--;)
    {
        LD_UB4(src, 16, src0, src1, src2, src3);
        src += (4 * 16);

        out0_u = (v16u8) __msa_pckev_b((v16i8) src1, (v16i8) src0);
        out1_u = (v16u8) __msa_pckev_b((v16i8) src3, (v16i8) src2);
        out0_v = (v16u8) __msa_pckod_b((v16i8) src1, (v16i8) src0);
        out1_v = (v16u8) __msa_pckod_b((v16i8) src3, (v16i8) src2);

        ST_UB2(out0_u, out1_u, dst_u, 16);
        dst_u += (2 * 16);
        ST_UB2(out0_v, out1_v, dst_v, 16);
        dst_v += (2 * 16);
    }

    cnt = cnt % 4;

    switch(cnt)
    {
        case 3:
            LD_UB3(src, 16, src0, src1, src2);
            out0_u = (v16u8) __msa_pckev_b((v16i8) src1, (v16i8) src0);
            out1_u = (v16u8) __msa_pckev_b((v16i8) src2, (v16i8) src2);
            out0_v = (v16u8) __msa_pckod_b((v16i8) src1, (v16i8) src0);
            out1_v = (v16u8) __msa_pckod_b((v16i8) src2, (v16i8) src2);
            ST_UB(out0_u, dst_u);
            ST8x1_UB(out1_u, dst_u + 16);
            ST_UB(out0_v, dst_v);
            ST8x1_UB(out1_v, dst_v + 16);
            break;
        case 2:
            LD_UB2(src, 16, src0, src1);
            out0_u = (v16u8) __msa_pckev_b((v16i8) src1, (v16i8) src0);
            out0_v = (v16u8) __msa_pckod_b((v16i8) src1, (v16i8) src0);
            ST_UB(out0_u, dst_u);
            ST_UB(out0_v, dst_v);
            break;
        case 1:
            src0 = LD_UB(src);
            out0_u = (v16u8) __msa_pckev_b((v16i8) src0, (v16i8) src0);
            out0_v = (v16u8) __msa_pckod_b((v16i8) src0, (v16i8) src0);
            ST8x1_UB(out0_u, dst_u);
            ST8x1_UB(out0_v, dst_v);;
    }
}

void ihevcd_fmt_conv_420sp_to_rgb565_msa(UWORD8 *pu1_y_src, UWORD8 *pu1_uv_src,
                                         UWORD16 *pu2_rgb_dst, WORD32 wd,
                                         WORD32 ht, WORD32 src_y_strd,
                                         WORD32 src_uv_strd, WORD32 dst_strd,
                                         WORD32 is_u_first)
{
    hevcd_fmt_conv_420sp_to_rgb565_msa(pu1_y_src, pu1_uv_src, pu2_rgb_dst,
                                       wd, ht, src_y_strd, src_uv_strd,
                                       dst_strd, is_u_first);
}

void ihevcd_fmt_conv_420sp_to_420sp_msa(UWORD8 *pu1_y_src, UWORD8 *pu1_uv_src,
                                        UWORD8 *pu1_y_dst, UWORD8 *pu1_uv_dst,
                                        WORD32 wd, WORD32 ht, WORD32 src_y_strd,
                                        WORD32 src_uv_strd, WORD32 dst_y_strd,
                                        WORD32 dst_uv_strd)
{
    UWORD8 *pu1_src, *pu1_dst;
    WORD32 num_rows, num_cols, src_strd, dst_strd;
    WORD32 i, cnt;

    /* copy luma */
    pu1_src = (UWORD8 *)pu1_y_src;
    pu1_dst = (UWORD8 *)pu1_y_dst;

    num_rows = ht;
    num_cols = wd;

    src_strd = src_y_strd;
    dst_strd = dst_y_strd;

    for(i = 0; i < num_rows; i++)
    {
        cnt = num_cols / 16;

        if(cnt)
        {
            memcpy_multiple_of_16byte_msa(pu1_src, pu1_dst, cnt);
        }

        memcpy(pu1_dst + cnt * 16, pu1_src + cnt * 16, num_cols % 16);

        pu1_dst += dst_strd;
        pu1_src += src_strd;
    }

    /* copy U and V */
    pu1_src = (UWORD8 *)pu1_uv_src;
    pu1_dst = (UWORD8 *)pu1_uv_dst;

    num_rows = ht >> 1;
    num_cols = wd;

    src_strd = src_uv_strd;
    dst_strd = dst_uv_strd;

    for(i = 0; i < num_rows; i++)
    {
        cnt = num_cols / 16;

        if(cnt)
        {
            memcpy_multiple_of_16byte_msa(pu1_src, pu1_dst, cnt);
        }

        memcpy(pu1_dst + cnt * 16, pu1_src + cnt * 16, num_cols % 16);

        pu1_dst += dst_strd;
        pu1_src += src_strd;
    }

    return;
}

void ihevcd_fmt_conv_420sp_to_420p_msa(UWORD8 *pu1_y_src, UWORD8 *pu1_uv_src,
                                       UWORD8 *pu1_y_dst, UWORD8 *pu1_u_dst,
                                       UWORD8 *pu1_v_dst, WORD32 wd, WORD32 ht,
                                       WORD32 src_y_strd, WORD32 src_uv_strd,
                                       WORD32 dst_y_strd, WORD32 dst_uv_strd,
                                       WORD32 is_u_first,
                                       WORD32 disable_luma_copy)
{
    UWORD8 *pu1_src, *pu1_dst;
    UWORD8 *tmp;
    WORD32 num_rows, num_cols, src_strd, dst_strd, cnt;
    WORD32 i, j;

    if(0 == disable_luma_copy)
    {
        /* copy luma */
        pu1_src = (UWORD8 *)pu1_y_src;
        pu1_dst = (UWORD8 *)pu1_y_dst;

        num_rows = ht;
        num_cols = wd;

        src_strd = src_y_strd;
        dst_strd = dst_y_strd;

        for(i = 0; i < num_rows; i++)
        {
            cnt = num_cols / 16;

            if(cnt)
            {
                memcpy_multiple_of_16byte_msa(pu1_src, pu1_dst, cnt);
            }

            memcpy(pu1_dst + cnt * 16, pu1_src + cnt * 16, num_cols % 16);

            pu1_dst += dst_strd;
            pu1_src += src_strd;
        }
    }

    if(!is_u_first)
    {
        tmp = pu1_u_dst;
        pu1_u_dst = pu1_v_dst;
        pu1_v_dst = tmp;
    }

    num_rows = ht >> 1;
    num_cols = wd >> 1;

    src_strd = src_uv_strd;
    dst_strd = dst_uv_strd;

    for(i = 0; i < num_rows; i++)
    {
        cnt = num_cols / 8;

        if(cnt)
        {
            memcpy_ilv_multiple_of_16byte_msa(pu1_uv_src, pu1_u_dst, pu1_v_dst,
                                              cnt);
        }

        for(j = 0; j < num_cols % 8; j++)
        {
            pu1_u_dst[cnt * 8 + j] = pu1_uv_src[(cnt * 8 + j) * 2];
            pu1_v_dst[cnt * 8 + j] = pu1_uv_src[(cnt * 8 + j) * 2 + 1];
        }

        pu1_u_dst += dst_strd;
        pu1_v_dst += dst_strd;

        pu1_uv_src += src_strd;
    }

    return;
}
