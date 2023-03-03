#define HAVE_RTC
#define HAVE_RTC_DS1307

#include <TimeLib.h>
#include <WebSockets.h>
#include <ModbusRtu.h>
#include <PID_v1.h>
#include <EEPROMex.h>
#include <ArduinoJson.h>
#include "C:\Users\Max\Desktop\CocoriCO2\Libs/Mesocosmes.h"
#include "C:\Users\Max\Desktop\CocoriCO2\Libs/Hamilton.h"
#include "C:\Users\Max\Desktop\CocoriCO2\Libs/Condition.h"
#include "C:\Users\Max\Desktop\CocoriCO2\Libs/ModbusSensor.h"
#include <WebSocketsServer.h>
#include <Ethernet.h>

#include <SD.h>

#include <RTC.h>



//#include <IndustrialShields.h>
//#include <RS485.h>


//Pinout for mduino 42

#define PIN_DEBITMETRE_0  56
#define PIN_DEBITMETRE_1  57
#define PIN_DEBITMETRE_2  58
#define PIN_NIVEAU_L_0  2
#define PIN_NIVEAU_LL_0  24
#define PIN_NIVEAU_L_1  26
#define PIN_NIVEAU_LL_1  23
#define PIN_NIVEAU_L_2  25
#define PIN_NIVEAU_LL_2  22

#define PIN_PRESSION_EC  65
#define PIN_PRESSION_EA  64
#define PIN_NIVEAU_H_CO2  18

#define PIN_VANNE_EAU_CO2  41
#define PIN_V2V_EA  4
#define PIN_V2V_EC  5

#define PIN_VANNE_CO2  42
//#define PIN_VANNE_CO2  38
#define PIN_VANNE_EXONDATION  36
#define PIN_LED  37
#define PIN_CAPTEUR_FLUO  59

//PAC DE MEZE
#define PIN_V3V_PAC  6
#define PIN_TEMP_PAC  63


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
//ModbusSensorHamilton Hamilton;
ModbusSensor mbSensor(10);
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
tempo tempoRR;

int sensorIndex = 0;

bool pHSensor = true;

bool toggleCO2Valve = false;
//bool remplissageCO2 = false;

uint8_t AppSocketId = -1;


bool calibAuthorized = false;

enum {
    REQ_PARAMS = 0,
    REQ_DATA = 1,
    SEND_PARAMS = 2,
    SEND_DATA = 3,
    CALIBRATE_SENSOR = 4,
    REQ_MASTER_DATA = 5,
    SEND_MASTER_DATA = 6,
    REQ_MASTER_PARAMS = 7,
    SEND_MASTER_PARAMS = 8,
    SEND_PAC_PARAMS = 9,
    REQ_PAC_PARAMS = 10
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
    bool currentSun;
    double sortiePID_EA;
    double sortiePID_EC;
    double pression[2];
    double tempPAC;
    double sortiePID_TEC;
}MasterData;

MasterData masterData;

Regul regulPression[2];
Regul regulTempEC;
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
    regulTempEC.startAddress = address;
    address = regulTempEC.load(address);
}

int currentDay;
int currentMin;

extern unsigned int __bss_end;
extern unsigned int __heap_start;
extern void* __brkval;

