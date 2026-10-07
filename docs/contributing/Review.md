# Code review

How a changeset is reviewed, what counts as a finding against it, and what a fix has to do. It
is for anyone reviewing a change, and for the reviewer agents in `.claude/agents/`.

## What makes a review clean

- **A changeset answers for what it introduces.** A finding is a defect the changeset added, or
  a defect in a line it changed. A defect that was already there, in a file the change touched or
  anywhere else, is not a finding against the change.
- **A changeset merges only when its review is clean.** Clean means no finding at any severity.
  A finding the change introduced is fixed in the change, not deferred.
- **An older defect goes in the known-debt list**, under "Known review findings" in
  [TODO.md](../TODO.md), with its class. A reviewer reads that list first and does not report an
  entry on it again.
- **A change that moves or rewrites old code answers for what it rewrote.** Moving a file
  unchanged introduces nothing. Rewriting a function makes its defects the change's, unless the
  reviewer can show the old function had the same defect, in which case it goes on the debt list.

Background: [ADR-0083](../adr/0083-review-a-changeset-answers-for-what-it-introduces.md)

## What a reviewer reads

- The diff of the changeset, and every site of each pattern the diff touches. When a change adds
  a check to one loader, the reviewer reads every loader with the same shape.
- The reference documents for whatever the diff changes, to see that each rule the change moved
  is stated, and that the statement matches the code.
- The known-debt list.

A review ends with a line naming exactly what was read in full and what was read only as a diff.
A review that says "no findings" without that line has not shown it covered the change.

## What a finding records

| Field | Meaning |
|---|---|
| Class | One of the classes below |
| Severity | Major, if the change can produce a wrong result, a crash, a leak or undefined behaviour in a reachable case. Minor otherwise |
| Where | File and line |
| Defect and failure | What is wrong, and a concrete case where it goes wrong |
| Siblings | The other sites of the same pattern, and whether each has the defect |

## The classes

Most findings fall into a few shapes. The ones a tool can find are checked by a tool, and a
reviewer reports them only when the tool missed one.

| Class | What it covers | Checked by |
|---|---|---|
| **Boundary input** | A value from a file, the command line or a caller converted without a check: a float made an integer, a JSON value read as a string without testing its type, a count used against a buffer, a value that is not finite | `scripts/boundary.ts` on added lines, and review |
| **Drift** | A comment or document that no longer matches the code, or a claim a change makes that the code does not keep | Review |
| **Prose** | The writing rules in [Conventions.md](Conventions.md#writing): sentence length, openers, inverted sentences, figures of speech, lines extended past the wrap | `scripts/prose.ts`, on the lines a change adds |
| **Weak test** | A test that would pass without the code it claims to test | `scripts/failsfirst.ts`, and review |
| **Cleanup** | An object a constructor made before it threw, a null a function was given, a destructor that can throw, a resource never released | Review |
| **Build** | A target that reaches a header through another library's link, a link with the wrong visibility, a source missing from its list | `scripts/linkrule.ts` for apps and suites, and the configure checks in [Build.md](Build.md) for api libraries |
| **Contract** | Two parts of the code that each look right and disagree about a rule between them | Review |
| **Semantics** | Behaviour that differs from the specification the code implements, such as RenderMan or glTF | Review, and the conformance tests |

## What a fix has to do

- **Fix the pattern, not the site.** A fix names the pattern it corrects and checks every site of
  it in the same change. The commit message lists the sites it checked.
- **Test what the fix claims.** A test fails before the fix and passes after it. When a fix adds
  a guarantee to a header or a document, a test exercises that guarantee as it is written, in the
  case the words cover. A guarantee about a textured mesh needs a textured mesh in its test. A
  test of a defect that only the change itself makes possible passes before the change. Its doc
  comment then has a line that starts `Passes before the change:` and names what the change adds
  that makes the defect possible. [Testing.md](Testing.md#checking-that-a-new-test-fails-first)
  shows the form.
- **State the rule once.** A fix that changes a rule updates the reference document that owns it
  and the comment beside the code, and adds no other copy.

## Running a review

The `cpp-reviewer` agent reviews C++ against this page and the conventions. For a changeset of
any size, one review of the whole diff comes first. After a round of fixes, the next review reads
only the fixes and their sibling sites.
