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

#include "gs.h"
#include "mesh.h"
#include "draw.h"
#include "log.h"
#include "pad.h"

#define OFFSET_X 2048
#define OFFSET_Y 2048


#define VID_W 640
#define VID_H 448

#define TGT_FILE "CUBE.BIN"

#define fatalerror(st, msg, ...) printf("FATAL: " msg "\n", ##__VA_ARGS__); error_forever(st); ((void)0)
void error_forever(struct draw_state *st);

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

int main()
{
  printf("Hello\n");
  qword_t *buf = malloc(20000*16);
  char *file_load_buffer = malloc(310 * 1024);
  int file_load_buffer_len = 310*1024;

  struct draw_state st = {0};
  st.width = VID_W,
  st.height = VID_H,
  st.vmode = graph_get_region(),
  st.gmode = GRAPH_MODE_INTERLACED,

  // init DMAC
  dma_channel_initialize(DMA_CHANNEL_GIF, 0, 0);
  dma_channel_fast_waits(DMA_CHANNEL_GIF);

  // initialize graphics mode 
  gs_init(&st, GS_PSM_32, GS_PSMZ_24);

  struct model m = {0};
  struct model m1 = {0};
  struct model m2 = {0};
  struct model m3 = {0};





  int bytes_read = load_file(TGT_FILE, file_load_buffer, file_load_buffer_len);
  if (bytes_read <= 0) {
    fatalerror(&st, "failed to load file %s", TGT_FILE);
  }
  if (bytes_read % 16 != 0) {
    fatalerror(&st, "lengt of model file %s was not 0 %% 16", TGT_FILE);
  }

  if (!model_load(&m, file_load_buffer, bytes_read)) {
    fatalerror(&st, "failed to process model");
  }

    if (!model_load(&m1, file_load_buffer, bytes_read)) {
    fatalerror(&st, "failed to process model");
  }
  struct render_state r = {0};

  r.camera_pos[0] = 0.0f;
  r.camera_pos[2] = 10.0f;
  r.camera_pos[3] = 1.0f;

  r.clear_col[0] = 0x2c;
  r.clear_col[1] = 0x2f;
  r.clear_col[2] = 0x33;

  r.offset_x = OFFSET_X;
  r.offset_y = OFFSET_Y;

  struct model_instance cube = {0};
  cube.m = &m;
  cube.scale[0] = .8f;
  cube.scale[1] = .8f;
  cube.scale[2] = .8f;
  cube.scale[3] = 1.0f;
  cube.translate[2] = -10.f;

  pad_init();

int X = 0;
int T = 0;
int S = 0;
int C = 0;
//int yy = 1;
//bool zz = false;
  graph_wait_vsync();
  while(1) {
    pad_frame_start();
    pad_poll();


    X = button_held(0);
    S = button_held(1);
    C = button_held(12);
    T = button_held(11);
    
  if(X)
  {
    cube.m->r =   0x7C;
    cube.m->g =   0xB2;
    cube.m->b =   0xE8;
  }
  else if(S)
  {
    cube.m->r = 0xFF;

    cube.m->g = 0x69;

    cube.m->b = 0xF8;

  }
    else if(C)
  {
    cube.m->r = 0xFF;
    cube.m->g = 0x66;
    cube.m->b = 0x66;

  }
    else if(T)
  {
    cube.m->r = 0x40;
    cube.m->g = 0xE2;
    cube.m->b = 0x10;

  }
  else
  {
    cube.m->r = 0xFF;
    cube.m->g = 0xFF;
    cube.m->b = 0xFF;
  }




    info("Button X:%d S:%d C:%d T:%d \n", X, S,C,T );
    update_draw_matrix(&r);
    dma_wait_fast();
    qword_t *q = buf;
    memset(buf, 0, 20000*16);
    q = draw_disable_tests(q, 0, &st.zb);
    q = draw_clear(q, 0, 2048.0f - 320, 2048.0f - 244, 
        VID_W, VID_H, 
        r.clear_col[0], r.clear_col[1], r.clear_col[2]);
    q = draw_enable_tests(q, 0, &st.zb);
    qword_t *model_verts_start = q;
    memcpy(q, m.buffer, m.buffer_len);
    info("copied mesh buffer with len=%d", m.buffer_len);
    q += (m.buffer_len/16);
    q = draw_finish(q);
  
      mesh_transform((char*) (model_verts_start+MESH_HEADER_SIZE), &cube, &r);
    
    dma_channel_send_normal(DMA_CHANNEL_GIF, buf, q-buf, 0, 0);
    // print_buffer(buf, q-buf); 
    info("draw from buffer with length %d", q-buf);

    draw_wait_finish();
    graph_wait_vsync();

    int dx = button_held(DPAD_RIGHT) - button_held(DPAD_LEFT);
    int dy = button_held(DPAD_DOWN) - button_held(DPAD_UP);
    int dz = button_held(BUTTON_L1) - button_held(BUTTON_L2); 

      
    cube.translate[0] += 0.6f * dx;
    cube.translate[1] += 0.6f * dy;
    cube.translate[2] += 0.05f * dz;
    int cdx = button_held(BUTTON_R1) - button_held(BUTTON_R2);
    r.camera_pos[0] += 0.1f * cdx;
  }
}


void error_forever(struct draw_state *st)
{
  qword_t *buf = malloc(1200);
  while(1) {
    dma_wait_fast();
    qword_t *q = buf;
    memset(buf, 0, 1200);
    q = draw_disable_tests(q, 0, &st->zb);
    q = draw_clear(q, 0, 2048.0f - 320, 2048.0f - 244, VID_W, VID_H, 0xff, 0, 0);
    q = draw_finish(q);
    dma_channel_send_normal(DMA_CHANNEL_GIF, buf, q-buf, 0, 0);
    draw_wait_finish();
    graph_wait_vsync();
    sleep(2);
  }


}

/*
 *
 * xxxx xxxx xxxx . yyyy
 * 0100 0000 0000 . 0000
 *
2 */
