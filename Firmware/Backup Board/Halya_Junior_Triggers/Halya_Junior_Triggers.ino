int SwitchPins[] = {12,41,42};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Start");
  for (int i:SwitchPins) {
    pinMode(i,OUTPUT);
  }
}

void loop() {
  // put your main code here, to run repeatedly:

  //Working SwitchPins code:
  // for (int i:SwitchPins) {
  //   digitalWrite(i, HIGH);
  //   delay(1000);
  //   digitalWrite(i, LOW);
  // }
  
}
