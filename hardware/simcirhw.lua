
local simcirhw = {}

local SimcirHW = {}

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
  
  if self.circuit==nil then return false end

  local _tbl = {}
  for  k,d in pairs(self.circuit.inputSequence) do
    print(k,d)
    if self.inputs[k]~=nil then
      
    end
  end
  --string.gsub("0,200;1,200;0,400;1,600", "(%w+),(%w+);", function(n1, n2) print(n1,n2) table.insert(_tbl, {n1, n2}) end)
  
  self.input_sequence = _tbl
  
  return true
end

function SimcirHW:execute()
end

function SimcirHW:stop()
end

function simcirhw:new()
  local self = {}
  setmetatable(self, { __index = SimcirHW })
  
  self.inputs = {}
  self.outputs = {}
  self.outputs_expression = {}
  self.input_sequence = {}
  self.pin_map = {}
  
  self.str_circuit = ""
  self.circuit = {}
  
  return self
end

return simcirhw