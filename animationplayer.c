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
#include <proto/utility.h>
#include <proto/realtime.h>

#include <intuition/gadgetclass.h>
#include <libraries/realtime.h>
#include <gadgets/tapedeck.h>

#include "animationclass.h"

AROS_UFH3(ULONG, playerHookFunc, 
    AROS_UFHA(struct    Hook *,     hook, A0), 
    AROS_UFHA(struct Player *, obj, A2), 
    AROS_UFHA(struct pmTime *, msg, A1))
{ 
    AROS_USERFUNC_INIT
 
    struct Animation_Data *animd = (struct Animation_Data *)hook->h_Data;
    BOOL doTick = FALSE;
#if (0)
    D(bug("[animation.datatype]: %s(%08x)\n", __PRETTY_FUNCTION__, msg->pmt_Method));
#endif

    switch (msg->pmt_Method)
    {
	case PM_TICK:
            animd->ad_Tick++;

            if (animd->ad_Tick >= animd->ad_TicksPerFrame)
            {
                animd->ad_Tick = 0;
                animd->ad_FrameCurrent++;
                doTick = TRUE;
            }
	    break;

	case PM_SHUTTLE:
            doTick = TRUE;
	    break;

	case PM_STATE:
            doTick = TRUE;
	    break;
    }

    if (doTick && (animd->ad_PlayerProc) && (animd->ad_PlayerTick))
        Signal((struct Task *)animd->ad_PlayerProc, animd->ad_PlayerTick);

    return 0;

    AROS_USERFUNC_EXIT
}


AROS_UFH3(void, playerProc,
        AROS_UFHA(STRPTR,              argPtr, A0),
        AROS_UFHA(ULONG,               argSize, D0),
        AROS_UFHA(struct ExecBase *,   SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct ProcessPrivate *priv = FindTask(NULL)->tc_UserData;
    struct gpRender gprMsg;
    struct TagItem attrtags[] =
    {
        { TAG_IGNORE,   0},
        { TAG_DONE,     0}
    };
    ULONG signal;
    UWORD framelast = 0;

    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));

    if (priv)
    {
        D(bug("[animation.datatype] %s: private data @ 0x%p\n", __PRETTY_FUNCTION__, priv));
        D(bug("[animation.datatype] %s: dt obj @ 0x%p, instance data @ 0x%p\n", __PRETTY_FUNCTION__, priv->pp_Object, priv->pp_Data));

        if ((priv->pp_Data->ad_PlayerTick = AllocSignal(-1)) != 0)
        {
            D(bug("[animation.datatype]: %s: allocated tick signal (%x)\n", __PRETTY_FUNCTION__, priv->pp_Data->ad_PlayerTick));
            while (TRUE)
            {
                signal = priv->pp_Data->ad_PlayerTick | SIGBREAKF_CTRL_C;
                signal = Wait(signal);

                D(bug("[animation.datatype]: %s: signalled (%08x)\n", __PRETTY_FUNCTION__, signal));

                if (signal & SIGBREAKF_CTRL_C)
                    break;

                if (signal & priv->pp_Data->ad_PlayerTick)
                {
                    D(bug("[animation.datatype]: %s: TICK (frame %d)\n", __PRETTY_FUNCTION__, priv->pp_Data->ad_FrameCurrent));

                    if (priv->pp_Data->ad_FrameCurrent >= priv->pp_Data->ad_Frames)
                        priv->pp_Data->ad_FrameCurrent = 0;

                    if (priv->pp_Data->ad_FrameCurrent != framelast)
                    {
                        struct privRenderBuffer *rendFrame = (struct privRenderBuffer *)&gprMsg;
                        rendFrame->MethodID = PRIVATE_RENDERBUFFER;
                        rendFrame->Source = priv->pp_Data->ad_KeyFrame;

                        // frame has changed ... render it ..
                        DoMethodA(priv->pp_Object, (Msg)&gprMsg);

                        if (priv->pp_Data->ad_Tapedeck)
                        {
                            // update the tapedeck gadget..
                            attrtags[0].ti_Tag = TDECK_CurrentFrame;
                            attrtags[0].ti_Data = priv->pp_Data->ad_FrameCurrent;
                            SetAttrsA((Object *)priv->pp_Data->ad_Tapedeck, attrtags);

                            if (priv->pp_Data->ad_Window)
                            {
                                // tell the top level gadget to redraw...
                                gprMsg.MethodID   = GM_RENDER;
                                gprMsg.gpr_RPort  = priv->pp_Data->ad_Window->RPort;
                                gprMsg.gpr_GInfo  = NULL;
                                gprMsg.gpr_Redraw = 0;
                                DoGadgetMethodA(priv->pp_Object, priv->pp_Data->ad_Window, NULL, (Msg)&gprMsg);
                            }
                        }
                        framelast = priv->pp_Data->ad_FrameCurrent;
                    }
                }
            }
            FreeSignal(priv->pp_Data->ad_PlayerTick);
        }
    }

    D(bug("[animation.datatype]: %s: exiting ...\n", __PRETTY_FUNCTION__));

    return;

    AROS_USERFUNC_EXIT
}
