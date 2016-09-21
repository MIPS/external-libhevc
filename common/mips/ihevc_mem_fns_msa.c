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
#include "ihevc_mem_fns.h"
#include "ihevc_macros_msa.h"

static void memset_16bit_8wmult_msa(UWORD16 *src, WORD32 stride, WORD32 height,
                                    UWORD16 value, WORD32 no_of_ele)
{
    WORD32 row, col;
    UWORD16 *src_tmp;
    v8i16 vec = __msa_fill_h(value);

    for(row = height; row--;)
    {
        src_tmp = src;
        for(col = 0; col < (no_of_ele >> 3); col++)
        {
            ST_SH(vec, src_tmp);
            src_tmp += 8;
        }

        src += stride;
    }
}

void ihevc_memset_16bit_msa(UWORD16 *pu2_dst, UWORD16 value, UWORD32 num_words)
{
    UWORD32 tmp;

    tmp = (num_words / 8);
    if(tmp)
    {
        memset_16bit_8wmult_msa(pu2_dst, 1, 1, value, tmp * 8);
        pu2_dst += (tmp * 8);
    }

    tmp = num_words % 8;
    if(tmp)
    {
        UWORD32 i;
        for(i = 0; i < tmp; i++)
        {
            *pu2_dst++ = value;
        }
    }
}

void ihevc_memset_16bit_mul_8_msa(UWORD16 *pu2_dst, UWORD16 value,
                                  UWORD32 num_words)
{
    memset_16bit_8wmult_msa(pu2_dst, 1, 1, value, num_words);
}
