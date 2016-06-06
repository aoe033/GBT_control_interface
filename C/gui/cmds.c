/*
 * Copyright (c) 2016 by Anders Østevik
 * Permission is granted to use, distribute, or modify_pos this source,
 * provided that this copyright notice remains intact.
 *
 * Last modified: 2016-04-12
 * 
 * Notes: 
 *      The CommandEntry struct and its use is taken from "Stand-alone shell" by David I. Bell.
 *
 */
 
#include "cmds.h"

/*
 * The special maximum argument value which means that there is
 * no limit to the number of arguments on the command line.
 */
#define	INFINITE_ARGS	0x7fffffff

int
do_exit(WINDOW *win, int *fy, int argc,  char * argv)
{
  int status = (int)(argv[0] - '0');
  int fy_pos = *fy; //cursor_y position
  if (status < 0 || status > 1) 
  {
    mvwprintw(win, fy_pos++, 1, "Invalid parameter.. Use '1' or '0'");
    return fy_pos;
  }
  else
  {
    mvwprintw(win, fy_pos++, 1, "Exiting program. Status %d...", status);
    exit(status);
  }
  return fy_pos;
}

int
do_status(WINDOW *win, int *fy, int argc,  char * argv)
{
  int fy_pos = *fy; //cursor_y position
  
  if (strcmp(argv, "settings") == 0)
  {
    mvwprintw(win, fy_pos++, 1, "Current settings: \n Baud Rare -> %d\n COMport nr.-> %d (see rs232.c) \n COMport mode -> %s \n", 
    bdrate, cport_nr, cport_mode);
  }
  else  if (strcmp(argv, "comport") == 0)
  {
    char *comstatus = (char*) malloc (10 * sizeof(char));
    if (cport_status == 0) comstatus = "closed";
    else comstatus = "open";
    mvwprintw(win, fy_pos++, 1, "COMport status -> %s", comstatus);
    //free(comstatus);
  }
  else
  {
    mvwprintw(win, fy_pos++, 1, "Invalid parameter..");
  }
  return fy_pos;
}

int
do_openCom(WINDOW *win, int *fy, int argc,  char * argv)
{
  int fy_pos = *fy; //cursor_y position

  if (strcmp(argv, "comport") == 0) 
  {
    if (cport_status == 0)
    {
      if(RS232_OpenComport(cport_nr, bdrate, cport_mode))
      {
        mvwprintw(win,fy_pos++,1, "Can not open comport..");
        return fy_pos;
      }
      else
      {
        cport_status = 1;
        mvwprintw(win, fy_pos++, 1, "Comport Open! \n Type \"status [settings]\" to see current comport settings.");
      }
    }
    
  }
  else
  {
    mvwprintw(win, fy_pos++, 1, "Invalid parameter..");
  }
  return fy_pos;
}

int
do_closeCom(WINDOW *win, int *fy, int argc,  char * argv)
{
  int fy_pos = *fy; //cursor_y position

  if (strcmp(argv, "comport") == 0) 
  {
    if (cport_status == 1)
    {
        RS232_CloseComport(cport_nr);
        cport_status = 0;
        mvwprintw(win, fy_pos++, 1, "Closing Comport..");
    }
    else
    {
        mvwprintw(win, fy_pos++, 1, "Comport already closed! \n Type \"status [comport]\" to see comport status.");
    }
  }
  else
  {
    mvwprintw(win, fy_pos++, 1, "Invalid parameter..");
  }
  return fy_pos;
}

