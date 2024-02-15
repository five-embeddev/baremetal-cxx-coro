FROM ubuntu:22.04 AS riscv_dev_env 

ENV NVM_DIR=/root/.nvm
ENV XPACKS_REPO_FOLDER=/root/.xpack/repos
#ENV XPACKS_CACHE_FOLDER=/root/.xpack
ENV XPACKS_SYSTEM_FOLDER=/root/.xpack/system
#

ENV APT_OPTS="-o Acquire::Check-Valid-Until=false -o Acquire::Check-Date=false"

RUN DEBIAN_FRONTEND="noninteractive" \
    apt-get ${APT_OPTS} update &&  \
    apt-get ${APT_OPTS} -y install tzdata

RUN apt-get ${APT_OPTS} update -qq && apt-get ${APT_OPTS} install -y curl
RUN curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.38.0/install.sh | bash

ENV NODE_VERSION=16.16.0
RUN . "$NVM_DIR/nvm.sh" && nvm install ${NODE_VERSION}
RUN . "$NVM_DIR/nvm.sh" && nvm use v${NODE_VERSION}
RUN . "$NVM_DIR/nvm.sh" && nvm alias default v${NODE_VERSION}
ENV PATH="/root/.nvm/versions/node/v${NODE_VERSION}/bin/:${PATH}"
RUN node --version
RUN npm --version

RUN npm install --global xpm@latest

ENV XPACK_GCC_VERSION=12.2.0-3.1
RUN xpm install --global --verbose  @xpack-dev-tools/riscv-none-elf-gcc@${XPACK_GCC_VERSION}  
ENV PATH=/root/.xpack/repos/@xpack-dev-tools/riscv-none-elf-gcc/${XPACK_GCC_VERSION}/.content/bin:/bin/:/usr/bin

RUN echo $PATH
RUN ls -l /root/.xpack/repos/@xpack-dev-tools/riscv-none-elf-gcc/${XPACK_GCC_VERSION}/.content/bin/
RUN riscv-none-elf-gcc --version

RUN apt-get ${APT_OPTS} update -qq && apt-get ${APT_OPTS} install -y cmake git
RUN apt-get ${APT_OPTS} update -qq && apt-get ${APT_OPTS} install -y g++

# Copies your code file from your action repository to the filesystem path `/` of the container
COPY docker_entrypoint.sh /docker_entrypoint.sh

# Code file to execute when the docker container starts up (`entrypoint.sh`)
ENTRYPOINT ["/docker_entrypoint.sh"]

