Cách build:

1. Anh/em tạo file out trong src
2. Anh/em tạo file Default trong file out vừa tạo
3. Anh/em tạo file args.gn trong file Default vừa tạo sau đó paste nội dung như sau
      is_component_build = true
      is_debug = false
      symbol_level = 1
      enable_nacl = false
      use_jumbo_build = true
      is_official_build = false
      use_thin_lto = false
      enable_iterator_debugging = false

Cây thư mục như sau  src/out/Default/args.gn

4. Anh/em chạy lệnh gen 
      gn gen out\Default
5. Anh/em chạy lệnh build
      autoninja -C out\Default chrome -j 26
6. Anh/em run chrome bằng lệnh
      out\Default\chrome.exe
