/**
 *  Modbus master example 2:
 *  The purpose of this example is to query several sets of data
 *  from an external Modbus slave device.
 *  The link media can be USB or RS232.
 *
 *  Recommended Modbus slave:
 *  diagslave http://www.modbusdriver.com/diagslave.html
 *
 *  In a Linux box, run
 *  "./diagslave /dev/ttyUSB0 -b 19200 -d 8 -s 1 -p none -m rtu -a 1"
 * 	This is:
 * 		serial port /dev/ttyUSB0 at 19200 baud 8N1
 *		RTU mode and address @1
 */
#define HAVE_RTC
#define HAVE_RTC_DS1307

#include <TimeLib.h>
#include <WebSockets.h>
#include <ModbusRtu.h>
#include <PID_v1.h>
#include <EEPROMex.h>
#include <ArduinoJson.h>
#include "C:\Users\pierr\OneDrive\Documents\Arduino\libraries\cocorico2\Mesocosmes.h"
#include "C:\Users\pierr\OneDrive\Documents\Arduino\libraries\cocorico2\Hamilton.h"
#include "C:\Users\pierr\OneDrive\Documents\Arduino\libraries\cocorico2\Condition.h"
#include "C:\Users\pierr\OneDrive\Documents\Arduino\libraries\cocorico2\ModbusSensor.h"
#include <WebSocketsServer.h>
#include <C:\Users\pierr\OneDrive\Documents\Arduino\libraries\Mduino\Ethernet\src\Ethernet.h>

#include <SD.h>

#include <RTC.h>

//#include <IndustrialShields.h>
//#include <RS485.h>

//Pinout for mduino 42

const byte PIN_DEBITMETRE_1 = 56;
const byte PIN_DEBITMETRE_2 = 57;
const byte PIN_DEBITMETRE_3 = 58;
const byte PIN_NIVEAU_H_1 = 24;
const byte PIN_NIVEAU_L_1 = 2;
const byte PIN_NIVEAU_LL_1 = 55;
const byte PIN_NIVEAU_H_2 = 23;
const byte PIN_NIVEAU_L_2 = 26;
const byte PIN_NIVEAU_LL_2 = 54;
const byte PIN_NIVEAU_H_3 = 22;
const byte PIN_NIVEAU_L_3 = 25;
const byte PIN_NIVEAU_LL_3 = 3;

const byte PIN_NIVEAU_L_CO2 = 18;
const byte PIN_NIVEAU_H_CO2 = 19;
const byte PIN_VANNE_EAU_CO2 = 39;

const byte PIN_VANNE_CO2 = 38;
const byte PIN_VANNE_EXONDATION = 36;
const byte PIN_LED = 37;
const byte PIN_CAPTEUR_FLUO = 59;

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEA };
// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 1, 1);


WebSocketsServer webSocket = WebSocketsServer(81);


/**
 *  Modbus object declaration
 *  u8id : node id = 0 for master, = 1..247 for slave
 *  u8serno : serial port (use 0 for Serial)
 *  u8txenpin : 0 for RS-232 and USB-FTDI
 *               or any pin number > 1 for RS-485
 */
//Modbus master(0, 2, 45); // CONTROLLINO
Modbus master(0, 3, 46); // MDUINO

ModbusSensorHamilton Hamilton[5];// indexes O to 2 are mesocosms, index 4 is input measure tank, index 3 is acidification tank

ModbusSensor Podoc(10, &master);
ModbusSensor NTU(40, &master);
ModbusSensor PC4E(30, &master);

typedef struct tempo {
    unsigned long debut;
    unsigned long interval;
}tempo;

tempo tempoSensorRead;
tempo tempoRegulTemp;
tempo tempoRegulpH;
tempo tempoCheckMeso;
tempo tempoSendValues;
tempo tempoSendParams;
tempo tempowriteSD;
tempo tempoCheckSun;
tempo tempoCO2ValvePWM_on;
tempo tempoCO2ValvePWM_off;
tempo tempoCheckWaterLevelCO2;

int sensorIndex = 0;
bool calib = false;
bool pH = true;

bool toggleCO2Valve = false;
bool remplissageCO2 = false;

char buffer[600];
uint8_t AppSocketId = -1;


enum {
    REQ_PARAMS = 0,
    REQ_DATA = 1,
    SEND_PARAMS = 2,
    SEND_DATA = 3,
    CALIBRATE_SENSOR = 4,
    REQ_MASTER_DATA = 5,
    SEND_MASTER_DATA = 6,
    SEND_TIME =7
};

Condition condition[4];

typedef struct MasterData {
    double oxy;
    double cond;
    double turb;
    double fluo;
    double pH;
    double temperature;
    uint32_t nextSunUp;
    uint32_t nextSunDown;
    uint32_t nextTideHigh;
    uint32_t nextTideLow;
    bool currentSun;
    bool currentTide;
}MasterData;

