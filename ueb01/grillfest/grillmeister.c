/**
 * @file grillmeister.c
 * Modul fuer den Grillmeister-Task.
 *
 * @author inf1073   Lennart Kindermann
 * @author inf100849 Oliver Westphal
 */

/* System-Header einbinden */

#include "includes.h"

/* Lokale Header einbinden */

#include "wurst_helper.h"
#include "msg_helper.h"
#include "box.h"
#include "grill.h"

/* lokale Konstanten und Makros */
/** @privatesection */

#define TASK_STACKSIZE        2048
#define GRILLMEISTER_PRIORITY 16

/* Modulvariablen */

OS_STK    stk_grillmeister[TASK_STACKSIZE];
OS_EVENT *GrillmeisterMsgBox;

/* Forwarddeklarationen */

void task_grillmeister (void* pdata);

/* Öffentliche Funktionen */
/** @publicsection */

/**
 * Legt eine Nachricht vom Typ MsgTypes in die MsgBox ab.
 *
 * @param[in] msg_type Die Nachricht, die abgelegt werden soll.
 */
void send_grillmeister_msg (MsgTypes msg_type) {
  INT8U   err;
  msg_ptr msg = create_msg(msg_type);

  err = OSMboxPost(GrillmeisterMsgBox, msg);
  if (err != OS_ERR_NONE) {
    delete_msg(msg);
  }
}

/**
 *  Initialisiert den Grillmeister Task und MsgBox.
 */
void init_grillmeister (void)
{
  /* Message Box fuer Grillmeister alloziieren */
  GrillmeisterMsgBox = OSMboxCreate((void *)0);

  OSTaskCreateExt(task_grillmeister,
                  NULL,
                  (void *)&stk_grillmeister[TASK_STACKSIZE-1],
                  GRILLMEISTER_PRIORITY,
                  GRILLMEISTER_PRIORITY,
                  stk_grillmeister,
                  TASK_STACKSIZE,
                  NULL,
                  0);
}

/* Private Funktionen */
/** @privatesection */

/**
 * Task: Grillmeister
 *
 * Der Grillmeister platziert auf Anforderung durch den Benutzer jeweils eine Wurst aus
 * der Kuehlbox auf dem Grill. Hierfuer benötigt er 5 Sekunden.
 *
 * Wenn der Grillmeister nichts anderes zu tun hat, kontrolliert er eine zufaellig
 * ausgewaehlte Wurst auf ihren Braeunungsgrad. Ist diese Wurst von der Unterseite
 * mindestens zu 80% gebraeunt, dreht er sie auf die naechste Seite oder entfernt sie vom
 * Grill, wenn sie von allen Seiten mindestens zu 80% gebraeunt ist. Die Kontrolle dauert
 * eine Sekunde, das Wenden 5 Sekunden und das Entfernen vom Grill 10 Sekunden.
 *
 * Ressourcen: Grill -> un/lock
 *             Box   -> box_take
 *
 * @in GrillmeisterMsgBox
 *   START_TRINKEN: Pausiert den Grillmeister, bis er die Nachricht STOP_TRINKEN erhaelt.
 *   STOP_TRINKEN:  Beendet die Pause.
 *   PLACE_WURST:   Plaziert eine Wurst aus der Kuehlbox auf den Grill.
 */
void task_grillmeister(void* pdata)
{
  INT8U     err;
  INT8U     id;
  wurst_ptr wurst;
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
      wurst = box_take();
      /* wurst ist NULL, wenn keine Wurst in Kuehlbox */
      if (wurst != (void *)0) {
        OSTimeDlyHMSM(0, 0, 5, 0);
        grill_lock();
        grill_add(wurst);
        grill_unlock();
      }
    } else /* Wenn keine Message, Grill pruefen */
    if (msg == (void *)0) {
      grill_lock();
      if (grill_get_anzahl() != 0 && !grill_get_brennt()) {
        /* Random Wurst vom Grill in Zange nehmen. */
        id    = rand() % grill_get_anzahl();
        wurst = grill_get_wurst(id);
        grill_remove(id);
        grill_unlock();

        /* Wurst checken */
        OSTimeDlyHMSM(0, 0, 1, 0);
        if (wurst->seiten[wurst->seite] >= 80) {
          if (wurst->seite < 3) {
            /* Wurst wenden - 5 Sec*/
            OSTimeDlyHMSM(0, 0, 5, 0);
            wurst->seite++;
            grill_lock();
            grill_add(wurst);
          } else {
            /* Wurst verzehren - 10 Sec */
            OSTimeDlyHMSM(0, 0, 10, 0);
            delete_wurst(wurst);
          }
        } else {
          /* Wurst zurueck legen */
          grill_lock();
          grill_add(wurst);
        }
      } else {
        /* Delay, falls noch keine Wurst auf dem Grill -> Prioritaeten! */
        OSTimeDlyHMSM(0, 0, 0, 500);
      }
      grill_unlock();
    }
    delete_msg(msg);
  }
}
