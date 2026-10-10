#!/usr/bin/env python3
"""Run UI, storage, keyboard, mesh and OTA tests without hardware."""
from pathlib import Path
import argparse
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--sdk", help="optional macOS SDK compatible with the installed compiler")
args = parser.parse_args()
sdk_flags = ["-isysroot", args.sdk] if args.sdk else []
with tempfile.TemporaryDirectory(prefix="adv-native-tests-") as temp:
    nanopb=next((root/".pio/libdeps"/env/"Nanopb" for env in ("cardputer-adv-m6","cardputer-adv-m5","cardputer-adv-m4","cardputer-adv-m3","cardputer-adv-m2") if (root/".pio/libdeps"/env/"Nanopb/pb.h").exists()),None)
    if nanopb is None: raise RuntimeError("Compile a mesh environment first to resolve pinned Nanopb.")
    generated=root/"vendor/meshtastic-firmware/src/mesh/generated"
    proto_flags=["-I"+str(nanopb),"-I"+str(generated),"-DCARDPUTER_ADV_M6"]
    objects=[]
    for source in (nanopb/"pb_common.c",nanopb/"pb_encode.c",nanopb/"pb_decode.c"):
        obj=str(Path(temp)/(source.stem+".o"));subprocess.run(["cc",*sdk_flags,*proto_flags,"-fsanitize=address,undefined","-c",str(source),"-o",obj],check=True);objects.append(obj)
    proto=[str(p) for p in (generated/"meshtastic").glob("*.pb.cpp") if p.stem not in ("deviceonly.pb",)]
    binary=str(Path(temp)/"channels-test")
    subprocess.run(["c++",*sdk_flags,*proto_flags,"-std=c++17","-fsanitize=address,undefined","-Iinclude","tests/channels_test.cpp","src/protocol/channel_url.cpp","src/protocol/channel_plan.cpp",*proto,*objects,"-o",binary],cwd=root,check=True)
    subprocess.run([binary],cwd=root,check=True)
    binary=str(Path(temp)/"m6-ui-test")
    subprocess.run(["c++",*sdk_flags,*proto_flags,"-Itests/ui_fakes","-std=c++17","-fsanitize=address,undefined","-Iinclude","tests/m6_ui_test.cpp","src/ui/m6_channels.cpp","src/ui/ui_shell.cpp","src/mesh/mesh_validation.cpp","src/protocol/channel_url.cpp","src/protocol/channel_plan.cpp",*proto,*objects,"-o",binary],cwd=root,check=True)
    subprocess.run([binary],cwd=root,check=True)
    binary=str(Path(temp)/"m6-shell-test")
    subprocess.run(["c++",*sdk_flags,*proto_flags,"-DCARDPUTER_ADV_M4","-DCARDPUTER_ADV_M5","-Itests/ui_fakes","-std=c++17","-fsanitize=address,undefined","-Iinclude","tests/m5_integration_test.cpp","src/ui/m4_shell.cpp","src/ui/ui_shell.cpp","src/mesh/mesh_validation.cpp","-o",binary],cwd=root,check=True)
    subprocess.run([binary],cwd=root,check=True)
    binary=str(Path(temp)/"m7-shell-test")
    subprocess.run(["c++",*sdk_flags,*proto_flags,"-DCARDPUTER_ADV_M7","-DCARDPUTER_ADV_M4","-DCARDPUTER_ADV_M5","-Itests/ui_fakes","-std=c++17","-fsanitize=address,undefined","-Iinclude","tests/m5_integration_test.cpp","src/ui/m4_shell.cpp","src/ui/ui_shell.cpp","src/mesh/mesh_validation.cpp","-o",binary],cwd=root,check=True)
    subprocess.run([binary],cwd=root,check=True)
    subprocess.run([binary,"--dm-boot"],cwd=root,check=True)
    binary=str(Path(temp)/"m7-nodes-ui-test")
    subprocess.run(["c++",*sdk_flags,*proto_flags,"-DCARDPUTER_ADV_M7_NODES","-DCARDPUTER_ADV_M7","-DCARDPUTER_ADV_M4","-DCARDPUTER_ADV_M5","-Itests/ui_fakes","-std=c++17","-fsanitize=address,undefined","-Iinclude","tests/m5_integration_test.cpp","src/ui/m4_shell.cpp","src/ui/ui_shell.cpp","src/mesh/mesh_validation.cpp","src/mesh/node_tools.cpp","-o",binary],cwd=root,check=True)
    subprocess.run([binary,"--nodes"],cwd=root,check=True)
    subprocess.run([binary,"--dm-boot"],cwd=root,check=True)
    binary=str(Path(temp)/"m8-ui-test")
    subprocess.run(["c++",*sdk_flags,*proto_flags,"-DCARDPUTER_ADV_M8","-DCARDPUTER_ADV_UI_V2","-DCARDPUTER_ADV_UI_NATIVE","-DCARDPUTER_ADV_M7_NODES","-DCARDPUTER_ADV_M7","-DCARDPUTER_ADV_M4","-DCARDPUTER_ADV_M5","-Itests/ui_fakes","-std=c++17","-fsanitize=address,undefined","-Iinclude","tests/m5_integration_test.cpp","src/ui/m4_shell.cpp","src/ui/ui_shell.cpp","src/mesh/mesh_validation.cpp","src/mesh/node_tools.cpp","-o",binary],cwd=root,check=True)
    subprocess.run([binary,"--m8"],cwd=root,check=True)
    subprocess.run([binary,"--m8-chat"],cwd=root,check=True)
    subprocess.run([binary,"--m8-recovery"],cwd=root,check=True)
    subprocess.run([binary,"--continuous-chat"],cwd=root,check=True)
    # Compile production checkpoint/backup functions against a fault-injecting SD.
    # Only the Arduino FS virtual adapter is substituted; tested storage code is unchanged.
    core=(root/"src/storage/core_storage.cpp").read_text()
    start=core.index("class LockedFile :")
    end=core.index("CoreFilesystem core_filesystem;",start)
    core=core[:start]+"}\n"+core[end:]
    core=core.replace("CoreFilesystem::CoreFilesystem():FS(std::make_shared<CoreFSImpl>()) {}", "CoreFilesystem::CoreFilesystem() {}")
    Path(temp,"core_storage_native.cpp").write_text(core)
    binary=str(Path(temp)/"core-backup-test")
    subprocess.run(["c++",*sdk_flags,*proto_flags,"-DCARDPUTER_ADV_M2","-DCARDPUTER_ADV_M3","-DCARDPUTER_ADV_M8","-Itests/core_fakes","-Ivendor/meshtastic-firmware/src","-I"+temp,"-std=c++17","-fsanitize=address,undefined","-Iinclude","tests/core_storage_test.cpp","src/storage/store_codec.cpp","src/storage/snapshot_codec.cpp","src/mesh/mesh_validation.cpp",str(generated/"meshtastic/deviceonly.pb.cpp"),*proto,*objects,"-o",binary],cwd=root,check=True)
    subprocess.run([binary],cwd=root,check=True)
    for name, sources in (
        ("maintenance", ["tests/maintenance_test.cpp", "src/storage/maintenance.cpp"]),
        ("m8", ["tests/m8_test.cpp", "src/storage/store_codec.cpp", "src/storage/snapshot_codec.cpp", "src/mesh/mesh_validation.cpp"]),
        ("node_tools", ["tests/node_tools_test.cpp", "src/mesh/node_tools.cpp"]),
        ("m7_nodes_storage", ["tests/m5_storage_test.cpp", "src/storage/app_storage.cpp", "src/storage/store_codec.cpp", "src/mesh/mesh_validation.cpp"]),
        ("m7_storage", ["tests/m5_storage_test.cpp", "src/storage/app_storage.cpp", "src/storage/store_codec.cpp", "src/mesh/mesh_validation.cpp"]),
        ("m6_storage", ["tests/m5_storage_test.cpp", "src/storage/app_storage.cpp", "src/storage/store_codec.cpp", "src/mesh/mesh_validation.cpp"]),
        ("m5_ui", ["tests/m5_integration_test.cpp", "src/ui/m4_shell.cpp", "src/ui/ui_shell.cpp", "src/mesh/mesh_validation.cpp"]),
        ("m5_storage", ["tests/m5_storage_test.cpp", "src/storage/app_storage.cpp", "src/storage/store_codec.cpp", "src/mesh/mesh_validation.cpp"]),
        ("m4", ["tests/m4_integration_test.cpp", "src/ui/m4_shell.cpp", "src/ui/ui_shell.cpp", "src/mesh/mesh_validation.cpp"]),
        ("shell", ["tests/ui_shell_test.cpp", "src/ui/ui_shell.cpp", "src/mesh/mesh_validation.cpp"]),
        ("storage", ["tests/storage_codec_test.cpp", "src/storage/store_codec.cpp", "src/storage/snapshot_codec.cpp", "src/mesh/mesh_validation.cpp"]),
        ("file_replace", ["tests/file_replace_test.cpp"]),
        ("keyboard", ["tests/keyboard_decoder_test.cpp", "src/board/keyboard_decoder.cpp"]),
        ("mesh", ["tests/mesh_backend_test.cpp", "src/mesh/mesh_validation.cpp"]),
    ):
        binary = str(Path(temp) / (name + "-test"))
        case_flags = ["-Itests/ui_fakes", "-DCARDPUTER_ADV_M4"] if name == "m4" else []
        if name == "maintenance":case_flags=["-DCARDPUTER_ADV_M8",'-DAPP_VERSION="test"',"-Itests/maintenance_fakes","-Itests/storage_fakes","-Itests/core_fakes"]
        if name == "m8":case_flags=["-DCARDPUTER_ADV_M8"]
        if name == "m5_ui": case_flags = ["-Itests/ui_fakes", "-DCARDPUTER_ADV_M4", "-DCARDPUTER_ADV_M5"]
        if name == "m5_storage": case_flags = ["-Itests/storage_fakes", "-DCARDPUTER_ADV_M3", "-DCARDPUTER_ADV_M4", "-DCARDPUTER_ADV_M5"]
        if name in ("m6_storage","m7_storage","m7_nodes_storage"): case_flags = ["-Itests/storage_fakes", "-DCARDPUTER_ADV_M3", "-DCARDPUTER_ADV_M4", "-DCARDPUTER_ADV_M5", "-DCARDPUTER_ADV_M6"]
        if name in ("m7_storage","m7_nodes_storage"): case_flags += ["-DCARDPUTER_ADV_M7"]
        if name == "m7_nodes_storage": case_flags += ["-DCARDPUTER_ADV_M7_NODES"]
        subprocess.run(["c++", *sdk_flags, *case_flags, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", "-Iinclude", *sources, "-o", binary],
                       cwd=root, check=True)
        subprocess.run([binary], check=True)
        if name == "m7_storage": subprocess.run([binary,"--dm-reboot"],cwd=root,check=True)
        if name in ("m5_storage","m6_storage","m7_storage","m7_nodes_storage"):
            subprocess.run([binary,"--reboot"], cwd=root, check=True)
            subprocess.run([binary,"--interrupted"], cwd=root, check=True)
            if name=="m6_storage": subprocess.run([binary,"--url-corrupt"],cwd=root,check=True)
subprocess.run([sys.executable, "-m", "unittest", "discover", "-s", "tests", "-p", "test_*.py", "-v"],
               cwd=root, check=True)
print("UI, storage, keyboard, mesh and OTA package tests passed.")
