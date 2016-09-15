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
#define MAX_MSG          10

#define TEMP_INIT        100
#define TEMP_MAX         400
#define TEMP_MIN         0
#define TEMP_INC         20

#define B_WIDTH 640
#define W_SPACE 20
#define S_SPACE 5
#define W_TOP_Y 160
#define W_BOT_Y 300
#define S_BOT_Y (W_BOT_Y-20)


/* Definition of Task Stacks */
#define   TASK_STACKSIZE 2048
OS_STK    stk_fleischer[TASK_STACKSIZE];
OS_STK    stk_eingabe[TASK_STACKSIZE];
OS_STK    stk_grillmeister[TASK_STACKSIZE];
OS_STK    stk_physik[TASK_STACKSIZE];
OS_STK    stk_ausgabe[TASK_STACKSIZE];
OS_STK    stk_feuerwehr[TASK_STACKSIZE];

/* Definition of Task Priorities */

#define EINGABE_PRIORITY      10
#define PHYSIK_PRIORITY       13
#define GRILLMEISTER_PRIORITY 16
#define FEUERWEHR_PRIORITY    19
#define FLEISCHER_PRIORITY    22
#define AUSGABE_PRIORITY      25

/* keyboard data queue */
#define OS_KEYBOARD_Q_SIZE	10
OS_EVENT *keyQ;
void *keyQBuf[OS_KEYBOARD_Q_SIZE];

/* Datenstrukturen */

typedef enum {
  START_TRINKEN = 1,
  STOP_TRINKEN  = 2,
  PLACE_WURST   = 3,
  CREATE_WURST  = 4
} MsgTypes;

typedef MsgTypes * msg_ptr;

