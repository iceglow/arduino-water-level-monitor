#define FLEX_LEVEL_WARNING 475
#define FLEX_LEVEL_ALERT 400
#define ONBOARD_LED 13
#define FLEX_SENSOR 0

#define STATE_NORMAL  0
#define STATE_WARNING 1
#define STATE_ALERT   2

byte bendState = STATE_NORMAL;

void setup()
{
  // for serial window debugging
  Serial.begin(9600);
  // set pin for onboard led
  pinMode(ONBOARD_LED, OUTPUT);
}

int getBendValue()
{
  // poll FLEX_SENSOR voltage
  return analogRead(FLEX_SENSOR);
}

void sendWaterNormal(int bendValue)
{
  digitalWrite(ONBOARD_LED, LOW);
  Serial.print("Water Level Returned To Normal bendValue=");
  Serial.println(bendValue);
}

void sendWaterWarning(int bendValue)
{
  digitalWrite(ONBOARD_LED, HIGH);
  Serial.print("Water Level Warning bendValue=");
  Serial.println(bendValue);
}

void sendWaterAlert(int bendValue)
{
  digitalWrite(ONBOARD_LED, HIGH);
  Serial.print("Water Level Alert bendValue=");
  Serial.println(bendValue);
}

byte getBendState(int bendValue, byte currentState)
{
  byte newState = currentState;

  switch (currentState)
  {
    case STATE_NORMAL: // bendValue does not exceed high or low values
      if (bendValue <= FLEX_LEVEL_WARNING)
      {
        newState = STATE_WARNING;
        sendWaterWarning(bendValue);
      }
      break;
    case STATE_WARNING: // bendValue exceeds high or low values
      if (bendValue <= FLEX_LEVEL_ALERT)
      {
        newState = STATE_ALERT;
        sendWaterAlert(bendValue);
      }
      else if (bendValue > FLEX_LEVEL_WARNING)
      {
        newState = STATE_NORMAL;
        SendWaterNormal(bendValue);
      }
      break;
    case STATE_ALERT:
      if (bendValue > FLEX_LEVEL_ALERT) {
        newState = STATE_WARNING;
        sendWaterWarning(bendValue);
      }
      break;
  }

  return newState;
}

void loop()
{
  // wait a second each loop iteration
  delay(1000);

  int bendValue = getBendValue();

  // print bend_value to the serial port for baseline measurement
  Serial.print("bend_value=");
  Serial.println(bendValue);

  bendState = getBendState(bendValue, bendState);
}
