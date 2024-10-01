# Bed Climate Control Fan

This project is still a work in progress

## Summary

I found myself getting very hot during the summer at night which affected my sleep. I decided to look online to see if I could find any bed cooling products I wanted to get. One that peaked my interest is called the BedJet which blows climate controlled air under the covers and optionally through a special sheet they call a cloud sheet. However, even though it is cheaper than many bed cooling solutions, I still did not want to pay the $500 price tag and I wanted a project to keep up my engineering skills. I decided I was going to make my own version of the BedJet which I could customize by inluding any features I wanted. Currently, what I designed is able to blow air under the sheets at any speed ranging from 0 being off to 40 being fully on. This is all controlled by a remote which allows the user to adjust the fan speed level while displaying the level on a small OLED display.

## Technical Description

The core of this project are two ESP32 Devkits which communicate to each other using Bluetooth Low Energy (BLE). The remote acts as the server while the fan acts as the client. 

### Remote (Server)

The remote currently contains three buttons and a 128x32 pixel OLED display. Two of the buttons adjust the fan speed and one toggles the backlight on the LCD connected to the fan ESP32. The LCD will probably not be a part of the final design so this button will most likely be removed in the future. Whenever one of the buttons is pressed, the OLED display turns on and displays the current fan speed level and a placeholder for temperature. The ESP32 polls at a rate of 100 Hz to check if a button is pressed and if one is, it updates the data that it writes to the client via BLE. The data that the server writes to the client is the fan level multiplied by 10 plus the value of the backlight. The multiple pieces of data are sent over in a single characteristic because, in the past, I have had trouble getting the data transferred via multiple characteristics to be consistent. In my experience, when only using one characteristic, the data is nearly perfectly consistent. Since I am planning on running the remote off batteries, it needs to consume as little power as possible. This is accomplished by putting the ESP32 into deep sleep and setting the buttons to wake it up. The CPU clock speed was also reduced from 240 MHz to 160 MHz. I would prefer to lower it more but the at 80 MHz a watchdog timer occasionally gets triggered and anything below 80 MHz causes the BLE to not work.

### Fan (Client)

The fan controller currently contains a 12 V 120 mm server fan, a 5 V buck converter, and a LCD screen which will most likely be removed soon. The ESP32 that controls the fan reads the data written by the server, decodes it, and uses the information to set the LCD backlight value and fan level. The fan takes a PWM signal to set the speed. The ESP32 takes the level value from the server and multiplies it by 6.375 to provide the fan with a PWM value that ranges from 0 to 255. 

## Current and Future Work

Since this project is a work in progress, there are more features I want to add in hardware and firmware. Currently, all the electronics are being held on solderless breadboards which are not a very good permanent solution. 

### Heater

I am currently working on adding a heater to the fan so it can heat as well as cool. This turned out to be more complicated than I expected since the heater draws much more current than I am used to working with. This results in some components becoming very hot so I need to manage the heat of the components as well as the heater. I am using PWM to drive a MOSFET and effectively vary the voltage across the heater. When I first tried this, the power supply and ESP32 made an annoying buzzing sound which I figured was not good for the components. I tried using an RC lowpass filter to smooth the signal with components I had on hand, but the buzzing was only quieted and I found that I would need to significantly raise the capacitance to properly filter the signal. I also tried using the MOSFET as a variable resistor, but since it was consuming power much more power and is in a T220 package, it was almost impossible to prevent it from overheating. This aproach would also make the whole circuit consume more power which I am trying to minimize. I have purchased some very large 100 mF capacitors for the filter and I am currently testing to see how well these work. 

While I was trying to use the transistor as a variable resistor, I ran some full power tests to see how well the 12 V 100 W heater I'm using works and, when hooked up to my fan, it was barely able to heat up the air at all. This prompted me to look to see how much power similar things use and it turns out that hair dryers and space heaters generally use around 1500 - 2000 W of power. Since I am trying to heat a confined area under the covers of my bed, I will not need to consume that much power but it did tell me that I would probably need to use more power to heat my bed. I decided to get another heating element and run them in parallel with a more powerful power supply. I considered running them in series but that would require me to increase my supply to 24 V. This would make my circuit much more complicated and I am trying to keep it as simple as possible. I am currently testing this setup to see how well it works.