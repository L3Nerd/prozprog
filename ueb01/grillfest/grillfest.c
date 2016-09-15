#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "includes.h"
#include "ps2/ps2.h"
#include "ps2/ps2_keyscan.h"
#include "vga/vga.h"
#include "vga/vga_font.h"

/* Konstanten */
#define MAX_WUERSTE      30
#define TICKS_PER_SECOND 10
#define ANZAHL_SEITEN    4

/* Definition of Task Stacks */
#define   TASK_STACKSIZE 2048
OS_STK    stk_fleischer[TASK_STACKSIZE];
OS_STK    stk_eingabe[TASK_STACKSIZE];
OS_STK    stk_grillmeister[TASK_STACKSIZE];
OS_STK    stk_physik[TASK_STACKSIZE];
OS_STK    stk_ausgabe[TASK_STACKSIZE];

/* Definition of Task Priorities */

#define EINGABE_PRIORITY      10
#define PHYSIK_PRIORITY       13
#define GRILLMEISTER_PRIORITY 16
#define FLEISCHER_PRIORITY    22
#define AUSGABE_PRIORITY      25

/* keyboard data queue */
#define OS_KEYBOARD_Q_SIZE	10
OS_EVENT *keyQ;
void *keyQBuf[OS_KEYBOARD_Q_SIZE];

/* Datenstrukturen */

typedef struct {
  INT8U seiten[ANZAHL_SEITEN];
  INT8U seite;
} Wurst;

typedef Wurst * wurst_ptr;

typedef struct {
  wurst_ptr wuerste[MAX_WUERSTE];
  INT8U     anzahl;
  BOOLEAN   brennt;
  INT16U    temperatur;
} Grill;

/* Wurst Semaphore und Speicher */

OS_EVENT * WurstSema;
OS_MEM   * WurstBuff;
INT8U      WurstPart[MAX_WUERSTE][sizeof(Wurst)];

/* FleischerTrinkt Semaphore und Speicher */

OS_EVENT * FleischerBusySema;
BOOLEAN Fleischer_busy = true;

/* Grillmeister Semaphoren */
OS_EVENT * GrillmeisterTrinktSema;
OS_EVENT * PlaceWurstSema;
BOOLEAN    Grillmeister_trinkt = false;

/* Grill und Temperatur Semaphore */
OS_EVENT * GrillSema;
Grill grill;

/* Kuehlbox Semaphore, Speicher und Timer */
OS_EVENT * BoxQueue;
void     * BoxMsg[MAX_WUERSTE];
OS_TMR   * box_timer;
char     * box_timer_name = "box_timer";

/* keyboard data queue */
#define    OS_KEYBOARD_Q_SIZE  10
OS_EVENT * keyQ;
void     * keyQBuf[OS_KEYBOARD_Q_SIZE];

/* functions */

/*
 * term_clearLine
 *
 * Clears the line indicated by 'y'.
 * Sets the cursor at the beginning of the line after clering.
 *
 * @param y line to be cleared.
 */
static void term_clearLine(uint8_t y) {
  vga_setCurPos(0, y);
  vga_puts("                    "
      "                    "
      "                    "
      "                    "
      "                    "
      "      ", 0xFF);
  vga_setCurPos(0, y);
}

void toggle_fleischer_busy() {
  INT8U err;
  if (Fleischer_busy)
    OSSemPost(FleischerBusySema);
  else
    OSSemPend(FleischerBusySema, 0, &err);

  Fleischer_busy = !Fleischer_busy;
}

void toggle_Grillmeister_trinkt() {
  INT8U err;
  if (Grillmeister_trinkt)
    OSSemPost(GrillmeisterTrinktSema);
  else
    OSSemPend(GrillmeisterTrinktSema, 0, &err);

  Grillmeister_trinkt = !Grillmeister_trinkt;
}

/* keyboard hook
 *
 * get keyboard data from isr and post to queue (if space available)
 */
void key_cb (uint8_t key) {
  uint8_t err = OS_ERR_NONE;

  // scheduler mark for isr code
  OSIntEnter();

  err = OSQPost(keyQ, (void *) (uint32_t) key);
  if (err == OS_ERR_Q_FULL)
    printf("keyboard queue full, key (%02X) lost\n", key);

  OSIntExit();
}

void box_timer_cb (void *ptmr, void *callback_arg) {
  toggle_fleischer_busy();
}

/* Erstellt eine Wurst. */
wurst_ptr create_wurst()
{
  INT8U err;
  wurst_ptr wurst;

  OSSemPend(WurstSema, 0, &err);
  wurst = OSMemGet(WurstBuff, &err);

  /* Wurst Seiten initialisieren. */
  wurst->seiten[0] = 0;
  wurst->seiten[1] = 0;
  wurst->seiten[2] = 0;
  wurst->seiten[3] = 0;

  wurst->seite = 0;

  OSSemPost(WurstSema);

  return wurst;
}

