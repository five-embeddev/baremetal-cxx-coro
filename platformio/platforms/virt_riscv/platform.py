# Copyright 2014-present PlatformIO <contact@platformio.org>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import sys
import requests

from platformio.public import PlatformBase

IS_WINDOWS = sys.platform.startswith("win")

# TODO, improve this!
PLATFORMIO_PACKAGES=os.path.expanduser('~/.platformio/packages/')
XPACK_GCC_VERSION="12.2.0-3"
XPACK_TARGET="riscv-none-elf-"
XPACK_GCC="riscv-none-elf-gcc"
XPACK_GCC_HOST="linux-x64"
XPACK_ARCHIVE=f"xpack-{XPACK_GCC}-{XPACK_GCC_VERSION}-{XPACK_GCC_HOST}.tar.gz"
XPACK_ARCHIVE_URL=f"https://github.com/xpack-dev-tools/{XPACK_GCC}-xpack/releases/download/v{XPACK_GCC_VERSION}/{XPACK_ARCHIVE}"
TOOLS_PACKAGE=f"tool-xpack-{XPACK_GCC}-{XPACK_GCC_VERSION}"

class Virt_riscvPlatform(PlatformBase):

    def configure_default_packages(self, variables, targets):
        rc= super().configure_default_packages(variables, targets)
        xpack_package_path=os.path.join(PLATFORMIO_PACKAGES,TOOLS_PACKAGE)
        if not os.path.exists(xpack_package_path):
            print("NO TOOLS!")
            os.mkdir(xpack_package_path)
        xpack_archive_path=os.path.join(xpack_package_path,XPACK_ARCHIVE)
        if not os.path.exists(xpack_archive_path):
            print("NO ARCHIVE!")
            r = requests.get(XPACK_ARCHIVE_URL, stream=True)
            total_length = int(r.headers.get('content-length'))
            with open(xpack_archive_path , 'wb') as fout:
                for chunk in r.iter_content(chunk_size=1024): 
                    print(".", end="")
                    if chunk:
                        fout.write(chunk)
                        fout.flush() 
                print(".done")
        xpack_bin_path=os.path.join(xpack_package_path, f"xpack-{XPACK_GCC}-{XPACK_GCC_VERSION}", "bin")
        xpack_gcc_path=os.path.join(xpack_bin_path, XPACK_GCC)
        if not os.path.exists(xpack_gcc_path):
            print(f"GCC NOT FOUND: {xpack_gcc_path}")
            previous_dir = os.getcwd()
            try:
                os.chdir(xpack_package_path)
                os.system(f"tar xzf {XPACK_ARCHIVE}")
            finally:
                os.chdir(previous_dir)
        else:
            print(f"GCC INSTALLED AT: {xpack_gcc_path}")
        self.xpack_gcc_path = xpack_gcc_path
        self.xpack_bin_path = xpack_bin_path
        return rc
       
    def get_tool_path(self, tool):
        path = os.path.join(self.xpack_bin_path, f"{XPACK_TARGET}{tool}")
        print(f"{path} = {tool}")
        return path

    def get_boards(self, id_=None):
        result = super().get_boards(id_)
        if not result:
            return result
        if id_:
            return self._add_default_debug_tools(result)
        else:
            for key in result:
                result[key] = self._add_default_debug_tools(result[key])
        return result

    def _add_default_debug_tools(self, board):
        debug = board.manifest.get("debug", {})
        upload_protocols = board.manifest.get("upload",
                                              {}).get("protocols", [])
        if "tools" not in debug:
            debug["tools"] = {}

        tools = ("qemu",)
        for tool in tools:
            if tool in ("qemu"):
                if not debug.get("%s_machine" % tool):
                    continue
            elif (tool not in upload_protocols or tool in debug["tools"]):
                continue
            if tool == "qemu":
                machine64bit = "64" in board.get("build.mabi")
                debug_args = debug.get("qemu_options")
                if debug_args is None:
                    debug_args = []
                debug["tools"][tool] = {
                    "server": {
                        "package": "tool-qemu-riscv",
                        "arguments": [
                            "-nographic",
                            "-machine", "sifive_e", #debug.get("qemu_machine"),
                            "-d", "unimp,guest_errors",
                            "-gdb", "tcp::42123",
                            "-S"
                        ] + debug_args,
                        "executable": "bin/qemu-system-riscv%s" % (
                            "64" if machine64bit else "32")
                    }
                }

        board.manifest["debug"] = debug
        return board