uint16_t getFreeSram() {
    uint8_t newVariable;
    // heap is empty, use bss as start memory address
    if ((uint16_t)__brkval == 0)
        return (((uint16_t)&newVariable) - ((uint16_t)&__bss_end));
    // use heap end as the start of the memory address
    else
        return (((uint16_t)&newVariable) - ((uint16_t)__brkval));
};

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

    condition[0].Meso[0] = Mesocosme(PIN_DEBITMETRE_0, 0, PIN_NIVEAU_L_0, PIN_NIVEAU_LL_0, 0);
    condition[0].Meso[1] = Mesocosme(PIN_DEBITMETRE_1, 0, PIN_NIVEAU_L_1, PIN_NIVEAU_LL_1, 1);
    condition[0].Meso[2] = Mesocosme(PIN_DEBITMETRE_2, 0, PIN_NIVEAU_L_2, PIN_NIVEAU_LL_2, 2);
    condition[0].Meso[0].debit = 0;
    condition[0].Meso[1].debit = 0;
    condition[0].Meso[2].debit = 0;

    regulPression[0] = Regul();
    regulPression[1] = Regul();
    regulTempEC = Regul();

    load(2);

    for (uint8_t i = 0; i < 4; i++) {
        condition[i].condID = i;
        for (uint8_t j = 0; j < 3; j++) condition[i].Meso[j] = Mesocosme(j);
        condition[i].save();
        //condition[i].regulpH.consigne = 0;
        //condition[i].regulTemp.consigne = 0;
    }

    tempoSensorRead.interval = 200;//50
    tempoRegul.interval = 100;
    tempoCheckMeso.interval = 200;
    tempoSendValues.interval = 5000;
    tempowriteSD.interval = 5000 * 60;
    tempoCheckSun.interval = 60 * 1000;
    tempoSendParams.interval = 5000;
    tempoCheckWaterLevelCO2.interval = 1000;
    tempoRR.interval = 10000;

    tempoSensorRead.debut = millis() + 2000;

    //CONFIGURE LA MESURE DES CAPTEURS NTU ET CONDUCTIVITE EN GAMME 4
    mbSensor.query.u8id = 30;
    while (!mbSensor.configMesure(&master));
    mbSensor.query.u8id = 40;
    while (!mbSensor.configMesure(&master));



    Ethernet.begin(mac, ip);

    Serial.println(F("START"));
    int i = 0;
    while (i < 2) {
        if (Ethernet.hardwareStatus() != EthernetNoHardware) {
            Serial.println(F("Ethernet STARTED"));
            break;
        }
        delay(500); // do nothing, no point running without Ethernet hardware
        i++;
    }

    if (Ethernet.hardwareStatus() != EthernetNoHardware) {
        webSocket.begin();
        webSocket.onEvent(webSocketEvent);
    }



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
    currentDay = RTC.getMonthDay();
    currentMin = RTC.getMinute();

    setPIDparams();
    Serial.print("SD BEGIN : ");
    Serial.print(SD.begin(53));
    File root = SD.open("/");

    printDirectory(root, 0);
    updateSunTimes();
    Serial.println(F("End SETUP"));

    checkSun();

}

void printDirectory(File dir, int numTabs) {

    while (true) {

        File entry = dir.openNextFile();

        if (!entry) {

            // no more files

            break;

        }

        for (uint8_t i = 0; i < numTabs; i++) {

            Serial.print('\t');

        }

        Serial.print(entry.name());

        if (entry.isDirectory()) {

            Serial.println("/");

            printDirectory(entry, numTabs + 1);

        }
        else {

            // files have sizes, directories do not

            Serial.print("\t\t");

            Serial.println(entry.size(), DEC);

        }

        entry.close();

    }
}

void loop() {
    RTC.read();

    readMBSensors();

    if(!calib.calibEnCours) webSocket.loop();

    if (elapsed(&tempoRegul.debut, tempoRegul.interval)) {
        regulationpH();
        for (uint8_t i = 0; i < 2; i++) {
            masterData.pression[i] = readPressure(10, pinCapteur_pression[i], masterData.pression[i]);
            regulationPression(i);
        }
        regulationTemperaturePAC();
    }

    checkMesocosmes();
    checkWaterLevelCO2();
    printToSD();

    if (currentMin != RTC.getMinute()) {
        currentMin = RTC.getMinute();
        checkSun();
        if (currentDay != RTC.getMonthDay()) {
            currentDay = RTC.getMonthDay();
            updateSunTimes();
        }
    }
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
        //digitalWrite(PIN_VANNE_EAU_CO2, true);
        return digitalRead(PIN_NIVEAU_H_CO2);
    }
}


float readPressure(int lissage, uint8_t pin, double pression) {
    int ana = analogRead(pin); // 0-1023 value corresponding to 0-10 V corresponding to 0-20 mA
    //if using 330 ohm resistor so 20mA = 6.6V
    //int ana2 = ana * 10 / 6.6;
    int mA = map(ana, 0, 1023, 0, 2000); //map to milli amps with 2 extra digits
    int mbars = map(mA, 400, 2000, 0, 4000); //map to milli amps with 2 extra digits
    double anciennePression = pression;
    pression = ((double)mbars) / 1000.0; // pressure in bars
    pression = (lissage * pression + (100.0 - lissage) * anciennePression) / 100.0;
    return pression;
}


