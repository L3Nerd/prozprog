/**
 * @file box.c
 * Modul fuer die Kuehlbox.
 *
 * @author inf1073   Lennart Kindermann
 * @author inf100849 Oliver Westphal
 */

/* System-Header einbinden */

#include "includes.h"

/* Lokale Header einbinden */

#include "fleischer.h"
#include "wurst_helper.h"
#include "msg_helper.h"

/* lokale Konstanten und Makros */
/** @privatesection */

#define TICKS_PER_SECOND 10
#define MAX_BOX_COUNT    30

/* Modulvariablen */

/* Kuehlbox Semaphore, Speicher und Timer */
OS_EVENT * BoxQueue;
void     * BoxMsg[MAX_BOX_COUNT];
OS_TMR   * box_timer;
char     * box_timer_name = "box_timer";


/* Forwarddeklarationen */

void box_timer_cb (void *ptmr, void *callback_arg);

/* Öffentliche Funktionen */
/** @publicsection */

/**
 * Gibt die Anzahl an Wuersten in der Box zurueck.
 *
 * @returns Die Anzahl.
 */
INT16U box_get_anzahl (void)
{
  OS_Q_DATA data;

  OSQQuery(BoxQueue, &data);
  return data.OSNMsgs;
}

/**
 * Fuegt eine Wurst der Kuehlbox (MsgQueue) hinzu und stoppt den "Leere-Box"-Timer.
 *
 * @param[in] wurst Wurst-Pointer.
 */
void box_add (wurst_ptr wurst)
{
  INT8U err;
  OSTmrStop(box_timer, OS_TMR_OPT_NONE, NULL, &err);
  OSQPost(BoxQueue, wurst);
}

/**
 * Entfernt eine Wurst aus Kuehlbox (MsgQueue) und startet den "Leere-Box"-Timer,
 * falls keine Wurst mehr verblieben ist.
 *
 * @returns Wurst-Pointer, falls keine in der Box ist wurd NULL zurück gegeben.
 */
wurst_ptr box_take (void)
{
  INT8U     err;
  wurst_ptr wurst;

  wurst = OSQAccept(BoxQueue, &err);

  /* Wenn Box leer ist, Timer neustarten. */

  if (box_get_anzahl() == 0) {
    OSTmrStart(box_timer, &err);
  }

  return wurst;
}

/**
 * Initialisiert die Kuehlbox Queue und den Timer.
 */
void init_box (void)
{
  INT8U err;

  /* Message Queue fuer Kuehlbox alloziieren */
  BoxQueue = OSQCreate(&BoxMsg[0], MAX_BOX_COUNT);

  /* Timer erstellen und starten. */
  box_timer = OSTmrCreate(60 * TICKS_PER_SECOND, 0, OS_TMR_OPT_ONE_SHOT, box_timer_cb, NULL, box_timer_name, &err);
  OSTmrStart(box_timer, &err);
}

/* Private Funktionen */
/** @privatesection */

/**
 * Callback fuer den Kuehlbox-Timer, sendet dem Fleischer die Nachricht, Wuerste herzustellen.
 */
void box_timer_cb (void *ptmr, void *callback_arg) {
  send_fleischer_msg(CREATE_WURST);
}
