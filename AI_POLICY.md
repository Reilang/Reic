# AI Policy

We recognize that LLM-based tools are powerful and can meaningfully assist
with writing, reviewing, and refactoring code.  We accept contributions that
involve LLM assistance under the conditions below.

## Disclosure

Any pull request that includes LLM-assisted code must state this clearly in the
PR description.  A simple note such as "portions of the implementation were
written with the help of an LLM" is sufficient.

## Human-authored communication

The PR description itself, and all comments and replies on the PR, must be
written by a human.  LLM-generated text is not permitted in these spaces — even
for translation purposes.  Write your own words.

## Understand your contribution

You must understand every line you submit.  If you cannot explain what a piece
of code does and why it is the right approach, do not include it.  LLM tools
are an accelerator, not a substitute for your own judgment.

## No automated agents

Pull requests submitted by automated agents or bots (e.g. unsupervised CI
scripts that open PRs, auto-fix bots, AI agent loops that commit and push
without human review) will be rejected.  Every PR must be submitted by a human
who stands behind the change.
