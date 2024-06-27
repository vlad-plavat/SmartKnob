#ifndef DRAW_FUNCTIONS_INCLUDED
#define DRAW_FUNCTIONS_INCLUDED

#include <stdint.h>

uint16_t lineBuffer[240];

void __not_in_flash_func(drawRectGradientH)(int16_t x, int16_t y, uint8_t h, uint8_t w, uint16_t color1, uint16_t color2){
    uint8_t red1 = color1>>11, red2 = color2>>11;
    uint8_t grn1 = (color1>>5)&0x3f, grn2 = (color2>>5)&0x3f;
    uint8_t blu1 = color1&0x1f, blu2 = color2&0x1f;
    for(int i=0; i<w; i++){
        uint8_t r = red1 + (red2-red1)*(i)/w;
        uint8_t g = grn1 + (grn2-grn1)*(i)/w;
        uint8_t b = blu1 + (blu2-blu1)*(i)/w;
        lineBuffer[i]= (r<<11) | (g<<5) | b;
    }
    h--;w--;//function calculates inclusive limits
    int16_t top = y<0?0:y;
    int16_t left = x<0?0:x;
    int16_t right = (x+w)>=WIDTH?WIDTH-1:(x+w);
    int16_t bottom = (y+h)>=HEIGHT?HEIGHT-1:(y+h);
    if(top>bottom) return;
    if(left>right) return;
    for(register int line = top; line<=bottom; line++){
        uint8_t maxcol = (WIDTH-1)-cuts[line];
        register uint16_t* lineptr = frame[line]-cuts[line];
        register int col = left<cuts[line]?cuts[line]:left;
        register int rgt = right>maxcol?maxcol:right;
        for(; col<=rgt; col++){
            lineptr[col] = lineBuffer[col-left];
        }
    }
}

void __not_in_flash_func(drawRectGradientV)(int16_t x, int16_t y, uint8_t h, uint8_t w, uint16_t color1, uint16_t color2){
    uint8_t red1 = color1>>11, red2 = color2>>11;
    uint8_t grn1 = (color1>>5)&0x3f, grn2 = (color2>>5)&0x3f;
    uint8_t blu1 = color1&0x1f, blu2 = color2&0x1f;
    for(int i=0; i<h; i++){
        uint8_t r = red1 + (red2-red1)*(i)/h;
        uint8_t g = grn1 + (grn2-grn1)*(i)/h;
        uint8_t b = blu1 + (blu2-blu1)*(i)/h;
        lineBuffer[i]= (r<<11) | (g<<5) | b;
    }
    h--;w--;//function calculates inclusive limits
    int16_t top = y<0?0:y;
    int16_t left = x<0?0:x;
    int16_t right = (x+w)>=WIDTH?WIDTH-1:(x+w);
    int16_t bottom = (y+h)>=HEIGHT?HEIGHT-1:(y+h);
    if(top>bottom) return;
    if(left>right) return;
    for(register int line = top; line<=bottom; line++){
        uint8_t maxcol = (WIDTH-1)-cuts[line];
        register uint16_t* lineptr = frame[line]-cuts[line];
        register int col = left<cuts[line]?cuts[line]:left;
        register int rgt = right>maxcol?maxcol:right;
        for(; col<=rgt; col++){
            lineptr[col] = lineBuffer[line-top];
        }
    }
}

void __not_in_flash_func(drawRectangle)(int16_t x, int16_t y, uint8_t h, uint8_t w, uint16_t color){
    if(h==0 || w==0) return;
    h--;w--;//function calculates inclusive limits
    int16_t top = y<0?0:y;
    int16_t left = x<0?0:x;
    int16_t right = (x+w)>=WIDTH?WIDTH-1:(x+w);
    int16_t bottom = (y+h)>=HEIGHT?HEIGHT-1:(y+h);
    if(top>bottom) return;
    if(left>right) return;
    for(register int line = top; line<=bottom; line++){
        uint8_t maxcol = (WIDTH-1)-cuts[line];
        register uint16_t* lineptr = frame[line]-cuts[line];
        register int col = left<cuts[line]?cuts[line]:left;
        register int rgt = right>maxcol?maxcol:right;
        for(; col<=rgt; col++){
            lineptr[col] = color;
        }
    }
}

