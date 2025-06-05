FROM ghcr.io/werto87/arch_linux_docker_image/archlinux_base_devel_conan:2025_06_05_13_41_14 as BUILD

COPY cmake /home/build_user/modern_durak_matchmaking_proxy/cmake
COPY CMakeLists.txt /home/build_user/modern_durak_matchmaking_proxy
COPY test /home/build_user/modern_durak_matchmaking_proxy/test
COPY conanfile.py /home/build_user/modern_durak_matchmaking_proxy
COPY main.cxx /home/build_user/modern_durak_matchmaking_proxy
COPY ProjectOptions.cmake /home/build_user/modern_durak_matchmaking_proxy

WORKDIR /home/build_user/modern_durak_matchmaking_proxy  

RUN sudo chown -R build_user /home/build_user &&\
    conan remote add modern_durak http://modern-durak.com:8081/artifactory/api/conan/conan &&\
    conan profile detect &&\
    conan install . --output-folder=build --settings compiler.cppstd=gnu23 --build=missing

WORKDIR /home/build_user/modern_durak_matchmaking_proxy/build

#workaround false positive with gcc release  myproject_WARNINGS_AS_ERRORS=Off
RUN cmake ..\
    -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake\
    -DBUILD_TESTS=True \
    -D CMAKE_BUILD_TYPE=Release\
    -D myproject_WARNINGS_AS_ERRORS=Off  &&\
    cmake --build .

WORKDIR /etc/letsencrypt/live/test-name/

RUN sudo mkcert -install &&\
    mkcert localhost && mv\
    localhost.pem fullchain.pem &&\
    mv localhost-key.pem privkey.pem &&\
    openssl dhparam -out dhparam.pem 2048

WORKDIR /home/build_user/modern_durak_matchmaking_proxy/build

RUN test/_test  -d yes --order lex [integration]

FROM ghcr.io/werto87/arch_linux_docker_image/archlinux_base:2024_06_13_07_30_51 

COPY --from=BUILD /home/build_user/modern_durak_matchmaking_proxy/build/run_server /home/build_user/modern_durak_matchmaking_proxy/modern_durak_matchmaking_proxy

CMD [ "/home/build_user/modern_durak_matchmaking_proxy/modern_durak_matchmaking_proxy" ]