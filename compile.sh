em++ -O3 -Iinclude -I. -Iimmer src/yl/**.cpp -std=c++17 --bind -o wasm/main.js --embed-file .predef.yl
