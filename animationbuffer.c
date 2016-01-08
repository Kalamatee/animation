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
    ULONG signal;

    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));

    if (priv)
    {
        D(bug("[animation.datatype] %s: private data @ 0x%p\n", __PRETTY_FUNCTION__, priv));
        D(bug("[animation.datatype] %s: dt obj @ 0x%p, instance data @ 0x%p\n", __PRETTY_FUNCTION__, priv->pp_Object, priv->pp_Data));

        if ((priv->pp_Data->ad_LoadFrames = AllocSignal(-1)) != 0)
        {
            D(bug("[animation.datatype]: %s: allocated load signal (%x)\n", __PRETTY_FUNCTION__, priv->pp_Data->ad_LoadFrames));
            while (TRUE)
            {
                signal = priv->pp_Data->ad_PlayerTick | SIGBREAKF_CTRL_C;
                signal = Wait(signal);
                D(bug("[animation.datatype]: %s: signalled (%08x)\n", __PRETTY_FUNCTION__, signal));

                if (signal & SIGBREAKF_CTRL_C)
                    break;

                if (signal & priv->pp_Data->ad_LoadFrames)
                {
                    D(bug("[animation.datatype]: %s: Loading Frames...\n", __PRETTY_FUNCTION__));
                }
            }
        }
        priv->pp_Data->ad_BufferProc = NULL;
    }
    D(bug("[animation.datatype]: %s: exiting ...\n", __PRETTY_FUNCTION__));

    return;

    AROS_USERFUNC_EXIT
}