unsigned long dateToTimestamp(int year, int month, int day, int hour, int minute) {

    tmElements_t te;  //Time elements structure
    time_t unixTime; // a time stamp
    te.Day = day;
    te.Hour = hour;
    te.Minute = minute;
    te.Month = month;
    te.Second = 0;
    te.Year = year - 1970;
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

    //digitalWrite(PIN_LED, false);
    digitalWrite(PIN_LED, masterData.currentSun);
    return masterData.currentSun;
}


void updateSunTimes() {
    const char* path = "sun.csv";
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
            dataFile.print(RTC.getTime()); dataFile.print(sep); dataFile.print(masterData.currentSun); dataFile.print(sep); dataFile.print("0"); dataFile.print(sep); dataFile.print(masterData.oxy); dataFile.print(sep); dataFile.print(masterData.cond); dataFile.print(sep); dataFile.print(masterData.turb); dataFile.print(sep); dataFile.print(masterData.fluo); dataFile.print(sep); dataFile.print(masterData.temperature); dataFile.print(sep); dataFile.print(masterData.pH); dataFile.print(sep);

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

    regulTempEC.pid = PID((double*)&masterData.tempPAC, &regulTempEC.sortiePID, &regulTempEC.consigne, regulTempEC.Kp, regulTempEC.Ki, regulTempEC.Kd, DIRECT);
    regulTempEC.pid.SetOutputLimits(50, 255);
    regulTempEC.pid.SetMode(AUTOMATIC);
    regulTempEC.pid.SetControllerDirection(DIRECT);

    for (int i = 0; i < 2; i++) {
        regulPression[i].pid = PID((double*)&masterData.pression[i], &regulPression[i].sortiePID, &regulPression[i].consigne, regulPression[i].Kp, regulPression[i].Ki, regulPression[i].Kd, DIRECT);
        regulPression[i].pid.SetOutputLimits(80, 255);
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
        //Serial.print("pression:"); Serial.print(ID); Serial.print("="); Serial.println(regulPression[ID].sortiePID_pc);
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
        }
        else  condition[0].regulpH.pid.Compute();

        condition[0].regulpH.sortiePID_pc = (int)condition[0].regulpH.sortiePID;

        dutyCycle = condition[0].regulpH.sortiePID;
    }
    unsigned long cycleDuration = 10000;
    unsigned long  onTime = dutyCycle * cycleDuration / 100;
    unsigned long  offTime = cycleDuration - onTime;
    if (onTime == 0) toggleCO2Valve = false;
    else if (offTime == 0) toggleCO2Valve = true;
    else if (toggleCO2Valve) {
        if (elapsed(&tempoCO2ValvePWM_on.debut, onTime)) {
            tempoCO2ValvePWM_off.debut = millis();
            toggleCO2Valve = false;
        }
    }
    else {
        if (elapsed(&tempoCO2ValvePWM_off.debut, offTime)) {
            tempoCO2ValvePWM_on.debut = millis();
            toggleCO2Valve = true;
        }
    }
    digitalWrite(PIN_VANNE_CO2, toggleCO2Valve);
    return dutyCycle;
}

double readTemp(int lissage, uint8_t pin, double temp) {
    int ana = analogRead(pin); // 0-1023 value corresponding to 0-10 V corresponding to 0-20 mA
    //actually, using 330 ohm resistor so 20mA = 6.6V
    //int ana2 = ana * 10 / 6.6;
    int mA = map(ana, 0, 1023, 0, 2000); //map to milli amps with 2 extra digits
    int t = map(mA, 400, 2000, 0, 5000); //map to milli amps with 2 extra digits
    double ancienneTemp = temp;
    temp = ((double)t) / 100.0; // pressure in bars
    temp = (lissage * temp + (100.0 - lissage) * ancienneTemp) / 100.0;
    return temp;
}


