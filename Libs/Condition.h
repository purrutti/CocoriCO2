#ifndef MESO
#include "C:\Users\pierr\OneDrive\Documents\Arduino\libraries\cocorico2\Mesocosmes.h"
#endif // !MESO
#include <EEPROMex.h>

#include <PID_v1.h>

const size_t jsonDocSize_params = 300;
const size_t jsonDocSize_data = 512;

const int bufferSize = 500;

bool customRegul = true;
double regulFilter = 0.01;

double lastTemp = 18;

double meanPIDOut_temp = 120;
int meanPIDOut_pH = 0;



const char* cmd = "cmd";
const char* cID = "cID";
const char* sID = "sID";
const char* time = "time";

const char* rPressionEA = "rPressionEA";
const char* cons = "cons";
const char* Kp = "Kp";
const char* Ki = "Ki";
const char* Kd = "Kd";
const char* aForcage = "aForcage";
const char* consForcage = "consForcage";
const char* offset = "offset";
const char* rPressionEC = "rPressionEC";
const char* rTempEC = "rTempEC";

const char* sun = "sun";
const char* oxy = "oxy";
const char* cond = "cond";
const char* turb = "turb";
const char* fluo = "fluo";
const char* pH = "pH";
const char* sal = "sal";
const char* temp = "temp";
const char* pressionEA = "pressionEA";
const char* pressionEC = "pressionEC";
const char* sPID_EA = "sPID_EA";
const char* sPID_EC = "sPID_EC";
const char* sPID_TEC = "sPID_TEC";
const char* nextSunUp = "nextSunUp";
const char* nextSunDown = "nextSunDown";

const char* sPID_pc = "sPID_pc";
const char* sMesoID = "MesoID";
const char* debit = "debit";
const char* LevelH = "LevelH";
const char* LevelL = "LevelL";
const char* LevelLL = "LevelLL";

const char* rTemp = "rTemp";
const char* rpH = "rpH";
const char* sdata = "data";




class Regul {
public:

    double sortiePID;
    double consigne;
    double Kp;
    double Ki;
    double Kd;
    double sortiePID_pc;
    bool autorisationForcage;
    int consigneForcage;
    double offset;
    PID pid;
    int startAddress;

    int save(int startAddress) {
        int add = startAddress;
        EEPROM.updateDouble(add, consigne); add += sizeof(double);
        EEPROM.updateDouble(add, Kp); add += sizeof(double);
        EEPROM.updateDouble(add, Ki); add += sizeof(double);
        EEPROM.updateDouble(add, Kd); add += sizeof(double);
        EEPROM.updateDouble(add, offset); add += sizeof(double);

        EEPROM.updateInt(add, autorisationForcage); add += sizeof(int);
        EEPROM.updateInt(add, consigneForcage); add += sizeof(int);
        return add;
    }

    int load(int startAddress) {
        int add = startAddress;
        consigne = EEPROM.readDouble(add); add += sizeof(double);
        Kp = EEPROM.readDouble(add); add += sizeof(double);
        Ki = EEPROM.readDouble(add); add += sizeof(double);
        Kd = EEPROM.readDouble(add); add += sizeof(double);
        offset = EEPROM.readDouble(add); add += sizeof(double);

        autorisationForcage = EEPROM.readInt(add); add += sizeof(int);
        consigneForcage = EEPROM.readInt(add); add += sizeof(int);
        return add;
    }
};


class Condition {
public:
    //uint8_t socketID;
    uint8_t condID;
    Mesocosme Meso[3];
    double mesureTemperature;
    double mesurepH;

    Regul regulTemp, regulpH;

    int startAddress;

    Condition() {
        regulTemp = Regul();
        regulpH = Regul();
    }

    int save() {
        int add = startAddress;
        add += sizeof(int);
        add = regulpH.save(add);
        add = regulTemp.save(add);
        return add;
    }

    int load() {
        int add = startAddress;
        add += sizeof(int);
        add = regulpH.load(add);
        add = regulTemp.load(add);
        return add;
    }


    bool serializeData(uint32_t timeString, uint8_t sender, char* buffer) {
        //Serial.println("SENDDATA");
        //DynamicJsonDocument doc(512);

        StaticJsonDocument<512> doc;

        doc[cmd] = 3;
        doc[cID] = condID;
        doc[sID] = sender;
        doc[temp] = mesureTemperature;
        doc[pH] = mesurepH;


        //Serial.print(F("CONDID:")); Serial.println(condID);
        //Serial.print(F("socketID:")); Serial.println(socketID);
        doc[time] = timeString;

        JsonArray data = doc.createNestedArray(sdata);
        JsonObject dataArray[3];

        JsonObject regulT = doc.createNestedObject(rTemp);
        regulT[cons] = regulTemp.consigne;
        regulT[sPID_pc] = regulTemp.sortiePID_pc;

        JsonObject regulp = doc.createNestedObject(rpH);
        regulp[cons] = regulpH.consigne;
        regulp[sPID_pc] = regulpH.sortiePID_pc;

        for (int i = 0; i < 3; i++) {
            dataArray[i] = data.createNestedObject();
            dataArray[i][sMesoID] = Meso[i]._mesocosmeIndex;
            dataArray[i][temp] = Meso[i].temperature;
            dataArray[i][pH] = Meso[i].pH;

            dataArray[i][debit] = Meso[i].debit;
            dataArray[i][LevelH] = Meso[i].alarmeNiveauHaut;
            dataArray[i][LevelL] = Meso[i].alarmeNiveauBas;
            dataArray[i][LevelLL] = Meso[i].alarmeNiveauTresBas;
        }
        
        serializeJson(doc, buffer, bufferSize);
        Serial.print(F("SEND DATA:"));
        Serial.println(buffer);
        return true;
    }

