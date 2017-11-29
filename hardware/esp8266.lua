
local class = require 'pl.class'
local pretty = require 'pl.pretty'

class.Esp8266()

function Esp8266:_init()
  self.named_pins = {
    D0=0,
    D1=1
  }
end

function Esp8266:__tostring()
  return pretty.write(self)
end

return Esp8266