#define HAVE_RTC
#define HAVE_RTC_DS1307

#include <TimeLib.h>
#include <WebSockets.h>
#include <ModbusRtu.h>
#include <PID_v1.h>
#include <EEPROMex.h>
#include <ArduinoJson.h>
#include "C:\Users\CRCBN\Desktop\code automates/Libs/Mesocosmes.h"
#include "C:\Users\CRCBN\Desktop\code automates/Libs/Hamilton.h"
#include "C:\Users\CRCBN\Desktop\code automates/Libs/Condition.h"
#include "C:\Users\CRCBN\Desktop\code automates/Libs/ModbusSensor.h"
#include <WebSocketsServer.h>
#include <Ethernet.h>

#include <SD.h>

#include <RTC.h>

//#include <IndustrialShields.h>
//#include <RS485.h>

//Pinout for mduino 42

const byte PIN_DEBITMETRE_0 = 56;
const byte PIN_DEBITMETRE_1 = 57;
const byte PIN_DEBITMETRE_2 = 58;
const byte PIN_NIVEAU_L_0 = 2;
const byte PIN_NIVEAU_LL_0 = 24;
const byte PIN_NIVEAU_L_1 = 26;
const byte PIN_NIVEAU_LL_1 = 23;
const byte PIN_NIVEAU_L_2 = 25;
const byte PIN_NIVEAU_LL_2 = 22;

const byte PIN_PRESSION_EC = 55;
const byte PIN_PRESSION_EA = 54;
const byte PIN_NIVEAU_H_CO2 = 3;

const byte PIN_VANNE_EAU_CO2 = 39;
const byte PIN_V2V_EA = 4;
const byte PIN_V2V_EC = 5;

const byte PIN_VANNE_CO2 = 38;
const byte PIN_VANNE_EXONDATION = 36;
const byte PIN_LED = 37;
const byte PIN_CAPTEUR_FLUO = 59;

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEA };
// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 1, 160);


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

//ModbusSensorHamilton Hamilton[5];// indexes O to 2 are mesocosms, index 4 is input measure tank, index 3 is acidification tank
ModbusSensorHamilton Hamilton;
ModbusSensor mbSensor(10, &master);
/*ModbusSensor Podoc(10, &master);
ModbusSensor NTU(40, &master);
ModbusSensor PC4E(30, &master);*/

typedef struct tempo {
    unsigned long debut;
    unsigned long interval;
}tempo;

tempo tempoSensorRead;
tempo tempoRegul;
tempo tempoCheckMeso;
tempo tempoSendValues;
tempo tempoSendParams;
tempo tempowriteSD;
tempo tempoCheckSun;
tempo tempoCO2ValvePWM_on;
tempo tempoCO2ValvePWM_off;
tempo tempoCheckWaterLevelCO2;

int sensorIndex = 0;

bool pH = true;

bool toggleCO2Valve = false;
//bool remplissageCO2 = false;

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
    REQ_MASTER_PARAMS = 7,
    SEND_MASTER_PARAMS = 8
};

Condition condition[4];

typedef struct Calibration {
    int sensorID;
    int calibParam;
    float value;
    bool calibEnCours;
    bool calibRequested;
}Calibration;

Calibration calib;

typedef struct MasterData {
    double oxy;
    double cond;
    double turb;
    double fluo;
    double pH;
    double temperature;
    double salinite;
    uint32_t nextSunUp;
    uint32_t nextSunDown;
    uint32_t nextTideHigh;
    uint32_t nextTideLow;
    bool currentSun;
    bool currentTide;
    double sortiePID_EA;
    double sortiePID_EC;
    double pression[2];
}MasterData;

MasterData masterData;

Regul regulPression[2];
uint8_t pinV2V_pression[2];
uint8_t pinCapteur_pression[2];


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
    for (int i = 0; i < 2; i++) {
        regulPression[i].startAddress = address;
        address = regulPression[i].load(address);
    }
}

int currentDay;
int currentMin;

