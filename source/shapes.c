#include "shapes.h"
#include "gfxutils.h"
#include "baseSDL.h"
#include "list.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#define STRIPSIZE 5
#define BORDERSIZE 4

Rectangle_t *RectangleCreate(SDL_Rect pos, SDL_Color color, int flood){
    Rectangle_t *out = malloc(sizeof(Rectangle_t));
    out->color = color;
    out->pos = pos;
    out->flood = flood;

    return out;
}

void RectangleFree(Rectangle_t *Rectangle){
    free(Rectangle);
}

RectangleOutline_t *RectangleOutlineCreate(SDL_Rect pos, SDL_Color color, int flood, int borderSize){
    RectangleOutline_t *out = malloc(sizeof(RectangleOutline_t));
    out->rect.color = color;
    out->rect.pos = pos;
    out->rect.flood = flood;
    out->borderSize = borderSize;

    return out;
}

void RectangleOutlineFree(RectangleOutline_t *Rectangle){
    free(Rectangle);
}

Image_t *ImageCreate(SDL_Texture *tex, SDL_Rect pos){
    Image_t *out = malloc(sizeof(Image_t));
    out->pos = pos;
    out->texture = tex;

    return out;
}

void ImageFree(Image_t *img){
    SDL_DestroyTexture(img->texture);
    free(img);
}

Text_t *TextCreate(int x, int y, char *text, SDL_Color color, TTF_Font *font){
    Text_t *out = malloc(sizeof(Text_t));
    out->x = x;
    out->y = y;
    out->color = color;
    out->text = CopyTextUtil(text);
    out->font = font;

    return out;
}

void TextFree(Text_t *text){
    free(text->text);
    free(text);
}

TextCentered_t *TextCenteredCreate(SDL_Rect pos, char *text, SDL_Color color, TTF_Font *font){
    TextCentered_t *out = malloc(sizeof(TextCentered_t));
    out->text.x = pos.x;
    out->text.y = pos.y;
    out->text.color = color;
    out->text.text = CopyTextUtil(text);
    out->text.font = font;
    out->w = pos.w;
    out->h = pos.h;

    return out;
}

void TextCenteredFree(TextCentered_t *text){
    free(text->text.text);
    free(text);
}

Button_t *ButtonCreate(SDL_Rect pos, SDL_Color primary, SDL_Color secondary, SDL_Color textcolor, SDL_Color highlight, u8 options, u8 style, char *text, TTF_Font *font, func_ptr function){
    Button_t *out = malloc(sizeof(Button_t));
    out->pos = pos;
    out->primary = primary;
    out->secondary = secondary;
    out->textcolor = textcolor;
    out->options = options;
    out->style = style;
    out->text = CopyTextUtil(text);
    out->function = function;
    out->highlight = highlight;
    out->font = font;

    return out;
}

void ButtonFree(Button_t *btn){
    free(btn->text);
    free(btn);
}

void DrawButton(Button_t *button){
    SDL_Color backcolor = (button->options & BUTTON_PRESSED) ? button->secondary : ((button->options & BUTTON_HIGHLIGHT) ? button->highlight : button->primary);
    Rectangle_t *rect = RectangleCreate(button->pos, backcolor, 1);
    TextCentered_t *text = TextCenteredCreate(button->pos, button->text, button->textcolor, button->font);

    DrawRectSDL(rect);
    DrawCenteredTextSDL(text);

    switch (button->style){
        case ButtonStyleBorder:;
            RectangleOutline_t outline = {*rect, BORDERSIZE};
            outline.rect.color = button->secondary;
            DrawRectOutlineSDL(&outline);
            break;
        case ButtonStyleBottomStrip:;
            Rectangle_t *strip = RectangleCreate(POS(button->pos.x, button->pos.y + button->pos.h - STRIPSIZE, button->pos.w, STRIPSIZE), button->secondary, 1);
            DrawRectSDL(strip);
            RectangleFree(strip);
            break;
        case ButtonStyleTopStrip:;
            Rectangle_t *stripTop = RectangleCreate(POS(button->pos.x, button->pos.y, button->pos.w, STRIPSIZE), button->secondary, 1);
            DrawRectSDL(stripTop);
            RectangleFree(stripTop);
            break;
    }   

    if (button->options & BUTTON_DISABLED){
        rect->color = COLOR(0, 0, 0, 75);
        DrawRectSDL(rect);
    }

    RectangleFree(rect);
    TextCenteredFree(text);
}

