# Note Template

Copy the skeleton into `NN-folder/topic.md`, fill it in, and delete the comments. The style rules explain the conventions behind it, and the checklist at the end is the acceptance test.

It combines the layout of [array.md](../01-basic/array.md), the Why / What / How / Relation / Example framework, and the Markdown guidance listed under Sources.

## Skeleton

````markdown
# Topic Name

<!-- Optional: a 1-3 sentence TL;DR as a blockquote, only when Why and What do not fit on one screen. -->

## Why

<!-- The problem it solves, and why the obvious alternative is not enough. Explanation only. -->

## What

<!-- One-sentence definition, then the model: a formula, a table or a small diagram in a `text` fence. -->

## How

### i. Key concepts

<!-- Terms that are easy to confuse: put the pair side by side. Explanation only. -->

### ii. Mechanism

<!-- Steps or semantics as a numbered list, one action per item. Use #### 1. only when a ### section needs splitting, and go no deeper. -->

### iii. Syntax and signatures

<!-- Lookup material: declarations, forms, tables. Terse, no argument. -->

### iv. Usage

<!-- Short snippets. Every snippet carries a comment saying what it shows. -->

```cpp
int* p = new int[3];   // what this line shows
delete[] p;
```

## Relation

### i. Issues

<!-- What goes wrong without it, or what it is confused with. -->

### ii. Connection

<!-- What solves those issues, and how. -->

**See also:** [Other note](../NN-folder/other.md): why it is related.

## Example

<!-- A complete program that compiles and runs, then its output, then what the output verifies. -->

```cpp
// program
```

Output:

```text
program output
```

Verifies:

- the claim in the note that this output confirms

## Key Points

<!-- Optional, as in array.md: layered summary, common mistakes, one-sentence summary. -->

## Note

<!-- Optional, as in array.md: deep dives that would break the flow (history, terminology, derivations). -->

## Reference

- cppreference: [page title](https://en.cppreference.com/w/cpp/...)
- C++ standard (draft), by claim:
  - [expr.new](https://eel.is/c++draft/expr.new): the claim it backs
````

## Style rules

1. **One H1, first.** The file starts with a single `#` title, followed by `##` sections. Headings use the `#` style, have a blank line above and below, never skip a level, and have unique text. (Google; MD001 MD003 MD022 MD024 MD025 MD041)
2. **Fixed section order.** Why, What, How, Relation, Example, then the optional Key Points and Note, with Reference always last. (array.md puts What before Why; the framework puts Why first.) Inside a `##` section use `### i.` (roman), then `#### 1.` (arabic), and nothing deeper. Use sentence case for `###` and below. (array.md; Google)
3. **One job per subsection:** explain (concepts, mechanism), look up (signatures, tables) or do (usage, example). Do not mix them inside one subsection. (The three categories come from Diataxis: explanation, reference, how-to.)
4. **Code is always fenced, with a language:** `cpp`, and `text` for diagrams and program output. Put a blank line above and below, and a comment on every snippet saying what it shows. (Google; MD031 MD040)
5. **Tables are for uniform two-dimensional data** (forms x properties): short cells, no prose, a blank line above and below. Anything else is a list. (Google; MD055 MD056 MD058)
6. **Ration the emphasis.** Bold the sentence that must be remembered, and lead-in labels. Definitions and side remarks go in a blockquote such as `> **Note:** ...`. Use no raw HTML except comments, and no `> [!NOTE]` alerts: they are a GitHub extension and may show as literal text in other viewers. (array.md; Google; GitHub docs)
7. **Whitespace.** No trailing spaces, no tabs, at most one blank line in a row, exactly one newline at the end of the file. Start a new paragraph after a blank line, not after two trailing spaces. (MD009 MD010 MD012 MD047; GitHub docs)
8. **Do not hard-wrap prose.** One paragraph per line; let the editor wrap it. This deliberately differs from Google's 80-column rule, so a diff shows whole paragraphs. Turn MD013 off if you run markdownlint.
9. **Links.** Descriptive link text (never "here"), relative paths between notes, no bare URLs. The Reference section lists the sources, and each standard section is tied to the claim it backs, the way cppreference ends its pages with References and See also. (Google; MD034)

## Before you commit

- [ ] **Explain:** Why and What fit on one screen, and I can say them without looking.
- [ ] **Model:** What contains a formula, a table or a diagram.
- [ ] **Connect:** Relation says what breaks without it, what fixes that, and links at least one other note.
- [ ] **Verify:** the Example was compiled and run, the real output is pasted, and every Verifies line matches a claim in the note.
- [ ] **Format:** no markdownlint findings, for example `npx markdownlint-cli2 "NN-folder/topic.md"` with MD013 off.

Companions (optional): once a note is stable, add `topic-checklist.md` and `topic-quiz.md`, aligned section by section with the note, as for array.

## Sources

- [Google Markdown style guide](https://google.github.io/styleguide/docguide/style.html): layout, headings, code, tables, links, whitespace
- [markdownlint rules](https://github.com/DavidAnson/markdownlint/blob/main/doc/Rules.md): the MD numbers cited above
- [Diataxis compass](https://diataxis.fr/compass/): explanation, reference, how-to, tutorial
- [GitHub basic writing and formatting syntax](https://docs.github.com/en/get-started/writing-on-github/getting-started-with-writing-and-formatting-on-github/basic-writing-and-formatting-syntax): alerts, line breaks
- [cppreference operator new](https://en.cppreference.com/w/cpp/memory/new/operator_new): the Notes, References and See also layout
