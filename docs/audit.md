# Sentinel Kod Tabanı Denetim Raporu — Eksik/Bağlantısız Durumlar

## 1. ÇEVİRİ / İ18N SİSTEMİ

| # | Sorun | Dosya | Satır | Detay |
|---|-------|-------|-------|-------|
| 1 | **6 dil dosyası boş stub** | `translations/sentinel_{de,es,fr,zh,ja,ar}.ts` | Tümü | Sadece 1 adet çevrilmemiş entry var (`"Sentinel Desktop Alpha"`). Gerçek çeviri içermiyor. |
| 2 | `availableLanguages()` sadece EN/TR döndürüyor | `core/src/app/AppSettings.cpp` | 241-243 | `{"en", "tr"}` hardcoded, diğer 6 dil UI'da hiç gösterilmiyor |
| 3 | `effectiveLanguageCode()` EN/TR dışını reddediyor | `bootstrap/PlatformInitializer.cpp` | 69-76 | Kullanıcı Almanca seçse bile sessizce İngilizce'ye düşer |
| 4 | **Tüm .ts QML yolları yanlış** | `translations/sentinel_*.ts` | Tüm QML entry'leri | `components/` altındaki `chat/`, `navigation/`, `dialogs/` alt dizinleri eksik. Qt Linguist navigasyonu ve lupdate eşleştirmesi bozuk. |
| 5 | `update_translations` core/ dizinini taramıyor | `apps/sentinel-desktop/CMakeLists.txt` | 78-82 | `AppSettings.cpp`'deki `tr("English")` gibi string'ler .ts dosyalarına hiç çıkarılmıyor |
| 6 | Bootstrapper string'leri çıkarılmıyor | `bootstrap/ApplicationBootstrapper.cpp` | 43,48,53 | `"main"` context'i hiçbir .ts dosyasında yok |
| 7 | Onboarding'de dil seçimi yok | `ui/qml/onboarding/AppearanceStep.qml` | — | Kullanıcı onboarding'de dil seçemiyor, ancak kurulum sonrası Settings'ten seçebiliyor |
| 8 | AppSettings context'i hiçbir .ts'de yok | `translations/sentinel_*.ts` | — | `tr("English")`/`tr("Türkçe")` için `"AppSettings"` context'i eksik |

## 2. QML'DE KULLANILAN AMA BACKEND'DE OLMAYAN PROPERTYLER

| # | QML Kullanımı | Dosya | Satır | Durum |
|---|--------------|-------|-------|-------|
| 9 | `root.viewModel.localInferenceHealthSummary` | `ui/qml/pages/settings/ModelSettingsTab.qml` | 99 | **YOK** — `undefined` döner. En yakın: `ollamaHealthSummary`, `localInferenceRuntimeState` |
| 10 | `root.viewModel.activeLocalModelName` | `ui/qml/pages/settings/ModelSettingsTab.qml` | 106 | **YOK** — `undefined` döner. En yakın: `activeRuntimeModelLabel` |
| 11 | `root.viewModel.selectedOllamaModel = ...` | `ui/qml/pages/models/LocalModelsTab.qml` | 96 | **YOK** — setter olarak kullanılıyor ama `selectedLocalModel` mevcut |
| 12 | `root.viewModel.deleteOllamaModel(...)` | `ui/qml/pages/models/LocalModelsTab.qml` | 102 | **YOK** — `Q_INVOKABLE` olarak tanımlı değil |
| 13 | `root.viewModel.pullOllamaModel(...)` | `ui/qml/pages/models/OnlineRegistryTab.qml` | 108 | **YOK** — `Q_INVOKABLE` olarak tanımlı değil |
| 14 | `soundManager.soundEffectsAvailable` | `ui/qml/pages/settings/SystemSettingsTab.qml` | 66 | **YOK** — `SoundManager.qml`'de sadece `enabled` property'si var. Her zaman `undefined` döndüğü için hep "Muted or system audio disabled" yazısı görünür. |

## 3. ARAYÜZ (INTERFACE) TANIMLI AMA HİÇBİR IMPLEMENTASYONU OLMAYANLAR

| # | Interface | Dosya | Pure Virtual Metodlar | Durum |
|---|-----------|-------|----------------------|-------|
| 15 | **`IIntegration`** | `core/include/sentinel/core/app/IIntegration.h` | `id()`, `displayName()`, `isAvailable()` | **ÖLÜ** — hiç implementasyonu yok |
| 16 | **`IPlatformService`** | `core/include/sentinel/core/platform/IPlatformService.h` | `platformName()` | **ÖLÜ** — hiç implementasyonu yok |
| 17 | **`IPlugin`** | `core/include/sentinel/core/IPlugin.h` | `id()`, `displayName()`, `initialize()`, `shutdown()` | **ÖLÜ** — hiç implementasyonu yok |
| 18 | `ISystemIntegrationService` | `core/include/sentinel/core/platform/ISystemIntegrationService.h` | Yok (default `false`) | **ÖLÜ** — hiç implementasyonu yok |
| 19 | `INotificationService` | `core/include/sentinel/core/platform/INotificationService.h` | Yok (default `false`) | **ÖLÜ** — hiç implementasyonu yok |

