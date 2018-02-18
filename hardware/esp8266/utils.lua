
print(tmr.now())

gpio.write(0, gpio.LOW)

print(tmr.now())

gpio.write(0, gpio.HIGH)

print(tmr.now())