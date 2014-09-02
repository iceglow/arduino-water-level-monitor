#define FLEX_TOO_HI 475
#define FLEX_TOO_LOW 465
#define ONBOARD_LED 13
#define FLEX_SENSOR 0

int bend_value  = 0;
byte bend_state = 0;

void setup()
{
  // for serial window debugging
  Serial.begin(9600);
  // set pin for onboard led
  pinMode(ONBOARD_LED, OUTPUT);
}

void SendWaterAlert(int bend_value, int bend_state)
{
  digitalWrite(ONBOARD_LED, bend_state ? HIGH : LOW);
  if (bend_state)
    Serial.print("Water Level Threshold Exceeded, bend_value=");
  else
    Serial.print("Water Level Returned To Normal bend_value=");
  Serial.println(bend_value);
}

void loop()
{
  // wait a second each loop iteration
  delay(1000);
  // poll FLEX_SENSOR voltage
  bend_value = analogRead(FLEX_SENSOR);

  // print bend_value to the serial port for baseline measurement
  // comment this out once baseline, upper and lower threshold
  // limits have been defined
  Serial.print("bend_value=");
  Serial.println(bend_value);
	
  switch (bend_state)
  {
    case 0: // bend_value does not exceed high or low values
      if (bend_value >= FLEX_TOO_HI || bend_value <= FLEX_TOO_LOW)
      {
        bend_state = 1;
        SendWaterAlert(bend_value, bend_state);
      }
      break;
    case 1: // bend_value exceeds high or low values
      if (bend_value < FLEX_TOO_HI && bend_value > FLEX_TOO_LOW)
      {
        bend_state = 0;
        SendWaterAlert(bend_value, bend_state);
      }
      break;
  }
}