int regulationTemperaturePAC() {

    masterData.tempPAC = readTemp(10, PIN_TEMP_PAC, masterData.tempPAC);

    if (regulTempEC.autorisationForcage) {
        if (regulTempEC.consigneForcage > 0 && regulTempEC.consigneForcage <= 100) {
            analogWrite(PIN_V3V_PAC, (int)(regulTempEC.consigneForcage * 255 / 100));
        }
        else {
            analogWrite(PIN_V3V_PAC, 0);
        }
        regulTempEC.sortiePID_pc = (int)map(regulTempEC.consigneForcage, 0, 255, 0, 100);
        return regulTempEC.consigneForcage;
    }
    else {

        if (elapsed(&tempoRR.debut, tempoRR.interval)) {
            double diff = lastTemp - masterData.tempPAC;
            double err = regulTempEC.consigne - masterData.tempPAC;

            double adjust = (regulTempEC.Kd * diff + regulTempEC.Ki * err);

            meanPIDOut_temp += adjust;
            if (meanPIDOut_temp > 255) meanPIDOut_temp = 255;
            if (meanPIDOut_temp < 0) meanPIDOut_temp = 0.0;

            lastTemp = masterData.tempPAC;

        }
        regulTempEC.sortiePID_pc = (int)map(meanPIDOut_temp, 50, 255, 0, 100);
        if (regulTempEC.sortiePID_pc < 0) regulTempEC.sortiePID_pc = 0;
        analogWrite(PIN_V3V_PAC, meanPIDOut_temp);

        return meanPIDOut_temp;

        /*if (elapsed(&tempoRR.debut, tempoRR.interval)) {
            double diff = lastTemp - masterData.tempPAC;
            double err = regulTempEC.consigne - masterData.tempPAC;

            int adjust = (int)(regulTempEC.Kd * diff + regulTempEC.Ki * err);
            if (adjust > 2) adjust = 2;
            if (adjust <- 2) adjust = -2;


            meanPIDOut_temp += adjust;

            if (meanPIDOut_temp > 255) meanPIDOut_temp = 255;
            if (meanPIDOut_temp < 0) meanPIDOut_temp = 0;

            //meanPIDOut_temp = map(meanPIDOut_temp, 0, 255, 100, 255);

            lastTemp = masterData.tempPAC;
        }
        regulTempEC.sortiePID_pc = (int)map(meanPIDOut_temp, 50, 255, 0, 100);
        if (regulTempEC.sortiePID_pc < 0) regulTempEC.sortiePID_pc = 0;
        analogWrite(PIN_V3V_PAC, meanPIDOut_temp);*/
    }

    

    return meanPIDOut_temp;

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
        readJSON((char*)payload, num);

        break;
    case WStype_ERROR:
        Serial.print(num); Serial.println(F(" ERROR!"));
        break;
    }
}

void readJSON(char* json, uint8_t num) {
    StaticJsonDocument<jsonDocSize_data> doc;
    char buffer[bufferSize];

    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        //return;
    }

    uint8_t command = doc[cmd];
    uint8_t condID = doc[cID];
    uint8_t senderID = doc[sID];

    uint32_t heure; 
   if (senderID == 4) {
      
        heure = doc[time];
        if (heure > 0) {
            RTC.setTime(heure);
            RTC.write();
        }
    }
    RTC.read();
    heure = RTC.getTime();

    switch (command) {
    case REQ_PARAMS:
        condition[condID].load();
        if (condID > 0) {
            double value = condition[condID].regulpH.offset + masterData.pH;
            if (value > 4.0 && value < 10.0) condition[condID].regulpH.consigne = value;

            value = condition[condID].regulTemp.offset + masterData.temperature;
            if (value > 0.0 && value < 50.0) condition[condID].regulTemp.consigne = value;
        }
        condition[condID].serializeParams(heure, 0, buffer);
        webSocket.sendTXT(num, buffer);
        break;
    case REQ_DATA:
        if (condID > 0) {
            condition[condID].regulpH.consigne = condition[condID].regulpH.offset + masterData.pH;
            condition[condID].regulTemp.consigne = condition[condID].regulTemp.offset + masterData.temperature;
        }

        condition[condID].serializeData(heure, 0, buffer);
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
            condition[condID].serializeParams(heure, 0, buffer);
            webSocket.sendTXT(num, buffer);

        }
        if (condID == 0) setPIDparams();


        break;
    case REQ_MASTER_PARAMS:
        Serial.println(F("REQ MASTER PARAMS"));
        SerializeMasterParams(heure, buffer);
        webSocket.sendTXT(num, buffer);
        break;
    case SEND_MASTER_PARAMS:
        deserializeMasterParams(doc);
        for (int i = 0; i < 2; i++) {
            regulPression[i].save(regulPression[i].startAddress);
        }
        setPIDparams();
        SerializeMasterParams(heure, buffer);
        webSocket.sendTXT(num, buffer);
        break;
    case SEND_PAC_PARAMS:
        deserializePACParams(doc);
        regulTempEC.save(regulTempEC.startAddress);
        setPIDparams();
        SerializePACParams(heure, buffer);
        webSocket.sendTXT(num, buffer);
        break;
    case REQ_PAC_PARAMS:
        SerializePACParams(heure, buffer);
        webSocket.sendTXT(num, buffer);
        break;
    case SEND_DATA:
        condition[condID].load();
        condition[condID].deserializeData(doc);
        if (condID > 0) {
            condition[condID].regulpH.consigne = condition[condID].regulpH.offset + masterData.pH;
            condition[condID].regulTemp.consigne = condition[condID].regulTemp.offset + masterData.temperature;
        }
        condition[condID].serializeParams(heure, 0, buffer);
        webSocket.sendTXT(num, buffer);
        //webSocket.sendTXT(num, json);
        break;
    case REQ_MASTER_DATA:
        SerializeMasterData(heure, buffer);
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
            String msg = "{cmd:4,cID:";
            msg += String(condID) + ",sID:4,";
            msg += "sensorID:" + String(calib.sensorID);
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

