/*
    Copyright © 2015-2016, The AROS Development	Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/realtime.h>

#include <libraries/realtime.h>

AROS_UFH3(ULONG, playerHookFunc, 
    AROS_UFHA(struct    Hook *,     hook, A0), 
    AROS_UFHA(struct Player *, obj, A2), 
    AROS_UFHA(struct pmTime *, msg, A1))
{ 
    AROS_USERFUNC_INIT
 
//    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));

    switch (msg->pmt_Method)
    {
	case PM_TICK:
	    break;

	case PM_SHUTTLE:
	    break;

	case PM_STATE:
	    break;
    }

    return (0);

    AROS_USERFUNC_EXIT
}
