FROM ubuntu:22.04
ARG UID
ARG GID
ARG TARGETPLATFORM
ARG BUILDPLATFORM
ARG SUPPORT_RUST

ENV LANG=en_US.UTF-8

RUN echo "I am running on $BUILDPLATFORM, building for $TARGETPLATFORM"
# make /bin/sh symlink to bash instead of dash:
RUN echo "dash dash/sh boolean false" | debconf-set-selections
RUN DEBIAN_FRONTEND=noninteractive dpkg-reconfigure dash

RUN mv /etc/apt/sources.list /etc/apt/sources.list.bak

RUN if [ "$TARGETPLATFORM" = "linux/amd64" ]; then \
		echo "deb http://mirrors.aliyun.com/ubuntu/ jammy main restricted universe multiverse" >> /etc/apt/sources.list; \
		echo "deb http://mirrors.aliyun.com/ubuntu/ jammy-updates main restricted universe multiverse" >> /etc/apt/sources.list; \
		echo "deb http://mirrors.aliyun.com/ubuntu/ jammy-backports main restricted universe multiverse" >> /etc/apt/sources.list; \
		echo "deb http://mirrors.aliyun.com/ubuntu/ jammy-security main restricted universe multiverse" >> /etc/apt/sources.list; \
		echo "deb-src http://mirrors.aliyun.com/ubuntu/ jammy main restricted universe multiverse" >> /etc/apt/sources.list; \
		echo "deb-src http://mirrors.aliyun.com/ubuntu/ jammy-updates main restricted universe multiverse" >> /etc/apt/sources.list; \
		echo "deb-src http://mirrors.aliyun.com/ubuntu/ jammy-backports main restricted universe multiverse" >> /etc/apt/sources.list; \
		echo "deb-src http://mirrors.aliyun.com/ubuntu/ jammy-security main restricted universe multiverse" >> /etc/apt/sources.list; \
    else \
		echo "deb http://mirrors.aliyun.com/ubuntu-ports/ jammy main restricted universe multiverse" >> /etc/apt/sources.list; \
		echo "deb http://mirrors.aliyun.com/ubuntu-ports/ jammy-updates main restricted universe multiverse" >> /etc/apt/sources.list; \
		echo "deb http://mirrors.aliyun.com/ubuntu-ports/ jammy-backports main restricted universe multiverse" >> /etc/apt/sources.list; \
		echo "deb http://mirrors.aliyun.com/ubuntu-ports/ jammy-security main restricted universe multiverse" >> /etc/apt/sources.list; \
		echo "deb-src http://mirrors.aliyun.com/ubuntu-ports/ jammy main restricted universe multiverse" >> /etc/apt/sources.list; \
		echo "deb-src http://mirrors.aliyun.com/ubuntu-ports/ jammy-updates main restricted universe multiverse" >> /etc/apt/sources.list; \
		echo "deb-src http://mirrors.aliyun.com/ubuntu-ports/ jammy-backports main restricted universe multiverse" >> /etc/apt/sources.list; \
		echo "deb-src http://mirrors.aliyun.com/ubuntu-ports/ jammy-security main restricted universe multiverse" >> /etc/apt/sources.list; \
    fi

RUN apt-get update

# for aarch64-linux-gnu-gdb for xrlinux
RUN apt-get install -y libncurses5 libpython2.7

RUN apt-get install -y git vim wget python3-pip ccache sudo tree
RUN apt-get install -y clang-format
RUN pip3 install conan jinja2
# for render
RUN apt-get install -y libx11-dev libxinerama-dev libgl1-mesa-dev libxrandr-dev
# for gdb
RUN apt-get install -y gdb-multiarch
# for lv_font_conv
RUN apt-get install -y nodejs
RUN apt-get update
RUN apt-get install -y npm
RUN apt-get install -y curl
RUN npm install -g lv_font_conv
RUN apt update
RUN apt install -y build-essential gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu
RUN apt-get install -y libudev-dev
RUN apt-get install -y protobuf-compiler

# RUN rm -rf /var/lib/apt/lists/*

RUN groupadd -g $GID nreal || true
RUN useradd -m -u $UID -g $GID dev || true
RUN echo "dev ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

RUN echo "export RUSTUP_DIST_SERVER=https://mirrors.ustc.edu.cn/rust-static" >> /home/dev/.bashrc
RUN echo "export RUSTUP_UPDATE_ROOT=https://mirrors.ustc.edu.cn/rust-static/rustup" >> /home/dev/.bashrc

# for gdb pretty print
RUN mkdir -p /home/dev/.gdb
COPY --chown=$UID:$GID printers.py /home/dev/.gdb/printers.py
COPY --chown=$UID:$GID gdbinit /home/dev/.gdbinit
COPY --chown=$UID:$GID entrypoint.sh /home/dev/entrypoint.sh
RUN chmod +x /home/dev/entrypoint.sh

USER $UID:$GID
WORKDIR /home/dev

RUN mkdir -p /home/dev/.cargo
COPY --chown=$UID:$GID cargo_config.toml /home/dev/.cargo/config.toml

RUN if [ "$SUPPORT_RUST" = "true" ]; then \
        curl https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain 1.68.0; \
        /home/dev/.cargo/bin/rustup target add aarch64-unknown-linux-musl; \
    fi

ENTRYPOINT ["/home/dev/entrypoint.sh"]
CMD ["/bin/bash"]