void __not_in_flash_func(drawImage)(int16_t x, int16_t y, uint8_t h, uint8_t w, void *image){
    h--;w--;//function calculates inclusive limits
    int16_t top = y<0?0:y;
    int16_t left = x<0?0:x;
    int16_t right = (x+w)>=WIDTH?WIDTH-1:(x+w);
    int16_t bottom = (y+h)>=HEIGHT?HEIGHT-1:(y+h);
    if(top>=bottom) return;
    if(left>=right) return;
    register uint16_t* imgLineptr = image;
    imgLineptr += (w+1)*h-(top-y)*(w+1);
    for(register int line = top; line<=bottom; line++){
        dma_channel_set_read_addr(GC9A01_dma_buf, imgLineptr, false);
        dma_channel_set_write_addr(GC9A01_dma_buf, lineBuffer, false);
        dma_channel_set_trans_count(GC9A01_dma_buf, (w+1+1)/2, true);

        register uint8_t maxcol = (WIDTH-1)-cuts[line];
        register uint16_t* lineptr = frame[line]-cuts[line];
        register int col = left<cuts[line]?cuts[line]:left;
        register int rgt = right>maxcol?maxcol:right;
        register uint8_t imgCol=(col-x);
        while(dma_channel_is_busy(GC9A01_dma_buf));
        for(; col<=rgt; col++){
            if(lineBuffer[imgCol] != 0)
                lineptr[col] = lineBuffer[imgCol];
            imgCol++;
        }
        imgLineptr -= (w+1);
    }
}

//16.16 fixed point math
void __not_in_flash_func(drawRotatedImageOrLine)(int32_t x, int32_t y, int32_t h, int32_t w, int32_t angle, void *image, uint16_t color){
    double anglef = angle/(64.0*1024);
    int32_t horig = h, worig = w;
    uint16_t *img = image;
    int32_t sinof = sin(anglef*DEG2RAD)*(64.0*1024);
    int32_t cosof = cos(anglef*DEG2RAD)*(64.0*1024);
    w--; h--;
    x=x<<16; y=y<<16; w=w<<16; h=h<<16;
    int32_t x1=x, y1=y;
    //drawRectangle(FIX_TO_PX(x1),FIX_TO_PX(y1),3,3,0xffff);
    int32_t x2=((int64_t)w*cosof)>>16; x2+=x;
    int32_t y2=((int64_t)w*sinof)>>16; y2+=y;
    //drawRectangle(FIX_TO_PX(x2),FIX_TO_PX(y2),3,3,0xffff);
    int32_t x3=(((int64_t)w*cosof)>>16) - (((int64_t)h*sinof)>>16); x3+=x;
    int32_t y3=(((int64_t)w*sinof)>>16) + (((int64_t)h*cosof)>>16); y3+=y;
    //drawRectangle(FIX_TO_PX(x3),FIX_TO_PX(y3),3,3,0xffff);
    int32_t x4= - (((int64_t)h*sinof)>>16); x4+=x;
    int32_t y4=   (((int64_t)h*cosof)>>16); y4+=y;
    //drawRectangle(FIX_TO_PX(x4),FIX_TO_PX(y4),3,3,0xffff);

    int32_t xmin = MIN(MIN(x1,x2),MIN(x3,x4)), ymin = MIN(MIN(y1,y2),MIN(y3,y4));
    int32_t xmax = MAX(MAX(x1,x2),MAX(x3,x4)), ymax = MAX(MAX(y1,y2),MAX(y3,y4));
    
    //drawRectangle(FIX_TO_PX(xmin),FIX_TO_PX(ymin),3,3,0xff00);
    //drawRectangle(FIX_TO_PX(xmax),FIX_TO_PX(ymax),3,3,0xff00);

    int32_t lmax = FIX_TO_PX(ymax), colmax = FIX_TO_PX(xmax);
    int32_t lmin = FIX_TO_PX(ymin), colmin = FIX_TO_PX(xmin);

    int16_t top = lmin<0?0:lmin;
    int16_t left = colmin<0?0:colmin;
    int16_t right = colmax>WIDTH?WIDTH-1:colmax;
    int16_t bottom = lmax>HEIGHT?HEIGHT-1:lmax;
    if(top>=bottom) return;
    if(left>=right) return;

    for(int32_t lin = top; lin<=bottom; lin++){
        register uint16_t* lineptr = frame[lin]-cuts[lin];
        int32_t trimmed_left = left<cuts[lin]?cuts[lin]:left;
        int32_t trimmed_right = right>((WIDTH-1)-cuts[lin])?(WIDTH-1)-cuts[lin]:right;
        
        int32_t colorig =  ((((int64_t)(trimmed_left<<16)-x)*cosof)>>16) + ((((int64_t)((lin)<<16)-y)*sinof)>>16);
        int32_t linorig =  ((((int64_t)(trimmed_left<<16)-x)*sinof)>>16) - ((((int64_t)((lin)<<16)-y)*cosof)>>16) + (horig<<16);
        if(image != NULL){
            for(int32_t col = trimmed_left; col<=trimmed_right; col++){
                uint32_t linorig2 = (linorig>>16), colorig2 = (colorig>>16);
                linorig += sinof;
                colorig += cosof;

                if(linorig2 >= (uint32_t)(horig)) continue;
                if(colorig2 >= (uint32_t)(worig)) continue;
                register uint16_t color = *(img+worig*linorig2+colorig2);
                if(color)
                    lineptr[col] = color;
            }
        }else{
            for(int32_t col = trimmed_left; col<=trimmed_right; col++){
                uint32_t linorig2 = linorig>>16, colorig2 = colorig>>16;
                linorig += sinof;
                colorig += cosof;

                if(linorig2 >= (uint32_t)(horig)) continue;
                if(colorig2 >= (uint32_t)(worig)) continue;
                lineptr[col] = color;
            }
        }
    }
}

