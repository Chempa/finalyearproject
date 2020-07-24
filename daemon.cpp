#include <iostream>
#include <csignal>
#include <cstdlib>
#include <vector>

#include <chrono>
#include <thread>
#include <functional>

#include <pigpio.h>
#include <stdio.h>

#include "nfc/nfc.h"
#include "r502/r502a.h"
#include "sql/sql.h"

using namespace std;

#define DASHBOARD_IRQ 27
#define DOOR 6

string host = "localhost";
string user = "phpmyadmin";
string password = "password";
string database = "ucss";

Sql mysql(host, user, password, database);

Nfc *nfc = nullptr;
R502a *fng = nullptr;


void exitHandle(int handle){ 
    cout<<"Exiting\n";
    gpioWrite(DOOR,0);
    exit(0);
}

void one_shot(std::function<void(void)> func, unsigned int delay)
{
    std::thread([func, delay]() { 
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        func();
    }).detach();
}

void tripple(){
    for(int i=0;i<3;i++){ 
        gpioSetPWMfrequency(13, 640);
        gpioPWM(13, 100);
        time_sleep(0.1);
        gpioSetPWMfrequency(13, 640);
        gpioPWM(13, 0);
        time_sleep(0.1);
    }
}

void once(double d){ 
    gpioSetPWMfrequency(13, 640);
    gpioPWM(13, 100);
    time_sleep(d); 
    gpioPWM(13, 0);
    time_sleep(d);
}

int readPin(int pin){
    FILE *fp;
    int status;
    char path[10];
    string command = "gpio read " + to_string(pin);
    fp = popen(command.c_str(), "r");
    if (fp == NULL)
        return -1; 

    while (fgets(path, 10, fp) != NULL)
    {}

    status = pclose(fp);
    
    if(path[0] == '1'){
        return 1;
    }else{
        return 0;
    }
}
    
void writePin(int pin, int value){
    string command = "gpio write " + to_string(pin) + " " + to_string(value);
    // system(command.c_str());
    popen(command.c_str(), "r");
}

void openDoor(){
    once(0.4); 
    gpioWrite(DOOR,1);
}
void closeDoor(){
    gpioWrite(DOOR,0);
}



// string readFinger(){
//     string fngdata;
//     uint8_t model[1600];
//     uint16_t length;
//     int status = 0;
//     if(fng->getFingerTemplate(model, &length)){
//         // printf("Got template succesfully\n");
//         status = 1;
//     }else{
//         fng->redLed(true);
//         // printf("Read template failed\n"); 
//         return "";
//     }

//     {
//         char data[3200];
//         for(uint16_t i = 0;i<length;i++){ 
//             char buf[5];
//             // snprintf(buf,5,"%02x",model[i]);

//             // printf("0x%02x",model[i]); 
            
//             data[(i*2)] = buf[0];
//             data[(i*2)+1] = buf[1];
//         }
//         // printf("\nDATA LEN =  %d\n",length);
//         data[(length*2)] = '\0';
//         fngdata = string(data);
//     } 
//     // fng->blueLed(false); 
//     return fngdata;
// }

vector<byte> HexToBytes(const string& hex) {
    vector<byte> bytes;

    for (unsigned int i = 0; i < hex.length(); i += 2) {
        string byteString = hex.substr(i, 2);
        byte c = (byte) strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(c);
    }

    return bytes;
}