MasterData masterData;


void save(int startAddress) {
    int address = startAddress;
    for (int i = 0; i < 4; i++) {
        address = condition[i].save();
    }
}

void load(int startAddress) {
    int address = startAddress;
    for (int i = 0; i < 4; i++) {
        condition[i].startAddress = address;
        address = condition[i].load();
    }
}

int currentDay;

void setup() {
    pinMode(PIN_DEBITMETRE_1, INPUT);
    pinMode(PIN_DEBITMETRE_2, INPUT);
    pinMode(PIN_DEBITMETRE_3, INPUT);
    pinMode(PIN_NIVEAU_H_1, INPUT);
    pinMode(PIN_NIVEAU_L_1, INPUT);
    pinMode(PIN_NIVEAU_LL_1, INPUT);
    pinMode(PIN_NIVEAU_H_2, INPUT);
    pinMode(PIN_NIVEAU_L_2, INPUT);
    pinMode(PIN_NIVEAU_LL_2, INPUT);
    pinMode(PIN_NIVEAU_H_3, INPUT);
    pinMode(PIN_NIVEAU_L_3, INPUT);
    pinMode(PIN_NIVEAU_LL_3, INPUT);
    pinMode(PIN_CAPTEUR_FLUO, INPUT);

    pinMode(PIN_NIVEAU_L_CO2, INPUT);
    pinMode(PIN_NIVEAU_H_CO2, INPUT);
    
    pinMode(PIN_VANNE_EAU_CO2, OUTPUT);
    pinMode(PIN_VANNE_CO2, OUTPUT);
    pinMode(PIN_VANNE_EXONDATION, OUTPUT);
    pinMode(PIN_LED, OUTPUT);

    //Controllino_RTC_init();
    //Controllino_SetTimeDateStrings(__DATE__, __TIME__); /* set compilation time to the RTC chip */

    Serial.begin(115200);
    master.begin(9600); // baud-rate at 19200
    master.setTimeOut(2000); // if there is no answer in 5000 ms, roll over

    /*
    Condition 0 = Ambiant temperature
    */
    for (uint8_t i = 0; i < 5; i++) Hamilton[i].setSensor(i + 1, &master);
    condition[0].Meso[0] = Mesocosme(PIN_DEBITMETRE_1, PIN_NIVEAU_H_1, PIN_NIVEAU_L_1, PIN_NIVEAU_LL_1, 0);
    condition[0].Meso[1] = Mesocosme(PIN_DEBITMETRE_2, PIN_NIVEAU_H_2, PIN_NIVEAU_L_2, PIN_NIVEAU_LL_2, 1);
    condition[0].Meso[2] = Mesocosme(PIN_DEBITMETRE_3, PIN_NIVEAU_H_3, PIN_NIVEAU_L_3, PIN_NIVEAU_LL_3, 2);
    condition[0].Meso[0].debit = 0;
    condition[0].Meso[1].debit = 0;
    condition[0].Meso[2].debit = 0;

    load(2);

    for (uint8_t i = 0; i < 4; i++) {
        condition[i].condID = i;
        condition[i].socketID = -1;
        for(uint8_t j=0;j<3;j++) condition[i].Meso[j] = Mesocosme(j);
        condition[i].save();
        //condition[i].regulpH.consigne = 0;
        //condition[i].regulTemp.consigne = 0;
    }

    tempoSensorRead.interval = 200;
    tempoRegulTemp.interval = 100;
    tempoRegulpH.interval = 100;
    tempoCheckMeso.interval = 200;
    tempoSendValues.interval = 5000;
    tempowriteSD.interval = 5000;
    tempoCheckSun.interval = 60*1000;
    tempoSendParams.interval = 5000;
    tempoCheckWaterLevelCO2.interval = 1000;

    tempoSensorRead.debut = millis() + 2000;

    
    Ethernet.begin(mac, ip);

    Serial.println(F("START"));
    int i = 0;
    while (i<20) {
        if (Ethernet.hardwareStatus() != EthernetNoHardware) {
            Serial.println(F("Ethernet STARTED"));
            break;
        }
        delay(500); // do nothing, no point running without Ethernet hardware
        i++;
    }

    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    

    /*RTC.setYear(YEAR);                      //sets year
    RTC.setMonth(MONTH);                   //sets month
    RTC.setMonthDay(DAY);                   //sets day
    RTC.setHour(HOUR);                      //sets hour
    RTC.setMinute(MINUTE);                  //sets minute
    RTC.setSecond(SECOND);                  //sets second

    RTC.write();*/
    

    RTC.read();
    currentDay = RTC.getMonthDay();

    setPIDparams();

    SD.begin(53);

    updateSunTimes();
    updateTideTimes();

}

