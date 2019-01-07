# ConcRete
A lock-free, concurrent Rete implementation in C++

## Rete
The <a href="https://en.wikipedia.org/wiki/Rete_algorithm">Rete</a> algorithm efficiently evaluates if-then rules, sharing work between rules that share conditions and caching intermediate results to reduce computation required from step to step.

Previous efforts to parallelize Rete matching include <a href="http://www.unf.edu/public/cap4630/kmartin/gradfall94/CParaOPS5/">CParaOPS5</a> and <a href="http://mas.cs.umass.edu/Documents/Neiman/thesis.pdf">UMass Parallel OPS5 (UMPOPS)</a>.
Both CParaOPS5 and UMPOPS require precompilation of rules into C and subsequently into an executable binary.
This approach does not lend itself to modification of the rule set for running agents.
This also eliminates any concerns regarding concurrency of rule modifications for running agents.
(Dynamic compilation of ConcRete rules using <a href="https://llvm.org/">LLVM</a> could be an interesting future direction.)

<a href="include/Zeni/Rete">ConcRete</a> supports concurrent insertion and removal of working memory elements (WMEs) and rules as well.
While CParaOPS5 avoided race conditions through its rule scheduler and UMPOPS avoided race conditions through elaborate locking schemes that the rules themselves supported, ConcRete has a lock-free design that guarantees no race conditions regardless of scheduling of the supported operations.

The ConcRete alpha network is unusual in that alpha network tests are built into the hash map one level higher up.
As a result, conditions consisting of three variables are unsupported.
Alpha and beta nodes provided include:
* Filter nodes on the first slot of a WME (the "id" in the 3-tuple &mdash; implicitly encoded at the top level only)
* Filter nodes on the second slot of a WME (the "attribute" in the 3-tuple)
* Filter nodes on the third slot of a WME (the "value" in the 3-tuple)
* Predicate nodes that test whether a variable is equal to / less than / ... another variable or a constant value
* Join nodes that test whether variables on the left match variables on the right
* Existential join nodes that pass the left token conditionally on one or more matches on the right
* Negation join nodes that pass the left token conditionally on non-existence of matches on the right
* Action nodes that take C++ function objects

The concurrent construction, deduplication, removal, and left/right-unlinking of these nodes in addition to WME insertion and removal is supported through the use of a data structure that combines variants of <a href="https://en.wikipedia.org/wiki/Ctrie">Ctries</a> with variants of more traditional <a href="https://en.wikipedia.org/wiki/Hash_array_mapped_trie">Hash array mapped tries (HAMTs)</a>.
Conceptually, each node in the Rete includes a concurrent hash map in which each value is set of further concurrent hash maps.
The internal concurrent hash maps must share a linearization point in order to allow simultaneous insertion+snapshot and removal+snapshot operations across the set of concurrent hash maps.
The outer concurrent hash map relaxes that requirement and allows for reduced contention within a given node.
Data structures involved can be found in <a href="include/Zeni/Concurrency/Container/Lockfree/">include/Zeni/Concurrency/Container/Lockfree/</a>.

None of the Ctrie or HAMT variants used by ConcRete requires a garbage collector to avoid leaking memory.
Epoch-based memory reclamation is sufficient for delaying of object destruction until all accessors are done using internal objects.
I believe that makes my Ctrie implementations the first to function without memory leaks without the use of a garbage collector.

## Console
Console provides a shell interface to ConcRete. It acts like a rule-based system that is descended from the <a href="http://www.dtic.mil/dtic/tr/fulltext/u2/a106558.pdf">OPS5</a> architecture by Charles L Forgy, both in terms of syntax and in that it uses a Rete derivative, which is the focus of this project.

The functions currently provided are defined by the <a href="include/Zeni/Rete/Parser">Parser</a> (implemented using the excellent <a href="https://github.com/taocpp/PEGTL">PEGTL</a> project) and include:
* (cbind <em>heretofore unbound variable</em>)
* (excise <em>rule names</em>)
* (excise-all)
* (exit)
* (genatom) # never generates the same string consisting solely of alphanumeric symbols twice
* (make <em>WMEs (Working Memory Elements)</em>)
* (p rule-name <em>conditions</em> --> <em>actions</em> <-- <em>retractions</em>)
* (remove <em>WMEs</em>) # notably different from OPS5 which used indices to refer to existing WMEs
* (source <em>filename</em>) # recursive relative path following not yet implemented
* (write <em>constants and bound variables</em>)

A simple production that prints WMEs that match 42 in the third slot could be written: <br/>
```
(p print-wme-42 <br/>
  (<a> ^<b> 42) <br/>
--> <br/>
  (write <a> | ^| <b> | 42| (crlf)) <br/>
)
```

A production that adds a WME to working memory only as long as it is supported: <br/>
```
(p add-temporarily <br/>
  (<query> ^sum <sum>) <br/>
  (<sum> ^left <l>) <br/>
  (<sum> ^right <r>) <br/>
--> <br/>
  (<l> + <r>) <br/>
  (cbind <output>) <br/>
  (make <sum> ^equals <output>) <br/>
<-- <br/>
  (remove <sum> ^equals <output>) <br/>
)
```

A production that adds another production to working memory only as long as it is supported: <br/>
```
(p rule-maker <br/>
  (subrule ^create true) <br/>
--> <br/>
  (genatom) <br/>
  (cbind <subp>) <br/>
  (p <subp> <br/>
    (subrule ^write true) <br/>
  --> <br/>
    (write |Hello, world!| (crlf)) <br/>
  ) <br/>
<-- <br/>
  (excise <subp>) <br/>
)
```

Note that if you wish to type these rules into the Console rather than sourcing them, it will be necessary to remove the line-breaks.
Console makes no effort to delay parsing until rule completion, and will reject these rules when split across multiple lines.