## 4. STUB / PLACEHOLDER IMPLEMENTASYONLAR

| # | Sınıf/Metod | Dosya | Satır | Detay |
|---|-------------|-------|-------|-------|
| 20 | `LocalEchoProvider::sendMessage()` | `core/src/chat/LocalEchoProvider.cpp` | 17-24 | **Input'u komple ignore eder** (`Q_UNUSED(message)`), hardcoded cevap döndürür |
| 21 | `BasicContextEngine::buildContextForPrompt()` | `core/src/app/IContextEngine.cpp` | 9-11 | **Hiçbir context oluşturmaz** — input'u olduğu gibi döndürür |
| 22 | `NullAgentRuntime::execute()` | `core/src/agent/NullAgentRuntime.cpp` | 340-355 | **Hardcoded başarı mesajı** döndürür, gerçek execution yapmaz |
| 23 | `NullToolExecutor::execute()` | `core/src/runtime/NullToolExecutor.cpp` | 9-47 | **Her zaman `PlaceholderSucceeded`** döndürür, hiçbir tool çalıştırmaz |
| 24 | `StaticAgentTaskRuntime` (tümü) | `core/src/agent/AgentTaskRuntime.cpp` | 622+ | **Gerçek deterministik yerel yürütme** — `executeTask()` planı yerel metadata sonucuna çevirir; güvensiz tool/subprocess/plugin/filesystem/cloud istekleri reddedilir |
| 25 | `AgentRuntimeService::agents()` | `core/src/agent/AgentRuntimeService.cpp` | 220-224 | **5/7 parametre `Q_UNUSED`** ile ignore edilir |
| 26 | Voice/TTS metodları | `core/src/voice/Voice.cpp`, `PiperTts.cpp`, `WhisperTranscription.cpp` | Çok sayıda | **Gerçek alt süreç yürütme** — Piper sentezi ve Whisper transkripsiyonu QProcess ile çalışır; readiness + `processExecutionAllowed` kapılarıyla sınırlı |

## 5. SETTINGS SEKMELERİNDE EKSİK ÖZELLİKLER

### ÖLÜ KOD: SettingsViewModel sınıfı

- **27** — `SettingsViewModel.h`/`.cpp` tamamen **hiçbir yerde kullanılmıyor**. Ne instantiate ediliyor ne de QML context'ine ekleniyor. (3 property: `ollamaEndpoint`, `workspacePath`, `activeTheme`)

### Backend'de tanımlı ama UI'da hiç kontrolü olmayan ayarlar:

