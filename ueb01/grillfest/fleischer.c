/**
 * @file fleischer.c
 * Modul fuer den Fleischer-Task.
 *
 * @author inf1073   Lennart Kindermann
 * @author inf100849 Oliver Westphal
 */

/* System-Header einbinden */

#include "includes.h"

/* Lokale Header einbinden */

#include "msg_helper.h"
#include "wurst_helper.h"
#include "box.h"

/* lokale Konstanten und Makros */
/** @privatesection */

#define TASK_STACKSIZE     2048
#define FLEISCHER_PRIORITY 22

/* Modulvariablen */

OS_STK    stk_fleischer[TASK_STACKSIZE];
OS_EVENT *FleischerMsgBox;

/* Forwarddeklarationen */

void task_fleischer (void* pdata);

/* Öffentliche Funktionen */
/** @publicsection */

/**
 * Legt eine Nachricht vom Typ MsgTypes in die MsgBox ab.
 *
 * @param[in] msg_type Die Nachricht, die abgelegt werden soll.
 */
void send_fleischer_msg (MsgTypes msg_type) {
  INT8U   err;
  msg_ptr msg = create_msg(msg_type);
  err = OSMboxPost(FleischerMsgBox, msg);
  if (err != OS_ERR_NONE)
    delete_msg(msg);
}

/**
 *  Initialisiert den Fleischer Task und MsgBox.
 */
void init_fleischer (void)
{
  /* Message Box fuer Fleischer alloziieren */
  FleischerMsgBox = OSMboxCreate((void *)0);

  OSTaskCreateExt(task_fleischer,
                  NULL,
                  (void *)&stk_fleischer[TASK_STACKSIZE-1],
                  FLEISCHER_PRIORITY,
                  FLEISCHER_PRIORITY,
                  stk_fleischer,
                  TASK_STACKSIZE,
                  NULL,
                  0);
}


/* Private Funktionen */
/** @privatesection */

/**
 * Task: Fleischer
 *
 * Der Fleischer beginnt neue Wuerste zu erzeugen und die Kuehlbox zu befüllen, wenn er
 * vom Benutzer dazu aufgefordert wird oder die Kuehlbox laenger als 60 Sekunden leer ist.
 * Fuer das Herstellen einer Wurst benoetigt er eine Sekunde.
 *
 * Ressourcen: Box -> add
 *
 * @in FleischerMsgBox
 *   START_TRINKEN: Pausiert den Fleischer, bis er die Nachricht STOP_TRINKEN erhaelt.
 *   STOP_TRINKEN:  Beendet die Pause.
 *   CREATE_WURST:  Startet das Erstellen von Wuersten im Sekundentakt.
 */
void task_fleischer (void* pdata)
{
  INT8U     err;
  wurst_ptr wurst;
  msg_ptr   msg;

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
      box_add(wurst);
      /* Selbst Nachricht schicken, um weiter zu machen.
       * Falls zwischenzeitig START_TRINKEN eingagengen ist, wird getrunken -> da MsgBox. */
      send_fleischer_msg(CREATE_WURST);
    }
    delete_msg(msg);
  }
}
