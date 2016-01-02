/*
    Copyright © 2015-2016, The AROS Development	Team. All rights reserved.
    $Id$
*/

#include <graphics/gfx.h>
#include <datatypes/pictureclass.h>

#define	MIN(a,b) (((a) < (b)) ?	(a) : (b))
#define	MAX(a,b) (((a) > (b)) ?	(a) : (b))

#define	ANIMDF_CONTROLPANEL     (1 << 0)
#define	ANIMDF_IMMEDIATE        (1 << 1)
#define	ANIMDF_REMAP            (1 << 2)
#define	ANIMDF_REPEAT           (1 << 3)
#define	ANIMDF_ADAPTFPS         (1 << 4)
#define	ANIMDF_SMARTSKIP        (1 << 5)
#define	ANIMDF_ADJUSTPALETTE    (1 << 6)

struct Animation_Data
{
    struct Gadget               *ad_Tapedeck;
    struct ColorRegister        *ad_ColorRegs;
    UBYTE			*ad_ColorTable;
    UBYTE			*ad_ColorTable2;
    ULONG			*ad_CRegs;
    ULONG                       *ad_GRegs;
    UWORD                       ad_NumColors;
    UWORD                       ad_NumAlloc;
    ULONG                       ad_Flags;               /* object control flags */
    struct BitMapHeader         ad_BitMapHeader;        /* objects embedded bitmap header */
};
