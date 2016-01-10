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
#include <proto/utility.h>
#include <proto/realtime.h>
#include <proto/layers.h>

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
    BOOL doTick = FALSE, doLoad = TRUE;
#if (0)
    D(bug("[animation.datatype]: %s(%08x)\n", __PRETTY_FUNCTION__, msg->pmt_Method));
#endif

    switch (msg->pmt_Method)
    {
	case PM_TICK:
            animd->ad_Tick++;

            if (animd->ad_PlayerData->pp_BufferLevel < animd->ad_PlayerData->pp_BufferFrames)
                doLoad = TRUE;

            if (animd->ad_Tick >= animd->ad_TicksPerFrame)
            {
                animd->ad_Tick = 0;
                animd->ad_FrameCurrent++;
                if (animd->ad_FrameCurrent >= animd->ad_Frames)
                    animd->ad_FrameCurrent = 0;
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

#if (0)
    if (doLoad && (animd->ad_BufferProc) && (animd->ad_LoadFrames))
        Signal((struct Task *)animd->ad_BufferProc, animd->ad_LoadFrames);
#endif
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
    struct AnimFrame *curFrame = NULL, *prevFrame = NULL;
    struct gpRender gprMsg;
    struct TagItem attrtags[] =
    {
        { TAG_IGNORE,   0},
        { TAG_DONE,     0}
    };
    UWORD frame = 0, framelast = 0;
    ULONG signal;

    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));

    if (priv)
    {
        D(bug("[animation.datatype] %s: private data @ 0x%p\n", __PRETTY_FUNCTION__, priv));
        D(bug("[animation.datatype] %s: dt obj @ 0x%p, instance data @ 0x%p\n", __PRETTY_FUNCTION__, priv->pp_Object, priv->pp_Data));

        ObtainSemaphore(&priv->pp_FlagsLock);
        priv->pp_PlayerFlags |= PRIVPROCF_RUNNING;
        ReleaseSemaphore(&priv->pp_FlagsLock);

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

                if ((priv->pp_PlayerFlags & PRIVPROCF_ENABLED) && (signal & priv->pp_Data->ad_PlayerTick))
                {
                    frame = priv->pp_Data->ad_FrameCurrent;
                    D(bug("[animation.datatype]: %s: TICK (frame %d)\n", __PRETTY_FUNCTION__, frame));

                    if (frame != framelast)
                    {
                        struct privRenderBuffer *rendFrame = (struct privRenderBuffer *)&gprMsg;
                        rendFrame->MethodID = PRIVATE_RENDERBUFFER;

                        ObtainSemaphoreShared(&priv->pp_Data->ad_AnimFramesLock);

                        if ((!prevFrame) || (frame == 0))
                            curFrame=GetHead(&priv->pp_Data->ad_AnimFrames);
                        else
                            curFrame = prevFrame;

                        if (frame > 0)
                        {
                            while ((curFrame = GetSucc(curFrame)) != NULL)
                            {
                                if (curFrame->af_Frame.alf_Frame == frame)
                                    break;
                            }
                        }

                        ReleaseSemaphore(&priv->pp_Data->ad_AnimFramesLock);

                        if ((curFrame) && (curFrame->af_Frame.alf_BitMap))
                        {
                            rendFrame->Source = curFrame->af_Frame.alf_BitMap;
                            D(bug("[animation.datatype]: %s: Rendering Frame @ 0x%p\n", __PRETTY_FUNCTION__, curFrame));
                            D(bug("[animation.datatype]: %s: #%d BitMap @ 0x%p\n", __PRETTY_FUNCTION__, curFrame->af_Frame.alf_Frame, curFrame->af_Frame.alf_BitMap));
                        }
                        else
                        {
                            if ((priv->pp_Data->ad_BufferProc) && (priv->pp_Data->ad_LoadFrames))
                                Signal((struct Task *)priv->pp_Data->ad_BufferProc, priv->pp_Data->ad_LoadFrames);

                            if ((prevFrame) && (prevFrame->af_Frame.alf_BitMap))
                            {
                                frame = framelast;
                                rendFrame->Source = prevFrame->af_Frame.alf_BitMap;
                            }
                            else
                            {
                                frame = 0;
                                rendFrame->Source = priv->pp_Data->ad_KeyFrame;
                            }
                        }

                        // frame has changed ... render it ..
                        DoMethodA(priv->pp_Object, (Msg)&gprMsg);

                        if ((priv->pp_Data->ad_Window) && !(priv->pp_Data->ad_Flags & ANIMDF_LAYOUT))
                        {
                            if (priv->pp_Data->ad_Tapedeck)
                            {
                                // update the tapedeck gadget..
                                attrtags[0].ti_Tag = TDECK_CurrentFrame;
                                attrtags[0].ti_Data = frame;

                                SetAttrsA((Object *)priv->pp_Data->ad_Tapedeck, attrtags);
                            }
                            // tell the top level gadget to redraw...
                            gprMsg.MethodID   = GM_RENDER;
                            gprMsg.gpr_RPort  = priv->pp_Data->ad_Window->RPort;
                            gprMsg.gpr_GInfo  = NULL;
                            gprMsg.gpr_Redraw = 0;
                            DoGadgetMethodA(priv->pp_Object, priv->pp_Data->ad_Window, NULL, (Msg)&gprMsg);
                        }
                        prevFrame = curFrame;
                        framelast = frame;
                    }
                }
            }
            FreeSignal(priv->pp_Data->ad_PlayerTick);
        }
        ObtainSemaphore(&priv->pp_FlagsLock);
        priv->pp_PlayerFlags &= ~PRIVPROCF_RUNNING;
        ReleaseSemaphore(&priv->pp_FlagsLock);
        priv->pp_Data->ad_PlayerProc = NULL;
    }

    D(bug("[animation.datatype]: %s: exiting ...\n", __PRETTY_FUNCTION__));

    return;

    AROS_USERFUNC_EXIT
}
