/*
 * Copyright (c) 2016 by Anders Østevik
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Last modified: 2016-04-14
 * 
 * Notes: 
 *       The files "signals_switches.txt" and "signals_probes.txt" must be included in the debug/release folder before running program.
 *       Windows: 
 *              Make sure to include the pdcurses.dll file in the debug/release folder.
 *      The CommandEntry struct and its idea of use is borrowed from the "Stand-alone shell" open source software by David I. Bell.
 *
 */
 #ifndef MAIN_H
#define MAIN_H

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "rs232.h"
#include "timer.h"

#include "signals.h"
#include "cmds.h"

/*
 * Globals:
 *    cport_nr -> Comport number (see rs232.c)
 *    bdrate -> Baud Rate (see rs232.c)
 *    GBT_SWTABLE -> Table that stores gbt-switch data (used in cmds.c)
 *    sSwitch -> Signal struct containing information about GBT switches
 *    sProbe -> Signal struct containing information about GBT probes
 */

extern int cport_nr;        /* See rs232.c for comports*/
extern int cport_status;
extern char cport_mode[];  //8 data bits, No parity, 1 stop bit
extern int bdrate;       /* See rs232.c for baudrates*/

extern Signal sSwitch[MAX_SWITCHES];
extern Signal sProbe[MAX_PROBES];

/*
 * GBT_SWTABLE holds the switch data (see signals_switch.txt for more information)
 * use write command to change data values
 * The software will continously write (use "write cont") to the GBT-fpga with the switch values from this table.
 * This is to prevent any switch bits from flipping (unfixed bug), which happens randomly when not writing to it.
 */
extern Byte GBT_SWTABLE[MAX_SWITCHES];

#endif