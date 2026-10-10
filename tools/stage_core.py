"""Materialize a reviewed overlay without changing the pinned upstream checkout."""
from pathlib import Path
import subprocess

def stage_core(root):
    upstream = root / 'vendor/meshtastic-firmware'
    stage = root / '.pio/core-m2'
    def write(path, data):
        path.parent.mkdir(parents=True, exist_ok=True)
        if not path.exists() or path.read_bytes() != data:
            path.write_bytes(data)
    tracked = subprocess.check_output(["git", "ls-files", "-z", "--", "src"], cwd=upstream).decode().split("\0")
    expected = set()
    for relative in filter(None, tracked):
        target = stage / relative
        expected.add(target)
        write(target, (upstream / relative).read_bytes())
    for old in (stage / "src").rglob("*"):
        if old.is_file() and old not in expected:
            old.unlink()
    def replace(relative, old, new):
        path = stage / 'src' / relative
        text = path.read_text()
        if text.count(old) != 1:
            raise RuntimeError(f'Overlay anchor mismatch: {relative}: {old[:60]}')
        write(path, text.replace(old, new).encode())
    # Headless read-only adapter; routing, retries, rate limiting and timeout remain upstream.
    replace('modules/TraceRouteModule.h', '    bool startTraceRoute(NodeNum node);',
        '    bool startTraceRoute(NodeNum node);\n#ifdef CARDPUTER_ADV_M8\n    TraceRouteRunState advState() const { return runState; }\n    const String& advResult() const { return resultText; }\n    uint32_t advCooldown(uint32_t now) const { return initialized && lastTraceRouteTime && uint32_t(now-lastTraceRouteTime)<cooldownMs ? cooldownMs-uint32_t(now-lastTraceRouteTime) : 0; }\n#endif')
    replace('main.cpp', 'void setup()\n' , 'void meshtastic_setup()\n')
    replace('main.cpp', 'void loop()\n', 'void meshtastic_loop()\n')
    replace('main.cpp', '    initSPI();', '    if (!spiLock) initSPI();')
    replace('main.cpp', '    auto rIf = initLoRa();\n\n    lateInitVariant();', '    auto rIf = initLoRa();\n    if (!rIf) RadioLibInterface::instance = nullptr;\n\n    lateInitVariant();')
    replace('main.cpp', '    nodeDB = new NodeDB;', '    nodeDB = new NodeDB;\n    if (cardputer_adv::core_storage_fault()) return;')
    replace('main.cpp', '    if (!runASAP && loopCanSleep()) {', '    if (false && !runASAP && loopCanSleep()) {')
    replace('main.cpp', '#include "configuration.h"', '#include "configuration.h"\n#include "cardputer_adv/core_storage.hpp"\n#include "cardputer_adv/shared_spi.hpp"')
    replace('main.cpp', '    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);', '    cardputer_adv::shared_spi_begin();')
    replace('FSCommon.h', '#include "LittleFS.h"\n#define FSCom LittleFS\n#define FSBegin() FSCom.begin(true) // format on failure', '#include "cardputer_adv/core_storage.hpp"\n#define FSCom cardputer_adv::core_filesystem\n#define FSBegin() FSCom.begin()')
    replace('concurrency/Lock.cpp', 'xSemaphoreCreateBinary()', 'xSemaphoreCreateRecursiveMutex()')
    replace('concurrency/Lock.cpp', '    assert(handle);\n    if (xSemaphoreGive(handle) == false) {\n        abort();\n    }', '    assert(handle);')
    replace('concurrency/Lock.cpp', 'xSemaphoreTake(handle, portMAX_DELAY)', 'xSemaphoreTakeRecursive(handle, portMAX_DELAY)')
    replace('concurrency/Lock.cpp', 'xSemaphoreGive(handle)', 'xSemaphoreGiveRecursive(handle)')
    # Launcher NVS is not used as application storage or boot metadata.
    p = stage / 'src/platform/esp32/main-esp32.cpp'
    text = p.read_text()
    start = text.index('    nvs_stats_t nvs_stats;')
    end = text.index('#if !MESHTASTIC_EXCLUDE_WIFI', start)
    write(p, (text[:start] + "    esp_err_t res;\n" + text[end:]).encode())
    replace('mesh/NodeDB.cpp', '    Preferences preferences;\n    preferences.begin("meshtastic", false);\n    myNodeInfo.reboot_count = preferences.getUInt("rebootCounter", 0);\n    preferences.end();', '    myNodeInfo.reboot_count = 0;')
    replace('mesh/NodeDB.cpp', '            state = LoadFileResult::DECODE_FAILED;', '            state = LoadFileResult::DECODE_FAILED;\n            cardputer_adv::core_storage_set_fault();')
    replace('mesh/NodeDB.cpp', '    return okay;\n}\n\nbool NodeDB::saveChannelsToDisk()', '    return okay && writeSucceeded;\n}\n\nbool NodeDB::saveChannelsToDisk()')
    replace('mesh/NodeDB.cpp', 'bool NodeDB::saveNodeDatabaseToDisk()\n{', 'bool NodeDB::saveNodeDatabaseToDisk()\n{\n    cardputer_adv::CoreSaveGuard saveGuard;\n    if (cardputer_adv::core_storage_fault()) return false;')
    replace('mesh/NodeDB.cpp', '    return saveProto(nodeDatabaseFileName, nodeDatabaseSize, &meshtastic_NodeDatabase_msg, &nodeDatabase, false);', '    bool saved = saveProto(nodeDatabaseFileName, nodeDatabaseSize, &meshtastic_NodeDatabase_msg, &nodeDatabase, true);\n    return saved && cardputer_adv::core_storage_commit();')
    replace('mesh/Channels.h', '    Channels() {}', '    Channels() {}\n    CryptoKey storageKey(ChannelIndex index) { return getKey(index); }')
    replace('SafeFile.h', '    uint8_t hash = 0;', '    uint8_t hash = 0;\n#ifdef CARDPUTER_ADV_M3\n    uint32_t storageCrc = 0;\n    size_t storageBytes = 0;\n#endif')
    replace('SafeFile.cpp', '#include "SafeFile.h"', '#include "SafeFile.h"\n#ifdef CARDPUTER_ADV_M3\n#include "cardputer_adv/store_codec.hpp"\n#endif')
    replace('SafeFile.cpp', '    hash ^= ch;\n    return f.write(ch);', '    hash ^= ch;\n#ifdef CARDPUTER_ADV_M3\n    storageCrc = cardputer_adv::store_crc(&ch, 1, storageCrc);\n    ++storageBytes;\n#endif\n    return f.write(ch);')
    replace('SafeFile.cpp', '    return f.write((uint8_t const *)buffer, size);', '#ifdef CARDPUTER_ADV_M3\n    storageCrc = cardputer_adv::store_crc(buffer, size, storageCrc);\n    storageBytes += size;\n#endif\n    return f.write((uint8_t const *)buffer, size);')
    replace('SafeFile.cpp', 'bool SafeFile::testReadback()\n{\n    concurrency::LockGuard g(spiLock);', 'bool SafeFile::testReadback()\n{\n#ifndef CARDPUTER_ADV_M3\n    concurrency::LockGuard g(spiLock);\n#endif')
    replace('SafeFile.cpp', '    int c = 0;', '#ifdef CARDPUTER_ADV_M3\n    uint8_t buffer[256];\n    uint32_t crc = 0;\n    size_t total = 0, count;\n    while ((count = f2.read(buffer, sizeof(buffer))) > 0) {\n        crc = cardputer_adv::store_crc(buffer, count, crc);\n        total += count;\n    }\n    f2.close();\n    return total == storageBytes && crc == storageCrc;\n#else\n    int c = 0;')
    replace('SafeFile.cpp', '    return true;\n}\n\n#endif', '    return true;\n#endif\n}\n\n#endif')
    replace('SafeFile.h', 'bool fullAtomic = false', 'bool fullAtomic = true')
    replace('mesh/NodeDB.cpp', 'bool NodeDB::saveToDisk(int saveWhat)\n{', 'bool NodeDB::saveToDisk(int saveWhat)\n{\n    cardputer_adv::CoreSaveGuard saveGuard;\n    if (cardputer_adv::core_storage_fault()) return false;')
    replace('mesh/NodeDB.cpp', '    return success;\n}\n\nconst meshtastic_NodeInfoLite *NodeDB::readNextMeshNode', '    if (success) success = cardputer_adv::core_storage_commit();\n    return success;\n}\n\nconst meshtastic_NodeInfoLite *NodeDB::readNextMeshNode')
    replace('mesh/NodeDB.cpp', '    loadFromDisk();\n    cleanupMeshDB();', '    loadFromDisk();\n    if (cardputer_adv::core_storage_fault()) return;\n    cleanupMeshDB();')
    for field in ('devicestate', 'config', 'moduleConfig', 'channelFile'):
        # Existing configuration is never replaced by defaults on an incompatible schema.
        old = f"    if ((state != LoadFileResult::LOAD_SUCCESS) || (devicestate.version < DEVICESTATE_MIN_VER)) {{" if field == 'devicestate' else f"        if ({field}.version < DEVICESTATE_MIN_VER) {{"
        new = f"    if (cardputer_adv::core_storage_existing() && {field}.version != DEVICESTATE_CUR_VER) {{ cardputer_adv::core_storage_set_fault(); return; }}\n" + old
        replace('mesh/NodeDB.cpp', old, new)
    p = stage / 'src/mesh/NodeDB.cpp'
    text = p.read_text()
    start = text.index('bool NodeDB::factoryReset(bool eraseBleBonds)')
    end = text.index('void NodeDB::installDefaultNodeDatabase()', start)
    write(p, (text[:start] + 'bool NodeDB::factoryReset(bool)\n{\n    cardputer_adv::core_storage_set_fault();\n    return false;\n}\n\n' + text[end:]).encode())
    replace('mesh/RadioLibInterface.cpp', '#include "RadioLibInterface.h"', '#include "RadioLibInterface.h"\n#include "cardputer_adv/mesh_backend.hpp"')
    replace('mesh/RadioLibInterface.cpp', '    if (sendingPacket)\n        completeSending();', '    if (sendingPacket) {\n        cardputer_adv::mesh_radio_transmitted(sendingPacket->from, sendingPacket->id);\n        completeSending();\n    }')
    variant=(upstream/'variants/esp32s3/m5stack_cardputer_adv/variant.h').read_text()
    variant+='\n#undef HAS_I2S\n#undef HAS_NEOPIXEL\n#undef HAS_BMI270\n#undef BUTTON_PIN\n#undef I2C_SDA1\n#undef I2C_SCL1\n'
    write(stage/'variant/variant.h',variant.encode())
    return stage