int
do_read(WINDOW *win, int *fy, int argc, char * argv)
{
  char *string = strdup(argv); //strtok will change the string, safecopy in dedicated char
  char *stok = NULL;      //stores the string tokens 
  int fy_pos = *fy; //cursor_y position
  
  for (stok = strtok (string, " "); stok; stok = strtok (NULL, " \n"))
  {
    if(strcmp(stok, "cont") == 0)
    {
      //Read continously
      mvwprintw(win, fy_pos++, 1, "Reading continously");  
      /*
       * Flag the send/receive module to continously send read requests of all addresses. 
       */
      return fy_pos;
    }  
    else if(strcmp(stok, "all") == 0)
    {
      //Read all
      mvwprintw(win, fy_pos++, 1, "Reading all");  
      return fy_pos;
    }  
    
    if ((stok[3]) || (!stok[2]) || (!stok[1])) 
    {
      mvwprintw(win, fy_pos++, 1, "Error: Invalid argument %s...", stok);
      return fy_pos;
    }
    
    //Converted to Byte
    unsigned char n[2];
    n[0] = stok[1] - '0';
    n[1] = stok[2] - '0';
    unsigned char i_c = (n[0] * 10) + n[1]; 
    
    if (i_c > GBT_DATA_WIDTH-1)
    {
      mvwprintw(win, fy_pos, 1, "Warning: Address 0x%x is out of scope. Read ignored.\n", i_c);  
    }
    else if (stok[0] == 'S' || stok[0] == 's')
    {
      unsigned char Adr = (unsigned char) i_c;
      mvwprintw(win, fy_pos, 1, "Send read request to 0x%x: %s\n", Adr, stok);   
    }
    else if (stok[0] == 'P' || stok[0] == 'p')
    {
      unsigned char Adr = (unsigned char) i_c + 36;
      mvwprintw(win, fy_pos, 1, "Send read request to 0x%x: %s\n", Adr, stok);   
    }
    else
    {
      mvwprintw(win, fy_pos++, 1, "Error: Invalid argument %s...", stok);
      return fy_pos;
    }    
  }
  free(string);
  free(stok);
  
  return fy_pos;
}

int
do_write(WINDOW *win, int *fy, int argc, char * argv)
{
    char *string = strdup(argv); //strtok will change the string, safecopy in dedicated char
    char *stok = NULL;      //stores the string tokens 
    int itok = 0; 
    int fy_pos = *fy; //cursor_y position
    
    for (stok = strtok (string, " "); stok; stok = strtok (NULL, " \n"))
    {
        unsigned char data;
        if (itok == 0)  // Check data bit, should be the first char encountered by strtok
        {
          if(strcmp(stok, "cont") == 0)
          {
            //Write continously
           mvwprintw(win, fy_pos++, 1, "Write continously to all switches from table");  
         /*
           * Flag the send/receive module to continously send write requests to all switches. 
           */
            return fy_pos;
          }  
          else if(strcmp(stok, "all") == 0)
          {
            //Write to all switches from table
            mvwprintw(win, fy_pos++, 1, "Writing to all switches from table");  
            return fy_pos;
          }  
          
          data = stok[0] - '0';
          if (data < 0x00 || data > 0x01) 
          {
             mvwprintw(win, fy_pos++, 1, "Error: invalid data %x", data);
             return fy_pos;
          }
        }
        else
        {
          if ((stok[3]) || (!stok[2]) || (!stok[1])) 
          {
             mvwprintw(win, fy_pos++, 1, "Error: Invalid argument %s...", stok);
             return fy_pos;
          }
          
          //Converted to Byte
          unsigned char n[2];
          n[0] = stok[1] - '0';
          n[1] = stok[2] - '0';
          unsigned char i_c = (n[0] * 10) + n[1]; 
          
          if (stok[0] == 'S' || stok[0] == 's')
          {
            if (i_c > MAX_SWITCHES-1)
            {
               mvwprintw(win, fy_pos, 1, "Warning: Address 0x%x is out of scope. Write ignored.\n", i_c);  
            }    
            unsigned char Adr = (unsigned char) i_c;
            mvwprintw(win, fy_pos, 1, "Send write request to 0x%x: %s with value %x\n", Adr, stok, data);   

            /*
              *  In this version, one can only write to the table. 
              * The table-values are continously sent via uart to fpga to set the switches.  
              */
            GBT_SWTABLE[i_c] = data;
            /*
              * Signal_setData sets the switches here for test purposes only. 
              * In reality, the switches are to be set by incoming read data from the real switches in the fpga. 
              */
            Signal_setData(sSwitch[i_c], GBT_SWTABLE[i_c]);     
          }
          else if (stok[0] == 'P' || stok[0] == 'p')
          {
             mvwprintw(win, fy_pos, 1, "Warning: Attempting to write to a probe. Write to %s ignored.\n", stok);        
          }
          else
          {
             mvwprintw(win, fy_pos++, 1, "Error: Invalid argument %s...", stok);
            return fy_pos;
          }    
        }
        itok++;
    }
    
    //printf ("success = %d",success);
    free(string);
    free(stok);
    return fy_pos;
}