void loop() {
    RTC.read();
    //if(!calib) requestSensors();
    //else calibration();
    //requestAllStatus();

    readMBSensors();

    webSocket.loop();

    regulationpH();

    
    checkMesocosmes();
    checkWaterLevelCO2();
    printToSD();

    if (elapsed(&tempoCheckSun.debut, tempoCheckSun.interval)) {
        checkTide();
        checkSun();
        if (currentDay != RTC.getMonthDay()) {
            currentDay = RTC.getMonthDay();
            updateSunTimes();
        }
    }

    updateParams();
    condition[0].condID = 0;
}

bool elapsed(unsigned long* previousMillis, unsigned long interval) {
    if (*previousMillis == 0) {
        *previousMillis = millis();
    }
    else {
        if ((unsigned long)(millis() - *previousMillis) >= interval) {
            *previousMillis = 0;
            return true;
        }
    }
    return false;
}

bool checkWaterLevelCO2() {
    if (elapsed(&tempoCheckWaterLevelCO2.debut, tempoCheckWaterLevelCO2.interval)) {

        /*if (!remplissageCO2) {
            if (!digitalRead(PIN_NIVEAU_L_CO2)) remplissageCO2 = true;
        }
        else {
            if (digitalRead(PIN_NIVEAU_H_CO2)) {
                remplissageCO2 = false;
            }
        }*/
        digitalWrite(PIN_NIVEAU_L_CO2, LOW);
        digitalWrite(PIN_NIVEAU_H_CO2, LOW);
        Serial.print("FDC BAS:"); Serial.println(digitalRead(PIN_NIVEAU_L_CO2));
        Serial.print("FDC HAUT:"); Serial.println(digitalRead(PIN_NIVEAU_H_CO2));

        if (digitalRead(PIN_NIVEAU_H_CO2)) remplissageCO2 = true;
        else remplissageCO2 = false;
        digitalWrite(PIN_VANNE_EAU_CO2, remplissageCO2);
       // digitalWrite(PIN_VANNE_EAU_CO2, true);
        return remplissageCO2;
    }
}

void updateParams() {
    if(elapsed(&tempoSendParams.debut, tempoSendParams.interval)) sendParams();
}

unsigned long dateToTimestamp(int year, int month, int day, int hour, int minute) {

    tmElements_t te;  //Time elements structure
    time_t unixTime; // a time stamp
    te.Day = day;
    te.Hour = hour;
    te.Minute = minute;
    te.Month = month;
    te.Second = 0;
    te.Year = year-1970;
    unixTime = makeTime(te);
    return unixTime;
}
void checkMesocosmes() {
    if (elapsed(&tempoCheckMeso.debut, tempoCheckMeso.interval)) {
        for (int i = 0; i < 3; i++) {
            condition[0].Meso[i].readFlow(10);
            condition[0].Meso[i].checkLevel();
        }
    }

}


bool checkSun() {
    RTC.read();
    uint32_t currentTime = RTC.getTime();
    if (masterData.currentSun) {
        if (masterData.nextSunDown == currentTime) {
            masterData.currentSun = false;
        }
    }
    else {
        if (masterData.nextSunUp == currentTime) {
            masterData.currentSun = true;
        }
    }
    digitalWrite(PIN_LED, masterData.currentSun);
    return masterData.currentSun;
}

bool checkTide() {
    RTC.read();
    uint32_t currentTime = RTC.getTime();
    if (masterData.currentTide) {//marée basse
        if (masterData.nextTideHigh == currentTime) {
            masterData.currentTide = false;
            updateTideTimes();
        }
    }
    else {
        if (masterData.nextTideLow == currentTime) {
            masterData.currentTide = true;
            updateTideTimes();
        }
    }
    digitalWrite(PIN_VANNE_EXONDATION, masterData.currentTide);
    return masterData.currentTide;
}

