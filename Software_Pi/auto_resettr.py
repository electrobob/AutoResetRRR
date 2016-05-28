#!/bin/python
# Script working with the auto resetter to safely shutdown the Pi in case a reboot is required
# Will send shutdown command if set GPIO is held low for > 2 seconds, so it is not triggered by blinky
# add chronjob to run at boot: @reboot sudo python /your_path/auto_resettr.py 
# Created: 22/05/2016
# Author : bob bogdan@electrobob.com
# http://www.electrobob.com/autoresetrrr/


import RPi.GPIO as GPIO
import os
import time
gpio_nr = 21

GPIO.setmode(GPIO.BCM)
GPIO.setup(gpio_nr, GPIO.IN, GPIO.PUD_UP)
try:
	while True:
		time.sleep(1)
		counts=0;
		while GPIO.input(gpio_nr)== GPIO.LOW:
			counts = counts+1
			time.sleep(0.1)
			if counts > 20:
				os.system("sudo shutdown -h now")
				GPIO.cleanup()
except KeyboardInterrupt:
	GPIO.cleanup()
GPIO.cleanup()
