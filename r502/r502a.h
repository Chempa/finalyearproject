#ifndef R502A_H
#define R502A_H

// UNCOMMENT BELLOW LINE FOR DEBUG OUTPUT
#define R502A_DEBUG

#include <iostream>
#include <cstdint> 
#include <cassert>
#include <pigpio.h> 
#include "ceSerial.h"


#define DEFAULTTIMEOUT 5000
#define R502a_TIMEOUT 0xFFFF
#define R502a_BADPACKET 0xFE

#define R502a_STARTCODE 0xEF01
#define R502a_CAPACITY 0x00C8

#define R502a_COMMANDPACKET 0x1
#define R502a_DATAPACKET 0x02
#define R502a_ACKPACKET 0x07
#define R502a_ENDDATAPACKET 0x08

// STATUS CODES
#define R502a_OK 0x00

// COMMANDS
#define R502a_VERIFYPASSWORD 0x13
#define R502a_GETIMAGE 0x01
#define R502a_IMAGE2TZ 0x02
#define R502a_REGMODEL 0x05
#define R502a_EMPTY 0x0D
#define R502a_STORE 0x06
#define R502a_LOAD 0x07
#define R502a_UPLOAD 0x08
#define R502a_UPLOAD_IMG 0x0A
#define R502a_DOWNLOAD 0x09
#define R502a_DOWNLOAD_IMG 0x0B
#define R502a_DELETE 0x0C
#define R502a_SEARCH 0x04
#define R502a_MATCH_CHAR_1_2 0x03
#define R502a_TEMPLATECOUNT 0x1D
#define R502a_HANDSHAKE 0x40
#define R502a_LED 0X35
#define R502a_SETSYSPARAM 0X0E


const int MAX_PACKET_SIZE = 40000;

// ALL FUNCTIONS RETURN a boolean. True if operation successfull and False if operation failed

#include <cstdio>
class R502a
{
public:
    R502a(std::string Device, long BaudRate=57600, long DataSize=8, char ParityType='N', float NStopBits=1);
    bool begin();
    bool verifyPassword(void);
    bool handShake(void);
    bool setSysParam(uint8_t id,uint8_t content);
    bool setBaud(int baud);
    bool ledConfig(uint8_t ctrl=0x03,uint8_t speed=0xff,uint8_t color=0x01,uint8_t count=0x00);
    bool blueLed(bool blink=false){ if(blink)return ledConfig(0x02,50,0x02); return ledConfig(0x03,50,0x02);}
    bool redLed(bool blink=false){if(blink)return ledConfig(0x02,50,0x01); return ledConfig(0x03,50,0x01);}
    bool voiletLed(bool blink=false){if(blink)return ledConfig(0x02,50,0x03); return ledConfig(0x03,50,0x03);}
    bool getImage(void);
    bool getDataPacket(uint8_t *data, uint16_t* datalen);
    bool image2Tz(uint8_t slot = 1);
    bool createModel(void);
    bool emptyDatabase(void);
    bool storeModel(uint16_t id,uint8_t slot=0x01);
    bool loadModel(uint16_t id,uint8_t slot=0x01);
    bool getModel(uint8_t* model, uint16_t *length, uint8_t slot=0x01);
    bool sendModel(uint8_t* model, uint16_t length, uint8_t slot=0x01);
    bool deleteModel(uint16_t id);
    bool fingerSearch(uint16_t *fingerID, uint16_t *confidence);
    bool matchFngs(uint16_t *matchingScore);
    bool getTemplateCount(uint16_t *templateCount);
    inline void setPassword(uint32_t password){this->password=password;}
    inline void setAddress(uint32_t address){this->address=address;}
    bool waitForFinger(int secs);
    void dumpData(uint16_t len);
    void stop();
    void once(){
        gpioSetPWMfrequency(13, 640);
        gpioPWM(13, 4);
        time_sleep(0.9);
        gpioPWM(13, 0);
        time_sleep(0.3  );
    }
    ~R502a();


    // FUNCTIONAL
    bool enrol(int id);
    bool searchFinger(uint16_t *id,uint16_t *confidence);
    bool matchFingers(uint16_t *matchingScore,uint8_t* otherFingerModel,uint16_t length);
    bool getFingerTemplate(uint8_t* model, uint16_t *length, uint8_t slot=0x01);


private:
    ce::ceSerial* serial;
    uint32_t password;
    uint32_t address;
    uint8_t buffer[MAX_PACKET_SIZE];
    uint8_t BUF[MAX_PACKET_SIZE];
    uint16_t BUFINDEX=-1;
    uint8_t IRQ = 22;

    bool writePacket(uint32_t addr, uint8_t packettype, uint16_t len);
    uint16_t getReply(uint16_t timeout=DEFAULTTIMEOUT);
};

#endif // R502A_H
