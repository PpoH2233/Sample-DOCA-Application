# Sample DOCA Application

DOCA examples and the BlueField eSwitch management application. The eSwitch
source, build instructions, configuration and CLI are in
[application/eswitch_management/README.md](application/eswitch_management/README.md).

## Codex skills in this repository

Open Codex from this repository root on the BlueField so it can discover the
project skills in `.codex/skills/`. Each skill has a `SKILL.md` entry point and
supporting task and capability notes.

| Skill | Location | Use for |
| --- | --- | --- |
| `doca-flow` | [`.codex/skills/doca-flow/SKILL.md`](.codex/skills/doca-flow/SKILL.md) | DOCA Flow ports, representors, pipes, steering, CT and counters; use with eSwitch fast path work. |
| `doca-common` | [`.codex/skills/doca-common/SKILL.md`](.codex/skills/doca-common/SKILL.md) | DOCA device discovery, capabilities, context lifecycle, buffers, progress engine and logging. |
| `doca-bf3-deployment` | [`.codex/skills/doca-bf3-deployment/SKILL.md`](.codex/skills/doca-bf3-deployment/SKILL.md) | BlueField-3 RShim/BFB bring-up, management channel and DPU mode diagnosis. |

The Flow and Common directories also contain `TASKS.md` and
`CAPABILITIES.md`. The BF3 directory contains those files plus
[`references/details.md`](.codex/skills/doca-bf3-deployment/references/details.md).

For example, after cloning the repository on the BlueField:

```bash
cd /doca_devel/Sample-DOCA-Application
codex
```

In Codex, use `/skills` to inspect the available skills or ask it to use
`$doca-flow` and `$doca-common` when debugging `eswitch-management`. Use
`$doca-bf3-deployment` for platform bring-up questions. Codex can also select
these skills from the task description. Repository skill discovery follows the
[Codex project skill layout](https://developers.openai.com/blog/eval-skills#2-create-the-skill).

These are the three DOCA skills available in the source environment when this
bundle was added. Some of their broader DOCA cross-references point to skills
that are not bundled here, such as `doca-setup`, `doca-debug` and
`doca-hardware-safety`. For those topics, use the installed DOCA SDK headers,
shipped samples and official NVIDIA documentation as the source of truth.
Do not perform a BFB reflash, DPU mode change or firmware burn from an
unresolved cross-reference; identify the target and obtain explicit approval
first.

The skills are instructions and references, not DOCA libraries. On the
BlueField, check the installed SDK and the local build before applying an API
example:

```bash
pkg-config --modversion doca-common doca-flow
meson compile -C /build/eswitch-management
```