/*
 * Command table struct
 */
typedef struct
{
  const char * name;
  int (*func)(WINDOW *win, int *fy, int argc, char * argv);
  int minArgs;
  int maxArgs;
  const char * description;
  const char * usage;
} CommandEntry;

/*
 * The table of built-in commands.
 * This is terminated with an entry containing NULL values.
 */
static const CommandEntry commandEntryTable[] =
{
    {
        "read",    do_read,    1,  INFINITE_ARGS,
        "Read from a GBT switch or probe. For continously read of data, use \"cont\" as argument",
        "[[name0] [name1] ...]"
    },

    {
        "write",	do_write, 1,  INFINITE_ARGS,
        "Write to a GBT switch. For continously write to GBT, use \"cont\" as argument",
        "[data [name1] [name2] ...]"
    },
    
    {
        "status",	do_status, 1,  2,
        "check COMport settings or status",
        "[settings/comport]"
    },
    
    {
        "open",	do_openCom, 1,  2,
        "Opens the COMport",
        "[comport]"
    },
    
    {
        "close",	do_closeCom, 1,  2,
        "Closes the COMport",
        "[comport]"
    },
    
    {
        "exit",    do_exit,    1,  1,
        "Exits the program",
        "[status (1 or 0)]"
    },
    
    {
        NULL,       0,      0,  0,
        NULL,
        NULL
    }
};

/*
 * Compares input to Command table.
 * If parts of command as input, show full command name
 * If command without any arguments as input, show name, description and usage
 * Else, execute the command function
 */
int 
do_cmd(WINDOW *win, int *fy, int argc, char *sCmd, char *sArgs) 
{
  const CommandEntry * entry;
  char * str;
  str = NULL;
  int  success = 0;
  int fy_pos = *fy; //cursor_y position
  
  if (argc < 1) return 0; //If there is no commands, return
  else str = sCmd;
  
  /*
   * Check for an exact match
   */
  if (str)
  {
      for (entry = commandEntryTable; entry->name; entry++)
      {
          if (strcmp(sCmd, entry->name) == 0)    // If the sCmd matches a command entry, excecute function
          {
              if ((argc < 2 || sArgs == NULL)) //If there is no arguments, describe the command
              {
                mvwprintw(win, fy_pos, 1, "%s", entry->description);
                mvwprintw(win, fy_pos++, 1, "usage: %s %s", entry->name, entry->usage);
                success = 1;
                return fy_pos;
              }
              return entry->func(win, &fy_pos, --argc, sArgs); //Excecute command function
          }
      }
  }
  
    /*
     * Print short information about commands which contain the
     * specified word.
     */
    for (entry = commandEntryTable; entry->name; entry++)
    {
      if ((str == NULL) || (strstr(entry->name, str) != NULL) ||
          (strstr(entry->usage, str) != NULL))
      {
       mvwprintw(win, fy_pos, 1, "%-10s %s\n", entry->name, entry->usage);
       success = 1;
      }
    }
    
  if (success == 0)  mvwprintw(win, fy_pos++, 1, "Error: Command not recognized.");
  return fy_pos;
}

int countArgs(const char sentence[])
{
    int counted = 0; // result
    // state:
    const char* it = sentence;
    int inword = 0;

    do switch(*it) {
        case '\0': 
        case ' ': case '\t': case '\n': case '\r': // TODO others?
            if (inword) { inword = 0; counted++; }
            break;
        default: inword = 1;
    } while(*it++);

    return counted;
}

int 
do_commandline(WINDOW *win, int *fy, char *myCmd)
{
    char sCmd[30];
    char sArgs[100];
    //int fy_pos = fy; //cursor_y position
    sscanf(myCmd, "%s %[^\n]", sCmd, sArgs);
    
    //Prevent overflow
    sCmd[30] = '\0';
    sArgs[100] = '\0';
    
    int num = countArgs(myCmd);
    int fy_pos = *fy;
    fy_pos = do_cmd(win, &fy_pos, num, sCmd, sArgs);
    
    return fy_pos;
}