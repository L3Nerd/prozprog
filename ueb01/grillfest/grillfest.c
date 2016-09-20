/**
 * @file grillfest.c
 *
 * Initialisieren der Grillfest Aufgabe -> Tasks und Datenstrukturen alloziieren.
 * Sowie µC/OS-2 starten.
 *
 * @author inf1073   Lennart Kindermann
 * @author inf100849 Oliver Westphal
 */

#include "includes.h"
#include <time.h>
#include <stdlib.h>

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