void __not_in_flash_func(drawRotatedScaledImage)(int32_t x, int32_t y, int32_t h, int32_t w, int32_t angle, void *image, int32_t scalex, int32_t scaley){
    double anglef = angle/(64.0*1024);
    int32_t horig = h, worig = w;
    int32_t one_over_scalex = ((((int64_t)1<<16)<<32)/scalex)>>16;
    int32_t one_over_scaley = ((((int64_t)1<<16)<<32)/scaley)>>16;
    uint16_t *img = image;
    int32_t sinof = sin(anglef*DEG2RAD)*(64.0*1024);
    int32_t cosof = cos(anglef*DEG2RAD)*(64.0*1024);
    int32_t sinof_divx = (((int64_t)sinof*one_over_scalex)>>16)/**one_over_scaley)>>16*/;
    int32_t cosof_divx = (((int64_t)cosof*one_over_scalex)>>16)/**one_over_scalex)>>16*/;
    int32_t sinof_divy = (((int64_t)sinof*one_over_scaley)>>16)/**one_over_scaley)>>16*/;
    int32_t cosof_divy = (((int64_t)cosof*one_over_scaley)>>16)/**one_over_scalex)>>16*/;
    w--; h--;
    x=x<<16; y=y<<16; w=w*scalex; h=h*scaley;//<<16 cancels >>16 for w and h
    int32_t x1=x, y1=y;
    //drawRectangle(FIX_TO_PX(x1),FIX_TO_PX(y1),3,3,0xffff);
    int32_t x2=((int64_t)w*cosof)>>16; x2+=x;
    int32_t y2=((int64_t)w*sinof)>>16; y2+=y;
    //drawRectangle(FIX_TO_PX(x2),FIX_TO_PX(y2),3,3,0xffff);
    int32_t x3=(((int64_t)w*cosof)>>16) - (((int64_t)h*sinof)>>16); x3+=x;
    int32_t y3=(((int64_t)w*sinof)>>16) + (((int64_t)h*cosof)>>16); y3+=y;
    //drawRectangle(FIX_TO_PX(x3),FIX_TO_PX(y3),3,3,0xffff);
    int32_t x4= - (((int64_t)h*sinof)>>16); x4+=x;
    int32_t y4=   (((int64_t)h*cosof)>>16); y4+=y;
    //drawRectangle(FIX_TO_PX(x4),FIX_TO_PX(y4),3,3,0xffff);

    int32_t xmin = MIN(MIN(x1,x2),MIN(x3,x4)), ymin = MIN(MIN(y1,y2),MIN(y3,y4));
    int32_t xmax = MAX(MAX(x1,x2),MAX(x3,x4)), ymax = MAX(MAX(y1,y2),MAX(y3,y4));
    
    //drawRectangle(FIX_TO_PX(xmin),FIX_TO_PX(ymin),3,3,0xff00);
    //drawRectangle(FIX_TO_PX(xmax),FIX_TO_PX(ymax),3,3,0xff00);

    int32_t lmax = FIX_TO_PX(ymax), colmax = FIX_TO_PX(xmax);
    int32_t lmin = FIX_TO_PX(ymin), colmin = FIX_TO_PX(xmin);

    int16_t top = lmin<0?0:lmin;
    int16_t left = colmin<0?0:colmin;
    int16_t right = colmax>WIDTH?WIDTH-1:colmax;
    int16_t bottom = lmax>HEIGHT?HEIGHT-1:lmax;
    if(top>=bottom) return;
    if(left>=right) return;

    for(int32_t lin = top; lin<=bottom; lin++){
        register uint16_t* lineptr = frame[lin]-cuts[lin];
        int32_t trimmed_left = left<cuts[lin]?cuts[lin]:left;
        int32_t trimmed_right = right>((WIDTH-1)-cuts[lin])?(WIDTH-1)-cuts[lin]:right;
        
        int32_t colorig =  ((((int64_t)(trimmed_left<<16)-x)*cosof_divx)>>16) + ((((int64_t)((lin)<<16)-y)*sinof_divx)>>16);
        int32_t linorig =  ((((int64_t)(trimmed_left<<16)-x)*sinof_divy)>>16) - ((((int64_t)((lin)<<16)-y)*cosof_divy)>>16) + (horig<<16);
        

        for(int32_t col = trimmed_left; col<=trimmed_right; col++){
            uint32_t linorig2 = (linorig>>16), colorig2 = (colorig>>16);
            linorig += sinof_divy;
            colorig += cosof_divx;

            if(linorig2 >= (uint32_t)(horig)) continue;
            if(colorig2 >= (uint32_t)(worig)) continue;
            register uint16_t color = *(img+worig*linorig2+colorig2);
            if(color)
                lineptr[col] = color;
        }
        
    }
}

