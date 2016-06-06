
/*
 * Defined in cmds.h:
 * #ifdef _WIN32
 * #include <curses.h>
 * #include <Windows.h>
 * #else
 * #include <ncurses.h>
 * #include <unistd.h>
 * #include <fcntl.h>
 *#endif
 */

#include "main.h"

/*
 * Version 0.1: "Barely Working"
 */
static const char * const   version = "0.1";

#ifdef _WIN32
  #define MAIN_SLEEP 1 //Tx-while loop repeat every TX_TIMER milliseconds
  #define RX_TIMER 10 //Rx-timer repeat every RX_TIMER milliseconds	
#else
  #define MAIN_SLEEP 1000 //If compiling with linux, use microseconds
  #define RX_TIMER 10000 //If compiling with linux, use microseconds
#endif

//All numbers over max data + address (11000001 -> 0xC1) are free to use as request commands
//Only READ, WRITE_0 and WRITE_1 (with a followed address byte) are recognized by the FPGA state machine
#define REQ_READ 0xDD	//Send read request (followed by an address byte). The FPGA returns a byte containing data bit and the corresponding address.
#define REQ_WRITE_0 0xEE //Send write request with data 0 (followed by the address to write data to). The FPGA returns nothing.
#define REQ_WRITE_1 0xFF //Send write request with data 1 (followed by the address to write data to). The FPGA returns nothing.
//***************************************************//

//NOT Recognized by the fpga
#define REQ_IDLE 0x00	//When idle, the program is ready to send another request
#define REQ_REPEAT 0xFD
#define REQ_FAILED 0xCC
#define SINGLE_REQ 2	//Number of bytes sent per single request. Byte#1 = read/write and data (if any). Byte#2 = Address

#define CMD_WIDTH 100
#define BUF_WIDTH 30

int readCont = 0;
int writeCont = 0;

void timer_handler(void); /* Timer interrupt handler */
void exitProgram(void); 
void append_str(char *s, char c);  /* Attach  char to string */
int count_char(char *s, char tok);
void GBT_PrintAllData(void); /* Prints out all available signal data */
void Transmitt(void); /* Function for transmitting read and write requests to the comport*/
void Receive(void); /* Function for receiving read requests to the comport */


/* Global extern variables (main.h)*/
/* Use with care! */
int cport_nr = 4; /* /dev/ttyUSB4 / COM5  */
char cport_mode[]={'8','N','1',0};  //8 data bits, No parity, 1 stop bit
int cport_status = 0; // 0 -> closed, 1 -> open
int bdrate = 57600;

Signal sSwitch[MAX_SWITCHES] = { NULL };
Signal sProbe[MAX_PROBES] = { NULL };

Byte GBT_SWTABLE[MAX_SWITCHES] = {
  1,0,0,0,0,1,0,1,0,0,
  0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0
};

//**********************************************//

/*
 * UART tx and rx struct. Keeps track of UART status
 */
 typedef enum {READY, READ, WRITE, FAILED, REPEAT} e_Req;
 
 typedef struct
{
  e_Req req; //Rx -> Have received byte? / Tx -> Ready to send?
  Byte adr; //Rx -> Expected address / Tx -> Sent address
} uStatus;  //Uart status

typedef struct
{
  //int (*func)(int argc, char * argv);
  uStatus tx;
  uStatus rx;
} UART;

//UART uart;
//
//uart.tx.req = READY;
//uart.tx.adr = 0x00;
//
//uart.rx.req = READY;
//uart.rx.adr = uart.tx.adr;


//FLAGS
Byte exitFlag = 0;
Byte TX_sentRequest = REQ_IDLE;
Byte request = REQ_IDLE;
Byte TX_sentAddress = 0x00; //Address from 0x00 -> 0x41 (65 dec)

