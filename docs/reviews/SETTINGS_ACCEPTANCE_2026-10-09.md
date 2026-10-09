# Settings kabul ve eksik iş raporu — 9 Ekim 2026

Bu rapor, önceki Settings UX raporunun beş açık madde hakkındaki durumunu günceller. Personal-Brain komutu bu ortamda bulunamadı; repository yerel mimari, güvenlik, derleme ve test yönergeleri kullanıldı. Kullanıcının örnek görüntüleri yerleşim referansı olarak değerlendirildi.

## Yanıt profili: uygulandı

Workspace & Memory içindeki **Response profile**, kullanıcıya görünür, düzenlenebilir tek bir talimat belgesidir. Developer, Learning, Research ve Planning şablonları yalnız editörü doldurur; kullanıcı kaydetmeden etkinleşmez. En fazla 2000 karakter; boş kaydetmek devre dışı bırakır. Eski Metadata only seçicisi yeniden kullanılmadı.

Talimat daemon ayar sözleşmesinde kalıcıdır. Yeni Chat isteklerine ve etkileşimli Agent oturumunun dondurulmuş bağlamına eklenir. Kullanıcının mesajı, aktif hedefi veya sohbet kaydı değiştirilmez. Agent bağlam bütçesine dahildir; yetersiz bütçede atlanabilir. Talimat mevcut kullanıcı isteğine ve güvenlik kurallarına tabidir. Model seçimi, araç kayıtları, sandbox ve erişim/onay politikaları ayrı kalır. Zamanlanmış görevler bu kişisel profili devralmaz. Modelin üslup talimatına her zaman uyması garanti edilmez.

Testler: Chat sağlayıcısına gerçek istek metni ulaşması, kullanıcı mesajının korunması, profili kapatma, Agent planlayıcısına tek kez iletim ve bağlam boyut sınırı. Daemon ayar yazma/okuma turu da test edildi.

## Backup / Recovery: dosya akışı uygulandı

Sistem bölümünde alan seçimi, JSON dışa aktarma, dosya önizlemesi, birleştirme / seçilen alanları değiştirme, açık geri yükleme onayı, ilerleme, iptal ve sonuç durumu bulunur. Önizleme dosyayı uygulamaz. Daemon tam doğrulama ve mevcut CoreBackupService işlem/geri alma sözleşmesini kullanır.

Aktarım: 48 KiB parçalar, 16 MiB sınır, SHA-256 bütünlük kontrolü, bağlantıya bağlı transfer kimliği, en fazla dört aktarım ve on dakikalık süre sınırı. Dosya QSaveFile ile atomik ve yalnız dosya sahibi okuma/yazma izinleriyle kaydedilir. Aktif Agent/Chat çalışması sırasında geri yükleme reddedilir. Commit başladıktan sonra iptal sunulmaz. Çevrimdışı dosya incelemesi mümkündür; dışa aktarma/geri yükleme bağlantı gerektirir.

Kapsam: temel runtime ayarları (tema, dil, ağ modu), workspace profilleri, extension yapılandırması, desteklenen sohbet ve bellek verileri. Credentials, model dosyaları ve geçici ses kayıtları yoktur. Bu **tam uygulama snapshot'ı değildir**: desktop sunum tercihleri ve yeni yanıt profili bu v1 yedek ayar alanında bulunmaz; ekran temel ayar kapsamını açıklar. Büyük sohbet/bellek depolarında mevcut backend kayıt sınırları geçerlidir.

Recovery bölümü sağlık/koşul kodlarını, yarım kalmış sohbet/Agent sayılarını ve yarım model indirmelerini gösterir; managed geçici indirmeyi açık kullanıcı işlemiyle kaldırabilir. Model, araç veya Agent otomatik yeniden çalıştırılmaz. Bozuk depolar için genel bir otomatik onarım sihirbazı veya tüm kesilmiş çalışmalar için yönlendirmeli devam akışı henüz yoktur.

Testler: gerçek istemci–daemon dosya kaydetme/önizleme/geri yükleme, 300 KB'yi aşan çok parçalı JSON turu, hash uyuşmazlığı, sıra dışı offset, başka bağlantının aktarımına erişim ve iptal edilmiş transfer reddi. Kullanıcının gerçek verisine import yapılmadı.

## Çeviri ölçümü: yapıldı; içerik tamamlanmadı

Qt lupdate ile kataloglar güncel kaynaklara eşitlendi. Önceki altı katalogda 72 İngilizce kaynak anahtarı tamamen eksikti. Şimdi kaynak kapsamı tüm kataloglarda %100; bu çevirinin tamamlandığı anlamına gelmez. Aşağıdaki oran yalnız TS'deki **finished** durumudur. İngilizce kaynak metinlerin fallback olarak İngilizce görünmesi ve kaynakla aynı kalan teknik adlar ayrıca yorumlanmalıdır.

| Dil | Finished / aktif metin | TS tamamlanma | Eksik kaynak anahtarı |
| --- | --- | --- | --- |
| ar_SA | 478/1216 | %39.31 | 0 |
| de_DE | 478/1216 | %39.31 | 0 |
| en_US | 572/1216 | %47.04 | 0 |
| es_ES | 478/1216 | %39.31 | 0 |
| fr_FR | 478/1216 | %39.31 | 0 |
| ja_JP | 478/1216 | %39.31 | 0 |
| tr_TR | 618/1216 | %50.82 | 0 |
| zh_CN | 478/1216 | %39.31 | 0 |

