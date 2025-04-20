;; Comments
(comment) @comment

;; Preprocessor Directives
(preproc_include path: (_) @string.special) ; Highlight the path in #include
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
  (preproc_include)
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
  "<="
  "<<"
  "<<="
  "="
  "=="
  "!="
  ">"
  ">="
  ">>"
  ">>="
  "|"
  "|="
  "||"
  "?"
  "^"
  "^="
  "~"
  "*"
  "*="
  "/"
  "/="
  "%"
  "%="
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

;; Types
(primitive_type) @type.builtin ; int, float, void, bool, char, etc.
(auto) @type.builtin ; Treat auto like a built-in type specifier

;; Type Definitions
(class_specifier name: (type_identifier) @type.definition)
(struct_specifier name: (type_identifier) @type.definition)
(union_specifier name: (type_identifier) @type.definition)
(enum_specifier name: (type_identifier) @type.definition)
(alias_declaration name: (type_identifier) @type.definition) ; using MyType = ...
(type_definition declarator: (type_identifier) @type.definition) ; typedef ... MyType;

;; Type Usage (More general captures)
(type_identifier) @type

;; Function Declarations/Definitions
(function_definition
  declarator: [
    (function_declarator
      declarator: [(identifier) (field_identifier) (operator_name)] @function.definition)
    (pointer_declarator declarator: (function_declarator declarator: (identifier) @function.definition)) ; Function pointers
  ])
(function_declarator ; Standalone declarations
  declarator: [(identifier) (field_identifier) (operator_name)] @function)

;; Function Calls
(call_expression
  function: (identifier) @function.call)
(call_expression
  function: (field_expression field: (field_identifier) @function.call)) ; obj.method()
(call_expression
  function: (qualified_identifier name: (identifier) @function.call)) ; namespace::func()
(call_expression
  function: (template_function name: (identifier) @function.call)) ; func<T>()

;; Templates
(template_declaration) @keyword ; Highlight the 'template' keyword itself
(template_parameter_list) @type.parameter ; Highlight the <...> part
(template_function
  name: (identifier) @function)
(template_method
  name: (field_identifier) @function)


;; Namespaces
(using_declaration (qualified_identifier) @namespace) ; Could also be a variable/function, might need refinement

;; Identifiers (General Fallback - IMPORTANT: Place this near the end)
(identifier) @variable ; General identifier, potentially a variable
(field_identifier) @variable.member ; Member variables (obj.member)

;; Constants (Enum members)
(enumerator (identifier) @constant)

;; Attributes
(attribute_specifier) @attribute
(attribute (identifier) @attribute)

;; Concepts (C++20)
(concept_definition name: (identifier) @type.definition) ; Or maybe @keyword.definition

;; Qualified Identifiers (Refinement)
;; Try to differentiate types and namespaces within qualified identifiers
(qualified_identifier
  scope: (namespace_identifier) @namespace
  name: (identifier) @variable) ; Default to variable if not caught by function/type rules above
(qualified_identifier
  name: (identifier) @variable) ; Default to variable if not caught by function/type rules above

;; Make sure types in qualified identifiers are caught
(qualified_identifier name: (type_identifier) @type)