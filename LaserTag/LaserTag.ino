// OLED дисплей
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Звуковий модуль
#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

// Піни звукового модулю
byte soundRX_pin = 11;
byte soundTX_pin = 9;
SoftwareSerial mySoftwareSerial(soundRX_pin, soundTX_pin); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);


// Digital IO's
//int triggerPin             = 3;      // Push button for primary fire. Low = pressed
byte hitPin                 = 7;      // LED output pin used to indicate when the player has been hit.
byte IRreceive2Pin          = 3;     // Allows for checking external sensors are attached as well as distinguishing between sensor locations (eg spotting head shots)
byte IRtransmitPin          = 2;      // Primary fire mode IR transmitter pin: Use pins 2,4,7,8,12 or 13. DO NOT USE PWM pins!! More info: http://j44industries.blogspot.com/2009/09/arduino-frequency-generation.html#more
byte IRreceivePin           = 4;     // The pin that incoming IR signals are read from
//int speakerPin             = 3;      // Direct output to piezo sounder/speaker

// int IRtransmit2Pin         = 8;      // Secondary fire mode IR transmitter pin:  Use pins 2,4,7,8,12 or 13. DO NOT USE PWM pins!!
// int trigger2Pin            = 13;     // Push button for secondary fire. Low = pressed 
// закоментировал по причине нехватки пинов

// Player and Game details
byte myTeamID               = 1;      // 1-7 (0 = system message)
byte myPlayerID             = 2;      // Player ID
byte myGameID               = 0;      // Interprited by configureGane subroutine; allows for quick change of game types.
byte myWeaponID             = 0;      // Deffined by gameType and configureGame subroutine.
byte myWeaponHP             = 0;      // Deffined by gameType and configureGame subroutine.
byte ClipType               = 1;      // 0 - показує кількість обойм, при преезарядці надлишок патронів зникає; 1 - показує кількість патронів в обоймі, надлишок зберігається.
byte ClipSize               = 15;     // Deffined by gameType and configureGame subroutine.
byte maxClips               = 5;      // Deffined by gameType and configureGame subroutine.
byte maxLife                = 0;      // Deffined by gameType and configureGame subroutine.
byte automatic              = 1;      // Deffined by gameType and configureGame subroutine. Automatic fire 0 = Semi Auto, 1 = Fully Auto.
byte automatic2             = 0;      // Deffined by gameType and configureGame subroutine. Secondary fire auto?
int shootCooldownTime       = 350;
int reloadCooldownTime      = 3000;

//Incoming signal Details
int received[18];                    // Received data: received[0] = which sensor, received[1] - [17] byte1 byte2 parity (Miles Tag structure)
int check                  = 0;      // Variable used in parity checking

// Stats
int ammo                   = 0;      // Current ammunition (автоматично налаштовується)
byte life                   = 0;      // Current life (автоматично налаштовується)
int Clips                  = 3;      // Current clips (автоматично налаштовується)
int ClipAmmo               = 0;      // Current ammo in clips (автоматично налаштовується)

// Code Variables
int timeOut                 = 0;      // Deffined in frequencyCalculations (IRpulse + 50)
bool FIRE                   = 0;      // 0 = don't fire, 1 = Primary Fire, 2 = Secondary Fire
bool rTR                    = 0;      // Reload Trigger
bool LrTR                   = 0;      // Last Reload Trigger Reading
bool TR                     = 0;      // Trigger Reading
bool LTR                    = 0;      // Last Trigger Reading
bool T2R                    = 0;      // Trigger 2 Reading (For secondary fire)
bool LT2R                   = 0;      // Last Trigger 2 Reading (For secondary fire)

// Signal Properties
int IRpulse                = 600;    // Basic pulse duration of 600uS MilesTag standard 4*IRpulse for header bit, 2*IRpulse for 1, 1*IRpulse for 0.
int IRfrequency            = 38;     // Frequency in kHz Standard values are: 38kHz, 40kHz. Choose dependant on your receiver characteristics
int IRt                    = 0;      // LED on time to give correct transmission frequency, calculated in setup.
int IRpulses               = 0;      // Number of oscillations needed to make a full IRpulse, calculated in setup.
int header                 = 4;      // Header lenght in pulses. 4 = Miles tag standard

