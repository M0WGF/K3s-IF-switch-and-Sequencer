/* 
 *  Elecraft K3 / K3S two port PTT Sequencer and TVRX IF Switch with VSWR Trip.
 *  Version : 06.0
 *  Author  : Mark Horn - M0WGF - mhorn71@gmail.com
 *  Date    : 2019
 *  
 * PTT Note :  My default PTT Rx action for event one on both band 0 and 1 to be high so the relay is energised for Rx and unenergised for Tx,
 *             to change set the required behaviour in Default RX Behaviour section below. 
 *             
 * K3S Setup :  K3IOB is set to 'trn' any other setting will not work correctly,  I have my 2m tvrx as trn1 and 70cm tvrx as trn2.
 *
 */ 
//  Version 4.3 I/O Pins.
//
//1   -- reset
//2
//3
//4   -- D2  ---- BAND 0 LED
//5   -- D3  ---- BAND 1 LED
//6   -- D4  ---- BAND 2 LED
//7   -- vcc
//8   -- gnd
//9   -- Xtal
//10  -- xtal
//11  -- D5  ---- PTT LED
//12  -- D6  ---- VSWR BUZZER
//13  -- D7  ---- BAND 0 INPUT
//14  -- D8  ---- BAND 1 INPUT
//15  -- D9  ---- VSWR TRIP INPUT
//16  -- D10 ---- PTT INPUT
//17  -- D11 ---- TX INHIBIT 
//18  -- D12 ---- IF TX 
//19  -- D13 ---- IF RX
//20  -- avcc
//21
//22  -- agnd
//23  -- a0 ---- BAND 0 EVENT 1 *
//24  -- a1 ---- BAND 1 EVENT 1 *
//25  -- a2 ---- BAND 0 EVENT 2 *
//26  -- a3 ---- BAND 1 EVENT 2 *
//27  -- a4 ---- BAND 0 EVENT 3 *
//28  -- a5 ---- BAND 1 EVENT 3 *
 
// https://github.com/thomasfredericks/Bounce2
#include <Bounce2.h>


// Event Sequence Delays

int Sequence_Delay = 50; // Delay in milliseconds.
int Txinhibit_Delay = 50; 

// Default RX behaviour of events HIGH = On; LOW = Off

int band0_event1 = HIGH;
int band0_event2 = LOW;
int band0_event3 = LOW;

int band1_event1 = HIGH;
int band1_event2 = LOW;
int band1_event3 = LOW;

// Debugging
bool debug_main = false;

// Band input pins.
#define Band0Input 7 // digital pin 7 when high band 0 enabled.
#define Band1Input 8 // digital pin 8 when high band 1 enabled.

// Instantiate a bounce objects for Band inputs.
Bounce Band0InputDebouncer = Bounce(); 
Bounce Band1InputDebouncer = Bounce();

// VSWR TRIP input pin.
#define VswrTripInput 9  // digital pin 9, pin low = vswr trip enabled.

// Instantiate a bounce object for vswr trip input.
Bounce VswrTripInputDebouncer = Bounce();

// PTT input pin.
#define PttInput 10 // digital pin 10, pin low = PTT enabled.

// Instantiate a bounce object for vswr trip input.
Bounce PttInputDebouncer = Bounce();

// TX inhibit pin.
#define TxInhibit 11  // digital pin 11, pin high = inhibit enabled, low = inhibit disabled.

// IF relays
#define TxIfOutput 12 // digital pin 12 when low IF TX band 0 enabled when high TX band 0 enabled.
#define RxIfOutput 13 // digital pin 13 when low IF RX band 0 enabled when high RX band 1 enabled.

// Sequencer event sinks.
#define Band0SequencerEvent1 A0  // digital pin A5 darlington pair can switch 12 or 24V.
#define Band0SequencerEvent2 A2  // digital pin A4 sinks 30mA to GND.
#define Band0SequencerEvent3 A4  // digital pin A3 sinks 30mA to GND.

