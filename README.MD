# Kelime/İfade Sayım Botu

Verilen web sitelerinde (ve isteğe bağlı olarak alt sayfalarında) belirli kelime/ifadelerin kaç kez geçtiğini sayan, Türkçe'ye duyarlı bir masaüstü aracı. Basit bir web crawler mantığıyla site içi bağlantıları takip eder, sonuçları renkli bir tabloda gösterir ve CSV/JSON olarak dışa aktarır.

Tek dosyalık, saf Python (yalnızca `requests` + `beautifulsoup4` bağımlılığı) bir `tkinter` uygulamasıdır.

## Özellikler

- **Site içi tarama (crawler):** Bir başlangıç URL'sinden yola çıkıp aynı alan adındaki alt sayfaları BFS (genişlik öncelikli) mantığıyla gezer. Maksimum sayfa sayısı ve derinlik ayarlanabilir.
- **Türkçe'ye duyarlı sayım:** `İ/I/i/ı` dönüşümü doğru yapılır; büyük/küçük harf farkı gözetilmeden eşleşme sağlanır.
- **Akıllı eşleşme:** Tek kelimelik terimler (örn. `SLA`, `AR`) kelime sınırına duyarlı aranır (yani "araba" içindeki "ar" sayılmaz); çok kelimeli ifadelerdeki boşluklar esnek eşleşir.
- **Görsel sonuçlar:** Yalnızca terimin bulunduğu hücre yeşille işaretlenir. Bulunan hücreye tıklayınca o kelimenin hangi URL'lerde kaç kez geçtiğini gösteren bir pencere açılır (URL'ye çift tıklayınca tarayıcıda açılır).
- **TLS/SSL dayanıklılığı:** Katı veya eski TLS yapılandırmalı sunuculara (bazı kamu siteleri) bağlanabilmek için özel bir TLS adaptörü; SSL hatasında gerekirse sertifika doğrulamasını atlayarak yeniden deneme.
- **Anlaşılır hata mesajları:** SSL, bağlantı, zaman aşımı, HTTP kodu ve "JavaScript ile yüklenen boş sayfa" durumları ayrı ayrı raporlanır.
- **Dışa aktarma:** Sonuçlar CSV veya JSON olarak kaydedilebilir (JSON'da hem özet hem alt sayfa kırılımı bulunur).

## Kurulum

Python 3.8 veya üzeri gereklidir.

```bash
pip install requests beautifulsoup4
```

## Kullanım

```bash
python kelime_sayim_botu_tek_dosya.py
```

1. Üst kutuya taranacak site URL'lerini her satıra bir tane olacak şekilde girin.
2. Aranacak kelime/ifadeleri her satıra bir tane girin (veya "Varsayılan Terimleri Yükle" düğmesini kullanın).
3. İsterseniz site içi tarama seçeneklerini (maks. sayfa, maks. derinlik, aynı alan adı) ayarlayın.
4. "Taramayı Başlat" düğmesine tıklayın.
5. Sonuç tablosunda yeşil hücrelere tıklayarak detayları görebilir, "Sonuçları Dışa Aktar" ile CSV/JSON kaydedebilirsiniz.

## Çalıştırılabilir (.exe) oluşturma

Windows için terminal penceresi görünmeden tek dosyalık bir `.exe`:

```bash
pip install pyinstaller
pyinstaller --onefile --windowed --name KelimeSayimBotu kelime_sayim_botu_tek_dosya.py
```

Oluşan dosya `dist/KelimeSayimBotu.exe` altında olur.

## Nasıl çalışır?

Proje iki katmandan oluşur:

- **Çekirdek katman (arayüzden bağımsız):** Sayfa indirme (`fetch_page`), HTML ayrıştırma ve link çıkarma (`_parse_response`), Türkçe normalize (`tr_lower`), terim sayımı (`count_occurrences`) ve site içi tarama (`crawl_site`). Bu katman `tkinter`'a ihtiyaç duymaz ve ayrıca test edilebilir.
- **Arayüz katmanı (`WordCounterApp`):** Kullanıcı girdisini alır, taramayı ayrı bir iş parçacığında (thread) çalıştırır — böylece arayüz donmaz — ve sonuçları `root.after` ile ana thread'e güvenli şekilde aktararak renkli ızgarada gösterir.

Site içi tarama, sayfaları bir kuyrukta (`deque`) tutup "yakından uzağa" gezen BFS algoritmasıyla yapılır. `visited` seti tekrarları, `max_pages`/`max_depth` ise aşırı yayılmayı engeller.

## Bilinen sınırlamalar

- **JavaScript ile yüklenen siteler (SPA):** İçeriğini tarayıcıda JavaScript çalıştırarak oluşturan siteler (örneğin bazı kamu portalları) `requests` ile görüntülenemez. Böyle sayfalar "JavaScript ile yükleniyor" olarak işaretlenir. Bunlar için Selenium veya Playwright gibi tarayıcı otomasyonu gerekir.
- **SSL doğrulamasını atlama** seçeneği güvenlik açığı oluşturur; yalnızca başka türlü bağlanılamayan güvenilir siteler için kullanılmalıdır.
- Araç, `robots.txt` kurallarını okumaz. Siteleri sorumlu şekilde ve makul sınırlarla taramak kullanıcının sorumluluğundadır.

## Bağımlılıklar

- [requests](https://pypi.org/project/requests/)
- [beautifulsoup4](https://pypi.org/project/beautifulsoup4/)
- tkinter (Python ile birlikte gelir)

## Lisans

Bu proje MIT Lisansı altında dağıtılabilir. Kendi deposunda kullanmadan önce bir `LICENSE` dosyası eklemen önerilir.