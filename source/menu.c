#include "menu.h"
#include "baseSDL.h"
#include "list.h"
#include "shapes.h"
#include "gfxutils.h"
#include "renderer.h"
#include <switch.h>
#include <math.h>

enum {
    DirectionUp = 1,
    DirectionRight,
    DirectionDown,
    DirectionLeft
};

PadState pad;

void InitHid(){
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);
    hidInitializeTouchScreen();
}

int _JumpToClosestBox(ShapeLinker_t *list, int direction, ShapeLinker_t *src, int currentSelectionIndex){
    int closestSelection = -1, currentScore = 5000, offset = 0;
    int srcx = 0, srcy = 0, dstx = 0, dsty = 0, weight;

    // Change this later to support more than just buttons
    if (src->type == ButtonType){
        Button_t *srcbtn = CAST(Button_t, src->item);
        srcx = srcbtn->pos.x + srcbtn->pos.w / 2;
        srcy = srcbtn->pos.y + srcbtn->pos.h / 2;
    }
    else if (src->type == ListViewType){
        ListView_t *srclv = CAST(ListView_t, src->item);
        srcx = srclv->pos.x + srclv->pos.w / 2;
        srcy = srclv->pos.y + srclv->pos.h / 2;
    }
    else if (src->type == ListGridType){
        ListGrid_t *srcgv = src->item;
        srcx = srcgv->pos.x + srcgv->pos.w / 2;
        srcy = srcgv->pos.y + srcgv->pos.h / 2;
    }
    else {
        return -1;
    }

    for (ShapeLinker_t *iter = list; iter != NULL; iter = iter->next, offset++){
        if (iter->type < ListViewType)
            continue;

        if (currentSelectionIndex == offset)
            continue;

        // Change this later to support more than just buttons
        if (iter->type == ButtonType){
            Button_t *dstbtn = CAST(Button_t, iter->item);
            dstx = dstbtn->pos.x + dstbtn->pos.w / 2;
            dsty = dstbtn->pos.y + dstbtn->pos.h / 2;

            if (dstbtn->options & (BUTTON_DISABLED | BUTTON_NOJOYSEL))
                continue;
        }
        else if (iter->type == ListViewType){
            ListView_t *dstlv = CAST(ListView_t, iter->item);
            dstx = dstlv->pos.x + dstlv->pos.w / 2;
            dsty = dstlv->pos.y + dstlv->pos.h / 2;

            if (dstlv-> options & (LIST_DISABLED | LIST_AUTO))
                continue;
        }
        else if (iter->type == ListGridType){
            ListGrid_t *dstgv = iter->item;
            dstx = dstgv->pos.x + dstgv->pos.w / 2;
            dsty = dstgv->pos.y + dstgv->pos.h / 2;

            if (dstgv-> options & (LIST_DISABLED | LIST_AUTO))
                continue;
        }
        else {
            return -1;
        }
        weight = 0;
                
        if ((direction == DirectionDown && srcy < dsty) || (direction == DirectionUp && srcy > dsty)){
            weight += abs(srcy - dsty);
            weight += abs(srcx - dstx) * 5;
        }
        else if ((direction == DirectionLeft && srcx > dstx) || (direction == DirectionRight && srcx < dstx)){
            weight += abs(srcy - dsty) * 5;
            weight += abs(srcx - dstx);
        }

        if (weight < currentScore && weight){
            currentScore = weight;
            closestSelection = offset;
        }
                
    }

    return closestSelection;
}

int CheckTouchCollision(ShapeLinker_t *list){
    int offset = 0, touchX, touchY;
    HidTouchScreenState screenState = {0};

    if (!hidGetTouchScreenStates(&screenState, 1)){
        return -1;
    }

    if (screenState.count <= 0)
        return -1;

    touchX = screenState.touches[0].x; //+ (touch.dx / 2);
    touchY = screenState.touches[0].y; //+ (touch.dy / 2);

    for (ShapeLinker_t *iter = list; iter != NULL; iter = iter->next, offset++){
        if (iter->type < ListViewType)
            continue;

        switch (iter->type){
            case ButtonType:;
                Button_t *btn = iter->item;
                if (CheckTouchCollisionButton(btn, touchX, touchY))
                    return offset;
                break;
            case ListViewType:;
                ListView_t *lv = iter->item;
                if (CheckTouchCollisionListView(lv, touchX, touchY))
                    return offset;
                break;
            case ListGridType:;
                ListGrid_t *gv = iter->item;
                if (CheckTouchCollisionListGrid(gv, touchX, touchY))
                    return offset;
                break;
        }
    }

    return -1;
}

