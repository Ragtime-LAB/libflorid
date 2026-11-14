"""Exercise the real Tk UI with the real asynchronous serial SDK and a simulated byte device."""
from pathlib import Path
import sys
import time
import unittest
import tkinter as tk
ROOT=Path(__file__).resolve().parents[3]
sys.path[:0]=[str(ROOT/'tests'),str(ROOT/'tools/underwater_single_console')]
from test_single_python import sdk, Port
from pyflorid._single_serial import SerialBackend
from console import Console, Session

class ConsoleTests(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk()
        self.app=Console(self.root,sdk)
        self.port=Port()
        self.device=sdk.UnderwaterSingle('serial://test',backend=SerialBackend('fake',transport=self.port))
        self.app.transport.set('UART7 / RS232')
        self.app.attach(self.device)
        self.pump(.12)

    def pump(self, seconds):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            self.root.update()
            time.sleep(.003)

    def tearDown(self):
        if not self.app.closed:
            self.app.close()
        self.app.commands.shutdown(wait=True)
        self.app.upgrades.shutdown(wait=True)
        self.app.urgent.shutdown(wait=True)

    def test_four_rows_explicit_control_and_damp_while_settings_wait(self):
        self.assertEqual(len(self.app.table.get_children()),4)
        self.assertEqual(self.port.commands,[])
        self.assertEqual(self.port.mode_requests,[])
        self.app.tabs.select(1)
        self.app.start_control()
        self.pump(.08)
        self.assertEqual(self.port.mode,3)
        self.assertGreater(len(self.port.commands),5)
        self.port.settings_delay=.25
        self.app.gravity[0].set('.75')
        self.app.save_settings()
        self.pump(.02)
        self.app.call('失能',lambda d:d.disable())
        self.pump(.08)
        self.assertEqual(self.port.mode,0)
        self.assertFalse(self.device.controlling)
        self.pump(.3)
        self.assertIn('revision=',self.app.storage.cget('text'))

    def test_invalid_field_does_not_block_disable_or_send_partial_target(self):
        self.app.start_control()
        self.pump(.06)
        before=self.port.commands[-1]
        self.app.values[1][2].set('bad')
        self.app.commit()
        self.assertIn('Kp',self.app.notice.get())
        self.assertTrue(self.app.entries[1][2].instate(['invalid']))
        self.pump(.02)
        self.assertEqual(self.port.commands[-1][5:],before[5:])
        self.app.call('失能',lambda d:d.disable())
        self.pump(.06)
        self.assertEqual(self.port.mode,0)
        self.app.upload()
        self.assertIn('USB',self.app.notice.get())

    def test_retiring_connection_stops_sender_before_waiting_for_calls(self):
        self.app.start_control()
        self.pump(.06)
        self.port.settings_delay=.25
        self.app.save_settings()
        self.pump(.02)
        self.app.disconnect()
        self.assertFalse(self.device.controlling)
        self.assertEqual(self.app.mode.cget('text'),'未连接')
        self.pump(.3)
        self.assertTrue(self.port.closed)

if __name__=='__main__':
    unittest.main()
