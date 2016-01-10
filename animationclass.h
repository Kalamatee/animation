/*
    Copyright © 2015-2016, The AROS Development	Team. All rights reserved.
    $Id$
*/

#include <graphics/gfx.h>
#include <datatypes/pictureclass.h>
#include <datatypes/animationclass.h>

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
#define ANIMDF_LAYOUT           (1 << 29)               
#define ANIMDF_REMAPPEDPENS     (1 << 30)               
#define ANIMDF_SHOWPANEL        (1 << 31)

#define ANIMPLAYER_TICKFREQ     ((struct RealTimeBase *)RealTimeBase)->rtb_Reserved1

struct ProcessPrivate;

struct Animation_Data
{
    ULONG                       ad_Flags;               /* object control flags                 */
    UWORD                       ad_Frames;              /* # of frames                          */
    UWORD                       ad_FrameCurrent;
    UWORD                       ad_FramesPerSec;
    UWORD                       ad_TicksPerFrame;       /* TICK_FREQ / ad_FramesPerSec */
    UWORD                       ad_Tick;
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
    ULONG                       ad_PenPrecison;
    struct Player               *ad_Player;
    struct ProcessPrivate       *ad_PlayerData;
    struct Process              *ad_BufferProc;         /* buffering process */
    struct Process              *ad_PlayerProc;         /* playback process */
    struct Hook                 ad_PlayerHook;
    UBYTE                       ad_LoadFrames;          /* signal frames need to be loaded */
    UBYTE                       ad_PlayerTick;          /* signal frames needs to change */
    struct SignalSemaphore      ad_AnimFramesLock;
    struct List                 ad_AnimFrames;
};

struct ProcessPrivate
{
    Object                      *pp_Object;
    struct Animation_Data       *pp_Data;
    struct SignalSemaphore      pp_FlagsLock;
    volatile ULONG              pp_PlayerFlags;
    volatile ULONG              pp_BufferFlags;
    ULONG                       pp_BufferFrames;       /* no of frames to buffer */
    ULONG                       pp_BufferLevel;        /* no of frames buffered */
};

#define PRIVPROCF_ENABLED       (1 << 0)
#define PRIVPROCF_RUNNING       (1 << 1)

/* our nodes used to play the anim! */
struct AnimFrame
{
    struct Node                 af_Node;
    struct adtFrame             af_Frame;
};

#define PRIVATE_INITPLAYER              (0x0808000000 + 1)
#define PRIVATE_ALLOCCOLORTABLES        (0x0808000000 + 2)
#define PRIVATE_FREECOLORTABLES         (0x0808000000 + 3) 
#define PRIVATE_FREEPENS                (0x0808000000 + 4)             
#define PRIVATE_ALLOCBUFFER             (0x0808000000 + 5)
#define PRIVATE_RENDERBUFFER            (0x0808000000 + 6)
#define PRIVATE_REMAPBUFFER             (0x0808000000 + 7)

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


struct privRenderBuffer
{
    STACKED ULONG MethodID;
    STACKED struct BitMap *Source;
};
