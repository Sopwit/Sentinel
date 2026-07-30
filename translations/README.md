# Sentinel Translation & Localization (i18n)

This directory contains the XML translation source (`.ts`) files for Sentinel Desktop.

## Supported Locales
- `sentinel_en.ts` — English (Master / Default)
- `sentinel_tr.ts` — Turkish
- `sentinel_de.ts` — German
- `sentinel_es.ts` — Spanish
- `sentinel_fr.ts` — French
- `sentinel_zh.ts` — Chinese Simplified
- `sentinel_ja.ts` — Japanese
- `sentinel_ar.ts` — Arabic (RTL)

## Updating Translations (`lupdate`)
To extract new or updated `qsTr()` strings from QML and C++ source code into `.ts` files, run:

```bash
# Via CMake custom target
cmake --build build --target update_translations

# Or manually with Qt Linguist lupdate
lupdate apps/sentinel-desktop ui/qml -ts translations/sentinel_*.ts
```

## Binary `.qm` Files
Note: `.qm` compiled binary files are generated dynamically in the build directory during CMake build time (`qt_add_translations`).
**.qm files MUST NOT be committed to git version control.**
