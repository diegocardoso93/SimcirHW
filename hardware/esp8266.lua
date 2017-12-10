
local Esp8266 = {}

Esp8266.named_pins = {
  D0 = 0,
  D1 = 1,
  D2 = 2,
}

function Esp8266.get_name()
  return "esp8266"
end

function Esp8266.get_pin(label)
  return Esp8266.named_pins[label]
end

return Esp8266