int main(int argc, char *argv[]) 
{ 
  //Window variables
  int parent_x, parent_y;
  int new_x, new_y; 
  int halfx;
  int wCmd_size = 3; // ... 

  //Commandline variables
  int ch;
  char myCmd[CMD_WIDTH];
  
  char cmdBuf[BUF_WIDTH][CMD_WIDTH] = { };
  int bufMaxIndex = 0;
  int bufIndex = 0;
  
  //Text position variables
  int fx = 0, fy = 18, tmp_fy = 0;  //wField text pos
  int gx = 0, gy = 1;   //wGbt text pos
   int cx = 1, cy = 1;   //wCmd text pos
 
  int print_toggle = 0; //Toggles between printing switches, probes or both (There is not enough space to print both)
  
  //Init functions for ncurses/curses
  initscr(); //Init screen
  noecho(); //Remove text-echo
  cbreak(); //Disables line buffering
  keypad(stdscr, TRUE); //Enable the use of special keys (KEY_*)
  nodelay(stdscr, TRUE); //Prevent program from stopping each time getch is called
  
  getmaxyx(stdscr, parent_y, parent_x); //set up initial windows 
  
  fy = parent_y - 5;  //Text position on wField (starts on bottom and scroll up)
  
  halfx = parent_x >> 1; 

  WINDOW *wField = newwin(parent_y - wCmd_size, halfx, 0, 0); 
  WINDOW *wCmd = newwin(wCmd_size, halfx, parent_y - wCmd_size, 0); // draw to our windows 
  WINDOW *wGbt = newwin(parent_y, halfx, 0, halfx); // draw to our windows 
  
  /* Activate scrolling on selected windows */
  scrollok(wField,TRUE);
 // idlok(wField, TRUE);
  //scrollok(wGbt,TRUE);
  leaveok(wField, TRUE);
  leaveok(wGbt, TRUE);
  leaveok(wCmd, FALSE);
  
  //nl();
  
  
  //clearok(wField, FALSE);
  //clearok(wGbt, FALSE);
  //clearok(wCmd, FALSE);
  //timeout(0);
  
  /* Initialize Signals from text-files */
  Signal_InitFromFile(sProbe, MAX_PROBES, "signals_probe.txt");	//Read in probe indexes and names from file. File included in Debug folder
  Signal_InitFromFile(sSwitch, MAX_SWITCHES, "signals_switch.txt"); //Read in switch indexes and names from file. File included in Debug folder
  
  int k;
  for (k = 0; k < MAX_SWITCHES; k++)
  {
    Signal_setData(sSwitch[k], GBT_SWTABLE[k]); //Increments signal data with 1
  }

  for (k = 0; k < MAX_PROBES; k++)
  {
    Signal_setData(sProbe[k],'U'); //Increments signal data with 1
  }
  
  //Draw borders
  box(wField, ACS_VLINE, ACS_HLINE);
  box(wCmd, ACS_VLINE, ACS_HLINE);
  box(wGbt, ACS_VLINE, ACS_HLINE);
        
  strcpy(myCmd, "");
  
  int index = 0;
  int flip = 0;
    
  while(1) 
  { 
    getmaxyx(stdscr, new_y, new_x); 
    if (new_y != parent_y || new_x != parent_x) 
    { 
      parent_x = new_x; 
      halfx = parent_x >> 1;
      parent_y = new_y; 
      
      fy = new_y - 5;
      
      wresize(wField, parent_y - wCmd_size, halfx); 
      wresize(wCmd, wCmd_size, halfx); 
      wresize(wGbt, parent_y, halfx); 
      
      mvwin(wField, 0,0); 
      mvwin(wCmd, new_y - wCmd_size, 0); 
      mvwin(wGbt, 0, halfx);
      
      wclear(stdscr); 
      wclear(wField); 
      wclear(wCmd); 
      wclear(wGbt); 
      
      //Draw borders
      box(wField, ACS_VLINE, ACS_HLINE);
      box(wCmd, ACS_VLINE, ACS_HLINE);
      box(wGbt, ACS_VLINE, ACS_HLINE);
      
      strcpy(myCmd, "");  //Reset commandline text
      strcpy(cmdBuf[bufMaxIndex], "");
      cx = 1; //Reset cursor
    }
  
   //if ((ch = getch()) != '\n' && ch != EOF)
    if ((ch = getch()) != ERR)
    {
      if (ch < 127 && ch > 31)
      {
          append_str(myCmd, ch);  //Attach char to string
          cx++;
          myCmd[CMD_WIDTH] = '\0'; //Prevents overflowing
          append_str(cmdBuf[bufMaxIndex], ch);
      }
      else if (ch == '\n' && myCmd[0] != '\0') //If enter has been pressed
      {
          box(wField, ' ', ' '); //Prevent the box from scrolling
          tmp_fy = fy;
          mvwprintw(wField, tmp_fy++, 1, ": %s", myCmd); 
          tmp_fy = do_commandline(wField, &tmp_fy, myCmd);
        
          int i;
          for (i = fy; i < tmp_fy; i++)
          {
              scroll(wField);
          }
          
          if (bufMaxIndex < BUF_WIDTH) bufMaxIndex++;
          else bufMaxIndex = 0;
          
          //Clear commandline window
          strcpy(myCmd, "");
          cx = 1; //Reset cursor
          wclear(wCmd);
      } 
      else
      {
         int size = strlen(myCmd);
         switch (ch)
        {
          //case KEY_LEFT:
            //if ( cx > 1) cx--;
          //break;
          //case KEY_RIGHT:
          //if (myCmd[cx-1] != '\0') cx++;
          //break;
          case KEY_PPAGE:
            if (print_toggle < 2) print_toggle++;
            else print_toggle = 0;
            wclear(wGbt);
          break;
          case KEY_NPAGE:
            if (print_toggle > 0) print_toggle--;
            else print_toggle = 2;
            wclear(wGbt);
          break;
          case KEY_BACKSPACE:
              myCmd[size-1] = '\0';
              cmdBuf[bufMaxIndex][size-1] = '\0';
              wclear(wCmd);
              if ( cx > 1) cx--;
          break;
          case KEY_UP:
              if (bufIndex < bufMaxIndex) bufIndex++; else bufIndex = 0;
              cx = count_char(cmdBuf[bufIndex], '\n');
              cx++;
              wclear(wCmd);
              strcpy(myCmd, "");
              strcpy(cmdBuf[bufMaxIndex], "");
              strcpy(myCmd, cmdBuf[bufIndex]);
            //  strcpy(cmdBuf[bufMaxIndex], cmdBuf[bufIndex]);
          break;
          case KEY_DOWN:
              if (bufIndex > 0) bufIndex--; else bufIndex = bufMaxIndex;
              cx = count_char(cmdBuf[bufIndex], '\n');
              cx++;
              wclear(wCmd);
              strcpy(myCmd, "");
              strcpy(cmdBuf[bufMaxIndex], "");
              strcpy(myCmd, cmdBuf[bufIndex]);
          break;
          default:
            //Do nothing...
          break;
        }
      }
      mvwprintw(wCmd, 1, 1, "%s", myCmd);   //Print input
    }
        
    // Print out signals in Signal Window
    gy = 1;
    mvwprintw(wGbt, gy++, 1,"Index: %-4s Name: %-45s Data:","", "", "");
    if (print_toggle == 0 || print_toggle == 2) gy += Signal_PrintSet(wGbt, &gy, sSwitch, MAX_SWITCHES);
    if (print_toggle == 1 || print_toggle == 2) gy += Signal_PrintSet(wGbt, &gy, sProbe, MAX_PROBES);
  
    //Draw borders
    box(wField, ACS_VLINE, ACS_HLINE);
    box(wCmd, ACS_VLINE, ACS_HLINE);
    box(wGbt, ACS_VLINE, ACS_HLINE);
    
    // Draw headers
    mvwprintw(wField, 0, 1, "Command History:"); 
    mvwprintw(wCmd, 0, 1, "Commandline:"); 
    //mvwprintw(wCmd, 1, 1, "-"); 
    mvwprintw(wGbt, 0, 1, "Signals:");
    
    wmove(wCmd, cy, cx);
    
    // Refresh windows 
    wrefresh(wField); 
    wrefresh(wCmd); 
    wrefresh(wGbt);
    
    //GBT_SWTABLE[index] = flip;
    //Signal_setData(sSwitch[index], GBT_SWTABLE[index]);
     
   // if(index < (MAX_SWITCHES-1)) index++;
  //  else {index = 0; flip = !flip;}
    
    #ifdef _WIN32
      Sleep(MAIN_SLEEP);
    #else
      usleep(MAIN_SLEEP); 
    #endif
  } // ... 
  
  delwin(wField); 
  delwin(wCmd); 
  delwin(wGbt);
  endwin(); 
  return 0;
}

