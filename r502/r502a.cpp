#include "r502a.h"

R502a::R502a(std::string Device, long BaudRate, long DataSize, char ParityType, float NStopBits)
{
    serial = new ce::ceSerial(Device,BaudRate,DataSize,ParityType,NStopBits);
    password = 0x00000000;
    address = 0xFFFFFFFF;
    
    gpioSetMode(IRQ, PI_INPUT);
    gpioSetPullUpDown(IRQ, PI_PUD_DOWN);
}

void R502a::stop(){
    serial->Close();
}

bool R502a::begin()
{
    if(serial->Open()!=0){
#ifdef R502A_DEBUG
        printf("R502a | Failed to open serial port-> %s\n",serial->GetPort().c_str());
        printf("Issues: Permission denied or Port (%s) is not available\n",serial->GetPort().c_str());
#endif
        return false;
    }else{
#ifdef R502A_DEBUG
        printf("R502a | %s opened successfully\n",serial->GetPort().c_str());
#endif
        return true;
    }
}

bool R502a::writePacket(uint32_t addr, uint8_t packettype, uint16_t len){
   
    uint8_t buf[5000];
    // uint8_t *buf = new uint8_t[len+9];
    long bl = 0;

    buf[bl++]=(R502a_STARTCODE>>8)&0xff;buf[bl++]=R502a_STARTCODE&0xff;
    buf[bl++]=(addr>>24)&0xff;buf[bl++]=(addr>>16)&0xff;buf[bl++]=(addr>>8)&0xff;buf[bl++]=(addr)&0xff;
    buf[bl++]=packettype;
    buf[bl++]=(len>>8)&0xff;buf[bl++]=(len)&0xff;
     
    uint16_t sum = (len>>8) + (len&0xFF) + packettype;
    for (uint16_t i=0; i< len-2; i++) {
        printf("I = %d\n",i);
        buf[bl++]=buffer[i];
        sum += buffer[i];
    }
    printf("\nOKAY\n");
    buf[bl++]=(sum>>8)&0xff;buf[bl++]=sum&0xff;
    
    if(serial->Write((char*)buf,bl) == false){
#ifdef R502A_DEBUG
        printf("R502a | Failed to write packet\npacket:");
        for (long i = 0;i<bl;i++) {
            printf("%02x ",buf[i]);
        }
        printf("\n");
#endif
        // delete [] buf;
        return false;
    }
    
#ifdef R502A_DEBUG
    printf("R502a | packet written successful\npacket: ");
    for (long i = 0;i<bl;i++) {
        printf("0x%02x ",buf[i]);
    }
    printf("\n");
#endif
    // delete [] buf;
    return true;
}


uint16_t R502a::getReply(uint16_t timeout) {
    uint8_t reply[500];
    int idx;
    uint16_t timer=0;
    BUFINDEX = -1;
    idx = 0;
#ifdef R502A_DEBUG
    printf("R502a | waiting for incoming data\n...");
#endif
    while (true) {
#ifdef R502A_DEBUG
        printf(".");
#endif
        bool ret;
        char c = serial->ReadChar(ret); 

        
        while (ret==0) {
             ce::ceSerial::Delay(1);
            timer+=1;
            if (timer >= timeout) {
#ifdef R502A_DEBUG
                printf("\nR502a | failed. timeout\n");
#endif
                return R502a_TIMEOUT;
            }
            c = serial->ReadChar(ret);
#ifdef R502A_DEBUG 
#endif
        }
        // something to read!
        reply[idx] = c; 
        BUF[++BUFINDEX] = c;
        printf(" (0x%02x),",c);

        if ((idx == 0) && (reply[0] != (R502a_STARTCODE >> 8))){ 
            continue;
        } 
        idx++;
        if (idx >= 9) {
            // printf("\n > 9 Runing\n");

            if ((reply[0] != (R502a_STARTCODE >> 8)) || (reply[1] != (R502a_STARTCODE & 0xFF))){
#ifdef R502A_DEBUG

                printf("\nREPLY [0] = 0x%02x\n",reply[0]);
                printf("REPLY [1] = 0x%02x\n",reply[1]);
                printf("R502a_STARTCODE = 0x%04x\n",R502a_STARTCODE);
                printf("R502a | Bad packet");
#endif
                return R502a_BADPACKET;
            }
            uint8_t packettype = reply[6];

            uint16_t len = reply[7];
            len <<= 8;
            len |= reply[8];
            len -= 2;

            if (idx <= (len+10)) continue;
            buffer[0] = packettype;
            for (uint16_t i=0; i<len; i++) {
                buffer[1+i] = reply[9+i];
            }


#ifdef R502A_DEBUG
        printf("R502a | ");
        printf(buffer[1]!=0x00 ? "Invalid reply" : "Reply read successfully");
        printf("\n Confirmation code: (0x%02x)",buffer[1]);
        printf("\n Packet type: (0x%02x)\n",packettype);
#endif
            return len;
        }
    }
}

