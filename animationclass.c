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

IPTR DT_GetMethod(struct IClass *cl, struct Gadget *g, struct opGet *msg)
{
    struct Animation_Data *animd = INST_DATA (cl, g);

    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));

    switch(msg->opg_AttrID)
    {
/*
        need to handle the following attribs -:

    case ADTA_KeyFrame:
    case ADTA_ModeID:
    case ADTA_Width:
    case ADTA_Height:
    case ADTA_Depth:
    case ADTA_Frames:
    case ADTA_Frame:
    case ADTA_FramesPerSecond:
    case ADTA_FrameIncrement:
    case ADTA_Sample:
    case ADTA_SampleLength:
    case ADTA_Period:
    case ADTA_Volume:
    case ADTA_Cycles:
    case ADTA_NumColors:
    case ADTA_NumAlloc:
    case ADTA_ColorTable:
    case ADTA_ColorTable2:
    case ADTA_ColorRegisters:
    case ADTA_CRegs:
    case ADTA_GRegs:
    case ADTA_Allocated:
    case PDTA_BitMapHeader:
    case DTA_TriggerMethods:
    case DTA_Methods:
*/
    }

    return (DoSuperMethodA (cl, g, (Msg) msg));
}

IPTR DT_SetMethod(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));

/*    
    need to handle the following attribs -:

    case ADTA_ModeID:
    case ADTA_Width:
    case ADTA_Height:
    case ADTA_Depth:
    case ADTA_Frames:
    case ADTA_KeyFrame:
    case ADTA_NumColors:
    case ADTA_FramesPerSecond:
    case ADTA_Remap:
    case SDTA_Sample:
    case SDTA_SampleLength:
    case SDTA_Period:
    case SDTA_Volume:
    case DTA_TopHoriz:
    case DTA_TotalHoriz:
    case DTA_VisibleHoriz:
    case DTA_TopVert:
    case DTA_TotalVert:
    case DTA_VisibleVert:
    case DTA_ControlPanel:
    case DTA_Immediate:
*/

    return NULL;
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

        D(bug("[animation.datatype] %s: Setting attributes.. \n", __PRETTY_FUNCTION__));
        DT_SetMethod(cl, g, msg);

        /* create a tapedeck gadget */
        if ((animd->ad_Tapedeck = NewObjectA(NULL, "tapedeck.gadget", tdtags)) != NULL)
        {
            D(bug("[animation.datatype] %s: Tapedeck @ 0x%p\n", __PRETTY_FUNCTION__, animd->ad_Tapedeck));
        }
    }

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
