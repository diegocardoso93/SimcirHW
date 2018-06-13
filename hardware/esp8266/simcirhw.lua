
require "circuitlib"
--require "nodemcu_fakelibs" -- gpio, tmr fake libs (disable in real hw)

--local Logger = require "logger"

local simcirhw = {}
local SimcirHW = {}
local HwInterface = {}


local MINIMAL_SLICE_SIZE = 100

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
    self.sliceTimer = tmr.create()
    self.sliceTimer:register(MINIMAL_SLICE_SIZE, tmr.ALARM_AUTO, function()
      if type(self.state[self.actual_step]) == "table" then
        self.state[self.actual_step].l = tmr.now()
        self:propagate()
      end
      if self.actual_step == self.steps then
        local tmessage = self:format_message_to_send()
        SCH.ws:send(tmessage)
        self.sliceTimer:stop()
        if self.circuit.maxtime > 0 then
          self.sliceTimer:stop()
          self.sliceTimer:unregister()
        end
      end
      self.actual_step = self.actual_step + 1
    end)
  else
    self.sliceTimer = tmr.create()
    self.sliceTimer:register(MINIMAL_SLICE_SIZE, tmr.ALARM_AUTO, function()
      self.state[self.actual_step] = {i={}, o={}, l=nil}
      self.state[self.actual_step].l = tmr.now()
      for k, inp in pairs(self.circuit.inputs) do
        self.state[self.actual_step].i[k] = gpio.read(HwInterface.get_pin(self.circuit.pin_map[k]))
      end
      self:eval()
      self:propagate()
      local tmessage = self:format_message_to_send()
      SCH.ws:send(tmessage)
    end)
  end
end

function SimcirHW:eval()
  -- loadscope
  local str_exec = ""
  for k, v in pairs(self.state[self.actual_step].i) do
    str_exec = str_exec .. k .. " = " .. v .. ";"
  end
  for out, exp in pairs(self.expressions) do
    self.state[self.actual_step].o[out] = loadstring(str_exec .. "return " .. exp)()
  end
end

function SimcirHW:format_message_to_send()
  if self.state ~= nil then
    local message = {
      type="datalog",
      data=self.state
    }
    return sjson.encode(message)
  end
end

function SimcirHW:read_pin(label)
  return gpio.read(HwInterface.get_pin(label))
end

function SimcirHW:get_expression(out)
  return self.expressions[out]
end

function SimcirHW:propagate()
  for k, v in pairs(self.state[self.actual_step].o) do
    gpio.write(HwInterface.get_pin(self.circuit.pin_map[k]), v)
  end
end

function SimcirHW:start()
  print("call start")
  self:generate_lookup()
  self.executing = true
  self:configure_hw_gpios()
  self:configure_inputs()
  self.sliceTimer:start()
end

function SimcirHW:generate_lookup()
  if (self.circuit.maxtime > 0) then
    local t_slice_timer_counter = 0
    while (t_slice_timer_counter <= self.circuit.maxtime) do
      local find = false
      for k, inp in pairs(self.circuit.inputs) do
        -- virtual input
        if type(inp) == "table" then
          local acum_time = 0
          if inp.timeslices[#inp.timeslices] ~= 0 then inp.timeslices[#inp.timeslices+1] = 0 end
          for i, time in ipairs(inp.timeslices) do
            if acum_time == t_slice_timer_counter then
              ind = (t_slice_timer_counter/100) + 1
              if self.state[ind] == nil then self.state[ind] = {i={}, o={}} end
              self.state[ind].i[k] = inp.values[i]
              find = true
            end
            acum_time = acum_time + time
          end
        elseif self.circuit.pin_map[k] == nil then
          -- fixed input value
          if ind ~= nil then ind = 1 end
          if self.state[ind] == nil then self.state[ind] = {i={}, o={}} end
          self.state[ind].i[k] = inp
        end
      end

      if find then
        for k, _ in pairs(self.circuit.inputs) do
          if self.state[ind].i[k] == nil then self.state[ind].i[k] = self.state[last].i[k] end
        end
        last = ind
        local str_exec = ""
        for k, v in pairs(self.state[ind].i) do
          str_exec = str_exec .. k .. " = " .. v .. ";"
        end
        for out, exp in pairs(self.expressions) do
          self.state[ind].o[out] = loadstring(str_exec .. "return " .. exp)()
        end
      end
      t_slice_timer_counter = t_slice_timer_counter + MINIMAL_SLICE_SIZE
      self.steps = self.steps + 1
      if self.steps%40 == 0 then tmr.wdclr() end -- manually feed watchdog
    end
  end
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
  self.logger:push_state(self.state[self.actual_step])
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
  self.state = {}
  self.expressions = {}
  
  --self.logger = Logger:new()
  self.ws = {}
  self.slice_timer = nil
  self.slice_timer_counter = 0
  self.executing = false

  self.steps = 1
  self.actual_step = 1

  return self
end

return simcirhw