int main(){
    signal(SIGINT, exitHandle); 
    // signal(SIGABRT, exitHandle); 
    // signal(SIGTERM, exitHandle); 
    // signal(SIGTSTP, exitHandle);
    // signal(SIGTSTP, exitHandle);

    if(mysql.connect()){
        cout<<"Connected Successfully\n";
    }else{
        cout<<"Connection failed\n";
        return -1;
    }

    while(1){ 
        if(readPin(DASHBOARD_IRQ) == 1 && nfc != nullptr){
            nfc->stop();
            delete nfc; nfc = nullptr;
            gpioTerminate();
        }
        if(nfc == nullptr){
            cout<<"make nfc\n";
            while(readPin(DASHBOARD_IRQ) == 1){
                // time_sleep(.1);
            } 
            while(gpioInitialise() < 0)
            { 
                time_sleep(.3);
            } 
            time_sleep(1);
            nfc = new Nfc();
            nfc->wait = false;
            cout<<"make nfc done\n";
        }

        char buf[257]; 
        bool nfcret = nfc->readToken(buf);
        cout<<"Read returned "<<nfcret<<endl;
        if(nfcret){
            cout<<"read nfc";
            buf[256]='\0';
            string token(buf);
            string userid;
            bool flag = true;
            cout<<"Read token ok.\n";
            cout<<"Token len = "<<token.length()<<endl; 

            if(flag && mysql.query("select user_id from nfc where card_id=\"" + nfc->UID + "\" AND nfc_token=\"" + token + "\"")){
                if(mysql.res->next()){
                    userid = mysql.res->getString("user_id");
                }else{
                    cout<<"Failed HERE\n";
                    flag = false;
                }
            }else{
                cout<<"Query failed\n";
                flag = false;
                tripple();
                continue;
            }

            if(flag && mysql.query("select status_id from usertable where user_id=\"" + userid + "\";")){
                if(mysql.res->next()){
                    if(mysql.res->getInt("status_id") != 0){
                        flag = false;
                    }
                }else{
                    cout<<"Failed DB USER\n";
                    flag = false;
                    tripple();
                    continue;
                }
            }else{
                cout<<"Query failed\n";
                flag = false;
                tripple();
                continue;
            }

            if(flag && mysql.query("select fin_token from fingerprint where user_id=\"" + userid + "\"")){
                if(mysql.res->next()){
                    token = mysql.res->getString("fin_token"); 
                }else{
                    cout<<"Failed FNG DB\n";
                    tripple();
                    continue;
                }
                R502a fng("/dev/ttyS0",115200);
                bool x = fng.begin();
                if(true){
                    fng.blueLed(true);
                    //  Match fingers
                    uint16_t matchingScore;
                    bool AUTH_SUCCESS = false;
                    vector<byte> model = HexToBytes(token);
                    if(fng.matchFingers(&matchingScore,model.data(),1536)){
                        printf("Matched succesfully\n");
                        printf("Matching score = %d",matchingScore);
                        AUTH_SUCCESS = true;
                    }else{
                        printf("Match failed\n"); 
                        AUTH_SUCCESS = false;
                        tripple();
                        fng.blueLed(false); 
                        continue;
                    }
                    // VERIFY 
                    if(AUTH_SUCCESS){
                        cout<<"\nDONE!!!\n\n";
                        fng.blueLed(false); 
                        openDoor();
                        time_sleep(10);
                        closeDoor();
                        
                        // one_shot(closeDoor,2000);
                    }else{
                        tripple(); 
                        fng.redLed(true);
                        time_sleep(3);
                        fng.blueLed(false); 
                        continue;
                    }
                    
                }else{
                    tripple();
                    continue;
                }
            }else{ 
                cout<<"Query failed\n";
                tripple();
                continue;
            }
            
        }else{ 
            // tripple();
        } 
        time_sleep(.2);
    }
}

// int main2(){

//     signal(SIGINT, exitHandle); 
//     signal(SIGABRT, exitHandle); 
//     signal(SIGTERM, exitHandle); 
//     signal(SIGTSTP, exitHandle);
//     signal(SIGTSTP, exitHandle);

//     writePin(27,0);

//     if(mysql.connect()){
//         cout<<"Connected Successfully\n";
//     }else{
//         cout<<"Connection failed\n";
//         return -1;
//     }
 
//     while(1){ 
//         if(readPin(DASHBOARD_IRQ) == 1){
//             if(nfc != nullptr){
//                 nfc->stop();
//                 fng->stop();
//                 delete nfc; nfc = nullptr;
//                 delete fng; fng = nullptr;
//                 gpioTerminate();
//                 cout<<"Releasing Resources...\n";
//             }  
//         }
//         cout<<"LOOP";
//         time_sleep(0.1);

