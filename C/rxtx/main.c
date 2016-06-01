/*
***************************************************************************
*
* Author: Anders Østevik
*
* Copyright (C) 2015 Anders Østevik
*
* Email: aoe033@student.uib.no
*
***************************************************************************
*
* This program is based on the RS232 module written by Teunis van Beelen 
* for serial communication (http://www.teuniz.net/RS-232/#)
*
*
* Function: Sends distinctive patterns to the fpga uart and then reads them back again.
*	cport_nr -> comport number, see rs232.c for legal values
*	bdrate -> baud rate, see rs232.c for legal values
*	rs232 mode -> 8N1: 8 data bits, no parity, 1 stop bit
*
***************************************************************************
*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "rs232.h"
#include "timer.h"

#include "signals.h"

#define GBT_DATA_WIDTH 66 // 65 -> 0 //Number of data bytes
#define BUFFER_SIZE 1024

void timer_handler(void);
void exitProgram(void);

int cport_nr = 4,        /* /dev/ttyUSB4 / COM5 (see rs232.c) */
   bdrate = 19200;       /* 9600, 19200, 57600, 115200 baud (see rs232.c) */

Signal sSwitch[MAX_SWITCHES];
Signal sProbe[MAX_PROBES];

int GBT_i = 0; //0->4
Byte GBT_SWTABLE[5][MAX_SWITCHES] = {
  {1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1},
  {0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0},
  {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};
//FLAGS

Byte exitFlag = 0;

//All numbers over max data + address (11000001 -> 0xC1) are free to use as request commands
#define REQ_IDLE 0x00	//When idle, the program is ready to send another request
#define REQ_REPEAT 0xFD
#define REQ_READ 0xDD	//Send read request. The FPGA returns 1 byte containing data bit and the corresponding address.
#define REQ_WRITE_0 0xEE //Send write request with data 0. The FPGA returns nothing.
#define REQ_WRITE_1 0xFF //Send write request with data 1. The FPGA returns nothing.

#define REQ_READ_ALL 0xEA
#define REQ_READ_PROBES 0xEB
#define REQ_READ_SWITCHES 0xEC

#define REQ_WRITE_COUNT 0xFA
#define REQ_FAILED 0xDD

#define SINGLE_REQ 2	//Number of bytes sent per single request. Byte#1 = read/write and data (if any). Byte#2 = Address

Byte txStatus = REQ_IDLE; //Transmitter status

//Byte TX_sentAddress = 0x00; //Address from 0x00 -> 0x41 (65 dec)

#define RUNTIME -1  //Runtime limit. Increments by 1 every TX_TIMER-milliseconds. If negative, run forever!

#ifdef _WIN32
	 #define TX_TIMER 1 //Tx-while loop repeat every TX_TIMER milliseconds
	 #define RX_TIMER 10 //Rx-timer repeat every RX_TIMER milliseconds	
#else
	 #define TX_TIMER 1000 //If compiling with linux, use microseconds
	 #define RX_TIMER 10000 //If compiling with linux, use microseconds
#endif

void GBT_PrintAllData(void)
{
  printf("\nGBT Data: ");

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

Byte rxData[GBT_DATA_WIDTH];
//int f_PRINT = 1;

void Receive(void)
{
  if (txStatus == REQ_READ) // If transmitter has sent out a read request
    {
        int n = 0; //Rx Buffer index (See buf)
        Byte buf[10];    //Rx Buffer
        n = RS232_PollComport(cport_nr, buf, sizeof(buf));	//Read comport and put data into buffer
       // printf("n: %d\n", n);
        if(n > 0)
        {   
          //buf[n] = 0;
          txStatus = REQ_IDLE;
          int i = 0;
          for (i = 0; i < n; i++)
          {
            Byte Adr = 0;
            Byte Data = 0;
            Data = (buf[i] & ( 1 << 7 )) >> 7;	//Shift out data bit
            Adr = buf[i] << 1;	//Remove data-bit by shifting it out
            Adr = Adr >> 1;	//Shift back and get address
            //printf("Adr %x: %x, ", Adr, Data);
            int a = (int)Adr;
            rxData[a] = Data;
            
            //if (a <= GBT_DATA_WIDTH) //Compare received switch data with stored switch data
           // {
           //   f_PRINT = 1;
            //}
          }
        }
        else txStatus = REQ_REPEAT; // If nothing is received, repeat
    }
}

Byte txReq = REQ_READ;  //Request to be sent
Byte txAdr = 0x00;  //Address to be sent
Byte txData[2] = {0xDD, 0x00};  //Data to be sent

void Transmitt(void)
{
  if (txStatus == REQ_IDLE)	//Send a request to read one gbt data address at a time
  {
      if(txReq == REQ_READ) 
      {
        txStatus = REQ_READ; 
        txData[0] = REQ_READ;
      }
      else if (txReq == REQ_WRITE_0)
      {
        //Replicates the table by sending write-1 or -0 requests according to the table values.
        if(GBT_SWTABLE[GBT_i][(int)txAdr] == 0) {txData[0] = REQ_WRITE_0;}
        else txData[0] = REQ_WRITE_1; 
      }
      
      //TX_sentAddress = txAdr;
      txData[1] = txAdr;
      RS232_SendBuf(cport_nr, txData, sizeof(txData));
      
      if (txAdr < GBT_DATA_WIDTH-1)
      {
          txAdr++;
      } 
      else 
      {
        txAdr = 0x00;
        switch (txReq) 
        {
          case REQ_READ:
            txReq = REQ_WRITE_0;
          break;
          
          case REQ_WRITE_0:
            txReq = REQ_READ;
            if (GBT_i < 4) GBT_i++;
            else GBT_i = 0;
          break;   
     
          case REQ_WRITE_1: //Should not happen
            txReq = REQ_READ;
          break;  
        }
      }
  }
    else if (txStatus == REQ_REPEAT)
    {
      txStatus = REQ_READ;
      //TX_sentAddress = txAdr;
      RS232_SendBuf(cport_nr, txData, sizeof(txData));
    }
}

int main()
{
  char mode[]={'8','N','1',0};  //8 data bits, No parity, 1 stop bit
  //Byte txData[4096];
  if(RS232_OpenComport(cport_nr, bdrate, mode))
  {
    printf("Can not open comport\n");
    return(0);
  } 
 
  printf("Program started.\nComport opened!\n");

  Signal_InitFromFile(sProbe, MAX_PROBES, "signals_probe.txt");	//Read in probe indexes and names from file. File included in Debug folder
  Signal_InitFromFile(sSwitch, MAX_SWITCHES, "signals_switch.txt"); //Read in switch indexes and names from file. File included in Debug folder
  
  int k;
  for (k = 0; k < MAX_SWITCHES; k++)
  {
    Signal_setData(sSwitch[k],0); //Increments signal data with 1
  }

  for (k = 0; k < MAX_PROBES; k++)
  {
    Signal_setData(sProbe[k],0); //Increments signal data with 1
  }
  
  if(start_timer(RX_TIMER, &timer_handler))	//Handles received data
  {
    printf("\ntimer error!\n");
    return(1);
  }
 
  int runtime = 0;
  
  while(exitFlag == 0)	//Handles transmitted data
  {
    if (RUNTIME > 0)	// If negative -> Run FOREVER!
    {
      if (runtime < RUNTIME) runtime++;
      else exitFlag = 1;
    }
  
    Transmitt();
  
    #ifdef _WIN32
      Sleep(TX_TIMER);
    #else
      usleep(TX_TIMER);  /* sleep for 100 milliSeconds */
    #endif
    
    Receive();
  }
  printf("\nReturn 0!\n");

  return(0);
}

void exitProgram(void)
{
  stop_timer();
  //RS232_flushRX(cport_nr);
  RS232_CloseComport(cport_nr);

  printf("\nExiting!\n");

  Signal_FreeArray(sSwitch, MAX_SWITCHES);
  Signal_FreeArray(sProbe, MAX_PROBES);
}

void timer_handler(void)
{
  if (exitFlag == 0)
  {
        //Print out received data
        //if (f_PRINT == 1)
        //{
          printf("\nData: ");
          int i=0;
          for (i = 0; i < 66; i++)
          {
            printf("%x", rxData[i]);
          }      
          //printf("\n");
          //f_PRINT = 0;
        //}
  }
else exitProgram();
}