#define Band1SequencerEvent1 A1  // digital pin A2 darlington pair can switch 12 or 24V.
#define Band1SequencerEvent2 A3  // digital pin A1 sinks 30mA to GND.
#define Band1SequencerEvent3 A5  // digital pin A0 sinks 30mA to GND.

// Band indicators.
#define Band0Led 2 // digital pin 2, pin high = 2m LED enabled.
#define Band1Led 3 // digital pin 3, pin high = 70cm LED enabled.
#define Band2Led 4 // digital pin 4, pin high = HF LED enabled.

// PTT indicator.
#define PttLed 5 // digital pin 5, pin high = TX LED enabled.

// VSWR trip buzzer.

#define VswrTripBuzzer 6  // digital pin 8, pin high = Buzzer enabled.

// Variables.
int BandSequencer = 0;   // Selected sequencer 0 - 2, 0 being VHF, 1 UHF and 2 HF where 2 bypasses sequenced events excluding tx inhibit. 
bool InitialBandSetup = true;  //  A simple trip so we set the inital band if we either turn or reset the K3S switcher unit after the K3S has been turned on.


void setup() {
  // Setup serial interface for debugging.
  Serial.begin(9600);
  
  // Attach the Band input debouncer instances 
  Band0InputDebouncer.attach(Band0Input,INPUT); 
  Band0InputDebouncer.interval(100); // Use a debounce interval of 25 milliseconds
  Band1InputDebouncer.attach(Band1Input,INPUT); 
  Band1InputDebouncer.interval(100); // Use a debounce interval of 25 milliseconds

  // Attach the VSWR TRIP input debounce instance
  VswrTripInputDebouncer.attach(VswrTripInput, INPUT_PULLUP);
  VswrTripInputDebouncer.interval(100);

  // Attach the PTT input debounce instance
  PttInputDebouncer.attach(PttInput, INPUT_PULLUP);
  PttInputDebouncer.interval(30);

  // pinMode Outputs.
  pinMode(TxInhibit, OUTPUT);
  
  pinMode(TxIfOutput, OUTPUT);
  pinMode(RxIfOutput, OUTPUT);
  
  pinMode(Band0SequencerEvent1, OUTPUT);
  pinMode(Band0SequencerEvent2, OUTPUT);
  pinMode(Band0SequencerEvent3, OUTPUT);

  pinMode(Band1SequencerEvent1, OUTPUT);
  pinMode(Band1SequencerEvent2, OUTPUT);
  pinMode(Band1SequencerEvent3, OUTPUT);

  pinMode(Band0Led, OUTPUT);
  pinMode(Band1Led, OUTPUT);
  pinMode(Band2Led, OUTPUT);

  pinMode(PttLed, OUTPUT);

  pinMode(VswrTripBuzzer, OUTPUT);

  // Set the TxInhibit default to HIGH so the K3S can't generate any RF to start with.
  digitalWrite(TxInhibit, HIGH);
  

}