static __force_inline void drawRotatedImage(int32_t x, int32_t y, int32_t h, int32_t w, int32_t angle, void *image){
    double anglef = angle/(64.0*1024);
    anglef+=45;
    int32_t sinof = sin(anglef*DEG2RAD)*(64.0*1024);
    int32_t cosof = cos(anglef*DEG2RAD)*(64.0*1024);
    int32_t hyp = sqrt(w*w/4 + h*h/4);
    x -= (cosof*hyp)>>16;
    y -= (sinof*hyp)>>16;
    drawRotatedImageOrLine(x, y, h, w, angle, image, 0);
}

void __not_in_flash_func(drawRotatedLine)(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t thickness, uint16_t color){
    float hypot = sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
    int32_t angle = (acos(1.0*(y2-y1)/hypot)*RAD2DEG)*(64.0*1024);
    int32_t sinof = (1.0*(x2-x1)/hypot)*(64.0*1024);
    int32_t cosof = (1.0*(y2-y1)/hypot)*(64.0*1024);
    if(sinof<0) angle = -angle;

    int32_t deltax = ((int64_t)(thickness<<16)/2*cosof)>>16;
    int32_t deltay = ((int64_t)(thickness<<16)/2*sinof)>>16;

    drawRotatedImageOrLine(x1-FIX_TO_PX(deltax), y1+FIX_TO_PX(deltay), hypot, thickness, -angle, NULL, color);
}


void __not_in_flash_func(drawPartialCircleFrac)(int32_t x, int32_t y, int32_t radius, uint16_t color, uint16_t fraction){
    int32_t ymin=y-FIX_TO_PX(radius), ymax=y+FIX_TO_PX(radius);
    int32_t y_from_center = -FIX_TO_PX(radius);
    fraction = 0xffff-fraction;
    int32_t frac_limit = FIX_TO_PX(((int64_t)radius*2*fraction)>>16);
    
    for(int32_t line = ymin; line<=ymax; line++, y_from_center++){
        if(line-ymin < frac_limit) continue;
        if(line<0 || line>=HEIGHT) continue;
        float width_squared = (((int64_t)radius*radius)>>(16+16)) - (y_from_center-0.5)*(y_from_center-0.5);
        if(width_squared<=0) continue;
        int32_t line_width = sqrt(width_squared)+0.75;
        
        drawRectangle(x-line_width,line,1,line_width*2, color);//efficient enough; has clipping
    }
}

static __force_inline void drawCircleFrac(int32_t x, int32_t y, int32_t r, uint16_t c){
    drawPartialCircleFrac(x,y,r,c,0xffff);
}

static __force_inline void drawRotatedLineRoundEdges(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t thickness, uint16_t color){
    drawRotatedLine(x1,y1,x2,y2,thickness,color);
    drawCircleFrac(x1,y1, (thickness<<16)/2, color);
    drawCircleFrac(x2,y2, (thickness<<16)/2, color);
}

static __force_inline void fillScreen(uint16_t color){
    uint16_t *p = frame[0];
    uint16_t *p2 = p + pixcount;
    while(p!=p2){
        *p = color;
        p++;
    }
}

