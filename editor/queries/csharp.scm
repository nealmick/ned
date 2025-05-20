; Keywords
[
  "using"
  "namespace"
  "class"
  "public"
  "private"
  "protected"
  "readonly"
  "async"
  "new"
  "if"
  "else"
  "return"
  "get"
  "set"
  "var"
  "for"
  "foreach"
  "while"
  "do"
  "try"
  "catch"
  "finally"
  "throw"
  "this"
  "base"
] @keyword

; Strings
(string_literal) @string

; Comments
(comment) @comment


; Functions/Methods
(method_declaration
  name: (identifier) @function
)

; Parameters
(parameter
  name: (identifier) @variable.parameter
)

; Variables
(field_declaration
  (variable_declaration
    (variable_declarator
      name: (identifier) @variable
    )
  )
)

(local_declaration_statement
  (variable_declaration
    (variable_declarator
      name: (identifier) @variable
    )
  )
)

; Attributes
(attribute) @attribute

; Properties
(property_declaration
  name: (identifier) @property
)

; Object creation
(object_creation_expression
  type: (identifier) @type
)

; Method invocations
(invocation_expression
  function: (member_access_expression
    name: (identifier) @function.call
  )
)

; Namespaces
(namespace_declaration
  name: (qualified_name) @namespace
)

; Using directives
(using_directive
  (qualified_name (identifier) @namespace)
)


; LINQ keywords
(query_expression
  [
    "from"
    "where"
    "select"
    "group"
    "orderby"
    "join"
    "let"
    "ascending"
    "descending"
  ] @keyword
)

; Lambda expressions
(lambda_expression
  "=>" @operator
)

; Await expressions
(await_expression
  "await" @keyword
)

; Member access
(member_access_expression
  name: (identifier) @property
)

; Constructors
(constructor_declaration
  name: (identifier) @function
)