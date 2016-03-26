# Slcanuino

This is an Arduino sketch which makes a CAN-BUS shield into a CAN-USB adapter for Linux SocketCAN(can-utils). Library files under Canbus folder originaly comes from 'CAN-BUS ECU Reader demo sketch v4' on skpang and were modified. The files should be copied under ~/Arduino/libraries/ to compile the sketch file:'slcan.ino'.

http://skpang.co.uk/catalog/arduino-canbus-shield-with-usd-card-holder-p-706.html


# Supported hardware

I tested this sketch on the following 'CAN-BUS Shield'.

https://www.sparkfun.com/products/10039


# How to use

Burn your Arduino with this and install can-utils for your linux environment in advance.

# Deps
1. slcan (kernel module)
2. SocketCAN (http://www.pengutronix.de/software/libsocketcan/)
3. can-utils (https://github.com/linux-can/can-utils)


setup:

    $ sudo slcan\_attach -f -s6 -o /dev/ttyUSB0  
    $ sudo slcand -S 1000000 ttyUSB0 can0  
    $ sudo ifconfig can0 up  

then,

    $ candump can0

cleanup:

    $ sudo ifconfig can0 down  
    $ sudo killall slcand  
