# ADR 0002: External localization and SDL-driven UI scale

## Status

Accepted.

## Decision

AI3 loads project-owned UTF-8 strings from JSON files in `assets/locales`. CMake copies `assets` beside
the executable for normal build layouts; development builds also have an explicit compiled source-resource
fallback. `en-US` is mandatory and supplies both the initial locale and per-key fallback. Missing keys render
as `[missing: key]` so incomplete resources remain visible during development.

Locale metadata includes a font-profile hint. Localization remains UTF-8 capable independently of glyph
coverage; the current `latin` profile uses Dear ImGui's redistributable embedded default font and is not a
promise of arbitrary-script rendering. A later profile can select another legally distributable font and
rebuild the atlas without changing string lookup.

SDL's window display scale is the UI scale source of truth. The ImGui lifecycle owns rebuilding its font
atlas and deriving style metrics from unscaled defaults whenever the scale changes meaningfully. Editor code
consumes the effective scale only for deliberate drawing metrics. Window titles append stable `###ai3_*`
identifiers, so translated visible titles and DPI changes do not alter docking persistence.

## Consequences

Translations can switch at runtime without restarting when the active font profile covers their glyphs.
Locale persistence is deferred until AI3 has a settings component; adding one solely for this selection would
prematurely establish broader settings policy. Scale changes preserve editor and docking state but rebuild the
font atlas, which is an intentional lifecycle operation rather than per-frame coordinate multiplication.
