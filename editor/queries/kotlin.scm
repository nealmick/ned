;;; Keywords - Verified against your AST
; Basic keywords
[
  "package" 
  "import"
  "fun"
  "return"
  "throw"
  "when"
  "else"
  "in"
  "is"
  "as"
  "try"
  "catch"
  "finally"
  "class"
  "interface"
  "object"
  "val"
  "var"
  "enum"
  "typealias"
] @keyword


;; Loops - Added based on AST patterns
(for_statement "for" @repeat)
(while_statement "while" @repeat)
(do_while_statement "do" @repeat)


;; Modifiers - Simplified list
[
  "public"
  "private"
  "protected"
  "internal"
  "open"
  "abstract"
  "final"
  "override"
  "data"
] @keyword

;; Literals - Keep existing working parts
(integer_literal) @number
(real_literal) @float
(string_literal) @string
(boolean_literal) @boolean
(null_literal) @boolean

;; Functions
(function_declaration . (simple_identifier) @function)
(call_expression . (simple_identifier) @function)

;; Types
(type_identifier) @type
(user_type (type_identifier) @type)





;; Punctuation
["(" ")" "{" "}" "[" "]"] @punctuation.bracket
["." "," ";"] @punctuation.delimiter
["->" "=" ">" "<" "&&" "||" "+" "-" "*" "/"] @operator