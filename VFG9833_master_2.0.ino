#include <SPI.h>
#include <MD_cmdProcessor.h>
#include <MD_AD9833.h>
#include <EEPROM.h>
//
// Sketch built by michael@art-of-electronics.blog
// Take a look at the GitHub-repositories of MajicDesigns. He is doing good work!
//
// Anyone may use it free of charge - for more projects see art-of-electronics.blog
//
// Commands are issued via the serial monitor to effect changes in the AD9833
// hardware using the library functions.
//
// Dependencies:
// MD_cmdProcessor is available at https://github.com/MajicDesigns/MD_AD9833
//
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

// Pins for SPI comm with the AD9833
const uint8_t PIN_DATA = 11;  ///< SPI Data pin number
const uint8_t PIN_CLK = 13;	  ///< SPI Clock pin number
const uint8_t PIN_FSYNC = 10;	///< SPI Load pin number (FSYNC in AD9833 usage)
const uint8_t PIN_CS = 8; //Chip select for electronic potentiometer
uint32_t masterclock; // masterclock frequency; default is 25 MHz
long stp_time; float stp; int amm;
int lp=1; float wiper; float vco_delta; float am=0; int wform=0;
float out=0; float outv=0; float vv=0; int ln=0;
const float factor = 4.800/1023; // measure your supply voltage exactly and change value  "5.000" if necessary

