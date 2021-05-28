// LCD display
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the LCD address to 0x27/0x3f for a 16 chars and 2 line display

// Радіо модуль
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include "mString.h"
// Піни радіо модуля
int SS_1 = 10; int CE_1 = 9; // RX, TX
// Піни 11, 12, 13 зайняті автоматично
RF24 radio(CE_1, SS_1); // "Создать" модуль на пинах SS_1 и CE_1
byte addresses[][6] = {"1Node","2Node","3Node","4Node","5Node","6Node"};              // Radio pipe addresses for the 2 nodes to communicate.

// RFID модуль
#include <SPI.h>
#include <MFRC522.h>

/*Using Hardware SPI of Arduino */
/*MOSI (11), MISO (12) and SCK (13) are fixed */
/*You can configure SS and RST Pins*/
#define SS_PIN 10  /* Slave Select Pin */
#define RST_PIN 7  /* Reset Pin */

/* Create an instance of MFRC522 */
MFRC522 mfrc522(SS_PIN, RST_PIN);
/* Create an instance of MIFARE_Key */
MFRC522::MIFARE_Key key;          

/* Set the block to which we want to write data */
/* Be aware of Sector Trailer Blocks */
int blockNum = 2;  
/* Create an array of 16 Bytes and fill it with data */
/* This is the actual data which is going to be written into the card */
byte blockData [16] = {"tower-1---------"};

/* Create another array to read data from Block */
/* Legthn of buffer should be 2 Bytes more than the size of Block (16 Bytes) */
byte bufferLen = 18;
byte readBlockData[18];

MFRC522::StatusCode status;

byte zero[] = {
  B11111,
  B10001,
  B10001,
  B10001,
  B10001,
  B10001,
  B10001,
  B11111
};

byte two[] = {
  B11111,
  B11001,
  B11001,
  B11001,
  B11001,
  B11001,
  B11001,
  B11111
};

byte three[] = {
  B11111,
  B11101,
  B11101,
  B11101,
  B11101,
  B11101,
  B11101,
  B11111
};

byte five[] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

byte squareBracket[] = {
  B00111,
  B00011,
  B00011,
  B00001,
  B00001,
  B00011,
  B00011,
  B00111
};

byte squareBracketTwo[] = {
  B11100,
  B11000,
  B11000,
  B10000,
  B10000,
  B11000,
  B11000,
  B11100
};

byte point[] = {
  B00100,
  B01010,
  B10001,
  B10001,
  B10001,
  B10001,
  B01010,
  B00100
};

byte pointFill[] = {
  B00100,
  B01110,
  B11111,
  B11111,
  B11111,
  B11111,
  B01110,
  B00100
};

long randNumber;

// ============== Приём данных по однопроводному юарту ==============
// подключаем софт юарт
#include "softUART.h"
// делаем только приёмником (экономит память)
softUART<5, GBUS_RX> UART(1000); // пин 5, скорость 1000

// подключаем GBUS
#include "GBUS.h"
GBUS bus(&UART, 5, 20); // обработчик UART, адрес 5, буфер 20 байт

struct myStruct {
  byte dieCheck;
  byte teamID;
  byte playerID;
  byte ValidID;
};
myStruct LiveData;

struct TransmitData {
  byte teamID;
  byte myID;
  byte playerID;
  byte ValidID = 233;
};
TransmitData transmitData;

typedef struct {
  byte packageID;
  byte gameID;
  byte playerCount;
  byte ValidID;
} ConfigData;
ConfigData reciveData;

typedef struct {
  byte packageID;
  byte myID;
  byte ValidID;
} myStruct2;
myStruct2 reciveData3;

//struct Nicknames {
//  mString<16> PlayerName[8];
//};
//Nicknames reciveData2[3];

