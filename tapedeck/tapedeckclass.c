/*
    Copyright © 2016, The AROS Development Team. All rights reserved.
    $Id$
*/

/***********************************************************************************/

#define USE_BOOPSI_STUBS
#include <exec/libraries.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/graphics.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <devices/inputevent.h>
#include <gadgets/aroscycle.h>
#include <proto/alib.h>

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

#include "tapedeck_intern.h"

/***********************************************************************************/

#define IM(o) ((struct Image *)(o))
#define EG(o) ((struct Gadget *)(o))

#include <clib/boopsistubs.h>

/***********************************************************************************/

Object *TapeDeck__OM_NEW(Class *cl, Class *rootcl, struct opSet *msg)
{
    struct TapeDeckData 	*data;
    struct TextAttr 	*tattr;
    Object  	    	*o;

    struct TagItem  	frametags[] = {
        { IA_Width	, 200 		},
        { IA_Height	, 16 		},
        { IA_EdgesOnly	, FALSE		},
	{ IA_FrameType	, FRAME_BUTTON	},
        { TAG_DONE	, 0UL 		}
    };
    
    o = (Object *)DoSuperMethodA(cl, (Object *)rootcl, (Msg)msg);
    if (!o)
        return NULL;

    data = INST_DATA(cl, o);

#if (0)
    tattr = (struct TextAttr *)GetTagData(GA_TextAttr, (IPTR) NULL, msg->ops_AttrList);
    if (tattr) data->font = OpenFont(tattr);
#endif

    EG(o)->GadgetRender = NewObjectA(NULL, FRAMEICLASS, frametags);
    if (!EG(o)->GadgetRender)
    {
        IPTR methodid = OM_DISPOSE;
        CoerceMethodA(cl, o, (Msg)&methodid);
        return NULL;
    }

    return o;
}

/***********************************************************************************/

VOID TapeDeck__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct TapeDeckData *data = INST_DATA(cl, o);
    
    if (EG(o)->GadgetRender)
        DisposeObject(EG(o)->GadgetRender);

    DoSuperMethodA(cl,o,msg);
}

/***********************************************************************************/

IPTR TapeDeck__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct TapeDeckData *data = INST_DATA(cl, o);
    IPTR    	     retval = 1;

    switch(msg->opg_AttrID)
    {
	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
    }
    
    return retval;
}

/***********************************************************************************/

IPTR TapeDeck__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    struct TapeDeckData 	 *data = INST_DATA(cl, o);
    struct TagItem       *tag, *taglist = msg->ops_AttrList;
    STRPTR  	    	 *mylabels;
    BOOL    	    	  rerender = FALSE;
    IPTR    	    	  result;

    result = DoSuperMethodA(cl, o, (Msg)msg);

    while((tag = NextTagItem(&taglist)))
    {
        switch(tag->ti_Tag)
        {
	    case GA_Disabled:
		rerender = TRUE;
		break;
        }
    }

    /* SDuvan: Removed test (cl == OCLASS(o)) */

    if(rerender)
    {
        struct RastPort *rport;

        rport = ObtainGIRPort(msg->ops_GInfo);
        if(rport)
        {
            DoMethod(o, GM_RENDER, (IPTR)msg->ops_GInfo, (IPTR)rport, GREDRAW_UPDATE);
            ReleaseGIRPort(rport);
            result = FALSE;
        }
    }

    return result;
}

/***********************************************************************************/

VOID TapeDeck__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct TapeDeckData *data = INST_DATA(cl, o);

    /* Full redraw: clear and draw border */
    DrawImageState(msg->gpr_RPort,IM(EG(o)->GadgetRender),
                   EG(o)->LeftEdge, EG(o)->TopEdge,
                   EG(o)->Flags&GFLG_SELECTED?IDS_SELECTED:IDS_NORMAL,
                   msg->gpr_GInfo->gi_DrInfo);
}

/***********************************************************************************/