void append_str(char *s, char c)
{
  int len = strlen(s);
  s[len] = c;
  s[len+1] = '\0';
}

int count_char(char *s, char tok)
{
  int m;
  int charcount = 0;
  for(m=0; s[m]; m++) 
  {
      if (tok != 0)
      {
        if(s[m] != tok) charcount++;
      }
      else charcount++;
  }
 return charcount;
 }


void GBT_PrintTable(void)
{
  int i;
  for (i = 0; i < MAX_SWITCHES; i++)
  {
    if (sSwitch[i] == NULL) return;	//Return if there is no more signal information to print
    Byte data = Signal_getData(sSwitch[i]);
    printf("%d", data);
  }
  printf(" ");
  for (i = 0; i < MAX_PROBES; i++)
  {
    if (sProbe[i] == NULL) return;	//Return if there is no more signal information to print
    Byte data = Signal_getData(sProbe[i]);
    printf("%d", data);
  }
}

void Receive(void)
{
  if (TX_sentRequest == REQ_READ)
    {
      int n = 0; //Rx Buffer index (See buf)
      Byte buf[128];    //Rx Buffer
      n = RS232_PollComport(cport_nr, buf, sizeof(buf));	//Read comport and put data into buffer
     // printf("n: %d\n", n);
      TX_sentRequest = REQ_IDLE;
      if(n > 0)
      {   
        //buf[n] = 0;
        TX_sentRequest = REQ_IDLE;
        int i = 0;
        for (i = 0; i < n; i++)
        {
          Byte Adr = 0;
          Byte Data = 0;
          Data = (buf[i] & ( 1 << 7 )) >> 7;    //Shift out data bit
          Adr = buf[i] << 1;    //Remove data-bit by shifting it out
          Adr = Adr >> 1;    //Shift back and get address
          //if (Adr == 2) printf("Rx: Adr %x: %x ", Adr, Data);
          //int a = (int)Adr;
          //rx_data[a] = Data;
          if (Adr < GBT_DATA_WIDTH) //For some odd reason, some of the received addresses have higher values than the GBT_DATA_WIDTH, causing pointer access errors if assigned to the signal pointers 
          {
            if (Adr > MAX_SWITCHES - 1) //0-35 = sSwitch, 36-65 sProbe
            {
              Adr = Adr - (MAX_SWITCHES); //Adr = 36 - MAX_SWITCHES = 0 -> Probe[0]
              Signal_setData(sProbe[(int)Adr], Data);
            }
            else
            {
              Signal_setData(sSwitch[(int)Adr], Data);
            }
          }
        }
      }
      else TX_sentRequest = REQ_REPEAT;
    }
}

