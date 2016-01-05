/*
    Copyright � 2015-2016, The AROS Development	Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/layers.h>
#include <proto/datatypes.h>
#include <proto/cybergraphics.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <graphics/gfxbase.h>
#include <graphics/rpattr.h>
#include <intuition/imageclass.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/animationclass.h>
#include <cybergraphx/cybergraphics.h>

#include "animationclass.h"

ADD2LIBS("gadgets/tapedeck.gadget", 0, struct Library *, TapeDeckBase);

const IPTR SupportedMethods[] =
{
    OM_NEW,
    OM_GET,
    OM_SET,
    OM_UPDATE,
    OM_DISPOSE,

    GM_LAYOUT,
    GM_HITTEST,
    GM_GOACTIVE,
    GM_GOINACTIVE,
    GM_HANDLEINPUT,
    GM_RENDER,

    DTM_FRAMEBOX,

    DTM_TRIGGER,

    DTM_CLEARSELECTED,
    DTM_COPY,
    DTM_PRINT,
    DTM_WRITE,

#if (0)
    ADTM_LOCATE,
    ADTM_PAUSE,
    ADTM_START,
    ADTM_STOP,
    ADTM_LOADFRAME,
    ADTM_UNLOADFRAME,
    ADTM_LOADNEWFORMATFRAME,
    ADTM_UNLOADNEWFORMATFRAME,
#endif

    (~0)
};

struct DTMethod SupportedTriggerMethods[] =
{
    { "Play",                   "PLAY",		        STM_PLAY        },
    { "Stop",                   "STOP",		        STM_STOP        },
    { "Pause",                  "PAUSE",	        STM_PAUSE       },

    { "Rewind",                 "REWIND",	        STM_REWIND      },
    { "Fast Forward",           "FF",		        STM_FASTFORWARD },

    { NULL,                     NULL,                   0               },
};

/*** PRIVATE METHODS ***/
IPTR DT_FreePens(struct IClass *cl, struct Gadget *g, Msg msg)
{
    struct Animation_Data *animd = INST_DATA (cl, g);
    int i;

    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));

    if ((animd->ad_ColorMap) && (animd->ad_NumAlloc > 0))
    {
        D(bug("[animation.datatype] %s: attempting to free %d pens\n", __PRETTY_FUNCTION__, animd->ad_NumAlloc));
        D(bug("[animation.datatype] %s: colormap @ 0x%p\n", __PRETTY_FUNCTION__, animd->ad_ColorMap));
        for (i = animd->ad_NumAlloc - 1; i >= 0; i--)
        {
            D(bug("[animation.datatype] %s: freeing pen %d\n", __PRETTY_FUNCTION__, animd->ad_Allocated[i]));
            ReleasePen(animd->ad_ColorMap, animd->ad_Allocated[i]);
        }

        animd->ad_NumAlloc = 0;
    }

    return 1;
}

IPTR DT_FreeColorTables(struct IClass *cl, struct Gadget *g, Msg msg)
{
    struct Animation_Data *animd = INST_DATA (cl, g);

    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));

    if (animd->ad_ColorRegs)
        FreeMem(animd->ad_ColorRegs, 1 + animd->ad_NumColors * sizeof (struct ColorRegister));
    if (animd->ad_ColorTable)
        FreeMem(animd->ad_ColorTable, 1 + animd->ad_NumColors * sizeof (UBYTE));
    if (animd->ad_ColorTable2)
        FreeMem(animd->ad_ColorTable2, 1 + animd->ad_NumColors * sizeof (UBYTE));
    if (animd->ad_Allocated)
    {
        DoMethod((Object *)g, PRIVATE_FREEPENS);
        FreeMem(animd->ad_Allocated, 1 + animd->ad_NumColors * sizeof (UBYTE));
    }
    if (animd->ad_CRegs)
        FreeMem(animd->ad_CRegs, 1 + animd->ad_NumColors * (sizeof (ULONG) * 3));
    if (animd->ad_GRegs)
        FreeMem(animd->ad_GRegs, 1 + animd->ad_NumColors * (sizeof (ULONG) * 3));

    return 1;
}

