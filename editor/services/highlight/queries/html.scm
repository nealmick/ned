; html.scm - Working version (no errors)
; Uses existing ThemeColors only

; Tags -------------- 
(tag_name) @type
(erroneous_end_tag_name) @comment

; Script/Style tags
(tag_name) @function @type
  (#match? @function "^(script|style)$")


; Attributes ----------
(attribute_name) @keyword
(attribute_value) @string

; Special values
((attribute_value) @number
  (#match? @number "^[0-9]+(px|em|%|s)?$")) ; Handles CSS units

((attribute_value) @variable 
  (#match? @variable "^#[0-9a-fA-F]{3,6}$")) ; Hex colors

; HTML Basics -----------
(comment) @comment
(doctype) @keyword
(entity) @number
["<" ">" "</" "/>"] @text

; Template tags ---------
((text) @keyword  ; {% ... %}
  (#match? @keyword "^(\\s*\\{%).*(%}\\s*)$"))

((text) @variable ; {{ ... }}
  (#match? @variable "^(\\s*\\{\\{).*(\\}\\}\\s*)$"))