// Transmission data
int byte1[8];                        // String for storing byte1 of the data which gets transmitted when the player fires.
int byte2[8];                        // String for storing byte1 of the data which gets transmitted when the player fires.
int myParity               = 0;      // String for storing parity of the data which gets transmitted when the player fires.

// Received data
byte memory                 = 10;     // Number of signals to be recorded: Allows for the game data to be reviewed after the game, no provision for transmitting / accessing it yet though.
byte hitNo                  = 0;      // Hit number
// Byte1
byte player[10];                      // Array must be as large as memory
byte team[10];                        // Array must be as large as memory
// Byte2
byte weapon[10];                      // Array must be as large as memory
byte hp[10];                          // Array must be as large as memory
byte parity[10];                      // Array must be as large as memory

static uint32_t timing01, timing1, timing2, timing3; // Переменная для хранения точки отсчета millis

bool shootCooldown = false, reloadCooldown = false;
// отправка данных по однопроводному юарту
// подключаем софт юарт
#include "softUART.h"
// делаем только отправителем (экономит память)
softUART<5, GBUS_TX> UART(1000); // пин 5, скорость 1000

// подключаем GBUS
#include "GBUS.h"
GBUS bus(&UART, 3, 20); // обработчик UART, адрес 3, буфер 20 байт


struct myStruct {
  byte dieCheck;
  byte teamID;
  byte playerID;
  byte ValidID = 111;
};
myStruct DieInformation;

void setup() {
  // Serial coms set up to help with debugging.
  Serial.begin(9600);              
  Serial.println("Startup...");
  
  // Pin declarations
  
  pinMode(A0, INPUT);
  pinMode(A2, INPUT);
//  pinMode(trigger2Pin, INPUT);
//  pinMode(speakerPin, OUTPUT);
  pinMode(hitPin, OUTPUT);
  pinMode(IRtransmitPin, OUTPUT);
//  pinMode(IRtransmit2Pin, OUTPUT);
  pinMode(IRreceivePin, INPUT);
//  pinMode(IRreceive2Pin, INPUT); // Пин для датчиков на голове

  mySoftwareSerial.begin(9600);
  
  DieInformation.dieCheck = 0;
  DieInformation.playerID = 0;
  
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  
  if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }
  Serial.println(F("DFPlayer Mini online."));
  
  myDFPlayer.setTimeOut(500); //Set serial communictaion time out 500ms
  
  //----Set volume----
  myDFPlayer.volume(30);  //Set volume value (0~30).
  
  //----Set different EQ----
 myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  
  //----Set device we use SD as default----
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);

  Serial.println("Ready1");

  frequencyCalculations();   // Calculates pulse lengths etc for desired frequency
  configureGame();           // Look up and configure game details
  tagCode();                 // Based on game details etc works out the data that will be transmitted when a shot is fired


  digitalWrite(A0, HIGH);      // Not really needed if your circuit has the correct pull up resistors already but doesn't harm
  digitalWrite(A2, HIGH);      // Not really needed if your circuit has the correct pull up resistors already but doesn't harm
//  digitalWrite(trigger2Pin, HIGH);     // Not really needed if your circuit has the correct pull up resistors already but doesn't harm

  Serial.println("Ready2");
  
  u8g2.begin(); // Initialization OLED display
  playMp3("Ready");
  Serial.println("Ready....");
}



void loop(){
//  OLEDdisplay();
  receiveIR();
  if(FIRE != 0){
    shoot();
  }
  triggers();
  cooldowns();

  if(reloadCooldown == false){
   if(millis() - timing01 > 100){
    OLEDdisplay();
    }
  } else {OLEDdisplay();}
  SoftUARTsending();
}



// SUB ROUTINES
void SoftUARTsending(){
  // в тике сидит отправка и приём
  bus.tick();
  static uint32_t tmr;
//  if (millis() - tmr >= 2000) {
    tmr = millis();
    // отправляем каждые 2 секунды
    bus.sendData(5, DieInformation);  // на адрес 5
//  }
}