IPTR DT_AllocColorTables(struct IClass *cl, struct Gadget *g, struct privAllocColorTables *msg)
{
    struct Animation_Data *animd = INST_DATA (cl, g);

    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));

    if ((msg->NumColors != animd->ad_NumColors) && 
        (animd->ad_NumColors > 0))
    {
        DoMethod((Object *)g, PRIVATE_FREECOLORTABLES);
    }
    animd->ad_NumColors = msg->NumColors;
    if (animd->ad_NumColors > 0)
    {
        animd->ad_ColorRegs = AllocMem(1 + animd->ad_NumColors * sizeof (struct ColorRegister), MEMF_CLEAR);
        D(bug("[animation.datatype] %s: ColorRegs @ 0x%p\n", __PRETTY_FUNCTION__, animd->ad_ColorRegs));
        animd->ad_ColorTable = AllocMem(1 + animd->ad_NumColors * sizeof (UBYTE), MEMF_CLEAR); // shared pen table
        D(bug("[animation.datatype] %s: ColorTable @ 0x%p\n", __PRETTY_FUNCTION__, animd->ad_ColorTable));
        animd->ad_ColorTable2 = AllocMem(1 + animd->ad_NumColors * sizeof (UBYTE), MEMF_CLEAR);
        D(bug("[animation.datatype] %s: ColorTable2 @ 0x%p\n", __PRETTY_FUNCTION__, animd->ad_ColorTable2));
        animd->ad_Allocated = AllocMem(1 + animd->ad_NumColors * sizeof (UBYTE), MEMF_CLEAR);
        D(bug("[animation.datatype] %s: Allocated pens Array @ 0x%p\n", __PRETTY_FUNCTION__, animd->ad_Allocated));
        animd->ad_CRegs = AllocMem(1 + animd->ad_NumColors * (sizeof (ULONG) * 3), MEMF_CLEAR); // RGB32 triples used with SetRGB32CM
        D(bug("[animation.datatype] %s: CRegs @ 0x%p\n", __PRETTY_FUNCTION__, animd->ad_CRegs));
        animd->ad_GRegs = AllocMem(1 + animd->ad_NumColors * (sizeof (ULONG) * 3), MEMF_CLEAR); // remapped version of ad_CRegs
        D(bug("[animation.datatype] %s: GRegs @ 0x%p\n", __PRETTY_FUNCTION__, animd->ad_GRegs));
    }

    return 1;
}

IPTR DT_AllocBuffer(struct IClass *cl, struct Gadget *g, struct privAllocBuffer *msg)
{
    struct Animation_Data *animd = INST_DATA (cl, g);

    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));

    if (animd->ad_FrameBuffer)
        FreeBitMap(animd->ad_FrameBuffer);

    animd->ad_FrameBuffer = AllocBitMap(animd->ad_BitMapHeader.bmh_Width, animd->ad_BitMapHeader.bmh_Height, msg->Depth,
                                  BMF_CLEAR, msg->Friend);

    D(bug("[animation.datatype] %s: Frame Buffer @ 0x%p\n", __PRETTY_FUNCTION__, animd->ad_FrameBuffer));
    D(bug("[animation.datatype] %s:     %dx%dx%d\n", __PRETTY_FUNCTION__, animd->ad_BitMapHeader.bmh_Width, animd->ad_BitMapHeader.bmh_Height, msg->Depth));

    if (animd->ad_KeyFrame)
    {
        BltBitMap(animd->ad_KeyFrame, 0, 0, animd->ad_FrameBuffer, 0, 0, animd->ad_BitMapHeader.bmh_Width, animd->ad_BitMapHeader.bmh_Height, 0xC0, 0xFF, NULL);
        animd->ad_Flags |= ANIMDF_DOREMAP;
    }

    return (IPTR)animd->ad_FrameBuffer;
}

IPTR DT_RemapBuffer(struct IClass *cl, struct Gadget *g, Msg msg)
{
    struct Animation_Data *animd = INST_DATA (cl, g);
    struct TagItem bestpenTags[] =
    {
        { OBP_Precision,        PRECISION_IMAGE },
        { TAG_DONE,             0               }
    };
    struct RastPort remapRP;
    ULONG curpen;
    int i, x;

    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));

    if ((animd->ad_Window) && (animd->ad_Flags & ANIMDF_DOREMAP))
    {
        animd->ad_ColorMap = animd->ad_Window->WScreen->ViewPort.ColorMap;
        D(bug("[animation.datatype] %s: colormap @ 0x%p\n", __PRETTY_FUNCTION__, animd->ad_ColorMap));

        for (i = 0; i < animd->ad_NumColors; i++)
        {
            curpen = animd->ad_NumAlloc++;
            animd->ad_Allocated[curpen] = ObtainBestPenA(animd->ad_Window->WScreen->ViewPort.ColorMap,
                animd->ad_CRegs[i * 3], animd->ad_CRegs[i * 3 + 1], animd->ad_CRegs[i * 3 + 2],
                bestpenTags);

            // get the actual color components for the pen.
            GetRGB32(animd->ad_Window->WScreen->ViewPort.ColorMap,
                animd->ad_Allocated[curpen], 1, &animd->ad_GRegs[animd->ad_NumAlloc * 3]);

            D(bug("[animation.datatype] %s: bestpen #%d for %02x%02x%02x\n", __PRETTY_FUNCTION__, animd->ad_Allocated[curpen], (animd->ad_CRegs[i * 3] & 0xFF), (animd->ad_CRegs[i * 3 + 1] & 0xFF), (animd->ad_CRegs[i * 3 + 2] & 0xFF)));

            animd->ad_ColorTable[i] = animd->ad_Allocated[curpen];
            animd->ad_ColorTable2[i] = animd->ad_Allocated[curpen];
        }
        
        InitRastPort(&remapRP);
        remapRP.BitMap = animd->ad_FrameBuffer;

        for(i = 0; i < animd->ad_BitMapHeader.bmh_Height; i++)
        {
            for(x = 0; x < animd->ad_BitMapHeader.bmh_Width; x++)
            {
                curpen = ReadPixel(&remapRP, x, i);
                SetAPen(&remapRP, animd->ad_ColorTable2[curpen]);
                WritePixel(&remapRP, x, i);
            }
        }

        animd->ad_Flags &= ~ANIMDF_DOREMAP;
    }

    return 1;
}

