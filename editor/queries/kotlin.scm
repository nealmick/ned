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

;; Function-related
(function_declaration "fun" @keyword.function)
(jump_expression "return" @keyword.return)
(throw "throw" @keyword.exception)

;; Control flow
(when "when" @conditional)
(else "else" @conditional)

;; Loops - Added based on AST patterns
(for_statement "for" @repeat)
(while_statement "while" @repeat)
(do_while_statement "do" @repeat)

;; Type declarations
(class_declaration "class" @keyword)
(interface_declaration "interface" @keyword)
(object_declaration "object" @keyword)

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
  "const"
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

;; Comments
(comment) @comment

;; Punctuation
["(" ")" "{" "}" "[" "]"] @punctuation.bracket
["." "," ";"] @punctuation.delimiter
["->" "=" ">" "<" "&&" "||" "+" "-" "*" "/"] @operator