#include <SPI.h>
#include <MD_cmdProcessor.h>
#include <MD_AD9833.h>
//
// Test sketch for the functions of the MD_AD9833 library.
//
// Commands are issued via the serial monitor to effect changes in the AD9833
// hardware using the library functions.
//
// Dependencies:
// MD_cmdProcessor is available at https://github.com/MajicDesigns/MD_AD9833
//
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

// Pins for SPI comm with the AD9833 IC
const uint8_t PIN_DATA = 11;  ///< SPI Data pin number
const uint8_t PIN_CLK = 13;	  ///< SPI Clock pin number
const uint8_t PIN_FSYNC = 10;	///< SPI Load pin number (FSYNC in AD9833 usage)
const uint8_t PIN_CS = 9; //Chip Select für das Poti
byte address = 0x11; //Adresse des Poti
long stp_time; float stp; int amm;
int lp=1; float wiper; float vco_delta;

MD_AD9833	AD(PIN_FSYNC); // Hardware SPI
//MD_AD9833	AD(PIN_DATA, PIN_CLK, PIN_FSYNC); // Arbitrary SPI pins

// handler functions
void handlerHelp(char* param);

void handlerF1(char* param) 
{
  float  f = strtod(param, nullptr);

  Serial.print(F("\nFreq 1: ")); Serial.print(f);
  AD.setFrequency(MD_AD9833::CHAN_0, f);
}

void handlerF2(char* param) 
{
  float  f = strtod(param, nullptr);

  Serial.print(F("\nFreq 2: ")); Serial.print(f);
  AD.setFrequency(MD_AD9833::CHAN_1, f);
}

void handlerP1(char* param) 
{
  uint32_t  ul = strtoul(param, nullptr, 10);

  Serial.print(F("\nPhase 1: ")); Serial.print(ul / 10); Serial.print(F(".")); Serial.print(ul % 10);
  AD.setPhase(MD_AD9833::CHAN_0, (uint16_t)ul);
}

void handlerP2(char* param) 
{
  uint32_t  ul = strtoul(param, nullptr, 10);

  Serial.print(F("\nPhase 2: ")); Serial.print(ul / 10); Serial.print(F(".")); Serial.print(ul % 10);
  AD.setPhase(MD_AD9833::CHAN_1, (uint16_t)ul);
}

void handlerOF(char* param) 
{
  MD_AD9833::channel_t chan = AD.getActiveFrequency();

  Serial.print(F("\nFreq source: ")); Serial.print(*param);
  switch (*param)
  {
  case '1': chan = MD_AD9833::CHAN_0; break;
  case '2': chan = MD_AD9833::CHAN_1; break;
  default: Serial.print(F(" error"));
  }
  AD.setActiveFrequency(chan);
}

void handlerOP(char* param) 
{
  MD_AD9833::channel_t chan = AD.getActivePhase();

  Serial.print(F("\nPhase source: ")); Serial.print(*param);
  switch (*param)
  {
  case '1': chan = MD_AD9833::CHAN_0; break;
  case '2': chan = MD_AD9833::CHAN_1; break;
  default: Serial.print(F(" error"));
  }
  AD.setActivePhase(chan);
}

void handlerOW(char* param) 
{
  MD_AD9833::mode_t mode = AD.getMode();

  Serial.print(F("\nOutput wave: ")); Serial.print(*param);
  switch (toupper(*param))
  {
  case 'O': mode = MD_AD9833::MODE_OFF;    break;
  case 'S': mode = MD_AD9833::MODE_SINE;   break;
  case 'T': mode = MD_AD9833::MODE_TRIANGLE;  break;
  case 'Q': mode = MD_AD9833::MODE_SQUARE1;  break;
  default: Serial.print(F(" error"));
  }
  AD.setMode(mode);
}

void handlerR(char* param) 
{ 
  Serial.print(F("\nReset"));
  if (*param == '1') Serial.print(F(" hold"));
  AD.reset(*param == '1');
}

void handlerSW(char* xxx) 
{ 
  String param = xxx; 

  long colon1 = param.indexOf(':'); // Index des ersten Doppelpunkts
  long colon2 = param.indexOf(':', colon1 + 1); // Index des zweiten Doppelpunkts
  long colon3 = param.indexOf(':', colon2 + 1); // Index des dritten Doppelpunkts
  long colon4 = param.indexOf(':', colon3 + 1); // Index des dritten Doppelpunkts

  String fstart_str = param.substring(0, colon1); // den ersten Wert fstart extrahieren
  String fstop_str = param.substring(colon1 + 1, colon2); // den zweiten Wert fstop extrahieren
  String step_str = param.substring(colon2 + 1, colon3); // den dritten Wert step extrahieren
  String step_time_str = param.substring(colon3 + 1); // den vierten Wert step_time extrahieren
  String mode_str = param.substring(colon4 + 1); // den vierten Wert step_time extrahieren

  long fstart = fstart_str.toInt(); // den ersten Wert als Integer konvertieren
  long fstop = fstop_str.toInt(); // den zweiten Wert als Integer konvertieren
  long step = step_str.toInt(); // den dritten Wert als Integer konvertieren
  long step_time = step_time_str.toInt(); // den vierten Wert als Integer konvertieren
  int mode = mode_str.toInt(); // den fünften Wert als Integer konvertieren

  Serial.println("Der Sweep erfolgt mit diesen Parametern:");
  Serial.print("\nStartfrequenz [Hz] <fstart>: ");
  Serial.println(fstart);
  Serial.print("Endfrequenz   [Hz] <fstop>: ");
  Serial.println(fstop);
  Serial.print("In Schritten von [Hz] <step>: ");
  Serial.println(step);
  Serial.print("Wartet zwischen den Schritten [µs] <step_time>: ");
  Serial.println(step_time);
  if (mode == 0) {
    Serial.println("einmaliger Sweep");
    }
  else {
    Serial.print("kontinuierlicher Sweep");
  }
  do {
    float f = fstart; //Startpunkt setzen
    float ffstop = fstop; //Stop setzen
    stp = step; 
    stp_time = step_time; 
    digitalWrite(2,LOW); delay(1); digitalWrite(2,HIGH); //Triggerimpuls einmal pro Sweep
  
    if (fstart < fstop) {
      for (f;f <= ffstop; f = f + stp) {
      //Serial.print(F("\nFreq 1: ")); Serial.print(f); // Für Debugging 
      AD.setFrequency(MD_AD9833::CHAN_0, f);
      delayMicroseconds(stp_time);
      }
    }
    else {
      for (f;f >= ffstop; f = f - stp) {
      AD.setFrequency(MD_AD9833::CHAN_0, f);
      delayMicroseconds(stp_time);
      }
    }
  }while (mode == 1);
}
void handlerAM(char* param) 
{ 
  float  am = strtod(param, nullptr);
  amm = int(am);
  Serial.print(F("\nAmplitude: ")); Serial.println(amm);
  digitalPotWrite(amm);
}

