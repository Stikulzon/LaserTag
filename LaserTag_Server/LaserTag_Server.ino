// Радіо модуль
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include "mString.h"
#include "Parser.h"
#include <SoftwareSerial.h>
// Піни радіо модуля
int SS1 = 10; int CE1 = 9; // RX, TX
// Піни 11, 12, 13 зайняті автоматично
RF24 radio(CE1, SS1); // "Создать" модуль на пинах SS_1 и CE_1

SoftwareSerial mySerial(6, 7); // указываем пины rx и tx соответственно

byte addresses[][6] = {"1Node","2Node","3Node","4Node","5Node","6Node"};              // Radio pipe addresses for the 2 nodes to communicate.
byte RecivedCounter [3];
byte playerCount = 3;
byte gameID = 1;
byte playersID[8];

typedef struct {
  byte packageID;
  byte Status;
  byte ValidID = 111;
} ReciveData2;
ReciveData2 reciveData2;

struct ReciveData1 {
  byte teamID;
  byte myID;
  byte playerID;
  byte ValidID;
};
ReciveData1 reciveData;

typedef struct {
  byte packageID;
  byte gameID;
  byte playerCount;
  byte ValidID = 111;
} ConfigData;
ConfigData sendInformation;

typedef struct {
  byte packageID;
  byte MyID;
  byte ValidID = 111;
} myStruct2;
myStruct2 sendInformation2;

//struct Nicknames {
  mString<16> Nickname1[8];
//};
//Nicknames Nickname1[3];

struct myStruct3 {
  byte packageID;
  byte enemyTeamScore;
  byte myTeamScore;
  byte enemyBases;
  byte myBases;
  bool kill;
  byte killerID;
  byte killedID;
  byte ValidID = 111;
};
myStruct3 sendInformation3;

struct myStruct4 {
  byte packageID;
  bool kill;
  byte killerID;
  byte killedID;
  byte ValidID = 111;
};
myStruct4 sendInformation4;

unsigned long time1, time2; // Record the current microsecond count   

bool configIsStart, gameIsStart;
int packageID = 1, pakageID1 = 1;
bool mode = 0;

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(5);
  printf_begin();

//  for(int i = 0; i<deviceCount; i++){
//    digitalWrite(SS1[i], HIGH); // turn off radio2
//  }
//  if(deviceCount>1){
  radio.begin();
  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.enableAckPayload();               // Allow optional ack payloads
  radio.setRetries(0,15);                 // Smallest time between retries, max no. of retries
  radio.setPayloadSize(32);                // Here we are sending 1-byte payloads to test the call-response speed
//  radio.openWritingPipe(addresses[1]);        // Both radios listen on the same pipes by default, and switch when writing
  radio.openReadingPipe(1,addresses[0]);      // Open a reading pipe on address 0, pipe 1
  radio.setChannel(0x60);  //выбираем канал (в котором нет шумов!)
  radio.startListening();                 // Start listening
  radio.setPALevel (RF24_PA_LOW); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_1MBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  radio.powerUp();
  radio.printDetails();                   // Dump the configuration of the rf unit for debugging
//  }
  
      Nickname1[0] = "Start";
      Nickname1[1] = "Masa";
      Nickname1[2] = "Sasha";
      Nickname1[3] = "Dasha";
}

void loop() {
  if (millis() - time2 > 1000){ // Вместо 10000 подставьте нужное вам значение паузы 
    if (configIsStart == true || gameIsStart == true){
      time2 = millis();
      resp();
    }
  }
  req();
  if(Serial.available()){
    parseString();
  }
}

void parseString(){
  char str[128];
//  char* strings[10];
//  mString<128> inputData;
  int amount = Serial.readBytesUntil(';', str, 128);
  str[amount] = NULL;
  
  Parser data(str, ',');
  int am = data.split();
  int sw = atoi(data[0]);
  switch(sw){
    case 1:
    playerCount = atoi(data[2]);
      for (int i = 1; i < am; i++) {
//        Serial.println(data[i]);
      }
    break;
    case 2:
      for (int i = 1; i < am; i++) {
        gameID = atoi(data[1]);
        Nickname1[i] = data[i+1];
//        Serial.println(data[i]);
        Serial.println("Нікнейми записані упішно.");
      }
    break;
    case 5:
      configIsStart = true;
      packageID = 1;
    break;
    case 6:
      gameIsStart = true;
      packageID = 3;
      Serial.println("Гра починається!");
    break;
    case 7:
      configIsStart = true;
      gameIsStart = true;
      packageID = 1;
    break;
    case 9:
      configIsStart = false;
      gameIsStart = false;
      packageID = 1;
    break;
    case 0:
      Serial.println("Данні записуються у наступному вигляді:");
      Serial.println("Індіфкатор,data,data,data...;");
      Serial.println("Індіфкатори команд:");
      Serial.println("0 - Допомога");
      Serial.println("1 - Налаштування гри (індіфкатор,palyerCount)");
      Serial.println("2 - Нікнейми (індіфкатор,gameID,нікнейм1,нікнейм2,нікнейм3...)");
      Serial.println("5 - Розпочати налаштування");
      Serial.println("6 - Розпочати гру");
      Serial.println("7 - Автоматичне налаштування і початок гри");
      Serial.println("9 - Зупинити гру");
      
//      Serial.println("Приклад команди:");
//      Serial.println("1,gameID,palyerCount;");
//      Serial.println("2,gameID,palyerCount;");
//      Serial.println("1,gameID,palyerCount;");
    break;
    default:
      Serial.println("Input ERROR, try '0' to call help");
    break;
  }
}