/*** PUBLIC METHODS ***/
IPTR DT_GetMethod(struct IClass *cl, struct Gadget *g, struct opGet *msg)
{
    struct Animation_Data *animd = INST_DATA (cl, g);

    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));

    switch(msg->opg_AttrID)
    {
    case ADTA_KeyFrame:
        D(bug("[animation.datatype] %s: ADTA_KeyFrame\n", __PRETTY_FUNCTION__));
        *msg->opg_Storage = (IPTR) animd->ad_KeyFrame;
        break;

    case ADTA_ModeID:
        D(bug("[animation.datatype] %s: ADTA_ModeID\n", __PRETTY_FUNCTION__));
        *msg->opg_Storage = (IPTR) animd->ad_ModeID;
        break;

    case ADTA_Width:
        D(bug("[animation.datatype] %s: ADTA_Width\n", __PRETTY_FUNCTION__));
        *msg->opg_Storage = (IPTR) animd->ad_BitMapHeader.bmh_Width;
        D(bug("[animation.datatype] %s:     = %d\n", __PRETTY_FUNCTION__, *msg->opg_Storage));
        break;

    case ADTA_Height:
        D(bug("[animation.datatype] %s: ADTA_Height\n", __PRETTY_FUNCTION__));
        *msg->opg_Storage = (IPTR) animd->ad_BitMapHeader.bmh_Height;
        D(bug("[animation.datatype] %s:     = %d\n", __PRETTY_FUNCTION__, *msg->opg_Storage));
        break;

    case ADTA_Depth:
        D(bug("[animation.datatype] %s: ADTA_Depth\n", __PRETTY_FUNCTION__));
        *msg->opg_Storage = (IPTR) animd->ad_BitMapHeader.bmh_Depth;
        D(bug("[animation.datatype] %s:     = %d\n", __PRETTY_FUNCTION__, *msg->opg_Storage));
        break;

    case ADTA_Frames:
        D(bug("[animation.datatype] %s: ADTA_Frames\n", __PRETTY_FUNCTION__));
        *msg->opg_Storage = (IPTR) animd->ad_Frames;
        break;

    case ADTA_Frame:
        D(bug("[animation.datatype] %s: ADTA_Frame\n", __PRETTY_FUNCTION__));
        break;

    case ADTA_FramesPerSecond:
        D(bug("[animation.datatype] %s: ADTA_FramesPerSecond\n", __PRETTY_FUNCTION__));
        *msg->opg_Storage = (IPTR) animd->ad_FramesPerSec;
        break;

    case ADTA_FrameIncrement:
        D(bug("[animation.datatype] %s: ADTA_FrameIncrement\n", __PRETTY_FUNCTION__));
        break;
    case ADTA_Sample:
        D(bug("[animation.datatype] %s: ADTA_Sample\n", __PRETTY_FUNCTION__));
        break;
    case ADTA_SampleLength:
        D(bug("[animation.datatype] %s: ADTA_SampleLength\n", __PRETTY_FUNCTION__));
        break;
    case ADTA_Period:
        D(bug("[animation.datatype] %s: ADTA_Period\n", __PRETTY_FUNCTION__));
        break;
    case ADTA_Volume:
        D(bug("[animation.datatype] %s: ADTA_Volume\n", __PRETTY_FUNCTION__));
        break;
    case ADTA_Cycles:
        D(bug("[animation.datatype] %s: ADTA_Cycles\n", __PRETTY_FUNCTION__));
        break;

    case ADTA_NumColors:
        D(bug("[animation.datatype] %s: ADTA_NumColors\n", __PRETTY_FUNCTION__));
        *msg->opg_Storage = (IPTR) animd->ad_NumColors;
        D(bug("[animation.datatype] %s:     = %d\n", __PRETTY_FUNCTION__, *msg->opg_Storage));
        break;

    case ADTA_NumAlloc:
        D(bug("[animation.datatype] %s: ADTA_NumAlloc\n", __PRETTY_FUNCTION__));
        *msg->opg_Storage = (IPTR) animd->ad_NumAlloc;
        D(bug("[animation.datatype] %s:     = %d\n", __PRETTY_FUNCTION__, *msg->opg_Storage));
        break;

    case ADTA_ColorRegisters:
        D(bug("[animation.datatype] %s: ADTA_ColorRegisters\n", __PRETTY_FUNCTION__));
        *msg->opg_Storage = (IPTR) animd->ad_ColorRegs;
        D(bug("[animation.datatype] %s:     = %p\n", __PRETTY_FUNCTION__, *msg->opg_Storage));
        break;

    case ADTA_ColorTable:
        D(bug("[animation.datatype] %s: ADTA_ColorTable\n", __PRETTY_FUNCTION__));
        *msg->opg_Storage = (IPTR) animd->ad_ColorTable;
        D(bug("[animation.datatype] %s:     = %p\n", __PRETTY_FUNCTION__, *msg->opg_Storage));
        break;

    case ADTA_ColorTable2:
        D(bug("[animation.datatype] %s: ADTA_ColorTable2\n", __PRETTY_FUNCTION__));
        *msg->opg_Storage = (IPTR) animd->ad_ColorTable2;
        D(bug("[animation.datatype] %s:     = %p\n", __PRETTY_FUNCTION__, *msg->opg_Storage));
        break;

    case ADTA_Allocated:
        D(bug("[animation.datatype] %s: ADTA_Allocated\n", __PRETTY_FUNCTION__));
        *msg->opg_Storage = (IPTR) animd->ad_Allocated;
        D(bug("[animation.datatype] %s:     = %p\n", __PRETTY_FUNCTION__, *msg->opg_Storage));
        break;

    case ADTA_CRegs:
        D(bug("[animation.datatype] %s: ADTA_CRegs\n", __PRETTY_FUNCTION__));
        *msg->opg_Storage = (IPTR) animd->ad_CRegs;
        D(bug("[animation.datatype] %s:     = %p\n", __PRETTY_FUNCTION__, *msg->opg_Storage));
        break;

    case ADTA_GRegs:
        D(bug("[animation.datatype] %s: ADTA_GRegs\n", __PRETTY_FUNCTION__));
        *msg->opg_Storage = (IPTR) animd->ad_GRegs;
        D(bug("[animation.datatype] %s:     = %p\n", __PRETTY_FUNCTION__, *msg->opg_Storage));
        break;

    case PDTA_BitMapHeader:
        D(bug("[animation.datatype] %s: PDTA_BitMapHeader\n", __PRETTY_FUNCTION__));
        *msg->opg_Storage = (IPTR) &animd->ad_BitMapHeader;
        break;

    case DTA_TriggerMethods:
        D(bug("[animation.datatype] %s: DTA_TriggerMethods\n", __PRETTY_FUNCTION__));
        *msg->opg_Storage = (IPTR) SupportedTriggerMethods;
        break;

    case DTA_Methods:
        D(bug("[animation.datatype] %s: DTA_Methods\n", __PRETTY_FUNCTION__));
        *msg->opg_Storage = (IPTR) SupportedMethods;
        break;

    case DTA_ControlPanel:
        D(bug("[animation.datatype] %s: DTA_ControlPanel\n", __PRETTY_FUNCTION__));
        if ((animd->ad_Tapedeck) && (animd->ad_Flags & ANIMDF_CONTROLPANEL))
            *msg->opg_Storage = (IPTR) TRUE;
        else
            *msg->opg_Storage = (IPTR) FALSE;
        break;

    case DTA_Immediate:
        D(bug("[animation.datatype] %s: DTA_Immediate\n", __PRETTY_FUNCTION__));
        if (animd->ad_Flags & ANIMDF_IMMEDIATE)
            *msg->opg_Storage = (IPTR) TRUE;
        else
            *msg->opg_Storage = (IPTR) FALSE;
        break;

    case ADTA_Remap:
        D(bug("[animation.datatype] %s: ADTA_Remap\n", __PRETTY_FUNCTION__));
        if (animd->ad_Flags & ANIMDF_REMAP)
            *msg->opg_Storage = (IPTR) TRUE;
        else
            *msg->opg_Storage = (IPTR) FALSE;
        break;

#if (0)
    case ADTA_Repeat:
        D(bug("[animation.datatype] %s: ADTA_Repeat\n", __PRETTY_FUNCTION__));
        if (animd->ad_Flags & ANIMDF_REPEAT)
            *msg->opg_Storage = (IPTR) TRUE;
        else
            *msg->opg_Storage = (IPTR) FALSE;
        break;

    /* NB -: the autodoc isnt clear if we can 'get' these ... */
    case ADTA_AdaptiveFPS:
        D(bug("[animation.datatype] %s: ADTA_AdaptiveFPS\n", __PRETTY_FUNCTION__));
        if (animd->ad_Flags & ANIMDF_ADAPTFPS)
            *msg->opg_Storage = (IPTR) TRUE;
        else
            *msg->opg_Storage = (IPTR) FALSE;
        break;

    case ADTA_SmartSkip:
        D(bug("[animation.datatype] %s: ADTA_SmartSkip\n", __PRETTY_FUNCTION__));
        if (animd->ad_Flags & ANIMDF_SMARTSKIP)
            *msg->opg_Storage = (IPTR) TRUE;
        else
            *msg->opg_Storage = (IPTR) FALSE;
        break;

    case ADTA_OvertakeScreem:
        D(bug("[animation.datatype] %s: ADTA_OvertakeScreem\n", __PRETTY_FUNCTION__));
        if (animd->ad_Flags & ANIMDF_ADJUSTPALETTE)
            *msg->opg_Storage = (IPTR) TRUE;
        else
            *msg->opg_Storage = (IPTR) FALSE;
        break;
#endif
    default:
        return DoSuperMethodA (cl, g, (Msg) msg);
    }

    return (IPTR)TRUE;
}

