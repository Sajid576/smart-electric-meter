#include <LiquidCrystal.h>
#include <Keypad.h>


//Current Measureent
#define VIN A0
const float VCC   = 5.0;// supply voltage Normally 5V.
const int model = 0;   // enter the model number (see below)
float cutOffLimit = 0.1;// set the current which below that value, doesn't matter.
/*
          "ACS712ELCTR-05B-T",// for model use 0
          "ACS712ELCTR-20A-T",// for model use 1
          "ACS712ELCTR-30A-T"// for model use 2            
*/
float sensitivity[] ={
          0.185,// for ACS712ELCTR-05B-T
          0.100,// for ACS712ELCTR-20A-T
          0.066// for ACS712ELCTR-30A-T    
}; 
const float QOV =  0.5 * VCC; //set quiescent Output voltage of 0.5V
float voltage;
// end Current Measurement



//LCD0 setup
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
//end LCD0 Setup 

//Keypad setup
const byte ROWS = 4; 
const byte COLS = 3; 
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {27, 28, 29, 30}; 
byte colPins[COLS] = {24, 25, 26};
Keypad cusKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 
//end Keypad Setup




const int MQ2Input = 53;  
const int BuzzerOutput = 52;
int MQ2State = 0;


//Program Variables
  String s ="ABCDEFG";
  String RfResponse = "Null  ";


  //Booleans For Programm Control Flow
  bool displayInfoStage = true;
    bool displayInfoDiaplayed = false;
  bool settingsVerificationOptionStage = false;
    bool settingsVerificationInfoDisplayed = false;
  bool intruderAlertStage = false;
  bool valueResetOptionStage = false;
    bool valueResetOptionInfoDisplayed = false;
   bool powerLimitNotification = true;
   bool intruderNotification = true;
   bool fireNotification = true;
    
//Power Measurement Variables
float energy = 0.0;
float limit  =10.0;
float powerThreshold = 9.0;


void setup() {
  lcd.begin(16, 2);
  lcd.setCursor(0, 1);
  lcd.clear();
  pinMode(BuzzerOutput,OUTPUT);
  Serial.begin(9600);
  Serial3.begin(9600);
}


