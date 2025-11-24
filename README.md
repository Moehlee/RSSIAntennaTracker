Open-Source Antenna Tracker

I added a non-commercial license, because i don't want that a company steals it

I did not Test it during flying, but i will share my experience

Parts list:
- ESP32F401CEU Blackpill 
- OWLRC Modul or any Fatshark module
- 2.54mm socket strip or 2x female servo cable
- 2x 13kg servo
- DC-DC Step Down Converter 4-40V 8A
  -> that powers the system
- DC-DC Step Down Converter 4-40V 3A
  -> just powers the module
- LCD 1602A
- 10k rotary potentiometer
- LED i found on the floor
- 220Ω Resistor
- Stlink i had the V2 
- XT60 female
- toggle switch
- buzzer
- 4-pin AUX cable
- m3 and m4 screws

Furthermore, you don't need exactly the same parts for it to work. My module uses RX5808 receivers, where the RSSI pin needs to be soldered on.

3D prints:
- https://www.thingiverse.com/thing:3500798/files
  - i used dock-front and fpv-module, the dock body i modified
- https://www.thingiverse.com/thing:1941574
- the case i made myself, they are on github
