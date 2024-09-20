# Bed Climate Control Fan

This project is still a work in progress

## Summary

I found myself getting very hot during the summer at night which affected my sleep. I decided to look online to see if I could find any bed cooling products I wanted to get. One that peaked my interest is called the BedJet which blows climate controlled air under the covers and optionally through a special sheet they call a cloud sheet. However, even though it is cheaper than many bed cooling solutions, I still did not want to pay the $500 price tag and I wanted a project to keep up my engineering skills. I decided I was going to make my own version of the BedJet which I could customize by inluding any features I wanted. Currently, what I designed is able to blow air under the sheets at any speed ranging from 0 being off to 40 being fully on. This is all controlled by a remote which allows the user to adjust the fan speed level while displaying the level on a small OLED display.

## Technical Description

The core of this project are two ESP32 Devkits which communicate to each other using Bluetooth Low Energy (BLE). The remote acts as the server while the fan acts as the client. 

### Remote (Server)

The remote currently contains three buttons and a 128x32 pixel OLED display. Two of the buttons adjust the fan speed and one toggles the backlight on the LCD connected to the fan ESP32. The LCD will probably not be a part of the final design so this button will most likely be removed in the future. Whenever one of the buttons is pressed, the OLED display turns on and displays the current fan speed level and a placeholder for temperature. The ESP32 polls at a rate of 10 Hz to check if a button is pressed and if one is, it updates the data that it writes to the client via BLE. The data that the server writes to the client is the fan level multiplied by 10 plus the value of the backlight. The multiple pieces of data are sent over in a single characteristic because, in the past, I have had trouble getting the data transferred via multiple characteristics to be consistent. In my experience, when only using one characteristic, the data is nearly perfectly consistent.

### Fan (Client)

