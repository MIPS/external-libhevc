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
#include "ihevc_mem_fns.h"
#include "ihevc_macros_msa.h"

static void memset_8bit_8wmult_msa(UWORD8 *src, UWORD8 val, WORD32 no_of_ele)
{
    WORD32 col;
    uint64_t value_64bit = val * 0x0101010101010101;

    for(col = 0; col < no_of_ele; col = col + 8)
    {
        SD(value_64bit, src + col);
    }
}

static void memset_8bit_16wmult_msa(UWORD8 *src, UWORD8 val, WORD32 no_of_ele)
{
    WORD32 col;
    v16i8 val_vec = __msa_fill_b(val);

    for(col = 0; col < no_of_ele; col = col + 16)
    {
        ST_SB(val_vec, src + col);
    }
}

static void memset_8bit_24wmult_msa(UWORD8 *src, UWORD8 val, WORD32 no_of_ele)
{
    WORD32 col, col_2;
    uint64_t value_64bit = val * 0x0101010101010101;
    v16i8 val_vec = __msa_fill_b(val);

    col_2 = 16;
    for(col = 0; col < no_of_ele; col = col + 24)
    {
        ST_SB(val_vec, src + (col));
        SD(value_64bit, src + col_2);
        col_2 = col_2 + 24;
    }
}

static void memset_16bit_8wmult_msa(UWORD16 *src, UWORD16 value, WORD32 no_of_ele)
{
    WORD32 col;
    UWORD16 *src_tmp;
    v8i16 vec = __msa_fill_h(value);

    src_tmp = src;
    for(col = 0; col < (no_of_ele >> 3); col++)
    {
        ST_SH(vec, src_tmp);
        src_tmp += 8;
    }
}


void ihevc_pad_left_luma_msa(UWORD8 *pu1_src, WORD32 src_strd,
                             WORD32 ht, WORD32 pad_size)
{
    WORD32 row;

    for(row = 0; row < ht; row++)
    {
        if(24 == pad_size)
        {
            memset_8bit_24wmult_msa(pu1_src - pad_size, *pu1_src, pad_size);
        }
        else if(8 == pad_size)
        {
            memset_8bit_8wmult_msa(pu1_src - pad_size, *pu1_src, pad_size);
        }
        else if(0 == (pad_size % 16))
        {
            memset_8bit_16wmult_msa(pu1_src - pad_size, *pu1_src, pad_size);
        }

        pu1_src += src_strd;
    }

}

void ihevc_pad_left_chroma_msa(UWORD8 *pu1_src, WORD32 src_strd,
                               WORD32 ht, WORD32 pad_size)
{
    WORD32 row;
    UWORD16 *pu2_src = (UWORD16 *)pu1_src;

    src_strd >>= 1;
    pad_size >>= 1;

    for(row = 0; row < ht; row++)
    {
        memset_16bit_8wmult_msa(pu2_src - pad_size, *pu2_src, pad_size);
        pu2_src += src_strd;
    }
}

void ihevc_pad_right_luma_msa(UWORD8 *pu1_src, WORD32 src_strd,
                              WORD32 ht, WORD32 pad_size)
{
    WORD32 row;

    for(row = 0; row < ht; row++)
    {
        if(24 == pad_size)
        {
            memset_8bit_24wmult_msa(pu1_src, *(pu1_src - 1), pad_size);
        }
        else if(8 == pad_size)
        {
            memset_8bit_8wmult_msa(pu1_src, *(pu1_src - 1), pad_size);
        }
        else if(0 == (pad_size % 16))
        {
            memset_8bit_16wmult_msa(pu1_src, *(pu1_src - 1), pad_size);
        }

        pu1_src += src_strd;
    }
}

void ihevc_pad_right_chroma_msa(UWORD8 *pu1_src, WORD32 src_strd,
                                WORD32 ht, WORD32 pad_size)
{
    WORD32 row;
    UWORD16 *pu2_src = (UWORD16 *)pu1_src;

    src_strd >>= 1;
    pad_size >>= 1;

    for(row = 0; row < ht; row++)
    {
        memset_16bit_8wmult_msa(pu2_src, pu2_src[-1], pad_size);
        pu2_src += src_strd;
    }
}
