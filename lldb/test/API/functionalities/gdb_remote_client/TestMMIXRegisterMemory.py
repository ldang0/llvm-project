import lldb
from lldbsuite.test.decorators import *
from lldbsuite.test.gdbclientutils import *
from lldbsuite.test.lldbgdbclient import GDBRemoteTestBase


SPECIAL_REGISTERS = [
    "rB", "rD", "rE", "rH", "rJ", "rM", "rR", "rBB",
    "rC", "rN", "rO", "rS", "rI", "rT", "rTT", "rK",
    "rQ", "rU", "rV", "rG", "rL", "rA", "rF", "rP",
    "rW", "rX", "rY", "rZ", "rWW", "rXX", "rYY", "rZZ",
]


def target_xml():
    registers = []
    for number in range(256):
        generic = ""
        if number == 253:
            generic = ' generic="fp"'
        elif number == 254:
            generic = ' generic="sp"'
        registers.append(
            f'<reg name="r{number}" bitsize="64" regnum="{number}" '
            f'type="uint64" group="general"{generic}/>'
        )
    for number, name in enumerate(SPECIAL_REGISTERS, 256):
        registers.append(
            f'<reg name="{name}" bitsize="64" regnum="{number}" '
            'type="uint64" group="special"/>'
        )
    registers.append(
        '<reg name="pc" bitsize="64" regnum="288" type="code_ptr" '
        'group="general" generic="pc"/>'
    )
    return (
        '<?xml version="1.0"?><target><architecture>mmix</architecture>'
        '<feature name="org.gnu.gdb.mmix.core">'
        + "".join(registers)
        + "</feature></target>"
    )


class MMIXResponder(MockGDBServerResponder):
    def __init__(self):
        super().__init__()
        self.registers = [0] * 289
        self.registers[231] = 0x1122334455667788
        self.registers[254] = 0x8000000000002000
        self.registers[266] = 0x8000000000000100
        self.registers[267] = 0x8000000000000800
        self.registers[288] = 0x1020304050607080
        self.memory = {address: 0 for address in range(0x2000, 0x2200)}
        self.memory.update(
            {address: byte
             for address, byte in enumerate(b"MMIXDATA", start=0x2000)}
        )

    @staticmethod
    def encode(value):
        return value.to_bytes(8, "big").hex()

    def qXferRead(self, obj, annex, offset, length):
        if obj == "features" and annex == "target.xml":
            xml = target_xml()
            return xml[offset : offset + length], offset + length < len(xml)
        return None, False

    def qHostInfo(self):
        return "ptrsize:8;endian:big;"

    def qfThreadInfo(self):
        return "m1"

    def qC(self):
        return "QC1"

    def haltReason(self):
        return "T05thread:1;"

    def readRegisters(self):
        return "".join(self.encode(value) for value in self.registers)

    def readRegister(self, register):
        return self.encode(self.registers[register])

    def writeRegister(self, register, value_hex):
        self.registers[register] = int.from_bytes(bytes.fromhex(value_hex), "big")
        return "OK"

    def readMemory(self, addr, length):
        try:
            return bytes(self.memory[addr + offset] for offset in range(length)).hex()
        except KeyError:
            return "E01"

    def writeMemory(self, addr, data_hex):
        data = bytes.fromhex(data_hex)
        if any(addr + offset not in self.memory for offset in range(len(data))):
            return "E01"
        for offset, byte in enumerate(data):
            self.memory[addr + offset] = byte
        return "OK"


class MMIXTruncatedRegisterResponder(MMIXResponder):
    def readRegister(self, register):
        if register == 231:
            return "0011"
        return super().readRegister(register)