void deserializeMasterParams(StaticJsonDocument<300> doc) {

    JsonObject regulPression0 = doc[F("rPressionEA")];
    regulPression[0].consigne = regulPression0[F("cons")]; // 24.2
    regulPression[0].Kp = regulPression0[F("Kp")]; // 2.1
    regulPression[0].Ki = regulPression0[F("Ki")]; // 2.1
    regulPression[0].Kd = regulPression0[F("Kd")]; // 2.1
    const char* regul0_autorisationForcage = regulPression0[F("aForcage")];
    if (strcmp(regul0_autorisationForcage, "true") == 0 || strcmp(regul0_autorisationForcage, "True") == 0) regulPression[0].autorisationForcage = true;
    else regulPression[0].autorisationForcage = false;
    regulPression[0].consigneForcage = regulPression0[F("consForcage")]; // 2.1
    regulPression[0].offset = regulPression0[F("offset")];

    JsonObject regulPression1 = doc[F("rPressionEC")];
    regulPression[1].consigne = regulPression1[F("cons")]; // 24.2
    regulPression[1].Kp = regulPression1[F("Kp")]; // 2.1
    regulPression[1].Ki = regulPression1[F("Ki")]; // 2.1
    regulPression[1].Kd = regulPression1[F("Kd")]; // 2.1
    const char* regul1_autorisationForcage = regulPression1[F("aForcage")];
    if (strcmp(regul1_autorisationForcage, "true") == 0 || strcmp(regul1_autorisationForcage, "True") == 0) regulPression[1].autorisationForcage = true;
    else regulPression[1].autorisationForcage = false;
    regulPression[1].consigneForcage = regulPression1[F("consForcage")]; // 2.1
    regulPression[1].offset = regulPression1[F("offset")];


}


void deserializePACParams(StaticJsonDocument<300> doc) {

    Serial.println("DESERIALIZE PAC PARAMS");
    JsonObject regulTemp = doc[F("rTempEC")];
    regulTempEC.consigne = regulTemp[F("cons")]; // 24.2
    regulTempEC.Kp = regulTemp[F("Kp")]; // 2.1
    regulTempEC.Ki = regulTemp[F("Ki")]; // 2.1
    regulTempEC.Kd = regulTemp[F("Kd")]; // 2.1
    const char* regulTemp_autorisationForcage = regulTemp[F("aForcage")];
    if (strcmp(regulTemp_autorisationForcage, "true") == 0 || strcmp(regulTemp_autorisationForcage, "True") == 0) regulTempEC.autorisationForcage = true;
    else regulTempEC.autorisationForcage = false;
    regulTempEC.consigneForcage = regulTemp[F("consForcage")]; // 2.1
    regulTempEC.offset = regulTemp[F("offset")];

    Serial.print("regulTempEC.consigne:"); Serial.println(regulTempEC.consigne);

}