void updateSunTimes() {
    const char* path = "param/sun.csv";
    File dataFile = SD.open(path, FILE_READ);
    uint8_t currentDay = RTC.getMonthDay();
    uint8_t currentMonth = RTC.getMonth();
    uint16_t currentYear = RTC.getYear();


    if (dataFile) {
        String line = dataFile.readStringUntil('\n');
        while (line.length() > 0) {
            //Serial.print("line:"); Serial.println(line);
            String date = line.substring(0, line.indexOf(';'));
            int day = date.substring(0, line.indexOf('/')).toInt();
            int month = date.substring(line.indexOf('/') + 1, line.lastIndexOf('/')).toInt();


            if (day == currentDay && month == currentMonth) {
                String start = line.substring(line.indexOf(';') + 1, line.lastIndexOf(';'));
                String end = line.substring(line.lastIndexOf(';') + 1);
                int startHour = start.substring(0, start.indexOf(':')).toInt();
                int startMinute = start.substring(start.indexOf(':') + 1).toInt();
                int endHour = end.substring(0, end.indexOf(':')).toInt();
                int endMinute = end.substring(end.indexOf(':') + 1).toInt();

                masterData.nextSunUp = dateToTimestamp(currentYear, month, day, startHour, startMinute);
                masterData.nextSunDown = dateToTimestamp(currentYear, month, day, endHour, endMinute);

                Serial.print("masterData.nextSunUp:"); Serial.println(masterData.nextSunUp);
                Serial.print("masterData.nextSunDown :"); Serial.println(masterData.nextSunDown);

                /*Serial.print("day:"); Serial.println(day);
                Serial.print("month:"); Serial.println(month);
                Serial.print("year:"); Serial.println(year);
                Serial.print("start:"); Serial.println(start);
                Serial.print("end:"); Serial.println(end);
                Serial.print("startHour:"); Serial.println(startHour);
                Serial.print("start Minute:"); Serial.println(startMinute);
                Serial.print("end Hour:"); Serial.println(endHour);
                Serial.print("end Minute:"); Serial.println(endMinute);*/
                break;
            }

            line = dataFile.readStringUntil('\n');
        }
        dataFile.close();

    }
}

void updateTideTimes() {
    //masterData.currentTide = true;
    //masterData.nextTideHigh = RTC.getTime() + 1000;
    //masterData.nextTideLow = RTC.getTime() + 2000;
    const char* path = "param/tide.csv";
    File dataFile = SD.open(path, FILE_READ);
    uint8_t currentDay = RTC.getMonthDay();
    uint8_t currentMonth = RTC.getMonth();
    uint16_t currentYear = RTC.getYear();


    if (dataFile) {
        String line = dataFile.readStringUntil('\n');
        unsigned long timeStamp = 0;
        while (line.length() > 0 ) {
            //Serial.print("line:"); Serial.println(line);
            String date = line.substring(0, line.indexOf(';'));
            int day = date.substring(0, line.indexOf('/')).toInt();
            int month = date.substring(line.indexOf('/') + 1, line.lastIndexOf('/')).toInt();
            String time = line.substring(line.indexOf(';') + 1, line.lastIndexOf(';'));
            int hour = time.substring(0, time.indexOf(':')).toInt();
            int minute = time.substring(time.indexOf(':') + 1).toInt();
            timeStamp = dateToTimestamp(currentYear, month, day, hour, minute);

            if (timeStamp > RTC.getTime()) {
                double height = line.substring(line.lastIndexOf(';') + 1).toDouble();
                if (height < 5) masterData.nextTideLow = timeStamp;
                else masterData.nextTideHigh = timeStamp;
                line = dataFile.readStringUntil('\n');

                date = line.substring(0, line.indexOf(';'));
                day = date.substring(0, line.indexOf('/')).toInt();
                month = date.substring(line.indexOf('/') + 1, line.lastIndexOf('/')).toInt();
                time = line.substring(line.indexOf(';') + 1, line.lastIndexOf(';'));
                hour = time.substring(0, time.indexOf(':')).toInt();
                minute = time.substring(time.indexOf(':') + 1).toInt();
                timeStamp = dateToTimestamp(currentYear, month, day, hour, minute);
                height = line.substring(line.lastIndexOf(';') + 1).toDouble();
                if (height < 5) masterData.nextTideLow = timeStamp;
                else masterData.nextTideHigh = timeStamp;

                Serial.print("masterData.nextTideLow:"); Serial.println(masterData.nextTideLow);
                Serial.print("masterData.nextTideHigh :"); Serial.println(masterData.nextTideHigh);

                break;
            }

            line = dataFile.readStringUntil('\n');
        }
        dataFile.close();

    }
}