IPTR DT_SetMethod(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    struct Animation_Data *animd = INST_DATA (cl, g);
    struct TagItem *tstate = msg->ops_AttrList;
    struct TagItem attrtags[] =
    {
        { TAG_IGNORE,   0},
        { TAG_DONE,     0}
    };
    struct TagItem *tag;
    IPTR rv = 0;

    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));

    while((tag = NextTagItem(&tstate)) != NULL)
    {
        switch(tag->ti_Tag)
        {
        case ADTA_ModeID:
            D(bug("[animation.datatype] %s: ADTA_ModeID (%08x)\n", __PRETTY_FUNCTION__, tag->ti_Data));
            animd->ad_ModeID = (IPTR) tag->ti_Data;
            break;

        case ADTA_Width:
            D(bug("[animation.datatype] %s: ADTA_Width (%d)\n", __PRETTY_FUNCTION__, tag->ti_Data));
            animd->ad_BitMapHeader.bmh_Width = (UWORD) tag->ti_Data;
            attrtags[0].ti_Tag = DTA_NominalHoriz;
            attrtags[0].ti_Data = tag->ti_Data;
            SetAttrsA((Object *)g, attrtags);
            break;

        case ADTA_Height:
            D(bug("[animation.datatype] %s: ADTA_Height (%d)\n", __PRETTY_FUNCTION__, tag->ti_Data));
            animd->ad_BitMapHeader.bmh_Height = (UWORD) tag->ti_Data;
            attrtags[0].ti_Tag = DTA_NominalVert;
            if ((animd->ad_Tapedeck) && (animd->ad_Flags & ANIMDF_CONTROLPANEL))
                GetAttr(GA_Height, (Object *)animd->ad_Tapedeck, (IPTR *)&attrtags[0].ti_Data);
            else
                attrtags[0].ti_Data = 0;
            attrtags[0].ti_Data += tag->ti_Data;
            SetAttrsA((Object *)g, attrtags);
            break;

        case ADTA_Depth:
            D(bug("[animation.datatype] %s: ADTA_Depth (%d)\n", __PRETTY_FUNCTION__, tag->ti_Data));
            animd->ad_BitMapHeader.bmh_Depth = (UBYTE) tag->ti_Data;
            break;

        case ADTA_Frames:
            D(bug("[animation.datatype] %s: ADTA_Frames (%d)\n", __PRETTY_FUNCTION__, tag->ti_Data));
            animd->ad_Frames = (UWORD) tag->ti_Data;
            break;

        case ADTA_KeyFrame:
            D(bug("[animation.datatype] %s: ADTA_KeyFrame (0x%p)\n", __PRETTY_FUNCTION__, tag->ti_Data));
            animd->ad_KeyFrame = (struct BitMap *) tag->ti_Data;
            break;

        case ADTA_NumColors:
            D(bug("[animation.datatype] %s: ADTA_NumColors (%d)\n", __PRETTY_FUNCTION__, tag->ti_Data));
            DoMethod((Object *)g, PRIVATE_ALLOCCOLORTABLES, tag->ti_Data);
            break;

        case ADTA_FramesPerSecond:
            D(bug("[animation.datatype] %s: ADTA_FramesPerSecond (%d)\n", __PRETTY_FUNCTION__, tag->ti_Data));
            animd->ad_FramesPerSec = (UWORD) tag->ti_Data;
            break;

        case SDTA_Sample:
            D(bug("[animation.datatype] %s: SDTA_Sample\n", __PRETTY_FUNCTION__));
            break;
        case SDTA_SampleLength:
            D(bug("[animation.datatype] %s: SDTA_SampleLength\n", __PRETTY_FUNCTION__));
            break;
        case SDTA_Period:
            D(bug("[animation.datatype] %s: SDTA_Period\n", __PRETTY_FUNCTION__));
            break;
        case SDTA_Volume:
            D(bug("[animation.datatype] %s: SDTA_Volume\n", __PRETTY_FUNCTION__));
            break;

        case DTA_TopHoriz:
            D(bug("[animation.datatype] %s: DTA_TopHoriz (%d)\n", __PRETTY_FUNCTION__, tag->ti_Data));
            animd->ad_HorizTop = (UWORD) tag->ti_Data;
            break;

        case DTA_TotalHoriz:
            D(bug("[animation.datatype] %s: DTA_TotalHoriz (%d)\n", __PRETTY_FUNCTION__, tag->ti_Data));
            animd->ad_HorizTotal = (UWORD) tag->ti_Data;
            break;

        case DTA_VisibleHoriz:
            D(bug("[animation.datatype] %s: DTA_VisibleHoriz (%d)\n", __PRETTY_FUNCTION__, tag->ti_Data));
            animd->ad_HorizVis = (UWORD) tag->ti_Data;
            break;

        case DTA_TopVert:
            D(bug("[animation.datatype] %s: DTA_TopVert (%d)\n", __PRETTY_FUNCTION__, tag->ti_Data));
            animd->ad_VertTop = (UWORD) tag->ti_Data;
            break;

        case DTA_TotalVert:
            D(bug("[animation.datatype] %s: DTA_TotalVert (%d)\n", __PRETTY_FUNCTION__, tag->ti_Data));
            animd->ad_VertTotal = (UWORD) tag->ti_Data;
            break;

        case DTA_VisibleVert:
            D(bug("[animation.datatype] %s: DTA_VisibleVert (%d)\n", __PRETTY_FUNCTION__, tag->ti_Data));
            animd->ad_VertVis = (UWORD) tag->ti_Data;
            break;

        case DTA_ControlPanel:
            D(bug("[animation.datatype] %s: DTA_ControlPanel\n", __PRETTY_FUNCTION__));
            if ((animd->ad_Tapedeck) && (tag->ti_Data))
                animd->ad_Flags |= ANIMDF_CONTROLPANEL;
            else
                animd->ad_Flags &= ~(ANIMDF_CONTROLPANEL);
            break;

        case DTA_Immediate:
            D(bug("[animation.datatype] %s: DTA_Immediate\n", __PRETTY_FUNCTION__));
            if (tag->ti_Data)
                animd->ad_Flags |= ANIMDF_IMMEDIATE;
            else
                animd->ad_Flags &= ~(ANIMDF_IMMEDIATE);
            break;

        case ADTA_Remap:
            D(bug("[animation.datatype] %s: ADTA_Remap\n", __PRETTY_FUNCTION__));
            if (tag->ti_Data)
                animd->ad_Flags |= ANIMDF_REMAP;
            else
                animd->ad_Flags &= ~(ANIMDF_REMAP);
            break;

#if (0)
        case ADTA_Repeat:
            D(bug("[animation.datatype] %s: ADTA_Repeat\n", __PRETTY_FUNCTION__));
            if (tag->ti_Data)
                animd->ad_Flags |= ANIMDF_REPEAT;
            else
                animd->ad_Flags &= ~(ANIMDF_REPEAT);
            break;

        case ADTA_AdaptiveFPS:
            D(bug("[animation.datatype] %s: ADTA_AdaptiveFPS\n", __PRETTY_FUNCTION__));
            if (tag->ti_Data)
                animd->ad_Flags |= ANIMDF_ADAPTFPS;
            else
                animd->ad_Flags &= ~(ANIMDF_ADAPTFPS);
            break;

        case ADTA_SmartSkip:
            D(bug("[animation.datatype] %s: ADTA_SmartSkip\n", __PRETTY_FUNCTION__));
            if (tag->ti_Data)
                animd->ad_Flags |= ANIMDF_SMARTSKIP;
            else
                animd->ad_Flags &= ~(ANIMDF_SMARTSKIP);
            break;

        case ADTA_OvertakeScreem:
            D(bug("[animation.datatype] %s: ADTA_OvertakeScreem\n", __PRETTY_FUNCTION__));
            if (tag->ti_Data)
                animd->ad_Flags |= ANIMDF_ADJUSTPALETTE;
            else
                animd->ad_Flags &= ~(ANIMDF_ADJUSTPALETTE);
            break;
#endif
        }
    }

    return (DoSuperMethodA (cl, g, (Msg) msg));
}