struct myStruct3 {
  byte packageID;
  byte enemyTeamScore;
  byte myTeamScore;
  byte enemyBases;
  byte myBases;
  bool kill;
  byte killerID;
  byte killedID;
  byte ValidID;
};
myStruct3 reciveData4;

struct myStruct4 {
  byte packageID;
  bool kill;
  byte killerID;
  byte killedID;
  byte ValidID;
};
myStruct4 reciveData5;

static uint32_t timing1, timing2; // Переменная для хранения точки отсчета millis
byte myPlayerID;


byte myID = 1;
mString<16> Nicknames1[8];
int packageID = 1, gameID;
bool mode = 0, gameIsBegin = 0;

void setup() {
  Serial.begin(9600);
  printf_begin();
  
  // RFID модуль
  digitalWrite(SS_1, HIGH); // turn off radio1
  mfrc522.PCD_Init();
  Serial.println("Scan a MIFARE 1K Tag to write data...");
  
  // Радіо модуль
  digitalWrite(SS_PIN, HIGH); // turn off RFID
  radio.begin();
  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.enableAckPayload();               // Allow optional ack payloads
  radio.setRetries(0,15);                 // Smallest time between retries, max no. of retries
  radio.setPayloadSize(32);                // Here we are sending 1-byte payloads to test the call-response speed
  radio.openWritingPipe(addresses[0]);        // Both radios listen on the same pipes by default, and switch when writing
  radio.openReadingPipe(1,addresses[myID]);      // Open a reading pipe on address 0, pipe 1
  radio.setChannel(0x60);  //выбираем канал (в котором нет шумов!)
  radio.startListening();                 // Start listening
  
  radio.setPALevel (RF24_PA_LOW); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_1MBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  
  radio.powerUp();
  radio.printDetails();                   // Dump the configuration of the rf unit for debugging
  
  
  //LCD Display initialisation
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.createChar(0, zero);
  lcd.createChar(1, two);
  lcd.createChar(2, three);
  lcd.createChar(3, five);
  lcd.createChar(4, squareBracket);
  lcd.createChar(5, squareBracketTwo);
  lcd.createChar(6, point);
  lcd.createChar(7, pointFill);
  lcd.home();
  lcd.setCursor(0, 0);
  lcd.print("Ready to play");
  
}

void loop() {
  digitalWrite(SS_PIN, HIGH); // turn off RFID
  req();
  LCD_display();
  if(gameIsBegin == true){
    SoftUARTrecive();
  }
  digitalWrite(SS_1, HIGH); // turn off radio
  RFID();
}

void LCD_display(){
  if (millis() - timing1 > 1000){ // Вместо 10000 подставьте нужное вам значение паузы 
    timing1 = millis();
    if (gameIsBegin == true){
      if (reciveData.gameID == 1) {
  // Progress Bar
  randNumber = random(99);
  lcd.setCursor(5,0);
  lcd.print(reciveData4.myTeamScore);
  lcd.print("   ");
  lcd.setCursor(7,0);
  lcd.write(4);
  updateProgressBar(randNumber, 99, 0, 0, 4);
  
  randNumber = random(99);
  lcd.setCursor(9,0);
  lcd.print(reciveData4.enemyTeamScore);
  lcd.print("   ");
  lcd.setCursor(8,0);
  lcd.write(5);
  updateProgressBar(randNumber, 99, 11, 0, 4);

  // Point's
  lcd.setCursor(0,1);
  lcd.write(6);
  lcd.setCursor(1,1);
  lcd.write(6);
  lcd.setCursor(2,1);
  lcd.write(6);
  lcd.setCursor(13,1);
  lcd.write(6);
  lcd.setCursor(14,1);
  lcd.write(6);
  lcd.setCursor(15,1);
  lcd.write(6);

  lcd.setCursor(3,1);
  lcd.write(4);
  lcd.setCursor(12,1);
  lcd.write(5);
  
  lcd.setCursor(4,1);
  lcd.print("RED WIN!");
        if(reciveData4.kill == true) {
          lcd.setCursor(0,1);
          lcd.print(Nicknames1[reciveData4.killedID].buf);
          lcd.setCursor(Nicknames1[reciveData4.killedID].length() + 1, 1);
          lcd.print("killed you!");
          timing1 += 3000;
        }
      }
      if (reciveData.gameID == 2 && reciveData4.kill == true) {
        lcd.setCursor(0,1);
        lcd.print(Nicknames1[reciveData4.killedID].buf);
        lcd.setCursor(Nicknames1[reciveData4.killedID].length() + 1, 1);
        lcd.print("killed you!");
        timing1 += 3000;
      }
    }
  }
}

