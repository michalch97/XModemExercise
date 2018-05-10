#include "functions.h"

bool crc;
char *COM;
char *file;

int main() {
    bool exit = false;
    while (!exit) {
        string args[4];
        cout<<" > Send (S), receive (R) or exit (exit)?"<<endl;
        cin >> args[0];
        if (args[0] == "exit") {
            exit = true;
            break;
        }
        https://support.microsoft.com/en-gb/help/115831/howto-specify-serial-ports-larger-than-com9
        cout<<" > Choose the COM port (COM{portNumber})"<<endl;
        cin >> args[1];
        cout<<" > File name"<<endl;
        cin >> args[2];
        cout<<" > CRC16 (CRC) or algebraic checksum (AC)?"<<endl;
        cin >> args[3];

        char* comTemp = new char[args[1].size() + 1];
        copy(args[1].begin(), args[1].end(), comTemp);
        comTemp[args[1].size()] = '\0';

        char* fileTemp = new char[args[2].size() + 1];
        copy(args[2].begin(), args[2].end(), fileTemp);
        fileTemp[args[2].size()] = '\0';

        COM = comTemp; //port z przewodem
        file = fileTemp; //plik do przeslania lub nazwa pliku do zapisu
        crc = args[3] == "CRC" ? true : false;
        if (args[0] == "S") { //wysylanie
            send();
            exit = true;
        }
        else if (args[0] == "R") { //odbieranie
            receive();
            exit = true;
        }
        else
            cout<<" > ERROR: Wrong option"<<endl;
        delete [] comTemp;
        delete [] fileTemp;
    }
    return 0;
}