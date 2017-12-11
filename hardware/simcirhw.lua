
require "circuitlib"

require "nodemcu_fakelibs" -- gpio, tmr fake libs (disable in real hw)

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

function SimcirHW:configure_inputs()
  for k, inp in pairs(self.circuit.inputs) do
    if type(inp) == "table" then
      local acc_time = 0
      for i, time in ipairs(inp.timeslices) do
        local t_tmr = tmr.create()
        t_tmr:register(acc_time, tmr.ALARM_SINGLE, 
          function (t)
            self:update_input_state(k, inp.values[i])
            self:eval()
            self:propagate()
            -- temporary debug here
            for x, y in pairs(self.state.outputs) do
              print(x .. " -> " .. y)
            end
            t:unregister()
          end)
        acc_time = acc_time + time
        -- TODO: sort by index?!
        self.timer_slices[#self.timer_slices+1] = t_tmr
      end
    else
      -- static input
      self:update_input_state(k, inp)
      self:eval()
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

function SimcirHW:update_input_state(k, v)
  self.state.inputs[k] = v
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
  self:configure_inputs()
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

function simcirhw:new()
  local self = {}
  setmetatable(self, { __index = SimcirHW })
  
  self.str_circuit = ""
  self.circuit = {
    inputs   = {},
    outputs  = {},
    pin_map  = {},
    hardware = {},
    cycles   = {},
  }
  self.state = {
    outputs    = {},
    inputs     = {}
  }
  self.expressions = {}
  self.timer_slices = {}
  return self
end

return simcirhw