    bool serializeParams(uint32_t timeString, uint8_t sender, char* buffer) {
        StaticJsonDocument<300> doc;
        //Serial.println(F("SEND PARAMS"));

        doc[cmd] = 2;
        doc[cID] = condID;
        doc[sID] = sender;
        doc[time] = timeString;
        /*doc["mesureTemp"] = Hamilton[3].temp_sensorValue;
        doc["mesurepH"] = Hamilton[3].pH_sensorValue;*/

        JsonObject regulT = doc.createNestedObject(F("rTemp"));
        regulT[cons] = regulTemp.consigne;
        regulT[Kp] = regulTemp.Kp;
        regulT[Ki] = regulTemp.Ki;
        regulT[Kd] = regulTemp.Kd;
        if (this->regulTemp.autorisationForcage) regulT[aForcage] = "true";
        else regulT[aForcage] = "false";
        regulT[consForcage] = regulTemp.consigneForcage;
        regulT[offset] = regulTemp.offset;

        JsonObject regulp = doc.createNestedObject(F("rpH"));
        regulp[cons] = regulpH.consigne;
        regulp[Kp] = regulpH.Kp;
        regulp[Ki] = regulpH.Ki;
        regulp[Kd] = regulpH.Kd;
        if (regulpH.autorisationForcage) regulp[aForcage] = "true";
        else regulp[aForcage] = "false";
        regulp[consForcage] = regulpH.consigneForcage;
        regulp[offset] = regulpH.offset;
        serializeJson(doc, buffer, bufferSize);
    }

    void deserializeParams(StaticJsonDocument<jsonDocSize_params> doc) {

        JsonObject regulp = doc[rpH];
        regulpH.consigne = regulp[cons]; // 24.2
        regulpH.Kp = regulp[Kp]; // 2.1
        regulpH.Ki = regulp[Ki]; // 2.1
        regulpH.Kd = regulp[Kd]; // 2.1
        const char* regulpH_autorisationForcage = regulp[aForcage];
        if (strcmp(regulpH_autorisationForcage, "true") == 0 || strcmp(regulpH_autorisationForcage, "True") == 0) regulpH.autorisationForcage = true;
        else regulpH.autorisationForcage = false;
        regulpH.consigneForcage = regulp[consForcage]; // 2.1
        regulpH.offset = regulp[offset];

        JsonObject regulT = doc[rTemp];

        regulTemp.consigne = regulT[cons]; // 24.2
        regulTemp.Kp = regulT[Kp]; // 2.1
        regulTemp.Ki = regulT[Ki]; // 2.1
        regulTemp.Kd = regulT[Kd]; // 2.1
        const char* regulTemp_autorisationForcage = regulT[aForcage];
        if (strcmp(regulTemp_autorisationForcage, "true") == 0 || strcmp(regulTemp_autorisationForcage, "True") == 0) regulTemp.autorisationForcage = true;
        else regulTemp.autorisationForcage = false;
        regulTemp.consigneForcage = regulT[consForcage]; // 2.1
        regulTemp.offset = regulT[offset];

    }

    void deserializeData(StaticJsonDocument<jsonDocSize_data> doc) {


        for (JsonObject elem : doc[sdata].as<JsonArray>()) {

            int MesoID = elem[sMesoID]; // 0, 2, 3
            Meso[MesoID] = MesoID;
            Meso[MesoID].temperature = elem[temp]; // 0, 0, 0
            Meso[MesoID].pH = elem[pH]; // 0, 0, 0
            Meso[MesoID].debit = elem[debit]; // 0, 0, 0
            Meso[MesoID].alarmeNiveauHaut = elem[LevelH]; // 0, 0, 0
            Meso[MesoID].alarmeNiveauBas = elem[LevelL]; // 0, 0, 0
            Meso[MesoID].alarmeNiveauTresBas = elem[LevelLL]; // 0, 0, 0
        }
        regulTemp.consigne = doc[rTemp][cons]; // 1
        regulTemp.sortiePID_pc = doc[rTemp][sPID_pc]; // 2

        regulpH.consigne = doc[rpH][cons]; // 3
        regulpH.sortiePID_pc = doc[rpH][sPID_pc]; // 4

        mesureTemperature = doc[temp];
        mesurepH = doc[pH];


    }


};