ListItem_t *ListItemCreate(SDL_Color LColor, SDL_Color RColor, SDL_Texture *LImg, char *LText, char *RText){
    ListItem_t *out = calloc(1, sizeof(ListItem_t));
    out->leftColor = LColor;
    out->rightColor = RColor;
    out->leftImg = LImg;
    
    if (LText != NULL)
        out->leftText = CopyTextUtil(LText);
    
    if (RText != NULL)
        out->rightText = CopyTextUtil(RText);

    return out;
}

void ListItemFree(ListItem_t *li){
    if (li->leftText != NULL)
        free(li->leftText);
    
    if (li->rightText != NULL)
        free(li->rightText);

    free(li);
}

ListView_t *ListViewCreate(SDL_Rect pos, int entrySize, SDL_Color primary, SDL_Color selected, SDL_Color pressed, u8 options, ShapeLinker_t *textList, func_ptr function, func_ptr selectionChanged, TTF_Font *font){
    ListView_t *out = malloc(sizeof(ListView_t));
    out->primary = primary;
    out->selected = selected;
    out->pressed = pressed;
    out->options = options;
    out->highlight = 0;
    out->offset = 0;
    out->text = textList;
    out->function = function;
    out->font = font;
    out->entrySize = entrySize;
    out->pos = pos;
    out->changeSelection = selectionChanged;

    return out;
}

void ListViewFree(ListView_t *lv){
    ShapeLinkDispose(&lv->text);
    free(lv);
}

void DrawListView(ListView_t *listview){
    int listLen = ShapeLinkCount(listview->text);
    if (listview->options & LIST_AUTO){
        listview->highlight = listLen - 1;
    }

    if (listview->highlight >= 0){
        int minView, maxView, minFrame, maxFrame;
        minFrame = listview->offset;
        maxFrame = minFrame + listview->pos.h;
        minView = listview->entrySize * listview->highlight;
        maxView = minView + listview->entrySize;

        if (minFrame > minView){
            listview->offset -= minFrame - minView;
        }
        if (maxFrame < maxView){
            listview->offset += maxView - maxFrame;
        }

        //minFrame = listview->offset, maxFrame = minFrame + listview->pos.h;
    }

    if (listview->options & LIST_AUTO)
        listview->highlight = -1;

    int currentListOffset = listview->offset / listview->entrySize, currentListOffsetRemainder = listview->offset % listview->entrySize;
    int curPixOffset = currentListOffsetRemainder * -1;

    Rectangle_t bg = {listview->pos, listview->primary, 1};
    DrawRectSDL(&bg);

    SetClipRectSDL(&listview->pos);
    ShapeLinker_t *link = ShapeLinkOffset(listview->text, currentListOffset);

    int roffset = 5;

    if (listview->entrySize * listLen > listview->pos.h){
        Rectangle_t scrollBg = {POS(listview->pos.x + listview->pos.w - 50, listview->pos.y, 50, listview->pos.h), listview->selected, 1};

        float percentOnScreen =  (float)listview->pos.h / (listview->entrySize * listLen);
        float sizePerPixel =  ((float)listview->pos.h - listview->pos.h * percentOnScreen) / (float)(listLen * listview->entrySize - listview->pos.h);

        Rectangle_t scrollBar = {POS(listview->pos.x + listview->pos.w - 50, listview->pos.y + (sizePerPixel * listview->offset), 50, (listview->pos.h * percentOnScreen)), listview->pressed, 1};

        DrawRectSDL(&scrollBg);
        DrawRectSDL(&scrollBar);
        roffset += 50;
    }

    for (; link != NULL && curPixOffset < listview->pos.h; link = link->next){
        ListItem_t *item = link->item;
        int xoffset = 5;

        if (currentListOffset == listview->highlight && listview->options & (LIST_SELECTED | LIST_ALWAYSRENDERSELECTED)){
            SDL_Color color = (listview->options & LIST_PRESSED) ? listview->pressed : listview->selected;

            if (listview->options & LIST_ALWAYSRENDERSELECTED && !(listview->options & LIST_SELECTED)){
                color.a = 100;
            }

            Rectangle_t high = {POS(listview->pos.x, listview->pos.y + curPixOffset, listview->pos.w - roffset + 5, listview->entrySize), color, 1};
            DrawRectSDL(&high);
        }
        
        if (item->leftImg != NULL){
            int w, h;
            SDL_QueryTexture(item->leftImg, NULL, NULL, &w, &h);

            Image_t img = {item->leftImg, POS(listview->pos.x + xoffset, listview->pos.y + curPixOffset + ((listview->entrySize - h) / 2), w, h)};
            DrawImageSDL(&img);
            
            xoffset += xoffset + w;
        }

        if (item->leftText != NULL){
            SizeInfo_t LInfo = GetTextSizeSDL(listview->font, item->leftText);
            int xPos = listview->pos.x + xoffset;

            if (listview->options & LIST_CENTERLEFT){
                SizeInfo_t info = GetTextSizeSDL(listview->font, item->leftText);
                xPos = listview->pos.x + xoffset + (listview->pos.w - xoffset - info.w) / 2;
            }

            Text_t LText = {xPos, listview->pos.y + curPixOffset + ((listview->entrySize - LInfo.h) / 2), item->leftText, item->leftColor, listview->font};
            DrawTextSDL(&LText);
        }

        if (item->rightText != NULL){
            SizeInfo_t RInfo = GetTextSizeSDL(listview->font, item->rightText);
            Text_t RText = {listview->pos.x + listview->pos.w - roffset - RInfo.w, listview->pos.y + curPixOffset + ((listview->entrySize - RInfo.h) / 2), item->rightText, item->rightColor, listview->font};
            DrawTextSDL(&RText);
        }

        curPixOffset += listview->entrySize;
        currentListOffset++;
    }

    SetClipRectSDL(NULL);
}

