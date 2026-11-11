# Gömülü OTA Köprüsü

[![CI](https://github.com/erencicekk25/embedded-ota-bridge/actions/workflows/ci.yml/badge.svg)](https://github.com/erencicekk25/embedded-ota-bridge/actions/workflows/ci.yml)
![Language](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

Bu proje, bir firmware dosyasını TCP üzerinden parça parça gönderen ve dosyayı bilgisayardaki simüle flash alanına yazan küçük bir OTA demosudur. Gerçek bir cihaza bağlanmaz; güncellenen flash alanı, diskte oluşturulan bir binary dosyasıyla temsil edilir.

## Gereksinimler

- C++17 destekleyen bir derleyici
- CMake 3.16 veya üzeri

Windows'ta Winsock, Linux'ta Berkeley Sockets kullanılır. CRC32, dosyalar ve soketler için gerekenler dışında harici bir kütüphane yoktur.

## Derleme ve test

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Çalıştırma

Önce sunucuyu başlatın. Son parametre, simüle flash alanının bayt cinsinden kapasitesidir:

```sh
build/ota_server 9000 flash.bin 1048576
```

Windows'ta çalıştırılabilir dosyalar yapılandırmaya göre `build/Release` altında bulunabilir.

Başka bir terminalde firmware dosyasını gönderin:

```sh
build/ota_client 127.0.0.1 9000 firmware.bin
```

Sunucu tek bir güncelleme kabul eder ve ardından kapanır. Yeni bir aktarım için sunucuyu yeniden başlatın.

## Aktarım nasıl işler?

İstemci, dosya boyutunu ve CRC32 değerini içeren sabit uzunlukta bir başlık gönderir; ardından firmware verisini 4096 baytlık parçalarla iletir. Sunucu veriyi önce `flash.bin.staging` dosyasına yazar ve CRC32 değerini aktarım sırasında hesaplar. Boyut sınırı aşılırsa, bağlantı kesilirse veya CRC32 uyuşmazsa güncelleme reddedilir. Doğrulama başarılı olduğunda geçici dosya flash imajının yerini alır ve sunucu istemciye onay gönderir; böylece yarım kalan bir aktarım mevcut imajı bozmaz ve istemci güncellemenin sonucunu öğrenir.

CRC32, aktarım sırasında oluşan bozulmaları saptamak içindir; kimlik doğrulama veya şifreleme sağlamaz. Bu örneği güvenilmeyen bir ağa açmayın. Gerçek cihazlarda güvenli önyükleme, imza doğrulama, kimlik doğrulama ve şifreli iletişim ayrıca uygulanmalıdır.
