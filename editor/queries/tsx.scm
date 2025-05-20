;; TSX Queries - Final Corrected
;; =============================

[
  (string) @string
  (template_string) @string
  (template_substitution) @punctuation.special
]

;; JSX Elements =================================
(jsx_opening_element
  name: (identifier) @tag)  ;; Components as tags

(jsx_self_closing_element
  name: (identifier) @tag)

(jsx_closing_element
  name: (identifier) @tag)

(jsx_attribute
  (property_identifier) @attribute)  ;; JSX specific attributes

;; Object Properties & Methods ==================
(pair
  key: (property_identifier) @property)  ;; Object keys

(shorthand_property_identifier) @property  ;; Destructured props

;; Function Calls ===============================
(call_expression
  function: (identifier) @function)

;; Special React Hooks ==========================
(call_expression
  function: (identifier) @hook
  (#match? @hook "^use[A-Z]"))

;; Variables & Parameters =======================
(identifier) @variable
(required_parameter
  (identifier) @variable.parameter)

;; Type Annotations =============================
(type_identifier) @type
(generic_type
  (type_identifier) @type)

;; Import/Export ================================
(import_specifier
  name: (identifier) @function)  ;; Imported functions

(export_statement) @keyword

;; Keywords =====================================
[
  "if" "else" "for" "while" "do" "switch" "case" "default"
  "break" "continue" "return" "try" "catch" "finally" "throw"
  "const" "let" "var" "function" "class" "new" "import" "export"
  "from" "async" "await" "yield" "typeof" "instanceof" "delete"
  "void" "type" "interface" "enum" "public" "private" "protected"
  "as" "extends" "static" "readonly" "declare" "namespace" "module"
  "get" "set" "abstract" "implements" "asserts" "infer" "is" "keyof"
  "=>" "satisfies" "override" "default"
] @keyword