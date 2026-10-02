# Runtime resources

`resources/` holds canonical static assets embedded or consumed by the runtime:
application icons, UI icons, fonts, and sounds. QML components belong in `ui/`;
translations belong in `translations/`; platform bundle and installer metadata
belongs in `packaging/`.

Keep third-party license notices with the assets they cover. The bundled Tabler
icons retain their `LICENSE` file; bundled font notices are in
[`fonts/LICENSES/`](fonts/LICENSES/README.md). Do not add generated package output, models,
archives, or installer binaries here.
