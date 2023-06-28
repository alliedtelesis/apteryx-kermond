# docker build --no-cache -t apteryx-kermond .
# docker build -t apteryx-kermond .
# docker run -it --user "$(id -u):$(id -g)" -v $(pwd)/..:/home apteryx-kermond
# docker run -it --user "$(id -u):$(id -g)" -v $(pwd)/..:/home apteryx-kermond /bin/bash
# docker run -it --user "$(id -u):$(id -g)" -v $(pwd)/..:/home -e TEST_WRAPPER="gdb -ex run --args" apteryx-kermond
# docker run -it --user "$(id -u):$(id -g)" -v $(pwd)/..:/home -e TEST_WRAPPER="valgrind --leak-check=full" apteryx-kermond
# docker run -it --user "$(id -u):$(id -g)" -v $(pwd)/..:/home -e TEST_WRAPPER="valgrind --tool=cachegrind" apteryx-kermond
# docker run -it --user "$(id -u):$(id -g)" -v $(pwd)/..:/home -e TEST_WRAPPER="valgrind --leak-check=full" apteryx-kermond ./run.sh
FROM ubuntu:20.04

RUN apt-get update -y

ENV TZ=Pacific/Auckland
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get install -y \
    build-essential
RUN apt-get install -y \
    libglib2.0-dev \
    libxml2-dev \
    libcunit1-dev
RUN apt-get install -y \
    liblua5.2-dev \
    python3-pytest \
    git
RUN apt-get install -y \
    libnl-3-dev \
    libnl-route-3-dev
RUN apt-get install -y \
    libjansson-dev
RUN apt-get install -y \
    flake8
RUN apt-get install -y \
    lcov
RUN apt-get install -y \
    iproute2
RUN apt-get install -y \
    valgrind \
    gdb

ARG USER_ID=1000
ARG GROUP_ID=1000
RUN groupadd -g ${GROUP_ID} manager && useradd -l -u ${USER_ID} -g manager manager

ENV LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/home/apteryx-kermond/.build/usr/lib"
ENV PATH="$PATH:/home/apteryx-kermond/.build/usr/bin"
WORKDIR /home/apteryx-kermond
CMD ["/bin/sh", "-c", "./run.sh"]