void loop() {
  // Display Info Stage
  if(displayInfoStage){
    if(!displayInfoDiaplayed){
      String line1 = "U: "+String(energy,2)+" L: "+String(limit,1);
      String line2 = "* For Settings";
      LCDWriter(line1,line2);
      displayInfoDiaplayed=true;
    }
    char settingsOption =  cusKeypad.getKey();
    if(settingsOption=='*'){
      settingsVerificationOptionStage=true;
      displayInfoStage=false;
    }
    String line1 = "U: "+String(energy,2)+" L: "+String(limit,1);
    String line2 = "* For Settings";
    LCDWriter(line1,line2);
  }
  //Enter Settings Verification Stage
  if(settingsVerificationOptionStage){
    if(!settingsVerificationInfoDisplayed){
      String line1 = "Use RFID Card";
      String line2 = "# To Back";
      LCDWriter(line1,line2);
      settingsVerificationInfoDisplayed=true;
    }
    char settingsOption =  cusKeypad.getKey();
    if(settingsOption=='#'){
      Serial.println('#');
      displayInfoStage=true;
      displayInfoDiaplayed=false;
      settingsVerificationInfoDisplayed=false;      
      settingsVerificationOptionStage=false;
    }
    listenRFResponse();
    if(RfResponse.equals("True  ")){
      Serial.println("ID matched");      
      valueResetOptionStage = true;
      valueResetOptionInfoDisplayed = false;
      displayInfoStage=false;
      displayInfoDiaplayed=false;
      settingsVerificationInfoDisplayed=false;      
      settingsVerificationOptionStage=false;
    }
    if(RfResponse.equals("False ")){
      Serial.println("ID did not match.");
      intruderAlertStage = true;
      displayInfoDiaplayed=false;
      settingsVerificationInfoDisplayed=false;
      displayInfoStage=false;
      settingsVerificationOptionStage=false;
    }    
  }

  // Value Reset Option Stage
  if(valueResetOptionStage) {
    if(!valueResetOptionInfoDisplayed){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("*->Limit,0->Back");
      lcd.setCursor(0,1);
      lcd.print("#->Threshold");
      valueResetOptionInfoDisplayed= true;
    }    
    char valueResetOption =  cusKeypad.getKey();
    if(valueResetOption == '0'){
      valueResetOptionStage = false;
      displayInfoDiaplayed=false;
      settingsVerificationInfoDisplayed=false;
      displayInfoStage=true;
      settingsVerificationOptionStage=false;
    }else if(valueResetOption == '*'){
      limit=getLimitFromUser();
      Serial.println(limit);
    }else if(valueResetOption == '#'){
      powerThreshold=getThresholdFromUser();
      Serial.println(powerThreshold);
    }
  } 
  //Intruder Detected Stage
  if(intruderAlertStage){
    digitalWrite(BuzzerOutput,HIGH);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Intruder");
    listenRFResponse();
    if(intruderNotification){
      SendMessage("!!!Intruder Alert!!!");
      intruderNotification=false;
    }
    
    if(RfResponse.equals("True  ")){
      intruderAlertStage = false;
      displayInfoDiaplayed=false;
      settingsVerificationInfoDisplayed=false;
      displayInfoStage=true;
      settingsVerificationOptionStage=false;
      intruderNotification=true;
    }
  }

  //Measure Energy
  energy = energy+currentSensor();
  if(energy>=(limit-powerThreshold)){
    if(powerLimitNotification){
      SendMessage("Remaining Power is low. Please Recharge");
      powerLimitNotification=false;
    }    
  }
  Serial.println(energy);

  // MQ2 sensor:
  MQ2State = digitalRead(MQ2Input);
  if(MQ2State == HIGH){
    digitalWrite(BuzzerOutput,HIGH);
    if(fireNotification){
      SendMessage("!! FIRE AT HOUSE. Call 999 !!");
      fireNotification= false;
    }
  }else{
    digitalWrite(BuzzerOutput,LOW);
    fireNotification= true;
  }
  delay(200);
}

void listenRFResponse(){
  for(int i=0;i<6;++i){
    RfResponse[i]=Serial.read();
  }
}

float currentSensor(){
  float voltage_raw =   (5.0 / 1023.0)* analogRead(VIN);// Read the voltage from sensor
  voltage =  voltage_raw - QOV;// 0.000 is a value to make voltage zero when there is no current
  float current = (voltage-0.05) / sensitivity[model];
  float power = 12*current; // Voltage*Current
  Serial.print(power);
  Serial.print("  ");
  float energy = power*(200.0/1000.0);  
  float energyUnit = (energy/1000)*100;    
  return energyUnit;
}

void LCDWriter(String Line1,String Line2){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(Line1);
  lcd.setCursor(0,1);
  lcd.print(Line2);
}

float getLimitFromUser(){
  float limitVal=0.0;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("*=>Enter Limit");
  String limitString="";
  char ch=cusKeypad.getKey();
  while(true){
    if(ch=='*') break;
    limitString=limitString+String(ch);
    lcd.setCursor(0,1);
    lcd.print(limitString);
    ch=cusKeypad.getKey();
  }
  limitVal=limitString.toFloat();
  valueResetOptionInfoDisplayed=false;
  energy=0;
  powerLimitNotification=true;
  return limitVal;
}

int getThresholdFromUser(){
  float thresholdVal=0.0;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("#=>Enter Trheshold");
  String thresholdString="";
  char ch=cusKeypad.getKey();
  while(true){
    if(ch=='#') break;
    thresholdString=thresholdString+String(ch);
    lcd.setCursor(0,1);
    lcd.print(thresholdString);
    ch=cusKeypad.getKey();
  }
  thresholdVal=thresholdString.toFloat();
  valueResetOptionInfoDisplayed=false;
  return thresholdVal;
}


void SendMessage(String message){
  Serial3.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
  delay(10); 
  Serial3.println("AT+CMGS=\"+88001700112233\"\r"); // Replace x with mobile number
  delay(10);
  Serial3.println(message);// The SMS text you want to send
  delay(10);
  Serial3.println((char)26);// ASCII code of CTRL+Z
  delay(10);
}
