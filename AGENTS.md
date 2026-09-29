# Repository guidance for Codex

Start with [README.md](README.md) for the project map and bundled DOCA skills.
For eSwitch work, read
[application/eswitch_management/README.md](application/eswitch_management/README.md)
and the relevant CLI or testing note before changing behavior. The project
skills are in `.codex/skills/` and are available when Codex starts from this
repository.

## The universal verification contract

The installed BlueField DOCA SDK headers and shipped samples are the API
authority. Check `pkg-config --modversion doca-common doca-flow` on the target
and verify symbols against its headers before changing DOCA calls. Build with
Meson on the BlueField and report when hardware tests could not be run.

For fast path work, distinguish pipe/entry creation from packets actually
hitting hardware. Inspect `eswitchctl status` for LPM, CT authorization,
admitted capacity, promotions and failures while sending controlled traffic.
Treat `ct_authorization=arm-only` as Arm NAT, even if a CT pipe exists.

The bundled skills contain references to other DOCA skills that are not in
this checkout. Do not assume an absent skill's instructions are available.
For BFB reflashing, DPU mode changes and firmware burns, identify the exact
target and obtain explicit user approval before changing hardware state.