void setup() {
    pinV2V_pression[0] = PIN_V2V_EA;
    pinV2V_pression[1] = PIN_V2V_EC;

    pinCapteur_pression[0] = PIN_PRESSION_EA;
    pinCapteur_pression[1] = PIN_PRESSION_EC;

    pinMode(PIN_DEBITMETRE_0, INPUT);
    pinMode(PIN_DEBITMETRE_1, INPUT);
    pinMode(PIN_DEBITMETRE_2, INPUT);
    pinMode(PIN_NIVEAU_L_0, INPUT);
    pinMode(PIN_NIVEAU_LL_0, INPUT);
    pinMode(PIN_NIVEAU_L_1, INPUT);
    pinMode(PIN_NIVEAU_LL_1, INPUT);
    pinMode(PIN_NIVEAU_L_2, INPUT);
    pinMode(PIN_NIVEAU_LL_2, INPUT);
    pinMode(PIN_CAPTEUR_FLUO, INPUT);
    pinMode(PIN_NIVEAU_H_CO2, INPUT);
    pinMode(PIN_PRESSION_EA, INPUT);
    pinMode(PIN_PRESSION_EC, INPUT);

    pinMode(pinCapteur_pression[0], INPUT);
    pinMode(pinCapteur_pression[1], INPUT);

    pinMode(PIN_VANNE_EAU_CO2, OUTPUT);
    pinMode(PIN_V2V_EC, OUTPUT);
    pinMode(PIN_V2V_EA, OUTPUT);
    pinMode(PIN_VANNE_CO2, OUTPUT);
    pinMode(PIN_VANNE_EXONDATION, OUTPUT);
    pinMode(PIN_LED, OUTPUT);
    pinMode(pinV2V_pression[0], OUTPUT);
    pinMode(pinV2V_pression[1], OUTPUT);

    //Controllino_RTC_init();
    //Controllino_SetTimeDateStrings(__DATE__, __TIME__); /* set compilation time to the RTC chip */

    Serial.begin(115200);
    master.begin(9600); // baud-rate at 19200
    master.setTimeOut(2000); // if there is no answer in 5000 ms, roll over

    /*
    Condition 0 = Ambiant temperature
    */
    //for (uint8_t i = 0; i < 5; i++) Hamilton[i].setSensor(i + 1, &master);
    Hamilton.setSensor(0, &master);
    condition[0].Meso[0] = Mesocosme(PIN_DEBITMETRE_0, 0, PIN_NIVEAU_L_0, PIN_NIVEAU_LL_0, 0);
    condition[0].Meso[1] = Mesocosme(PIN_DEBITMETRE_1, 0, PIN_NIVEAU_L_1, PIN_NIVEAU_LL_1, 1);
    condition[0].Meso[2] = Mesocosme(PIN_DEBITMETRE_2, 0, PIN_NIVEAU_L_2, PIN_NIVEAU_LL_2, 2);
    condition[0].Meso[0].debit = 0;
    condition[0].Meso[1].debit = 0;
    condition[0].Meso[2].debit = 0;

    regulPression[0] = Regul();
    regulPression[1] = Regul();

    load(2);

    for (uint8_t i = 0; i < 4; i++) {
        condition[i].condID = i;
        for(uint8_t j=0;j<3;j++) condition[i].Meso[j] = Mesocosme(j);
        condition[i].save();
        //condition[i].regulpH.consigne = 0;
        //condition[i].regulTemp.consigne = 0;
    }

    tempoSensorRead.interval = 200;
    tempoRegul.interval = 100;
    tempoCheckMeso.interval = 200;
    tempoSendValues.interval = 5000;
    tempowriteSD.interval = 5000*60;
    tempoCheckSun.interval = 60*1000;
    tempoSendParams.interval = 5000;
    tempoCheckWaterLevelCO2.interval = 1000;

    tempoSensorRead.debut = millis() + 2000;

    
    Ethernet.begin(mac, ip);

    Serial.println(F("START+"));
    int i = 0;
    while (i<2) {
        if (Ethernet.hardwareStatus() != EthernetNoHardware) {
            Serial.println(F("Ethernet STARTED"));
            break;
        }
        delay(500); // do nothing, no point running without Ethernet hardware
        i++;
    }
    Serial.println(F("+++"));

    if (Ethernet.hardwareStatus() != EthernetNoHardware) {
        webSocket.begin();
        webSocket.onEvent(webSocketEvent);
    }
    Serial.println(F("end websocket"));
    
    

    /*RTC.setYear(2021);                      //sets year
    RTC.setMonth(7);                   //sets month
    RTC.setMonthDay(7);                   //sets day
    RTC.setHour(14);                      //sets hour
    RTC.setMinute(41);                  //sets minute
    RTC.setSecond(0);                  //sets second
    Serial.println(F("End RTC setup"));
    RTC.write();
    Serial.println(F("End RTC write"));*/

    RTC.read();
    Serial.println(F("End RTC read"));
    currentDay = RTC.getMonthDay();
    currentMin = RTC.getMinute();

    Serial.println(F("End RTC"));
    setPIDparams();
    Serial.println(F("End Params"));
    Serial.print(SD.begin(53));
    Serial.println(F("End SD"));
    updateSunTimes();
    updateTideTimes();
    Serial.println(F("End SETUP"));

    checkTide();
    checkSun();

}

