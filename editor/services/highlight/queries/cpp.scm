;; ==============================================================================
;; C++ Highlighting Query
;; ==============================================================================

;; Comments
(comment) @comment

;; Preprocessor Directives
(preproc_include) @keyword ; Highlight #include as a keyword
(preproc_include
  path: [
    (string_literal) @string ; Highlight paths in quotes (e.g., "editor_cursor.h")
    (system_lib_string) @string ; Highlight paths in angle brackets (e.g., <algorithm>)
  ])
(preproc_def name: (identifier) @constant.macro) ; Highlight the macro name in #define
[
  (preproc_directive)
  (preproc_if)
  (preproc_ifdef)
  (preproc_else)
  (preproc_elif)
  (preproc_call)
  (preproc_def)
  (preproc_function_def)
] @preprocessor

;; Keywords
[
  "alignas"
  "alignof"
  "asm"
  "break"
  "case"
  "catch"
  "class"
  "co_await"
  "co_return"
  "co_yield"
  "concept"
  "const"
  "consteval"
  "constexpr"
  "constinit"
  "continue"
  "decltype"
  "default"
  "delete"
  "do"
  "else"
  "enum"
  "explicit"
  "export"
  "extern"
  "final"
  "for"
  "friend"
  "goto"
  "if"
  "import"
  "inline"
  "module"
  "mutable"
  "namespace"
  "new"
  "noexcept"
  "operator"
  "override"
  "private"
  "protected"
  "public"
  "register"
  "requires"
  "return"
  "sizeof"
  "static"
  "static_assert"
  "struct"
  "switch"
  "template"
  "thread_local"
  "throw"
  "try"
  "typedef"
  "typename"
  "union"
  "using"
  "virtual"
  "volatile"
  "while"
] @keyword

;; Operators
[
  "--"
  "-"
  "-="
  "->"
  "&"
  "&="
  "&&"
  "+"
  "++"
  "+="
  "<"
  "="
  "=="
  "!="
  ">"
  ">>"
  "|"
  "||"
  "?"
  "^"
  "~"
  "*"
  "*="
  "/"
  "/="
  "%"
  ":"
  "::"
  "."
  ".*"
  "->*"
] @operator

;; Punctuation
[
  ";"
  ","
] @punctuation.delimiter

[
  "("
  ")"
  "["
  "]"
  "{"
  "}"
] @punctuation.bracket

;; Literals
(string_literal) @string
(raw_string_literal) @string
(char_literal) @string.special ; Or just @string
(escape_sequence) @string.escape

(number_literal) @number

(true) @constant.builtin
(false) @constant.builtin

;; Built-in Variables/Keywords acting like variables
(this) @variable.builtin

;; === Types ===

;; Primitive Types
(primitive_type) @type ; int, float, void, bool, char, etc.
(auto) @type ; Treat auto like a type specifier

;; User-Defined Types
(class_specifier name: (type_identifier) @type)
(struct_specifier name: (type_identifier) @type)
(union_specifier name: (type_identifier) @type)
(enum_specifier name: (type_identifier) @type)

;; Type Definitions
(alias_declaration name: (type_identifier) @type) ; using MyType = ...
(type_definition declarator: (type_identifier) @type) ; typedef ... MyType;

;; Type Usage (General)
(type_identifier) @type

;; === Function Declarations/Definitions ===

;; Function Definitions
(function_definition
  declarator: (function_declarator
    declarator: (identifier) @function)) ; Function name in definitions

;; Function Declarations
(declaration
  declarator: (function_declarator
    declarator: (identifier) @function)) ; Function name in declarations

;; Method Definitions
(function_definition
  declarator: (function_declarator
    declarator: (field_identifier) @function)) ; Method name in definitions

;; Method Declarations
(declaration
  declarator: (function_declarator
    declarator: (field_identifier) @function)) ; Method name in declarations

;; Function Calls
(call_expression
  function: (identifier) @function)
(call_expression
  function: (field_expression field: (field_identifier) @function)) ; obj.method()
(call_expression
  function: (qualified_identifier name: (identifier) @function)) ; namespace::func()
(call_expression
  function: (template_function name: (identifier) @function)) ; func<T>()

;; Templates
(template_declaration) @keyword
(template_parameter_list) @type
(template_function
  name: (identifier) @function)
(template_method
  name: (field_identifier) @function)

;; Namespaces
(using_declaration (qualified_identifier) @type)

;; Identifiers (General Fallback)
(identifier) @variable
(field_identifier) @function

;; Constants (Enum members)
(enumerator (identifier) @constant)

;; Attributes
(attribute_specifier) @function
(attribute (identifier) @function)

;; Concepts (C++20)
(concept_definition name: (identifier) @type)

;; Qualified Identifiers (Refinement)
(qualified_identifier
  scope: (namespace_identifier) @type
  name: (identifier) @variable)
(qualified_identifier
  name: (identifier) @variable)
