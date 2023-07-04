# docker build --no-cache -t apteryx-kermond .
# docker build -t apteryx-kermond .
# docker run --privileged -it --user manager -v $(pwd)/..:/home apteryx-kermond
# docker run --privileged -it --user manager -v $(pwd)/..:/home apteryx-kermond /bin/bash
# docker run --privileged -it --user manager -v $(pwd)/..:/home -e TEST_WRAPPER="gdb -ex run --args" apteryx-kermond
# docker run --privileged -it --user manager -v $(pwd)/..:/home -e TEST_WRAPPER="valgrind --leak-check=full" apteryx-kermond
# docker run --privileged -it --user manager -v $(pwd)/..:/home -e TEST_WRAPPER="valgrind --tool=cachegrind" apteryx-kermond
# docker run --privileged -it --user manager -v $(pwd)/..:/home -e TEST_WRAPPER="valgrind --leak-check=full" apteryx-kermond ./run.sh
FROM ubuntu:20.04

RUN apt-get update -y

ENV TZ=Pacific/Auckland
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
RUN apt-get install -y \
    build-essential \
    git \
    valgrind \
    gdb \
    sudo
RUN apt-get install -y \
    libglib2.0-dev \
    libxml2-dev \
    libcunit1-dev \
    liblua5.2-dev \
    libnl-3-dev \
    libnl-route-3-dev \
    libjansson-dev
RUN apt-get install -y \
    pip \
    python3-pytest \
    flake8 \
    lcov \
    iproute2
RUN pip install pyang
RUN pip install netifaces

ARG USER=manager
ARG UID=1000
ARG GID=1000
ARG PWD=/home/${USER}
RUN groupadd -g ${GID} ${USER}
RUN adduser --disabled-password --gecos '' --home /home/${USER} --uid ${UID} --gid ${GID} --shell /bin/bash ${USER}
RUN adduser ${USER} sudo
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers
USER ${USER}

ENV LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/home/apteryx-kermond/.build/usr/lib"
ENV PATH="$PATH:/home/apteryx-kermond/.build/usr/bin"
WORKDIR /home/apteryx-kermond
CMD ["/bin/bash", "-c", "./run.sh"]