void loop() {
    RTC.read();

    readMBSensors();

    webSocket.loop();

    if (elapsed(&tempoRegul.debut, tempoRegul.interval)) {
        regulationpH();
        for (uint8_t i = 0; i < 2; i++) {
            masterData.pression[i] = readPressure(10, pinCapteur_pression[i], masterData.pression[i]);
            regulationPression(i);
        }
    }
    
    checkMesocosmes();
    checkWaterLevelCO2();
    printToSD();

    if (currentMin != RTC.getMinute()) {
        currentMin = RTC.getMinute();
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

       // if (digitalRead(PIN_NIVEAU_H_CO2)) remplissageCO2 = true;
        //else remplissageCO2 = false;
        digitalWrite(PIN_VANNE_EAU_CO2, digitalRead(PIN_NIVEAU_H_CO2));
       // digitalWrite(PIN_VANNE_EAU_CO2, true);
        return digitalRead(PIN_NIVEAU_H_CO2);
    }
}


float readPressure(int lissage, uint8_t pin, double pression) {
    int ana = analogRead(pin); // 0-1023 value corresponding to 0-10 V corresponding to 0-20 mA
    //actually, using 330 ohm resistor so 20mA = 6.6V
    int ana2 = ana * 10 / 6.6;
    int mA = map(ana2, 0, 1023, 0, 2000); //map to milli amps with 2 extra digits
    int mbars = map(mA, 400, 2000, 0, 4000); //map to milli amps with 2 extra digits
    double anciennePression = pression;
    pression = ((double)mbars) / 1000.0; // pressure in bars
    pression = (lissage * pression + (100.0 - lissage) * anciennePression) / 100.0;
    return pression;
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

    Serial.print("current Time :"); Serial.println(currentTime);
    Serial.print("masterData.nextSunUp:"); Serial.println(masterData.nextSunUp);
    Serial.print("masterData.nextSunDown :"); Serial.println(masterData.nextSunDown);

    bool sun = false;

    if (currentTime < masterData.nextSunDown) {
        if (currentTime < masterData.nextSunUp) {
            if (masterData.nextSunDown < masterData.nextSunUp) sun = true;
            else sun = false;
        }
        else {
            sun = true;
        }
    }
    else {
        if (currentTime > masterData.nextSunUp) {
            if (masterData.nextSunDown < masterData.nextSunUp) sun = true;
            else sun = false;
        }
        else {
            sun = false;
        }
    }

    if (sun != masterData.currentSun) {
        updateSunTimes();
    }
    masterData.currentSun = sun;

    Serial.print("current Sun :"); Serial.println(masterData.currentSun);
    digitalWrite(PIN_LED, masterData.currentSun);
    return masterData.currentSun;
}

