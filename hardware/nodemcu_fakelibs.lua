
gpio = {
  -- initial pins state
  pins = {
    [0]=0,
    [1]=0,
    [2]=0,
    [3]=0,
    [4]=0,
    [5]=0,
    [6]=0,
    [7]=0
  },
  
  pins_type = {},
  
  -- constants
  INPUT = 1,
  OUTPUT = 1,
  HIGH = 1,
  LOW = 0
}

function gpio.mode(pin, mode)
  gpio.pins_type[pin] = mode
end

function gpio.write(pin, level)
  if gpio.pins_type[pin] ~= gpio.OUTPUT then
    error("attempt to write a non output pin.")
  end
  gpio.pins[pin] = level
end

function gpio.read(pin, level)
  if gpio.pins_type[pin] ~= gpio.INPUT then
    error("attempt to read a non output pin.")
  end
  return gpio.pins[pin]
end



tmr = {}
local _tmr = {}

function _tmr:register(interval, mode, func)
  self.co = coroutine.create(func)
end

function _tmr:unregister()
  self.co = nil
  self = nil
  collectgarbage()
end

function _tmr:start()
  coroutine.resume(self.co, self.tmr)
end

function tmr.create()
  local self = {}
  setmetatable(self, { __index = _tmr })
  
  return self
end
