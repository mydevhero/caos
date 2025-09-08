### Build&Compile Debug

```
mkdir -p _build/Debug

cd _build/Debug

cmake -DCMAKE_BUILD_TYPE:STRING=Debug -G Ninja -DCMAKE_CXX_CLANG_TIDY:STRING=clang-tidy -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON -DENABLE_CLANG_TIDY=ON ../../

cmake --build .
```

### Build&Compile Release

```
mkdir -p _build/Release

cd _build/Release

cmake -DCMAKE_BUILD_TYPE:STRING=Release -G Ninja -DCMAKE_CXX_CLANG_TIDY:STRING=clang-tidy -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON -DENABLE_CLANG_TIDY=ON ../../

cmake --build .
```