typedef struct {
  float seiten[ANZAHL_SEITEN];
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

/* Message Semaphore und Speicher */

OS_EVENT * MsgSema;
OS_MEM   * MsgBuff;
INT8U      MsgPart[MAX_MSG][sizeof(MsgTypes)];

/* Fleischer Message Box */
OS_EVENT * FleischerMsgBox;

/* Grillmeister Message Box */
OS_EVENT * GrillmeisterMsgBox;

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

/* Message erstellen. */
msg_ptr create_msg(MsgTypes msg_type) {
  INT8U err;
  msg_ptr msg;

  OSSemPend(MsgSema, 0, &err);
  msg = OSMemGet(MsgBuff, &err);
  *msg = msg_type;
  OSSemPost(MsgSema);

  return msg;
}

/* Entfernt eine Message aus der Memory Partition */
void delete_msg(msg_ptr msg) {
  INT8U err;
  if (msg != (void*)0) {
    OSSemPend(MsgSema, 0, &err);
    OSMemPut(MsgBuff, msg);
    OSSemPost(MsgSema);
  }
}

void send_fleischer_msg(MsgTypes msg_type) {
  msg_ptr msg = create_msg(msg_type);
  OSMboxPost(FleischerMsgBox, msg);
}

void send_grillmeister_msg(MsgTypes msg_type) {
  msg_ptr msg = create_msg(msg_type);
  OSMboxPost(GrillmeisterMsgBox, msg);
}

/* Callback fuer die Kuehlbox. */
void box_timer_cb (void *ptmr, void *callback_arg) {
  send_fleischer_msg(CREATE_WURST);
}

/* Erstellt eine Wurst und initialisiert sie. */
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

/* Entfernt eine Wurst aus der Memory Partition. */
void delete_wurst(wurst_ptr wurst) {
  INT8U err;
  OSSemPend(WurstSema, 0, &err);
  OSMemPut(WurstBuff, (void *)wurst);
  OSSemPost(WurstSema);
}

/* Legt Wurst auf Grill. */
void grill_add (wurst_ptr wurst) {
  grill.wuerste[grill.anzahl] = wurst;
  grill.anzahl++;
}

/* Entfernt Wurst von Grill. */
void grill_remove (INT8U id) {
  INT8S c;
  for(c = id; c < grill.anzahl - 1; c++)
    grill.wuerste[c] = grill.wuerste[c+1];
  grill.anzahl--;
}

void increase_temperatur() {
  INT8U err;
  OSSemPend(GrillSema, 0, &err);
  if (grill.temperatur + TEMP_INC <= TEMP_MAX)
    grill.temperatur += TEMP_INC;
  OSSemPost(GrillSema);
}

void decrease_temperatur() {
  INT8U err;
  OSSemPend(GrillSema, 0, &err);
  if (grill.temperatur - TEMP_INC >= TEMP_MIN)
    grill.temperatur -= TEMP_INC;
  OSSemPost(GrillSema);
}

/*
 * Task: Fleischer
 *
 * Erzeugt neue Wuerste.
 */
void task_fleischer(void* pdata)
{
  INT8U err;
  wurst_ptr wurst;
  msg_ptr msg;
  while (1)
  {
    /* Nachrichten verarbeiten */
    msg = OSMboxPend(FleischerMsgBox, 0, &err);
    if ((*msg) == START_TRINKEN) {
      while((*msg) != STOP_TRINKEN) {
        delete_msg(msg);
        msg = OSMboxPend(FleischerMsgBox, 0, &err);
      }
    } else
    if ((*msg) == CREATE_WURST) {
      OSTimeDlyHMSM(0, 0, 1, 0);
      wurst = create_wurst();
      OSTmrStop(box_timer, OS_TMR_OPT_NONE, NULL, &err);
      OSQPost(BoxQueue, wurst);

      send_fleischer_msg(CREATE_WURST);
    }
    delete_msg(msg);
  }
}

/*
 * Task: Grillmeister
 *
 * Prueft Wuerste, wendet sie und nimmt sie vom Grill.
 */
void task_grillmeister(void* pdata)
{
  INT8U     err;
  INT8U     id;
  wurst_ptr wurst;
  OS_Q_DATA data;
  msg_ptr   msg;
  while (1)
  {
    /* Nachrichten verarbeiten */
    msg = OSMboxAccept(GrillmeisterMsgBox);

    if ((*msg) == START_TRINKEN) {
      while((*msg) != STOP_TRINKEN) {
        delete_msg(msg);
        msg = OSMboxPend(GrillmeisterMsgBox, 0, &err);
      }
    } else
    if ((*msg) == PLACE_WURST) {
      wurst = OSQAccept(BoxQueue, &err);
      /* wurst ist NULL, wenn keine Wurst in Kuehlbox */
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
    } else /* Wenn keine Message, Grill pruefen */
    if (msg == (void *)0) {
      OSSemPend(GrillSema, 0, &err);
      if (grill.anzahl != 0 && !grill.brennt) {
        /* Random Wurst vom Grill in Zange nehmen. */
        id    = rand() % grill.anzahl;
        wurst = grill.wuerste[id];
        grill_remove(id);
        OSSemPost(GrillSema);

        /* Wurst checken */
        OSTimeDlyHMSM(0, 0, 1, 0);
        if (wurst->seiten[wurst->seite] >= 80) {
          if (wurst->seite < 3) {
            /* Wurst wenden */
            OSTimeDlyHMSM(0, 0, 5, 0);
            wurst->seite++;
            OSSemPend(GrillSema, 0, &err);
            grill_add(wurst);
          } else {
            /* Wurst vom Grill nehmen */
            OSTimeDlyHMSM(0, 0, 10, 0);
            delete_wurst(wurst);
          }
        } else {
          /* Wurst zurueck legen */
          OSSemPend(GrillSema, 0, &err);
          grill_add(wurst);
        }
      } else {
        /* Delay, falls noch keine Wurst auf dem Grill -> Prioritaeten! */
        OSTimeDlyHMSM(0, 0, 0, 500);
      }
      OSSemPost(GrillSema);
    }
    delete_msg(msg);
  }
}

/* Task: Physik
 *
 * Berechnet den Braeunungsgrad der Wuerste und entzuendet ggf. den Grill.
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
      i++;
    }

    OSSemPost(GrillSema);
  }
}

/* Task: Feuerwehr
 *
 * Prueft und loescht ggf. den Grill.
 */
void task_feuerwehr(void* pdata)
{
  INT8U err;
  INT8U i;
  wurst_ptr wurst;
  INT8U tmp_anzahl;

  while (1)
  {
    OSTimeDlyHMSM(0, 1, 0, 0);
    printf("[FW] Prueft!\n");
    OSSemPend(GrillSema, 0, &err);
    if (grill.brennt) {
      i = 0;
      tmp_anzahl = grill.anzahl;
      while(i < tmp_anzahl) {
        wurst = grill.wuerste[i];
        grill_remove(i);
        delete_wurst(wurst);
        i++;
      }
      grill.brennt     = false;
      grill.temperatur = TEMP_INIT;
    }
    OSSemPost(GrillSema);
  }
}

/*
 * Task: Eingabe
 *
 * Verarbeitet Tastendruecke.
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
      printf("[EINGABE] '%c' pressed\n", c);
      if (!(c & KEY_SPECIAL)) {
        if (c == 'f' || c == 'F') {
          send_fleischer_msg(START_TRINKEN);
        }
        else if (c == 'v' || c == 'V') {
          send_fleischer_msg(STOP_TRINKEN);
        }
        else if (c == 'd' || c == 'D') {
          send_fleischer_msg(CREATE_WURST);
        }
        else if (c == 'h' || c == 'H') {
          send_grillmeister_msg(PLACE_WURST);
        }
        else if (c == 'g' || c == 'G') {
          send_grillmeister_msg(START_TRINKEN);
        }
        else if (c == 'b' || c == 'B') {
          send_grillmeister_msg(STOP_TRINKEN);
        }
        else if (c == '+')
          increase_temperatur();
        else if (c == '-')
          decrease_temperatur();
      }
    }
  }
}

/*
 * Task: Ausgabe
 *
 * Zeigt den aktuellen Status ueber VGA an.
 */
void task_ausgabe(void* pdata) {
  char        tmp_str[50];
  OS_Q_DATA   data;
  INT8U       err;
  INT8U       box_count;
  INT8U       grill_count;
  INT16U      grill_temp;
  char        grill_brennt[8];
  INT8U       color;

  INT16U i, j, ax, ay, cx, cy, qx, qy, px, py, w_width, s_width;

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

    ay = W_BOT_Y;
    cy = W_TOP_Y;
    py = S_BOT_Y;

    w_width = (B_WIDTH - (grill.anzahl + 1) * W_SPACE) / grill.anzahl;
    s_width = (w_width - 5 * S_SPACE) / 4;

    for (i = 0; i < grill.anzahl; i++) {
      ax = (W_SPACE + w_width) * i + W_SPACE;
      cx = ax + w_width;
      vga_quad(ax, ay, cx, cy, false, 0xFF);
      for (j = 0; j < 4; j++) {
        px = (S_SPACE + s_width) * j + S_SPACE + ax;
        qx = px + s_width;
        qy = S_BOT_Y - (INT16U) grill.wuerste[i]->seiten[j];

        if (j == grill.wuerste[i]->seite){
          if ((INT16U) grill.wuerste[i]->seiten[j] < 80) {
            color = 252;
          } else
          if ((INT16U) grill.wuerste[i]->seiten[j] >= 80){
            color = 124;
          } else
          if ((INT16U) grill.wuerste[i]->seiten[j] >= 100){
            color = 236;
          }
        } else {
          if ((INT16U) grill.wuerste[i]->seiten[j] < 80) {
            color = 144;
          } else
          if ((INT16U) grill.wuerste[i]->seiten[j] >= 80){
            color = 80;
          } else
          if ((INT16U) grill.wuerste[i]->seiten[j] >= 100){
            color = 136;
          }
        }
        vga_quad(px, py, qx, qy, true, color);
      }
      vga_line(ax, S_BOT_Y - 100, cx, S_BOT_Y - 100, 128);
      vga_line(ax, S_BOT_Y - 80, cx, S_BOT_Y - 80, 48);
    }
    vga_flush();
  }
}

