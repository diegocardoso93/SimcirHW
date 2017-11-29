
local class = require 'pl.class'
local pretty = require 'pl.pretty'

class.SimcirHW()

function SimcirHW:_init()
  self.inputs = {}
  self.outputs = {}
  self.outputs_expression = {}
  self.input_sequence = {}
  self.pin_map = {}
  
  self.str_circuit = ""
  self.circuit = {}
  
  self.named_pins = {
    D0=0,
    D1=1
  }
end

function SimcirHW:__tostring()
  return pretty.write(self)
end

function SimcirHW:receive_circuit(str_ckt)
  self.str_circuit = str_ckt
end

function SimcirHW:parse_circuit()
  self.circuit = loadstring("return " .. self.str_circuit)()
  
  for k,input in pairs(self.circuit["inputs"]) do
    self.inputs[input] = {}
  end
end

function SimcirHW:parse_input_sequence()

  local _tbl = {}
  string.gsub("0,200;1,200;0,400;1,600", "(%w+),(%w+);", function(n1, n2) print(n1,n2) table.insert(_tbl, {n1, n2}) end)  
  self.input_sequence = _tbl

end


function SimcirHW:execute()
end

function SimcirHW:stop()
end

return SimcirHW
