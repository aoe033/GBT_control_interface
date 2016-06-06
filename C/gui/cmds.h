
#ifndef CMDS_H
#define CMDS_H

#ifdef _WIN32
  #include <curses.h>
  #include <Windows.h>
#else
  #include <ncurses.h>
  #include <unistd.h>
  //#include <fcntl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "signals.h"
#include "main.h"


/*
 * Built-in command functions.
 */
extern  int do_exit(WINDOW *win, int *fy, int argc, char * argv);
extern  int do_read(WINDOW *win, int *fy, int argc, char * argv);
extern  int do_write(WINDOW *win, int *fy, int argc, char * argv);

extern  int do_openCom(WINDOW *win, int *fy, int argc, char * argv);
extern  int do_closeCom(WINDOW *win, int *fy, int argc, char * argv);
extern  int do_status(WINDOW *win, int *fy, int argc, char * argv);

extern  int do_cmd(WINDOW *win, int *fy, int argc, char *sCmd, char *sArgs);
extern  int do_commandline(WINDOW *win, int *fy, char *myCmd);
#endif