hikocsp - C++ Server Pages (CSP)
================================

hikocsp.exe usage
-----------------

### Synopsis
```
hikocsp --input=filename.csp --output=filename.hpp
```

  Argument           | Description
 :------------------ |:-----------------------
  \-\-input=<path>   | The filename of the template. recommended extension is .csp
  \-\-output=<path>  | The filename of the result. extension: .cpp, .hpp, .ixx
  
CSP Template format
-------------------
  
### `$<` block `$>`
A CSP template starts in C++ verbatim-mode, it switches to block-mode when the
parser encounters `$<` outside of a quoted-string. The parser switches back
to C++ verbatim-mode when it encounters `$>`.
  
All characters in C++ verbatim-mode are directly placed in the generated code.
  
The block-mode contains text, but also placeholders and short pieces of C++ code.
Code will be generated for text and placeholders to write to the `_out` variable.
The `_out` variable is of type `std::string` (or equivilant).
  
### `${` expression `}`
The placeholder is for writing the result of a C++ expression to `_out`.
The resulting generated code of a single-expression place-holder is as follows:
  
```cpp
_out += std::format("{}", expression);
```

The expression must be a valid and complete C++ expression.
_This rule exists to determine when a `}` can terminate the place-holder._
  
### `${` format-string `,` expression... `}`
A placeholder with multiple arguments can be used to format text using a format-string.
The resulting generated code of a format-placeholder is as follows:
  
```cpp
_out += std::format(format-string, expression...);
```

The expression must be a valid and complete C++ expression.
_This rule exists to determine when a `}` can terminate the place-holder._

### `$` C++ line `\n`
Short pieces of verbatim C++ code can be added to the generated code using this syntax.
    
Syntax
------
  
```
template := ( verbatim | block )*
 
block := '$<' command* '$>'
command := text | placeholder | cpp-line | dollar-escape | newline-escape
  
text := [^$]*
placeholder := '${' expression ( ',' expression )* '}'
cpp-line := '$' [^{>$] verbatim '\n'
dollar-escape := '$$'
newline-escape := '$' [ \t\r] [\n\f\v]

# cpp-expression is a C++ expression.
# A C++ expression may be terminated when the resulting expression is valid.
expression := ...

# cpp-verbatim is a piece of C++ code.
# Verbatim C++ code may be terminated at any time, except inside a quoted string.
verbatim := ...
```
  
  
