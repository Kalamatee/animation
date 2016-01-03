/*
    Copyright © 2015-2016, The AROS Development	Team. All rights reserved.
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
    GM_HELPTEST,

    DTM_PROCLAYOUT,
    DTM_FRAMEBOX,

    DTM_REMOVEDTOBJECT,
    DTM_TRIGGER,

//    DTM_SELECT,
//    DTM_CLEARSELECTED,
    DTM_COPY,
    DTM_PRINT,
    DTM_WRITE,

    ADTM_LOCATE,
    ADTM_PAUSE,
    ADTM_START,
    ADTM_STOP,
    ADTM_LOADFRAME,
    ADTM_UNLOADFRAME,
    ADTM_LOADNEWFORMATFRAME,
    ADTM_UNLOADNEWFORMATFRAME,

    (~0)
};

void AnimDT_AllocColorTables(struct Animation_Data *animd, UWORD numcolors)
{
    if ((numcolors != animd->ad_NumColors) && 
        (animd->ad_NumColors > 0))
    {
        if (animd->ad_ColorRegs)
            FreeMem(animd->ad_ColorRegs, 1 + animd->ad_NumColors * sizeof (struct ColorRegister));
        if (animd->ad_ColorTable)
            FreeMem(animd->ad_ColorTable, 1 + animd->ad_NumColors * sizeof (UBYTE));
        if (animd->ad_ColorTable2)
            FreeMem(animd->ad_ColorTable2, 1 + animd->ad_NumColors * sizeof (UBYTE));
        if (animd->ad_CRegs)
            FreeMem(animd->ad_CRegs, 1 + animd->ad_NumColors * (sizeof (ULONG) * 3));
        if (animd->ad_GRegs)
            FreeMem(animd->ad_GRegs, 1 + animd->ad_NumColors * (sizeof (ULONG) * 3));
    }
    animd->ad_NumColors = numcolors;
    if (animd->ad_NumColors > 0)
    {
        animd->ad_ColorRegs = AllocMem(1 + animd->ad_NumColors * sizeof (struct ColorRegister), MEMF_CLEAR);
        D(bug("[animation.datatype] %s: ColorRegs @ 0x%p\n", __PRETTY_FUNCTION__, animd->ad_ColorRegs));
        animd->ad_ColorTable = AllocMem(1 + animd->ad_NumColors * sizeof (UBYTE), MEMF_CLEAR); // shared pen table
        D(bug("[animation.datatype] %s: ColorTable @ 0x%p\n", __PRETTY_FUNCTION__, animd->ad_ColorTable));
        animd->ad_ColorTable2 = AllocMem(1 + animd->ad_NumColors * sizeof (UBYTE), MEMF_CLEAR);
        D(bug("[animation.datatype] %s: ColorTable2 @ 0x%p\n", __PRETTY_FUNCTION__, animd->ad_ColorTable2));
        animd->ad_CRegs = AllocMem(1 + animd->ad_NumColors * (sizeof (ULONG) * 3), MEMF_CLEAR); // RGB32 triples used with SetRGB32CM
        D(bug("[animation.datatype] %s: CRegs @ 0x%p\n", __PRETTY_FUNCTION__, animd->ad_CRegs));
        animd->ad_GRegs = AllocMem(1 + animd->ad_NumColors * (sizeof (ULONG) * 3), MEMF_CLEAR); // remapped version of ad_CRegs
        D(bug("[animation.datatype] %s: GRegs @ 0x%p\n", __PRETTY_FUNCTION__, animd->ad_GRegs));
    }
}

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
        break;
    case DTA_Methods:
        D(bug("[animation.datatype] %s: DTA_Methods\n", __PRETTY_FUNCTION__));
        break;

    case DTA_ControlPanel:
        D(bug("[animation.datatype] %s: DTA_ControlPanel\n", __PRETTY_FUNCTION__));
        if (animd->ad_Flags & ANIMDF_CONTROLPANEL)
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
            break;

        case ADTA_Height:
            D(bug("[animation.datatype] %s: ADTA_Height (%d)\n", __PRETTY_FUNCTION__, tag->ti_Data));
            animd->ad_BitMapHeader.bmh_Height = (UWORD) tag->ti_Data;
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
            D(bug("[animation.datatype] %s: ADTA_KeyFrame (%d)\n", __PRETTY_FUNCTION__, tag->ti_Data));
            animd->ad_KeyFrame = (UWORD) tag->ti_Data;
            break;

        case ADTA_NumColors:
            D(bug("[animation.datatype] %s: ADTA_NumColors (%d)\n", __PRETTY_FUNCTION__, tag->ti_Data));
            AnimDT_AllocColorTables(animd, (UWORD)tag->ti_Data);
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
            if (tag->ti_Data)
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
    IPTR RetVal;

    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));

#if (0)
    NotifyAttrChanges((Object *) g, msg->gpl_GInfo, 0,
   				 GA_ID, g->GadgetID,
   				 DTA_Busy, TRUE,
   				 TAG_DONE);
#endif

    RetVal = DoSuperMethodA(cl, (Object *) g, (Msg) msg);
    
    RetVal += (IPTR) DoAsyncLayout((Object *) g, msg);

    return(RetVal);
}

IPTR DT_Render(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));
    return NULL;
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
