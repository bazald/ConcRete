(make @S1 ^attr 13)
(p rule-a (@S1 ^attr <r>)-->(write |Hello | <r> | world!| (crlf))<--(write |Goodbye | <r> | world!| (crlf)))
(p rule-b (@S1 ^attr 13)-->(make @S1 ^attr 14))
(p rule-c (@S1 ^attr <v>)-->(genatom)(cbind <d>)(p <d> (@S1 ^attr <v>)-->(write |Subcommand | <v> | fired!| (crlf))<--(write |Subcommand | <v> | retracted!| (crlf)))<--(excise <d>))
(p rule-e (@S1 ^attr <v1>)-->(genatom)(cbind <f>)(p <f> (@S1 ^attr <v1> ^attr <v2>)-->(genatom)(cbind <g>)(p <g> (@S1 ^attr <v1> ^attr <v2>)-->(write |Subcommand | <v1> |-| <v2> | fired!| (crlf))<--(write |Subcommand | <v1> |-| <v2> | retracted!| (crlf)))<--(excise <g>))<--(excise <f>))
(p rule-h (@S1 ^int <<1 2 3 4 5>>)(@S1 ^name <<a b c d e>>)-->(write X)<--(write Y))
(make @S1 ^name a ^name c ^name d ^name g ^name 3 ^int 2 ^int 4 ^int 5 ^int 6 ^int b)
(excise rule-a rule-b rule-c rule-e rule-h)
(exit)
