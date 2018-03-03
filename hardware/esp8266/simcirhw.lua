
require "circuitlib"
--require "nodemcu_fakelibs" -- gpio, tmr fake libs (disable in real hw)

local Logger = require "logger"

local simcirhw = {}
local SimcirHW = {}
local HwInterface = {}


local MINIMAL_SLICE_SIZE = 100
local LOGGER_REFRESH_RATE = 100

function SimcirHW:eval_message(str_msg)
  local message = sjson.decode(str_msg)
  self.message = message
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
  if self.circuit.maxtime > 0 then
    LOGGER_REFRESH_RATE = self.circuit.maxtime
  end
  self.sliceTimer = tmr.create()
  self.sliceTimer:register(MINIMAL_SLICE_SIZE, tmr.ALARM_AUTO, function()
    for k, inp in pairs(self.circuit.inputs) do
      -- virtual input
      if type(inp) == "table" then
        local acum_time = 0
        for i, time in ipairs(inp.timeslices) do
          if acum_time == self.slice_timer_counter then
            self.state.inputs[k] = inp.values[i]
          end
          acum_time = acum_time + time
        end
      elseif self.circuit.pin_map[k] == nil then
        -- fixed input value
        self.state.inputs[k] = inp
      else
        -- read pin
        self.state.inputs[k] = gpio.read(HwInterface.get_pin(self.circuit.pin_map[k]))
      end
    end
    self:eval()
    self:propagate()
    self:register_log()

    if self.slice_timer_counter == LOGGER_REFRESH_RATE then
      self.sliceTimer:stop()
      SCH.logger:format_message_to_send()
      SCH.ws:send(SCH.logger.message)
      self.sliceTimer:start()
      SCH.logger:clean()
      self.slice_timer_counter = 0
      if self.circuit.maxtime > 0 then
        self.sliceTimer:stop()
        self.sliceTimer:unregister()
      end
    else
      self.slice_timer_counter = self.slice_timer_counter + MINIMAL_SLICE_SIZE
    end
  end)
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
  self:propagate()
  --self:execute()
  self.sliceTimer:start()
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
  node.restart()
end

function SimcirHW:destroy()
  if self.sliceTimer then
    self.sliceTimer:stop()
    self.sliceTimer:unregister()
  end
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
    maxtime  = {}
  }
  self.state = {
    outputs = {},
    inputs  = {}
  }
  self.expressions = {}
  
  self.logger = Logger:new()
  self.ws = {}
  self.slice_timer = nil
  self.slice_timer_counter = 0

  return self
end

return simcirhw