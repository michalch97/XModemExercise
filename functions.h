#ifndef ZADANIE_2_FUNCTIONS_H
#define ZADANIE_2_FUNCTIONS_H

#include <windows.h>
#include <iostream>

#define SOH 0x1
#define EOT 0x4
#define ACK 0x6
#define NAK 0x15
#define C 'C'
#define SUB 26

extern bool crc;
extern HANDLE comHandle; //uchwyt na obiekt (void *)
extern BOOL portReady;
extern DCB dcb; //ustawienia komunikacji szeregowej
extern int BaudRate;
extern char *COM;
extern char *file;

bool Initialize(char *ComPortName);
void Receive(char *receiveBufor, int length);
void Send(char *txchar, int len);
bool send();
bool receive();

using namespace std;

#endif //ZADANIE_2_FUNCTIONS_H
