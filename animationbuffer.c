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

/*
    Process to handle loading/discarding (buffering) of  animation frames
    realtime player & playback thread will signal us when needed.
*/

AROS_UFH3(void, bufferProc,
        AROS_UFHA(STRPTR,              argPtr, A0),
        AROS_UFHA(ULONG,               argSize, D0),
        AROS_UFHA(struct ExecBase *,   SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct ProcessPrivate *priv = FindTask(NULL)->tc_UserData;
    struct AnimFrame *curFrame = NULL, *lastFrame = NULL;
    ULONG signal;

    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));

    if (priv)
    {
        D(bug("[animation.datatype] %s: private data @ 0x%p\n", __PRETTY_FUNCTION__, priv));
        D(bug("[animation.datatype] %s: dt obj @ 0x%p, instance data @ 0x%p\n", __PRETTY_FUNCTION__, priv->pp_Object, priv->pp_Data));

        ObtainSemaphore(&priv->pp_FlagsLock);
        priv->pp_BufferFlags |= PRIVPROCF_RUNNING;
        ReleaseSemaphore(&priv->pp_FlagsLock);

        if ((priv->pp_Data->ad_LoadFrames = AllocSignal(-1)) != 0)
        {
            D(bug("[animation.datatype]: %s: allocated load signal (%x)\n", __PRETTY_FUNCTION__, priv->pp_Data->ad_LoadFrames));
            while (TRUE)
            {
                if ((priv->pp_BufferFlags & PRIVPROCF_ENABLED) &&
                    (priv->pp_BufferLevel < priv->pp_BufferFrames) &&
                    (priv->pp_BufferLevel < priv->pp_Data->ad_Frames))
                {
                    D(bug("[animation.datatype]: %s: %d:%d\n", __PRETTY_FUNCTION__, priv->pp_BufferLevel, priv->pp_BufferFrames));
                    signal = priv->pp_Data->ad_LoadFrames;
                }
                else
                {
                    signal = priv->pp_Data->ad_LoadFrames | SIGBREAKF_CTRL_C;
                    signal = Wait(signal);
                };

                D(bug("[animation.datatype]: %s: signalled (%08x)\n", __PRETTY_FUNCTION__, signal));

                if (signal & SIGBREAKF_CTRL_C)
                    break;

                if ((priv->pp_BufferFlags & PRIVPROCF_ENABLED) && (signal & priv->pp_Data->ad_LoadFrames))
                {
                    D(bug("[animation.datatype]: %s: Loading Frames...\n", __PRETTY_FUNCTION__));

                    if ((curFrame) ||
                        ((curFrame = AllocMem(sizeof(struct AnimFrame), MEMF_ANY)) != NULL))
                    {
                        D(bug("[animation.datatype]: %s: using AnimFrame @ 0x%p\n", __PRETTY_FUNCTION__, curFrame));

                        curFrame->af_Frame.MethodID = ADTM_LOADFRAME;

                        if (lastFrame)
                        {
                            curFrame->af_Frame.alf_Frame = lastFrame->af_Frame.alf_Frame + 1;
                            curFrame->af_Frame.alf_TimeStamp = lastFrame->af_Frame.alf_TimeStamp + 1;
                        }
                        else
                        {
                            curFrame->af_Frame.alf_Frame = 0;
                            curFrame->af_Frame.alf_TimeStamp = 0;
                        }
                        curFrame->af_Frame.alf_BitMap = NULL;
                        curFrame->af_Frame.alf_CMap = NULL;
                        curFrame->af_Frame.alf_Sample = NULL;
                        curFrame->af_Frame.alf_UserData = NULL;

                        D(bug("[animation.datatype]: %s: Loading Frame #%d\n", __PRETTY_FUNCTION__, curFrame->af_Frame.alf_Frame));

                        if (DoMethodA(priv->pp_Object, (Msg)&curFrame->af_Frame))
                        {
                            priv->pp_BufferLevel++;
                            D(bug("[animation.datatype]: %s: Loaded! bitmap @ %p\n", __PRETTY_FUNCTION__, curFrame->af_Frame.alf_BitMap));
                            ObtainSemaphore(&priv->pp_Data->ad_AnimFramesLock);
                            AddTail(&priv->pp_Data->ad_AnimFrames, &curFrame->af_Node);
                            ReleaseSemaphore(&priv->pp_Data->ad_AnimFramesLock);
                            lastFrame =  curFrame;
                            curFrame = NULL;
                        }
                    }
                }
            }
        }
        ObtainSemaphore(&priv->pp_FlagsLock);
        priv->pp_BufferFlags &= ~PRIVPROCF_RUNNING;
        ReleaseSemaphore(&priv->pp_FlagsLock);
        priv->pp_Data->ad_BufferProc = NULL;
    }
    D(bug("[animation.datatype]: %s: exiting ...\n", __PRETTY_FUNCTION__));

    return;

    AROS_USERFUNC_EXIT
}