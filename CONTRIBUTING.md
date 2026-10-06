# Contributing

Issues and pull requests are welcome. This page holds the rules the code follows and what a change needs before it is merged. [ARCHITECTURE.md](ARCHITECTURE.md) describes every layer and component, and [HOW_IT_WORKS.md](HOW_IT_WORKS.md) shows the flows.

## Table of Contents

- [The rules](#the-rules)
- [Where code goes](#where-code-goes)
- [Build and test](#build-and-test)
- [Pull requests](#pull-requests)
- [Writing](#writing)

## The rules

Every change keeps to these six. A pull request that breaks one is sent back, however small it is.

| Rule | What it means here |
|---|---|
| iDesign layers | Code lives in the layer its job belongs to: clients, managers, engines, resource access, infrastructure, contracts, core, utilities. Calls only flow downwards. Managers never call each other; the client coordinates them when a use case needs two |
| SOLID | Single responsibility, open/closed, Liskov substitution, interface segregation and dependency inversion all apply. A new backend, audio system, notifier or theme is a new file, and existing ones stay untouched. Contracts stay small: add a new one before widening an old one |
| One type per file | Each type has its own header in `include/<layer>/<type>.h` and its implementation in `src/<layer>/<type>.c`, both named after it |
| Dependency inversion | Managers and clients depend on contracts (a struct of function pointers with a `ctx` pointer), never on a concrete implementation. Every implementation is created in exactly one place, `src/main.c`, and handed in from there |
| Composition over inheritance | Behaviour is assembled from small parts: the composite notifier, the composite event observer, the caching decorators, the audio strategy. Wrap or combine a contract to extend it |
| Separation of concerns | Policy in engines (no I/O), orchestration in managers, I/O at the edges in resource access and infrastructure. Widgets draw, managers decide, stores persist |

## Where code goes

| You are adding | It goes in |
|---|---|
| A rule or decision with no I/O | `engines` |
| A use case that ties rules and I/O together | `managers` |
| Something that reads or writes the database, a file format or WhatsApp | `resource_access`, behind a contract |
| Something that talks to the operating system (audio, clipboard, camera, sockets) | `infrastructure`, behind a contract |
| An interface between layers | `contracts` |
| A domain type or enum | `core` |
| A helper with no knowledge of tawk's domain | `utilities` |
| Anything drawn or typed in the terminal | `clients/tui` |
| An operation for agents or the shell commands | `clients/control`, and documented in [CONTROL.md](CONTROL.md) |

The "Adding things" section of [ARCHITECTURE.md](ARCHITECTURE.md) walks through the common cases step by step.

## Build and test

```sh
make            # builds ./tawk; there must be no warnings
make test       # the tests that need no WhatsApp account
make check      # smoke test: runs ./tawk --version
```

`make help` lists the other targets and options, such as `WHATSMEOW=0` to build without Go.

A change that alters behaviour comes with a test in `tests/`, added to `TESTS` in the `Makefile`. Tests must run without a WhatsApp account.

## Pull requests

- One change per pull request, with a title that says what it does.
- `make` with no warnings and `make test` passing, on your own machine, before you open it.
- Update the documentation the change touches: [MANUAL.md](MANUAL.md) for anything a user sees, [CONFIGURATION.md](CONFIGURATION.md) for a setting, [CONTROL.md](CONTROL.md) for the control socket, [ARCHITECTURE.md](ARCHITECTURE.md) for a new component or contract.
- A change to a screen in the manual needs its picture redrawn (`make screenshots`).
- Say what you tested and on which system (Linux, macOS, WSL).

## Writing

- Plain British English in code comments, documentation and commit messages.
- No em dashes, and no dashes used as separators or asides. Use commas, colons, full stops or brackets.
- Comments only where the reason is not obvious from the code.
- Commit messages say what the change does, in the project's own voice, with no co-author or "generated with" lines.

## Release notes

Every version gets a section at the top of [RELEASE_NOTES.md](RELEASE_NOTES.md), headed `## <version> (<date>)`, with one plain line for each change a user would notice. `tawk --update` prints every section above the installed version, so a change that is not written there is a change nobody is told about. The commit that raises the version also adds its section.
