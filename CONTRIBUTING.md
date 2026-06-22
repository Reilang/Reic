# Contributing to ReiC

Thanks for your interest in contributing!

## Current phase

ReiC is in the **conceptual design phase**. The language syntax, semantics,
and compilation pipeline are all still taking shape. At this stage, most
valuable contributions are **design discussions and concept exploration**
rather than code — open an issue to talk through ideas before writing
anything substantial.

## AI-assisted contributions

We welcome LLM-assisted work, but with clear ground rules.  Please read
[AI_POLICY.md](AI_POLICY.md) before submitting anything that involved an LLM.
The short version: disclose it, write PR descriptions and comments yourself,
and understand every line you submit.

## Getting started

1. Open an issue to discuss what you'd like to work on
2. Read `CLAUDE.md` for project conventions (C11, K&R, naming, etc.)
3. Build with `meson setup builddir && ninja -C builddir`
4. Make sure the compiler stays at zero warnings

## License

ReiC is [GPLv3+](LICENSE). By contributing, you agree that your work will be
distributed under the same license.
