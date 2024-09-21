FROM fiveembeddev/riscv_xpack_gcc_dev_env:latest

ENV APT_OPTS="-o Acquire::Check-Valid-Until=false -o Acquire::Check-Date=false"

RUN DEBIAN_FRONTEND="noninteractive" \
    sudo apt-get ${APT_OPTS} update &&  \
    sudo apt-get ${APT_OPTS} -y install tzdata
RUN sudo apt-get ${APT_OPTS} update -qq && sudo apt-get ${APT_OPTS} install -y git

# Copies your code file from your action repository to the filesystem path `/` of the container
COPY docker_entrypoint.sh /docker_entrypoint.sh

# Code file to execute when the docker container starts up (`entrypoint.sh`)
ENTRYPOINT ["/docker_entrypoint.sh"]