void printToSD() {

    if (elapsed(&tempowriteSD.debut, tempowriteSD.interval)) {
        // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
        String path =  String("data/"+RTC.getMonth()) + "_" + String(RTC.getYear()) + ".csv";
        //Serial.println(path);
        
        if (!SD.exists(path)) {
            File dataFile = SD.open(path, FILE_WRITE);
            //Serial.println(F("file does not exist. Writing headers"));

            if (dataFile) {
                dataFile.print(F("timestamp;Sun;Tide;Ambiant.O2;Ambiant.Conductivity;Ambiant.Turbidity;Ambiant.Fluo;Ambiant.Temperature;Ambiant.pH;"));
                for (int i = 0; i < 4; i++) {
                    String header = F("Condition["); header += i; header += F("].Temperature;");
                    header += F("Condition["); header += i; header += F("].pH;");
                    header += F("Condition["); header += i; header += F("].consigne_pH;");
                    header += F("Condition["); header += i; header += F("].sortiePID_pH;");
                    if (i > 0) {
                        header += F("Condition["); header += i; header += F("].consigne_Temperature;");
                        header += F("Condition["); header += i; header += F("].sortiePID_Temperature;");
                    }
                    dataFile.print(header);
                    for (int j = 0; j < 3; j++) {
                        header = F("Condition["); header += i; header += F("].Meso["); header += j; header += F("].Temperature;"); dataFile.print(header);
                        header = F("Condition["); header += i; header += F("].Meso["); header += j; header += F("].pH;"); dataFile.print(header);
                        header = F("Condition["); header += i; header += F("].Meso["); header += j; header += F("].FlowRate;"); dataFile.print(header);
                        header = F("Condition["); header += i; header += F("].Meso["); header += j; header += F("].LevelH;"); dataFile.print(header);
                        header = F("Condition["); header += i; header += F("].Meso["); header += j; header += F("].LevelL;"); dataFile.print(header);
                        header = F("Condition["); header += i; header += F("].Meso["); header += j; header += F("].LevelLL;"); dataFile.print(header);
                    }
                }
                dataFile.println();
                dataFile.close();
                // print to the serial port too:
            }
            else {
                //Serial.println(F("Error opening file"));
            }
        }
        File dataFile = SD.open(path, FILE_WRITE);
        
        
        // if the file is available, write to it:
        if (dataFile) {
            char sep = ';';
            dataFile.print(RTC.getTime()); dataFile.print(sep); dataFile.print(masterData.currentSun); dataFile.print(sep); dataFile.print(masterData.currentTide); dataFile.print(sep); dataFile.print(masterData.oxy); dataFile.print(sep); dataFile.print(masterData.cond); dataFile.print(sep); dataFile.print(masterData.turb); dataFile.print(sep); dataFile.print(masterData.fluo); dataFile.print(sep); dataFile.print(masterData.temperature); dataFile.print(sep); dataFile.print(masterData.pH); dataFile.print(sep);
            
            for (int i = 0; i < 4; i++) {
                dataFile.print(condition[i].mesureTemperature); dataFile.print(sep);
                dataFile.print(condition[i].mesurepH); dataFile.print(sep);
                dataFile.print(condition[i].regulpH.consigne); dataFile.print(sep);
                dataFile.print(condition[i].regulpH.sortiePID_pc); dataFile.print(sep);
                if (i > 0) {
                    dataFile.print(condition[i].regulTemp.consigne); dataFile.print(sep);
                    dataFile.print(condition[i].regulTemp.sortiePID_pc); dataFile.print(sep);
                }

                for (int j = 0; j < 3; j++) {
                    dataFile.print(condition[i].Meso[j].temperature); dataFile.print(sep);
                    dataFile.print(condition[i].Meso[j].pH); dataFile.print(sep);
                    dataFile.print(condition[i].Meso[j].debit); dataFile.print(sep);
                    dataFile.print(condition[i].Meso[j].alarmeNiveauHaut); dataFile.print(sep);
                    dataFile.print(condition[i].Meso[j].alarmeNiveauBas); dataFile.print(sep);
                    dataFile.print(condition[i].Meso[j].alarmeNiveauTresBas); dataFile.print(sep);
                }
            }
            dataFile.println("");
            dataFile.close();
            // print to the serial port too:
        }
        // if the file isn't open, pop up an error:
        else {
            Serial.println(F("error opening file"));
        }
    }
    
}


void setPIDparams() {
    condition[0].regulpH.pid = PID((double*)&Hamilton[3].pH_sensorValue, &condition[0].regulpH.sortiePID, &condition[0].regulpH.consigne, condition[0].regulpH.Kp, condition[0].regulpH.Ki, condition[0].regulpH.Kd, DIRECT);
    condition[0].regulpH.pid.SetOutputLimits(0, 100);
    condition[0].regulpH.pid.SetMode(AUTOMATIC);
    condition[0].regulpH.pid.SetControllerDirection(REVERSE);
}

