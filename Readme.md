## Compilation

``g++ -std=c++14 -pthread -Wno-deprecated -o miner miner.cpp sha256.cpp block.cpp util.cpp -lboost_system -ljsoncpp -lws2_32 -I"C:\msys64\mingw64\include" -L"C:\msys64\mingw64\lib"``

## Libs used

 - Boost
 - JsonCPP
 

SHA256 description: https://eips.ethereum.org/assets/eip-2680/sha256-384-512.pdf