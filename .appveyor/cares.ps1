pushd C:/ASIO-CARES
wget https://github.com/c-ares/c-ares/archive/master.zip -OutFile c-ares.zip
7z x ./c-ares.zip
pushd c-ares-master
mkdir build
pushd build
cmake .. -G "Visual Studio 15 2017 Win64" -DCARES_STATIC=On -DCARES_SHARED=Off
cmake --build . --config Release
cp ./ares_*.h ../
popd
popd
popd
