# Deferred QML warning inventory

2026-10-08, macOS Qt 6.11.2. Full lint exits 0. 766 warnings before cleanup; 72 remain. No category is disabled.

All remaining diagnostics are `unqualified` intentional bootstrap-context exceptions. These names are registered with `QQmlContext::setContextProperty` by ApplicationBootstrapper (and by the certification helper); standalone qmllint cannot resolve that context. Moving shared objects to typed required properties requires a broader injection-contract migration deferred until the QML redesign. Owned lexical/delegate references, missing properties/imports, invalid layout ownership and duplicate bindings were fixed.

| File | Line:column | Symbol | Classification / reason |
|---|---|---|---|
| `ui/qml/Main.qml` | 22:29 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/Main.qml` | 215:36 | `agentInspectorViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/Main.qml` | 366:21 | `quickPanelController` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/LocalModelsTab.qml` | 104:48 | `ollamaPuller` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 16:29 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 457:26 | `ollamaLibraryFetcher` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 458:24 | `lmStudioLibraryFetcher` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 500:27 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 510:27 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 542:23 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 554:27 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 619:13 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 620:13 | `ollamaLibraryFetcher` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 622:9 | `lmStudioLibraryFetcher` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 630:13 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 631:13 | `ollamaLibraryFetcher` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 655:22 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 656:22 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 691:29 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 692:29 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 724:14 | `ollamaPuller` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 724:39 | `ollamaPuller` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 725:27 | `ollamaPuller` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 1050:42 | `ollamaLibraryFetcher` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 1070:206 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 1170:42 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 1184:50 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 1184:85 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 1215:31 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 1228:26 | `ollamaLibraryFetcher` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 1254:31 | `ollamaLibraryFetcher` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 1296:64 | `ollamaLibraryFetcher` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 1317:22 | `ollamaLibraryFetcher` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 1429:71 | `ollamaPuller` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 1430:71 | `ollamaPuller` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 1568:159 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 1724:36 | `ollamaPuller` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 1725:35 | `ollamaPuller` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 1726:76 | `ollamaPuller` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 1727:23 | `ollamaPuller` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 1730:13 | `ollamaPuller` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/ModelsPage.qml` | 1733:13 | `ollamaPuller` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/models/OnlineRegistryTab.qml` | 110:48 | `ollamaPuller` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/settings/SettingsPage.qml` | 53:17 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/settings/SystemSettingsTab.qml` | 112:27 | `nativeDesktop` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/settings/SystemSettingsTab.qml` | 113:26 | `nativeDesktop` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/settings/SystemSettingsTab.qml` | 116:41 | `nativeDesktop` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/settings/SystemSettingsTab.qml` | 120:60 | `nativeDesktop` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/settings/SystemSettingsTab.qml` | 125:26 | `nativeDesktop` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/settings/SystemSettingsTab.qml` | 127:59 | `nativeDesktop` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/settings/SystemSettingsTab.qml` | 133:32 | `nativeDesktop` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/settings/SystemSettingsTab.qml` | 133:65 | `nativeDesktop` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/settings/SystemSettingsTab.qml` | 133:111 | `nativeDesktop` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/pages/settings/SystemSettingsTab.qml` | 135:30 | `nativeDesktop` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/components/navigation/BottomDock.qml` | 32:17 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/components/chat/HomeChatSurface.qml` | 967:41 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/components/dialogs/ModelDetailPopup.qml` | 39:40 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/components/dialogs/ModelDetailPopup.qml` | 59:32 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/components/dialogs/ModelDetailPopup.qml` | 72:32 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/components/dialogs/ModelDetailPopup.qml` | 132:13 | `ollamaModelDetailFetcher` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/components/dialogs/ModelDetailPopup.qml` | 134:13 | `ollamaModelDetailFetcher` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/components/dialogs/ModelDetailPopup.qml` | 141:13 | `ollamaModelDetailFetcher` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/components/dialogs/ModelDetailPopup.qml` | 146:9 | `ollamaModelDetailFetcher` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/components/dialogs/ModelDetailPopup.qml` | 150:17 | `ollamaModelDetailFetcher` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/components/dialogs/ModelDetailPopup.qml` | 152:24 | `ollamaModelDetailFetcher` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/components/dialogs/ModelDetailPopup.qml` | 415:82 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/components/dialogs/ModelDetailPopup.qml` | 422:29 | `ollamaPuller` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/components/dialogs/TrayCompanionWindow.qml` | 23:35 | `nativeDesktop` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/components/dialogs/RuntimeDetailPopup.qml` | 29:53 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/components/dialogs/RuntimeDetailPopup.qml` | 86:12 | `shellViewModel` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/components/dialogs/RuntimeDetailPopup.qml` | 87:12 | `lmStudioLibraryFetcher` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
| `ui/qml/components/dialogs/RuntimeDetailPopup.qml` | 87:45 | `lmStudioLibraryFetcher` | `unqualified`; intentional C++ bootstrap context, unresolved by standalone lint. |
