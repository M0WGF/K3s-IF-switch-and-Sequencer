# K3/s IF switch and Sequencer
Two port IF switch and three event sequencer for use with the Elecraft K3 and K3s transceivers. 

Switch between two external transverters directly from the Elecraft K3/s transceiver, the unit also comes with a three
port sequencer where the first event is for switching a relay and the last two events pull to ground.

The unit is based around an Arduino UNO see the .ino file for the script, it uses one no standard library which is
called Bounce2 and be downloaded from https://github.com/thomasfredericks/Bounce2

The unit has also been designed so that when the VSWR trip is pulled to ground the unit goes into a disabled state
stopping the transmitter and sounding a buzzer until the unit is reset. 

There are two boards the main board and an indicator board.  


For further detaisl see : 
https://m0wgf.blogspot.com/2019/12/dual-transverter-if-switch-and.html 