void parseString(){
//  char* strings[5];
//  int players = reciveData2.PlayerName.split(strings, ',');
  
//  Serial.println(strings[1]); 
  //lcd.setCursor(0, 1);
  //lcd.print(strings[LiveData.playerID]);
  //lcd.setCursor(strlen(strings[LiveData.playerID]) + 1, 1);
  //lcd.print("killed you!");
  //delay(1000);    
}

void SoftUARTrecive(){
  // в тике сидит отправка и приём
  bus.tick();

  if (bus.gotData()) {
    // выводим данные
    myStruct LiveData;
    bus.readData(LiveData);
    
    Serial.println(myID);
    Serial.println(LiveData.playerID);
    Serial.println(LiveData.teamID);
    Serial.println(LiveData.dieCheck);
    transmitData.myID = myID;
    transmitData.playerID = LiveData.playerID;
    transmitData.teamID = LiveData.teamID;
    if (LiveData.dieCheck == 1)
    {
      resp();
    }
  }
}

void resp(){
  if (mode != 1){
      radio.stopListening();
      mode = 1;
  }
  
  Serial.println(transmitData.myID);
  Serial.println(transmitData.playerID);
  Serial.println(transmitData.teamID);
  timing2 = micros();
  radio.stopListening();
  if ( radio.write(&transmitData,sizeof(transmitData)) ){
    if(!radio.available()){ // If nothing in the buffer, we got an ack but it is blank
      printf("Отримав відповідь. Затримка: %lu мікросекунд\n\r",micros()-timing2);
    }else{printf("Помилка передачі данних.\n\r");}
  }
  
}