void OLEDdisplay(){
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB10_tr);
    //u8g2.drawStr(0,24,"HP:");

    
    u8g2.setCursor(0, 20);
    u8g2.print(F("HP:"));
    
    if(reloadCooldown == true)
    {
      float remainingCooldownTime = float(reloadCooldownTime - (millis() - timing3))*0.001;
      u8g2.setCursor(80, 20);
      u8g2.print(remainingCooldownTime);
    }
    
    u8g2.setCursor(40, 20);
    u8g2.print(life);

    u8g2.setFont(u8g2_font_inb33_mn);
    
    u8g2.setCursor(0, 64);
    u8g2.print(ammo);
    
    u8g2.setFont(u8g2_font_inb16_mn);
    
    u8g2.setCursor(55, 64);
    u8g2.print(F("/"));
    u8g2.setCursor(70, 64);
    
    if (ClipType == 0)
    {
     u8g2.print(Clips);
    } else
    if (ClipType == 1)
    {
      u8g2.print(ClipAmmo);
    }
    
  } while ( u8g2.nextPage() );
  
  timing01 = millis();
}

void playMp3(String soundName){
  if(soundName == "Shoot"){
     for (int i = 1;i < 254;i++) { // Loop plays start up noise
      if (i == 1) {
        myDFPlayer.play(random(4, 6));
      }
    } 
  }
  if(soundName == "Ready"){
     for (int i = 1;i < 254;i++) { // Loop plays start up noise
      if (i == 1) {
        myDFPlayer.play(3);
      }
    } 
  }
  if(soundName == "takeHit"){
//  myDFPlayer.play();
  }
  if(soundName == "Reload"){
     for (int i = 1;i < 254;i++) { // Loop plays start up noise
      if (i == 1) {
        myDFPlayer.play(3);
      }
    } 
  }
  if(soundName == "NoAmmo"){
     for (int i = 1;i < 254;i++) { // Loop plays start up noise
      if (i == 1) {
        myDFPlayer.play(4);
      }
    } 
  }
  if(soundName == "Die"){
  //myDFPlayer.play();
  }
  if(soundName == "HP"){
  //myDFPlayer.play();
  }
}

//void playTone(int tone, int duration) { // A sub routine for playing tones like the standard arduino melody example
//  for (long i = 0; i < duration * 1000L; i += tone * 2) {
//    digitalWrite(speakerPin, HIGH);
//    delayMicroseconds(tone);
//    digitalWrite(speakerPin, LOW);
//    delayMicroseconds(tone);
//}
//

void receiveIR() { // Void checks for an incoming signal and decodes it if it sees one.
  int error = 0;

  if(digitalRead(IRreceivePin) == LOW){    // If the receive pin is low a signal is being received.
    digitalWrite(hitPin,HIGH);
//    if(digitalRead(IRreceive2Pin) == LOW){ // Is the incoming signal being received by the head sensors?
//      received[0] = 1;
//    }
//    else{
      received[0] = 0;
//    }

    
//    for (int i = 1; i <= 1000 || digitalRead(IRreceivePin) == HIGH; i++) // задаем начальное значение 1, конечное 1000 и задаем шаг цикла - 1.
//    {
//    }

    Serial.println("Original Signal: "); 
//    if(digitalRead(IRreceivePin) == LOW){
      unsigned long timing0;
      timing0 = millis();
    for(int i = 1; i <= 17; i++) {                        // Repeats several times to make sure the whole signal has been received
      received[i] = pulseIn(IRreceivePin, LOW);  // pulseIn command waits for a pulse and then records its duration in microseconds. 
      Serial.print(received[i]); 
      
      // Time out
        if (millis() - timing0 > 500){ 
         Serial.println("Time Out!"); 
         timing0 = millis();
         for(int o = 1; i <= 17; i++) {  
          received[i] = 0;
         } 
         
     }
    }
    Serial.println(" "); 

    Serial.print("sensor: ");                            // Prints if it was a head shot or not.
    Serial.print(received[0]); 
    Serial.print("...");

    for(int i = 1; i <= 17; i++) {  // Looks at each one of the received pulses
      int receivedTemp[18];
      receivedTemp[i] = 2;
      if(received[i] > (IRpulse - 200) &&  received[i] < (IRpulse + 200)) {receivedTemp[i] = 0;}                      // Works out from the pulse length if it was a data 1 or 0 that was received writes result to receivedTemp string
      if(received[i] > (IRpulse + IRpulse - 200) &&  received[i] < (IRpulse + IRpulse + 200)) {receivedTemp[i] = 1;}  // Works out from the pulse length if it was a data 1 or 0 that was received  
      received[i] = 3;                   // Wipes raw received data
      received[i] = receivedTemp[i];     // Inputs interpreted data

      Serial.print(" ");
      Serial.print(received[i]);         // Print interpreted data results
    }
    Serial.println("");                  // New line to tidy up printed results

    // Parity Check. Was the data received a valid signal?
    check = 0;
    for(int i = 1; i <= 16; i++) {
      if(received[i] == 1){check = check + 1;}
      if(received[i] == 2){error = 1;}
    }
    // Serial.println(check);
    check = check >> 0 & B1;
    // Serial.println(check);
    if(check != received[17]){error = 1;}
    if(error == 0){Serial.println("Valid Signal");}
    else{Serial.println("ERROR");}
    if(error == 0){interpritReceived();}
    digitalWrite(hitPin,LOW);
//    }
    
  }
}