void loop() {

  bool Band0Changed = Band0InputDebouncer.update(); // Update the Band0InputDebouncer instance 
  bool Band1Changed = Band1InputDebouncer.update(); // Update the Band1InputDebouncer instance

  if (InitialBandSetup) {
    
    Band0Changed = true; // Set one of the band changes to true to force a loop through the band change routine.
    InitialBandSetup = false; // Once we've done an inital loop we revert to normal behaviour of updating from Debouncer instance.
    
  }
  
  // *** Band change routine. *** 

  if ((Band0Changed) || (Band1Changed)) { // If either band changes then run the band change routine.

    int Band0Value = Band0InputDebouncer.read(); // Read current high or low input into variable.
    int Band1Value = Band1InputDebouncer.read(); // Read current high or low input into variable.

    if (( Band0Value == LOW ) && ( Band1Value == LOW )){
      if (debug_main) { Serial.println("BandChange - Band0Value LOW, Band1Value LOW, = K3S HF Bands"); };

      // Set the band indicator leds.
      digitalWrite(Band0Led, LOW);
      digitalWrite(Band1Led, LOW);
      digitalWrite(Band2Led, HIGH);
      
      BandSequencer = 2;

      PttRx(); // Call the PttRx function so we set the default Rx behaviour.
    }
    else if (( Band0Value == HIGH ) && ( Band1Value == LOW )) {
      if (debug_main) { Serial.println("BandChange - Band0Value HIGH, Band1Value LOW, = K3S VHF Band"); };

      // Set the band indicator leds.
      digitalWrite(Band0Led, HIGH);
      digitalWrite(Band1Led, LOW);
      digitalWrite(Band2Led, LOW);

      // Set the band IF relays to off for VHF band.
      digitalWrite(TxIfOutput, LOW);
      digitalWrite(RxIfOutput, LOW);
     
      BandSequencer = 0;

      PttRx(); // Call the PttRx function so we set the default Rx behaviour.
    }
    else if (( Band0Value == LOW ) && ( Band1Value == HIGH )) {
      if (debug_main) { Serial.println("BandChange - Band0Value LOW, Band1Value HIGH, = K3S UHF Band"); };

      // Set the band indicator leds.
      digitalWrite(Band0Led, LOW);
      digitalWrite(Band1Led, HIGH);
      digitalWrite(Band2Led, LOW);

      
      // Set the band IF relays to on for UHF band.
      digitalWrite(TxIfOutput, HIGH);
      digitalWrite(RxIfOutput, HIGH);
    
      BandSequencer = 1;

      PttRx(); // Call the PttRx function so we set the default Rx behaviour.
    }
  }

  // *** VSWR_TRIP change routine. ***

  VswrTripInputDebouncer.update();

  if ( VswrTripInputDebouncer.fell() ) {
    if (debug_main) { Serial.println("VSWR_TRIP has gone low so we need to cease all actions."); };

    VswrTrip();
    
  }

  // *** PTT change routine. ***

  PttInputDebouncer.update();

  if ( PttInputDebouncer.fell() ) {
    if (debug_main) { Serial.println("PTT has gone low so we need to call the sequencer routine for the appropriate band."); };

    PttTx();
    
  }
  else if ( PttInputDebouncer.rose() ) {
    if (debug_main) { Serial.println("PTT has gone high so we need to cease the sequencer routine for the appropriate band."); };

    PttRx();
    
  }

}

void VswrTrip(){

   if (debug_main) { Serial.println("VswrTrip called."); };

   PttRx();
  
   while (1) {  // Here we loop forever or until reset is called.
    digitalWrite(Band0Led, LOW);
    digitalWrite(Band1Led, LOW);
    digitalWrite(Band2Led, LOW);
    tone(VswrTripBuzzer, 1000);
    delay(500);
    digitalWrite(Band0Led, HIGH);
    digitalWrite(Band1Led, HIGH);
    digitalWrite(Band2Led, HIGH);
    noTone(VswrTripBuzzer);
    delay(500);
  }
}

void PttTx() {

  if (debug_main) { Serial.println("PttTx called."); };

  digitalWrite(PttLed, HIGH);
  
  if (BandSequencer == 0){
    // VHF
    if (debug_main) { Serial.println("PttTx BandSequence 0 - VHF."); };
    digitalWrite(Band0SequencerEvent1, !band0_event1);  // Change over relay is held high for tx so if we have a failure we default to the tx path.
    delay(Sequence_Delay);
    digitalWrite(Band0SequencerEvent2, !band0_event2); // when high the event transistor will sink 30mA.
    delay(Sequence_Delay);
    digitalWrite(Band0SequencerEvent3, !band0_event3); // when high the event transistor will sink 30mA.
    delay(Txinhibit_Delay);
    digitalWrite(TxInhibit, LOW);  // when low the K3S will all RF to be generated.
  }
  else if (BandSequencer == 1){
    // UHF
    if (debug_main) { Serial.println("PttTx BandSequence 1 - UHF."); };
    digitalWrite(Band1SequencerEvent1, !band1_event1);  // Change over relay is held low for tx so if we have a failure we default to the tx path.
    delay(Sequence_Delay);
    digitalWrite(Band1SequencerEvent2, !band1_event2); // when high the event transistor will sink 30mA.
    delay(Sequence_Delay);
    digitalWrite(Band1SequencerEvent3, !band1_event3); // when high the event transistor will sink 30mA.
    delay(Txinhibit_Delay);
    digitalWrite(TxInhibit, LOW);  // when low the K3S will all RF to be generated.
  }
  else if (BandSequencer == 2){
    // HF
    if (debug_main) { Serial.println("PttTx BandSequence 2 - HF."); };
    digitalWrite(TxInhibit, LOW);  // when low the K3S will all RF to be generated.
  }
}

