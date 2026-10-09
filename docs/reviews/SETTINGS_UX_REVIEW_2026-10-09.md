# Sentinel Settings değerlendirmesi — 9 Ekim 2026

> Güncel takip: [Settings kabul ve eksik iş raporu](SETTINGS_ACCEPTANCE_2026-10-09.md). Aşağıdaki ilk değerlendirmedeki backup/profil ve ölçüm durumu bu takip raporuyla güncellenmiştir.


## Sonuç ve ekran yapısı

Settings, ana pencerenin içerik alanını dolduran bir sayfadır; modal/popup değildir. Sol ana gezinme rayı korunur. Sayfanın kendi menüsünde arama ve sekiz görev odaklı bölüm vardır: Genel, Görünüm, Modeller ve Sağlayıcılar, Ses, Workspace ve Bellek, Gizlilik ve İzinler, Bildirimler, Sistem. Referans görseller yalnızca yerleşim örneğidir; başka ürüne ait hesap, faturalama, ebeveyn denetimi veya cloud computer özellikleri Sentinel'e eklenmedi.

Geniş ekranda kategori menüsü sabittir; 760 pikselin altındaki içerik genişliğinde kategori seçimi üstte bir açılır listeye dönüşür. Sağ içerik 960 piksel ile sınırlandırılır, başlık ve alt başlıklar altında kartlarla gösterilir. Genel bölüm dil, sesli geri bildirim, companion, başlangıç ve kısayol tercihlerini; Sistem ise güncelleme ve tanılamayı içerir.

## Dil sayısı yeterli mi?

Desktop paketinde sekiz dil kataloğu bulunuyor: İngilizce, Türkçe, Almanca, İspanyolca, Fransızca, Çince, Japonca ve Arapça. Dil menüsü mevcut derlemede gerçekten paketlenen `.qm` dosyalarından üretilir; çeviri kaynağı paketlenmeyen test derlemelerinde yalnızca İngilizce/Türkçe görünmesi beklenir.

Sekiz dil ilk sürüm için makul bir kapsam; geniş küresel hedef için son nokta olarak değerlendirmiyorum. Sonraki adaylar Portekizce, Korece, Hintçe, İtalyanca ve Rusça. Bu sıralama bir ürün önerisidir, kullanım telemetrisi veya pazar payı ölçümü değildir. Bu çalışmada dil sayısı yapay olarak artırılmadı; boş katalog eklemek destek sağlamak anlamına gelmez. Çeviri içerik kalitesi bu değerlendirmenin kapsamı dışındadır.

## Temalar ve erişilebilirlik

Kalıcı tema anahtarları korunur; mevcut kullanıcı tercihlerinin ve eski paletlerin davranışı değişmez. Kullanıcıya görünen adlar yenilendi:

| Kalıcı anahtar / eski ad | Yeni görünen ad |
| --- | --- |
| Liquid Glass Light | Daylight |
| Liquid Glass Dark | Obsidian |
| Sentinel Classic | Slate |
| Midnight Blue | Deep Ocean |
| Aurora Teal | Evergreen |
| Graphite Grey | Carbon |
| Solarized Light | Honey |
| Nord Frost | Polar Night |
| Dracula | Velvet |
| Tokyo Night | Indigo |

Dört yeni açık palet: Porcelain, Sandstone, Meadow, Blue Mist. Önceden tema motorunda olup seçicide bulunmayan Honey ve Indigo da seçilebilir. Böylece altı açık ve sekiz koyu seçenek var. Yeni açık temalar mevcut açık tema kontrol/metin temelini kullanır, yüzey renkleri ayrıdır.

Reduced Motion merkezi süreleri etkiliyordu; bazı doğrudan sayısal geçişler ve sonsuz dekoratif döngüler bu tercihi atlıyordu. Bu geçişler ortak süre politikasına bağlandı; atmosfer, shimmer ve model kartı dekoratif döngüleri durdurulur. Yükleme durumu göstergeleri görünür kalır. Bu, tüm platformların kendi yerel animasyonlarını kapatan bir işletim sistemi ayarı değildir.

High Contrast metni zaten etkiliyordu, ancak düşük opaklıklı sınırlar ve saydam paneller okunabilirliği zayıflatıyordu. Artık panel yüzeyleri opak, metinden türetilen çok silik sınırlar daha belirgin ve Settings kart sınırları daha güçlüdür. Bu değişiklik tüm özel renk çiftlerinin WCAG sertifikası değildir; bunun için ayrıca bileşen bazında kontrast ölçümü gerekir.

Yeni Saydamlığı Azalt tercihi kalıcıdır. Panel/cam yüzeyleri opaklaşır, cam bulanıklığı token'ları sıfırlanır. DesktopSettingsStore bunu yerel sunum tercihi olarak saklar; daemon'a çalışma ayarı olarak göndermez. Yoğunluk seçeneklerinde klavye odağı etkinleştirildi; ayar anahtarları erişilebilir ad/açıklama taşır.

## Bunaltan veya yanıltan seçenekler