IPTR DT_NewMethod(struct IClass *cl, Object *o, struct opSet *msg)
{
    struct Gadget *g;
    struct Animation_Data *animd;
    struct TagItem tdtags[] =
    {
        { GA_RelVerify, TRUE},
        { GA_Width, 200},
        { GA_Height, 15},
        { TAG_DONE, 0}
    };

    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));

    g = (struct Gadget *) DoSuperMethodA(cl, o, (Msg) msg);
    if (g)
    {
        D(bug("[animation.datatype] %s: Created object 0x%p\n", __PRETTY_FUNCTION__, g));

        animd = (struct Animation_Data *) INST_DATA(cl, g);

        animd->ad_Flags = ANIMDF_CONTROLPANEL|ANIMDF_REMAP;

        if (msg->ops_AttrList)
        {
            D(bug("[animation.datatype] %s: Setting attributes.. \n", __PRETTY_FUNCTION__));
            DT_SetMethod(cl, g, msg);
        }

        D(bug("[animation.datatype] %s: Prepare controls.. \n", __PRETTY_FUNCTION__));
        /* create a tapedeck gadget */
        if ((animd->ad_Tapedeck = NewObjectA(NULL, "tapedeck.gadget", tdtags)) != NULL)
        {
            D(bug("[animation.datatype] %s: Tapedeck @ 0x%p\n", __PRETTY_FUNCTION__, animd->ad_Tapedeck));
        }
    }

    D(bug("[animation.datatype] %s: returning %p\n", __PRETTY_FUNCTION__, g));

    return (IPTR)g;
}

