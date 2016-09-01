/*************************************************************************
* Copyright (c) 2004 Altera Corporation, San Jose, California, USA.      *
* All rights reserved. All use of this software and documentation is     *
* subject to the License Agreement located at the end of this file below.*
**************************************************************************
* Description:                                                           *
* The following is a simple hello world program running MicroC/OS-II.The * 
* purpose of the design is to be a very simple application that just     *
* demonstrates MicroC/OS-II running on NIOS II.The design doesn't account*
* for issues such as checking system call return codes. etc.             *
*                                                                        *
* Requirements:                                                          *
*   -Supported Example Hardware Platforms                                *
*     Standard                                                           *
*     Full Featured                                                      *
*     Low Cost                                                           *
*   -Supported Development Boards                                        *
*     Nios II Development Board, Stratix II Edition                      *
*     Nios Development Board, Stratix Professional Edition               *
*     Nios Development Board, Stratix Edition                            *
*     Nios Development Board, Cyclone Edition                            *
*   -System Library Settings                                             *
*     RTOS Type - MicroC/OS-II                                           *
*     Periodic System Timer                                              *
*   -Know Issues                                                         *
*     If this design is run on the ISS, terminal output will take several*
*     minutes per iteration.                                             *
**************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include "includes.h"
#include "ps2/ps2.h"
#include "ps2/ps2_keyscan.h"
#include "vga/vga.h"
#include "vga/vga_font.h"

/* Konstanten */
#define MAX_WUERSTE (100)
#define TICKS_PER_SECOND (10)

/* Definition of Task Stacks */
#define   TASK_STACKSIZE       2048
OS_STK    stk_fleischer[TASK_STACKSIZE];
OS_STK    stk_eingabe[TASK_STACKSIZE];
OS_STK    stk_grillmeister[TASK_STACKSIZE];

/* Definition of Task Priorities */

#define EINGABE_PRIORITY        10
#define FLEISCHER_PRIORITY      22
#define GRILLMEISTER_PRIORITY   16
#define AUSGABE_PRIORITY        25

/* keyboard data queue */
#define OS_KEYBOARD_Q_SIZE	10
OS_EVENT *keyQ;
void *keyQBuf[OS_KEYBOARD_Q_SIZE];

/* Datenstrukturen */

typedef struct {
  INT8U seiten[4];
} Wurst;

typedef Wurst * wurst_ptr;

typedef struct {

} Grill;

/* Wurst Semaphore und Speicher */

OS_EVENT * WurstSema;
OS_MEM   * WurstBuff;
INT8U      WurstPart[MAX_WUERSTE][sizeof(Wurst)];

/* FleischerTrinkt Semaphore und Speicher */

OS_EVENT * FleischerBusySema;

BOOLEAN fleischer_busy = true;

/* Kuehlbox Semaphore und Speicher */

OS_EVENT * BoxQueue;
void     * BoxMsg[MAX_WUERSTE];

OS_TMR * box_timer;
char box_timer_name[9] = "box_timer";

/* keyboard data queue */
#define OS_KEYBOARD_Q_SIZE  10
OS_EVENT *keyQ;
void *keyQBuf[OS_KEYBOARD_Q_SIZE];

void toggle_fleischer_busy() {
  INT8U err;
  if (fleischer_busy)
    OSSemPost(FleischerBusySema);
  else
    OSSemPend(FleischerBusySema, 0, &err);

  fleischer_busy = !fleischer_busy;
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

  OSMemPut(WurstBuff, wurst);
  OSSemPost(WurstSema);

  return wurst;
}

/* Fleischer: erzeugt Würste */
void task_fleischer(void* pdata)
{
  INT8U err;
  wurst_ptr wurst;
  OS_Q_DATA data;
  while (1)
  {
    OSSemPend(FleischerBusySema, 0, &err);

    OSTimeDlyHMSM(0, 0, 1, 0);
    wurst = create_wurst();

    OSQPost(BoxQueue, wurst);
    OSTmrStop(box_timer, OS_TMR_OPT_NONE, NULL, &err);

    OSSemPost(FleischerBusySema);

    /* Temp Ausgabe */
    OSQQuery(BoxQueue, &data);
    printf("[F]Anzahl Würste: %i\n", data.OSNMsgs);
  }
}

/* Grillmeister: brät Würste */
void task_grillmeister(void* pdata)
{
  INT8U err;
  OS_Q_DATA data;
  wurst_ptr wurst;
  while (1)
  {
    /*OSSemPend(FleischerBusySema, 0, &err);*/

    OSTimeDlyHMSM(0, 0, 5, 0);
    wurst = OSQPend(BoxQueue, 0, &err);

    OSQQuery(BoxQueue, &data);
    if (data.OSNMsgs <= 0)
      OSTmrStart(box_timer, &err);
    printf("[G]Anzahl Würste: %i\n", data.OSNMsgs);

    /*OSSemPost(FleischerBusySema);*/
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
      // poor mans terminal emulation
      if (!(c & KEY_SPECIAL)) {
        if (c == 'f' || c == 'F')
          toggle_fleischer_busy();
       /* else if (c == 'g' || c == 'G')
          toggle_grillmeister_trinken();*/
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

/* The main function creates two task and starts multi-tasking */
int main(void)
{
  INT8U err;

  /* Partition und Semaphore für Würste alloziieren */
  WurstSema = OSSemCreate(1);
  WurstBuff = OSMemCreate(WurstPart, MAX_WUERSTE, sizeof(Wurst), &err);

  /* Semaphore für FleischerTrinkt alloziieren */
  FleischerBusySema = OSSemCreate(0);

  /* Message Queue für Kühlbox alloziieren */
  BoxQueue = OSQCreate(&BoxMsg[0], MAX_WUERSTE);

  /* setup ps2 macro and user callback */
  keyQ = OSQCreate(&keyQBuf[0], OS_KEYBOARD_Q_SIZE);
  ps2_init();
  ps2_keyListener(key_cb);

  /* Timer erstellen und starten. */

  box_timer = OSTmrCreate(5 * TICKS_PER_SECOND, 0, OS_TMR_OPT_ONE_SHOT, box_timer_cb, NULL, box_timer_name, &err);
  OSTmrStart(box_timer, &err);

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
               
  OSTaskCreateExt(task_eingabe,
                  NULL,
                  (void *)&stk_eingabe[TASK_STACKSIZE-1],
                  EINGABE_PRIORITY,
                  EINGABE_PRIORITY,
                  stk_eingabe,
                  TASK_STACKSIZE,
                  NULL,
                  0);
  OSStart();
  return 0;
}

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2004 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
* This agreement shall be governed in all respects by the laws of the State   *
* of California and by the laws of the United States of America.              *
* Altera does not recommend, suggest or require that this reference design    *
* file be used in conjunction or combination with any other product.          *
******************************************************************************/