void delete_wurst(wurst_ptr wurst) {
  INT8U err;
  OSSemPend(WurstSema, 0, &err);
  OSMemPut(WurstBuff, wurst);
  OSSemPost(WurstSema);
}

void place_wurst() {
  OSSemPost(PlaceWurstSema);
}

void grill_add (wurst_ptr wurst) {
  grill.wuerste[grill.anzahl] = wurst;
  grill.anzahl++;
}

void grill_remove (INT8U id) {
  INT8S c;
  for(c = id; c < grill.anzahl - 1; c++)
    grill.wuerste[c] = grill.wuerste[c+1];
  grill.anzahl--;
}

void increase_temperatur() {
  INT8U err;
  OSSemPend(GrillSema, 0, &err);
  grill.temperatur += 20;
  OSSemPost(GrillSema);
}

void decrease_temperatur() {
  INT8U err;
  OSSemPend(GrillSema, 0, &err);
  grill.temperatur -= 20;
  OSSemPost(GrillSema);
}

/* Fleischer: erzeugt Wuerste */
void task_fleischer(void* pdata)
{
  INT8U err;
  wurst_ptr wurst;
  while (1)
  {
    OSSemPend(FleischerBusySema, 0, &err);

    OSTimeDlyHMSM(0, 0, 1, 0);
    wurst = create_wurst();

    OSQPost(BoxQueue, wurst);
    OSTmrStop(box_timer, OS_TMR_OPT_NONE, NULL, &err);

    OSSemPost(FleischerBusySema);
  }
}

/* Grillmeister: braet Wuerste */
void task_grillmeister(void* pdata)
{
  INT8U     err;
  INT8U     sem_value;
  INT8U     id;
  wurst_ptr wurst;
  OS_Q_DATA data;
  while (1)
  {
    OSSemPend(GrillmeisterTrinktSema, 0, &err);

    /* Semaphore um User-Eingabe zu verarbeiten. */
    sem_value = OSSemAccept(PlaceWurstSema);

    if (sem_value > 0) {
      wurst = OSQAccept(BoxQueue, &err);
      if (wurst != (void *)0) {
        OSTimeDlyHMSM(0, 0, 5, 0);
        OSSemPend(GrillSema, 0, &err);
        grill_add(wurst);
        OSSemPost(GrillSema);
        /* Wenn Box leer ist Timer Starten. */
        OSQQuery(BoxQueue, &data);
        if (data.OSNMsgs == 0) {
          OSTmrStart(box_timer, &err);
        }
      }
    } else {
      if (grill.anzahl != 0) {
        /* Random Wurst vom Grill in Zange nehmen. */
        OSSemPend(GrillSema, 0, &err);
        id    = rand() % grill.anzahl;
        printf("[Meister] Hat id: %i\n", id);
        wurst = grill.wuerste[id];
        grill_remove(id);
        OSSemPost(GrillSema);

        /* Eine Sekunde zum checken. */
        OSTimeDlyHMSM(0, 0, 1, 0);
        if (wurst->seiten[wurst->seite] >= 80) {
          if (wurst->seite < 3) {
            OSTimeDlyHMSM(0, 0, 5, 0);
            wurst->seite++;
            OSSemPend(GrillSema, 0, &err);
            grill_add(wurst);
            OSSemPost(GrillSema);
          } else {
            OSTimeDlyHMSM(0, 0, 10, 0);
            delete_wurst(wurst);
          }
        } else {
          OSSemPend(GrillSema, 0, &err);
          grill_add(wurst);
          OSSemPost(GrillSema);
        }
      } else {
        OSTimeDlyHMSM(0, 0, 0, 500);
      }
    }
    OSSemPost(GrillmeisterTrinktSema);
  }
}

/* Task: Fuer Physik
 *
 * Berechnet den Braeunungsgrad der Wuerste.
 */
void task_physik(void* pdata)
{
  INT8U err;
  wurst_ptr wurst;
  INT8U i;
  while (1)
  {
    OSTimeDlyHMSM(0, 0, 1, 0);
    OSSemPend(GrillSema, 0, &err);

    i = 0;
    while(i < grill.anzahl && !grill.brennt) {
      wurst = grill.wuerste[i];
      wurst->seiten[wurst->seite] += grill.temperatur/100;
      if (wurst->seiten[wurst->seite] > 100) {
        grill.brennt = true;
      }

      printf("[Physik] %i Seite: %i bei %i %\n", wurst, wurst->seite, wurst->seiten[wurst->seite]);
      i++;
    }

    OSSemPost(GrillSema);
  }
}

/*
 * Task: For User input
 *
 * Polls the keyboard input queue for new keypresses
 */