/* Starting the whole GRILLFEST */
int main(void)
{
  INT8U err;
  time_t t;

  time(&t);
  srand((unsigned int)t);

  /* Partition und Semaphore fuer Wuerste alloziieren */
  WurstSema = OSSemCreate(1);
  WurstBuff = OSMemCreate(&WurstPart[0][0], MAX_WUERSTE, 4 * sizeof(Wurst), &err);

  /* Partition und Semaphore fuer Messages alloziieren */
  MsgSema = OSSemCreate(1);
  MsgBuff = OSMemCreate(&MsgPart[0][0], MAX_MSG, sizeof(MsgTypes), &err);

  /* Message Box fuer Fleischer alloziieren */
  FleischerMsgBox = OSMboxCreate((void *)0);

  /* Message Box fuer Grillmeister alloziieren */
  GrillmeisterMsgBox = OSMboxCreate((void *)0);

  /* Message Queue fuer Kuehlbox alloziieren */
  BoxQueue = OSQCreate(&BoxMsg[0], MAX_WUERSTE);

  /* Grill und Temperatur Sema */
  GrillSema = OSSemCreate(1);

  /* setup ps2 macro and user callback */
  keyQ = OSQCreate(&keyQBuf[0], OS_KEYBOARD_Q_SIZE);
  ps2_init();
  ps2_keyListener(key_cb);

  /* VGA Init */
  vga_dma_init();

  /* Timer erstellen und starten. */
  box_timer = OSTmrCreate(60 * TICKS_PER_SECOND, 0, OS_TMR_OPT_ONE_SHOT, box_timer_cb, NULL, box_timer_name, &err);
  OSTmrStart(box_timer, &err);

  grill.anzahl     = 0;
  grill.brennt     = false;
  grill.temperatur = TEMP_INIT;

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

  OSTaskCreateExt(task_feuerwehr,
                  NULL,
                  (void *)&stk_feuerwehr[TASK_STACKSIZE-1],
                  FEUERWEHR_PRIORITY,
                  FEUERWEHR_PRIORITY,
                  stk_feuerwehr,
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