int regulationpH() {
    int dutyCycle = 0;
    if (elapsed(&tempoRegulpH.debut, tempoRegulpH.interval)) {
        
        if (condition[0].regulpH.autorisationForcage) {
            if (condition[0].regulpH.consigneForcage > 0 && condition[0].regulpH.consigneForcage <= 100) {
                dutyCycle = condition[0].regulpH.consigneForcage;
            }
            else {
                digitalWrite(PIN_VANNE_CO2, 0);
                return 0;
            }
        }
        else {

            condition[0].regulpH.pid.Compute();
            condition[0].regulpH.sortiePID_pc = (int)condition[0].regulpH.sortiePID;
            dutyCycle = condition[0].regulpH.sortiePID;
        }
        unsigned long cycleDuration = 10000;
        unsigned long  onTime = dutyCycle * cycleDuration / 100;
        unsigned long  offTime = cycleDuration - onTime;
        if(onTime==0) toggleCO2Valve = false;
        else if(offTime ==0) toggleCO2Valve = true;
        else if (toggleCO2Valve) {
            if (elapsed(&tempoCO2ValvePWM_on.debut, onTime)) {
                tempoCO2ValvePWM_off.debut = millis();
                toggleCO2Valve = false;
                Serial.println("TOGGLE OFF");
            }
        }
        else {
            if (elapsed(&tempoCO2ValvePWM_off.debut, offTime)) {
                tempoCO2ValvePWM_on.debut = millis();
                toggleCO2Valve = true;
                Serial.println("TOGGLE ON");
            }
        }
        digitalWrite(PIN_VANNE_CO2, toggleCO2Valve);
        
    }
    return dutyCycle;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t lenght) {

    switch (type) {
    case WStype_DISCONNECTED:
        //Serial.print(num); Serial.println(" Disconnected!");
        break;
    case WStype_CONNECTED:
        Serial.print(num); Serial.println(F(" Connected!"));

        // send message to client
        webSocket.sendTXT(num, F("Connected"));
        break;
    case WStype_TEXT:

        //Serial.print(num); Serial.print(F(" Payload:")); Serial.println((char*)payload);
        Serial.print("Payload received from "); Serial.println(num); Serial.println(": "); Serial.println((char*)payload);
        readJSON((char*)payload, num);


        // send message to client
        // webSocket.sendTXT(num, "message here");

        // send data to all connected clients
        // webSocket.broadcastTXT("message here");
        break;
    case WStype_ERROR:
        Serial.print(num); Serial.println(F(" ERROR!"));
        break;
    }
}

void readJSON(char* json, uint8_t num) {
    StaticJsonDocument<512> doc;
    deserializeJson(doc, json);

    uint8_t command = doc["command"];
    uint8_t condID = doc["condID"];
    uint8_t senderID = doc["senderID"];


    if (senderID == 4) {
        uint32_t time = doc["time"];
        if (time > 0) {
            RTC.setTime(time);
            RTC.write();
        }
    }
        
    //if (senderID == 4) {//La trame vient du PC de supervision
        //AppSocketId = num;
        switch (command) {
        case REQ_PARAMS:
            condition[condID].load();
            if (condID > 0) {
                condition[condID].regulpH.consigne = condition[condID].regulpH.offset + masterData.pH;
                condition[condID].regulTemp.consigne = condition[condID].regulTemp.offset + masterData.temperature;
            }
            condition[condID].serializeParams(buffer, RTC.getTime(),0);
            Serial.print("SEND PARAMS:"); Serial.println(buffer);
            webSocket.sendTXT(num, buffer);
            break;
        case REQ_DATA:
            if (condID > 0) {
                condition[condID].regulpH.consigne = condition[condID].regulpH.offset + masterData.pH;
                condition[condID].regulTemp.consigne = condition[condID].regulTemp.offset + masterData.temperature;
            }
            condition[condID].serializeData(buffer, RTC.getTime(),0);
            webSocket.sendTXT(num, buffer);
            break;
        case SEND_PARAMS:
            condition[condID].load();
            condition[condID].deserializeParams(doc);
            if (condID > 0) {
                condition[condID].regulpH.consigne = condition[condID].regulpH.offset + masterData.pH;
                condition[condID].regulTemp.consigne = condition[condID].regulTemp.offset + masterData.temperature;
            }
            condition[condID].save();
            
            

            //if (condID > 0) webSocket.broadcastTXT(buffer); //On forward le parametrage
            if (senderID == 4) {
                condition[condID].serializeParams(buffer, RTC.getTime(), 0);
                webSocket.sendTXT(num, buffer); 
                               
            }
            if(condID == 0) setPIDparams();


            break;
        case SEND_DATA:
            condition[condID].load();
            condition[condID].deserializeData(doc);
            if (condID > 0) {
                condition[condID].regulpH.consigne = condition[condID].regulpH.offset + masterData.pH;
                condition[condID].regulTemp.consigne = condition[condID].regulTemp.offset + masterData.temperature;
            }
            condition[condID].serializeParams(buffer, RTC.getTime(), 0);
            webSocket.sendTXT(num, buffer);
            //webSocket.sendTXT(num, json);
            break;
        case REQ_MASTER_DATA:
            SerializeMasterData(buffer, RTC.getTime());
            webSocket.sendTXT(num, buffer);
            break;

            /*case CALIBRATE_SENSOR:
                readCalibRequest(doc);
                break;*/
        default:
            webSocket.sendTXT(num, F("wrong request1"));
            break;
        }

    //}
    /*else {//La trame vient d'un automate slave
        switch (command) {
        case REQ_PARAMS:
            condition[condID].load();
            if (condID > 0) {
                condition[condID].regulpH.consigne = condition[condID].regulpH.offset + masterData.pH;
                condition[condID].regulTemp.consigne = condition[condID].regulTemp.offset + masterData.temperature;
            }
            condition[condID].serializeParams(buffer, RTC.getTime(),0);
            webSocket.sendTXT(num, buffer);
            break;
        case SEND_PARAMS:
            condition[condID].deserializeParams(doc);
            if (condID > 0) {
                condition[condID].regulpH.consigne = condition[condID].regulpH.offset + masterData.pH;
                condition[condID].regulTemp.consigne = condition[condID].regulTemp.offset + masterData.temperature;
            }
            condition[condID].save();
            condition[condID].serializeParams(buffer, RTC.getTime(),0);
            //webSocket.sendTXT(num, buffer);
            if(AppSocketId >0 && AppSocketId < 10) webSocket.sendTXT(AppSocketId, buffer);


            break;
        case SEND_DATA:
            condition[condID].deserializeData(doc);
            //webSocket.sendTXT(num, json);
            break;

            
        default:
            webSocket.sendTXT(num, F("wrong request2"));
            break;
        }
        
    }*/
    //Serial.print(F("Buffer:")); Serial.println(buffer);
}

