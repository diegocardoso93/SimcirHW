
require "circuitlib"

require "fakelib" -- gpio, tmr fake libs (disable in real hw)

local simcirhw = {}

local SimcirHW = {}

local HwInterface = {}

function SimcirHW:receive_circuit(str_ckt)
  self.str_circuit = str_ckt
end

function SimcirHW:parse_circuit()
  self.circuit = loadstring("return " .. self.str_circuit)()
  
  for k, out in pairs(self.circuit.outputs) do
    self.expressions[k] = convert_circuit(out)
  end
  
  HwInterface = require(self.circuit.hardware)
  --print(self.expressions["Out"])
end

function SimcirHW:configure_timeslices()
  
end

function SimcirHW:eval()
  -- eval all expressions
end

function SimcirHW:read_pin(label)
  return gpio.read(HwInterface.get_pin(label))
end

function SimcirHW:start()
  
  for k, inp in pairs(self.circuit.inputs) do
    if type(inp) == "table" then
      print("table")
      -- TODO: configure_timeslices
    else
      print("str")
      -- static input
      print(gpio.read(HwInterface.get_pin("D2")))
      gpio.write(2, 1)
      print(gpio.read(2))
    end
  end
  
end

function SimcirHW:execute()
  -- step by step, start timers tick if necessary
  -- eval ckt
end

function SimcirHW:stop()
  -- stop timers (infinity sequence)
end

function simcirhw:new()
  local self = {}
  setmetatable(self, { __index = SimcirHW })
  
  self.str_circuit = ""
  self.circuit = {
    inputs   = {},
    outputs  = {},
    pin_map  = {},
    hardware = {},
  }
  self.expressions = {}
  
  return self
end

return simcirhw