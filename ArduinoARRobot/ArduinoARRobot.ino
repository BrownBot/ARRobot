
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>

#include "credentials.h"

#define SERVOMIN 180  //
#define SERVOMAX 420  //
#define SLOW 50       //
#define FAST 10       //
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates

const char *ssid = networkSSID;
const char *password = networkPASSWORD;
const char *mqttServer = mqttSERVER;
const char *mqttUsername = mqttUSERNAME;
const char *mqttPassword = mqttPASSWORD;

String hostName = "brownbot.dyndns.org";
int pingResult;

char subTopic[] = "robot0";  //payload[0] will control/set LED
char pubTopic[] = "tempcur"; //payload[0] will have ledState value

char activeCommand;

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

  activeCommand = (char)payload[0];
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

void moveleg(int n, int an, int f, int t, int w)
{
  yield();
  if (t >= 0)
  {
    servocontrol(3 * (n - 1) + 2, t);
  };
  delay(w);
  if (f >= 0)
  {
    servocontrol(3 * (n - 1) + 1, f);
  };
  delay(w);
  if (an >= 0)
  {
    servocontrol(3 * (n - 1), an);
  };
  delay(w);
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
  moveleg(n, a, -1, -1, w);
}

void iniz()
{
  moveleg(1, 90, 90, 90, FAST);
  moveleg(2, 90, 90, 90, FAST);
  moveleg(3, 90, 90, 90, FAST);
  moveleg(4, 90, 90, 90, FAST);
}

void stand()
{
  moveleg(1, -1, 50, 80, FAST);
  moveleg(2, -1, 50, 80, FAST);
  moveleg(3, -1, 50, 80, FAST);
  moveleg(4, -1, 35, 80, FAST);
}

void squat()
{
  moveleg(1, -1, 130, 100, FAST);
  moveleg(2, -1, 130, 100, FAST);
  moveleg(3, -1, 130, 100, FAST);
  moveleg(4, -1, 130, 100, FAST);
}

void wave_r()
{
  for (int j = 0; j < 3; j++)
  {
    moveleg(4, -1, 160, 0, SLOW);
    for (int i = 30; i <= 110; i++)
    {
      moveleg(4, i, -1, -1, FAST);
    }
    for (int i = 110; i >= 20; i--)
    {
      moveleg(4, i, -1, -1, FAST);
    }
  }
  moveleg(4, 90, 90, 90, SLOW);
}

void wave_l()
{
  for (int j = 0; j < 3; j++)
  {
    moveleg(1, -1, 160, 0, SLOW);
    for (int i = 30; i <= 110; i++)
    {
      moveleg(1, i, -1, -1, FAST);
    }
    for (int i = 110; i >= 20; i--)
    {
      moveleg(1, i, -1, -1, FAST);
    }
  }
  moveleg(1, 90, 90, 90, SLOW);
}

void skew_r()
{
  moveleg(1, 20, -1, -1, FAST);
  moveleg(2, 20, -1, -1, FAST);
  moveleg(3, 20, -1, -1, FAST);
  moveleg(4, 160, -1, -1, FAST);
}

void skew_l()
{
  moveleg(1, 160, -1, -1, FAST);
  moveleg(2, 160, -1, -1, FAST);
  moveleg(3, 160, -1, -1, FAST);
  moveleg(4, 20, -1, -1, FAST);
}

void courtsy()
{
  moveleg(1, -1, 140, 80, FAST);
  moveleg(2, -1, -1, 80, FAST);
  moveleg(3, -1, 40, 80, FAST);
  moveleg(4, -1, 40, 80, FAST);
}

void prepare_jump()
{
  moveleg(1, -1, 40, 80, FAST);
  moveleg(2, -1, -1, 80, FAST);
  moveleg(3, -1, 140, 80, FAST);
  moveleg(4, -1, 140, 80, FAST);
}

void rotate_r()
{
  char dat = 'X';
  while (dat != 'E')
  {
    dat = activeCommand;
    stp(1, 160, SLOW);
    stp(3, 160, SLOW);
    //  stp(5,20, SLOW);
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

void rotate_l()
{
  char dat = 'X';
  while (dat != 'E')
  {
    dat = activeCommand;
    stp(1, 20, SLOW);
    stp(3, 20, SLOW);
    //  stp(5,160, SLOW);
    stp(2, 20, SLOW);
    stp(4, 160, SLOW);
    //  stp(6,160, SLOW);
    delay(50);
    mov(1, 90, FAST);
    mov(2, 90, FAST);
    mov(3, 90, FAST);
    mov(4, 90, FAST);
    //    mov(5,90, FAST);
    //    mov(6,90, FAST);
  }
}

void forward()
{
  yield();
  char dat = 'X';
  while (dat != 'E')
  {
    dat = activeCommand;
    //  mov(6,40, FAST);
    mov(1, 40, FAST);
    mov(2, 40, FAST);
    //  mov(5,40, FAST);
    mov(4, 40, FAST);
    mov(3, 40, FAST);
    stp(1, 140, SLOW);
    //  stp(6,140, SLOW);
    //  stp(5,140, SLOW);
    stp(2, 140, SLOW);
    stp(3, 140, SLOW);
    stp(4, 140, SLOW);
  }
  delay(SLOW);
  iniz();
}

void backward()
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

  //iniz();
//  stand();
  iniz();

//  pwm.setPWM(0, 0, 3000);
//  pwm.setPWM(0  , 0, 1000);

}

void loop()
{
  if (!client.connected()) 
  {
    reconnect();
  }
  
  char olddato;
  //   while (bluetooth.available())
  //   {
  char dato = activeCommand; //bluetooth.read(); // read Bluetooth character

  switch (dato)
  {
    case 'A':
    {
      if (olddato == 'E')
        stand();
      else
        forward();
      olddato = dato;
      break;
    }
    case 'B':
    {
      if (olddato == 'E')
        squat();
      else
        backward();
      olddato = dato;
      break;
    }
    case 'C':
    {
      if (olddato == 'E')
        courtsy();
      else
        skew_r();
      olddato = dato;
      break;
    }
    case 'D':
    {
      if (olddato == 'E')
        prepare_jump();
      else
        skew_l();
      olddato = dato;
      break;
    }
    case 'G':
    {
      if (olddato == 'E')
        wave_l();
      else
        rotate_l();
      olddato = dato;
      break;
    }
    case 'F': //
    {
      if (olddato == 'E')
        wave_r();
      else
        rotate_r();
      olddato = dato;
      break;
    }
    case 'E':
    {
      if (olddato == 'E')
      {
        iniz();
        olddato = '#';
      }
      else
        olddato = dato;
      break;
    }
  }
  
  client.loop();
  //     }
}
