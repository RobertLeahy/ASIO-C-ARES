pushd C:/ASIO-CARES
wget https://dl.bintray.com/boostorg/release/1.64.0/source/boost_1_64_0.zip -OutFile boost_1_64_0.zip
7z x ./boost_1_64_0.zip
pushd boost_1_64_0
./bootstrap.bat
./b2 toolset=msvc-14.1 address-model=64 link=static --with-system
popd
popd
