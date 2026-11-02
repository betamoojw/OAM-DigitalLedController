import re
import unittest
from pathlib import Path


ROOT = Path(__file__).parents[1]


class FirmwareStartupTests(unittest.TestCase):
    def test_runtime_uses_declared_firmware_revision(self):
        source = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
        self.assertIn("const uint8_t firmwareRevision = 0;", source)
        self.assertIn("openknx.init(firmwareRevision);", source)

    def test_modules_have_stable_unique_flash_ids(self):
        source = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
        modules = re.findall(r"openknx\.addModule\((\d+),\s*(\w+)\);", source)
        self.assertEqual(
            modules,
            [
                ("1", "digitalLedModule"),
                ("2", "openknxFunctionBlocksModule"),
                ("3", "openknxLogic"),
                ("4", "openknxVirtualButtonModule"),
            ],
        )
        self.assertEqual(len({module_id for module_id, _ in modules}), len(modules))

    def test_framework_initializes_before_module_registration_and_setup(self):
        source = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
        init_position = source.index("openknx.init(firmwareRevision);")
        first_module_position = source.index("openknx.addModule(")
        setup_position = source.index("openknx.setup();")
        self.assertLess(init_position, first_module_position)
        self.assertLess(first_module_position, setup_position)


if __name__ == "__main__":
    unittest.main()
