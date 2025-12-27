// Nachtlamp kijkt of de ander wakker is.
// Bij controleren wordt een "ik ben wakker" token verstuurd via LoRa.
// H.Zimmerman, 27-12-2025.

#include <M5Unified.h>
#include <RadioLib.h>

// Arduino Nesso N1 specific pins.

#define LORA_MOSI_PIN 21
#define LORA_MISO_PIN 22
#define LORA_SCK_PIN 20
#define LORA_IRQ_PIN 15
#define LORA_CS_PIN 23
#define LORA_BUSY_PIN 19

// How many seconds elapse after button is pressed? The person who pressed the button
// may fall asleep again.
#define TIMEOUT 3600

// How many seconds to keep the message on the screen after hitting the button.
#define DISPLAYTIMEOUT 5

// undef/comment to switch compilations.
// There need to be two distinct versions for bi-directional communication.

// #define FIORE

// LORA tokens to broadcast.
#ifdef FIORE
const String receiveToken = "Hoi!";
const String sendToken = "Hai!";
#else
const String receiveToken = "Hai!";
const String sendToken = "Hoi!";
#endif

// SX1262: NSS, DIO1, NRST, BUSY.
SX1262 radio = new Module(LORA_CS_PIN, LORA_IRQ_PIN, RADIOLIB_NC, LORA_BUSY_PIN);

volatile bool packetReceived = false;

volatile int receivedCounter = -1; // seconds

volatile int displayTimeout = 0; // seconds

#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif

// This function is called when a complete packet is received by the module.

void setFlag(void) 
{
  // We got a packet, set the flag.
  packetReceived = true;
}

void setup() 
{
  M5.begin();
  M5.Display.setRotation(1);

  // LED_BUILTIN at E1.P7.
  auto& ioe = M5.getIOExpander(0);

  // LORA_RESET.
  ioe.digitalWrite(7, false);
  delay(100);
  ioe.digitalWrite(7, true);
  delay(100);

  ioe.digitalWrite(5, true);  // LORA_LNA_ENABLE
  ioe.digitalWrite(6, true);  // LORA_ANTENNA_SWITCH

  // Initialize radio once at startup.

  int state = radio.begin(868.0, 125.0f, 12, 5, 0x34, 22, 20, 3.0, true);

  // Set the function that will be called when new packet is received.

  radio.setPacketReceivedAction(setFlag);

  // Start listening for LoRa packets.

  radio.startReceive();
}

void SendToken()
{
  // Use blocking transmit to ensure message is sent.

  String s = sendToken;
  radio.transmit(s);
  radio.startReceive();
}

void loop() 
{
  M5.update();

  bool deAnderIsWakker = false;

  if (receivedCounter > 0)
  {
    --receivedCounter;
    deAnderIsWakker = true;
  }

  // Check if button A was pressed.

  if (M5.BtnA.wasPressed()) 
  {
    SendToken();

    displayTimeout = DISPLAYTIMEOUT;

    M5Canvas canvas(&M5.Display);

    #ifdef FIORE
    canvas.createSprite(10, 10);
    canvas.fillSprite(deAnderIsWakker ? TFT_DARKGREEN : TFT_BROWN);
    #else
    canvas.createSprite(M5.Display.width(), M5.Display.height());
    canvas.fillSprite(deAnderIsWakker ? TFT_GREEN : TFT_RED);
    #endif

    canvas.pushSprite(0, 0);
    canvas.deleteSprite();
  }

  if (displayTimeout == 0)
  {
    // Blank the screen.
    M5Canvas canvas(&M5.Display);

    canvas.createSprite(M5.Display.width(), M5.Display.height());
    canvas.fillSprite(TFT_BLACK);
    canvas.pushSprite(0, 0);
    canvas.deleteSprite();
  }

  if (displayTimeout >= 0)
  {
    --displayTimeout;
  }

  // Check if packet was received.

  if (packetReceived) 
  {
    packetReceived = false;
    
    // Read the received packet.

    String str;

    int state = radio.readData(str);
    
    if (state == RADIOLIB_ERR_NONE) 
    {
      if (str == receiveToken)
      {
        receivedCounter = TIMEOUT;
      }
    } 

    // Return to receive mode.

    radio.startReceive();
  }

  // Delay second.

  delay(1000);
}
