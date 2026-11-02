import unittest
import xml.etree.ElementTree as ET
from pathlib import Path


ROOT = Path(__file__).parents[1]
NS = {"knx": "http://knx.org/xml/project/20", "op": "http://github.com/OpenKNX/OpenKNXproducer"}


class EtsXmlTests(unittest.TestCase):
    def test_dynamic_template_exposes_every_parameter_and_object_once(self):
        module_root = ET.parse(
            ROOT / "lib" / "OFM-DigitalLEDController" / "src" / "DigitalLedModule.share.xml"
        ).getroot()
        template_root = ET.parse(
            ROOT / "lib" / "OFM-DigitalLEDController" / "src" / "DigitalLedModule.templ.xml"
        ).getroot()

        parameter_refs = {
            node.attrib["Id"] for node in module_root.findall(".//knx:ParameterRef", NS)
        }
        visible_parameter_refs = [
            node.attrib["RefId"] for node in template_root.findall(".//knx:ParameterRefRef", NS)
        ]
        self.assertEqual(len(parameter_refs), 75)
        self.assertCountEqual(visible_parameter_refs, parameter_refs)

        object_refs = {
            node.attrib["Id"] for node in template_root.findall(".//knx:ComObjectRef", NS)
        }
        visible_object_refs = [
            node.attrib["RefId"] for node in template_root.findall(".//knx:ComObjectRefRef", NS)
        ]
        self.assertEqual(len(object_refs), 209)
        self.assertCountEqual(visible_object_refs, object_refs)

        for choice in template_root.findall(".//knx:choose", NS):
            self.assertIn(choice.attrib["ParamRefId"], parameter_refs)
            for branch in choice.findall("knx:when", NS):
                self.assertEqual(branch.attrib["test"], "1")

        parameter_block = template_root.find(".//knx:ParameterBlock", NS)
        self.assertIsNotNone(parameter_block)

        def parameter_ref_id(number):
            return f"%AID%_UP-%TT%{number:05d}_R-%TT%{number:05d}01"

        def object_ref_id(number):
            return f"%AID%_O-%TT%%CC%{number:03d}_R-%TT%%CC%{number:03d}01"

        direct_parameters = {
            node.attrib["RefId"] for node in parameter_block.findall("knx:ParameterRefRef", NS)
        }
        conditional_parameters = {}
        conditional_objects = {}
        for choice in parameter_block.findall("knx:choose", NS):
            controller = choice.attrib["ParamRefId"]
            conditional_parameters.setdefault(controller, set()).update(
                node.attrib["RefId"] for node in choice.findall(".//knx:ParameterRefRef", NS)
            )
            conditional_objects.setdefault(controller, set()).update(
                node.attrib["RefId"] for node in choice.findall(".//knx:ComObjectRefRef", NS)
            )

        expected_parameter_groups = {
            15: {17, 18},
            23: set(range(30, 49)),
            24: {29, 49, 50, 51, 52, 53},
            55: {62},
            56: {63},
            64: {65, 66, 67},
            71: {72},
            73: {74},
        }
        self.assertEqual(
            {controller: values for controller, values in conditional_parameters.items() if values},
            {
                parameter_ref_id(controller): {
                    parameter_ref_id(number) for number in numbers
                }
                for controller, numbers in expected_parameter_groups.items()
            },
        )
        expected_direct_parameters = set(range(1, 76)) - set().union(
            *expected_parameter_groups.values()
        )
        self.assertEqual(
            direct_parameters,
            {parameter_ref_id(number) for number in expected_direct_parameters},
        )

        expected_object_groups = {
            14: set(range(54, 68)),
            15: set(range(68, 76)),
            16: {76, 77, 78, 79, 82, 83},
            20: set(range(84, 96)),
            21: set(range(96, 106)),
            22: set(range(106, 110)),
            23: {80, 81, 110, 111},
            24: set(range(112, 119)),
            25: set(range(119, 123)),
            26: set(range(123, 128)),
            27: set(range(128, 133)),
            28: set(range(133, 136)),
            54: set(range(136, 143)),
            55: set(range(143, 150)),
            56: set(range(150, 153)),
            57: {153},
            58: {154, 155},
            59: set(range(156, 159)),
            60: set(range(159, 163)),
            61: set(range(163, 166)),
            64: set(range(166, 184)),
            68: set(range(184, 192)),
            69: set(range(192, 196)),
            70: {196, 197},
            71: set(range(198, 203)),
            73: set(range(203, 206)),
            75: set(range(206, 210)),
        }
        self.assertEqual(
            {controller: values for controller, values in conditional_objects.items() if values},
            {
                parameter_ref_id(controller): {
                    object_ref_id(number) for number in numbers
                }
                for controller, numbers in expected_object_groups.items()
            },
        )
        direct_objects = {
            node.attrib["RefId"] for node in parameter_block.findall("knx:ComObjectRefRef", NS)
        }
        self.assertEqual(direct_objects, {object_ref_id(number) for number in range(1, 54)})

    def test_development_and_release_products_have_compatibility_metadata(self):
        for filename in ("DigitalLEDController.xml", "DigitalLEDController-Release.xml"):
            root = ET.parse(ROOT / "src" / filename).getroot()
            self.assertEqual(root.tag, "{http://knx.org/xml/project/20}KNX")
            minimum = root.find("op:minOpenKNXproducerVersion", NS)
            self.assertIsNotNone(minimum)
            self.assertEqual(minimum.attrib["value"], "4.3.12")
            self.assertIsNotNone(root.find("op:ETS", NS))
            self.assertIsNotNone(root.find("knx:generate", NS))

    def test_development_and_release_products_include_config_transfer(self):
        config_root = ET.parse(ROOT / "src" / "DigitalLEDController.conf.xml").getroot()
        config_values = {node.attrib["name"]: node.attrib["value"] for node in config_root.findall("op:config", NS)}
        self.assertEqual(config_values["%UCT_VerifyVersion%"], "0.5")

        for filename in ("DigitalLEDController.xml", "DigitalLEDController-Release.xml"):
            root = ET.parse(ROOT / "src" / filename).getroot()
            modules = {node.attrib["prefix"]: node for node in root.findall("op:define", NS)}
            config_transfer = modules["UCT"]
            self.assertEqual(config_transfer.attrib["ModuleType"], "99")
            self.assertEqual(config_transfer.attrib["noConfigTransfer"], "true")
            self.assertEqual(config_transfer.attrib["share"], "../lib/OFM-ConfigTransfer/src/ConfigTransfer.share.xml")

            verify = config_transfer.find("op:verify", NS)
            self.assertIsNotNone(verify)
            self.assertEqual(verify.attrib["File"], "../lib/OFM-ConfigTransfer/library.json")
            self.assertEqual(verify.attrib["ModuleVersion"], "%UCT_VerifyVersion%")

    def test_module_xml_exposes_runtime_parameters_and_help(self):
        root = ET.parse(ROOT / "lib" / "OFM-DigitalLEDController" / "src" / "DigitalLedModule.share.xml").getroot()
        names = {node.attrib["Name"] for node in root.findall(".//knx:Parameter", NS)}
        self.assertTrue({
            "RequestXInterval",
            "NightlightDuration",
            "NightlightMode",
            "NightlightTargetBrightness",
            "DefaultBrightness",
            "DefaultPalette",
            "DefaultEffect",
            "DefaultEffectSpeed",
            "DefaultEffectIntensity",
            "CctKelvinMin",
            "CctKelvinMax",
            "DefaultTransitionMs",
            "SegmentId",
            "EnableSegmentObjects",
            "EnableSyncObjects",
            "EnableAdvancedObjects",
            "DefaultSyncSendGroups",
            "DefaultSyncReceiveGroups",
            "SuppressSyncNotifications",
            "EnableEffectOptionObjects",
            "EnableSegmentLayoutObjects",
            "EnableSecondaryColorObjects",
            "EnablePlaylistObjects",
            "EnablePresetManagementObjects",
            "EnableRealtimeObjects",
            "EnableHealthObjects",
            "EnableCapabilityObjects",
            "EnableLedmapObjects",
            "PlaylistPresetSlot",
            "PlaylistPreset1",
            "PlaylistDuration1",
            "PlaylistTransition1",
            "EnableDestructivePresetActions",
            "EnableNightlightRuntimeObjects",
            "EnableSegmentManagementObjects",
            "EnablePlaylistRuntimeObjects",
            "EnableSyncNodeObjects",
            "EnablePowerObjects",
            "EnableNetworkObjects",
            "EnableLedInfoObjects",
            "EnableTimeObjects",
            "EnableDestructiveSegmentActions",
            "PlaylistStopPreset",
            "EnableMultiSegmentObjects",
            "ManagedSegment1Id",
            "ManagedSegment2Id",
            "ManagedSegment3Id",
            "EnableMatrix2dObjects",
            "EnableFilesystemObjects",
            "EnableRealtimeSourceObjects",
            "EnableCommunicationHealthObjects",
            "CommunicationTimeoutSeconds",
            "EnablePowerAlarmObjects",
            "PowerWarningPercent",
            "EnableSegmentCapabilityObjects",
        }.issubset(names))

        objects = {node.attrib["Name"]: node for node in root.findall(".//knx:ComObject", NS)}
        expected_dpts = {
            "Ch%C%White": "DPST-5-010",
            "Ch%C%Cct": "DPST-7-600",
            "Ch%C%WarmWhite": "DPST-5-010",
            "Ch%C%ColdWhite": "DPST-5-010",
            "Ch%C%Hue": "DPST-5-003",
            "Ch%C%Saturation": "DPST-5-001",
            "Ch%C%Value": "DPST-5-001",
            "Ch%C%RGBW": "DPST-251-600",
            "Ch%C%RedRel": "DPST-3-7",
            "Ch%C%EffectRel": "DPST-3-7",
            "Ch%C%SegmentSw": "DPST-1-1",
            "Ch%C%SegmentBrightness": "DPST-5-001",
            "Ch%C%SegmentFreeze": "DPST-1-1",
            "Ch%C%Transition": "DPST-7-002",
            "Ch%C%SyncSend": "DPST-1-1",
            "Ch%C%SyncSendGroups": "DPST-5-010",
            "Ch%C%LiveOverride": "DPST-5-010",
            "Ch%C%StateRequest": "DPST-1-1",
            "Ch%C%Reboot": "DPST-1-1",
            "Ch%C%EffectCustom1": "DPST-5-010",
            "Ch%C%EffectOption1": "DPST-1-1",
            "Ch%C%SegmentStart": "DPST-7-001",
            "Ch%C%SegmentOffset": "DPST-8-001",
            "Ch%C%SecondaryRGBW": "DPST-251-600",
            "Ch%C%PlaylistSave": "DPST-1-1",
            "Ch%C%PresetSave": "DPST-5-010",
            "Ch%C%LiveModeValue": "DPST-16-0",
            "Ch%C%WifiRssiValue": "DPST-6-010",
            "Ch%C%UptimeValue": "DPST-13-100",
            "Ch%C%BuildIdValue": "DPST-12-001",
            "Ch%C%Ledmap": "DPST-5-010",
            "Ch%C%Timebase": "DPST-12-001",
            "Ch%C%NightlightDuration": "DPST-5-010",
            "Ch%C%NightlightRemainingValue": "DPST-8-001",
            "Ch%C%SegmentName": "DPST-16-0",
            "Ch%C%SegmentDelete": "DPST-1-1",
            "Ch%C%PlaylistStop": "DPST-1-1",
            "Ch%C%NodeCountValue": "DPST-8-001",
            "Ch%C%LedCurrentValue": "DPST-7-001",
            "Ch%C%IpAddressValue": "DPST-12-001",
            "Ch%C%WifiBssidValue": "DPST-16-0",
            "Ch%C%LedCountValue": "DPST-7-001",
            "Ch%C%DeviceTime": "DPST-12-001",
            "Ch%C%LocalDateValue": "DPST-11-1",
            "Ch%C%LocalTimeValue": "DPST-10-1",
            "Ch%C%ManagedSegment1On": "DPST-1-1",
            "Ch%C%ManagedSegment1Brightness": "DPST-5-001",
            "Ch%C%ManagedSegment1Cct": "DPST-7-600",
            "Ch%C%ManagedSegment1Rgbw": "DPST-251-600",
            "Ch%C%ManagedSegment1Options": "DPST-5-010",
            "Ch%C%ManagedSegment1Name": "DPST-16-0",
            "Ch%C%MatrixStartY": "DPST-7-001",
            "Ch%C%MatrixTranspose": "DPST-1-1",
            "Ch%C%Map1D2D": "DPST-5-010",
            "Ch%C%FsUsedValue": "DPST-12-001",
            "Ch%C%RealtimeSourceIpValue": "DPST-12-001",
            "Ch%C%WebsocketClientsValue": "DPST-8-001",
            "Ch%C%WledOnlineState": "DPST-1-1",
            "Ch%C%LastResponseAgeValue": "DPST-13-100",
            "Ch%C%CurrentPercentValue": "DPST-5-001",
            "Ch%C%HasCctState": "DPST-1-1",
        }
        for name, dpt in expected_dpts.items():
            self.assertIn(name, objects)
            self.assertEqual(objects[name].attrib["DatapointType"], dpt)

        template = ET.parse(ROOT / "lib" / "OFM-DigitalLEDController" / "src" / "DigitalLedModule.templ.xml").getroot()
        visible_help = " ".join(node.attrib.get("Text", "") for node in template.iter())
        self.assertIn("Statusabfrage", visible_help)
        self.assertIn("Nachtlicht", visible_help)
        self.assertIn("Startwerte und Effekte", visible_help)
        self.assertIn("WLED JSON API", visible_help)

        choices = {
            node.attrib["ParamRefId"]: {
                reference.attrib["RefId"]
                for reference in node.findall(".//knx:ComObjectRefRef", NS)
            }
            for node in template.findall(".//knx:choose", NS)
        }
        self.assertIn("%AID%_O-%TT%%CC%054_R-%TT%%CC%05401", choices["%AID%_UP-%TT%00014_R-%TT%0001401"])
        self.assertIn("%AID%_O-%TT%%CC%068_R-%TT%%CC%06801", choices["%AID%_UP-%TT%00015_R-%TT%0001501"])
        self.assertIn("%AID%_O-%TT%%CC%083_R-%TT%%CC%08301", choices["%AID%_UP-%TT%00016_R-%TT%0001601"])
        self.assertIn("%AID%_O-%TT%%CC%084_R-%TT%%CC%08401", choices["%AID%_UP-%TT%00020_R-%TT%0002001"])
        self.assertIn("%AID%_O-%TT%%CC%096_R-%TT%%CC%09601", choices["%AID%_UP-%TT%00021_R-%TT%0002101"])
        self.assertIn("%AID%_O-%TT%%CC%110_R-%TT%%CC%11001", choices["%AID%_UP-%TT%00023_R-%TT%0002301"])
        self.assertIn("%AID%_O-%TT%%CC%127_R-%TT%%CC%12701", choices["%AID%_UP-%TT%00026_R-%TT%0002601"])
        self.assertIn("%AID%_O-%TT%%CC%135_R-%TT%%CC%13501", choices["%AID%_UP-%TT%00028_R-%TT%0002801"])
        self.assertIn("%AID%_O-%TT%%CC%136_R-%TT%%CC%13601", choices["%AID%_UP-%TT%00054_R-%TT%0005401"])
        self.assertIn("%AID%_O-%TT%%CC%143_R-%TT%%CC%14301", choices["%AID%_UP-%TT%00055_R-%TT%0005501"])
        self.assertIn("%AID%_O-%TT%%CC%150_R-%TT%%CC%15001", choices["%AID%_UP-%TT%00056_R-%TT%0005601"])
        self.assertIn("%AID%_O-%TT%%CC%153_R-%TT%%CC%15301", choices["%AID%_UP-%TT%00057_R-%TT%0005701"])
        self.assertIn("%AID%_O-%TT%%CC%154_R-%TT%%CC%15401", choices["%AID%_UP-%TT%00058_R-%TT%0005801"])
        self.assertIn("%AID%_O-%TT%%CC%156_R-%TT%%CC%15601", choices["%AID%_UP-%TT%00059_R-%TT%0005901"])
        self.assertIn("%AID%_O-%TT%%CC%159_R-%TT%%CC%15901", choices["%AID%_UP-%TT%00060_R-%TT%0006001"])
        self.assertIn("%AID%_O-%TT%%CC%163_R-%TT%%CC%16301", choices["%AID%_UP-%TT%00061_R-%TT%0006101"])
        self.assertIn("%AID%_O-%TT%%CC%166_R-%TT%%CC%16601", choices["%AID%_UP-%TT%00064_R-%TT%0006401"])
        self.assertIn("%AID%_O-%TT%%CC%184_R-%TT%%CC%18401", choices["%AID%_UP-%TT%00068_R-%TT%0006801"])
        self.assertIn("%AID%_O-%TT%%CC%192_R-%TT%%CC%19201", choices["%AID%_UP-%TT%00069_R-%TT%0006901"])
        self.assertIn("%AID%_O-%TT%%CC%196_R-%TT%%CC%19601", choices["%AID%_UP-%TT%00070_R-%TT%0007001"])
        self.assertIn("%AID%_O-%TT%%CC%198_R-%TT%%CC%19801", choices["%AID%_UP-%TT%00071_R-%TT%0007101"])
        self.assertIn("%AID%_O-%TT%%CC%203_R-%TT%%CC%20301", choices["%AID%_UP-%TT%00073_R-%TT%0007301"])
        self.assertIn("%AID%_O-%TT%%CC%206_R-%TT%%CC%20601", choices["%AID%_UP-%TT%00075_R-%TT%0007501"])

        numbers = sorted(int(node.attrib["Number"]) for node in objects.values())
        self.assertEqual(numbers[-1], 409)
        self.assertNotIn(410, numbers)


if __name__ == "__main__":
    unittest.main()
