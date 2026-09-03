import lldb
from lldbsuite.test.decorators import *
from lldbsuite.test.gdbclientutils import *
from lldbsuite.test.lldbgdbclient import GDBRemoteTestBase

from TestMMIXRegisterMemory import MMIXResponder


class MMIXBreakpointResponder(MMIXResponder):
    def __init__(self):
        super().__init__()
        self.registers[288] = 0x1000
        self.breakpoints = set()

    def setBreakpoint(self, packet):
        kind, address, size = packet[1:].split(",")
        if kind != "0" or int(size, 16) != 4:
            return "E01"
        address = int(address, 16)
        if address % 4 or address not in range(0x1000, 0x1100):
            return "E01"
        self.breakpoints.add(address)
        return "OK"

    def other(self, packet):
        if packet.startswith("z"):
            kind, address, size = packet[1:].split(",")
            address = int(address, 16)
            if kind != "0" or int(size, 16) != 4 or address not in self.breakpoints:
                return "E01"
            self.breakpoints.remove(address)
            return "OK"
        return super().other(packet)

    def cont(self):
        following = [address for address in self.breakpoints
                     if address >= self.registers[288]]
        self.registers[288] = min(following) if following else 0x10FC
        return "T05thread:1;"

    def _respond_impl(self, packet):
        if packet == "s" or packet.startswith("vCont;s"):
            self.registers[288] += 4
            return "T05thread:1;"
        return super()._respond_impl(packet)


class TestMMIXBreakpoints(GDBRemoteTestBase):
    def connect_mmix(self, responder):
        self.server.responder = responder
        self.dbg.SetDefaultArchitecture("mmix")
        target = self.dbg.CreateTargetWithFileAndArch(None, None)
        return target, self.connect(target)

    @skipIfXmlSupportMissing
    @skipIfRemote
    @skipIfLLVMTargetMissing("MMIX")
    def test_remote_software_breakpoints(self):
        responder = MMIXBreakpointResponder()
        target, process = self.connect_mmix(responder)

        first = target.BreakpointCreateByAddress(0x1020)
        self.assertEqual(first.GetNumLocations(), 1)
        self.assertEqual(responder.breakpoints, {0x1020})

        adjacent = target.BreakpointCreateByAddress(0x1024)
        duplicate = target.BreakpointCreateByAddress(0x1020)
        self.assertEqual(adjacent.GetNumLocations(), 1)
        self.assertEqual(duplicate.GetNumLocations(), 1)
        self.assertEqual(responder.breakpoints, {0x1020, 0x1024})

        process.Continue()
        thread = process.GetThreadAtIndex(0)
        self.assertStopReason(thread.GetStopReason(), lldb.eStopReasonBreakpoint)
        self.assertEqual(thread.GetFrameAtIndex(0).GetPC(), 0x1020)
        self.assertEqual(first.GetHitCount(), 1)

        process.Continue()
        self.assertEqual(thread.GetFrameAtIndex(0).GetPC(), 0x1024)
        self.assertEqual(first.GetHitCount(), 1)
        self.assertEqual(adjacent.GetHitCount(), 1)

        packets = responder.packetLog.get_received()
        self.assertIn("Z0,1020,4", packets)
        self.assertIn("Z0,1024,4", packets)
        self.assertIn("z0,1020,4", packets)

    @skipIfXmlSupportMissing
    @skipIfRemote
    @skipIfLLVMTargetMissing("MMIX")
    def test_rejects_invalid_breakpoint_requests(self):
        responder = MMIXBreakpointResponder()
        target, _ = self.connect_mmix(responder)

        unaligned = target.BreakpointCreateByAddress(0x1021)
        invalid = target.BreakpointCreateByAddress(lldb.LLDB_INVALID_ADDRESS)
        read_only = target.BreakpointCreateByAddress(0x2000)
        self.assertEqual(unaligned.GetNumLocations(), 1)
        self.assertEqual(invalid.GetNumLocations(), 1)
        self.assertEqual(read_only.GetNumLocations(), 1)
        self.assertFalse(responder.breakpoints)
        packets = responder.packetLog.get_received()
        self.assertIn("Z0,1021,4", packets)
        self.assertIn("Z0,2000,4", packets)
