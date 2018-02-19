
require "circuitlib"
--require "nodemcu_fakelibs" -- gpio, tmr fake libs (disable in real hw)

local Logger = require "logger"

local simcirhw = {}
local SimcirHW = {}
local HwInterface = {}

local sjson = require "json"

function SimcirHW:eval_message(str_msg)
  local message = sjson.decode(str_msg)
  if message.type == "circuit" then
    self.message = message
  end
end

function SimcirHW:prepare_circuit(circuit)
  self.circuit = circuit
  
  for k, out in pairs(self.circuit.outputs) do
    self.expressions[k] = convert_circuit(out)
  end
  
  HwInterface = require(self.circuit.hardware)
  HwInterface.set_board(self.circuit.board)
end

function SimcirHW:configure_hw_gpios()
  
  function debounce(func)
    local last = 0
    local delay = 5000
    
    return function(...)
      local now = tmr.now()
      if now - last < delay then return end
      
      last = now
      return func(...)
    end
  end
  
  function get_input_name(label)
    for k, v in pairs(self.circuit.pin_map) do
      if v == label then
        return k
      end
    end
  end

  function update_input(pin, level)
    print("inp changed", pin, level)
    local inp_name = get_input_name(HwInterface.get_pin_label(pin))
    self.state.inputs[inp_name] = gpio.read(pin)
    self:eval()
    self:propagate()
    self:register_log()
  end
  
  for k, v in pairs(self.circuit.pin_map) do
    if self.circuit.inputs[k] then
      local pin_numb = HwInterface.get_pin(v)
      gpio.config({ gpio={pin_numb}, dir=gpio.IN, pull=gpio.PULL_DOWN })
      update_input(pin_numb)
      gpio.trig(pin_numb, gpio.INTR_UP_DOWN, update_input)
    end
    if self.circuit.outputs[k] then
      gpio.config({ gpio={HwInterface.get_pin(v)}, dir=gpio.OUT })
    end
  end
end

function SimcirHW:configure_inputs()
  for k, inp in pairs(self.circuit.inputs) do
    -- virtual input
    if type(inp) == "table" then
      local acc_time = 100
      inp.timeslices[#inp.timeslices+1] = inp.timeslices[#inp.timeslices]
      inp.values[#inp.values+1] = inp.values[#inp.values]
      for i, time in ipairs(inp.timeslices) do
        local t_tmr = tmr.create()
        t_tmr:register(acc_time, tmr.ALARM_SINGLE, 
          function (t)
            self.state.inputs[k] = inp.values[i]
            self:eval()
            self:propagate()
            self:register_log()
            t:unregister()
            if i == #inp.values then
              self.logger:format_message_to_send()
              self.ws.send(self.logger.message)
            end
          end)
        acc_time = acc_time + time
        self.timer_slices[#self.timer_slices+1] = t_tmr
      end
    elseif self.circuit.pin_map[k] == nil then
      -- fixed input value
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
  print("call start")
  self:configure_hw_gpios()
  self:configure_inputs()
  self:execute()
end

function SimcirHW:execute()
  self:propagate()
  for i = 1, self.circuit.cycles, 1 do
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

function SimcirHW:register_log()
  self.logger:push_state(self.state)
end

function simcirhw:new()
  local self = {}
  setmetatable(self, { __index = SimcirHW })
  
  self.message = nil
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
  
  self.logger = Logger:new()
  self.ws = {}
  
  return self
end

return simcirhw