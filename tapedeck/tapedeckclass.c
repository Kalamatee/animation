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
#   define DEBUG 1
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

    D(bug("[tapedeck.gadget]: %s()\n", __PRETTY_FUNCTION__));

    o = (Object *)DoSuperMethodA(cl, (Object *)rootcl, (Msg)msg);
    if (!o)
        return NULL;

    data = INST_DATA(cl, o);

#if (0)
    tattr = (struct TextAttr *)GetTagData(GA_TextAttr, (IPTR) NULL, msg->ops_AttrList);
    if (tattr) data->font = OpenFont(tattr);
#endif

    return o;
}

/***********************************************************************************/

VOID TapeDeck__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct TapeDeckData *data = INST_DATA(cl, o);

    D(bug("[tapedeck.gadget]: %s()\n", __PRETTY_FUNCTION__));

    if (EG(o)->GadgetRender)
        DisposeObject(EG(o)->GadgetRender);

    DoSuperMethodA(cl,o,msg);
}

/***********************************************************************************/

IPTR TapeDeck__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct TapeDeckData *data = INST_DATA(cl, o);
    IPTR    	     retval = 1;

    D(bug("[tapedeck.gadget]: %s()\n", __PRETTY_FUNCTION__));

    switch(msg->opg_AttrID)
    {
        case GA_Height:
            *msg->opg_Storage = 20;
            break;

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

    D(bug("[tapedeck.gadget]: %s()\n", __PRETTY_FUNCTION__));

    result = DoSuperMethodA(cl, o, (Msg)msg);

    while((tag = NextTagItem(&taglist)))
    {
        switch(tag->ti_Tag)
        {
	    case GA_Disabled:
		rerender = TRUE;
		break;

            D(
            case GA_Left:
                bug("[tapedeck.gadget] %s: GA_Left - %d\n", __PRETTY_FUNCTION__, tag->ti_Data);
                break;

            case GA_Top:
                bug("[tapedeck.gadget] %s: GA_Top - %d\n", __PRETTY_FUNCTION__, tag->ti_Data);
                break;

            case GA_Width:
                bug("[tapedeck.gadget] %s: GA_Width - %d\n", __PRETTY_FUNCTION__, tag->ti_Data);
                break;

            case GA_Height:
                bug("[tapedeck.gadget] %s: GA_Height - %d\n", __PRETTY_FUNCTION__, tag->ti_Data);
                break;
            )
        }
    }

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

IPTR TapeDeck__GM_LAYOUT(Class *cl, Object *o, struct gpLayout *msg)
{
    struct TagItem  	frametags[] = {
        { IA_Width	, 0 		},
        { IA_Height	, 0 		},
        { IA_EdgesOnly	, FALSE		},
	{ IA_FrameType	, FRAME_BUTTON	},
        { TAG_DONE	, 0UL 		}
    };

    D(bug("[tapedeck.gadget]: %s()\n", __PRETTY_FUNCTION__));

    GetAttr(GA_Width, o, (IPTR *)&frametags[0].ti_Data);
    GetAttr(GA_Height, o, (IPTR *)&frametags[1].ti_Data);
    
    EG(o)->GadgetRender = NewObjectA(NULL, FRAMEICLASS, frametags);

    return 1;
}

IPTR TapeDeck__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct TapeDeckData *data = INST_DATA(cl, o);
    LONG rend_x, rend_y;

    D(bug("[tapedeck.gadget]: %s()\n", __PRETTY_FUNCTION__));

    if (EG(o)->GadgetRender)
    {
        /* Full redraw: clear and draw border */
        DrawImageState(msg->gpr_RPort, IM(EG(o)->GadgetRender),
                       EG(o)->LeftEdge, EG(o)->TopEdge,
                       EG(o)->Flags&GFLG_SELECTED?IDS_SELECTED:IDS_NORMAL,
                       msg->gpr_GInfo->gi_DrInfo);
    }

    SetAPen(msg->gpr_RPort, msg->gpr_GInfo->gi_DrInfo->dri_Pens[SHADOWPEN]);
  
    rend_x = EG(o)->Width >> 1;

    {
        /* Play */

        rend_y = EG(o)->TopEdge + ((EG(o)->Height - 9) >> 1);

        RectFill(msg->gpr_RPort, EG(o)->LeftEdge + rend_x, rend_y , EG(o)->LeftEdge + rend_x + 1, rend_y + 10);
        RectFill(msg->gpr_RPort, EG(o)->LeftEdge + rend_x + 2, rend_y + 1 , EG(o)->LeftEdge + rend_x + 3, rend_y + 9);
        RectFill(msg->gpr_RPort, EG(o)->LeftEdge + rend_x + 4, rend_y + 2 , EG(o)->LeftEdge + rend_x + 5, rend_y + 8);
        RectFill(msg->gpr_RPort, EG(o)->LeftEdge + rend_x + 6, rend_y + 3 , EG(o)->LeftEdge + rend_x + 7, rend_y + 7);
        RectFill(msg->gpr_RPort, EG(o)->LeftEdge + rend_x + 8, rend_y + 4 , EG(o)->LeftEdge + rend_x + 9, rend_y + 6);
        RectFill(msg->gpr_RPort, EG(o)->LeftEdge + rend_x + 10, rend_y + 5 , EG(o)->LeftEdge + rend_x + 11, rend_y + 5);
    }

    {
        /* volume control */

        rend_x = EG(o)->LeftEdge + EG(o)->Width - 17;
        rend_y = EG(o)->TopEdge + ((EG(o)->Height - 9) >> 1);

        RectFill(msg->gpr_RPort, rend_x + 1, rend_y + 4, rend_x + 4, rend_y + 6);
        RectFill(msg->gpr_RPort, rend_x + 5, rend_y + 3, rend_x + 5, rend_y + 7);
        RectFill(msg->gpr_RPort, rend_x + 6, rend_y + 1, rend_x + 6, rend_y + 9);
        RectFill(msg->gpr_RPort, rend_x + 13, rend_y,     rend_x + 13, rend_y + 1);
        RectFill(msg->gpr_RPort, rend_x + 14, rend_y + 2, rend_x + 14, rend_y + 8);
        RectFill(msg->gpr_RPort, rend_x + 13, rend_y + 9, rend_x + 13, rend_y + 10);
        RectFill(msg->gpr_RPort, rend_x + 11, rend_y + 2, rend_x + 11, rend_y + 3);
        RectFill(msg->gpr_RPort, rend_x + 12, rend_y + 4, rend_x + 12, rend_y + 6);
        RectFill(msg->gpr_RPort, rend_x + 11, rend_y + 7, rend_x + 11, rend_y + 8);
        RectFill(msg->gpr_RPort, rend_x + 9,  rend_y + 4, rend_x + 9,  rend_y + 6);
        RectFill(msg->gpr_RPort, rend_x + 10, rend_y + 5, rend_x + 10, rend_y + 5);
    }

    return 1;
}

/***********************************************************************************/

IPTR TapeDeck__GM_GOACTIVE(Class *cl, Object *o, struct gpInput *msg)
{	
    struct RastPort 	*rport;
    IPTR    	    	retval;

    D(bug("[tapedeck.gadget]: %s()\n", __PRETTY_FUNCTION__));

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

    D(bug("[tapedeck.gadget]: %s()\n", __PRETTY_FUNCTION__));

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

    D(bug("[tapedeck.gadget]: %s()\n", __PRETTY_FUNCTION__));

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
