/**
 * @file eingabe.c
 * Modul fuer den Eingabe-Task.
 *
 * @author inf1073   Lennart Kindermann
 * @author inf100849 Oliver Westphal
 */

/* System-Header einbinden */

#include "includes.h"
#include <stdio.h>

/* Lokale Header einbinden */

#include "grill.h"
#include "fleischer.h"
#include "grillmeister.h"
#include "msg_helper.h"
#include "ps2/ps2.h"
#include "ps2/ps2_keyscan.h"

/* lokale Konstanten und Makros */
/** @privatesection */

#define TASK_STACKSIZE     2048
#define EINGABE_PRIORITY   10
#define OS_KEYBOARD_Q_SIZE 10

/* Modulvariablen */

OS_STK stk_eingabe[TASK_STACKSIZE];

/* keyboard data queue */
OS_EVENT *keyQ;
void     *keyQBuf[OS_KEYBOARD_Q_SIZE];

/* Forwarddeklarationen */

void key_cb (uint8_t key);
void task_eingabe (void* pdata);

/* �ffentliche Funktionen */
/** @publicsection */

/**
 *  Initialisiert den Eingabe-Task, sowie die Key-Queue und den Key-Listener.
 */
void init_eingabe (void)
{
  /* setup ps2 macro and user callback */
  keyQ = OSQCreate(&keyQBuf[0], OS_KEYBOARD_Q_SIZE);
  ps2_init();
  ps2_keyListener(key_cb);

  OSTaskCreateExt(task_eingabe,
                  NULL,
                  (void *)&stk_eingabe[TASK_STACKSIZE-1],
                  EINGABE_PRIORITY,
                  EINGABE_PRIORITY,
                  stk_eingabe,
                  TASK_STACKSIZE,
                  NULL,
                  0);
}

/* Private Funktionen */
/** @privatesection */

/**
 * keyboard hook
 *
 * get keyboard data from isr and post to queue (if space available)
 */
void key_cb (uint8_t key)
{
  uint8_t err = OS_ERR_NONE;

  // scheduler mark for isr code
  OSIntEnter();

  err = OSQPost(keyQ, (void *) (uint32_t) key);
  if (err == OS_ERR_Q_FULL)
    printf("keyboard queue full, key (%02X) lost\n", key);

  OSIntExit();
}

/**
 * Task: Eingabe
 *
 * Verarbeitet User-Eingaben:
 *   f/F : Fleischer Trinken geben
 *   v/V : Fleischer Trinken entziehen
 *   d/D : Fleischer Wuerste produzieren lassen
 *   g/G : Grillmeister Trinken geben
 *   b/B : Grillmeister Trinken entziehen
 *   h/H : Grillmeister Wurst auf den Grill legen lassen
 *   +   : Grilltemperatur erhoehen
 *   -   : Grilltemperatur verringern
 *
 * Ressourcen: Fleischer    -> send_msg
 *             Grillmeister -> send_msg
 *             Grill        -> un/lock
 *
 */
void task_eingabe (void* pdata) {
  uint8_t   qErr = OS_ERR_NONE;
  uint32_t *qMsg;

  while (1) {
    /* wait for keyboard input */
    qMsg = OSQPend(keyQ, 0, &qErr);
    if (qErr == OS_ERR_NONE) {
      /*  convert into character */
      uint8_t c = (uint32_t)qMsg & 0xFF;
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
        else if (c == '+') {
          grill_lock();
          grill_increase_temperatur();
          grill_unlock();
        }
        else if (c == '-') {
          grill_lock();
          grill_decrease_temperatur();
          grill_unlock();
        }
      }
    }
  }
}