//         if(nfc == nullptr && fng == nullptr && (readPin(27) == 0 )){
//             cout<<"Acquiring Resources...\n";
//             writePin(27,0);
//             while(gpioInitialise() < 0)
//             { 
//                 // time_sleep(1);
//             } 
//             nfc = new Nfc();
//             fng = new R502a("/dev/ttyS0",115200);
//             bool x = fng->begin();
//             nfc->wait = false;
            
//             if(x == false){
//                 fng->redLed(true);
//                 // printf("initialization failed\n");  
//                 delete nfc; nfc = nullptr;
//                 delete fng; fng = nullptr;
//                 continue;
//             }else{
//                 // printf("initialization successfull\n");
//             }

//             //  VERIFY PASSWORD
//             uint32_t password = 0x00000000;
//             cout<<"VERIFY PASS"<<endl;
//             fng->setPassword(password);
//             if(fng->verifyPassword()){
//                 // printf("Password verified succesfully\n");
//             }else{
//                 fng->redLed(true);
//                 // printf("Password verification failed\n"); 
//                 delete nfc; nfc = nullptr;
//                 delete fng; fng = nullptr;
//                 continue;
//             }
//             cout<<"VERIFY PASS"<<endl;
//             fng->blueLed(false);
//         }

//         if(nfc == nullptr)continue;
        
//         char buf[257]; 
//         if(nfc->readToken(buf)){
//             buf[256]='\0';
//             string token(buf);
//             string userid;
//             bool flag = true;
//             cout<<"Read token ok.\n";
//             cout<<"Token len = "<<token.length()<<endl; 

//             if(flag && mysql.query("select user_id from nfc where card_id=\"" + nfc->UID + "\" AND nfc_token=\"" + token + "\"")){
//                 if(mysql.res->next()){
//                     userid = mysql.res->getString("user_id");
//                 }else{
//                     cout<<"Failed HERE\n";
//                     flag = false;
//                 }
//             }else{
//                 cout<<"Query failed\n";
//                 flag = false;
//                 tripple();
//                 continue;
//             }

//             if(flag && mysql.query("select status_id from usertable where user_id=\"" + userid + "\";")){
//                 if(mysql.res->next()){
//                     if(mysql.res->getInt("status_id") != 0){
//                         flag = false;
//                     }
//                 }else{
//                     cout<<"Failed DB USER\n";
//                     flag = false;
//                     tripple();
//                     continue;
//                 }
//             }else{
//                 cout<<"Query failed\n";
//                 flag = false;
//                 tripple();
//                 continue;
//             }

//             if(flag && mysql.query("select fin_token from fingerprint where user_id=\"" + userid + "\"")){
//                 if(mysql.res->next()){
//                     token = mysql.res->getString("fin_token"); 
//                 }else{
//                     cout<<"Failed FNG DB\n";
//                     tripple();
//                     continue;
//                 }
            
//                 if(true){
//                     fng->blueLed(true);
//                     //  Match fingers
//                     uint16_t matchingScore;
//                     bool AUTH_SUCCESS = false;
//                     vector<byte> model = HexToBytes(token);
//                     if(fng->matchFingers(&matchingScore,model.data(),1536)){
//                         printf("Matched succesfully\n");
//                         printf("Matching score = %d",matchingScore);
//                         AUTH_SUCCESS = true;
//                     }else{
//                         printf("Match failed\n"); 
//                         AUTH_SUCCESS = false;
//                         tripple();
//                         continue;
//                     }
//                     // VERIFY 
//                     if(AUTH_SUCCESS){
//                         cout<<"\nDONE!!!\n\n";
//                         openDoor();
//                         time_sleep(5);
//                         closeDoor();
//                         fng->blueLed(false); 
//                         // one_shot(closeDoor,2000);
//                     }else{
//                         tripple(); 
//                         fng->redLed(true);
//                         time_sleep(3);
//                         fng->blueLed(false); 
//                         continue;
//                     }
                    
//                 }else{
//                     tripple();
//                     continue;
//                 }
//             }else{ 
//                 cout<<"Query failed\n";
//                 tripple();
//                 continue;
//             }
            
//         }else{ 
//             // tripple();
//         } 
//         time_sleep(1);
//     }
// }