void interpritReceived(){  // After a message has been received by the ReceiveIR subroutine this subroutine decidedes how it should react to the data
  if(hitNo == memory){hitNo = 0;} // hitNo sorts out where the data should be stored if statement means old data gets overwritten if too much is received
  team[hitNo] = 0;
  player[hitNo] = 0;
  weapon[hitNo] = 0;
  hp[hitNo] = 0;
  // Next few lines Effectivly converts the binary data into decimal
  // Im sure there must be a much more efficient way of doing this
  if(received[1] == 1){team[hitNo] = team[hitNo] + 4;}
  if(received[2] == 1){team[hitNo] = team[hitNo] + 2;}
  if(received[3] == 1){team[hitNo] = team[hitNo] + 1;} 

  if(received[4] == 1){player[hitNo] = player[hitNo] + 16;}
  if(received[5] == 1){player[hitNo] = player[hitNo] + 8;}
  if(received[6] == 1){player[hitNo] = player[hitNo] + 4;}
  if(received[7] == 1){player[hitNo] = player[hitNo] + 2;}
  if(received[8] == 1){player[hitNo] = player[hitNo] + 1;}

  if(received[9] == 1){weapon[hitNo] = weapon[hitNo] + 4;}
  if(received[10] == 1){weapon[hitNo] = weapon[hitNo] + 2;}
  if(received[11] == 1){weapon[hitNo] = weapon[hitNo] + 1;} 

  if(received[12] == 1){hp[hitNo] = hp[hitNo] + 16;}
  if(received[13] == 1){hp[hitNo] = hp[hitNo] + 8;}
  if(received[14] == 1){hp[hitNo] = hp[hitNo] + 4;}
  if(received[15] == 1){hp[hitNo] = hp[hitNo] + 2;}
  if(received[16] == 1){hp[hitNo] = hp[hitNo] + 1;}

  parity[hitNo] = received[17];

  Serial.print("Hit No: ");
  Serial.print(hitNo);
  Serial.print("  Player: ");
  DieInformation.playerID = player[hitNo];
  Serial.print(player[hitNo]);
  Serial.print("  Team: ");
  Serial.print(team[hitNo]);
  DieInformation.teamID = player[hitNo];
  Serial.print("  Weapon: ");
  Serial.print(weapon[hitNo]);
  Serial.print("  HP: ");
  Serial.print(hp[hitNo]);
  Serial.print("  Parity: ");
  Serial.println(parity[hitNo]);


  //This is probably where more code should be added to expand the game capabilities at the moment the code just checks that the received data was not a system message and deducts a life if it wasn't.
  if (player[hitNo] != 0){hit();}
  hitNo++ ;
}