int RunSelection(Context_t *ctx){
    if (ctx->selected->type == ButtonType){
        Button_t *btn = ctx->selected->item;
        SETBIT(btn->options, BUTTON_PRESSED, 0);
        if (btn->function != NULL)
            return btn->function(ctx);
    }
    else if (ctx->selected->type == ListViewType){
        ListView_t *lv = ctx->selected->item;
        SETBIT(lv->options, LIST_PRESSED, 0);
        if (lv->function != NULL)
            return lv->function(ctx);
    }
    else if (ctx->selected->type == ListGridType){
        ListGrid_t *gv = ctx->selected->item;
        SETBIT(gv->options, LIST_PRESSED, 0);
        if (gv->function != NULL)
            return gv->function(ctx);
    }

    return 0;
}

void ActivateSelection(Context_t *ctx){
    if (ctx->selected->type == ButtonType){
        ((Button_t*)ctx->selected->item)->options |= BUTTON_PRESSED;
    }
    else if (ctx->selected->type == ListViewType){
        ((ListView_t*)ctx->selected->item)->options |= LIST_PRESSED;
    }
    else if (ctx->selected->type == ListGridType){
        ((ListGrid_t*)ctx->selected->item)->options |= LIST_PRESSED;
    }
}

void SelectSelection(Context_t *ctx){
    if (ctx->selected->type == ButtonType){
        ((Button_t*)ctx->selected->item)->options |= BUTTON_HIGHLIGHT;
    }
    else if (ctx->selected->type == ListViewType){
        ((ListView_t*)ctx->selected->item)->options |= LIST_SELECTED;
    }
    else if (ctx->selected->type == ListGridType){
        ((ListGrid_t*)ctx->selected->item)->options |= LIST_SELECTED;
    }
}

int menuRun = 1;