IPTR DT_DisposeMethod(struct IClass *cl, Object *o, Msg msg)
{
    struct Animation_Data *animd = INST_DATA (cl, o);

    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));

    DoMethod(o, PRIVATE_FREECOLORTABLES);

    if (animd->ad_FrameBuffer)
        FreeBitMap(animd->ad_FrameBuffer);

    if (animd->ad_Tapedeck)
        DisposeObject (animd->ad_Tapedeck);

    return DoSuperMethodA(cl, o, msg);
}

IPTR DT_GoActiveMethod(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));
    return NULL;
}

IPTR DT_GoInActiveMethod(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));
    return NULL;
}

IPTR DT_HandleInputMethod(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));
    return NULL;
}

IPTR DT_HelpTestMethod(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));
    return NULL;
}

IPTR DT_HitTestMethod(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));
    return NULL;
}

IPTR DT_Layout(struct IClass *cl, struct Gadget *g, struct gpLayout *msg)
{
    struct Animation_Data *animd = INST_DATA (cl, (Object *)g);
    struct IBox *gadBox;
    IPTR RetVal;

    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));

    // cache the window pointer
    animd->ad_Window = msg->gpl_GInfo->gi_Window;

    RetVal = DoSuperMethodA(cl, (Object *)g, (Msg)msg);

    GetAttr(DTA_Domain, (Object *)g, (IPTR *)&gadBox);

    // propogate our known dimensions to the tapedeck ..
    if (animd->ad_Tapedeck)
    {
        struct TagItem tdAttrs[] =
        {
            { GA_Left,          gadBox->Left                                    },
            { GA_Top,           0                                               },
            { GA_Width,         animd->ad_BitMapHeader.bmh_Width                },
            { GA_Height,        0                                               },
            { TAG_DONE,         0                                               }
        };

        GetAttr(GA_Height, (Object *)animd->ad_Tapedeck, (IPTR *)&tdAttrs[3].ti_Data);

        D(bug("[animation.datatype]: %s: tapedeck height = %d\n", __PRETTY_FUNCTION__, tdAttrs[3].ti_Data));

        animd->ad_Flags |= ANIMDF_SHOWPANEL;

        // try to adjust to accomodate it ..
        if (gadBox->Height > animd->ad_BitMapHeader.bmh_Height + tdAttrs[3].ti_Data)
            tdAttrs[1].ti_Data = gadBox->Top + animd->ad_BitMapHeader.bmh_Height;
        else
        {
            if (gadBox->Height > tdAttrs[3].ti_Data)
                tdAttrs[1].ti_Data = gadBox->Top + gadBox->Height - tdAttrs[3].ti_Data;
            else
            {
                // TODO: Adjust the tapedeck height or hide it?
                D(bug("[animation.datatype]: %s: tapedeck to big for visible space!\n", __PRETTY_FUNCTION__));
                animd->ad_Flags &= ~ANIMDF_SHOWPANEL;
            }
        }

        if (gadBox->Width > animd->ad_BitMapHeader.bmh_Width)
            tdAttrs[2].ti_Data = animd->ad_BitMapHeader.bmh_Width;
        else
            tdAttrs[2].ti_Data = gadBox->Width;

        SetAttrsA((Object *)animd->ad_Tapedeck, tdAttrs);

        // .. and ask it to layout
        // NB - Do not use async layout or it crashes .. =(
        DoMethod((Object *)animd->ad_Tapedeck,
            GM_LAYOUT, (IPTR)msg->gpl_GInfo, (IPTR)msg->gpl_Initial);
    }