void PttRx() {

  if (debug_main) { Serial.println("PttRx called."); };

  digitalWrite(PttLed, LOW);

  if (BandSequencer == 0){
    // VHF
    if (debug_main) { Serial.println("PttRx BandSequence 0 - VHF."); };
    digitalWrite(TxInhibit, HIGH);  // when high the K3S will not be able to generate RF.
    delay(Txinhibit_Delay);
    digitalWrite(Band0SequencerEvent3, band0_event3);   // when low the event transistor will cease sinking 30mA.
    delay(Sequence_Delay);
    digitalWrite(Band0SequencerEvent2, band0_event2);   // when low the event transistor will cease sinking 30mA.
    delay(Sequence_Delay);
    digitalWrite(Band0SequencerEvent1, band0_event1);  // Change over relay is held low for rx so if we have a failure we default to the tx path.

    if (band0_event1 == HIGH) {
      digitalWrite(Band1SequencerEvent1, LOW);   // Do the opposite action for Band 1 Event 1 otherwise both leds will be which would be visually confusing.
    }
  }
  else if (BandSequencer == 1){
    // UHF
    if (debug_main) { Serial.println("PttRx BandSequence 1 - UHF."); };
    digitalWrite(TxInhibit, HIGH);  // when high the K3S will not be able to generate RF.
    delay(Txinhibit_Delay);
    digitalWrite(Band1SequencerEvent3, band1_event3);   // when low the event transistor will cease sinking 30mA.
    delay(Sequence_Delay);
    digitalWrite(Band1SequencerEvent2, band1_event2);   // when low the event transistor will cease sinking 30mA.
    delay(Sequence_Delay);
    digitalWrite(Band1SequencerEvent1, band1_event1);  // Change over relay is held low for rx so if we have a failure we default to the tx path.

    /* As I have event1 on both band 0 & 1 high this looks confusing so I switch the unused bands event 1 to off, remember to
       comment this line out if the default current band event 1 is low.
    */

    if (band1_event1 == HIGH) {
      digitalWrite(Band0SequencerEvent1, LOW);   // Do the opposite action for Band 0 Event 1 otherwise both leds will be which could be visually confusing.
    }
  }
  else if (BandSequencer == 2){
    // HF
    if (debug_main) { Serial.println("PttRx BandSequence 2 - HF."); };
    digitalWrite(TxInhibit, HIGH);  // when high the K3S will not be able to generate RF.
    
    /* As I have event1 on both band 0 & 1 high this looks confusing so I switch the unused bands event 1 to off, remember to
       comment this line out if the default current band event 1 is low.
    */

    // Set all Band 1 events to off otherwise visually it could be confusing.
    digitalWrite(Band1SequencerEvent1, LOW);
    digitalWrite(Band1SequencerEvent2, LOW);
    digitalWrite(Band1SequencerEvent3, LOW);

    // Set all Band 0 events to off otherwise visually it could be confusing.
    digitalWrite(Band0SequencerEvent1, LOW);
    digitalWrite(Band0SequencerEvent2, LOW);
    digitalWrite(Band0SequencerEvent3, LOW);   
  }  
 
}
