#include <zconf.h>
#include "functions.h"

HANDLE comHandle; //uchwyt na obiekt (void *)
BOOL portReady;
DCB dcb; //ustawienia komunikacji szeregowej
//int BaudRate = 57600;
int BaudRate = 115200;

bool Initialize(char *ComPortName) {

    comHandle = CreateFile(ComPortName, GENERIC_READ | GENERIC_WRITE, // port do inicjalizacji, odczyt i zapis,
                           0,    // ochrona portu przed dostepem z innego procesu
                           NULL,    // bez ochrony
                           OPEN_EXISTING, // otwiera istniejacy port
                           0,    //  0 bo przy OPEN_EXISTING system ignoruje ten parametr odpowiedzialny za atrybuty portu
                           NULL        // NULL ta sama sytuacja co wyzej przy 0 - szablon z ustawieniami atrybutow dla tworzeonego pliku
    );

    SetupComm(comHandle, 1, 128);

    portReady = GetCommState(comHandle,
                             &dcb);//zwraca DCB - ustawienia komunikacji szeregowej dla podanego urzadzenia: urzadzanie, DCB
    if (!portReady) {
        cout << " > ERROR: GetCommState returned null" << endl;
        return false;
    }

    dcb.BaudRate = BaudRate; //szybkosc transmisji w bitach na sekunde
    dcb.ByteSize = 8; //ilosc bitow w bajcie
    dcb.Parity = NOPARITY; //bez bitow parzystosci
    dcb.StopBits = ONESTOPBIT; //jeden bit stopu
    dcb.fAbortOnError = TRUE; //konczy operacje gdy wystapi blad i nie akceptuje zadnych operacji komunikacji dopoki nie aplikacja nie potwierdzi otrzymania bledu za pomoca funkcji ClearCommError

    dcb.fOutX = FALSE; //XON/XOFF wylaczone dla wysylania - gdy TRUE wysylanie zostaje zatrzymane gdy zwrocony zostanie XoffChar i startuje znow gdy zostanie zwocony XonChar
    dcb.fInX = FALSE; //XON/XOFF wylaczone dla odbierania - gdy TRUE XoffChar charakter zostaje wyslany gdy bufor danych przychodzacych jest pelny i XonChar zostaje wyslany gdy bufor danych przychodzacych jest pusty

    dcb.fOutxCtsFlow = FALSE; //wylacza CTS (clear-to-send) - gotowosc do wysylania pakietow
    dcb.fRtsControl = RTS_CONTROL_HANDSHAKE; //RTS (request-to-send) - wyjscie zadania wyslania pakietow

    dcb.fOutxDsrFlow = FALSE; //wylaczone DRS (data-set-ready) wejscie gotowosci modemu
    dcb.fDtrControl = DTR_CONTROL_ENABLE; //wlacza linie DTR (data-terminal-ready) gotowosc terminala
    dcb.fDtrControl = DTR_CONTROL_DISABLE; //wylacza linie DTR (data-terminal-ready) gotowosc terminala
    dcb.fDtrControl = DTR_CONTROL_HANDSHAKE; // flow control

    portReady = SetCommState(comHandle, &dcb); // przypisanie ustawien do urzadzenia: urzadzenie, DCB

    if (!portReady) {
        cout << " > ERROR: SetCommState returned null" << endl;
        return false;
    }
}

void Receive(char *receiveBufor, int length) {
    DWORD position = 0, receivedBytes = 0;
    while (position < length) {
        ReadFile(comHandle, receiveBufor + position, length - position, &receivedBytes, NULL);
        //ReadFile(uchwyt urzadzenia/portu, bufor na odczytane dane, ilosc bajtow do odczytania, prawdziwa ilosc odczytanych bajtow, NULL bo plik nie otwarty z FILE_FLAG_OVERLAPPED )
        position += receivedBytes;
    }
}

void Send(char *txchar, int len) {
    DWORD num; // nie mozna dac NULL, bo ostatni parametr jest NULL i uzywamy hFile
    WriteFile(comHandle, txchar, len, &num, NULL);
    //WriteFile(uchwyt urzadzenia/portu, bufor z danymi do wyslania, ilosc bajtow do wyslania, prawdziwa ilosc wyslanych bajtow, NULL bo plik nie otwarty z FILE_FLAG_OVERLAPPED)
}

