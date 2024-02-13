# gcc -o GameAssist.so -shared -fPIC GameAssist.c   for .so
gcc -o usr/share/GameAssist/GameAssist -std=c++11 -static -static-libgcc -static-libstdc++ GameAssist.cpp -lstdc++
gcc -o usr/share/GameAssist/GameAssist.so -std=c++11 -static-libgcc -static-libstdc++ -shared -fPIC GameAssist.cpp -lstdc++