bool SerializeMasterParams(uint32_t timeString, char* buffer) {
    StaticJsonDocument<300> doc;

    doc[cmd] = SEND_MASTER_PARAMS;
    doc[cID] = 0;
    doc[sID] = 0;
    doc[time] = timeString;

    JsonObject regulT = doc.createNestedObject(rPressionEA);
    regulT[cons] = regulPression[0].consigne;
    regulT[Kp] = regulPression[0].Kp;
    regulT[Ki] = regulPression[0].Ki;
    regulT[Kd] = regulPression[0].Kd;
    if (regulPression[0].autorisationForcage) regulT[aForcage] = F("true");
    else regulT[aForcage] = F("false");
    regulT[consForcage] = regulPression[0].consigneForcage;
    regulT[offset] = regulPression[0].offset;

    JsonObject regulp = doc.createNestedObject(rPressionEC);
    regulp[cons] = regulPression[1].consigne;
    regulp[Kp] = regulPression[1].Kp;
    regulp[Ki] = regulPression[1].Ki;
    regulp[Kd] = regulPression[1].Kd;
    if (regulPression[1].autorisationForcage) regulp[aForcage] = F("true");
    else regulp[aForcage] = F("false");
    regulp[consForcage] = regulPression[1].consigneForcage;
    regulp[offset] = regulPression[1].offset;

   
    serializeJson(doc, buffer, 300);
    return true;
}

bool SerializePACParams(uint32_t timeString, char* buffer) {
    StaticJsonDocument<300> doc;

    doc[cmd] = SEND_PAC_PARAMS;
    doc[cID] = 0;
    doc[sID] = 0;
    doc[time] = timeString;


    Serial.print("SERIALIZE PAC PARAMS"); 
    Serial.print("regulTempEC.consigne:"); Serial.println(regulTempEC.consigne);

    JsonObject regulT = doc.createNestedObject("rTempEC");
    regulT[cons] = regulTempEC.consigne;
    regulT[Kp] = regulTempEC.Kp;
    regulT[Ki] = regulTempEC.Ki;
    regulT[Kd] = regulTempEC.Kd;
    if (regulTempEC.autorisationForcage) regulT[aForcage] = F("true");
    else regulT[aForcage] = F("false");
    regulT[consForcage] = regulTempEC.consigneForcage;
    regulT[offset] = regulTempEC.offset;
    serializeJson(doc, buffer, 300);
    return true;
}

double readFluo() {
    //Serial.print("READ FLUO");
    
    int value = analogRead(PIN_CAPTEUR_FLUO);  // pin 0-10V

    long Fluo_mV = map(value, 0, 1024, 0, 1000); //passage en 0-10V
    long Fluo = map(Fluo_mV, 0, 500, 0, 50000); // passage en micro gramme /L (max = 5 micro G)
    double  F = Fluo / 1000.0; //micro gremme / litre sans le gain de 100;
    masterData.fluo = F;

    /*Serial.print("value:"); Serial.println(value);
    Serial.print("mV:"); Serial.println(Fluo_mV);
    Serial.print("Fluo:"); Serial.println(Fluo);
    Serial.print("F:"); Serial.println(F);
    */
    return F;
}