void handlerVC(char* xxx) 
{ 
  String param = xxx; 

  long colon1 = param.indexOf(':'); // Index des ersten Doppelpunkts
  long colon2 = param.indexOf(':', colon1 + 1); // Index des zweiten Doppelpunkts

  String fstart_str = param.substring(0, colon1); // den ersten Wert fstart extrahieren
  String fstop_str = param.substring(colon1 + 1, colon2); // den zweiten Wert fstop extrahieren

  long fstart = fstart_str.toInt(); // den ersten Wert als Integer konvertieren
  long fstop = fstop_str.toInt(); // den zweiten Wert als Integer konvertieren

  float f = fstart; //Startpunkt setzen
  float ffstop = fstop; //Stop setzen

  if (fstart > fstop) {
    ffstop = fstart; f = fstop; Serial.println("<fstart> war größer <fstop>. Habe ich korrigiert.");
    }
  fstart=f;
  Serial.println("Der VCO nutzt diese Parameter:");
  Serial.print("\nStartfrequenz [Hz] <fstart>: ");
  Serial.println(f);
  Serial.print("Endfrequenz   [Hz] <fstop>: ");
  Serial.println(ffstop);
  Serial.println("Unterbrechnung mit der Reset-Taste");

  do {
    wiper = analogRead(14);
    vco_delta = (wiper/1024.00)*(ffstop-fstart);
    f = fstart + vco_delta; 
    AD.setFrequency(MD_AD9833::CHAN_0, f);
    //Serial.print("Poti auf:  ");Serial.print(wiper); Serial.print("  vco_delta: "); Serial.print(vco_delta); Serial.print("  f: "); Serial.println(f); //Debug Hilfe
    delay(10);
  } while (lp ==1);
}

const MD_cmdProcessor::cmdItem_t PROGMEM cmdTable[] =
{
  { "?",  handlerHelp,  "",    "Help", 0 },
  { "h",  handlerHelp,  "",    "Help", 0 },
  { "f1", handlerF1,    "f", "Frequency 1 to f Hz", 1 },
  { "f2", handlerF2,    "f", "Frequency 2 to f Hz", 1 },
  { "p1", handlerP1,    "p", "Phase 1 set p tenths degree (1201=120.1)", 2 },
  { "p2", handlerP2,    "p", "Phase 2 set p tenths degree (1201=120.1)", 2 },
  { "of", handlerOF,    "c", "Output Frequency source channel [c=1/2]", 3 },
  { "op", handlerOP,    "c", "Output Phase source channel [c=1/2]", 3 },
  { "ow", handlerOW,    "t", "Output Wave type [t=(o)ff/(s)ine/(t)ri/s(q)re]", 3 },
  { "or", handlerR,     "h", "Reset AD9833 registers (hold if h=1)", 3 },
  { "sw", handlerSW,    "s", "Sweep Parameter fstart:fstop:step:step_time:mode", 4},
  { "am", handlerAM,    "a", "Amplitude (0-255)", 4},
  { "vc", handlerVC,    "v", "VCO fstart:fstop | fstop > start", 4},
};

MD_cmdProcessor CP(Serial, cmdTable, ARRAY_SIZE(cmdTable));

void handlerHelp(char* param)
{
  Serial.print(F("\n\n[MD_AD9833 Tester]"));
  Serial.print(F("\nEnsure line ending set to newline.\n"));
  CP.help();
  Serial.print(F("\n"));
}

void digitalPotWrite(int value)
{
  digitalWrite(PIN_CS, LOW); 
  SPI.transfer(address); 
  SPI.transfer(value); 
  digitalWrite(PIN_CS, HIGH);
}

void setup()
{
  Serial.begin(57600);
  
  AD.begin();
  CP.begin();
  CP.help();


  pinMode(2, OUTPUT); //Trigger-Output
  digitalWrite(2, HIGH); //Triggerimpuls durch Übergang HIGH>LOW und zurück
  pinMode(PIN_CS, OUTPUT); //CS für das Poti
  SPI.begin();
  digitalWrite(PIN_CS, HIGH);
  digitalPotWrite(0x00);
}

void loop()
{
  CP.run();
  
}