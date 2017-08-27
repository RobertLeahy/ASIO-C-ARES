$boost_minor = $args[0]
pushd C:/ASIO-CARES
wget "https://dl.bintray.com/boostorg/release/1.$($boost_minor).0/source/boost_1_$($boost_minor)_0.zip" -OutFile "boost_1_$($boost_minor)_0.zip"
7z x "./boost_1_$($boost_minor)_0.zip"
pushd "boost_1_$($boost_minor)_0"
./bootstrap.bat
./b2 toolset=msvc-14.1 address-model=64 link=static --with-system
popd
popd