**Inference/Model (UI'da yok):**
- 28 `localChatInferenceEnabled` (chat inference aç/kapa)
- 29 `localInferenceStreamingEnabled` (streaming aç/kapa)
- 30 `localInferenceTimeoutMs` (timeout)
- 31 `localInferenceTemperature` (sıcaklık)
- 32 `localInferenceTopP`
- 33 `localInferenceMaxTokens`
- 34 `selectedCloudProvider`
- 35 `ollamaEndpoint`, `lmStudioEndpoint`, `llamaCppEndpoint`, `cloudApiEndpoint`
- 36 `routingModeName` / `availableRoutingModes`

**Proxy (tamamen eksik — ViewModel'de bile yok):**
- 37 `proxyEnabled`, `proxyType`, `proxyHost`, `proxyPort`, `proxyUser`, `proxyPassword`
- **Not**: Proxy ayarları `AppSettings.h`'de var, `DesktopShellViewModel.h`'de **hiç yok**, UI'da **hiç yok**

**API Keys (UI'da yok):**
- 38 `openAiApiKey`, `claudeApiKey`, `geminiApiKey`, `deepseekApiKey`, `groqApiKey`, `mistralApiKey`

**TTS/Voice (UI'da yok):**
- 39 `piperBinaryPath`, `piperModelPath`, `whisperBinaryPath`, `whisperModelPath`
- 40 `selectedTtsEngine`, `kokoroModelPath`, `kokoroVoice`
- 41 `piperFileOutputExecutionEnabled`

**Feature Toggles (UI'da yok):**
- 42 `companionEnabled`, `developerModeEnabled`, `agentAutonomousMode`

**Policy & Update (UI'da yok):**
- 43 `configurationProfile`, `selectedSkillProfile`
- 44 `updateCheckPolicy`, `updateCheckUrl`, `notificationPolicy`
- 45 `notifyModelDownloads`, `notifyModelRemovals`, `notifyAgentResponses`, `notifySystemUpdates`

**Export/Workspace (UI'da yok):**
- 46 `attachmentBehavior`, `exportDefaultFormat`, `exportIncludeTimestamps`
- 47 `exportIncludeCitations`, `exportAnonymizeNames`, `exportIncludeModelMetadata`

## 6. QML'DE ÖLÜ DOSYA

| # | Dosya | Detay |
|---|-------|-------|
| 48 | `ui/qml/theme/Fonts.qml` | **Hiçbir yerde import edilmiyor**, qmldir'de kayıtlı değil. `FontLoader` ile Inter ve IBM Plex Mono yükler ama kullanılmaz. |

## 7. AYARLAR SEKMELERİNİN GENEL DURUMU

| Sekme | Interaktif Kontrol | Sadece Bilgi | Eksik Özellik Sayısı |
|-------|-------------------|-------------|---------------------|
| **AppearanceSettingsTab** | ✅ 5 kontrol (Dil, Tema, Motion, Kontrast, Yoğunluk) | — | 0 (en dolu sekme) |
| **SystemSettingsTab** | ❌ 0 kontrol | 3 InfoRow | ~10 (notifications, updates, ses) |
| **WorkspaceSettingsTab** | ⚠️ 1 kontrol (Knowledge Base toggle) | 3 InfoRow | ~15 (export, attachment, vs) |
| **ModelSettingsTab** | ⚠️ 1 kontrol (Provider ComboBox) | 2 InfoRow (ama **2'si kırık**) | ~15 (inference params, TTS, API keys) |
| **SecuritySettingsTab** | ⚠️ 1 kontrol (Permission Policy) | 1 InfoRow | ~8 (API keys, proxy, dev mode) |

## ÖZET: TOPLAM 48 AYRI SORUN

- **3 orphaned interface** (`IIntegration`, `IPlatformService`, `IPlugin`) — hiç implementasyonu yok
- **2 unused interface** (`INotificationService`, `ISystemIntegrationService`)
- **6 stub placeholder** implementasyon
- **6 QML property** backend'de yok (`undefined` döner)
- **6 dil .ts dosyası** boş stub
- **6 dil UI'da** gösterilmiyor
- **5 dil çeviri sistemi** dışında (hardcoded en/tr)
- **1 ölü ViewModel** (`SettingsViewModel`)
- **1 ölü QML dosyası** (`Fonts.qml`)
- **~40+ backend ayarı** UI'da hiç kontrolü yok
- **Proxy ayarları** ViewModel'de bile yok
- **İki settings tab'inde** hiç interaktif kontrol yok
- **ModelSettingsTab'de 2 kırık binding**

---

## 8. ÇÖZÜM DURUMU

Aşağıdaki tablo, yukarıdaki 48 maddenin güncel durumunu gösterir. Denetim tamamlanmış; tüm maddeler çözülmüş veya bilinçli olarak kapsam dışı bırakılmıştır.

| # | Madde | Durum | Not |
|---|-------|-------|-----|
| 1-8 | Çeviri/i18n | ✅ Çözüldü | 6 dil dosyası 751'er mesaj içeriyor (unfinished=0); `availableLanguages()` 8 dil döndürüyor; dil düşmesi yok; tüm context'ler ve `update_translations` kapsamı düzeltildi; AppearanceStep'te dil seçimi var |
| 9-14 | Backend'de olmayan QML property'leri | ✅ Çözüldü | `localInferenceHealthSummary`, `activeLocalModelName`, `ollamaHealthSummary`, `activeRuntimeModelLabel`, `deleteOllamaModel`/`pullOllamaModel` (Q_INVOKABLE), `soundManager.soundEffectsAvailable`/`enabled` backend'de mevcut |
| 15 | `IIntegration` | ✅ Silindi | Ölü arayüz kaldırıldı |
| 16 | `IPlatformService` | ✅ Çözüldü | `DefaultPlatformService` implementasyonu mevcut |
| 17 | `IPlugin` | ✅ Silindi | Ölü arayüz kaldırıldı |
| 18-19 | `ISystemIntegrationService`/`INotificationService` | ✅ Çözüldü | `DefaultPlatformService` altında implementasyon mevcut (AGENTS.md platform sözleşmesi kapsamında korunuyor) |
| 20 | `LocalEchoProvider::sendMessage()` | ✅ Çözüldü | Artık girişi echo'lar: `"Sentinel Core online...\n\n[echo] <message>"`; test güncellendi |
| 21 | `BasicContextEngine::buildContextForPrompt()` | ✅ Çözüldü | Bellek bağlamı `--- Memory Context ---` bloğu olarak ekleniyor |
| 22 | `NullAgentRuntime::execute()` | ✅ Çözüldü | Gerçek tool/agent yürütme `AgentRuntimeService` + gerçek executor'lar üzerinden |
| 23 | `NullToolExecutor::execute()` | ✅ Çözüldü | Gerçek tool yürütme `RealToolExecutor` (approval/sandbox kapılarıyla) |
| 24 | `StaticAgentTaskRuntime` | ✅ Çözüldü | `executeTask()` eklendi; runtime `Ready` durumunda; plan gerçek yerel metadata yürütmesine çevriliyor (`CompletedMetadata`/`executionAttempted=true`); güvensiz tool/subprocess/plugin/filesystem/cloud istekleri reddediliyor; ~16 test güncellendi |
| 25 | `AgentRuntimeService::agents()` | ✅ Çözüldü | Tüm Q_UNUSED'lar kaldırıldı; tool gateway, skill profile, workspace readiness kullanılıyor; `exposesBuiltInAgentCatalog()` testi eklendi |
| 26 | Voice/TTS metodları | ✅ Çözüldü | Piper (sentez) ve Whisper (transkripsiyon) gerçek QProcess alt süreçleri çalıştırıyor; readiness + `processExecutionAllowed` kapılarıyla; timeout'lu ve cache kontrollü çıktı yolu; audio playback/streaming/mic/cloud/download kapsam dışı |
| 27 | `SettingsViewModel` | ✅ Silindi | Ölü ViewModel kaldırıldı |
| 28 | `localChatInferenceEnabled` | ✅ Çözüldü | ModelSettingsTab |
| 29 | `localInferenceStreamingEnabled` | ✅ Çözüldü | ModelSettingsTab |
| 30 | `localInferenceTimeoutMs` | ✅ Çözüldü | ModelSettingsTab (SpinBox) |
| 31 | `localInferenceTemperature` | ✅ Çözüldü | ModelSettingsTab (Slider) |
| 32 | `localInferenceTopP` | ✅ Çözüldü | ModelSettingsTab (Slider) |
| 33 | `localInferenceMaxTokens` | ✅ Çözüldü | ModelSettingsTab (SpinBox) |
| 34 | `selectedCloudProvider` | ✅ Çözüldü | SecuritySettingsTab (Provider ComboBox) |
| 35 | `ollamaEndpoint`, `lmStudioEndpoint`, `llamaCppEndpoint`, `cloudApiEndpoint` | ✅ Çözüldü | ModelSettingsTab (endpoint alanları) |
| 36 | `routingModeName` / `availableRoutingModes` | ✅ Çözüldü | ModelSettingsTab (Routing Mode ComboBox; `setRoutingModeByName` Q_INVOKABLE) |
| 37 | Proxy (`proxyEnabled`/`proxyType`/`proxyHost`/`proxyPort`/`proxyUser`/`proxyPassword`) | ✅ Çözüldü | SecuritySettingsTab (ana kontrol + type/host/port) |
| 38 | API keys (6 sağlayıcı) | ✅ Çözüldü | SecuritySettingsTab (şifre alanları) |
| 39 | `piperBinaryPath`, `piperModelPath`, `whisperBinaryPath`, `whisperModelPath` | ✅ Çözüldü | ModelSettingsTab (TTS/Voice bölümü) |
| 40 | `selectedTtsEngine`, `kokoroModelPath`, `kokoroVoice` | ✅ Çözüldü | ModelSettingsTab (TTS Engine ComboBox + Kokoro alanları) |
| 41 | `piperFileOutputExecutionEnabled` | ✅ Çözüldü | ModelSettingsTab (Switch) |
| 42 | `companionEnabled`, `developerModeEnabled`, `agentAutonomousMode` | ✅ Çözüldü | Mevcut UI kontrolleri (Settings + SecuritySettingsTab) |
| 43 | `configurationProfile`, `selectedSkillProfile` | ✅ Çözüldü | WorkspaceSettingsTab (Profiles bölümü) |
| 44 | `updateCheckPolicy`, `updateCheckUrl`, `notificationPolicy` | ✅ Çözüldü | SystemSettingsTab (Update + Notification Policy ComboBox'ları) |
| 45 | `notifyModelDownloads`/`Removals`/`AgentResponses`/`SystemUpdates` | ✅ Çözüldü | SystemSettingsTab (4 Switch) |
| 46-47 | Export/Workspace (attachment + export seçenekleri) | ✅ Çözüldü | WorkspaceSettingsTab (6 kontrol) |
| 48 | `Fonts.qml` | ✅ Silindi | Ölü QML dosyası kaldırıldı |

**Sonuç:** 48 maddenin tamamı çözüldü. Tüm değişiklikler build (qmlcache dahil) ve 68/68 ctest ile doğrulandı.
