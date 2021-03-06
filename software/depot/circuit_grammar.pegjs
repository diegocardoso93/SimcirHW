// Parsing Expression Grammar for validate logical operations
// with operators written in cursive
// ------------------------
//
// Accept expressions like: "(In1 AND In2) XOR (In1 OR NOT(In2))" -- only for syntax check
//                          "(1 AND 1) XOR (0 OR NOT(1))" -- for eval
// https://pegjs.org/online
// ------------------------
//	Grammar:
//	Expression <- Term ( _ ('AND' / 'OR' / 'NAND' / 'NOR' / 'XOR' / 'XNOR') _ Term)*
//	Term       <- _ '(' Expression ')' _ / 'NOT(' _ Expression _ ')' / Operand
//	Operand    <- (!"undefined" [A-z]+[0-9]* / [0-1])*
//	_          <- [ \t\n\r]*

Expression
  = head:Term tail:( _ ("AND" / "OR" / "NAND" / "NOR" / "XOR" / "XNOR") _ Term)* {
      return tail.reduce(function(result, element) {
        if (element[1] === "AND") { return result & element[3]; }
        if (element[1] === "OR") { return result | element[3]; }
      	if (element[1] === "NAND") { return Number(!(result && element[3])); }
        if (element[1] === "NOR") { return Number(!(result || element[3])); }
        if (element[1] === "XOR") { return result ^ element[3]; }
        if (element[1] === "XNOR") { return Number(!(result ^ element[3])); }
      }, head);
    }

Term
  = _ "(" _ expr:Expression _ ")" _ { return expr; }
  / "NOT(" _ expr:Expression _ ")" { return !expr; }
  / Operand

Operand "Operand"
  = (!"undefined" [A-z]+[0-9]* / [0-1])* {
  	return (text()==0 || text()==1) ?
    	parseInt(text(), 2) : 1;
  }

_ "whitespace"
  = [ \t\n\r]*
