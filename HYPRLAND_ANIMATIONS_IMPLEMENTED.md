# Hyprland-like animation bundle

Este bundle implementa uma camada nova de presets `hypr-*` no parser de presets do Picom:

- `hypr-open` / `hypr-popin`: abertura com pop-in, fade, blur e sombra sincronizados.
- `hypr-close` / `hypr-popout`: fechamento com pop-out rápido e curva ease-in.
- `hypr-fade-in` / `hypr-fade-out`: transições curtas de opacidade.
- `hypr-workspace-in` / `hypr-workspace-out`: troca de workspace com slide curto, escala sutil e fade.
- `hypr-geometry` / `hypr-move`: move/resize com curva spring visual e blend do frame salvo.

A configuração `picom.full-animated.conf` já usa esses presets globalmente e também nas regras mais importantes por tipo de janela.

## Instalação rápida

```bash
meson setup build -Dwith_docs=true -Ddbus=true -Dopengl=true -Dregex=true -Dunittest=false
meson compile -C build
sudo meson install -C build
mkdir -p ~/.config/picom
cp picom.full-animated.conf ~/.config/picom/picom.conf
picom --config ~/.config/picom/picom.conf --log-level=info
```

## Ajustes principais

- Para mais efeito de pop: reduza `scale` em `hypr-open`, por exemplo `0.80`.
- Para animações mais discretas: aumente `scale` para `0.90`–`0.94`.
- Para workspace mais perceptível: aumente `distance` de `0.16` para `0.22`–`0.30`.
- Para workspace mais parecido com fade/slide curto do Hyprland: mantenha `distance` entre `0.12` e `0.18`.

