---
title: "Grammar for X++"
weight: 1
---

This is a CFG for the X++ programming language. It does NOT specify semantic rules, those are loosely
defined in other language documentation.

{{< katex display >}}
\begin{align}
\textit{program} &\to \textit{import-stmts}^* (\textit{function }|\textit{ global-var }|\textit{ enum })^*  \\ \\

\textit{import-stmts} &\to \textbf{import } \textit{STRINGLITERAL} \textbf{ as } \textit{ident} \text{ ";"} \\ 
\textit{import-stmts} &\to \textbf{import } \textit{ident } (\textbf{ as } \textit{ident})? (\text{"," } \textit{ident})? \text{ ";"} \\

\textit{global-var} &\to \textbf{pub}? \textbf{ let } \textbf{mut}? \textit{ ident } (\text{ ":" type})? (\text{"="} expr)? \text{";"}\\
\textit{local-var} &\to \textbf{let } \textbf{mut}? \textit{ ident } (\text{ ":" type})? (\text{"="} expr)? \text{";"}\\
\textit{function} &\to \textbf{pub}? \textbf{ fn } \textit{ ident} \text{ "\\("} \textit{ para-list} \text{ "\\)"} \textit{ type } \textit{compound-stmt }\\
\textit{enum} &\to \textbf{pub}? \textbf{ enum} \textit{ ident } \textit{"\{ "} \textit{ident} (\textit{"," ident})^*\textit{ " \}"}\\

\text{compound-stmt} &\to \text{"\\\{" } \textit{stmt }^* \text{ "\\\}"} \\
\text{stmt} &\to
\begin{cases}
\textit{local-var-stmt} \\
\textit{return-stmt} \\
\textit{while-stmt} \\
\textit{if-stmt} \\
\textit{compound-stmt} \\
\textit{loop-stmt} \\
\textit{break-stmt} \\
\textit{continue-stmt} \\
\textit{expr-stmt} \\
\textit{empty-stmt} \\
\end{cases} \\
\textit{if-stmt} &\to \textbf{if} \textit{ expr compound-stmt } (\textbf{else if } \textit{expr compound-stmt})^* (\textbf{else} \textit{
stmt})? \\
\textit{while-stmt} &\to \textbf{while} \textit{ expr compound-stmt} \\
\textit{break-stmt} &\to \textbf{break} \text{ ";"} \\
\textit{continue-stmt} &\to \textbf{continue } \text{ ";"} \\
\textit{return-stmt} &\to \textbf{return} \textit{ expr}? \text{ ";"} \\
\textit{expr-stmt} &\to \textit{expr} \text{ ";"} \\
\textit{loop-stmt} &\to \textbf{ loop } \textit{ident } (\textbf{in} \textit{ expr} (\text{ ", "}\textit{ expr?})?)? \textit{ compound-stmt}\\

\textit{expr} &\to (\textit{assignment-expr } || \textit{size-of-expr}) ("as" ( \text{"(" } \textit{type} \text{ ")"}|| \textit{expr}))? \\
\textit{assignment-expr} &\to \textit{logical-or-expr } || \textit{unary-expr} \textbf{ ASSIGNMENT-OPERATOR } \textit{assignment-expr}\\  
\textit{logical-or-expr} &\to \textit{logical-and-expr } (\text{"||" } \textit{logical-and-expr})^* \\
\textit{logical-and-expr} &\to \textit{equality-expr } (\text{"\\\&\\\&" } \textit{equality-expr})^* \\
\textit{equality-expr} &\to \textit{relational-expr } ((\text{"==" } | \text{ "!=" }) \textit{ relational-expr})^* \\
\textit{relational-expr} &\to \textit{additive-expr } ((\text{"<" } | \text{ "<=" } | \text{ ">" } | \text{ ">=" })
\textit{ additive-expr})^* \\
\textit{additive-expr} &\to \textit{mult-expr } ((\text{"-" } | \text{ "+" }) \textit{ mult-expr})^* \\
\textit{mult-expr} &\to \textit{unary-expr } ((\text{"\%" } | \text{"*" } | \text{ "/" }) \textit{ unary-expr})^* \\
\textit{unary-expr} &\to
\begin{cases}
\textit{size-of-expr} \\
\textit{prefix-expr} \\
\textit{array-init-expr} \\
\textit{postfix-expr} \\
\end{cases} \\
\\

\textit{size-of-expr} &\to \textbf{ size\_of} ("(" \textit{ type } ")" | (\textit{ expr})) \\
\textit{prefix-expr} &\to \textbf{UNARY-OPERATOR} \textit{ expr} \\
\textit{array-init-expr} &\to \text{"[" } (\textit{ expr} (\text{", "} \textit{ expr})^*)^* \text{"]" } \\

\textit{postfix-expr} &\to
\begin{cases}
\textit{expr } \textbf{POSTFIX-OPERATOR} \\ 
\textit{func-call} \\
\textit{array-index} \\
\textit{import-expr} \\
\textit{field-access} \\
\textit{method-access} \\
\textit{enum-expr} \\
\textit{STRINGLITERAL} \\
\textit{INTLITERAL} \\ 
\textit{UINTLITERAL} \\ 
\textit{FLOATLITERAL} \\
\textit{BOOLLITERAL} \\ 
\textit{CHARLITERAL} \\
\textit{ident} \\
\text{"(" } \textit{expr} \text{ ")"} \\
\end{cases} \\
\\

\textit{func-call} &\to \textit{ident } "(" \textit{args} ")" \\
\textit{array-index} &\to \textit{expr } "[" \textit{expr} "]" \\
\textit{field-access} &\to \textit{expr } "." \textit{ident} \\
\textit{method-access} &\to \textit{expr } "." \textit{ident} \textit(args) \\
\textit{enum-expr} &\to \textit{expr } \text{ "::" }\textit{ident} \\
\textit{import-expr} &\to \textit{expr } \text{ "::" } \textit{expr} \\

\textit{args} &\to \textit{expr } (\text{","} \textit{ expr})^* \text{ | } \epsilon\\
\textit{para-list} &\to \textit{arg } (\textit{"," arg})^*  \text{ | } \epsilon \\ \\
\textit{arg} &\to \textbf{mut}? \textit{ ident } \text{ ":" } \textit{ type}\\

\textit{ident} &\to \textbf{letter} (\textbf{letter } | \textbf{ digit})^* || \textit{ \$} \\
\textit{type} &\to \textbf{ident u8 | u32 | u64 | i8 | i32 | i64 | void | bool | f32 | f64 | \textit{type}* | \textit{type}[\textit{INTLITERAL}] } \\

\textit{INTLITERAL} &\to [0-9]+ \\
\textit{UINTLITERAL} &\to [0-9]+u \\
\textit{FLOATLITERAL} &\to [0-9]^+\textit{"."}[0-9]? \\
\textit{STRINGLITERAL} &\to \textit{"} \textit{ident} \textit{"} \\
\textit{BOOLLITERAL} &\to true | false \\
\textit{CHARLITERAL} &\to \textit{'ident'} \\
\textit{ASSIGNMENT-OPERATOR} &\to \textbf{ "==" "+=" "-=" "*=" "/=" }\\
\textit{UNARY-OPERATOR} &\to \textbf{ "++" "--" "+" "-" "*" "\&"} \\
\textit{POSTFIX-OPERATOR} &\to \textbf{ "++" "--" } \\

\end{align}
{{< /katex >}}