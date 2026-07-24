; editor/queries/hcl.scm
; Final corrected version

; Blocks - capture block type identifiers
(block (identifier) @type)  ; "terraform", "provider", "resource" etc

; Block labels (string literals after block type)
(block (string_lit) @string)  ; "google", "cloud_run_service" etc

; Functions
(function_call (identifier) @function)  ; "file", "toset", "split"

; Variables and identifiers
(identifier) @variable

; Literals
(string_lit) @string
(numeric_lit) @number
((identifier) @keyword
 (#match? @keyword "^(true|false)$"))

; Comments
(comment) @comment

; Template syntax
(template_interpolation_start) @operator
(template_interpolation_end) @operator

; Attributes (left side of =)
(attribute (identifier) @property)  ; "bucket", "credentials", "region"

; Special blocks
(block (identifier) @type
 (#match? @type "^provider$|^resource$|^data$|^module$|^variable$|^output$|^locals$"))

; Punctuation
["{" "}" "(" ")" "[" "]"] @punctuation.bracket
"=" @operator

; Fixed index access
(new_index
  "[" @punctuation.bracket
  (expression (literal_value (numeric_lit) @number))
  "]" @punctuation.bracket)

; Fixed attribute access
(get_attr
  "." @punctuation.delimiter
  (identifier) @property)

; String templates
(quoted_template_start) @string
(quoted_template_end) @string
(template_literal) @string