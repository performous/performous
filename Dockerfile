FROM ubuntu:16.04

# Install dependencies Bleh

RUN mkdir /src/
COPY . /src/
WORKDIR /src/

RUN apt-get update && apt-get -y install software-properties-common && apt-add-repository -y ppa:litenstein/libicu60-xenial && apt-add-repository -y ppa:litenstein/libepoxy143-xenial && apt-add-repository -y ppa:litenstein/glm0-9-8-4-xenial && apt-get update && apt-get -y install cmake gettext help2man clang-3.8 libepoxy-dev libsdl2-dev libcairo2-dev libpango1.0-dev librsvg2-dev libboost-all-dev libavcodec-dev libavformat-dev libswscale-dev libswresample-dev libpng-dev libjpeg-dev libxml++2.6-dev portaudio19-dev libopencv-dev libportmidi-dev libqrencode-dev libicu-dev libglm-dev libssl-dev openssl wget && \
\
wget -O cpprestsdk.tar.gz https://github.com/Microsoft/cpprestsdk/archive/v2.10.2.tar.gz && \
tar -xvzf cpprestsdk.tar.gz && cd cpprestsdk-2.10.2/Release && \
mkdir build && cd build && \ 
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCPPREST_EXCLUDE_WEBSOCKETS=ON -DBUILD_TESTS=OFF -DBUILD_SAMPLES=OFF && make -j3 && \
make install && \
cd ../../../ && \
\
mkdir build.clang38 && \
cd build.clang38 && \
CC=clang-3.8 CXX=clang++-3.8 cmake .. && \
make VERBOSE=1 && \
\
cd .. && \
\
mkdir build.gcc-5 && \
cd build.gcc-5 && \
\
CC=gcc-5 CXX=g++-5 cmake .. && \
make VERBOSE=1

CMD ["performous"]