void task_eingabe(void* pdata) {
  uint8_t qErr = OS_ERR_NONE;
  uint32_t *qMsg;

  while (1) {
    // wait for keyboard input
    qMsg = OSQPend(keyQ, 0, &qErr);
    if (qErr == OS_ERR_NONE) {
      // convert into character
      uint8_t c = (uint32_t)qMsg & 0xFF;
      if (!(c & KEY_SPECIAL)) {
        if (c == 'd' || c == 'D')
          toggle_fleischer_busy();
        else if (c == 'p' || c == 'P') {
          OSSemPost(PlaceWurstSema);
        }
        else if (c == 'g' || c == 'G') {
          toggle_Grillmeister_trinkt();
        }
        else if (c == '+')
          increase_temperatur();
        else if (c == '-')
          decrease_temperatur();
      } else {
        c &= ~KEY_SPECIAL;
        // handle some special keys for demo control
        switch (c) {
        case KEY_F1:
          break;
        }
      }
    }
  }
}

void task_ausgabe(void* pdata) {
  char        tmp_str[50];
  OS_Q_DATA   data;
  INT8U       err;
  INT8U       box_count;
  INT8U       grill_count;
  INT16U      grill_temp;
  char        grill_brennt[8];

  while (1) {
    OSTimeDlyHMSM(0, 0, 1, 0);

    OSQQuery(BoxQueue, &data);
    box_count = data.OSNMsgs;

    OSSemPend(GrillSema, 0, &err);
    grill_count  = grill.anzahl;
    grill_temp   = grill.temperatur;
    sprintf(grill_brennt, "%s", grill.brennt ? "Brennt!" : "Grillt!");
    OSSemPost(GrillSema);

    /*term_clearLine(57);*/
    vga_setCurPos(0, 57);
    sprintf(tmp_str, "Box: %i Wuerste     Grill: %i Wuerste - %i C - %s", box_count, grill_count, grill_temp, grill_brennt);
    vga_puts(tmp_str, 0xFF);

    vga_quad(0, 400, 640, 0, true, 0x00);
    vga_quad(0, 400, 640, 400-(grill_temp/2), true, 128);
    vga_flush();
  }
}

/* The main function creates two task and starts multi-tasking */
int main(void)
{
  INT8U err;
  time_t t;

  time(&t);
  srand((unsigned int)t);
  /* Partition und Semaphore f�r W�rste alloziieren */
  WurstSema = OSSemCreate(1);
  WurstBuff = OSMemCreate(WurstPart, MAX_WUERSTE, sizeof(Wurst), &err);

  /* Semaphore f�r FleischerTrinkt alloziieren */
  FleischerBusySema = OSSemCreate(0);

  /* Message Queue f�r K�hlbox alloziieren */
  BoxQueue = OSQCreate(&BoxMsg[0], MAX_WUERSTE);

  /* Grill und Temperatur Sema */
  GrillSema = OSSemCreate(1);

  /* Grillmeister Semaphoren */
  GrillmeisterTrinktSema = OSSemCreate(1);
  PlaceWurstSema         = OSSemCreate(0);

  /* setup ps2 macro and user callback */
  keyQ = OSQCreate(&keyQBuf[0], OS_KEYBOARD_Q_SIZE);
  ps2_init();
  ps2_keyListener(key_cb);
  vga_dma_init();

  /* Timer erstellen und starten. */

  box_timer = OSTmrCreate(5 * TICKS_PER_SECOND, 0, OS_TMR_OPT_ONE_SHOT, box_timer_cb, NULL, box_timer_name, &err);
  OSTmrStart(box_timer, &err);

  grill.anzahl     = 0;
  grill.brennt     = false;
  grill.temperatur = 100;

  OSTaskCreateExt(task_fleischer,
                  NULL,
                  (void *)&stk_fleischer[TASK_STACKSIZE-1],
                  FLEISCHER_PRIORITY,
                  FLEISCHER_PRIORITY,
                  stk_fleischer,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  OSTaskCreateExt(task_grillmeister,
                  NULL,
                  (void *)&stk_grillmeister[TASK_STACKSIZE-1],
                  GRILLMEISTER_PRIORITY,
                  GRILLMEISTER_PRIORITY,
                  stk_grillmeister,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  OSTaskCreateExt(task_physik,
                  NULL,
                  (void *)&stk_physik[TASK_STACKSIZE-1],
                  PHYSIK_PRIORITY,
                  PHYSIK_PRIORITY,
                  stk_physik,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  OSTaskCreateExt(task_eingabe,
                  NULL,
                  (void *)&stk_eingabe[TASK_STACKSIZE-1],
                  EINGABE_PRIORITY,
                  EINGABE_PRIORITY,
                  stk_eingabe,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  OSTaskCreateExt(task_ausgabe,
                  NULL,
                  (void *)&stk_ausgabe[TASK_STACKSIZE-1],
                  AUSGABE_PRIORITY,
                  AUSGABE_PRIORITY,
                  stk_ausgabe,
                  TASK_STACKSIZE,
                  NULL,
                  0);
  OSStart();
  return 0;
}
