const int buttonPin1 = 22; 
const int buttonPin2 = 23;     

int buttonState1 = 0;         
int buttonState2 = 0; 

void setup() {
  // initialize the pushbutton pin as an input:
  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
//  Serial.print("Rakute");
  buttonState1 = digitalRead(buttonPin1);
  buttonState2 = digitalRead(buttonPin2);

  if(buttonState1 == HIGH){
    for(int i=0;i<1;++i){
      Serial.print("True  ");
    }   
  }

  if(buttonState2 ==HIGH){
    for(int i=0;i<1;++i){
      Serial.print("False ");
    }   
  }
  delay(200);
}
