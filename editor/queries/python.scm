(call
  function: (identifier) @function
)

; For method calls like object.method() or unpaid_invoices.auto_paging_iter()
(call
  function: (attribute
    attribute: (identifier) @function ; Capture the method name
  )
)


; Keywords
[
  "def" "class" "return" "import" "from" "as" "try" "except" "finally"
  "raise" "if" "elif" "else" "for" "while" "break" "continue" "pass"
  "and" "or" "not" "is" "in" "assert" "del" "global" "nonlocal" "yield" "lambda"
  "async" "await" "with"
] @keyword

; Literals
(true) @keyword
(false) @keyword

(none) @constant.builtin  ; Common category for None
(integer) @number
(float) @number
(string) @string          ; This will also catch f-strings, docstrings by default
(comment) @comment

; Common Operators
[
  "+" "-" "*" "**" "/" "//" "%"
  "<" "<=" ">" ">=" "==" "!="
  "="
  "." ; For attribute access like obj.attr
] @default

; Common Punctuation / Delimiters
[
  ":" ","
  "(" ")" "[" "]" "{" "}"
] @default


; Function Definition
(function_definition
  name: (identifier) @function
)
; Parameters within a function definition
(parameters
  (identifier) @variable.parameter  ; Simple parameter: param
)
(parameters
  (default_parameter name: (identifier) @variable.parameter) ; Param with default: param=value
)


; Class Definition
(class_definition
  name: (identifier) @type ; Class names are often styled as types
)


; Decorators
(decorator
  (identifier) @function.macro ; For simple decorators like @my_decorator
)


; Import Statements
(import_statement ; Covers "import module" and "import package.module"
  (dotted_name) @namespace
)
(import_statement ; Covers "import module as alias"
  (aliased_import
    name: (dotted_name) @namespace
    alias: (identifier) @namespace
  )
)



; Generic identifier - can be a fallback if needed, but use with caution as it can be too broad.
; If you find some variables are not highlighted, you can try uncommenting this.
; (identifier) @variable