bool SerializeMasterData(uint32_t timeString, char* buffer) {

    StaticJsonDocument<300> doc;
    doc[cmd] = SEND_MASTER_DATA;
    doc[cID] = 0;
    doc[sID] = 0;
    doc[time] = timeString;

    if (masterData.currentSun) doc[sun] = F("true");
    else doc[sun] = F("false");
    doc[oxy] = masterData.oxy;
    doc[cond] = masterData.cond;
    doc[turb] = masterData.turb;
    doc[fluo] = masterData.fluo;
    doc[pH] = masterData.pH;
    doc[sal] = masterData.salinite;
    doc[temp] = masterData.temperature;
    doc[pressionEA] = masterData.pression[0];
    doc[pressionEC] = masterData.pression[1];
    doc[sPID_EA] = regulPression[0].sortiePID_pc;
    doc[sPID_EC] = regulPression[1].sortiePID_pc;
    doc[sPID_TEC] = regulTempEC.sortiePID_pc;
    doc[F("tempPAC")] = masterData.tempPAC;

    doc[nextSunUp] = masterData.nextSunUp;
    doc[nextSunDown] = masterData.nextSunDown;

    serializeJson(doc, buffer, bufferSize);
    return true;
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
    int errorCode = 0;
    //sensorIndex = 0;
    if (elapsed(&tempoSensorRead.debut, tempoSensorRead.interval)) {

        readFluo();
        if (state == 0 && calib.calibRequested && calibAuthorized) {
            calib.calibRequested = false;
            calib.calibEnCours = true;
            mbSensor.querySent = false;
        }
        if (calib.calibEnCours) {
            calibrateSensor();
        }
        else {
            calibAuthorized = false;
            if (sensorIndex < 5) { // HAMILTON: indexes O to 2 are mesocosms, index 4 is input measure tank, index 3 is acidification tank
                //Hamilton.setSensor(sensorIndex + 1, &master);
                mbSensor.query.u8id = sensorIndex + 1;
                if (pHSensor) {

                    if (mbSensor.readPH(&master)) {

                        Serial.print(F("pH:")); Serial.println(mbSensor.pH_sensorValue);
                        if (mbSensor.pH_sensorValue > 0) {
                            if (sensorIndex < 3) condition[0].Meso[sensorIndex].pH = mbSensor.pH_sensorValue;
                            if (sensorIndex == 3) condition[0].mesurepH = mbSensor.pH_sensorValue;
                            if (sensorIndex == 4) masterData.pH = mbSensor.pH_sensorValue;

                        }
                        mbSensor.pH_sensorValue = -99;
                        pHSensor = false;
                    }
                }
                /*if (pHSensor) {

                    if (mbSensor.readErrors(&master)) {

                        Serial.print(F("pH:")); Serial.println(mbSensor.pH_sensorValue);
                        
                        mbSensor.pH_sensorValue = -99;
                        pHSensor = false;
                    }
                }*/
                else {
                    if (mbSensor.readTemp(&master)) {
                        Serial.print(F("temp:")); Serial.println(mbSensor.temp_sensorValue);
                        if (mbSensor.temp_sensorValue > 0) {
                            if (sensorIndex < 3) condition[0].Meso[sensorIndex].temperature = mbSensor.temp_sensorValue;
                            if (sensorIndex == 3) condition[0].mesureTemperature = mbSensor.temp_sensorValue;
                            if (sensorIndex == 4) {
                                masterData.temperature = mbSensor.temp_sensorValue;
                            }

                        }
                        mbSensor.temp_sensorValue = -99;
                        sensorIndex++;
                        pHSensor = true;
                    }
                }
            }
            else {

                calibAuthorized = false;
                switch (sensorIndex) {
                case 5:
                    mbSensor.query.u8id = 10;//PODOC
                    if (state == 0) {
                        errorCode = mbSensor.requestValues(&master);
                        if (errorCode == 1) {
                            state = 1;
                        }
                        else if(errorCode == -1) {
                            state = 0;
                            sensorIndex++;
                        }
                    }
                    else{
                        errorCode = mbSensor.readValues(&master);
                        if (errorCode == 1) {
                            Serial.print(F("Temperature:")); Serial.println(mbSensor.params[0]);
                            Serial.print(F("oxy %:")); Serial.println(mbSensor.params[1]);
                            Serial.print(F("oxy mg/L:")); Serial.println(mbSensor.params[2]);
                            Serial.print(F("oxy ppm:")); Serial.println(mbSensor.params[3]);
                            masterData.oxy = mbSensor.params[1];
                            state = 0;
                            sensorIndex++;
                            calibAuthorized = true;
                        }
                        else if (errorCode == 255){
                            state = 0;
                            sensorIndex++;
                            calibAuthorized = true;
                        }
                    }
                    break;
                case 6:
                    mbSensor.query.u8id = 40;//NTU
                    if (state == 0) {
                        errorCode = mbSensor.requestValues(&master);
                        if (errorCode == 1) {
                            state = 1;
                        }
                        else if (errorCode == 255) {
                            state = 0;
                            sensorIndex++;
                        }
                    }
                    else {
                        errorCode = mbSensor.readValues(&master);
                        if (errorCode == 1) {
                            Serial.print(F("Temperature:")); Serial.println(mbSensor.params[0]);
                            Serial.print(F("NTU:")); Serial.println(mbSensor.params[1]);
                            Serial.print(F("FNU:")); Serial.println(mbSensor.params[2]);
                            Serial.print(F("mg/L:")); Serial.println(mbSensor.params[3]);
                            masterData.turb = mbSensor.params[1];
                            state = 0;
                            sensorIndex++;
                            calibAuthorized = true;
                        }
                        else if (errorCode == 255) {
                            state = 0;
                            sensorIndex++;
                            calibAuthorized = true;
                        }
                    }
                    break;
                case 7:
                    mbSensor.query.u8id = 30;//Cond
                    if (state == 0) {
                        errorCode = mbSensor.requestValues(&master);
                        if (errorCode == 1) {
                            state = 1;
                        }
                        else if (errorCode == 255) {
                            state = 0;
                            sensorIndex = 0;
                        }
                    }
                    else {
                        errorCode = mbSensor.readValues(&master);
                        if (errorCode == 1) {
                            Serial.print(F("Temperature:")); Serial.println(mbSensor.params[0]);
                            Serial.print(F("conductivite:")); Serial.println(mbSensor.params[1]);
                            Serial.print(F("salinite:")); Serial.println(mbSensor.params[2]);
                            Serial.print(F("TDS:")); Serial.println(mbSensor.params[3]);
                            masterData.cond = mbSensor.params[1];
                            masterData.salinite = mbSensor.params[2];
                            state = 0;
                            sensorIndex=0;
                            calibAuthorized = true;
                        }
                        else if (errorCode == 255) {
                            state = 0;
                            sensorIndex = 0;
                            calibAuthorized = true;
                        }
                    }
                    break;
                case 8:
                    //TODO: FLUO
                    break;
                }

            }
        }
    }
}