| Bulgu | Karar / uygulama |
| --- | --- |
| Ayrı uzun Runtime Status satırı | Sağlayıcı seçicisinin yanına durum noktası; hazır yeşil, bekleme/busy sarı, kullanılamaz/hatalı kırmızı. Tooltip ve erişilebilir metin durumu açıklar. |
| Örnek üründeki çok sayıda ilgisiz kategori | Yalnızca Sentinel'de karşılığı olan sekiz kategori. |
| Sampler ayarlarının ilk bakışta kalabalığı | Sıcaklık, top-p, token ve timeout kontrolleri aynı bölümde Gelişmiş Ayarlar altında. |
| Bildirim tercihlerinin Sistem içinde kaybolması | Ayrı Bildirimler bölümü; teslim, kanal ve geçmiş kontrolleri. |
| Configuration Profile'ın yalnızca serbest metin etiketi olması | Ana Settings'ten kaldırıldı; kayıt formatı korunur. Gerçek ayar snapshot'ı seçiyormuş gibi sunulmaz. |
| SkillProfileService seçeneklerinin Metadata only / Placeholder olması | Davranışı değiştirmeyen seçici kaldırıldı. Yetki veya prompt etkisi varmış gibi yeniden adlandırılmadı. Backend tanılama kayıtları korunur. |
| Çok uzun teknik plan ve gateway bilgileri | Mevcut izin denetimleri korunur. Plan oluşturma/çalıştırma aracının Settings yerine Inspector'a taşınması sonraki ürün düzenlemesi için önerilir; bu çalışmada yürütme akışı taşınmadı. |
| Ses motorlarının manuel binary/model yolu gerektirmesi | İşlevsel oldukları için korundu. İşletim sistemine özgü kurulum sihirbazları gelecekte ayrı çalışma gerektirir. |

Profil sistemi için gerçek bir sonraki sürüm; kullanıcıya görünür talimatlar, açık etkinleştirme, sohbet/Agent kapsamı, yeni çalışmalara uygulanma ve izinlerden bağımsızlık gerektirir. Mevcut metadata nesnelerini sessizce sistem prompt'una eklemek uygun bir düzeltme değildir. Bu çalışmada gerçek profil yürütmesi eklenmedi.

## Backend–UI boşlukları

| Backend işlevi | Önceki durum | Uygulanan değişiklik |
| --- | --- | --- |
| Workspace oluşturma/yeniden adlandırma/çoğaltma | View model'de mevcut, bu Settings ekranında yok | Workspace bölümüne eklendi; mevcut servis ve kalıcılık kullanılır. |
| Rahatsız Etmeyin | Toast tarafında mevcut, oturumluk; native bildirimlerden kaçabiliyordu | Settings'te görünür, mevcut notification JSON belgesinde kalıcı; native ve banner filtresi ortak. Okundu/arşiv işlemleri bu tercihleri silmez. |
| Kanal susturma | Backend'de var, Settings'te yok | Tasks, Security, Workspace, Brain kanalları için denetimler. Model/Agent/Updates için mevcut Custom seçenekleri korunur. |
| Bildirim geçmişi işlemleri | Merkezde var, Settings'ten erişim yok | Geçmişi aç, tümünü okundu yap, arşivlenenleri temizle. |
| Important Only politikası | Normal öncelikli bazı bildirimler geçebiliyordu | Yalnız High/Critical; All politikasında Low da geçebilir. |
| Quick Panel onay/çalışma bildirimleri | Kategori ve öncelik taşımıyordu | Onay Security/High, başarısız Agent High, diğer terminal Agent Normal. Ortak filtre kullanılır. |
| RetentionPolicy ve SettingsService snapshot'ları | Backend'de mevcut, Settings'te düzenleyici yok | Gizlilik bölümünde beş kayıt alanı için kaynak sözleşmesindeki allowedValues kullanılarak düzenlenir. Yükleniyor/daemon yok durumları gösterilir. |
| Ses cihazı ve VAD seçimi | Backend ayar sözleşmesinde mevcut, bu Settings ekranında yok | Ses bölümünde sözleşmenin cihaz seçenekleri ve boolean VAD tercihi kullanılır. |
| Güncelleme TLS hata işleme | Sertifika hataları loglandıktan sonra yoksayılıyordu | ignoreSslErrors kaldırıldı; Qt'nin doğrulama hatası isteği başarısız kılar. |

Workspace kaldırılmadı: kök klasör, bağlam, ayar kapsamı ve yetki sınırları için gerçek karşılığı var. Yeni isim veya kopya oluşturmak ek dosya erişimi izni vermez; klasör seçimi açık kullanıcı işlemi olarak kalır.

Backup import/export ve recovery sözleşmeleri de backend'de mevcut. Tam dosya seçme, önizleme, çakışma çözme ve sonuç takibi gerektiren backup/recovery sihirbazları bu çalışmada eklenmedi. Bunları yalnız bir düğmeyle mevcutmuş gibi sunmak yerine açık kalan ürün işi olarak kaydediyorum. Ses motor/yol seçimi korunur; cihaz ve VAD tercihleri ayrıca görünür hâle getirildi.

## Doğrulama

Üretim QML ve gerçek view model kullanan disposable profile testi sekiz kategoriyi 1320/780/560 piksel pencere genişliklerinde açar; klavyeyle DND değişimini, altı açık paleti, reduced-motion sıfır süresini ve opak paneli kontrol eder. Kalıcılık ve ortak bildirim filtreleri C++ testlerine eklendi. Yerel runtime kurulumu, gerçek ses motoru kurulumu veya kullanıcı verisi temizliği test amacıyla başlatılmaz.

Son doğrulama: `cmake --preset tests` ve `cmake --build --preset tests` başarılı; `ctest --preset tests --output-on-failure -j6 --timeout 60` sonucunda 120/120 test geçti. Settings QML lint kontrolü hata vermedi; context property erişim uyarıları mevcut. IPC üretim kontrol dosyaları güncel ve `git diff --check` temiz. Yazılım renderer ile üretilen gerçek QML ekran görüntüsü de görsel olarak incelendi.
