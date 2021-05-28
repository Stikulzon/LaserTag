// Радіо модуль
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include "mString.h"
// Піни радіо модуля
int SS1 = 10; int CE1 = 9; // RX, TX
// Піни 11, 12, 13 зайняті автоматично
RF24 radio1(CE1, SS1); // "Создать" модуль на пинах SS_1 и CE_1
byte addresses[][6] = {"1Node","2Node","3Node","4Node","5Node","6Node"};              // Radio pipe addresses for the 2 nodes to communicate.

typedef struct {
  byte packageID;
  bool IsCapture;
  byte TeamIsCapture;
  byte ValidID;
} ReciveData;
ReciveData reciveData;

typedef struct {
  byte packageID;
  bool IsCapture;
  byte TeamIsCapture;
  byte ValidID;
} TransmitData;
TransmitData transmitData;

static uint32_t timing1, timing2; // Переменная для хранения точки отсчета millis
byte myPlayerID;
byte deviceCount = 1;
byte myID = 1;
int packageID = 1, gameID;
bool mode = 0, gameIsBegin = 0;

void setup() {
  Serial.begin(9600);
  printf_begin();


  // Радіо модуль
  radio1.begin();
  radio1.setAutoAck(1);                    // Ensure autoACK is enabled
  radio1.enableAckPayload();               // Allow optional ack payloads
  radio1.setRetries(0,15);                 // Smallest time between retries, max no. of retries
  radio1.setPayloadSize(32);                // Here we are sending 1-byte payloads to test the call-response speed
  radio1.openWritingPipe(addresses[0]);        // Both radios listen on the same pipes by default, and switch when writing
  radio1.openReadingPipe(1,addresses[myID]);      // Open a reading pipe on address 0, pipe 1
  radio1.setChannel(0x60);  //выбираем канал (в котором нет шумов!)
  radio1.startListening();                 // Start listening
  
  radio1.setPALevel (RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio1.setDataRate (RF24_1MBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  
  radio1.powerUp();
  radio1.printDetails();                   // Dump the configuration of the rf unit for debugging
}

void loop() {
  // put your main code here, to run repeatedly:
  req();
}

void resp(){
  if (mode != 1){
      radio1.stopListening();
      mode = 1;
  }
  
  timing2 = micros();
  radio1.stopListening();
  if ( radio1.write(&transmitData,sizeof(transmitData)) ){
    if(!radio1.available()){ // If nothing in the buffer, we got an ack but it is blank
      printf("Отримав відповідь. Затримка: %lu мікросекунд\n\r",micros()-timing2);
    }else{printf("Помилка передачі данних.\n\r");}
  }
  
}

void req(){
  
  if (mode != 2){
       radio1.startListening();                   // Need to start listening after opening new reading pipes
       mode = 2;
  }
  byte pipe;                          // Declare variables for the pipe and the byte received
  while( radio1.available(&pipe)){              // Read all available payloads
    if(packageID == 1){
      Serial.println("start1");
      radio1.read( &reciveData, sizeof(reciveData));
      radio1.writeAckPayload(pipe,&reciveData, 1);
      if(reciveData.ValidID == 111){
        gameIsBegin == 1;
        Serial.println("PackageID: ");
        Serial.println(reciveData.packageID);
        Serial.println("IsCapture: ");
        Serial.println(reciveData.IsCapture);
        Serial.println("TeamIsCapture: ");
        Serial.println(reciveData.TeamIsCapture);
      } else {
        Serial.println("ERROR");
        packageID = 1;
//        gameIsBegin = false;
      }
    } else if(packageID == 2){
    }
  }
}