double readFluo() {
    int value = analogRead(PIN_CAPTEUR_FLUO);  // pin 0-10V

    int Fluo_mV = map(value, 0, 1024, 0, 1000); //passage en 0-10V
    int Fluo = map(Fluo_mV, 0, 500, 0, 40000); // passage en micro gramme /L (max = 40 micro G)
    double  F = Fluo / 100000.0; //micro gremme / litre sans le gain de 100;
    masterData.fluo = F;
    return F;
}

bool SerializeMasterData(char* buffer, uint32_t timeString) {
    //Serial.println(F("SENDDATA"));
    StaticJsonDocument<512> doc;

    doc[F("command")] = SEND_MASTER_DATA;
    doc[F("condID")] = 0;
    doc[F("senderID")] = 0;
    doc[F("time")] = timeString;

    if (masterData.currentSun) doc[F("sun")] = "true";
    else doc[F("sun")] = "false";
    if (masterData.currentTide) doc[F("tide")] = "true";
    else doc[F("tide")] = "false";
    doc[F("oxy")] = masterData.oxy;
    doc[F("cond")] = masterData.cond;
    doc[F("turb")] = masterData.turb;
    doc[F("fluo")] = masterData.fluo;
    doc[F("pH")] = masterData.pH;
    doc[F("temperature")] = masterData.temperature;

    serializeJson(doc, buffer, 600);
    return true;
}

void sendParams() {
    for (int i = 1; i < 4; i++) {
        condition[i].load();
        condition[i].condID = i;
        condition[i].regulpH.consigne = condition[i].regulpH.offset + masterData.pH;
        condition[i].regulTemp.consigne = condition[i].regulTemp.offset + masterData.temperature;
        condition[i].serializeParams(buffer, RTC.getTime(),0);
        //webSocket.broadcastTXT(buffer);
    }    
}



/*
{
   "command":4,
   "CondID":1,
   "sensorID":1,
   "pHValue":6.2,
   "tempValue":21.4
}
*/

float checkValue(float val, float min, float max, float def) {
    if (val > min && val <= max) return val;
    return def;
}


/*void requestAllStatus() {
    if (elapsed(&tempoSensorRead.debut, tempoSensorRead.interval)) {
        switch (state) {
        case 0:
            Serial.println("PODOC:");
            if (Podoc.requestStatus()) {
                state++;
            }
            break;
        case 1:
            Serial.println("NTU:");
            if (NTU.requestStatus()) {
                state++;
            }
            break;
        case 2:
            Serial.println("PC4E:");
            if (PC4E.requestStatus()) {
                state = 0;
            }
            break;
        }
    }
}*/

int state = 0;

