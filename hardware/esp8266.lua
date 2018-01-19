
local Esp8266 = {}

Esp8266.named_pins = {
  D0  =  0, -- GPIO16
  D1  =  1, -- GPIO5
  D2  =  2, -- GPIO4
  D3  =  3, -- GPIO0
  D4  =  4, -- GPIO2
  D5  =  5, -- GPIO14
  D6  =  6, -- GPIO12
  D7  =  7, -- GPIO13
  D8  =  8, -- GPIO15
  RX  =  9, -- GPIO3
  TX  = 10, -- GPIO1
  SD2 = 11, -- GPIO9
  SD3 = 12  -- GPIO10
}

function Esp8266.get_name()
  return "esp8266"
end

function Esp8266.get_pin(label)
  return Esp8266.named_pins[label]
end

return Esp8266