void shoot() {
 if (shootCooldown == false && reloadCooldown == false){

  if(FIRE == 1){ // Has the trigger been pressed?

//    Serial.println("FIRE 1");
    sendPulse(IRtransmitPin, 4); // Transmit Header pulse, send pulse subroutine deals with the details
    delayMicroseconds(IRpulse);

    for(int i = 0; i < 8; i++) { // Transmit Byte1
      if(byte1[i] == 1){
        sendPulse(IRtransmitPin, 1);
        //Serial.print("1 ");
      }
      //else{Serial.print("0 ");}
      sendPulse(IRtransmitPin, 1);
      delayMicroseconds(IRpulse);
    }

    for(int i = 0; i < 8; i++) { // Transmit Byte2
      if(byte2[i] == 1){
        sendPulse(IRtransmitPin, 1);
       // Serial.print("1 ");
      }
      //else{Serial.print("0 ");}
      sendPulse(IRtransmitPin, 1);
      delayMicroseconds(IRpulse);
    }

    if(myParity == 1){ // Parity
      sendPulse(IRtransmitPin, 1);
    }
    sendPulse(IRtransmitPin, 1);
    delayMicroseconds(IRpulse);
    Serial.println("DONE 1");
    ammo--;

    playMp3("Shoot");

  }


  if(FIRE == 2){ // Where a secondary fire mode would be added
//    Serial.println("FIRE 2");
    sendPulse(IRtransmitPin, 4); // Header
    Serial.println("DONE 2");
  }
  //ammo = ammo - 1;
  shootCooldown = true;
  timing1 = millis();
 } 
  FIRE = 0;
}

void cooldowns(){
 if (shootCooldown == true) {
    if (millis() - timing1 > shootCooldownTime){
      timing1 = millis();
      shootCooldown = false; 
    }
  }
  if (reloadCooldown == true) {
    if (millis() - timing3 > reloadCooldownTime){
      timing3 = millis();
      reloadCooldown = false; 
      
      // Продовження перезарядки (знаю, запутано, але як є)
      if(ClipType == 0){
        ammo = ClipSize;
        Clips--;
      }
      if(ClipType == 1){
        if(ClipAmmo >= ClipSize){
        ClipAmmo -= (ClipSize - ammo);
        ammo = ClipSize;
        } else 
        if(ClipAmmo < ClipSize){
        ammo = ClipAmmo;
        ClipAmmo -= ClipAmmo;
        }
      }
    }
  }
}

void sendPulse(int pin, int length){ // importing variables like this allows for secondary fire modes etc.
// This void genertates the carrier frequency for the information to be transmitted
  int i = 0;
  int o = 0;
  while( i < length ){
    i++;
    while( o < IRpulses ){
      o++;
      digitalWrite(pin, HIGH);
      delayMicroseconds(IRt);
      digitalWrite(pin, LOW);
      delayMicroseconds(IRt);
    }
  }
}


void triggers() { // Checks to see if the triggers have been presses
  LTR = TR;       // Records previous state. Primary fire
  LT2R = T2R;     // Records previous state. Secondary fire
  TR = digitalRead(A0);      // Looks up current trigger button state
  rTR = digitalRead(A2);      // Looks up current trigger button state
//  T2R = digitalRead(trigger2Pin);    // Looks up current trigger button state
  // Code looks for changes in trigger state to give it a semi automatic shooting behaviour
  if(TR != LTR && TR == LOW){
    FIRE = 1;
  }
  if(T2R != LT2R && T2R == LOW){
    FIRE = 2;
  }
  if(TR == LOW && automatic == 1){
    FIRE = 1;
  }
  if(T2R == LOW && automatic2 == 1){
    FIRE = 2;
  }
  if(FIRE == 1 || FIRE == 2){
    if(ammo < 1){FIRE = 0; noAmmo();}
    if(life < 1){FIRE = 0; dead();}
    // Fire rate code to be added here  
//    Serial.println("FIRE");
  }
  if(rTR == LOW){
    if(ClipAmmo > 0 && ammo != ClipSize && reloadCooldown != true){
    playMp3("Reload");
    reloadCooldown = true;
    timing3 = millis();
    // Початок перезарядки, продовження в куллдауні
    }
  }

}