class TestMMIXRegisterMemory(GDBRemoteTestBase):
    def connect_mmix(self, responder):
        self.server.responder = responder
        self.dbg.SetDefaultArchitecture("mmix")
        target = self.dbg.CreateTargetWithFileAndArch(None, None)
        process = self.connect(target)
        return process, process.GetThreadAtIndex(0).GetFrameAtIndex(0)

    @skipIfXmlSupportMissing
    @skipIfRemote
    @skipIfLLVMTargetMissing("MMIX")
    def test_individual_register_access(self):
        responder = MMIXResponder()
        _, frame = self.connect_mmix(responder)

        self.assertEqual(
            frame.FindRegister("$231").GetValueAsUnsigned(),
            0x1122334455667788,
        )
        self.assertEqual(
            frame.FindRegister("pc").GetValueAsUnsigned(),
            0x1020304050607080,
        )
        self.assertEqual(
            frame.FindRegister("rO").GetValueAsUnsigned(),
            0x8000000000000100,
        )
        self.assertEqual(
            frame.FindRegister("rS").GetValueAsUnsigned(),
            0x8000000000000800,
        )
        packets = responder.packetLog.get_received()
        self.assertIn("pe7", packets)
        self.assertIn("p10a", packets)
        self.assertIn("p10b", packets)
        self.assertIn("p120", packets)

    @skipIfXmlSupportMissing
    @skipIfRemote
    @skipIfLLVMTargetMissing("MMIX")
    def test_bulk_register_and_memory_access(self):
        self.dbg.HandleCommand(
            "settings set plugin.process.gdb-remote.use-g-packet-for-reading true"
        )
        self.addTearDownHook(
            lambda: self.runCmd(
                "settings set plugin.process.gdb-remote.use-g-packet-for-reading false"
            )
        )
        responder = MMIXResponder()
        process, frame = self.connect_mmix(responder)

        self.assertEqual(
            frame.FindRegister("$231").GetValueAsUnsigned(),
            0x1122334455667788,
        )
        self.assertEqual(
            frame.FindRegister("pc").GetValueAsUnsigned(),
            0x1020304050607080,
        )
        self.assertEqual(
            frame.FindRegister("rO").GetValueAsUnsigned(),
            0x8000000000000100,
        )
        self.assertEqual(
            frame.FindRegister("rS").GetValueAsUnsigned(),
            0x8000000000000800,
        )

        self.assertTrue(frame.FindRegister("$231").SetValueFromCString(
            "0x8877665544332211"
        ))
        self.assertTrue(frame.FindRegister("pc").SetValueFromCString(
            "0x0102030405060708"
        ))
        self.assertEqual(responder.registers[231], 0x8877665544332211)
        self.assertEqual(responder.registers[288], 0x0102030405060708)
        self.assertEqual(responder.registers[266], 0x8000000000000100)
        self.assertEqual(responder.registers[267], 0x8000000000000800)

        error = lldb.SBError()
        memory = process.ReadMemory(0x2000, 8, error)
        self.assertSuccess(error)
        self.assertEqual(memory, b"MMIXDATA")
        self.assertEqual(process.WriteMemory(0x2000, b"mmixdata", error), 8)
        self.assertSuccess(error)
        self.assertEqual(process.ReadMemory(0x2000, 8, error), b"mmixdata")
        self.assertSuccess(error)
        self.assertIsNone(process.ReadMemory(0x3000, 8, error))
        self.assertFailure(error)

        packets = responder.packetLog.get_received()
        self.assertTrue(any(packet == "g" for packet in packets))
        self.assertTrue(any(packet.startswith("Pe7=8877665544332211")
                            for packet in packets))
        self.assertTrue(any(packet.startswith("P120=0102030405060708")
                            for packet in packets))

    @skipIfXmlSupportMissing
    @skipIfRemote
    @skipIfLLVMTargetMissing("MMIX")
    def test_truncated_register_value_is_unavailable(self):
        _, frame = self.connect_mmix(MMIXTruncatedRegisterResponder())
        value = frame.FindRegister("$231")
        self.assertTrue(value.IsValid())
        self.assertFailure(value.GetError())
        self.assertIsNone(value.GetValue())
