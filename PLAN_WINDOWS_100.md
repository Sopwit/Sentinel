# PLAN — Windows Deployment 100/100

**Mevcut:** 69/100
**Hedef:** 100/100

---

## KRİTİK (1 madde, ~15 puan)

- [ ] **MSIX Package** — Microsoft Store için MSIX. `AppxManifest.xml`, `makeappx` ile paketleme, CI'da MSIX artifact üretimi.

---

## YÜKSEK ETKİ (~15 puan)

- [ ] **Upgrade/Rollback (installer)** — Major upgrade akışı testi, eski sürümden yeniye yükseltme, başarısız upgrade'de rollback.
- [ ] **Silent Install/Uninstall** — NSIS `/S`, WiX `msiexec /qn` ile enterprise dağıtım testi. `RELEASE_CHECKLIST.md`'ye ekle.
- [ ] **File Associations** — `.sentinel` dosya türü kaydı (`HKCU\Software\Classes\.sentinel`). `QFileOpenEvent` handler.
- [ ] **Context Menu** — Windows Explorer "Open with Sentinel" context menu entry (`HKCU\Software\Classes\*\shell\Sentinel`).
- [ ] **Drag & Drop** — C++ `DropArea` veya QML `DropArea` ile dosya içe aktarma.

---

## ORTA ETKİ (~8 puan)

- [ ] **Auto-Update** — WinSparkle entegrasyonu veya MSIX auto-update ile arka plan güncelleme.
- [ ] **Background Download** — Güncelleme indirme progress UI, kullanıcı rahatsız edilmeden indirme.
- [ ] **Delta Update** — bsdiff/courgette ile binary diff güncelleme.
- [ ] **Signature Verification** — İndirilen güncelleme SHA256 checksum + imza doğrulama.
- [ ] **Release Channel** — Stable/Beta/Canary ayrımı, settings'te kanal seçimi.
- [ ] **Startup Time Optimization** — Lazy loading, deferred initialization, background initialization.
- [ ] **Idle RAM/CPU Profiling** — Bellek ve CPU profili çıkarma, optimizasyon.
- [ ] **Configurable Log Retention** — Log rotation süresi kullanıcı tarafından yapılandırılabilir olmalı.
- [ ] **Disk Usage Monitoring** — SQLite VACUUM, `%APPDATA%` boyut bildirimi.

---

## DÜŞÜK ETKİ (~6 puan)

- [ ] **Privacy Policy** — `PRIVACY.md` belgesi, uygulama içi bağlantı.
- [ ] **Terms of Service** — `TERMS.md` belgesi.
- [ ] **Performance Metrics** — Startup time, frame rate, memory usage loglama.
- [ ] **Windows 10/11 Test Raporu** — Kapsamlı uyumluluk testi ve rapor.
- [ ] **Crash Analytics** — Kullanıcı crash raporu gönderebilmesi (opt-in).
- [ ] **Multi-language (15+ dil)** — DE, FR, ES, JA, KO, ZH, PT, IT, RU, NL çevirileri.
- [ ] **Windows Service** — Arka plan servisi olarak çalışma (ileri faz).
- [ ] **Scheduled Tasks** — Windows Task Scheduler ile bakım (VACUUM, update check).
- [ ] **GDPR Compliance** — Veri işleme envanteri, DPIA dokümantasyonu.
- [ ] **Product Information** — CompanyName "Sentinel" → "Sopwit" düzeltmesi (kısmen yapıldı).
- [ ] **Install Scope (Per-Machine)** — `requestedExecutionLevel`, per-user/per-machine seçeneği.

---

## SKOR HEDEFİ

| Kategori | Şimdi | 100/100 |
|---|---|---|
| Kurulum | 5/10 | 10/10 |
| Dosya Yapısı | 5/10 | 8/10 |
| Registry | 6/10 | 9/10 |
| Windows Integration | 7/10 | 10/10 |
| Visual | 8/10 | 10/10 |
| Security | 7/10 | 9/10 |
| Code Signing | 5/10 | 9/10 |
| Update System | 2/10 | 8/10 |
| Crash Handling | 7/10 | 9/10 |
| Logging | 7/10 | 9/10 |
| Performance | 3/10 | 7/10 |
| Windows Services | 0/10 | 5/10 |
| Networking | 7/10 | 9/10 |
| User Experience | 7/10 | 9/10 |
| Localization | 4/10 | 8/10 |
| Installer Quality | 5/10 | 9/10 |
| CI/CD | 8/10 | 10/10 |
| Release Quality | 7/10 | 9/10 |
| Compliance | 4/10 | 8/10 |
| Monitoring | 1/10 | 5/10 |
| Testing | 6/10 | 9/10 |
| Packaging | 6/10 | 10/10 |
| **TOPLAM** | **69/100** | **100/100** |