void readMBSensors() {
    if (elapsed(&tempoSensorRead.debut, tempoSensorRead.interval)) {
        readFluo();
        if (sensorIndex < 5) {
            if (pH) {
                if (Hamilton[sensorIndex].readPH()) {
                    //Serial.print("sensor address:"); Serial.println(Hamilton[sensorIndex].query.u8id);
                    //Serial.print(F("pH:")); Serial.println(Hamilton[sensorIndex].pH_sensorValue);
                    if (sensorIndex < 3) condition[0].Meso[sensorIndex].pH = Hamilton[sensorIndex].pH_sensorValue;
                    if (sensorIndex == 3) condition[0].mesurepH = Hamilton[sensorIndex].pH_sensorValue;
                    if (sensorIndex == 4) masterData.pH = Hamilton[sensorIndex].pH_sensorValue;
                    pH = false;
                }
            }
            else {
                if (Hamilton[sensorIndex].readTemp()) {
                    //Serial.print(F("Temperature:")); Serial.println(Hamilton[sensorIndex].temp_sensorValue);
                    if (sensorIndex < 3) condition[0].Meso[sensorIndex].temperature = Hamilton[sensorIndex].temp_sensorValue;
                    if (sensorIndex == 3) condition[0].mesureTemperature = Hamilton[sensorIndex].temp_sensorValue;
                    if (sensorIndex == 4) masterData.temperature = Hamilton[sensorIndex].temp_sensorValue;
                    sensorIndex++;
                    pH = true;
                }
            }
        }
        else {
            switch (sensorIndex) {
            case 5:
                if (state == 0) {
                    if (Podoc.requestValues()) {
                        state = 1;
                    }
                }                
                else if (Podoc.readValues()) {
                    Serial.print(F("Temperature:")); Serial.println(Podoc.params[0]);
                   Serial.print(F("oxy %:")); Serial.println(Podoc.params[1]);
                    Serial.print(F("oxy mg/L:")); Serial.println(Podoc.params[2]);
                    Serial.print(F("oxy ppm:")); Serial.println(Podoc.params[3]);
                    masterData.oxy = Podoc.params[2];
                    state = 0;
                    sensorIndex++;
                }
                break;
            case 6:
                if (state == 0) {
                    if (NTU.requestValues()) {
                        state = 1;
                    }
                }
                else if (NTU.readValues()) {
                    //Serial.print(F("Temperature:")); Serial.println(NTU.params[0]);
                    //Serial.print(F("NTU:")); Serial.println(NTU.params[1]);
                    //Serial.print(F("FNU:")); Serial.println(NTU.params[2]);
                    //Serial.print(F("mg/L:")); Serial.println(NTU.params[3]);
                    masterData.turb = NTU.params[1];
                    state = 0;
                    sensorIndex++;
                }
                break;
            case 7:
                if (state == 0) {
                    if (PC4E.requestValues()) {
                        state = 1;
                    }
                }
                else if (PC4E.readValues()) {
                    Serial.print(F("Temperature:")); Serial.println(PC4E.params[0]);
                    Serial.print(F("conductivite:")); Serial.println(PC4E.params[1]);
                    Serial.print(F("salinite:")); Serial.println(PC4E.params[2]);
                    Serial.print(F("TDS:")); Serial.println(PC4E.params[3]);
                    masterData.cond = PC4E.params[1];
                    state = 0;
                    sensorIndex = 0;
                }
                break;
            case 8:
                //TODO: FLUO
                break;
            }
            
        }

        
    }
}


/*void requestSensors() {
    if (elapsed(&debutReq, 500)) {
        switch (state) {
        case 0:
            if (Hamilton.readPH()) {
                Serial.print("pH:"); Serial.println(Hamilton.pH_sensorValue);
                state ++;
            }
            break;
        case 1:
            if (Hamilton.readTemp()) {
                Serial.print("Temperature:"); Serial.println(Hamilton.temp_sensorValue);
                state++;
            }
            break;
        case 2:
            if (Podoc.requestValues()) {
                state ++;
            }
            break;
        case 3:
            if (Podoc.readValues()) {
                Serial.print("Temperature:"); Serial.println(Podoc.params[0]);
                Serial.print("oxy %:"); Serial.println(Podoc.params[1]);
                Serial.print("oxy mg/L:"); Serial.println(Podoc.params[2]);
                Serial.print("oxy ppm:"); Serial.println(Podoc.params[3]);
                state++;
            }
            break;
        case 4:
            if (NTU.requestValues()) {
                state ++;
            }
            break;
        case 5:
            if (NTU.readValues()) {
                Serial.print("Temperature:"); Serial.println(NTU.params[0]);
                Serial.print("NTU:"); Serial.println(NTU.params[1]);
                Serial.print("FNU:"); Serial.println(NTU.params[2]);
                Serial.print("mg/L:"); Serial.println(NTU.params[3]);
                state ++;
            }
            break;
        case 6:
            if (PC4E.requestValues()) {
                state++;
            }
            break;
        case 7:
            if (PC4E.readValues()) {
                Serial.print("Temperature:"); Serial.println(PC4E.params[0]);
                Serial.print("conductivite:"); Serial.println(PC4E.params[1]);
                Serial.print("salinite:"); Serial.println(PC4E.params[2]);
                Serial.print("TDS:"); Serial.println(PC4E.params[3]);
                step = 0;
                state = 0;
                //calib = true;
            }
            break;
        }
    }
}*/

/*void calibration() {

    if (elapsed(&debutReq, 500)) {
        switch (stateCalibration) {
        case 0:
            if (Podoc.calibrateCoeff(17.0, 514)) {
                Serial.println("calibrate temp");
                stateCalibration++;
            }
            break;
        case 1:
            if (Podoc.validateCalibration(638)) {
                Serial.println("validate calibration temp");
                stateCalibration++;
            }

            break;
        case 2:
            step = Hamilton.calibrate(5.0, step);
            if (step == 3) {
                state = 0;
                calib = false;
                stateCalibration = 0;
            }
            break;
        }
    }
}*/



