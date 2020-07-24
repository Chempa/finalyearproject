#include <pigpio.h> 
#include "MFRC522.h"

class Nfc{
public:
    Nfc();
    ~Nfc();
    bool writeToken(string token);
    bool readToken(char * token);
    bool waitForNfc(uint8_t timeout=10);
    bool writeBlock(byte *data,uint8_t id);
    bool readBlock(byte *data,uint8_t id);
    void stop();
    int STATUS = -1;
    string UID = "XXXXXXXX ";
    void once(){
        gpioSetPWMfrequency(13, 640);
        gpioPWM(13, 4);
        time_sleep(0.9);
        gpioPWM(13, 0);
        time_sleep(0.3  );
    }
    bool buzz = true;
    bool wait = true;

private:
    MFRC522 mfrc;
    MFRC522::MIFARE_Key key = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    int IRQ = 27;
    

    uint8_t tokenIndexes[16] = {
                                0x04,0x05,0x06,
                                0x08,0x09,0x0A,
                                0x0C,0x0D,0x0E,
                                0x10,0x11,0x12,
                                0x14,0x15,0x16,
                                0x18
                                };
    uint8_t doorIdIndex = 0x19;
    uint8_t userIdIndex = 0x1A;
};