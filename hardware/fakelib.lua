
gpio = {
  pins = {[0]=0,0,0,0,0,0,0,0}
}

function gpio.write(pin, level)
  gpio.pins[pin] = level
end

function gpio.read(pin, level)
  return gpio.pins[pin]
end