bool R502a::ledConfig(uint8_t ctrl,uint8_t speed,uint8_t color,uint8_t count){
#ifdef R502A_DEBUG
    printf("R502 | ledConfig\n");
#endif
    buffer[0]=R502a_LED;         buffer[1]=ctrl;
    buffer[2]=speed;    buffer[3]=color;
    buffer[4] = count;

    writePacket(address, R502a_COMMANDPACKET, 7);
    uint16_t len = getReply();

    if ((len == 1) && (buffer[0] == R502a_ACKPACKET) && (buffer[1] == R502a_OK)){
#ifdef R502A_DEBUG
        printf("R502 | LED Config successful\n");
#endif
        return true;
    } 
#ifdef R502A_DEBUG
        printf("R502a | LED Config failed\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
    return false;
}

bool R502a::setSysParam(uint8_t id,uint8_t content){
#ifdef R502A_DEBUG
    printf("R502 | Set system param\n");
#endif
    buffer[0]=R502a_SETSYSPARAM;         buffer[1]=id;
    buffer[2]=content;  

    writePacket(address, R502a_COMMANDPACKET, 5);
    uint16_t len = getReply();

    if ((len == 1) && (buffer[0] == R502a_ACKPACKET) && (buffer[1] == R502a_OK)){
#ifdef R502A_DEBUG
        printf("R502 | Set system param successful\n");
#endif
        return true;
    }
#ifdef R502A_DEBUG
        printf("R502a | Set system param failed\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
    return false;
}

bool R502a::setBaud(int baud){
        int ar[5] = {1*9600,2*9600,4*9600,6,12*9600};
        bool found = false;
        for (int i=0;i<5;i++){
            if(ar[i] == baud){
                found =true;break;
            }
        }
        if(found){
           return setSysParam(4,(int)baud/9600);
        }else{
            printf("Invalid baud Rate");
            return false;
        }
    }

bool R502a::verifyPassword()
{
#ifdef R502A_DEBUG
    printf("R502 | verifyPassword\n");
#endif
    buffer[0]=R502a_VERIFYPASSWORD;         buffer[1]=(uint8_t)(password >> 24);
    buffer[2]=(uint8_t)(password >> 16);    buffer[3]=(uint8_t)(password >> 8);
    buffer[4] = (uint8_t)password;

    writePacket(address, R502a_COMMANDPACKET, 7);
    uint16_t len = getReply();

    if ((len == 1) && (buffer[0] == R502a_ACKPACKET) && (buffer[1] == R502a_OK)){
#ifdef R502A_DEBUG
        printf("R502 | Password verification successful\n");
#endif
        return true;
    }
#ifdef R502A_DEBUG
        printf("R502a | Password verification failed\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
    return false;
}

bool R502a::handShake()
{
#ifdef R502A_DEBUG
    printf("R502 | handShake\n");
#endif
    buffer[0]=R502a_HANDSHAKE;
    writePacket(address, R502a_COMMANDPACKET, 3);
    uint16_t len = getReply(); 
    if ((len == 1) && (buffer[0] == R502a_ACKPACKET) && (*(buffer+1) == R502a_OK)){
#ifdef R502A_DEBUG
        printf("R502a | Device ready\n\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
        return true;
    }
#ifdef R502A_DEBUG
    printf("R502a | Device not ready\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
    return false;
}

bool R502a::getImage(void){
    for (int tries = 5;tries>0;tries--){
    #ifdef R502A_DEBUG
        printf("R502 | getImage\n");
    #endif
        buffer[0]=R502a_GETIMAGE;
        writePacket(address, R502a_COMMANDPACKET, 3);
        uint16_t len = getReply();

        if (((len < 1) && (buffer[0] != R502a_ACKPACKET))||*(buffer+1)!=0x00){
    #ifdef R502A_DEBUG
            printf("R502a | getImage failed\nConfirmation code: (0x%02x)",*(buffer+1));
    #endif
            time_sleep(1);
        }else{
            #ifdef R502A_DEBUG
                printf("R502a | getImage successful\nConfirmation code: (0x%02x)",*(buffer+1));
            #endif 
                return true;
        }
    } 
    return false; 
}

bool R502a::image2Tz( uint8_t slot ) {
#ifdef R502A_DEBUG
    printf("R502 | image2Tz\n");
#endif
    buffer[0]=R502a_IMAGE2TZ;   buffer[1]=slot;
    writePacket(address, R502a_COMMANDPACKET, 4);
    uint16_t len = getReply();

    if (((len < 1) && (buffer[0] != R502a_ACKPACKET))||*(buffer+1)!=0x00){
#ifdef R502A_DEBUG
        printf("R502a | image2Tz failed\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
        return false;
    }

#ifdef R502A_DEBUG
    printf("R502a | image2Tz successful\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
    return true;
}

bool R502a::createModel(void){
#ifdef R502A_DEBUG
    printf("R502 | createModel\n");
#endif
    buffer[0]=R502a_REGMODEL;
    writePacket(address, R502a_COMMANDPACKET, 3);
    uint16_t len = getReply();

    if (((len < 1) && (buffer[0] != R502a_ACKPACKET))||*(buffer+1)!=0x00){
#ifdef R502A_DEBUG
        printf("R502a | createModel failed\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
        return false;
    }

#ifdef R502A_DEBUG
    printf("R502a | createModel successful\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
    return true;
}

bool R502a::emptyDatabase(void){
#ifdef R502A_DEBUG
    printf("R502 | emptyDatabase\n");
#endif
    buffer[0]=R502a_EMPTY;
    writePacket(address, R502a_COMMANDPACKET, 3);
    uint16_t len = getReply();

    if (((len < 1) && (buffer[0] != R502a_ACKPACKET))||*(buffer+1)!=0x00){
#ifdef R502A_DEBUG
        printf("R502a | emptyDatabase failed\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
        return false;
    }

#ifdef R502A_DEBUG
    printf("R502a | emptyDatabase successful\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
    return true;
}

bool R502a::storeModel(uint16_t id, uint8_t slot){
#ifdef R502A_DEBUG
    printf("R502 | storeModel\n");
#endif
    buffer[0]=R502a_STORE;          buffer[1]=slot;
    buffer[2]=(uint8_t)(id >> 8);   buffer[3]=(uint8_t)(id & 0xFF);
    writePacket(address, R502a_COMMANDPACKET, 6);
    uint16_t len = getReply();

    if (((len < 1) && (buffer[0] != R502a_ACKPACKET))||*(buffer+1)!=0x00){
#ifdef R502A_DEBUG
        printf("R502a | storeModel failed\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
        return false;
    }

#ifdef R502A_DEBUG
    printf("R502a | storeModel successful\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
    return true;
}

bool R502a::loadModel(uint16_t id,uint8_t slot){
#ifdef R502A_DEBUG
    printf("R502 | loadModel\n");
#endif
    buffer[0]=R502a_LOAD;          buffer[1]=slot;
    buffer[2]=(uint8_t)(id >> 8);   buffer[3]=(uint8_t)(id & 0xFF);
    writePacket(address, R502a_COMMANDPACKET, 6);
    uint16_t len = getReply();

    if (((len < 1) && (buffer[0] != R502a_ACKPACKET))||*(buffer+1)!=0x00){
#ifdef R502A_DEBUG
        printf("R502a | loadModel failed\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
        return false;
    }

#ifdef R502A_DEBUG
    printf("R502a | loadModel successful\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
    return true;
}

bool R502a::getDataPacket(uint8_t *data, uint16_t* datalen){
    long int ind = 0;
    while(1){
        ce::ceSerial::Delay(50);
        uint16_t len = getReply(); 
        if(len == R502a_TIMEOUT)return false;
        for (int i = 1; i <= len; ++i)
        {
            data[ind++]=buffer[i];
        }
        *datalen=ind;
        if(buffer[0] == 0x08){
            return true;
        }
    }
}

bool R502a::getModel(uint8_t* model, uint16_t *length, uint8_t slot){
#ifdef R502A_DEBUG
    printf("R502 | getModel\n");
#endif
    buffer[0]=R502a_UPLOAD; buffer[1]=slot;
    writePacket(address, R502a_COMMANDPACKET, 4);
    uint16_t len = getReply(); 
    if (((len < 1) && (buffer[0] != R502a_ACKPACKET))||*(buffer+1)!=0x00){
#ifdef R502A_DEBUG
        printf("R502a | getModel failed\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
        return false;
    }

    uint8_t conf_code =*(buffer+1);
    // RECEIVE REMAINING DATA

    if(!getDataPacket(model,length))return false;

#ifdef R502A_DEBUG
    printf("R502a | getModel successful\nConfirmation code: (0x%02x)",conf_code);
#endif
    return true;
}


bool R502a::sendModel(uint8_t* model, uint16_t length, uint8_t slot){
#ifdef R502A_DEBUG
    printf("R502 | sendModel\n");
#endif
    buffer[0]=R502a_DOWNLOAD;   buffer[1]=slot;
    writePacket(address, R502a_COMMANDPACKET, 4);
    uint16_t len = getReply();

    if (((len < 1) && (buffer[0] != R502a_ACKPACKET))||*(buffer+1)!=0x00){
#ifdef R502A_DEBUG
        printf("R502a | sendModel failed\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
        return false;
    }


    uint8_t conf_code =*(buffer+1);
    int index = 0;
    for (int i = 0; i <= length; i+=256)
    {
        printf("Writing...\n");
        for (int j = i; j < i+256; j++)
            buffer[index++] = model[j];
        index = 0;
        uint8_t packettype = R502a_DATAPACKET;
        if((i+256)>length)
            packettype = R502a_ENDDATAPACKET;
        writePacket(address, packettype, 256+2);
    }
    printf("done\n");
#ifdef R502A_DEBUG
    printf("R502a | sendModel successful\nConfirmation code: (0x%02x)",conf_code);
#endif
    return true;
}

bool R502a::deleteModel(uint16_t id){
#ifdef R502A_DEBUG
    printf("R502 | deleteModel\n");
#endif
    buffer[0]=R502a_DELETE;     buffer[1]=(uint8_t)(id >> 8);
    buffer[2]=(uint8_t)(id & 0xFF);     buffer[3]=0x00;
    buffer[4]=0x01;
    writePacket(address, R502a_COMMANDPACKET, 7);
    uint16_t len = getReply();

    if (((len < 1) && (buffer[0] != R502a_ACKPACKET))||*(buffer+1)!=0x00){
#ifdef R502A_DEBUG
        printf("R502a | deleteModel failed\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
        return false;
    }

#ifdef R502A_DEBUG
    printf("R502a | deleteModel successful\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
    return true;
}

bool R502a::fingerSearch(uint16_t *fingerID, uint16_t *confidence){
#ifdef R502A_DEBUG
    printf("R502 | fingerSearch\n");
#endif
    *fingerID = 0xFFFF;
    *confidence = 0xFFFF;
    buffer[0]=R502a_SEARCH; buffer[1]=0x01;      buffer[2]=0x00;
    buffer[3]=0x00;         buffer[4]=0xff&(R502a_CAPACITY>>8);     buffer[5]=0xff&(R502a_CAPACITY & 0xFF);
    writePacket(address, R502a_COMMANDPACKET, 8);
    uint16_t len = getReply();

    if (((len < 1) && (buffer[0] != R502a_ACKPACKET))||*(buffer+1)!=0x00){
#ifdef R502A_DEBUG
        printf("R502a | fingerSearch failed\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
        return false;
    }


    *fingerID = buffer[2];
    *fingerID <<= 8;
    *fingerID |= buffer[3];

    *confidence = buffer[4];
    *confidence <<= 8;
    *confidence |= buffer[5];
#ifdef R502A_DEBUG
    printf("R502a | fingerSearch successful\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
    return true;
}

void R502a::dumpData(uint16_t len){
    printf("Logging Dump with len = %d\n",len);
    for (uint16_t i = 0; i < len; ++i)
    {
        printf("(0x%02x)", buffer[i]);
    }
    printf("\n");
    for (uint16_t i = 0; i <= BUFINDEX; ++i)
    {
        printf("0x%02x, ", BUF[i]);
    }
    printf("\n");
}


bool R502a::matchFngs(uint16_t *matchingScore)
{
#ifdef R502A_DEBUG
    printf("\n\nR502 | matchFingers\n");
#endif
    buffer[0] = R502a_MATCH_CHAR_1_2;
    writePacket(address, R502a_COMMANDPACKET, 3);
    uint16_t len = getReply(); 
    dumpData(len);
    printf("CONDITIONS:\n");
    std::cout<<(len == 1)<<"\n";
    std::cout<<(buffer[0] == R502a_ACKPACKET)<<"\n";
    std::cout<<(*(buffer+1) == R502a_OK)<<"\n";

    if (((len < 1) && (buffer[0] != R502a_ACKPACKET))||*(buffer+1)!=0x00){ 
        return false;
    }else{ 
        *matchingScore = *(2+buffer);
        *matchingScore <<= 8;
        *matchingScore |= *(buffer+3);
        return true;
    }
    
}

bool R502a::getTemplateCount(uint16_t *templateCount){
#ifdef R502A_DEBUG
    printf("\n\nR502 | getTemplateCount\n");
#endif
    *templateCount = 0xFFFF;
    // get number of templates in memory
    buffer[0]=R502a_TEMPLATECOUNT;
    writePacket(address, R502a_COMMANDPACKET, 3);
    uint16_t len = getReply();

    if (((len < 1) && (buffer[0] != R502a_ACKPACKET))||*(buffer+1)!=0x00){
#ifdef R502A_DEBUG
        printf("R502a | getTemplateCount failed\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
        return false;
    }


    *templateCount = *(2+buffer);
    *templateCount <<= 8;
    *templateCount |= *(3+buffer);
#ifdef R502A_DEBUG
    printf("R502a | fingerSearch successful\nConfirmation code: (0x%02x)",*(buffer+1));
#endif
    return true;
}







R502a::~R502a(){
    serial->Close();
    delete serial;
}

bool R502a::enrol(int id){
    for (int i = 0; i < 2; ++i)
    {
         if(!getImage()){
            printf("Get Finger Image failed\n");
            return false;
        } 
        if(!image2Tz(i+1)){
            printf("Image2Tz failed\n");
            return false;
        }
    } 

    if(!createModel()){
        printf("createModel failed\n");
        return false;
    }

    if(!storeModel(id)){
        printf("storeModel failed\n");
        return false;
    }
    return true;
}

bool R502a::searchFinger(uint16_t *id,uint16_t *confidence){
    if(!getImage()){
        printf("Get Finger Image failed\n");
        return false;
    } 
    if(!image2Tz()){
        printf("Image2Tz failed\n");
        return false;
    }

    if(!fingerSearch(id,confidence)){ 
        printf("Finger not found\n");
        return false;
    }

    return true;

}



bool R502a::matchFingers(uint16_t *matchingScore,uint8_t* otherFingerModel,uint16_t length){ 
    if(!waitForFinger(10)){
        printf("Couldn't find any finger\n");
        return false;
    }
    for (int i = 0; i < 2; ++i)
    {
         if(!getImage()){
            printf("Get Finger Image failed\n");
            return false;
        } 
        if(!image2Tz(i+1)){
            printf("Image2Tz failed\n");
            return false;
        }
    } 

    if(!createModel()){
        printf("Create model failed\n");
        return false;
    }

    //  Send finger model  
    if(sendModel(otherFingerModel,length,2)){
        printf("Sent model successfully\n");
    }else{
        printf("failed to send model!\n");
        return false;
    }

    if(!matchFngs(matchingScore)){ 
        printf("Fingers not matched\n");
        return false;
    }
    return true;
}

bool R502a::waitForFinger(int secs){
    double startTick;
    startTick = time_time();
    printf("Place finger on the module\n");
    blueLed(true);
    while(1){ 
        once();
        int x = gpioRead(IRQ);
        printf("IRQ = %d\n",x);
        if(x == 0){
            printf("true\n");
            voiletLed();
            return true;
        }else{
            // printf(".");
        }
        if((time_time() - startTick) > (double)(secs)){
            printf("false\n");
            redLed();
            return false;
        }
        // time_sleep(1);
    }
}

bool R502a::getFingerTemplate(uint8_t* model, uint16_t *length, uint8_t slot){
    if(!waitForFinger(10)){
        printf("Couldn't find any finger\n");
        return false;
    }
    for (int i = 0; i < 2; ++i)
    { 
        if(!getImage()){
            printf("Get Finger Image failed\n");
            return false;
        } 
        if(!image2Tz(i+1)){
            printf("Image2Tz failed\n");
            return false;
        }
    } 
    // blueLed();
    if(!createModel()){
        printf("Create model failed\n");
        return false;
    }

    if(!getModel(model,length,slot)){
        printf("Get model failed\n");
        return false;
    }
    
    return true;
} 