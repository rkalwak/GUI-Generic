FROM ubuntu:latest
LABEL maintainer="Krzysztof Lewandowski <krzysiek@supla.org>"

# Install required packages
RUN apt update && \
    DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt install -y tzdata && \
    apt install -y git wget cmake gcc make libpcap-dev curl \
                   python3 python-is-python3 python3-serial python3-pip python3-venv

# Setup workdir
RUN mkdir supla
WORKDIR /supla

# Download and install FreeRTOS
RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

# Download Gui-Generic
RUN git clone https://github.com/krycha88/GUI-Generic.git

# platformio config and install
RUN wget -O get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
RUN python3 get-platformio.py

RUN mkdir -p /usr/local/bin
RUN ln -s ~/.platformio/penv/bin/platformio /usr/local/bin/platformio
RUN ln -s ~/.platformio/penv/bin/pio /usr/local/bin/pio
RUN ln -s ~/.platformio/penv/bin/piodebuggdb /usr/local/bin/piodebuggdb

WORKDIR /supla/GUI-Generic

RUN platformio pkg install

ENV SUPLA_GG_PATH="/GUI-Generic/"