void configureGame() { // Where the game characteristics are stored, allows several game types to be recorded and you only have to change one variable (myGameID) to pick the game.
  if(myGameID == 0){
    myWeaponID = 1;
    ClipSize = 15;
    Clips = 3; 
    maxClips = 5;
    maxLife = 3;
    myWeaponHP = 1;
    
    // Автоматичне налаштування
    ammo = ClipSize;
    life = maxLife;
    if(ClipType == 1){
    ClipAmmo = ClipSize*Clips;
    }
    
  }
  if(myGameID == 1){
    myWeaponID = 1;
    ClipSize = 100;
    ammo = 100;
    maxLife = 10;
    life = 10;
    myWeaponHP = 2;
  }
}


void frequencyCalculations() { // Works out all the timings needed to give the correct carrier frequency for the IR signal
  IRt = (int) (500/IRfrequency);  
  IRpulses = (int) (IRpulse / (2*IRt));
  IRt = IRt - 4;
  // Why -4 I hear you cry. It allows for the time taken for commands to be executed.
  // More info: http://j44industries.blogspot.com/2009/09/arduino-frequency-generation.html#more

  Serial.print("Oscilation time period /2: ");
  Serial.println(IRt);
  Serial.print("Pulses: ");
  Serial.println(IRpulses);
  timeOut = IRpulse + 50; // Adding 50 to the expected pulse time gives a little margin for error on the pulse read time out value
}


void tagCode() { // Works out what the players tagger code (the code that is transmitted when they shoot) is
  byte1[0] = myTeamID >> 2 & B1;
  byte1[1] = myTeamID >> 1 & B1;
  byte1[2] = myTeamID >> 0 & B1;

  byte1[3] = myPlayerID >> 4 & B1;
  byte1[4] = myPlayerID >> 3 & B1;
  byte1[5] = myPlayerID >> 2 & B1;
  byte1[6] = myPlayerID >> 1 & B1;
  byte1[7] = myPlayerID >> 0 & B1;


  byte2[0] = myWeaponID >> 2 & B1;
  byte2[1] = myWeaponID >> 1 & B1;
  byte2[2] = myWeaponID >> 0 & B1;

  byte2[3] = myWeaponHP >> 4 & B1;
  byte2[4] = myWeaponHP >> 3 & B1;
  byte2[5] = myWeaponHP >> 2 & B1;
  byte2[6] = myWeaponHP >> 1 & B1;
  byte2[7] = myWeaponHP >> 0 & B1;

  myParity = 0;
  for (int i=0; i<8; i++) {
   if(byte1[i] == 1){myParity = myParity + 1;}
   if(byte2[i] == 1){myParity = myParity + 1;}
   myParity = myParity >> 0 & B1;
  }

  // Next few lines print the full tagger code.
  Serial.print("Byte1: ");
  Serial.print(byte1[0]);
  Serial.print(byte1[1]);
  Serial.print(byte1[2]);
  Serial.print(byte1[3]);
  Serial.print(byte1[4]);
  Serial.print(byte1[5]);
  Serial.print(byte1[6]);
  Serial.print(byte1[7]);
  Serial.println();

  Serial.print("Byte2: ");
  Serial.print(byte2[0]);
  Serial.print(byte2[1]);
  Serial.print(byte2[2]);
  Serial.print(byte2[3]);
  Serial.print(byte2[4]);
  Serial.print(byte2[5]);
  Serial.print(byte2[6]);
  Serial.print(byte2[7]);
  Serial.println();

  Serial.print("Parity: ");
  Serial.print(myParity);
  Serial.println();
}


void dead() { // void determines what the tagger does when it is out of lives
  Serial.println("DEAD");
  playMp3("Die");
  DieInformation.dieCheck = 1;
  DieInformation.playerID = 5;
  SoftUARTsending();
}


void noAmmo() { // Make some noise and flash some lights when out of ammo
  digitalWrite(hitPin,HIGH);
  playMp3("NoAmmo");
  digitalWrite(hitPin,LOW);
  Serial.println("No Ammo!");
  delay(50);
}


void hit() { // Make some noise and flash some lights when you get shot
  digitalWrite(hitPin,HIGH);
  life = life - hp[hitNo];
  Serial.print("Life: ");
  Serial.println(life);
  playMp3("Hit");
  digitalWrite(hitPin,LOW);
}

void Respawn(){
  DieInformation.dieCheck = 0;
}
