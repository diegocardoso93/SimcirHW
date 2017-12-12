

-----------------
-- GPIO MODULE --
-----------------
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


----------------
-- TMR MODULE --
----------------
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

----------------------
-- WEBSOCKET MODULE --
----------------------
local websocketclient = {}
local _websocketclient = {}

function _websocketclient:start()
  
end

function _websocketclient:connect(address)
  print("request connection to " .. address)
  self:bind("connection")
end

function _websocketclient:on(event, func)
  if event == "connection" then
    self.con_func = func
  elseif event == "receive" then
    self.rcv_func = func
  elseif event == "close" then
    self.cls_func = func
  end
end

-- exs:
-- ws:bind("receive", {msg:"yeap", opcode: 1}), -- opcode 1 text, 2 binary
-- ws:bind("close", {status = 200}) 
function _websocketclient:bind(event, _args)
  if event == "connection" then
    self.con_func(websocketclient)
  elseif event == "receive" then
    self.rcv_func(websocketclient, _args.msg, _args.opcode)
  elseif event == "close" then
    self.cls_func(websocketclient, _args.status)
  end
end

function _websocketclient:send(msg, opcode)
  print("message sent: " .. msg)
end

function websocketclient:new()
  self = {}
  setmetatable(self, { __index = _websocketclient })
  
  self.con_func = function() end
  self.rcv_func = function() end
  self.cls_func = function() end
  
  return self
end


websocket = {}

function websocket.createClient()
  return websocketclient:new()
end

