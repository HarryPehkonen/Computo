# CLAUDE.md — proposed changes (Hermes, 2026-10-16, card t_79c842b6)

Claude Code owns `CLAUDE.md`, so the two edits below are NOT applied. They are what the file
would say after this card: the documentation validation moved into the gate's fast tier, and
`.github/workflows/docs.yml` — the file the paragraph's stage list was written against — is
deleted.

## 1. The "Gates before any change is done" paragraph, first sentence

Current text (as committed at `ea30600`):

> Gates before any change is done — THE gate is `scripts/gate.sh`: one script, three callers
> (by hand, `.githooks/pre-commit` for the fast tier `build tests`, `.githooks/pre-push`
> for the full tier, armed once per clone with `git config core.hooksPath .githooks`).

Two things are wrong with it now. The fast tier is `format build tests docs` — `format` has
been in it since the gate became gate.toml and was never listed here, and `docs` joined it on
2026-10-16. And a list written into this file is a second home for a list gate.toml already
owns: `kit-ci --list` prints it, which is why the hooks name a tier instead of spelling one out.

Proposed:

> Gates before any change is done — THE gate is `scripts/gate.sh`: one script, three callers
> (by hand, `.githooks/pre-commit` for the fast tier, `.githooks/pre-push`
> for the full tier, armed once per clone with `git config core.hooksPath .githooks`).
> The tiers are gate.toml's `[tier.fast]` and `[tier.full]`, which the hooks NAME — so this
> paragraph no longer lists them; run `kit-ci --list` to see what each holds. (Fast is
> `format build tests docs`: `docs` joined on 2026-10-16 so that documentation drift fails at
> commit time on this machine, since the workflow that used to run those checks is gone.)

## 2. The closing paragraph of the same section

Current text:

> Those seven are not the whole gate: it also runs `tree`, `version` and `kitprobes`,
> plus two stages this repo added and the kit does not have — `release` (the same suite
> in an optimized `-O3 -DNDEBUG` configuration) and `docs` (the documented examples,
> operator coverage and the generated reference; it FAILS rather than skips when
> python3/PyYAML are missing). Tooling status — the commands, and where each stage's
> evidence lives — is in CODING_STANDARDS.md.

`kitprobes` was deleted in the 2026-10-06 conversion (the closing note in gate.toml records it);
the sentence has been stale since then, and this card is the first reason to touch it.

Proposed:

> Those seven are not the whole gate: it also runs `tree` and `version`, plus three stages this
> repo added and the kit does not have — `release` (the same suite in an optimized
> `-O3 -DNDEBUG` configuration), `docs` (the documented examples, operator coverage and the
> generated reference; it FAILS rather than skips when python3/PyYAML are missing) and
> `package` (deb/rpm built from source). Documentation is validated locally — `docs` is in the
> fast tier — and published locally with `scripts/docs-publish.sh`, which builds `_site/`,
> commits it to the `gh-pages` branch and points GitHub Pages at that branch; no GitHub Actions
> runs in this repo. Tooling status — the commands, and where each stage's evidence lives — is
> in CODING_STANDARDS.md.

## Not proposed

- Any change to the seven numbered items in that section: none of them is affected.
- Re-pointing the stage-by-stage evidence: CODING_STANDARDS.md needs no change for this card
  (it names the fast tier without listing it, and its `docs` paragraph is still accurate).
