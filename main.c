#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <draw.h>
#include <graph.h>
#include <gs_psm.h>
#include <gs_gp.h>
#include <dma.h>
#include <dma_tags.h>

#include <inttypes.h>

#define OFFSET_X 2048
#define OFFSET_Y 2048

#define myftoi4(x) ((x)<<4)

#define VID_W 640
#define VID_H 448

static qword_t *buf; 

#define DRAWBUF_LEN (100 * 16)

int print_buffer(qword_t *b, int len)
{
  printf("-- buffer\n");
  for(int i = 0; i < len; i++) {
    printf("%016llx %016llx\n", b->dw[0], b->dw[1]);
    b++;
  }
  printf("-- /buffer\n");
  return 0;
}

int gs_finish()
{
  qword_t *q = buf;
  q = draw_finish(q);
  dma_channel_send_normal(DMA_CHANNEL_GIF, buf, q-buf, 0, 0);
  dma_wait_fast();
  return 0;
}


zbuffer_t *z;
int gs_init(int width, int height, int psm, int psmz, int vmode, int gmode)
{
  framebuffer_t fb;
  fb.address = graph_vram_allocate(width, height, psm, GRAPH_ALIGN_PAGE);
  fb.width = width;
  fb.height = height;
  fb.psm = psm;
  fb.mask = 0;

  z->address = graph_vram_allocate(width, height, psmz, GRAPH_ALIGN_PAGE);
  z->enable = 0;
  z->method = 0;
  z->zsm = 0;
  z->mask = 0;

  graph_set_mode(gmode, vmode, GRAPH_MODE_FIELD, GRAPH_DISABLE); 
  graph_set_screen(0, 0, width, height);
  graph_set_bgcolor(0, 0, 0);
  graph_set_framebuffer_filtered(fb.address, width, psm, 0, 0);
  graph_enable_output();

  qword_t *q = buf;
  memset(buf, 0, DRAWBUF_LEN);
  q = draw_setup_environment(q, 0, &fb, z);
  q = draw_primitive_xyoffset(q, 0, 2048-(VID_W/2), 2048-(VID_H/2));
  q = draw_finish(q);
  dma_channel_send_normal(DMA_CHANNEL_GIF, buf, q-buf, 0, 0);
  draw_wait_finish();


  return 0;
}


static int tri1[] = {
  0, 0, 0,
  0, 0, 0,
  0, 0, 0,
};
//static int tri2[] = {
//  0, 448, 0,
//  0, 224, 0,
//  320, 448, 0,
//};
//
//static int tri3[] = {
//  640, 0, 0,
//  320, 0, 0,
//  640, 224, 0,
//};
//static int tri4[] = {
//  640, 448, 0,
//  320, 448, 0,
//  640, 224, 0,
//};
//
//static int tri11[] = {
//  320, 224, 0,
//  320, 0, 0,
//  0, 224, 0,
//};
//static int tri22[] = {
//  320, 224, 0,
//  0, 224, 0,
//  320, 448, 0,
//};
//
//static int tri33[] = {
//  320, 224, 0,
//  320, 0, 0,
//  640, 224, 0,
//};
//static int tri44[] = {
//  320, 224, 0,
//  320, 448, 0,
//  640, 224, 0,
//};



#define SHIFT_AS_I64(x, b) (((int64_t)x)<<b)