void req(){  
  if (mode != 2){
       radio.startListening();                   // Need to start listening after opening new reading pipes
       mode = 2;
  }
  byte pipe;                          // Declare variables for the pipe and the byte received
  if (pakageID1 == 1){
  while( radio.available(&pipe)){              // Read all available payloads
      Serial.print("Отримана інформація: ");
      radio.read( &reciveData, sizeof(reciveData));
      radio.writeAckPayload(pipe,&reciveData2, 1);
      if(reciveData.ValidID == 233){
        Serial.print(reciveData.teamID);
        Serial.print(" вбив ");
        Serial.println(reciveData.myID);
      } else {
        Serial.println("ERROR");
      }
//      Serial.println(reciveData.teamID + " вбив " + reciveData.myID);
  }
  } else 
  if (pakageID1 == 2){
  while( radio.available(&pipe)){              // Read all available payloads
      Serial.print("Отримана інформація: ");
      radio.read( &reciveData2, sizeof(reciveData2));
      radio.writeAckPayload(pipe,&reciveData2, 1);
      Serial.print(reciveData2.Status);
      if(reciveData2.ValidID == 122){
        Serial.print(reciveData2.packageID);
        Serial.print(reciveData2.Status);
      } else {
        Serial.println("ERROR");
      }
  }
  }
}

void resp(){
  if (mode != 1){
      mode = 1;
  }
  byte gotByte;
  
  radio.openWritingPipe(addresses[1]);
  if(configIsStart == true){
  switch(packageID){
    case 1:
    for (int o = 1; o<=playerCount; o++){
    Serial.println("Початок налаштування...");
    sendInformation.gameID = gameID;
    sendInformation.playerCount = playerCount;
    sendInformation.packageID = 2;
    
//      Serial.println("start1");
      time1 = micros();
      radio.openWritingPipe(addresses[o]);
      radio.stopListening();
      if ( radio.write(&sendInformation,sizeof(sendInformation)) ){
        if(!radio.available()){ // If nothing in the buffer, we got an ack but it is blank
          printf("Got blank response. round-trip delay: %lu microseconds\n\r",micros()-time1);
        }else{printf("Помилка передачі данних.\n\r");}
      }
    }
    delay(100);
    packageID = 2;
    break;
    
    case 2:
    for (int o = 1; o<=playerCount; o++){
//      Serial.println("start2");
      for (int i = 1; i<=playerCount; i++){
      Serial.println(Nickname1[i].buf);
//      Serial.println("send1");
        time1 = micros();
        radio.openWritingPipe(addresses[o]);
        radio.stopListening();
       delay(100);
        if ( radio.write(&Nickname1[i],sizeof(Nickname1[i])) ){
          if(!radio.available()){ // If nothing in the buffer, we got an ack but it is blank
            printf("Got blank response. round-trip delay: %lu microseconds\n\r",micros()-time1);
          }
        }
        else if ( radio.write(&Nickname1[i],sizeof(Nickname1[i])) ){
          if(!radio.available()){ // If nothing in the buffer, we got an ack but it is blank
            printf("Got blank response. round-trip delay: %lu microseconds\n\r",micros()-time1);
          }else{printf("Помилка передачі данних.\n\r");}
        }
      }
    }
    delay(100);
    packageID = 4;
    break;
  
    case 4:
    for (int o = 1; o<=playerCount; o++){
//      Serial.println("start4");  
      time1 = micros();
     sendInformation2.packageID = 3;
      sendInformation2.MyID = o;
      playersID[o] = o;
      radio.openWritingPipe(addresses[o]);
      radio.stopListening();     
      if ( radio.write(&sendInformation2,sizeof(sendInformation2)) ){
        if(!radio.available()){ // If nothing in the buffer, we got an ack but it is blank
          printf("Got blank response. round-trip delay: %lu microseconds\n\r",micros()-time1);     
        }else{printf("Помилка передачі данних.\n\r");}
    }
    }
    delay(100);
    packageID = 3;
    configIsStart = false;
    Serial.println("Налаштування завершено"); 
    if (gameIsStart == true) Serial.println("Гра починається!");
    break;
  }
  }
  
  if(gameIsStart == true && packageID == 3){
    sendInformation3.packageID = packageID;
    for (int o = 1; o<=playerCount; o++){
//      Serial.println("start3");  
      time1 = micros();
      radio.openWritingPipe(addresses[o]);
      radio.stopListening();     
      if ( radio.write(&sendInformation3,sizeof(sendInformation3)) ){
        if(!radio.available()){ // If nothing in the buffer, we got an ack but it is blank
          printf("Got blank response. round-trip delay: %lu microseconds\n\r",micros()-time1);     
        }else{printf("Помилка передачі данних.\n\r");}
      }
    }
  }
}
