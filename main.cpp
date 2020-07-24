#include <iostream>
#include <csignal>
#include <cstdlib>
#include <fstream>

#include "server/httplib.h"
#include "nfc/nfc.h"
#include "r502/r502a.h" 
#include <stdio.h>
 

using namespace std;


#define DAEMON_BLOCKER_PIN 27 //wPi PIN


// SERVER VARIABLES
string KEY = "";
string IP = "";
int PORT = 0;
httplib::Server *SVR_PTR;

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

void once(){ 
    gpioSetPWMfrequency(13, 640);
    gpioPWM(13, 100);
    time_sleep(0.4); 
    gpioPWM(13, 0);
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
    popen(command.c_str(), "r");
    cout<<"writePin Done"<<endl;
}


void readConfigFile(){
    {
		ios::sync_with_stdio(0);
    	cin.tie(0);
		freopen("server/config","r",stdin);
		string mask;
		cin>>mask;cin>>IP;
		cin>>mask;cin>>PORT;
		cin>>mask;cin>>KEY;
	}
    cout<<"Address = "<<IP<<":";
	cout<<PORT<<endl;
	cout<<"Key = "<<KEY<<endl;
}

void exitHandle(int handle){
    cout<<"Exiting\n";
    SVR_PTR->stop();
    exit(0);
}

void routerSettings(string ssid, string pass){
    ofstream myfile;
    string S = "ssid=" + ssid;
    string P = "\nwpa_passphrase=" + pass;

    myfile.open ("/etc/hostapd/hostapd.conf");
    myfile << "interface=wlan0\ndriver=nl80211\nhw_mode=g\nchannel=7\nwmm_enabled=0\nmacaddr_acl=0\nauth_algs=1\nignore_broadcast_ssid=0\nwpa=2\nwpa_key_mgmt=WPA-PSK\nwpa_pairwise=TKIP\nrsn_pairwise=CCMP\n";
    myfile << S << P ;
    myfile.close();
    system("./hotspot");
}

void reboot(){
    system("reboot now");
}


int main(){
    
    signal(SIGINT, exitHandle); 
    
    using namespace httplib;
	Server svr;
    SVR_PTR = &svr;
    readConfigFile();

    svr.Post("/router",[](const Request &req, Response &res){
		printf("Request Received\n");
		res.set_header("Access-Control-Allow-Origin", "*");
		string ssid,pass,key;
		if (req.has_param("ssid") && req.has_param("password") && req.has_param("key")) {
			ssid = req.get_param_value("ssid");
			pass = req.get_param_value("password");
			key = req.get_param_value("key");
		}
		if(key != KEY){
			res.set_content("{\"status\":404}", "application/json");
			return;
		} 
        routerSettings(ssid,pass);
		res.set_content("{\"status\":1}", "application/json");
	});  

    svr.Post("/reboot",[](const Request &req, Response &res){
		printf("Request Received\n");
		res.set_header("Access-Control-Allow-Origin", "*");
		string userid,token,key;
		if (req.has_param("key")) { 
			key = req.get_param_value("key");
		}
		if(key != KEY){
			res.set_content("{\"status\":404}", "application/json");
			return;
		} 
        reboot();
		res.set_content("{\"status\":1}", "application/json");
	});  

    svr.Post("/nfc",[](const Request &req, Response &res){
		printf("Request Received\n");
		res.set_header("Access-Control-Allow-Origin", "*");
		string userid,token,key;
		if (req.has_param("user_id") && req.has_param("nfc_token") && req.has_param("key")) {
			userid = req.get_param_value("user_id");
			token = req.get_param_value("nfc_token");
			key = req.get_param_value("key");
		}
		if(key != KEY){
			res.set_content("{\"status\":404}", "application/json");
			return;
		}
        if(token.length()!=256){ 
            res.set_content("{\"status\":404}", "application/json");
			return;
        } 
        int status;
        string uid;

        writePin(27,1);
        time_sleep(1);

        while(gpioInitialise() < 0)
        { 
            time_sleep(.3);
        } 
        // gpioWrite(16,1);
        // time_sleep(1);
        {            
            Nfc nfc;
            cout<<token<<endl;
            if(nfc.writeToken(token.c_str())){
                printf("Written Successfully\n");
                once();
            }else{
                printf("Failed to write\n");
                tripple();
            }
            status = nfc.STATUS;
            uid = nfc.UID;

           
        } 
        gpioWrite(16,0); 
        gpioTerminate();
		res.set_content("{\"status\":" + to_string(status) +",\"cardId\":\"" + uid + "\",\"user_id\":\"" + userid + "\"}", "application/json");
	});  

    svr.Post("/fng",[](const Request &req, Response &res){
		printf("Request Received\n");
		res.set_header("Access-Control-Allow-Origin", "*");
		string userid,key;
		if (req.has_param("user_id") &&  req.has_param("key")) {
			userid = req.get_param_value("user_id"); 
			key = req.get_param_value("key");
		}
		if(key != KEY){ 
			res.set_content("{\"status\":404}", "application/json"); 
			return;
		} 
        string fngdata;
        int status = 0;

        writePin(27,1);
        time_sleep(1);

        while(gpioInitialise() < 0)
        { 
            time_sleep(.3);
        } 
        
        {
            // while(gpioInitialise() < 0)
            // { 
            //     time_sleep(1);
            // } 
            R502a fng("/dev/ttyS0",115200);

            bool x = fng.begin();
            if(x == false){
                fng.redLed(true);
                printf("initialization failed\n");
                // gpioTerminate();
                gpioWrite(16,0); 
                res.set_content("{\"status\":1060}", "application/json"); 
                return;
            }else{
                printf("initialization successfull\n");
            }

            //  VERIFY PASSWORD
            uint32_t password = 0x00000000;
            fng.setPassword(password);
            if(fng.verifyPassword()){
                printf("Password verified succesfully\n");
            }else{
                fng.redLed(true);
                printf("Password verification failed\n");
                // gpioTerminate();
                gpioWrite(16,0); 
                res.set_content("{\"status\":1060}", "application/json"); 
                return;
            }
            fng.blueLed(true);
            
            uint8_t model[1600];
            uint16_t length;
            
            if(fng.getFingerTemplate(model, &length)){
                printf("Got template succesfully\n");
                status = 1;
            }else{
                fng.redLed(true);
                printf("Read template failed\n");
                tripple();
                // gpioTerminate();
                gpioWrite(16,0); 
                res.set_content("{\"status\":0}", "application/json"); 
                return;
            }

            {
                char data[3200];
                for(uint16_t i = 0;i<length;i++){ 
                    char buf[5];
                    snprintf(buf,5,"%02x",model[i]);

                    printf("0x%02x",model[i]); 
                    
                    data[(i*2)] = buf[0];
                    data[(i*2)+1] = buf[1];
                }
                printf("\nDATA LEN =  %d\n",length);
                data[(length*2)] = '\0';
                fngdata = string(data);
            } 
            fng.blueLed();
            once(); 
        }
        gpioWrite(16,0); 
        gpioTerminate();
		res.set_content("{\"status\":" + to_string(status) +",\"fng\":\"" + fngdata + "\",\"user_id\":\"" + userid + "\"}", "application/json");
	});

    
    printf("%d\n",svr.listen(IP.c_str(), PORT)); 
}
