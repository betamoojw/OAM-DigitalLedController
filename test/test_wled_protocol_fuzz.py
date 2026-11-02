import json
import random
import string
import unittest

from wled_protocol import WledFrameParser


class WledProtocolFuzzTests(unittest.TestCase):
    @staticmethod
    def feed_chunks(parser, payload, rng, now_ms=0):
        states = []
        position = 0
        chunk_index = 0
        while position < len(payload):
            chunk_size = rng.randint(1, 17)
            states.extend(
                parser.feed(
                    payload[position : position + chunk_size],
                    now_ms + chunk_index * 3,
                )
            )
            position += chunk_size
            chunk_index += 1
        return states

    def test_random_valid_states_survive_arbitrary_chunking(self):
        rng = random.Random(0x574C4544)
        for iteration in range(250):
            brightness = rng.randint(0, 255)
            main_segment_id = rng.randint(0, 63)
            name = "".join(
                rng.choice(string.ascii_letters + string.digits + '{}"\\')
                for _ in range(rng.randint(0, 14))
            )
            payload = json.dumps(
                {
                    "state": {
                        "on": bool(rng.getrandbits(1)),
                        "bri": brightness,
                        "mainseg": main_segment_id,
                        "seg": [{"id": 0, "n": name}],
                    }
                },
                separators=(",", ":"),
            )
            parser = WledFrameParser()
            states = self.feed_chunks(parser, "noise:" + payload + "\r\n", rng)
            self.assertEqual(len(states), 1, iteration)
            self.assertEqual(states[0].brightness, brightness, iteration)
            self.assertEqual(states[0].segment_name, name, iteration)

    def test_random_corruption_always_recovers_at_line_boundary(self):
        rng = random.Random(0x4B4E5854)
        alphabet = string.ascii_letters + string.digits + '{}[]":,\\ \t'
        valid_frame = '{"state":{"bri":42}}\r\n'
        parser = WledFrameParser(capacity=256)

        for iteration in range(500):
            noise = "".join(rng.choice(alphabet) for _ in range(rng.randint(0, 220)))
            parser.feed(noise + "\r\n", iteration * 10)
            states = parser.feed(valid_frame, iteration * 10 + 1)
            self.assertEqual([state.brightness for state in states], [42], iteration)

    def test_randomly_mutated_frames_do_not_break_following_frame(self):
        rng = random.Random(0x46555A5A)
        original = '{"state":{"on":true,"bri":128,"seg":[{"id":0,"n":"Desk"}]}}'
        replacement_characters = string.printable.replace("\r", "").replace("\n", "")

        for iteration in range(500):
            mutated = list(original)
            for _ in range(rng.randint(1, 8)):
                mutated[rng.randrange(len(mutated))] = rng.choice(replacement_characters)
            parser = WledFrameParser(capacity=256)
            parser.feed("".join(mutated) + "\r\n", iteration)
            states = parser.feed('{"state":{"bri":7}}\r\n', iteration + 1)
            self.assertEqual([state.brightness for state in states], [7], iteration)

    def test_truncated_frames_expire_at_every_sampled_boundary(self):
        rng = random.Random(0x54494D45)
        payload = '{"state":{"bri":123,"seg":[{"id":0,"n":"Timeout"}]}}'
        boundaries = rng.sample(range(1, len(payload)), 30)

        for boundary in boundaries:
            parser = WledFrameParser(timeout_ms=1000)
            parser.feed(payload[:boundary], 0)
            states = parser.feed('{"state":{"bri":9}}\r\n', 1000)
            self.assertEqual([state.brightness for state in states], [9], boundary)

    def test_capacity_boundary_counts_oversize_once_and_recovers(self):
        payload = '{"state":{"bri":7}}'

        exact_parser = WledFrameParser(capacity=len(payload) + 1)
        states = exact_parser.feed(payload + "\r\n", 0)
        self.assertEqual([state.brightness for state in states], [7])
        self.assertEqual(exact_parser.oversized_frames, 0)

        short_parser = WledFrameParser(capacity=len(payload))
        short_parser.feed(payload + ("\\" * 100) + "\r\n", 0)
        self.assertEqual(short_parser.oversized_frames, 1)
        states = short_parser.feed('{"state":{}}\r\n', 1)
        self.assertEqual(len(states), 1)

    def test_json_errors_exclude_semantically_invalid_wled_states(self):
        parser = WledFrameParser()
        malformed_frames = (
            '{"state":{"bri":invalid}}',
            '{"state":{"bri":1,}}',
            '{"state":{"bri":"unterminated}}',
        )
        for index, frame in enumerate(malformed_frames):
            parser.feed(frame + "\r\n", index)
        self.assertEqual(parser.json_errors, 2)

        parser.feed('{"state":{"bri":256}}\r\n', 10)
        parser.feed('{"not_state":true}\r\n', 11)
        self.assertEqual(parser.json_errors, 2)

    def test_repeated_uart_overruns_require_and_support_resynchronization(self):
        parser = WledFrameParser()
        for iteration in range(100):
            parser.feed('{"state":{"bri":', iteration, overrun=True)
            self.assertEqual(parser.feed('discarded}\r\n', iteration), [])
            states = parser.feed(
                f'{{"state":{{"bri":{iteration % 256}}}}}\r\n',
                iteration,
            )
            self.assertEqual(states[0].brightness, iteration % 256)
        self.assertEqual(parser.uart_overruns, 100)


if __name__ == "__main__":
    unittest.main()
