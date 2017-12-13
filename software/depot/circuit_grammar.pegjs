// Grammar for binary operations
// with operators written in cursive
// ------------------------
//
// Accept expressions like: "(0 AND 1) XOR (1 OR NOT(0))"
//
// https://pegjs.org/online
// ------------------------
//	Grammar:
//	Expression <- Term ( _ ('NAND' / 'NOR' / 'XOR' / 'XNOR') _ Term)*
//	Term       <- Factor ( _ ('AND' / 'OR') _ Factor)*
//	Factor     <- _ '(' Expression ')' _ / 'NOT(' _ Expression _ ')' / Bit
//	Bit        <- _ [0-1]
//	_          <- [ \t\n\r]*


Expression
    = head:Term tail:(_ ("NAND" / "NOR" / "XOR" / "XNOR") _ Term)* {
    return tail.reduce(function(result, element) {
        if (element[1] === "NAND") { return Number(!(result && element[3])); }
        if (element[1] === "NOR") { return Number(!(result || element[3])); }
        if (element[1] === "XOR") { return result ^ element[3]; }
        if (element[1] === "XNOR") { return Number(!(result ^ element[3])); }
    }, head);
}

Term
    = head:Factor tail:(_ ("AND" / "OR") _ Factor)* {
    return tail.reduce(function(result, element) {
        if (element[1] === "AND") { return result & element[3]; }
        if (element[1] === "OR") { return result | element[3]; }
    }, head);
}

Factor
    = _ "(" _ expr:Expression _ ")" _ { return expr; }
/ "NOT(" _ expr:Expression _ ")" { return !expr; }
/ Bit

Bit "binary"
    = _ [0-1] { return parseInt(text(), 2); }

_ "whitespace"
    = [ \t\n\r]*
