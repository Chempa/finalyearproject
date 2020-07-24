#include "nfc.h"
#include <iostream>
using namespace std;
Nfc::Nfc(){
}
Nfc::~Nfc(){

}  

void Nfc::stop(){
    mfrc.PICC_HaltA();
}

bool Nfc::writeBlock(byte *data,uint8_t id){
    int auth_return = mfrc.PCD_Authenticate(MFRC522::PICC_Command::PICC_CMD_MF_AUTH_KEY_A, (byte)id, &key, &mfrc.uid);
    cout<<"\nAuth  Returned : {"<<auth_return<<"}"<<endl;
    if(mfrc.MIFARE_Write(id, data, 16)==1) return true; 
    return false;
}

bool Nfc::readBlock(byte *data,uint8_t id){
    // for(int i = 0 ;i<16;i++){
    //     data[i] = (byte)'c';
    // }
    // return true;
    int auth_return = mfrc.PCD_Authenticate(MFRC522::PICC_Command::PICC_CMD_MF_AUTH_KEY_A, id, &key, &mfrc.uid);
    cout<<"\nAuth  Returned : {"<<auth_return<<"}"<<endl; 
    byte size = 18;
    int ret = mfrc.MIFARE_Read(id, data, &size);
    if(ret==1) return true; 
    // printf("Error code = %d \n",ret);
    return false;
}

bool Nfc::writeToken(string token){
    mfrc.PCD_Init();
    if(waitForNfc(10)==false){
        // printf("Couldn't detect any nfc card\n");
        STATUS = 0;
        return false; 
    }
    if( !mfrc.PICC_ReadCardSerial()){
        // printf("Couldn't read Serial\n");
        STATUS = 0;
        return false;
    }

    if(mfrc.uid.size != 4){
        // printf("Unknown Card\n");
        STATUS = 0;
        return false;
    }

    for(byte i = 0; i < 4; ++i){
        char buf[5];
        snprintf(buf,5,"%02x",mfrc.uid.uidByte[i]);
        UID[(i*2)] = buf[0];
        UID[(i*2)+1] = buf[1];
    }
    
    cout<<"UID = "<<UID<<endl;

    for(int i = 0 ;i<16;i++){
        byte buffer[18];
        for(int j = 0; j< 16;j++)
            buffer[j] = (byte)token[(i*16)+j];
        bool ret = writeBlock(buffer,tokenIndexes[i]);
        if(ret == false){
            // printf("WriteToken Failed\n");
            mfrc.PICC_HaltA();
            STATUS = -1;
            return false;
        }
    }
    mfrc.PICC_HaltA();
    time_sleep(0.5);
    STATUS = 1;
    // printf("Token written Successfuly\n");
    return true;
}
bool Nfc::readToken(char * token){ 
    mfrc.PCD_Init();
    if(waitForNfc(10)==false){
        // printf("Couldn't detect any nfc card\n");
        STATUS = 0;
        return false; 
    }
    // if(!mfrc.PICC_IsNewCardPresent())
    //     return false;

    if( !mfrc.PICC_ReadCardSerial()){
        // printf("Couldn't read Serial\n");
        STATUS = 0;
        mfrc.PICC_HaltA();
        return false;
    }

    if(mfrc.uid.size != 4){
        // printf("Unknown Card\n");
        STATUS = 0;
        mfrc.PICC_HaltA();
        return false;
    }

    for(byte i = 0; i < 4; ++i){
        char buf[5];
        snprintf(buf,5,"%02x",mfrc.uid.uidByte[i]);
        UID[(i*2)] = buf[0];
        UID[(i*2)+1] = buf[1];
    }
    
    cout<<"UID = "<<UID<<endl;

    // token = string(256,'0'); 
    for(int i = 0 ;i<16;i++){
        byte buffer[18];
        bool ret = readBlock(buffer,tokenIndexes[i]);
        if(ret == false){
            printf("ReadToken Failed\n");
            mfrc.PICC_HaltA();
            STATUS = -1;
            return false;
        }
        for(int j = 0; j< 16;j++){ 
            int x = (i*16)+j;
            cout<<"INDEx = " <<x<<endl;
            token[x] = (char)buffer[j];
        }
    }
    mfrc.PICC_HaltA();
    time_sleep(0.5);
    STATUS = 1;
    // printf("Token Read Successfuly\n");
    return true;
}
bool Nfc::waitForNfc(uint8_t timeout){
    if(!this->wait){
        if(!mfrc.PICC_IsNewCardPresent())
            return false;
        return true;
    }
    double startTick;
    startTick = time_time();
    while(1){ 
        if(this->buzz)
            once();
        if((time_time() - startTick) > (double)(timeout)){
            // printf("false\n");
            return false;
        }
        if(!mfrc.PICC_IsNewCardPresent())
            continue;
        return true;
    }

    
}