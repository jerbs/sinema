/**
 * Copyright (C) 2004 Billy Biggs <vektor@dumbterm.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "tomsmocomp.h"
#include "tomsmocomp/tomsmocompmacros.h"
#include "x86-64_macros.inc"

#define SearchEffortDefault 5
#define UseStrangeBobDefault false

class DScalerFilterTomsMoComp
{
public:
    DScalerFilterTomsMoComp() {}
    virtual ~DScalerFilterTomsMoComp() {}

#if defined __MMX__
#define IS_MMX
#define SSE_TYPE MMX
#define FUNCT_NAME /* DScalerFilterTomsMoComp:: */ filterDScaler_MMX
#include "tomsmocomp/TomsMoCompAll.inc"
#undef  IS_MMX
#undef  SSE_TYPE
#undef  FUNCT_NAME
#endif

#if defined __3DNOW__
#define IS_3DNOW
#define SSE_TYPE 3DNOW
#define FUNCT_NAME /* DScalerFilterTomsMoComp:: */ filterDScaler_3DNOW
#include "tomsmocomp/TomsMoCompAll.inc"
#undef  IS_3DNOW
#undef  SSE_TYPE
#undef  FUNCT_NAME
#endif

#if defined __SSE__
#define IS_SSE
#define SSE_TYPE SSE
#define FUNCT_NAME /* DScalerFilterTomsMoComp:: */ filterDScaler_SSE
#include "tomsmocomp/TomsMoCompAll.inc"
#undef  IS_SSE
#undef  SSE_TYPE
#undef  FUNCT_NAME
#endif

protected:
    int Fieldcopy(void *dest, const void *src, size_t count, 
                  int rows, int dst_pitch, int src_pitch)
    {
        unsigned char* pDest = (unsigned char*) dest;
        unsigned char* pSrc = (unsigned char*) src;
        int i;
        
        for (i=0; i < rows; i++) {
            pMyMemcpy(pDest, pSrc, count);
            pSrc += src_pitch;
            pDest += dst_pitch;
        }
        return 0;
    }

public:
    long SearchEffort;
    bool UseStrangeBob;

    MEMCPY_FUNC* pMyMemcpy;
    bool IsOdd;
    const unsigned char* pWeaveSrc;
    const unsigned char* pWeaveSrcP;
    unsigned char* pWeaveDest;
    const unsigned char* pCopySrc;
    const unsigned char* pCopySrcP;
    unsigned char* pCopyDest;
    int src_pitch;
    int dst_pitch;
    int rowsize;
    int height;
    int FldHeight;
};

static DScalerFilterTomsMoComp *filter;

void tomsmocomp_init( void )
{
    filter = new DScalerFilterTomsMoComp();
    filter->SearchEffort = SearchEffortDefault;
    filter->UseStrangeBob = UseStrangeBobDefault;
}

#if defined __MMX__
void tomsmocomp_filter_mmx( TDeinterlaceInfo* pInfo )
{
    filter->filterDScaler_MMX( pInfo );
}
#endif

#if defined __3DNOW__
void tomsmocomp_filter_3dnow( TDeinterlaceInfo* pInfo )
{
    filter->filterDScaler_3DNOW( pInfo );
}
#endif

#if defined __SSE__
void tomsmocomp_filter_sse( TDeinterlaceInfo* pInfo )
{
    filter->filterDScaler_SSE( pInfo );
}
#endif

