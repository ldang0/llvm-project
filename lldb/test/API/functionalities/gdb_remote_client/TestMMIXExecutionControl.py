import lldb
import threading
import time
from lldbsuite.test.decorators import *
from lldbsuite.test.gdbclientutils import *
from lldbsuite.test.lldbgdbclient import GDBRemoteTestBase

from TestMMIXRegisterMemory import MMIXResponder


class MMIXStepResponder(MMIXResponder):
    transitions = [
        ("sequential", 0x1000, 0x1004),
        ("taken-branch", 0x1010, 0x1030),
        ("untaken-branch", 0x1020, 0x1024),
        ("call", 0x1040, 0x1080),
        ("return", 0x1080, 0x1044),
        ("trap-boundary", 0x1090, 0x1094),
    ]

    def _respond_impl(self, packet):
        if packet == "s" or packet.startswith("vCont;s"):
            transitions = {before: after for _, before, after in self.transitions}
            self.registers[288] = transitions[self.registers[288]]
            return "T05thread:1;"
        return super()._respond_impl(packet)


class MMIXInterruptResponder(MMIXResponder):
    def cont(self):
        return self.RESPONSE_NONE

    def interrupt(self):
        return "T02thread:1;"


class TestMMIXExecutionControl(GDBRemoteTestBase):
    def connect_mmix(self, responder):
        self.server.responder = responder
        self.dbg.SetDefaultArchitecture("mmix")
        target = self.dbg.CreateTargetWithFileAndArch(None, None)
        process = self.connect(target)
        return process, process.GetThreadAtIndex(0)

    @skipIfXmlSupportMissing
    @skipIfRemote
    @skipIfLLVMTargetMissing("MMIX")
    def test_instruction_step_transitions(self):
        responder = MMIXStepResponder()
        process, thread = self.connect_mmix(responder)

        expected_cases = {
            "sequential",
            "taken-branch",
            "untaken-branch",
            "call",
            "return",
            "trap-boundary",
        }
        self.assertEqual(
            {name for name, _, _ in MMIXStepResponder.transitions},
            expected_cases,
        )
        for _, before, after in MMIXStepResponder.transitions:
            responder.registers[288] = before
            thread.StepInstruction(False)
            self.assertState(process.GetState(), lldb.eStateStopped)
            self.assertStopReason(thread.GetStopReason(),
                                  lldb.eStopReasonPlanComplete)
            self.assertEqual(thread.GetFrameAtIndex(0).GetPC(), after)

        packets = responder.packetLog.get_received()
        self.assertEqual(sum(packet == "s" for packet in packets),
                         len(MMIXStepResponder.transitions))

    @skipIfXmlSupportMissing
    @skipIfRemote
    @skipIfLLVMTargetMissing("MMIX")
    def test_async_interrupt(self):
        responder = MMIXInterruptResponder()
        process, thread = self.connect_mmix(responder)

        continue_thread = threading.Thread(target=process.Continue)
        continue_thread.start()
        for _ in range(100):
            if "c" in responder.packetLog.get_received():
                break
            time.sleep(0.01)
        self.assertIn("c", responder.packetLog.get_received())
        process.SendAsyncInterrupt()
        continue_thread.join(1)
        self.assertFalse(continue_thread.is_alive())
        self.assertState(process.GetState(), lldb.eStateStopped)
        self.assertStopReason(thread.GetStopReason(), lldb.eStopReasonSignal)
        self.assertEqual(thread.GetStopReasonDataAtIndex(0), 2)
