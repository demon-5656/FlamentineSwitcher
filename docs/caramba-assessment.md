# Caramba Assessment

## Summary

No public upstream source repository for Caramba Switcher was identified during the initial bootstrap of this project. Because of that, this repository is implemented as a greenfield Plasma-focused reinterpretation rather than a literal code fork.

## Reusable

- product idea: fix text typed in the wrong layout
- compact tray-oriented interaction model
- user-facing workflow of explicit correction actions

## Partially Reusable

- UX patterns around layout indicators and fast actions
- feature decomposition into conversion, switching and exclusions
- terminology around last-word and selection correction

## Rewrite Required

- platform backends
- layout integration
- window tracking
- build system
- tray implementation
- Plasma integration
- safe Wayland strategy

## Consequence for this repository

Direct source code reuse is intentionally set to zero until a redistributable upstream codebase can be reviewed.