short int CRC16(char *fileBufffer) {
    int crcSum = 0, divisor = 0x18005 << 15; //dzielnik do CRC
    for (int i = 0; i < 3; i++) //"poszerzanie" crcSum, zeby pasowal do dzielnika
        crcSum = crcSum * 256 + (unsigned char) fileBufffer[i];
    crcSum *= 256;

    for (int i = 3; i < 134; i++) {
        if (i < 128)
            crcSum += (unsigned char) fileBufffer[i]; //dolozenie kolejnego bajtu
        for (int j = 0; j < 8; j++) {
            if (crcSum & (1 << 31)) // wykonaj XOR, gdy bity na pierwszych miejscach w obu przypadkach sa rowne 1
                crcSum ^= divisor; // ^ - XOR
            crcSum <<= 1; //odpowiada pomnozeniu przez 256, gdy cala petla sie wykona
        }
    }
    return crcSum >> 16;
}

bool receive() {
    char controlBuffer[1], blockHeaderBuffer[3], fileBuffer[128];
    char previousBlockNumber = 0;
    bool stop = false;
    int receivedBytes = 0;

    if (!Initialize(COM)) // inicjalizacja polaczenia
        return false;

    controlBuffer[0] = crc ? C : NAK;
    Send(controlBuffer, 1); //okreslenie jakiego typu sumy kontrolnej uzywa odbiornik

    Receive(blockHeaderBuffer, 1);//odebranie SOH - start of heading

    FILE *f = fopen(file, "wb"); //binarne otwarcie pliku do zapisu

    while (!stop) {
        unsigned short receivedSum = 0, calculatedSum = 0; //wyzeruj zmienne na sumy kontrolne
        Receive(blockHeaderBuffer + 1, 2); //odczytaj naglowek pakietu - numer bloku i odwrocony numer bloku
        Receive(fileBuffer, 128); //odczytaj pakiet z plikiem

        if (blockHeaderBuffer[0] != SOH ||
            blockHeaderBuffer[1] != (previousBlockNumber + 1)) { //porownaj numery blokow
            controlBuffer[0] = NAK; //wyslij NAK - zly naglowek
            Send(controlBuffer, 1);
            continue; //przerwij aktualna petle i zacznij nowa
        }
        Receive((char *) &receivedSum, crc ? 2 : 1); //pobierz sume kontrolna

        if (crc)
            calculatedSum = CRC16(fileBuffer); // obliczanie sumy kontrolnej CRC
        else { // w drugim przypadku liczy algebraiczna sume kontrolna
            for (int i = 0; i < 128; i++)
                calculatedSum += (unsigned char) fileBuffer[i];
            calculatedSum %= 256;
        }

        if (receivedSum != calculatedSum) { //porownaj sumy kontrolne
            controlBuffer[0] = NAK; //wyslij NAK - niezgodnosc sum
            Send(controlBuffer, 1);
            continue; //przerwij aktualna petle i zacznij nowa
        }

        controlBuffer[0] = ACK; //jesli sumy sie zgadzaja potwierdz odebranie
        Send(controlBuffer, 1);

        previousBlockNumber++;
        if ((int) previousBlockNumber == 126)
            previousBlockNumber = 0;

        Receive(controlBuffer, 1); //odczytaj ewentualny bajt kontrolny konca transmisji
        if (controlBuffer[0] == EOT) {
            unsigned char last = 127;
            while (fileBuffer[last] == SUB)
                last--;
            fwrite(fileBuffer, last + 1, 1, f); //zapisz do pliku ostatni niepelny pakiet
            receivedBytes+=(last+1);
            cout<<"\r"<<"                                                ";
            cout<<"\r"<<"DONE! "<<receivedBytes<<" bytes";
            stop = true;
        } else {
            fwrite(fileBuffer, 128, 1, f); //zapisz dane do pliku - pelen pakiet
            receivedBytes+=128;
            cout<<"\r"<<"                                                ";
            cout<<"\r"<<"Receiving... "<<receivedBytes<<" bytes";
        }
    }
    fclose(f); //zamknij plik
    controlBuffer[0] = ACK; //wyslij potwierdzenie konca transmisji
    Send(controlBuffer, 1);

    return true;
}

