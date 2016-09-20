/**
 * @file ausgabe.c
 * Modul fuer den Ausgabe-Task.
 *
 * @author inf1073   Lennart Kindermann
 * @author inf100849 Oliver Westphal
 */

/* System-Header einbinden */

#include "includes.h"
#include <stdio.h>

/* Lokale Header einbinden */

#include "grill.h"
#include "box.h"
#include "wurst_helper.h"
#include "vga/vga.h"
#include "vga/vga_font.h"

/* lokale Konstanten und Makros */
/** @privatesection */

#define TASK_STACKSIZE   2048
#define AUSGABE_PRIORITY 25

#define B_WIDTH 640
#define W_SPACE 20
#define S_SPACE 5
#define W_TOP_Y 160
#define W_BOT_Y 300
#define S_BOT_Y (W_BOT_Y-20)

/* Modulvariablen */

OS_STK stk_ausgabe[TASK_STACKSIZE];

/* Forwarddeklarationen */

void task_ausgabe (void* pdata);

/* Öffentliche Funktionen */
/** @publicsection */

/**
 *  Initialisiert den Ausgabe-Task.
 */
void init_ausgabe()
{
  vga_dma_init();

  OSTaskCreateExt(task_ausgabe,
                  NULL,
                  (void *)&stk_ausgabe[TASK_STACKSIZE-1],
                  AUSGABE_PRIORITY,
                  AUSGABE_PRIORITY,
                  stk_ausgabe,
                  TASK_STACKSIZE,
                  NULL,
                  0);
}

/* Private Funktionen */
/** @privatesection */

/**
 * Task: Ausgabe
 *
 * Gibt den aktuellen Zustand des Grills und der Kuehlbox per VGA aus.
 *
 * Ressourcen: Kuehlbox -> box_get_anzahl
 *             Grill    -> un/lock
 *
 */
void task_ausgabe(void* pdata) {
  char        tmp_str[50];
  INT8U       err;
  INT8U       box_count;
  INT8U       grill_count;
  INT16U      grill_temp;
  char        grill_brennt[8];
  INT8U       color;
  wurst_ptr   wurst;
  INT8U       br; /* Braeunungsgrad */

  INT16U i, j, ax, ay, cx, cy, qx, qy, px, py, w_width, s_width;

  while (1) {
    OSTimeDlyHMSM(0, 0, 1, 0);

    box_count = box_get_anzahl();

    grill_lock();
    grill_count  = grill_get_anzahl();
    grill_temp   = grill_get_temperatur();
    sprintf(grill_brennt, "%s", grill_get_brennt() ? "Brennt!" : "Grillt!");
    grill_unlock();

    vga_setCurPos(0, 57);
    sprintf(tmp_str, "Box: %i Wuerste     Grill: %i Wuerste - %i C - %s", box_count, grill_count, grill_temp, grill_brennt);
    vga_puts(tmp_str, 0xFF);

    vga_quad(0, 400, 640, 0, true, 0x00);
    vga_quad(0, 400, 640, 400-(grill_temp/2), true, 128);

    ay = W_BOT_Y;
    cy = W_TOP_Y;
    py = S_BOT_Y;

    grill_lock();
    grill_count = grill_get_anzahl();
    w_width = (B_WIDTH - (grill_count + 1) * W_SPACE) / grill_count;
    s_width = (w_width - 5 * S_SPACE) / 4;

    for (i = 0; i < grill_count; i++) {
      wurst = grill_get_wurst(i);
      ax = (W_SPACE + w_width) * i + W_SPACE;
      cx = ax + w_width;
      vga_quad(ax, ay, cx, cy, false, 0xFF);
      for (j = 0; j < 4; j++) {
        br = wurst->seiten[j];
        px = (S_SPACE + s_width) * j + S_SPACE + ax;
        qx = px + s_width;
        qy = S_BOT_Y - (INT16U) br;

        if (j == wurst->seite){
          if (br < 80) {
            color = 252;
          } else
          if (br >= 80){
            color = 28;
          } else
          if (br >= 100){
            color = 224;
          }
        } else {
          if (br < 80) {
            color = 144;
          } else
          if (br >= 80){
            color = 16;
          } else
          if (br >= 100){
            color = 128;
          }
        }
        vga_quad(px, py, qx, qy, true, color);
      }
      vga_line(ax, S_BOT_Y - 100, cx, S_BOT_Y - 100, 128);
      vga_line(ax, S_BOT_Y - 80, cx, S_BOT_Y - 80, 48);
    }
    grill_unlock();
    vga_flush();
  }
}
