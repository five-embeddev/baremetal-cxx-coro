ALL_SRC=$(shell find src/ include/ -name '*.hpp' -or -name '*.cpp' -type f )
RISCV_CMAKE=${CURDIR}/cmake/riscv.cmake

CMAKE_OPTIONS_native=\
	-DCMAKE_BUILD_TYPE=Debug

CMAKE_OPTIONS_target=\
	-DCMAKE_TOOLCHAIN_FILE=${RISCV_CMAKE}

TARGET_ELF=build_target/src/main.elf

all: native target native_test

.PHONY : native target
native target:
	cmake \
			${CMAKE_OPTIONS_${@}} \
	        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		    -B build_$@ \
	        -S .
	cmake --build build_$@ --verbose

native_test : native
	cd build_native; ctest

.PHONY : docker_target docker_native
docker_target docker_native :
	docker build --tag=cxx_coro_riscv:latest .
	docker run \
       -it \
       -v `pwd`:/work \
       cxx_coro_riscv:latest \
	   ${@:docker_%=%} \
       /work \
       .

.PHONY : clang_tidy_native
clang_tidy_native : native
	@echo "*** Running CPP Tidy (native) ***"
	clang-tidy \
		-p build_native  \
	    -header-filter=.* \
		src/*.cpp

.PHONY : clang_tidy_target
clang_tidy_target: target
	@echo "*** Running CPP Tidy (target) ***"
	perl -pi -e 's/-march=rv32imac_zicsr/-D__riscv_xlen=64/' build_tidy/compile_commands.json
	clang-tidy-15 \
		-p build_target  \
	    -header-filter=.* \
		src/*.cpp

.PHONY : format
format:
	echo "FORMAT: ${ALL_SRC}"
	clang-format -i ${ALL_SRC}

# run static analysis tools to check code
cppcheck:
	cppcheck \
		--std=c++20 \
	    --enable=all  \
		-I include/ \
	    --suppress=missingIncludeSystem \
		--suppress=unusedFunction \
	    --suppress=comparePointers \
		--suppress=mismatchingContainers \
	    ${ALL_SRC}

# Run
RISCV_ISA=rv32imac_zicsr
SPIKE_CMD_FILE=run_spike_simple.cmd
SPIKE_MMAP=0x8000000:0x2000,0x80000000:0x4000,0x20010000:0x6a120
.PHONY: spike_sim
spike_sim : ${SPIKE_CMD_FILE} ${TARGET_ELF}
	docker run \
		-it \
		--rm \
		-v .:/project \
		fiveembeddev/forked_riscv_spike_dev_env:latest  \
		/opt/riscv-isa-sim/bin/spike \
        -l \
        --log=spike_sim.log \
        --isa=${RISCV_ISA} \
	    -m${SPIKE_MMAP} \
       --priv=m \
		--pc=0x20010000 \
		--vcd-log=spike_sim.vcd \
		--max-cycles=10000000  \
        -d \
	    --debug-cmd=${SPIKE_CMD_FILE} \
		build_target/src/main.elf


clean:
	rm -rf build_target build_native


pre-commit :
	pip install pre-commit
	pre-commit install
	pre-commit run --all-files