IPTR TapeDeck__GM_GOACTIVE(Class *cl, Object *o, struct gpInput *msg)
{	
    struct RastPort 	*rport;
    IPTR    	    	retval;
    
    EG(o)->Flags |= GFLG_SELECTED;
    
    rport = ObtainGIRPort(msg->gpi_GInfo);
    if (rport)
    {
        struct gpRender rmsg =
            { GM_RENDER, msg->gpi_GInfo, rport, GREDRAW_UPDATE }, *p_rmsg = &rmsg;
        DoMethodA(o, (Msg)p_rmsg);
        ReleaseGIRPort(rport);
        retval = GMR_MEACTIVE;
    } else
        retval = GMR_NOREUSE;
   
    return retval;
}

/***********************************************************************************/

IPTR TapeDeck__GM_HANDLEINPUT(Class *cl, Object *o, struct gpInput *msg)
{
    struct RastPort 	*rport;
    struct TapeDeckData 	*data;
    IPTR    	    	retval = GMR_MEACTIVE;

    data = INST_DATA(cl, o);
    
    if (msg->gpi_IEvent->ie_Class == IECLASS_RAWMOUSE)
    {
        if (msg->gpi_IEvent->ie_Code == SELECTUP)
        {
            if (G(o)->Flags & GFLG_SELECTED)
            {
                *msg->gpi_Termination = 1;
                retval = GMR_NOREUSE | GMR_VERIFY;
            } else
                /* mouse is not over gadget */
                retval = GMR_NOREUSE;
		
/*
            G(o)->Flags &= ~GFLG_SELECTED;
	    
            rport = ObtainGIRPort(msg->gpi_GInfo);
            if (rport)
            {
        	struct gpRender rmsg =
                    { GM_RENDER, msg->gpi_GInfo, rport, GREDRAW_UPDATE };
        	DoMethodA(o, (Msg)&rmsg);
        	ReleaseGIRPort(rport);
            }*/

        } else if (msg->gpi_IEvent->ie_Code == IECODE_NOBUTTON)
        {
            struct gpHitTest htmsg =
                { GM_HITTEST, msg->gpi_GInfo,
                  { msg->gpi_Mouse.X, msg->gpi_Mouse.Y },
                }, *p_htmsg = &htmsg;
            if (DoMethodA(o, (Msg)p_htmsg) != GMR_GADGETHIT)
            {
                if (EG(o)->Flags & GFLG_SELECTED)
                {
                    G(o)->Flags &= ~GFLG_SELECTED;
                    rport = ObtainGIRPort(msg->gpi_GInfo);
                    if (rport)
                    {
                        struct gpRender rmsg =
                            { GM_RENDER, msg->gpi_GInfo, rport, GREDRAW_UPDATE }, *p_rmsg = &rmsg;
                        DoMethodA(o, (Msg)p_rmsg);
                        ReleaseGIRPort(rport);
                    }
                }
            } else
            {
                if (!(EG(o)->Flags & GFLG_SELECTED))
                {
                    EG(o)->Flags |= GFLG_SELECTED;
                    rport = ObtainGIRPort(msg->gpi_GInfo);
                    if (rport)
                    {
                        struct gpRender rmsg =
                            { GM_RENDER, msg->gpi_GInfo, rport, GREDRAW_UPDATE }, *p_rmsg = &rmsg;
                        DoMethodA(o, (Msg)p_rmsg);
                        ReleaseGIRPort(rport);
                    }
                }
            }
        } else if (msg->gpi_IEvent->ie_Code == MENUDOWN)
            retval = GMR_REUSE;
    }
    return retval;
}

/***********************************************************************************/

IPTR TapeDeck__GM_GOINACTIVE(Class *cl, Object *o, struct gpGoInactive *msg)
{
    struct RastPort *rport;

    EG(o)->Flags &= ~GFLG_SELECTED;

    rport = ObtainGIRPort(msg->gpgi_GInfo);
    if (rport)
    {
        struct gpRender rmsg = { GM_RENDER, msg->gpgi_GInfo, rport, GREDRAW_UPDATE }, *p_rmsg = &rmsg;
	
        DoMethodA(o, (Msg)p_rmsg);
        ReleaseGIRPort(rport);
    }
    
    return 0;
}