Context_t MakeMenu(ShapeLinker_t *in, func_ptr buttonHandler, func_ptr runEveryFrame){
    int selectionMade = 0, touchSelection = -1, timer = 0, timeOfTimer = 21, hasScreenChanged;
    Context_t ctx = {0, 0, NULL, in, 0,0,0};
    
    for (ShapeLinker_t *iter = in; iter != NULL; iter = iter->next){
        if (iter->type >= ListViewType){
            ctx.selected = iter;
            break;
        }
        ctx.curOffset++;
    }

    if (ctx.selected == NULL)
        return (Context_t){0};
    
    SelectSelection(&ctx);

    while (menuRun){
        padUpdate(&pad);
        ctx.kDown = padGetButtonsDown(&pad);
        ctx.kUp = padGetButtonsUp(&pad);
        ctx.kHeld = padGetButtons(&pad);
        int touchSelTemp = CheckTouchCollision(ctx.all);
        if (touchSelTemp >= 0)
            touchSelection = touchSelTemp;

        hasScreenChanged = 1;
        
        if (timer > 0){
            timer--;
        }

        if (ctx.kDown & HidNpadButton_A){
            ActivateSelection(&ctx);
            selectionMade = 1;
        }

        else if (ctx.kUp & HidNpadButton_A && selectionMade){
            selectionMade = 0;
            ctx.origin = OriginFunction;

            if (RunSelection(&ctx) < 0)
                return ctx; 
        }

        else if (ctx.kDown & HidNpadButton_Plus){
            menuRun = 0;
            ctx.origin = OriginPlus;
            return ctx;
        }

        else if (touchSelection >= 0 && touchSelTemp < 0){
            if (ctx.selected->type == ButtonType){
                SETBIT(((Button_t*)ctx.selected->item)->options, BUTTON_HIGHLIGHT, 0);
            }
            else if (ctx.selected->type == ListViewType){
                SETBIT(((ListView_t*)ctx.selected->item)->options, LIST_SELECTED, 0);
            }

            ctx.curOffset = touchSelection;
            ctx.origin = OriginFunction;
            ctx.selected = ShapeLinkOffset(ctx.all, ctx.curOffset);
            touchSelection = -1;
            
            if (RunSelection(&ctx) < 0)
                return ctx; 
        }

        else if (ctx.kHeld & (HidNpadButton_AnyDown | HidNpadButton_AnyUp | HidNpadButton_AnyLeft | HidNpadButton_AnyRight)){            
            int direction = 0, res = -1;
            
            if (timer > 10 && ctx.kHeld & (HidNpadButton_StickRLeft | HidNpadButton_StickRUp | HidNpadButton_StickRRight | HidNpadButton_StickRDown))
                timer = 3;

            if (timer == 0){
                timer = timeOfTimer;
                timeOfTimer = 3;

                if (ctx.kHeld & HidNpadButton_AnyDown)
                    direction = DirectionDown;
                else if (ctx.kHeld & HidNpadButton_AnyUp)
                    direction = DirectionUp;
                else if (ctx.kHeld & HidNpadButton_AnyLeft)
                    direction = DirectionLeft;
                else if (ctx.kHeld & HidNpadButton_AnyRight)
                    direction = DirectionRight;

                if (ctx.selected->type == ListViewType){
                    ListView_t *lv = (ListView_t*)ctx.selected->item;

                    lv->options |= LIST_SELECTED;

                    int max = ShapeLinkCount(lv->text);
                    if (lv->highlight < max - 1 && direction == DirectionDown){
                        lv->highlight++;
                    }
                    else if (lv->highlight > 0 && direction == DirectionUp){
                        lv->highlight--;
                    }
                    else {
                        res = _JumpToClosestBox(ctx.all, direction, ctx.selected, ctx.curOffset);
                    }
                }
                else if (ctx.selected->type == ListGridType){
                    ListGrid_t *gv = ctx.selected->item;

                    gv->options |= LIST_SELECTED;

                    int count = ShapeLinkCount(gv->text);
                    int c = gv->fitOnX;
                    bool moveOut = false;

                    if (direction == DirectionRight){
                        if (gv->highlight < count - 1 && (gv->highlight % c != c - 1 || gv->options & GRID_NOSIDEESC))
                            gv->highlight++;
                        else if (gv->highlight == count - 1 && gv->highlight % c != c - 1)
                            gv->highlight -= c - 1;
                        else
                            moveOut = !(gv->options & GRID_NOSIDEESC);
                    }
                    else if (direction == DirectionLeft){
                        if (gv->highlight > 0 && (gv->highlight % c != 0 || gv->options & GRID_NOSIDEESC))
                            gv->highlight--;
                    }
                    else if (direction == DirectionDown){
                        if (gv->highlight < count - c)
                            gv->highlight += c;
                        else if (gv->highlight < count - count % c && count % c != 0)
                            gv->highlight = count - 1;
                        else
                            moveOut = true;
                    }
                        
                    else if (gv->highlight >= c && direction == DirectionUp)
                        gv->highlight -= c;
                    else 
                        moveOut = true;
                    
                    if (moveOut)
                        res = _JumpToClosestBox(ctx.all, direction, ctx.selected, ctx.curOffset);
                    
                }
                else {
                    ((Button_t*)ctx.selected->item)->options |= BUTTON_HIGHLIGHT;
                    res = _JumpToClosestBox(ctx.all, direction, ctx.selected, ctx.curOffset);
                }

                if (res >= 0){
                    selectionMade = 0;

                    if (ctx.selected->type == ButtonType){
                        SETBIT(((Button_t*)ctx.selected->item)->options, (BUTTON_HIGHLIGHT | BUTTON_PRESSED), 0);
                    }
                    else if (ctx.selected->type == ListViewType){
                        SETBIT(((ListView_t*)ctx.selected->item)->options, (LIST_PRESSED | LIST_SELECTED), 0);
                    }
                    else if (ctx.selected->type == ListGridType){
                        SETBIT(((ListGrid_t*)ctx.selected->item)->options, (LIST_PRESSED | LIST_SELECTED), 0);
                    }

                    ctx.curOffset = res;
                    ctx.selected = ShapeLinkOffset(ctx.all, ctx.curOffset);

                    SelectSelection(&ctx);
                }
            }
            else {
            	hasScreenChanged = 0;
            }
        }
        else if (ctx.kDown){
            if (buttonHandler != NULL){
                ctx.origin = OriginButtonPress;
                if (buttonHandler(&ctx) < 0)
                    return ctx;
            }
        }
        else if (!(ctx.kHeld & (HidNpadButton_AnyDown | HidNpadButton_AnyUp | HidNpadButton_AnyLeft | HidNpadButton_AnyRight))){
            timer = 0;
            timeOfTimer = 23;
        }
        else {
        	hasScreenChanged = 0;
        }

        if (ctx.selected->type == ListViewType){
            if (((ListView_t *)ctx.selected->item)->changeSelection != NULL){
                ctx.origin = OriginFunction;
                ((ListView_t *)ctx.selected->item)->changeSelection(&ctx);
            }
        }
        else if (ctx.selected->type == ListGridType){
            if (((ListGrid_t *)ctx.selected->item)->changeSelection != NULL){
                ctx.origin = OriginFunction;
                ((ListGrid_t *)ctx.selected->item)->changeSelection(&ctx);
            }
        }

        if (runEveryFrame != NULL){
            runEveryFrame(&ctx);
        }

        if (hasScreenChanged && menuRun)
        	RenderShapeLinkList(ctx.all);
        else if (menuRun)
        	svcSleepThread(16000000); // To stay consistent with vblank
    }

    ctx.origin = OriginPlus;
    return ctx;
}
