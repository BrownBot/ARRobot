
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>

#include "credentials.h"

#define SERVOMIN 110  //
#define SERVOMAX 530  //
#define SLOW 500       //
#define FAST 100       //
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates

const char *ssid = networkSSID;
const char *password = networkPASSWORD;
const char *mqttServer = mqttSERVER;
const char *mqttUsername = mqttUSERNAME;
const char *mqttPassword = mqttPASSWORD;

unsigned long previousMillis = 0; 

String hostName = "brownbot.dyndns.org";
int pingResult;

char subTopic[] = "robot0";  //payload[0] will control/set LED
char pubTopic[] = "tempcur"; //payload[0] will have ledState value

char activeCommand = 'H';
char readCommand = 'H';
int phase = 0;

int legTargets[4][3];
int legCurrent[4][3];
int legDeltas[4][3]; 

bool legDir[4] = { 1, 0, 1, 0 };

WiFiClient wifiClient;
PubSubClient client(wifiClient);

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

void setup_wifi()
{
  delay(10);

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  WiFi.setDNS(dns);
  Serial.print("Dns configured.");

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Pinging ");
  Serial.print(hostName);
  Serial.print(": ");
  pingResult = WiFi.ping(hostName);

  if (pingResult >= 0)
  {
    Serial.print("SUCCESS! RTT = ");
    Serial.print(pingResult);
    Serial.println(" ms");
  }
  else
  {
    Serial.print("FAILED! Error code: ");
    Serial.println(pingResult);
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  readCommand = (char)payload[0];

  if(readCommand == 'X')
  {
    for(i = 0; i < 4; i++)
    {
      int offset = 1 + i * 3;
      moveleg(i+1, (int)payload[offset], (int)payload[offset + 1], (int)payload[offset + 2]);
    }
  }
  //   // Switch on the LED if 1 was received as first character
  //   if ((char)payload[0] == 'T')
  //   {
  //     digitalWrite(LED_PIN, HIGH);
  //     servo.write(180);
  //   }
  //   else
  //   {
  //    servo.write(20);
  //     digitalWrite(LED_PIN, LOW);
  //   }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ArduinoClient-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqttUsername, mqttPassword))
    {
      Serial.println("connected");
      // ... and resubscribe
      client.subscribe(subTopic);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void servocontrol(int n, int a) // n= servo number a= angle
{
  a = map(a, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(n, 0, a);
}

void updateLeg(int leg)
{
    for(int j = 0; j < 3; j++)
    {
      int n = leg * 3 + j;
      
      // if we've hit out target just set the servo to the value
      if(legDeltas[leg][j] == 0 || legCurrent[leg][j] == legTargets[leg][j])
      {
        pwm.setPWM(n, 0, legTargets[leg][j]);
      }
      else
      {
        //check for limit hits
        legCurrent[leg][j] += legDeltas[leg][j];
        if(legDeltas[leg][j] > 0)
        {
          if(legCurrent[leg][j] >= legTargets[leg][j])
          {
            legDeltas[leg][j] = 0;
            pwm.setPWM(n, 0, legTargets[leg][j]);
          }
          else
          {
            pwm.setPWM(n, 0, legCurrent[leg][j]);
          }
        }
        else
        {
          if(legCurrent[leg][j] <= legTargets[leg][j])
          {
            legDeltas[leg][j] = 0;
            pwm.setPWM(n, 0, legTargets[leg][j]);
          }
          else
          {
            pwm.setPWM(n, 0, legCurrent[leg][j]);
          }
        }
      }
    }
}

int mapAngle(int a, bool reverse)
{
  if(reverse)
  {
    return map(180 - a, 0, 180, SERVOMIN, SERVOMAX);
  }
  return map(a, 0, 180, SERVOMIN, SERVOMAX);
}

int calcDelta(int curr, int target)
{
  if(curr == target) return 0;
  int d = target - curr;
  //int s = d / 10;
  if(d > 0)
  {
    return 1;
  }
  else
  {
    return -1;
  }
}

void moveleg(int n, int an, int f, int t)
{
  int leg = n - 1;
  bool dir = legDir[leg];

  if (t >= 0)
  {
    legTargets[leg][2] = mapAngle(t, dir);
    legDeltas[leg][2] = calcDelta(legCurrent[leg][2], legTargets[leg][2]);
    //servocontrol(3 * (n - 1) + 2, t);
  };
  
  if (f >= 0)
  {
    legTargets[leg][1] = mapAngle(f, dir);
    legDeltas[leg][1] = calcDelta(legCurrent[leg][1], legTargets[leg][1]);
    //servocontrol(3 * (n - 1) + 1, f);
  };

  if (an >= 0)
  {
    legTargets[leg][0] = mapAngle(an, dir);
    legDeltas[leg][0] = calcDelta(legCurrent[leg][0], legTargets[leg][0]);
    //servocontrol(3 * (n - 1), an);
  };

}



void stp(int n, int a, int w)
{
  yield();

  servocontrol(3 * (n - 1) + 1, 160);
  delay(w);
  servocontrol(3 * (n - 1) + 2, 40);
  delay(w);
  servocontrol(3 * (n - 1), a);
  delay(w);
  servocontrol(3 * (n - 1) + 2, 90);
  delay(w);
  servocontrol(3 * (n - 1) + 1, 90);
}

void mov(int n, int a, int w)
{
  moveleg(n, a, -1, -1);
}

void printLegs()
{
  for(int i = 0; i < 4; i++)
  {
    Serial.print("Leg: ");
    Serial.print("(");
    Serial.print(legCurrent[i][0]);
    Serial.print(",");
    Serial.print(legTargets[i][0]);
    Serial.print(",");
    Serial.print(legDeltas[i][0]);
    Serial.print(")");
  }
  Serial.println("");
  
}

void iniz()
{
  for(int i = 0; i < 4; i++)
  {
    for(int j = 0; j < 3; j++)
    {
      legCurrent[i][j] = mapAngle(90,legDir[i]);
      pwm.setPWM(i * 3 + j, 0, 300);
    }
  }
  moveleg(1, 90, 90, 90);
  moveleg(2, 90, 90, 90);
  moveleg(3, 90, 90, 90);
  moveleg(4, 90, 90, 90);
}

void rest()
{
  moveleg(1, 50, 170, 150);
  moveleg(2, 50, 170, 150);
  moveleg(3, 50, 170, 150);
  moveleg(4, 50, 180, 120);
}

void stand()
{
  moveleg(1, 50, 50, 50);
  moveleg(2, 50, 50, 50);
  moveleg(3, 50, 50, 50);
  moveleg(4, 50, 50, 50);
}

void squat()
{
  moveleg(1, -1, 130, 100);
  moveleg(2, -1, 130, 100);
  moveleg(3, -1, 130, 100);
  moveleg(4, -1, 130, 100);
}

void wave_r()
{
  for (int j = 0; j < 3; j++)
  {
    moveleg(4, -1, 160, 0);
    for (int i = 30; i <= 110; i++)
    {
      moveleg(4, i, -1, -1);
    }
    for (int i = 110; i >= 20; i--)
    {
      moveleg(4, i, -1, -1);
    }
  }
  moveleg(4, 90, 90, 90);
}

void wave_l()
{
  for (int j = 0; j < 3; j++)
  {
    moveleg(1, -1, 160, 0);
    for (int i = 30; i <= 110; i++)
    {
      moveleg(1, i, -1, -1);
    }
    for (int i = 110; i >= 20; i--)
    {
      moveleg(1, i, -1, -1);
    }
  }
  moveleg(1, 90, 90, 90);
}

void skew_r()
{
  moveleg(1, 20, -1, -1);
  moveleg(2, 20, -1, -1);
  moveleg(3, 20, -1, -1);
  moveleg(4, 160, -1, -1);
}

void skew_l()
{
  moveleg(1, 160, -1, -1);
  moveleg(2, 160, -1, -1);
  moveleg(3, 160, -1, -1);
  moveleg(4, 20, -1, -1);
}

void courtsy()
{
  moveleg(1, -1, 140, 80);
  moveleg(2, -1, -1, 80);
  moveleg(3, -1, 40, 80);
  moveleg(4, -1, 40, 80);
}

void prepare_jump()
{
  moveleg(1, -1, 40, 80);
  moveleg(2, -1, -1, 80);
  moveleg(3, -1, 140, 80);
  moveleg(4, -1, 140, 80);
}

void rotate_r()
{
  char dat = 'X';
  while (dat != 'E')
  {
    dat = activeCommand;
    stp(1, 160, SLOW);
    stp(3, 160, SLOW);
    stp(2, 160, SLOW);
    stp(4, 20, SLOW);
    //  stp(6,20, SLOW);
    delay(50);
    //  mov(6,90, FAST);
    //  mov(5,90, FAST);
    mov(4, 90, FAST);
    mov(3, 90, FAST);
    mov(2, 90, FAST);
    mov(1, 90, FAST);
  }
}

void rotate_l(int phase)
{
  int pha = phase % 8;
  switch (pha)
  {
    case 0:
    {
      moveleg(1, 50, -1, -1);
      moveleg(2, 50, -1, -1);
      moveleg(3, 50, -1, -1);
      moveleg(4, 50, -1, -1);
      break;
    }
    case 1:
    {
      moveleg(1, 30, -1, -1);
      moveleg(2, 70, -1, -1);
      moveleg(3, 30, -1, -1);
      moveleg(4, 70, -1, -1);
      break;
    }
    case 2:
    {
      moveleg(1, 10, 140, 140);
      moveleg(2, 90, 100, 100);
      moveleg(3, 10, 140, 140);
      moveleg(4, 90, 100, 100);
      break;
    }
    case 3:
    {
      moveleg(1, 30, -1, -1);
      moveleg(2, 70, -1, -1);
      moveleg(3, 30, -1, -1);
      moveleg(4, 70, -1, -1);
      break;
    }
    case 4:
    {
      moveleg(1, 50, -1, -1);
      moveleg(2, 50, -1, -1);
      moveleg(3, 50, -1, -1);
      moveleg(4, 50, -1, -1);
      break;
    }
    case 5:
    {
      moveleg(1, 70, -1, -1);
      moveleg(2, 30, -1, -1);
      moveleg(3, 70, -1, -1);
      moveleg(4, 30, -1, -1);
      break;
    }
    case 6:
    {
      moveleg(1, 90, 120, 120);
      moveleg(2, 10, 120, 120);
      moveleg(3, 90, 120, 120);
      moveleg(4, 10, 120, 120);
      break;
    }
    case 7:
    {
      moveleg(1, 70, -1, -1);
      moveleg(2, 30, -1, -1);
      moveleg(3, 70, -1, -1);
      moveleg(4, 30, -1, -1);
      break;
    }
  }
}

void forward(int phase)
{
  int pha = phase % 4;
  switch (pha)
  {
    case 0:
    {
      moveleg(1, 50, 90, 90);
      moveleg(2, 50, 90, 90);
      moveleg(3, 50, 90, 90);
      moveleg(4, 50, 90, 90);
      break;
    }
    case 1:
    {
      moveleg(1, 50, 90, 90);
      moveleg(2, 50, 90, 90);
      moveleg(3, 50, 90, 90);
      moveleg(4, 50, 120, 90);
      break;
    }
    case 2:
    {
      moveleg(1, 50, 90, 90);
      moveleg(2, 50, 90, 90);
      moveleg(3, 50, 90, 90);
      moveleg(4, 30, 120, 90);
      break;
    }
    case 3:
    {
      moveleg(1, 50, 90, 90);
      moveleg(2, 50, 90, 90);
      moveleg(3, 50, 90, 90);
      moveleg(4, 30, 90, 90);
      break;
    }
  }
}

void backward(int phase)
{
  yield();
  char dat = 'X';
  while (dat != 'E')
  {
    dat = activeCommand;
    mov(3, 140, FAST);
    mov(4, 140, FAST);
    //  mov(5,140, FAST);
    mov(2, 140, FAST);
    mov(1, 140, FAST);
    //  mov(6,140, FAST);
    stp(4, 40, SLOW);
    stp(3, 40, SLOW);
    stp(2, 40, SLOW);
    //  stp(5,40, SLOW);
    //  stp(6,40, SLOW);
    stp(1, 40, SLOW);
  }
  delay(SLOW);
  iniz();
}

void setup()
{
  Serial.begin(9600);

  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);

  delay(50);
  
  setup_wifi();
  client.setServer(mqttServer, 1883);
  client.setCallback(callback);

  
  iniz();
  printLegs();
  delay(1000);
  //stand();
  printLegs();

  
//  delay(1000);
//  iniz();

//  pwm.setPWM(0, 0, 3000);
//  pwm.setPWM(0  , 0, 1000);

}

void loop()
{
  if (!client.connected()) 
  {
    reconnect();
  }

  updateLeg(0);
  updateLeg(1);
  updateLeg(2);
  updateLeg(3);

  // non blocking timer
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= 1000) 
  {
    
    if(activeCommand != readCommand)
    {
      activeCommand = readCommand;
      phase = 0;
    }
    
    // save the last time
    previousMillis = currentMillis;

    switch (activeCommand)
    {
      case 'S':
      {
        //stand();
        rest();
        break;
      }
      case 'H':
      {
        rest();
        break;
      }
      case 'Q':
      {
        squat();
        break;
      }
      case 'F':
      {
        rotate_l(phase);
        //forward(phase);
        break;
      }
      case 'B':
      {
        backward(phase);
        break;
      }
      case 'C':
      {
        courtsy();
        break;
      }
      case 'J':
      {
        prepare_jump();
        break;
      }
      case '>':
      {
        skew_r();
        break;
      }
      case '<':
      {
        skew_l();
        break;
      }
      case 'L':
      {
        rotate_l(phase);
        break;
      }
      case 'R': //
      {
        rotate_r();
        break;
      }
    }
    phase++;
    printLegs();
  }
  
  
  client.loop();
  //     }
}
