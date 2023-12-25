# gcc -o GameAssist.so -shared -fPIC GameAssist.c   for .so
gcc -o usr/share/GameAssist/GameAssist -static -static-libgcc -static-libstdc++  GameAssist.c
gcc -o usr/share/GameAssist/GameAssist.so -shared -static-libgcc -static-libstdc++ -fPIC GameAssist.c
