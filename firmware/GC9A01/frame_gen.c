#include <stdio.h>
#include <math.h>

char line_lengths[240];

void print_lines(){
    printf("typedef struct fdat{\n");
    printf("uint16_t filler1[%d];\n",line_lengths[0]);
    for(int i=0;i<240;i++)
        printf("uint16_t line%03d[%3u];%s", i, 240-2*line_lengths[i],i%4==3?"\n":" ");
    printf("uint16_t filler2[%d];\n",line_lengths[239]);
    printf("} fdat;\n");
    printf("fdat A fdat1, fdat2;\n");
}

void print_frame(int f){
    printf("uint16_t A * frame%d[240];\n", f);
    printf("uint16_t F * const frame%d__F[240]={\n", f);
    for(int i=0;i<240;i++){
        printf("fdat%d.line%03d,%s",f,i,i%8==7?"\n":" ");
    }
    printf("};\n");
}

void print_cuts(){
    printf("uint8_t A cuts[240];\n");
    printf("const uint8_t F cuts__F[240]={\n");
    for(int i=0;i<240;i++){
        printf("%3d,%s",line_lengths[i],i%16==15?"\n":" ");
    }
    printf("};\n");
}

int main()
{
    printf("#ifndef _FRAMETYPE_H\n#define _FRAMETYPE_H\n#include <stdint.h>\n");
    printf("#define A __attribute__((section (\".frameAddress\")))\n");
    printf("#define F __in_flash()\n");
    printf("/*Auto generated*/\n");
    int mem=0;
    for(int i=0;i<240;i++){
        if(i<120)
            line_lengths[i]=120-sqrt(120*120-(120-(i+1))*(120-(i+1)));
        else
            line_lengths[i]=120-sqrt(120*120-(120-(239-i+1))*(120-(239-i+1)));
        //printf("uint16_t frame1_line%03d[%3u];uint16_t frame2_line%03d[%3u];\n", i, 240-2*line_lengths[i], i, 240-2*line_lengths[i]);
        mem += (240-2*line_lengths[i]);
    }
    mem += line_lengths[0]+line_lengths[239];
    print_lines();
    printf("\n\n");
    print_frame(1);
    printf("\n\n");
    print_frame(2);
    printf("\n\n");
    print_cuts();
    
    printf("\n\n\
    void init_frame_buffers(){\n\
        for(int i=0; i<240; i++){\n\
            cuts[i]=cuts__F[i];\n\
            frame1[i]=frame1__F[i];\n\
            frame2[i]=frame2__F[i];\n\
        };\n\
    }\n");


    printf("//memory used: %d bytes(data) out of %d in 3 banks\n", mem*2*2, 3*64*1024);
    printf("//memory used: %d per frame\n", mem*2);
    
    printf("#endif\n");
    return 0;
}
