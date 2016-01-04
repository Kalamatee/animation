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

#define ANIMDF_SHOWPANEL        (1 << 31)  // special flag used by rendering/layout code

struct Animation_Data
{
    ULONG                       ad_Flags;               /* object control flags */
    UWORD                       ad_Frames;             /* # of frames */
    UWORD                       ad_FramesPerSec;
    struct BitMap               *ad_KeyFrame;
    UWORD                       ad_pad0;
    UWORD                       ad_VertTop;
    UWORD                       ad_VertTotal;
    UWORD                       ad_VertVis;
    UWORD                       ad_HorizTop;
    UWORD                       ad_HorizTotal;
    UWORD                       ad_HorizVis;
    IPTR                        ad_ModeID;
    struct BitMapHeader         ad_BitMapHeader;        /* objects embedded bitmap header */
    struct Gadget               *ad_Tapedeck;
    UWORD                       ad_NumColors;
    UWORD                       ad_NumAlloc;
    struct ColorRegister        *ad_ColorRegs;
    UBYTE			*ad_ColorTable;
    UBYTE			*ad_ColorTable2;
    ULONG			*ad_CRegs;
    ULONG                       *ad_GRegs;
};