bool send() {
    char controlBuffer[1], blockHeaderBuffer[3], fileBuffer[128];

    if (!Initialize(COM)) // inicjalizacja polaczenia
        return false;

    cout<<"Waiting for receiver...";
    Receive(controlBuffer, 1);
    cout<<" \r                       ";
    if (controlBuffer[0] == NAK) //jesli odbiornik wysle NAK oznacza ze nie jest gotowy do dzialania z CRC
        crc = false;
    else if (controlBuffer[0] == C) //jesli odbiornik wysle C oznacza ze jest gotowy do dzialania z CRC
        crc = true;
    else
        return false;

    char blockNumber = 1;
    FILE *f = fopen(file, "rb"); //otwarcie pliku w trybie binarnym do odczytu
    fseek(f, 0, SEEK_END); //przesuniecie pozycji wskaznika w pliku na koniec zeby pozniej odczytac wielkosc pliku
    int fsize = ftell(f); //zwraca aktualna pozycje wskaznika w pliku, w tym przypadku zwraca wielkosc pliku
    fseek(f, 0, SEEK_SET); //przesuniecie pozycji wskaznika w pliku na poczatek pliku
    double percent;
    string percentBox = "[                    ]";
    cout.precision(3);

    while (ftell(f) < fsize) {//wykonuj do konca pliku
        unsigned char len = fread(fileBuffer, 1, 128,
                                  f);//wczytaj do fbuf 128 bajtow z pliku f, przypisz do len faktyczna ilosc wczytanych bajtow
        for (int i = len; i < 128; i++) {
            fileBuffer[i] = SUB; //uzupelnij pozostale bajty wartoscia SUB 26 - znak dopelnienia
        }

        unsigned short sum = 0;

        if (crc)
            sum = CRC16(fileBuffer); //obliczanie CRC
        else { //w drugim przypadku liczy algebraiczna sume kontrolna
            for (int i = 0; i < 128; i++)
                sum += (unsigned char) fileBuffer[i];
            sum %= 256;
        }
        //blockHeaderBuffer - naglowek pakietu zawierajacy: SOH, numer bloku danych, odwrocony numer bloku danych
        blockHeaderBuffer[0] = SOH;
        blockHeaderBuffer[1] = blockNumber;
        blockHeaderBuffer[2] = 255 - blockNumber;

        Send(blockHeaderBuffer, 3); //wyslanie naglowka pakietu
        Send(fileBuffer, 128); //wyslanie pakietu
        Send((char *) &sum, crc ? 2 : 1); //wyslanie sumy kontrolnej
        Receive(controlBuffer, 1); //oczekiwanie na potwierdzenie

        if (controlBuffer[0] == ACK) {
            blockNumber++; //zwieksz numer bloku jesli odbiorca potwierdzil poprawne otrzymanie pakietu
            if ((int) blockNumber == 127)
                blockNumber = 1;
            percent = (((double)ftell(f))/((double)fsize))*100.0;
            for(int i=1;i<=(2*percent)/10;i++){
                percentBox[i]='=';
            }
            cout<<"\r"<<"                                                                    ";
            cout<<"\r"<<(percent == 100 ? "DONE! " : "Sending... ")<<ftell(f)<<" / "<<fsize<<" bytes | "<<percent<<"% "<<percentBox;
        } else
            fseek(f, -1 * len,
                  SEEK_CUR); //cofnij wskaznik w pliku o wyslana ilosc bajtow, co pozwoli podjac kolejna probe wyslania pakietu
    }
    fclose(f); //zamknij plik
    do {
        controlBuffer[0] = EOT;
        Send(controlBuffer, 1); //wyslij EOT - end of transmission
        Receive(controlBuffer, 1); //oczekuj na ACK - potwierdzenie odebrania EOT i zakonczenia transmisji
    } while (controlBuffer[0] != ACK);
    return true;
}
