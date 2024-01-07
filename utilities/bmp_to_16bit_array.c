#include <stdio.h>
#include <string.h>
#include <stdint.h>

int main()
{
    char name[100];
    printf("Filename: ");
    scanf("%s", name);
    FILE *f = fopen(name,"rb");
    if(f==NULL){
        f = fopen("image.bmp","rb");
        strcpy(name,"image.bmp");
    }
    if(f==NULL){printf("Bad filename.\n");return 1;}
    name[strcspn(name,".")]='\0';
    FILE *o = fopen("vec.txt","wt");
    FILE *o8bvec = fopen("vec8b.txt","wt");
    FILE *o2 = fopen("out.bmp","wb");
    FILE *o12b = fopen("out12b.bmp","wb");
    FILE *o8b = fopen("out8b.bmp","wb");
    int n;
    char buf[100];

    fread (buf,0x36,1,f);
    fwrite(buf,0x36,1,o2);
    fwrite(buf,0x36,1,o12b);
    fwrite(buf,0x36,1,o8b);
    uint32_t IMAGE_WIDTH = *((uint32_t*)(buf+0x12));
    uint32_t IMAGE_HEIGHT = *((uint32_t*)(buf+0x16));
    printf("%d x %d\n", IMAGE_HEIGHT, IMAGE_WIDTH);
    //getchar();
    //printf("%c\n",c);
    //fprintf(o,"Hello\n");
    fprintf(o, "#define %s_width %u\n", name, IMAGE_WIDTH);
    fprintf(o, "#define %s_height %d\n", name, IMAGE_HEIGHT);
    fprintf(o, "uint16_t __in_flash(\"images\") %s_data[%d][%d]={\n{", name, IMAGE_HEIGHT, IMAGE_WIDTH);
    for(n=0;n<IMAGE_WIDTH*IMAGE_HEIGHT;n++){
        unsigned short b=0,g=0,r=0;
        fread (&b,1,1,f);
        fread (&g,1,1,f);
        fread (&r,1,1,f);
        unsigned short s;
        /*b=b/128;
        r=r/128;
        g=g/128;*/
//b=!b;r=!r;g=!g;
        //b*=128;
        //g*=128;
        //r*=128;

        //0-64-128-192-256
        //b = (b>>3)&0x1f;g = (g>>2)&0x3f;r = (r>>3)&0x1f;

        unsigned short b2=b&0b11111000,g2=g&0b11111100,r2=r&0b11111000;
        fwrite(&b2,1,1,o2);fwrite(&g2,1,1,o2);fwrite(&r2,1,1,o2);
        b2=b&0b11110000;g2=g&0b11110000;r2=r&0b11110000;
        fwrite(&b2,1,1,o12b);fwrite(&g2,1,1,o12b);fwrite(&r2,1,1,o12b);
        b2=b&0b11000000;g2=g&0b11100000;r2=r&0b11100000;
        fwrite(&b2,1,1,o8b);fwrite(&g2,1,1,o8b);fwrite(&r2,1,1,o8b);
        unsigned char cr = r2|(g2>>3)|(b2>>6);
        fprintf(o8bvec,"0x%x,",cr);
        if(n%IMAGE_WIDTH == IMAGE_WIDTH-1)
            fprintf(o8bvec,"\n");

        s = ((r<<8)&0xf800)|((g<<3)&0x07E0)|(b>>3);
//s = ((b<<8)&0xf800)|((g<<3)&0x07E0)|(r>>3);
        fprintf(o,"0x%x,",s);
        if((n+1)%IMAGE_WIDTH==0){
            fprintf(o,"}\n");
            if(n!=IMAGE_WIDTH*IMAGE_HEIGHT-1)fprintf(o,",{");
        }
    }

    fprintf(o, "};\n");

    fclose(f);
    fclose(o2);
    fclose(o12b);
    fclose(o8b);
    fclose(o8bvec);
    fclose(o);
    return 0;
}
