# Project instructions

## What this is

A custom-firmware fork of [InfiniTime](https://github.com/InfiniTimeOrg/InfiniTime),
the open-source firmware for the PineTime smartwatch. The `develop` branch is the working
branch, cut from the upstream `v1.16` (1.16.0) tag. Upstream history is periodically merged
in, so **minimize divergence from upstream** — prefer small, additive changes over rewrites,
and leave upstream files untouched when a goal can be met without editing them.

## The four cardinal rules

These are the bedrock. Every other rule in this file builds on them.

1. **Ask, don't assume.** If something is unclear, ask before writing a line. Never make
   silent assumptions about intent, architecture, or requirements.
2. **Simplest solution first.** Implement the simplest thing that could work. No
   abstractions or flexibility that were not asked for.
3. **Don't touch unrelated code.** If a file is not part of the task, do not modify it,
   even if it would be an improvement.
4. **Flag uncertainty explicitly.** If you are not confident about an approach or technical
   detail, say so before proceeding. Confidence without certainty causes more damage than
   admitting the gap.

## Specs and plans

- **Living specs** go in `docs/specs/`. These are tracked documentation — keep them current
  and commit them like any other doc (only when explicitly asked).
  - **Prose only.** Specs describe the design in words: the goal, the decisions, and the
    rationale. Keep code, YAML, shell snippets, and exact file diffs out of specs — those
    belong in the implementation plan.
  - **No dates.** Don't date living specs — no date in the filename, no date line in the
    document. They describe the current design, not a point-in-time snapshot.
- **Implementation plans** go in `docs/plans/` (these may keep a date — they are point-in-time).

## Git

- **Commit only when explicitly asked.** Do not commit or push as a side effect of doing work.
- The `develop` branch is the working branch; do not commit directly to `main`.
- End commit messages with the configured `Co-Authored-By` trailer.

## Building

- Firmware builds inside the public `infinitime/infinitime-build` Docker image; the standard
  entry point is `/opt/build.sh`. There is no need to rebuild that image to build firmware.
- CI configuration lives in `.github/workflows/`.