qword_t *draw(qword_t *q, uint64_t R,uint64_t G,uint64_t B, int tris[])
{
  
  uint64_t red = 0x00 + R;
  uint64_t blue = 0x00+ G;
  uint64_t green = 0x00 +B;

  // SET PRIM
  q->dw[0] = 0x1000000000000001;
  q->dw[1] = 0x000000000000000e;
  q++;
  q->dw[0] = GS_SET_PRIM(GS_PRIM_TRIANGLE, 0, 0, 0, 0, 0, 0, 0, 0);
  q->dw[1] = GS_REG_PRIM;
  q++;
  // 6 regs, x1, EOP
  q->dw[0] = 0x6000000000008001;
  // GIFTag header - col, pos, col, pos, col, pos
  q->dw[1] = 0x0000000000515151;
  q++;

  int cx = myftoi4(2048 - (VID_W/2));
  int cy = myftoi4(2048 - (VID_H/2));

  for(int i = 0; i < 3; i++) {

    q->dw[0] = (red&0xff) | (green&0xff)<<32;
    q->dw[1] = (blue&0xff) | (SHIFT_AS_I64(0x80, 32));
    q++;
    
    // 0xa -> 0xa0
    // fixed point format - xxxx xxxx xxxx.yyyy
    int base = i*3;
    int x = myftoi4(tris[base+0]) + cx;
    int y = myftoi4(tris[base+1]) + cy;
    int z = 0;
    q->dw[0] = x | SHIFT_AS_I64(y, 32);
    q->dw[1] = z; 
    // printf("drawing vertex %x %x %x\n", tri[base+0], tri[base+1], tri[base+2]);
    q++;
  }

  return q;
}

uint64_t R= 0;
uint64_t G= 0;
uint64_t B= 0;


uint64_t R1= 0;
uint64_t G1= 0;
uint64_t B1= 0;


int main()
{
  time_t time;
  
  srand(time);
  
  buf = malloc(DRAWBUF_LEN);
  z = malloc(sizeof(zbuffer_t));
  // init DMAC
  dma_channel_initialize(DMA_CHANNEL_GIF, 0, 0);
  dma_channel_fast_waits(DMA_CHANNEL_GIF);

  int vmode = graph_get_region();
  // initialize graphics mode 
  gs_init(VID_W, VID_H, GS_PSM_32, GS_PSMZ_24, vmode, GRAPH_MODE_INTERLACED);
  int swap = 0;
  graph_wait_vsync();




  while(1) {
R= rand() % (255 + 1 - 0) + 0; 
G= rand() % (255 + 1 - 0) + 0;
B= rand() % (255 + 1 - 0) + 0;



//for (size_t i = 0; i < 9; i++)
//{
 // printf("tri data: %d %d \n",i, tri1[i]);
//}



    dma_wait_fast();
    qword_t *q = buf;
    memset(buf, 0, DRAWBUF_LEN);
    q = draw_disable_tests(q, 0, z);
    q = draw_clear(q, 0, 2048.0f - 320, 2048.0f - 244, VID_W, VID_H, 20, 20, 20);
    q = draw_enable_tests(q, 0, z);

//if(swap %2)
//{
int y = 0;
for (size_t x = 0; x < 3; x++)
{
 for (size_t y = 0; y < 3; y++)  
 {
   tri1[0] = 213 * x;
   tri1[3] = 213 * (x + 1);
   tri1[6] = tri1[0];

   tri1[1] = 149 * y;
   tri1[4] = 149 * y;
   tri1[7] = 149 * (y + 1);

   q = draw(q, R,G,B, tri1);
     

   printf("%d %d \n", tri1[0], tri1[1]);  
   printf("%d %d \n", tri1[3], tri1[4]);
   printf("%d %d \n \n", tri1[6], tri1[7]);
 }
}
  //printf("R %d G %d B %d \n", R,G,B);
swap++;
    q = draw_finish(q);
    dma_channel_send_normal(DMA_CHANNEL_GIF, buf, q-buf, 0, 0);
    //print_buffer(buf, q-buf); 

    draw_wait_finish();
    // wait for vsync
    graph_wait_vsync();
    sleep(1);
  }
}

//nt * getRandom()
//
// static int r[3];
//
//[0] = rand() % (255 + 1 - 0) + 0;
//[1] = rand() % (255 + 1 - 0) + 0;
//[2] = rand() % (255 + 1 - 0) + 0;
// return r;
//