Placeholder kontrolünde uyumsuzluk bulunmadı. Almanca/İspanyolca/Fransızca için toplam dört uzunluk genişlemesi inceleme adayı var. Yeni temel profil/mikrofon düğmeleri ve profil şablonları Türkçeye çevrildi. Dil doğruluğunun ana dili konuşan kişilerce incelemesi yapılmadı; unfinished çeviriler tamamlanmış olarak işaretlenmedi.

Gerçek üretim QML'sinde sekiz dil × üç genişlik × sekiz kategori = 192 yerleşim ölçümü yapıldı. 1320/780/560 piksel genişliklerinde içerik alanı boyutları doğrulandı. Qt Text.truncated ile kalan kısaltmalar raporlandı: 16 farklı metin inceleme adayı. Bunların bir bölümü dar seçicilerin bilinçli elide davranışıdır; sıfır metin taşması sertifikası verilmez. Ayar satırı başlıkları artık satır sarar. Arapça Settings kategori menüsünün sağa geçtiği test edildi ve görüntüsü incelendi. Tüm ürünün eksiksiz RTL kabulü açık kalır.

Kanıtlar: [katalog denetimi](settings-acceptance/translations.json), [dil/yerleşim ölçümleri](settings-acceptance/language-layout.json). Yeniden üretim: `cmake --build build/tests --target update_translations`, `python3 tools/i18n/audit_catalogs.py --output docs/reviews/settings-acceptance/translations.json`.

## Erişilebilirlik: ölçüm ve düzeltme yapıldı

14 tema × normal/High Contrast × iki yüzey × primary/muted metin = 112 renk çifti, sRGB bağıl parlaklık formülüyle ölçüldü. Dracula, Tokyo Night ve Nord Frost ikincil metinleri AA altında kaldığı için düzeltildi. Son ölçümde 112/112 çift normal boy metin için 4.5:1 sınırını geçiyor; minimum 4.876:1.

Bu ölçüm bütün bileşenlerin tüm hover/disabled/accent renklerini, saydam katman birleşimlerini veya WCAG'nin tüm kriterlerini kapsamaz. Klavyeyle DND değişimi, görünür odak, reduced-motion sıfır süresi, opak panel ve Qt erişilebilirlik arayüzündeki DND adı test edildi. Gerçek VoiceOver/NVDA/Orca oturumu yürütülmedi; ekran okuyucu kabulü tamamlanmış değildir.

Kanıt: [kontrast ölçümleri](settings-acceptance/contrast.json). Görüntüler test harness'inin geçici dizinine `sentinel-settings-appearance.png`, `sentinel-settings-backup.png`, `sentinel-settings-profile.png`, `sentinel-settings-rtl.png` olarak üretilir.

## Voice setup: deneme akışı eklendi; kurulum/kabul kısmen açık

Ses bölümünde kurulu motor/model dosyalarını algılama, mevcut Local Whisper / sistem diktesi seçimi ve açık kullanıcı eylemiyle mikrofon denemesi vardır. Durdurunca metin yalnız ayarlardaki deneme sonucuna gider; sohbet taslağına veya modele iletilmez. Deneme ve normal sohbet transkript sinyalleri ayrıdır. Devam eden STT sırasında yeni girişin sonuç hedefini değiştirmesi engellendi.

Gerçek mikrofon kaydı veya STT model indirmesi test adına başlatılmadı. Bu makinede whisper-cli PATH üzerinde var; bakılan yaygın Whisper model dizinleri yok. Otomatik ses motoru/model kurulum ve indirme sihirbazı **tamamlanmadı**. Mevcut auto-detect uygun yerleşimi bulamazsa manuel dosya seçimi hâlâ gerekir. macOS sistem diktesi mevcut native adapter'ı kullanır; Linux/Windows sistem diktesi hâlâ işletim sistemi giriş yöntemine yönlendirir. Gerçek mikrofon + transkripsiyon doğruluğu kullanıcı tarafından başlatılacak fiziksel kabul testi gerektirir.

Resmî kurulum kaynakları: [whisper.cpp](https://github.com/ggml-org/whisper.cpp), [Homebrew formülü](https://formulae.brew.sh/formula/whisper.cpp). Kurulum kullanıcı adına bu çalışmada çalıştırılmadı.

## Doğrulama

CMake tests configure/build başarılı. `ctest --preset tests --output-on-failure -j6 --timeout 60`: **120/120 geçti** (31.58 saniye). QML lint hata vermedi; mevcut context/unqualified erişim uyarıları sürüyor. IPC ve desktop sözleşme üreticilerinin `--check` kontrolleri geçti; `git diff --check` temiz. QML ekranları gerçek üretim bileşenleri ve geçici disk depolarıyla render edilip görsel olarak incelendi. Donanım/STT, insan çeviri incelemesi ve gerçek ekran okuyucu kabulü otomatik test başarısıyla eş tutulmaz.
