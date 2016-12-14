/**
 * @file grillfest.c
 *
 * Initialisieren der Grillfest Aufgabe -> Tasks und Datenstrukturen alloziieren.
 * Sowie �C/OS-2 starten.
 *
 * @author inf1073   Lennart Kindermann
 * @author inf100849 Oliver Westphal
 */

#include "includes.h"
#include <time.h>
#include <stdlib.h>

#include "ausgabe.h"
#include "box.h"
#include "eingabe.h"
#include "feuerwehr.h"
#include "fleischer.h"
#include "grill.h"
#include "grillmeister.h"
#include "msg_helper.h"
#include "physik.h"
#include "wurst_helper.h"

/* Programmeinstiegspunkt. */
int main(void)
{
  time_t t;

  time(&t);
  srand((unsigned int)t);

  init_wurst_helper();
  init_msg_helper();
  init_box();
  init_grill();
  init_eingabe();
  init_ausgabe();
  init_fleischer();
  init_grillmeister();
  init_physik();
  init_feuerwehr();

  OSStart();
  return 0;
}