bool checkTide() {
    RTC.read();
    uint32_t currentTime = RTC.getTime();
    Serial.print("current Time :"); Serial.println(currentTime);
    Serial.print("masterData.nextTideHigh:"); Serial.println(masterData.nextTideHigh);
    Serial.print("masterData.nextTideLow :"); Serial.println(masterData.nextTideLow);

    bool tide = false;

    if (currentTime < masterData.nextTideLow) {
        if (currentTime < masterData.nextTideHigh) {
            if (masterData.nextTideLow < masterData.nextTideHigh) tide = true;
            else tide = false;
        }
        else {
            tide = true;
        }
    }
    else {
        if (currentTime > masterData.nextTideHigh) {
            if (masterData.nextTideLow < masterData.nextTideHigh) tide = true;
            else tide = false;
        }
        else {
            tide = false;
        }
    }
    tide = !tide;//because tide UP = valve closed

    if (tide != masterData.currentTide) {
        updateTideTimes();
    }
    masterData.currentTide = tide;

    Serial.print("current tide:"); Serial.println(masterData.currentTide);
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
        String path = "data/" + String(RTC.getMonth()) + "_" + String(RTC.getYear()) + ".csv";
        //String path = String("dataN.csv");
        Serial.println(path);
        
        if (!SD.exists(path)) {
            File dataFile = SD.open(path, FILE_WRITE);
            Serial.println(F("file does not exist. Writing headers"));

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
    condition[0].regulpH.pid = PID((double*)&condition[0].mesurepH, &condition[0].regulpH.sortiePID, &condition[0].regulpH.consigne, condition[0].regulpH.Kp, condition[0].regulpH.Ki, condition[0].regulpH.Kd, REVERSE);
    condition[0].regulpH.pid.SetOutputLimits(0, 50);
    condition[0].regulpH.pid.SetMode(AUTOMATIC);
    condition[0].regulpH.pid.SetControllerDirection(REVERSE);

    for (int i = 0; i < 2; i++) {
        regulPression[i].pid = PID((double*)&masterData.pression[i], &regulPression[i].sortiePID, &regulPression[i].consigne, regulPression[i].Kp, regulPression[i].Ki, regulPression[i].Kd, DIRECT);
        regulPression[i].pid.SetOutputLimits(50, 255);
        regulPression[i].pid.SetMode(AUTOMATIC);
        regulPression[i].pid.SetControllerDirection(DIRECT);
    }
}


int regulationPression(uint8_t ID) {//0 = Eau ambiante, 1 = Eau Chaude
    if (regulPression[ID].autorisationForcage) {
        if (regulPression[ID].consigneForcage > 0 && regulPression[ID].consigneForcage <= 100) {
            analogWrite(pinV2V_pression[ID], (int)(regulPression[ID].consigneForcage * 255 / 100));
        }
        else {
            analogWrite(pinV2V_pression[ID], 0);
        }
        return regulPression[ID].consigneForcage;
    }
    else {
        //condition.load();
        regulPression[ID].pid.Compute();
        //condition[0].Meso[mesoID].salSortiePID_pc = (int)(condition[0].Meso[mesoID].salSortiePID / 2.55); 
        regulPression[ID].sortiePID_pc = (int)map(regulPression[ID].sortiePID, 50, 255, 0, 100);
        if (regulPression[ID].sortiePID_pc < 0)regulPression[ID].sortiePID_pc = 0;
        analogWrite(pinV2V_pression[ID], regulPression[ID].sortiePID);
        return regulPression[ID].sortiePID_pc;
    }

}

int regulationpH() {
    int dutyCycle = 0;
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
            if (condition[0].mesurepH <= condition[0].regulpH.consigne) {
                condition[0].regulpH.sortiePID = 0;
            }else  condition[0].regulpH.pid.Compute();

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
        //Serial.print("Payload received from "); Serial.println(num); Serial.println(": "); Serial.println((char*)payload);
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

        switch (command) {
        case REQ_PARAMS:
            condition[condID].load();
            if (condID > 0) {
                condition[condID].regulpH.consigne = condition[condID].regulpH.offset + masterData.pH;
                condition[condID].regulTemp.consigne = condition[condID].regulTemp.offset + masterData.temperature;
            }
            condition[condID].serializeParams(buffer, RTC.getTime(),0);
            //Serial.print("SEND PARAMS:"); Serial.println(buffer);
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
        case REQ_MASTER_PARAMS:
            SerializeMasterParams(buffer, RTC.getTime());
            webSocket.sendTXT(num, buffer);
            break;
        case SEND_MASTER_PARAMS:
            deserializeMasterParams(doc);
            for (int i = 0; i < 2; i++) {
                regulPression[i].save(regulPression[i].startAddress);
            }
            setPIDparams();
            SerializeMasterParams(buffer, RTC.getTime());
            webSocket.sendTXT(num, buffer);
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

        case CALIBRATE_SENSOR:
            Serial.println(F("CALIB REQ received"));
            calib.sensorID = doc[F("sensorID")];
            calib.calibParam = doc[F("calibParam")];
            calib.value = doc[F("value")];

            Serial.print(F("calib.sensorID:")); Serial.println(calib.sensorID);
            Serial.print(F("calib.calibParam:")); Serial.println(calib.calibParam);
            Serial.print(F("calib.value:")); Serial.println(calib.value);
            if (condID == 0) {
                //calibrateSensor(calib.mesoID, calib.sensorID, calib.calibParam, calib.value);
                webSocket.sendTXT(num, json);
                calib.calibRequested = true;
            }
            else {
                //webSocket.broadcastTXT(json);
                String msg = "{command:4,condID:";
                msg += String(condID) + ",senderID:4,";
                msg +="sensorID:" + String(calib.sensorID);
                msg += ",calibParam:" + String(calib.calibParam);
                msg += ",value:" + String(calib.value);
                msg += "}";
                webSocket.broadcastTXT(msg);
            }
            break;

        default:
            webSocket.sendTXT(num, F("wrong request"));
            break;
        }
}

void deserializeMasterParams(StaticJsonDocument<512> doc) {

    JsonObject regulPression0 = doc[F("regulPressionEA")];
    regulPression[0].consigne = regulPression0[F("consigne")]; // 24.2
    regulPression[0].Kp = regulPression0[F("Kp")]; // 2.1
    regulPression[0].Ki = regulPression0[F("Ki")]; // 2.1
    regulPression[0].Kd = regulPression0[F("Kd")]; // 2.1
    const char* regul0_autorisationForcage = regulPression0[F("autorisationForcage")];
    if (strcmp(regul0_autorisationForcage, "true") == 0 || strcmp(regul0_autorisationForcage, "True") == 0) regulPression[0].autorisationForcage = true;
    else regulPression[0].autorisationForcage = false;
    regulPression[0].consigneForcage = regulPression0[F("consigneForcage")]; // 2.1
    regulPression[0].offset = regulPression0[F("offset")];

    JsonObject regulPression1 = doc[F("regulPressionEC")];
    regulPression[1].consigne = regulPression1[F("consigne")]; // 24.2
    regulPression[1].Kp = regulPression1[F("Kp")]; // 2.1
    regulPression[1].Ki = regulPression1[F("Ki")]; // 2.1
    regulPression[1].Kd = regulPression1[F("Kd")]; // 2.1
    const char* regul1_autorisationForcage = regulPression1[F("autorisationForcage")];
    if (strcmp(regul1_autorisationForcage, "true") == 0 || strcmp(regul1_autorisationForcage, "True") == 0) regulPression[1].autorisationForcage = true;
    else regulPression[1].autorisationForcage = false;
    regulPression[1].consigneForcage = regulPression1[F("consigneForcage")]; // 2.1
    regulPression[1].offset = regulPression1[F("offset")];

}

bool SerializeMasterParams(char* buffer, uint32_t timeString) {
    //Serial.println(F("SENDDATA"));
    StaticJsonDocument<512> doc;

    doc[F("command")] = SEND_MASTER_PARAMS;
    doc[F("condID")] = 0;
    doc[F("senderID")] = 0;
    doc[F("time")] = timeString;

    JsonObject regulT = doc.createNestedObject(F("regulPressionEA"));
    regulT[F("consigne")] = regulPression[0].consigne;
    regulT[F("Kp")] = regulPression[0].Kp;
    regulT[F("Ki")] = regulPression[0].Ki;
    regulT[F("Kd")] = regulPression[0].Kd;
    if (regulPression[0].autorisationForcage) regulT[F("autorisationForcage")] = "true";
    else regulT[F("autorisationForcage")] = "false";
    regulT[F("consigneForcage")] = regulPression[0].consigneForcage;
    regulT[F("offset")] = regulPression[0].offset;

    JsonObject regulp = doc.createNestedObject(F("regulPressionEC"));
    regulp[F("consigne")] = regulPression[1].consigne;
    regulp[F("Kp")] = regulPression[1].Kp;
    regulp[F("Ki")] = regulPression[1].Ki;
    regulp[F("Kd")] = regulPression[1].Kd;
    if (regulPression[1].autorisationForcage) regulp[F("autorisationForcage")] = "true";
    else regulp[F("autorisationForcage")] = "false";
    regulp[F("consigneForcage")] = regulPression[1].consigneForcage;
    regulp[F("offset")] = regulPression[1].offset;
    serializeJson(doc, buffer, 600);
    return true;
}

double readFluo() {
    int value = analogRead(PIN_CAPTEUR_FLUO);  // pin 0-10V

    int Fluo_mV = map(value, 0, 1024, 0, 1000); //passage en 0-10V
    int Fluo = map(Fluo_mV, 0, 500, 0, 5000); // passage en micro gramme /L (max = 5 ug car gain=x100)
    double  F = Fluo / 1000.0; //micro gremme / litre;
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
    doc[F("salinite")] = masterData.salinite;
    doc[F("temperature")] = masterData.temperature;
    doc[F("pressionEA")] = masterData.pression[0];
    doc[F("pressionEC")] = masterData.pression[1];
    doc[F("sortiePID_EA")] = regulPression[0].sortiePID_pc;
    doc[F("sortiePID_EC")] = regulPression[1].sortiePID_pc;

    doc[F("nextSunUp")] = masterData.nextSunUp;
    doc[F("nextSunDown")] = masterData.nextSunDown;
    doc[F("nextTideHigh")] = masterData.nextTideHigh;
    doc[F("nextTideLow")] = masterData.nextTideLow;

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

int state = 0;

void readMBSensors() {
    if (elapsed(&tempoSensorRead.debut, tempoSensorRead.interval)) {
        Serial.print("Sensor Index:"); Serial.println(sensorIndex);
        
        readFluo();
        if (state == 0 && calib.calibRequested) {
            calib.calibRequested = false;
            calib.calibEnCours = true;
        }
        if (calib.calibEnCours) {
            calibrateSensor();
        }
        else {
            if (sensorIndex < 5) { // HAMILTON: indexes O to 2 are mesocosms, index 4 is input measure tank, index 3 is acidification tank
                //Hamilton.setSensor(sensorIndex + 1, &master);
                Hamilton.query.u8id = sensorIndex + 1;
                if (pH) {
                   
                    if (Hamilton.readPH()) {
                        
                        Serial.print(F("pH:")); Serial.println(Hamilton.pH_sensorValue);
                        if (sensorIndex < 3) condition[0].Meso[sensorIndex].pH = Hamilton.pH_sensorValue;
                        if (sensorIndex == 3) condition[0].mesurepH = Hamilton.pH_sensorValue;
                        if (sensorIndex == 4) masterData.pH = Hamilton.pH_sensorValue;
                        pH = false;
                    }
                }
                else {
                    if (Hamilton.readTemp()) {
                        
                        if (sensorIndex < 3) condition[0].Meso[sensorIndex].temperature = Hamilton.temp_sensorValue;
                        if (sensorIndex == 3) condition[0].mesureTemperature = Hamilton.temp_sensorValue;
                        if (sensorIndex == 4) masterData.temperature = Hamilton.temp_sensorValue;
                        sensorIndex++;
                        pH = true;
                    }
                }
            }
            else {
                switch (sensorIndex) {
                case 5:
                    mbSensor.query.u8id = 10;//PODOC
                    if (state == 0) {
                        if (mbSensor.requestValues()) {
                            state = 1;
                        }
                    }
                    else if (mbSensor.readValues()) {
                        Serial.print(F("Temperature:")); Serial.println(mbSensor.params[0]);
                        Serial.print(F("oxy %:")); Serial.println(mbSensor.params[1]);
                        Serial.print(F("oxy mg/L:")); Serial.println(mbSensor.params[2]);
                        Serial.print(F("oxy ppm:")); Serial.println(mbSensor.params[3]);
                        masterData.oxy = mbSensor.params[1];
                        state = 0;
                        sensorIndex++;
                    }
                    break;
                case 6:
                    Serial.println(F("READ NTU"));
                    mbSensor.query.u8id = 40;//NTU
                    if (state == 0) {
                        if (mbSensor.requestValues()) {
                            state = 1;
                        }
                    }
                    else if (mbSensor.readValues()) {
                        Serial.print(F("Temperature:")); Serial.println(mbSensor.params[0]);
                        Serial.print(F("NTU:")); Serial.println(mbSensor.params[1]);
                        Serial.print(F("FNU:")); Serial.println(mbSensor.params[2]);
                        Serial.print(F("mg/L:")); Serial.println(mbSensor.params[3]);
                        masterData.turb = mbSensor.params[1];
                        state = 0;
                        sensorIndex++;
                    }
                    break;
                case 7:
                    mbSensor.query.u8id = 30;//NTU
                    if (state == 0) {
                        if (mbSensor.requestValues()) {
                            state = 1;
                        }
                    }
                    else if (mbSensor.readValues()) {
                        Serial.print(F("Temperature:")); Serial.println(mbSensor.params[0]);
                        Serial.print(F("conductivite:")); Serial.println(mbSensor.params[1]);
                        Serial.print(F("salinite:")); Serial.println(mbSensor.params[2]);
                        Serial.print(F("TDS:")); Serial.println(mbSensor.params[3]);
                        masterData.cond = mbSensor.params[1];
                        masterData.salinite = mbSensor.params[2];
                        state = 0;
                        sensorIndex = 0;
                    }
                    break;
                }

            }
        }
    }
}

int HamiltonCalibStep = 0;

void calibrateSensor() {
    Serial.println("CALIBRATE SENSOR");
    Serial.print("calib.sensorID:"); Serial.println(calib.sensorID);
    switch (calib.sensorID) {// HAMILTON: indexes O to 2 are mesocosms, index 4 is input measure tank, index 3 is acidification tank

    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
        Serial.println("CALIBRATE PH");
        Serial.print("calib.value:"); Serial.println(calib.value);

        Serial.print("HamiltonCalibStep:"); Serial.println(HamiltonCalibStep);
        Hamilton.query.u8id = calib.sensorID + 1;
        Serial.print("Hamilton.query.u8id:"); Serial.println(Hamilton.query.u8id);
        if (calib.calibParam == 99) {
            if (state == 0) {
                if (Hamilton.factoryReset()) state = 1;
            }
            else {
                calib.calibEnCours = false;
                state = 0;
            }
        }
        else {
            HamiltonCalibStep = Hamilton.calibrate(calib.value, HamiltonCalibStep);
            if (HamiltonCalibStep == 4) {
                HamiltonCalibStep = 0;
                calib.calibEnCours = false;
            }
        }
        break;


    case 5://PODOC
        Serial.println("CALIBRATE PODOC");
        mbSensor.query.u8id = 10;
        if (calib.calibParam == 99) {
            if (state == 0) {
                if (mbSensor.factoryReset()) state = 1;
            }
            else {
                calib.calibEnCours = false;
                state = 0;
            }
        }
        else {
            int offset;
            if (state == 0) {             
                //PODOC oxy
                if (calib.calibParam == 0) {
                    offset = 516;
                }
                else {
                    offset = 522;
                }           
                if (mbSensor.calibrateCoeff(calib.value, offset)) state = 1;
            }
            else {
                offset = 654;
                if (mbSensor.validateCalibration(offset)) {
                    state = 0;
                    calib.calibEnCours = false;
                }
            }
            
        }
        break;
    case 6://NTU
        mbSensor.query.u8id = 40;
        if (calib.calibParam == 99) {
            if (state == 0) {
                if (mbSensor.factoryReset()) state = 1;
            }
            else {
                calib.calibEnCours = false;
                state = 0;
            }
        }
        else {
            int offset;
            if (state == 0) {
                if (calib.calibParam == 0) {
                    offset = 516;// Gamme 4 jusqu'a 50 NTU, sinon changer d'adresse
                }
                else {
                    offset = 518;// Gamme 4 jusqu'a 50 NTU, sinon changer d'adresse
                }
                if (mbSensor.calibrateCoeff(calib.value, offset)) state = 1;
            }
            else {
                offset = 654;
                if (mbSensor.validateCalibration(offset)) {
                    state = 0;
                    calib.calibEnCours = false;
                }
            }
            
        }
        break;
    case 7://PC4E
        mbSensor.query.u8id = 30;
        if (calib.calibParam == 99) {
            if (state == 0) {
                if (mbSensor.factoryReset()) state = 1;
            }
            else {
                calib.calibEnCours = false;
                state = 0;
            }
        }
        else {
            int offset;
            if (state == 0) {
                if (calib.calibParam == 0) {
                    offset = 528;
                }
                else {
                    offset = 530;
                }
                if (mbSensor.calibrateCoeff(calib.value, offset)) state = 1;
            }
            else {
                offset = 654;
                if (mbSensor.validateCalibration(offset)) {
                    state = 0;
                    calib.calibEnCours = false;
                }
            }
        }
        break;
    }
}