void __not_in_flash_func(printLine)(int16_t x, int16_t y, uint16_t maxW, uint16_t scrolled, char *s, uint16_t color, uint8_t italic){
    register const char *p;
    int16_t top = y<0?0:y;
    register uint8_t charLine=top-y;
    int16_t bottom = (y+16)>=HEIGHT?HEIGHT-1:(y+16);
    for(register int line = top; line<bottom; line++){
        register uint16_t* lineptr = frame[line]-cuts[line];
        p=s;
        register int16_t frameCol = !italic?x:x+(top-line)/2-(top-y)/2;
        register int16_t right = x+maxW;
        register int16_t scrolleft = scrolled;
        int16_t limL = cuts[line];
        int16_t limR = (WIDTH-1)-cuts[line];
        while(*p){
            register uint8_t charIndex = (*p) - ' ';//first char
            if(charIndex <= 127){
                register unsigned char charWidth = widtbl_f16[charIndex];
                if(scrolleft > charWidth){ scrolleft -= charWidth; p++; continue;}
                register uint8_t charCol=scrolleft;
                scrolleft=0;
                if(charWidth <= 8){
                    register uint8_t charData = chrtbl_f16[charIndex][charLine];
                    register uint8_t mask = 0x80>>charCol;
                    for(; charCol<charWidth; charCol++){
                        if(frameCol >= right) break;
                        if(charData & mask){
                            if(frameCol >= limL && frameCol <= limR)
                                lineptr[frameCol] = color;
                        }
                        frameCol++;
                        mask = mask>>1;
                    }
                }else{
                    register uint16_t charData = ((uint16_t*)(chrtbl_f16[charIndex]))[charLine];
                    charData = __builtin_bswap16(charData);
                    register uint16_t mask = 0x8000>>charCol;
                    for(; charCol<charWidth; charCol++){
                        if(frameCol >= right) break;
                        if(charData & mask){
                            lineptr[frameCol] = color;
                        }
                        frameCol++;
                        mask = mask>>1;
                    }
                }
            }
            p++;
        }
        charLine++;
    }
}

void __not_in_flash_func(printLine32)(int16_t x, int16_t y, uint16_t maxW, uint16_t scrolled, char *s, uint16_t color, uint8_t italic){
    register const char *p;
    int16_t top = y<0?0:y;
    register uint8_t charLine=(top-y)/2;
    int16_t bottom = (y+16*2)>=HEIGHT?HEIGHT-1:(y+16*2);
    for(register int line = top; line<bottom; line++){//double size
        register uint16_t* lineptr = frame[line]-cuts[line];
        p=s;
        register int16_t frameCol = !italic?x:x+(top-line)/2-(top-y)/2;
        register int16_t right = x+maxW;
        register int16_t scrolleft = scrolled;
        int16_t limL = cuts[line];
        int16_t limR = (WIDTH-1)-cuts[line];
        while(*p){
            register uint8_t charIndex = (*p) - ' ';//first char
            if(charIndex <= 127){
                register unsigned char charWidth = widtbl_f16[charIndex];
                if(scrolleft > charWidth){ scrolleft -= charWidth; p++; continue;}
                register uint8_t charCol=scrolleft;
                scrolleft=0;
                if(charWidth <= 8){
                    register uint8_t charData = chrtbl_f16[charIndex][charLine];
                    register uint8_t mask = 0x80>>charCol;
                    for(; charCol<charWidth; charCol++){
                        if(frameCol >= right) break;
                        if(charData & mask){
                            if(frameCol >= limL && frameCol <= limR)
                                lineptr[frameCol] = color;//double size
                            if(frameCol+1 >= limL && frameCol+1 <= limR)
                                lineptr[frameCol+1] = color;
                        }
                        frameCol+=2;
                        mask = mask>>1;
                    }
                }else{
                    register uint16_t charData = ((uint16_t*)(chrtbl_f16[charIndex]))[charLine];
                    charData = __builtin_bswap16(charData);
                    register uint16_t mask = 0x8000>>charCol;
                    for(; charCol<charWidth; charCol++){
                        if(frameCol >= right) break;
                        if(charData & mask){
                            lineptr[frameCol] = color;//double size
                            lineptr[frameCol+1] = color;
                        }
                        frameCol+=2;
                        mask = mask>>1;
                    }
                }
            }
            p++;
        }
        if(line%2==1)//double size
            charLine++;
    }
}

uint16_t __not_in_flash_func(lengthOf)(const char *s){
    const char *p = s;
    register uint16_t width = 0;
    while(*p){
        register uint8_t charIndex = (*p) - ' ';//first char
        if(charIndex <= 127){
            width += widtbl_f16[charIndex];
        }
        p++;
    }
    return width;
}

#endif