int i = 1;
void req(){
  
  if (mode != 2){
       radio.startListening();                   // Need to start listening after opening new reading pipes
       mode = 2;
  }
  byte pipe;                          // Declare variables for the pipe and the byte received
  while( radio.available(&pipe)){              // Read all available payloads
    if(packageID == 1){
      Serial.println("start1");
      radio.read( &reciveData, sizeof(reciveData));
      radio.writeAckPayload(pipe,&reciveData, 1);
//      if(pipe==1){
      if(reciveData.ValidID == 111){
        Serial.println("packageID: ");
        Serial.println(reciveData.packageID);
        Serial.println("gameID: ");
        Serial.println(reciveData.gameID);
        Serial.println("playerCount: ");
        Serial.println(reciveData.playerCount);
        gameID = reciveData.gameID;
        packageID = reciveData.packageID;
      } else {
        Serial.println("ERROR");
        packageID = 1;
//        gameIsBegin = false;
      }
//      }
    } else if(packageID == 2){
//      for(int o = 0; o<=reciveData.playerCount; o++){
      radio.read( &Nicknames1[i], sizeof(Nicknames1[i]));
      radio.writeAckPayload(pipe,&Nicknames1[i], 1);
//      if(pipe==1){
      if (i<=reciveData.playerCount) {
        Serial.println("recive1");
        Serial.println(Nicknames1[i].buf);
        i++;
//      }
//      }
    }
      if (i>reciveData.playerCount){
        packageID = 4;
      }
    } else if(packageID == 4){
        Serial.println("start4");
        radio.read( &reciveData3, sizeof(reciveData3));
        Serial.println("start4");
        radio.writeAckPayload(pipe,&reciveData3, 1);
        Serial.println("start4");
//      if(pipe==1){
        if(reciveData3.ValidID == 111){
          Serial.println(reciveData3.packageID);
          packageID = reciveData3.packageID;
        } else {
          Serial.println("ERROR");
          packageID = 4;
          gameIsBegin = false;
        }
//      }
    } else if(packageID == 3){
      Serial.println("start3");
      radio.read( &reciveData4, sizeof(reciveData4));
      radio.writeAckPayload(pipe,&reciveData4, 1);
//      if(pipe==1){
      if(reciveData4.ValidID == 111){
        gameIsBegin = true;
      } else {
        Serial.println("ERROR");
      }
//      }
    }
    else if(packageID == 3 && gameID == 2){
      radio.read( &reciveData4, sizeof(reciveData4));
//      radio.writeAckPayload(pipe,&reciveData4, 1);
      gameIsBegin = true;
      LCD_display();
    } else {
      Serial.println("ERROR");
    }
  }
}
void updateProgressBar(unsigned long count, unsigned long totalCount, int cellToPrintOn, int lineToPrintOn, int ProgressBarLength)
 {
    double factor = totalCount/(ProgressBarLength*5);          //See note above!
    int percent = (count+1)/factor;
    int number = percent/5;
    int remainder = percent%5;
    int rem;
    
    switch (remainder){
      case 0:
        rem = 0;
        break;
      case 1:
        rem = 1;
        break;
      case 2:
        rem = 1;
        break;
      case 3:
        rem = 2;
        break;
      case 4:
        rem = 2;
        break;
      case 5:
        rem = 3;
        break;
    }
    
    if(number > 0)
    {
      for(int j = 0; j < number; j++)
      {
        lcd.setCursor(j+cellToPrintOn,lineToPrintOn);
       lcd.write(3);
      }
    }
       lcd.setCursor(number+cellToPrintOn,lineToPrintOn);
       lcd.write(rem); 
     if(number < ProgressBarLength)
    {
      for(int j = number+1; j <= ProgressBarLength; j++)
      {
        lcd.setCursor(j+cellToPrintOn,lineToPrintOn);
       lcd.write(0);
      }
    }  
 }

 void RFID(){
  
  /* Prepare the ksy for authentication */
  /* All keys are set to FFFFFFFFFFFFh at chip delivery from the factory */
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }
  
  /* Select one of the cards */
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }
    Serial.print("\n");
    Serial.println("**Card Detected**");
    /* Print UID of the Card */
    Serial.print(F("Card UID:"));
    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.print("\n");
    /* Print type of card (for example, MIFARE 1K) */
    Serial.print(F("PICC type: "));
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.println(mfrc522.PICC_GetTypeName(piccType));
     
    /* Read data from the same block */
    Serial.print("\n");
    Serial.println("Reading from Data Block...");
    ReadDataFromBlock(blockNum, readBlockData);
    /* If you want to print the full memory dump, uncomment the next line */
    //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
   
    /* Print the data read from block */
    Serial.print("\n");
    Serial.print("Data in Block:");
    Serial.print(blockNum);
    Serial.print(" --> ");
    String bytes = "";
    for (int j=0 ; j<16 ; j++)
    {
//    bytes = bytes + );
      Serial.print((char*)readBlockData[j]);
    }
    Serial.print("\n");
 }

void ReadDataFromBlock(int blockNum, byte readBlockData[]) 
{
  /* Authenticating the desired data block for Read access using Key A */
  byte status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK)
  {
     Serial.print("Authentication failed for Read: ");
     Serial.println(mfrc522.GetStatusCodeName(status));
     return;
  }
  else
  {
    Serial.println("Authentication success");
  }

  /* Reading data from the Block */
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  else
  {
    Serial.println("Block was read successfully");  
  }
  
}