MD_AD9833	AD(PIN_FSYNC); // Hardware SPI
//MD_AD9833	AD(PIN_DATA, PIN_CLK, PIN_FSYNC); // Arbitrary SPI pins

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
  wform=0;

  Serial.print(F("\nOutput wave: ")); Serial.print(*param);
  switch (toupper(*param))
  {
  case 'O': mode = MD_AD9833::MODE_OFF;    break;
  case 'S': mode = MD_AD9833::MODE_SINE;   break;
  case 'T': mode = MD_AD9833::MODE_TRIANGLE;  break;
  case 'Q': mode = MD_AD9833::MODE_SQUARE1; wform=1; break;
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

void handlerMC(char* param) 
{
  masterclock = strtoul(param, nullptr, 10);
  EEPROM.put(0, masterclock); 
  Serial.print(F("\nSet master clock to "));Serial.print(masterclock); Serial.print(" [Hz] \nNew default value has been stored.");Serial.println("\n### PRESS RESET TO APPLY ###");
}

void handlerSW(char* xxx) 
{ 
  String param = xxx; 

  long colon1 = param.indexOf(':'); // Index first colon
  long colon2 = param.indexOf(':', colon1 + 1); // Index second colon
  long colon3 = param.indexOf(':', colon2 + 1); // Index third colon
  long colon4 = param.indexOf(':', colon3 + 1); // Index fourth colon
  String fstart_str = param.substring(0, colon1); // find 1rst value
  String fstop_str = param.substring(colon1 + 1, colon2); // find 2nd value
  String step_str = param.substring(colon2 + 1, colon3); // find step value
  String step_time_str = param.substring(colon3 + 1); // find step_time value
  String mode_str = param.substring(colon4 + 1); // find step_time 

  long fstart = fstart_str.toInt(); // den ersten Wert als Integer konvertieren
  long fstop = fstop_str.toInt(); // den zweiten Wert als Integer konvertieren
  long step = step_str.toInt(); // den dritten Wert als Integer konvertieren
  long step_time = step_time_str.toInt(); // den vierten Wert als Integer konvertieren
  int mode = mode_str.toInt(); // den fünften Wert als Integer konvertieren

  Serial.println("Sweep is done with these parameters:");
  Serial.print("\nStart frequency [Hz] <fstart>: ");
  Serial.println(fstart);
  Serial.print("Stop frequency   [Hz] <fstop>: ");
  Serial.println(fstop);
  Serial.print("In steps of [Hz] <step>: ");
  Serial.println(step);
  Serial.print("Waiting between steps for [µs] <step_time>: ");
  Serial.println(step_time);
  Serial.print("Marker is set to: ");Serial.print(ln);Serial.println(" steps");
  if (mode == 0) {
    Serial.println("single sweep");
    }
  else {
    Serial.println("continous sweep");
  }
    if (ln > 0){
    Serial.print("\nAfter: ");Serial.print(ln);Serial.print(" steps, there will be a marker");
    }
   do {
    float f = fstart; //set start 
    float ffstop = fstop; //set stop 
    stp = step; 
    stp_time = step_time; 
    digitalWrite(2,LOW); delay(1); digitalWrite(2,HIGH); // one trigger per sweep at D2
  
    if (fstart < fstop) {
      int csteps = 1;
      for (f;f <= ffstop; f = f + stp) {
      //Serial.print(F("\nFreq 1: ")); Serial.print(f); Serial.print("cstep: "); Serial.print(csteps);// for debugging 
      AD.setFrequency(MD_AD9833::CHAN_0, f);
      delayMicroseconds(stp_time);
        if (csteps == ln) {
          digitalWrite(5, HIGH); delayMicroseconds(10); digitalWrite(5, LOW);
          csteps = 0;
        }
      csteps++; 
      }
    }
    else {
      int csteps = 1;
      for (f;f >= ffstop; f = f - stp) {
      AD.setFrequency(MD_AD9833::CHAN_0, f);
      delayMicroseconds(stp_time);
        if (csteps == ln) {
          digitalWrite(5, HIGH); delayMicroseconds(10); digitalWrite(5, LOW);
          csteps = 0;
        }
      csteps++; 
      }
    }
     
  }while (mode == 1);
}

void handlerAM(char* param) 
{ 
  float am = strtod(param, nullptr);
  amm = int(am);
  Serial.print(F("\nAmplitude: ")); Serial.println(amm);
  digitalWrite(PIN_CS, LOW); 
  SPI.transfer(0x11); 
  SPI.transfer(amm); 
  digitalWrite(PIN_CS, HIGH);
}

void handlerVC(char* xxx) 
{ 
  String param = xxx; 

  long colon1 = param.indexOf(':'); // find index
  long colon2 = param.indexOf(':', colon1 + 1); // find next index

  String fstart_str = param.substring(0, colon1); // find fstart 
  String fstop_str = param.substring(colon1 + 1, colon2); // find fstop

  long fstart = fstart_str.toInt(); 
  long fstop = fstop_str.toInt(); 

  float f = fstart; 
  float ffstop = fstop; 

  if (fstart > fstop) {
    ffstop = fstart; f = fstop; Serial.println("<fstart> was greater <fstop>. Swapped values.");
    }
  fstart=f;
  Serial.println("VCO using these paramters:");
  Serial.print("\nStart frequency [Hz] <fstart>: ");
  Serial.println(f);
  Serial.print("Stop frequency   [Hz] <fstop>: ");
  Serial.println(ffstop);
  Serial.println("Stop with RESET or power on/off.");

  do {
    wiper = analogRead(A0);
    vco_delta = (wiper/1024.00)*(ffstop-fstart);
    f = fstart + vco_delta; 
    AD.setFrequency(MD_AD9833::CHAN_0, f);
    delay(10);
  } while (lp ==1);
}

void handlerME(char* param) 
{ 
  measure();
  Serial.print("\nOutput voltage on Out 2 and Out 600 Ohm: ");Serial.print(outv); Serial.println(" [Vpp]");
}

void handlerAS(char* param) 
{ 
  float vv = strtod(param, nullptr); // Get preset value

    if (vv <= 3.5) { // maximum value that is possible
      Serial.print(F("\nSetting amplitude on output 2 to: ")); Serial.print(vv); Serial.println(" [Vpp]");
    }
    else {
      Serial.println(F("\nThis amplitude value for output 2 is > 3.5 Vpp. Choose lower value. ")); 
    }
    int amm=round(vv*1000/14); // 14 is the inclination of voltage / pot setting line
    
    digitalWrite(PIN_CS, LOW); 
    SPI.transfer(0x11); 
    SPI.transfer(amm); 
    digitalWrite(PIN_CS, HIGH);
    
    Serial.print(F("\nVoltage was set to: ")); Serial.print(vv); Serial.println(" [Vpp]"); Serial.print("amplitude set to: ");Serial.println(amm);  
}

void handlerCA(char* param){
  amm=0;
  while (amm<=250){
    digitalWrite(PIN_CS, LOW); 
    SPI.transfer(0x11); 
    SPI.transfer(amm); 
    digitalWrite(PIN_CS, HIGH);
    measure();
    if (amm>0){
      Serial.print("Amplitude set to: ");Serial.print(amm); Serial.print("  Voltage measured: ");Serial.println(outv);
    }
    amm=amm+10;
  }
}

void handlerMA(char* param) 
{
  ln = strtod(param, nullptr);
  Serial.print("\nSet a marker after ");Serial.print(ln); Serial.println(" steps");
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
  { "or", handlerR,     "r", "Reset AD9833 registers (hold if r=1)", 3 },
  { "mc", handlerMC,    "c", "Set AD9833 to masterclock frequency [Hz]"},
  { "sw", handlerSW,    "s", "Sweep Parameter fstart:fstop:step:step_time:mode", 4},
  { "ma", handlerMA,    "n", "After how many steps do you want a marker: ", 4},
  { "am", handlerAM,    "a", "Amplitude (0-255)", 4},
  { "as", handlerAS,    "v", "Amplitude (0-3.5) [Vpp]", 4},
  { "vc", handlerVC,    "s", "VCO fstart:fstop | fstop > start", 4},
  { "me", handlerME,    "",  "Measure output signal OUT 2 [Vpp]", 5},
  { "ca", handlerCA,    "",  "Calibration support \n", 5},
};

MD_cmdProcessor CP(Serial, cmdTable, ARRAY_SIZE(cmdTable));

void handlerHelp(char* param)
{
  Serial.print(F("\nEnsure line ending is set to newline.\n"));
  CP.help();
  Serial.print(F("\n"));
}

void measure() 
{
  out=0; outv=0;
  for (int i = 1; i < 6; i++) {
    out = factor*analogRead(1); // voltage at rectifier opamp
    outv=outv+out;
    delay(150); 
  }
  outv=(outv/5); // mean value of 5 measurements
}

void setup()
{
  Serial.begin(115200);
  
  AD.begin();
  EEPROM.get(0, masterclock);
  if (masterclock > 0) {   
    AD.setClk(masterclock);
  }
  Serial.print("\nMasterclock is set to ");Serial.print(masterclock); Serial.println(" MHz");
  CP.begin();
  CP.help();

  pinMode(2, OUTPUT); digitalWrite(2, HIGH); //Trigger is HIGH>LOW and back
  pinMode(5, OUTPUT); digitalWrite(5, LOW); // marker
  pinMode(PIN_CS, OUTPUT); digitalWrite(PIN_CS, HIGH); //CS for MCP potentiometer
  SPI.begin();
  
  amm=155; outv=0; out=0; ln=0;

}

void loop()
{
  CP.run();
  
}