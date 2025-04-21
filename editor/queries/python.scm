; ==============================================================================
; Python Highlighting Query
; ==============================================================================

; --- Function Definitions ---
(function_definition
  name: (identifier) @function)

; --- Class Definitions ---
(class_definition
  name: (identifier) @type)

; --- Function/Method Calls ---
(call
  function: (identifier) @function)

(call
  function: (attribute
    attribute: (identifier) @method))

; --- Built-in Exceptions/Classes ---
((identifier) @type.builtin
 (#match? @type.builtin "^[A-Z][A-Za-z0-9_]*$")
 (#is-not? @type.builtin "self|True|False|None"))

; --- Literals & Keywords ---
(comment) @comment
(string) @string
(integer) @number
(float) @number
(true) @boolean
(false) @boolean
(none) @constant.builtin

[
  "def" "class" "return" "if" "else" "elif" "for" "while"
  "import" "from" "as" "try" "except" "finally" "pass"
  "continue" "break" "raise" "with" "lambda" "nonlocal" "global"
] @keyword

[
  "and" "or" "not" "in" "is"
] @keyword.operator

[
  "=" "+" "-" "*" "/" "%" "**" "//" "==" "!=" "<" ">" "<=" ">="
  ":=" "+=" "-=" "*=" "/=" "%=" "**=" "//=" "&" "|" "^" "~" "<<" ">>"
  "@" "->"
] @operator

; --- Special Variables ---
((identifier) @variable.builtin
 (#eq? @variable.builtin "self"))

; --- Decorators ---
(decorator
  "@" @punctuation.special
  (identifier) @function)

; --- Type Annotations ---
(type (identifier) @type)

; --- Exceptions ---
(except_clause
  (identifier) @type)
