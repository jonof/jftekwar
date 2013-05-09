/* serial.h
   Created 1/7/94, Modified 2/11/95
   Copyright 1995 John Schultz
   All Rights Reserved
*/

#ifndef SERIAL_H
#define SERIAL_H

#define SERIAL_PORT_COM1 0
#define SERIAL_PORT_COM2 1
#define SERIAL_PORT_COM3 2
#define SERIAL_PORT_COM4 3
#define SERIAL_PORT_NONE 4

#define USE_16550 (1<<0)
#define SER_OK         0
#define SER_NO_PORT   -1
#define SER_NO_MEMORY -2

#define SER_NO_CHAR   -1

#define UART_NONE      0
#define UART_8250      1
#define UART_16550     2

#define FLG_CALLED (1<<4)

#ifdef __WATCOMC__
#pragma pack(1)
#define FAR
#else
#define FAR __far
#endif

typedef void __interrupt __far (__far *s_ifunc)(void);

typedef struct
{
  volatile short head;
  volatile short tail;
  volatile short count;
  short size;
  short seg;
} QUE;

typedef struct
{
  short jmp;
  short port;
  volatile short flags;
  QUE   rxq;
  QUE   txq;
} BIH_STRUCT;

typedef struct serQueue {
  QUE     FAR  *q;
  char    FAR  *que;
  short     selector;
} serQueue;

typedef struct serialData {
  serQueue tx,rx;
  long checkCTS;
  long fifoEnabled;
  long port;        /* Com1, com2, com3, or com4 */
  long portNum;     /* sdcom[4] index: 0..3 */
  long intvec;
  long intbit;
  BIH_STRUCT FAR *bih;
  unsigned short bihSel;
  unsigned short wRmSel;
  unsigned short wRmOfs;
  unsigned short wPmSel;
  unsigned long  wPmOfs;
  s_ifunc prevint;
  long prevLineControl;
  long prevModemControl;
  long prevPicMask;
  long prevIntEnable;
  long prevFifo;
  long prevBPS;
} serialData;

#ifdef __WATCOMC__
#pragma pack()
#endif

long initSerial(serialData * sd,long port,long flags,long txQsize,long rxQsize);
void deInitSerial(serialData * sd);

long setBPS(serialData * sd,long bpsRate);
long writeSer(serialData * sd,char ch);
void writeSerNoBuff(serialData * sd,char ch);
long readSer(serialData * sd);
long rxBuffEmpty(serialData * sd);
long txBuffEmpty(serialData * sd);
void flushRead(serialData * sd);
void flushWrite(serialData * sd);
void flushSer(serialData * sd);

void setCheckCTS(serialData * sd,long checkCTS);
void initDTR(serialData * sd);
void setDTR(serialData * sd,long state);
void setRTS(serialData * sd,long state);
long getDSR(serialData * sd);
long getDCD(serialData * sd);

long uartType(serialData * sd);

long serOpen(serialData * sd);

/* serial.h */

#endif
