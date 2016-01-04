/*
    Copyright © 2015-2016, The AROS Development	Team. All rights reserved.
    $Id$
*/

#include <graphics/gfx.h>
#include <datatypes/pictureclass.h>

#define	MIN(a,b) (((a) < (b)) ?	(a) : (b))
#define	MAX(a,b) (((a) > (b)) ?	(a) : (b))

// api flags
#define	ANIMDF_CONTROLPANEL     (1 << 0)
#define	ANIMDF_IMMEDIATE        (1 << 1)
#define	ANIMDF_REMAP            (1 << 2)
#define	ANIMDF_REPEAT           (1 << 3)
#define	ANIMDF_ADAPTFPS         (1 << 4)
#define	ANIMDF_SMARTSKIP        (1 << 5)
#define	ANIMDF_ADJUSTPALETTE    (1 << 6)
// special flags used by rendering/layout code
#define ANIMDF_DOREMAP          (1 << 30)               
#define ANIMDF_SHOWPANEL        (1 << 31)

struct Animation_Data
{
    ULONG                       ad_Flags;               /* object control flags                 */
    UWORD                       ad_Frames;              /* # of frames                          */
    UWORD                       ad_FramesPerSec;
    struct BitMap               *ad_KeyFrame;
    struct BitMap               *ad_FrameBuffer;        /* currently displayed frame            */
    UWORD                       ad_pad0;
    UWORD                       ad_VertTop;
    UWORD                       ad_VertTotal;
    UWORD                       ad_VertVis;
    UWORD                       ad_HorizTop;
    UWORD                       ad_HorizTotal;
    UWORD                       ad_HorizVis;
    IPTR                        ad_ModeID;
    struct BitMapHeader         ad_BitMapHeader;        /* objects embedded bitmap header       */
    struct Gadget               *ad_Tapedeck;
    UWORD                       ad_NumColors;
    UWORD                       ad_NumAlloc;
    struct Window               *ad_Window;
    struct ColorMap             *ad_ColorMap;
    struct ColorRegister        *ad_ColorRegs;
    ULONG			*ad_CRegs;
    ULONG                       *ad_GRegs;
    UBYTE			*ad_ColorTable;
    UBYTE			*ad_ColorTable2;
    UBYTE			*ad_Allocated;          /* pens we have actually allocated      */
};

#define PRIVATE_ALLOCCOLORTABLES        (0x0808000000 + 1)
#define PRIVATE_FREECOLORTABLES         (0x0808000000 + 2) 
#define PRIVATE_FREEPENS                (0x0808000000 + 3)             
#define PRIVATE_ALLOCBUFFER             (0x0808000000 + 4)
#define PRIVATE_REMAPBUFFER             (0x0808000000 + 5)

struct privAllocColorTables
{
    STACKED ULONG MethodID;
    STACKED ULONG NumColors;
};

struct privAllocBuffer
{
    STACKED ULONG MethodID;
    STACKED struct BitMap *Friend;
    STACKED UBYTE Depth;
};