#if (0)
    NotifyAttrChanges((Object *) g, msg->gpl_GInfo, 0,
   				 GA_ID, g->GadgetID,
   				 DTA_Busy, TRUE,
   				 TAG_DONE);
#endif

    return RetVal;
}

IPTR DT_Render(struct IClass *cl, struct Gadget *g, struct gpRender *msg)
{
    struct Animation_Data *animd = INST_DATA (cl, (Object *)g);
    struct IBox *gadBox;
    UWORD fillwidth, fillheight;

    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));

    GetAttr(DTA_Domain, (Object *)g, (IPTR *)&gadBox);

    if (gadBox->Width > animd->ad_BitMapHeader.bmh_Width)
        fillwidth = animd->ad_BitMapHeader.bmh_Width;
    else
        fillwidth = gadBox->Width;
        
    if (gadBox->Height > animd->ad_BitMapHeader.bmh_Height)
        fillheight = animd->ad_BitMapHeader.bmh_Height;
    else
        fillheight = gadBox->Height;

    if (!animd->ad_FrameBuffer)
    {
        IPTR bmdepth;

        bmdepth = GetBitMapAttr(msg->gpr_RPort->BitMap, BMA_DEPTH);
        if (bmdepth < animd->ad_BitMapHeader.bmh_Depth)
            bmdepth = animd->ad_BitMapHeader.bmh_Height;

        DoMethod((Object *)g, PRIVATE_ALLOCBUFFER, msg->gpr_RPort->BitMap, bmdepth);

        if ((animd->ad_KeyFrame) && (animd->ad_Flags & ANIMDF_REMAP))
        {
            DoMethod((Object *)g, PRIVATE_REMAPBUFFER);
        }
    }

    if (animd->ad_FrameBuffer)
    {
        BltBitMapRastPort(animd->ad_FrameBuffer, 0, 0, msg->gpr_RPort, gadBox->Left, gadBox->Top, fillwidth, fillheight, 0xC0);
    }
    else
    {
        // for now fill the animations area
        SetRPAttrs(msg->gpr_RPort, RPTAG_FgColor, 0xEE8888, TAG_DONE );
        RectFill(msg->gpr_RPort, gadBox->Left, gadBox->Top , gadBox->Left + fillwidth, gadBox->Top + fillheight);
    }

    if (animd->ad_Flags & ANIMDF_SHOWPANEL)
    {
        DoMethodA((Object *)animd->ad_Tapedeck, (Msg)msg);
    }

    return (IPTR)TRUE;
}

IPTR DT_FrameBox(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));
    return NULL;
}

IPTR DT_ProcLayout(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));
    return NULL;
}

IPTR DT_Print(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));
    return NULL;
}

IPTR DT_Copy(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));
    return NULL;
}

IPTR DT_RemoveDTObject(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));
    return NULL;
}

IPTR DT_Trigger(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));
    return NULL;
}

IPTR DT_Write(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));
    return NULL;
}

IPTR DT_Locate(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));
    return NULL;
}

IPTR DT_Pause(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));
    return NULL;
}

IPTR DT_Start(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));
    return NULL;
}

IPTR DT_Stop(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));
    return NULL;
}

IPTR DT_LoadFrame(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));
    return NULL;
}

IPTR DT_UnLoadFrame(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));
    return NULL;
}

IPTR DT_LoadNewFormatFrame(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));
    return NULL;
}

IPTR DT_UnLoadNewFormatFrame(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));
    return NULL;
}
