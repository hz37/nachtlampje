// Bijna okay versie van nachtlamp.
// Schrijft allemaal debugging strings.
// H.Zimmerman, 26-12-2025.

#include <M5Unified.h>
#include <RadioLib.h>

#define LORA_MOSI_PIN 21
#define LORA_MISO_PIN 22
#define LORA_SCK_PIN 20
#define LORA_IRQ_PIN 15
#define LORA_CS_PIN 23
#define LORA_BUSY_PIN 19

#define TIMEOUT 5

// ONE OR THE OTHER!
#define FIORE
//#define HENS

#ifdef FIORE
const String receiveToken = "Hoi!";
const String sendToken = "Hai!";
#else
const String receiveToken = "Hai!";
const String sendToken = "Hoi!";
#endif

// SX1262: NSS, DIO1, NRST, BUSY
SX1262 radio = new Module(LORA_CS_PIN, LORA_IRQ_PIN, RADIOLIB_NC, LORA_BUSY_PIN);

// flag to indicate that a packet was received
volatile bool packetReceived = false;
volatile int receivedFlag = -1;

// this function is called when a complete packet is received by the module
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif

void setFlag(void) 
{
  // we got a packet, set the flag
  packetReceived = true;
}

M5Canvas canvas(&M5.Display);

void setup() 
{
  M5.begin();
  M5.Display.setRotation(1);

  canvas.createSprite(M5.Display.width(), M5.Display.height());
  canvas.setTextColor(YELLOW);
  canvas.setTextScroll(true);

  // LED_BUILTIN at E1.P7
  auto& ioe = M5.getIOExpander(0);

  // LORA_RESET
  ioe.digitalWrite(7, false);
  delay(100);
  ioe.digitalWrite(7, true);
  delay(100);

  ioe.digitalWrite(5, true);  // LORA_LNA_ENABLE
  ioe.digitalWrite(6, true);  // LORA_ANTENNA_SWITCH

  // Initialize radio once at startup
  int state = radio.begin(868.0, 125.0f, 12, 5, 0x34, 22, 20, 3.0, true);
  if (state == RADIOLIB_ERR_NONE) 
  {
    canvas.println("[SX1262] Init success!");
  } 
  else 
  {
    canvas.print("Init failed, code: ");
    canvas.println(state);
    canvas.pushSprite(0, 0);
    while (true) { delay(10); }
  }

  // Set the function that will be called when new packet is received
  radio.setPacketReceivedAction(setFlag);

  // Start listening for LoRa packets
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) 
  {
    canvas.println("[SX1262] Receive mode started!");
  } 
  else 
  {
    canvas.println("[SX1262] Failed to start receive!");
    canvas.pushSprite(0, 0);
    while (true) { delay(10); }
  }
  
  canvas.pushSprite(0, 0);
}

void SendToken()
{
  canvas.println("Sending token...");
  canvas.pushSprite(0, 0);
  
  // Use blocking transmit to ensure message is sent
  String s = sendToken;
  int state = radio.transmit(s);

  if (state == RADIOLIB_ERR_NONE) 
  {
    canvas.println("Sent successfully!");
  } 
  else 
  {
    canvas.print("Send error: ");
    canvas.println(state);
  }
  canvas.pushSprite(0, 0);

  // Return to receive mode
  state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) 
  {
    canvas.print("Restart RX error: ");
    canvas.println(state);
    canvas.pushSprite(0, 0);
  }
}

void loop() 
{
  M5.update();

  // Check if button A was pressed
  if (M5.BtnA.wasPressed()) 
  {
    SendToken();
  }

  // Check if packet was received
  if (packetReceived) 
  {
    packetReceived = false;
    
    // Read the received packet
    String str;
    int state = radio.readData(str);
    
    if (state == RADIOLIB_ERR_NONE) 
    {
      if (str == receiveToken)
      {
        canvas.println("Received: " + str);
        receivedFlag = TIMEOUT;
      }
    } 
    else 
    {
      canvas.print("Read error: ");
      canvas.println(state);
    }
    canvas.pushSprite(0, 0);
    
    // Return to receive mode
    radio.startReceive();
  }

  // Display status
  String s;
  if (receivedFlag > 0)
  {
    canvas.setTextColor(GREEN);
    s = "De ander is wakker! " + String(receivedFlag);
    --receivedFlag;
  }
  else
  {
    canvas.setTextColor(RED);
    s = "De ander slaapt (weer)...";
  }

  canvas.println(s);
  canvas.pushSprite(0, 0); 

  delay(1000);
}