ProgressBar_t *ProgressBarCreate(SDL_Rect pos, SDL_Color primary, SDL_Color secondary, u8 style, u8 percentage){
    ProgressBar_t *out = malloc(sizeof(ProgressBar_t));
    out->pos = pos;
    out->primary = primary;
    out->secondary = secondary;
    out->style = style;
    out->percentage = percentage;

    return out;
}

void ProgressBarFree(ProgressBar_t *pb){
    free(pb);
}

void DrawProgressBar(ProgressBar_t *bar){
    int wfilled = bar->pos.w * bar->percentage / 100, wempty = bar->pos.w - wfilled;
    SDL_Rect filled = POS(bar->pos.x, bar->pos.y, wfilled, bar->pos.h);
    SDL_Rect empty = POS(bar->pos.x + wfilled, bar->pos.y, wempty, bar->pos.h);
    Rectangle_t filledRect = {filled, bar->primary, 1};
    Rectangle_t emptyRect = {empty, bar->secondary, 1};
    RectangleOutline_t emptyRectOutline = {{bar->pos, bar->secondary, 1}, BORDERSIZE};

    switch (bar->style){
        case ProgressBarStyleFlat:;
            DrawRectSDL(&filledRect);
            DrawRectSDL(&emptyRect);
            break;
        case ProgressBarStyleBorder:;
            DrawRectOutlineSDL(&emptyRectOutline);
            DrawRectSDL(&filledRect);
            break;
        case ProgressBarStyleCompleteBorder:;
            DrawRectSDL(&filledRect);
            DrawRectOutlineSDL(&emptyRectOutline);
            break;
        case ProgressBarStyleSize:;
            emptyRect.pos.y += emptyRect.pos.h - STRIPSIZE;
            emptyRect.pos.h = STRIPSIZE;
            DrawRectSDL(&filledRect);
            DrawRectSDL(&emptyRect);
            break;
    }
}

Glyph_t *GlyphCreate(int x, int y, u16 glyph, SDL_Color color, TTF_Font *font){
    Glyph_t *out = malloc(sizeof(Glyph_t));
    out->x = x;
    out->y = y;
    out->glyph = glyph;
    out->color = color;
    out->font = font;

    return out;
}

void GlyphFree(Glyph_t *glyph){
    free(glyph);
}