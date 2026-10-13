#include "mmx4_coop_assets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1; } } while(0)
static unsigned u32(const unsigned char *p) { return (unsigned)p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24; }
static int original(const char *path) {
    FILE *f=fopen(path,"rb");CHECK(f);CHECK(!fseek(f,0,SEEK_END));long n=ftell(f);CHECK(n>0);rewind(f);
    unsigned char *b=malloc((size_t)n);CHECK(b);CHECK(fread(b,1,(size_t)n,f)==(size_t)n);fclose(f);
    Mmx4Asset a;CHECK(mmx4_coop_arc_asset(b,(size_t)n,2,&a));CHECK(a.type==2);
    unsigned tables=u32(a.data)/4,frames=0;CHECK(tables>0&&tables<32);
    for(unsigned t=0;t<tables;++t) {
        unsigned table=u32(a.data+t*4);CHECK(table<a.size-4);
        unsigned count=(u32(a.data+table)&0xFFFFF)/4;CHECK(count>0&&count<4096);
        for(unsigned i=0;i<count;++i) {
            CHECK(table+i*4+4<=a.size);
            unsigned packed=u32(a.data+table+i*4),source=table+(packed&0xFFFFF);
            unsigned char decoded[32768];size_t written;
            CHECK(source<a.size);
            CHECK(mmx4_coop_decode_sprite(a.data+source,a.size-source,decoded,sizeof decoded,&written));
            CHECK(written==(packed>>20)*128u);++frames;
        }
    }
    printf("%s: %u original sprite frames decoded\n",path,frames);free(b);return 0;
}
int main(int argc,char **argv) {
    /* Captured Frost Tower row: native page-edge clipping left a gap at
     * screen y174. Rebuild the missing edge without dropping source row0. */
    uint32_t ice[10]={0x123456,0x2D000000,0x009F0073,0x78000FD0,
        0x009F0083,0x00200FE0,0x00AE0073,0x000000D0,0x00AE0083,0x000000E0};
    CHECK(mmx4_coop_sprite_quad_tile(ice)==0x00D2);
    CHECK(ice[0]==0x123456 && ice[1]==0x2D000000);
    CHECK(ice[6]==0x00AF0073 && ice[8]==0x00AF0083);
    CHECK(ice[3]==0x78000000 && ice[5]==0x00200010 && ice[7]==0x1000 && ice[9]==0x1010);
    static uint16_t pixels[256*256];uint16_t tile_pixels[256];
    for(unsigned i=0;i<256*256;++i)pixels[i]=(uint16_t)i;
    for(unsigned row=0;row<16;++row)for(unsigned col=0;col<16;++col)
    for(unsigned flip=0;flip<4;++flip) {
        unsigned u=col*16+(flip&1?15:0),v=row*16+(flip&2?15:0);
        unsigned end_u=flip&1?(col?u-16:0):(col==15?255:u+16);
        unsigned end_v=flip&2?(row?v-16:0):(row==15?255:v+16);
        uint32_t q[10]={0,0x2D808080,0xFFF0FFF0,u|(v<<8),0,end_u|(v<<8),
            0,u|(end_v<<8),0,end_u|(end_v<<8)};
        uint16_t key=mmx4_coop_sprite_quad_tile(q);
        CHECK(key==(col*16|(row*16<<8)|flip));
        CHECK(q[2]==0xFFF0FFF0 && q[4]==0xFFF00000 && q[6]==0x0000FFF0 && q[8]==0);
        mmx4_coop_sprite_tile_pixels(pixels,key,tile_pixels);
        for(unsigned y=0;y<16;++y)for(unsigned x=0;x<16;++x)
            CHECK(tile_pixels[y*16+x]==(row*16+(flip&2?15-y:y))*256+col*16+(flip&1?15-x:x));
    }
    unsigned char out[16];size_t n=999;
    /* Literal 0x1234, overlapping three-word backref, explicit terminator. */
    const unsigned char src[]={0,0x60,0x34,0x12,1,0x18,0,0,0,0};
    CHECK(mmx4_coop_decode_sprite(src,sizeof src,out,sizeof out,&n));CHECK(n==8);
    for(unsigned i=0;i<8;i+=2)CHECK(out[i]==0x34&&out[i+1]==0x12);
    CHECK(!mmx4_coop_decode_sprite(src,sizeof src,out,7,&n));CHECK(n==0);
    for(size_t i=0;i<sizeof src;++i)CHECK(!mmx4_coop_decode_sprite(src,i,out,sizeof out,&n));
    const unsigned char invalid[]={0,0x80,1,8};
    CHECK(!mmx4_coop_decode_sprite(invalid,sizeof invalid,out,sizeof out,&n));
    const unsigned char zeros[]={0,0xC0,0,0x10,0,0,0,0};
    CHECK(mmx4_coop_decode_sprite(zeros,sizeof zeros,out,sizeof out,&n));CHECK(n==4);
    CHECK(!out[0]&&!out[1]&&!out[2]&&!out[3]);
    unsigned char arc[4096]={0};arc[0]=1;arc[5]=16;arc[8]=9;arc[12]=3;arc[2048]=42;
    Mmx4Asset a;CHECK(mmx4_coop_arc_asset(arc,sizeof arc,0,&a));CHECK(a.type==9&&a.size==3&&a.data[0]==42);
    CHECK(!mmx4_coop_arc_asset(arc,sizeof arc,1,&a));
    CHECK(!mmx4_coop_arc_asset(arc,sizeof arc-1,0,&a));
    arc[13]=32;CHECK(!mmx4_coop_arc_asset(arc,sizeof arc,0,&a));
    for(int i=1;i<argc;++i)CHECK(!original(argv[i]));
    puts("co-op asset checks passed");return 0;
}