int HamiltonCalibStep = 0;

int stateCalib = 0;

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
        mbSensor.query.u8id = calib.sensorID + 1;
        Serial.print("Hamilton.query.u8id:"); Serial.println(mbSensor.query.u8id);
        if (calib.calibParam == 99) {
            if (stateCalib == 0) {
                if (mbSensor.factoryReset(&master)) stateCalib = 1;
            }
            else {
                calib.calibEnCours = false;
                stateCalib = 0;
            }
        }
        else {
            HamiltonCalibStep = mbSensor.calibrate(calib.value, HamiltonCalibStep, &master);
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
            if (stateCalib == 0) {
                if (mbSensor.factoryReset(&master)) stateCalib = 1;
            }
            else {
                calib.calibEnCours = false;
                stateCalib = 0;
            }
        }
        else {
            int offset;
            if (stateCalib == 0) {
                //PODOC oxy
                if (calib.calibParam == 0) {
                    offset = 516;
                }
                else {
                    offset = 522;
                }
                if (mbSensor.calibrateCoeff(calib.value, offset, &master)) stateCalib = 1;
            }
            else {
                offset = 654;
                if (mbSensor.validateCalibration(offset, &master)) {
                    stateCalib = 0;
                    calib.calibEnCours = false;
                }
            }

        }
        break;
    case 6://NTU
        mbSensor.query.u8id = 40;
        if (calib.calibParam == 99) {
            if (stateCalib == 0) {
                if (mbSensor.factoryReset(&master)) stateCalib = 1;
            }
            else {
                calib.calibEnCours = false;
                stateCalib = 0;
            }
        }
        else {
            int offset;
            if (stateCalib == 0) {
                if (calib.calibParam == 0) {
                    offset = 528;// Gamme 4 jusqu'a 4000 NTU, sinon changer d'adresse
                }
                else {
                    offset = 530;// Gamme 4 jusqu'a 4000 NTU, sinon changer d'adresse
                }
                if (mbSensor.calibrateCoeff(calib.value, offset, &master)) stateCalib = 1;
            }
            else {
                offset = 654;
                if (mbSensor.validateCalibration(offset, &master)) {
                    stateCalib = 0;
                    calib.calibEnCours = false;
                }
            }

        }
        break;
    case 7://PC4E
        mbSensor.query.u8id = 30;
        if (calib.calibParam == 99) {
            if (stateCalib == 0) {
                if (mbSensor.factoryReset(&master)) stateCalib = 1;
            }
            else {
                calib.calibEnCours = false;
                stateCalib = 0;
            }
        }
        else {
            int offset;
            if (stateCalib == 0) {

                if (calib.calibParam == 0) {
                    offset = 528;
                }
                else {
                    offset = 530; //518
                }
                    if (mbSensor.calibrateCoeff(calib.value, offset, &master)) 
                    stateCalib = 1;
            }
            else {
                offset = 654;//654
                if (mbSensor.validateCalibration(offset, &master)) {
                    stateCalib = 0;
                    calib.calibEnCours = false;
                }
            }
        }
        break;
    }
}