//Test Variables
Byte txReq = REQ_READ;
Byte REQ_ADDRESS = 0x00;
//Byte f_REQsent = 0;
//Byte f_ADRsent = 0;
Byte tx_data[2];

void Transmitt(void)
{
  if (TX_sentRequest == REQ_IDLE)	//Send a request to read one gbt data address at a time
  {
    if(txReq == REQ_READ) 
    {
      TX_sentRequest = REQ_READ; 
      tx_data[0] = REQ_READ;
    }
    else if (txReq == REQ_WRITE_0)
    {
      if(GBT_SWTABLE[(int)REQ_ADDRESS] == 0) {tx_data[0] = REQ_WRITE_0;}
      else tx_data[0] = REQ_WRITE_1;
    }
     TX_sentAddress = REQ_ADDRESS;
     tx_data[1] = REQ_ADDRESS;
     RS232_SendBuf(cport_nr, tx_data, sizeof(tx_data));
     
    if (REQ_ADDRESS < GBT_DATA_WIDTH-1) 
    {
      REQ_ADDRESS++;
    }
    else 
    {
      REQ_ADDRESS = 0x00;
      switch (txReq) 
      {
          case REQ_READ:
          txReq = REQ_WRITE_0;
          break;
        
          case REQ_WRITE_0:
          txReq = REQ_READ;
          break;   
   
          case REQ_WRITE_1:
          txReq = REQ_READ;
          break;  
      }
    }
  }
  else if (TX_sentRequest == REQ_REPEAT) /* If a byte is not received, repeat the request for that address */
  {
    TX_sentRequest = REQ_READ;
    TX_sentAddress = REQ_ADDRESS;
    //printf("-");
    RS232_SendBuf(cport_nr, tx_data, sizeof(tx_data));
  }
}

void exitProgram(void)
{
  stop_timer();
  RS232_flushRXTX(cport_nr);
  RS232_CloseComport(cport_nr);

  Signal_FreeArray(sSwitch, MAX_SWITCHES);
  Signal_FreeArray(sProbe, MAX_PROBES);
}

void timer_handler(void)
{
  if (exitFlag == 0)
  {
      Transmitt();
      Receive();
  }
else exitProgram();
}