# Correções finais aplicadas no bundle

Este bundle foi revisado para reduzir problemas de configuração e falhas silenciosas nas animações.

## Correções principais

- Validação estrita de opções de presets:
  - `duration`: número finito entre `0.001` e `60.0` segundos.
  - `scale`: número finito entre `0.05` e `2.0`.
  - `distance`: número finito entre `0.0` e `2.0`.
  - `direction`: string válida (`left`, `right`, `up`, `down`).
- Presets `hypr-*` agora rejeitam tipos errados em vez de usar defaults silenciosamente.
- Presets clássicos e wrappers (`appear`, `disappear`, `slide-*`, `fly-*`, `zoom-*`, `pop-*`, `geometry-*`) agora validam duração/escala/direção antes de alocar e especializar scripts.
- Evitado vazamento de script quando um preset falha durante validação.
- Registro interno de presets agora usa `struct win_script_preset` nomeada, declarada em `src/transition/preset.h`.

## Verificações realizadas

- `git diff --check`: sem problemas de whitespace.
- Verificação estrutural de chaves/parênteses/colchetes nos arquivos C e configs principais.
- Verificação de triggers e presets usados em:
  - `picom.full-animated.conf`
  - `picom.sample.full-animated.conf`
- Busca por marcadores de conflito Git.

## Observação de build

A compilação completa não foi executada neste ambiente porque faltam Meson e headers de desenvolvimento do projeto, como `libconfig.h`, `uthash.h`, headers XCB extras e pixman. O bundle permanece preparado para build normal em uma máquina com as dependências do Picom instaladas.
