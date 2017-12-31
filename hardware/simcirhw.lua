
require "circuitlib"
require "nodemcu_fakelibs" -- gpio, tmr fake libs (disable in real hw)

local Logger = require "logger"

local simcirhw = {}
local SimcirHW = {}
local HwInterface = {}

function SimcirHW:eval_message(str_msg)
  self.message = loadstring("return " .. self.str_msg)()
end

function SimcirHW:prepare_circuit(circuit)
  self.circuit = circuit
  
  for k, out in pairs(self.circuit.outputs) do
    self.expressions[k] = convert_circuit(out)
  end
  
  HwInterface = require(self.circuit.hardware)
end

function SimcirHW:configure_gpios()
  for k, v in pairs(self.circuit.pin_map) do
    if self.circuit.inputs[k] then
      gpio.mode(HwInterface.get_pin(v), gpio.INPUT)
    end
    if self.circuit.outputs[k] then
      gpio.mode(HwInterface.get_pin(v), gpio.OUTPUT)
    end
  end
end

function SimcirHW:configure_virtual_inputs()
  for k, inp in pairs(self.circuit.inputs) do
    if type(inp) == "table" then
      local acc_time = 0
      for i, time in ipairs(inp.timeslices) do
        local t_tmr = tmr.create()
        t_tmr:register(acc_time, tmr.ALARM_SINGLE, 
          function (t)
            self.state.inputs[k] = inp.values[i]
            self:eval()
            self:propagate()
            self:register_log()
            t:unregister()
          end)
        acc_time = acc_time + time
        -- TODO: sort by index?!
        self.timer_slices[#self.timer_slices+1] = t_tmr
      end
    else
      -- static input
      self.state.inputs[k] = inp
      self:eval()
      self:propagate()
      self:register_log()
    end
  end
end

function SimcirHW:eval()
  -- loadscope
  local str_exec = ""
  for k, v in pairs(self.state.inputs) do
    str_exec = str_exec .. k .. " = " .. v .. ";"
  end
  for out, exp in pairs(self.expressions) do
    self.state.outputs[out] = loadstring(str_exec .. "return " .. exp)()
  end
end

function SimcirHW:read_pin(label)
  return gpio.read(HwInterface.get_pin(label))
end

function SimcirHW:get_expression(out)
  return self.expressions[out]
end

function SimcirHW:propagate()
  for k, v in pairs(self.state.outputs) do
    gpio.write(HwInterface.get_pin(self.circuit.pin_map[k]), v)
  end
end

function SimcirHW:start()
  
  self:configure_gpios()
  self:configure_virtual_inputs()
  self:execute()

end

function SimcirHW:execute()
  self:propagate()
  for i = 0, self.circuit.cycles, 1 do
    for _, t in ipairs(self.timer_slices) do
      t:start()
    end
  end
end

function SimcirHW:stop()
end

function SimcirHW:destroy()
  self.ws = nil
  self = nil
  collectgarbage()
end

function SimcirHW:handle_ws_message(str_msg)
  print("msg received: " .. str_msg)
  self.message = loadstring("return " .. str_msg)()
  
  if self.message.type == "circuit" then
    self:prepare_circuit(self.message.circuit)
  end
end

function SimcirHW:register_log()
  self.logger:push_state(self.state)
end

function simcirhw:new()
  local self = {}
  setmetatable(self, { __index = SimcirHW })
  
  self.message = ""
  self.circuit = {
    inputs   = {},
    outputs  = {},
    pin_map  = {},
    hardware = {},
    cycles   = {},
  }
  self.state = {
    outputs = {},
    inputs  = {}
  }
  self.expressions = {}
  self.timer_slices = {}
  
  
  local ws = websocket.createClient()
  ws:on("connection", function() 
    print("got ws connection")
  end)
  ws:on("receive", function(_, msg, code)
    self:handle_ws_message(msg)
  end)
  ws:on("close", function(_)
     print("ws connection closed")
  end)
  ws:connect("ws://192.168.1.100:9061")
  self.ws = ws
  
  self.logger = Logger:new()
  
  return self
end

return simcirhw