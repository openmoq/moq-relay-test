import base64
import contextlib
import io
import logging
import socket
import tempfile
import unittest
from pathlib import Path

from aiomoqt.client import MOQTClient
from aiomoqt.messages import RequestOk
from aiomoqt.server import MOQTServer
from aiomoqt.types import MOQTException, MOQTMessageType, MOQTRequestError, ParamType, RequestErrorCode
from aiomoqt.utils.certs import generate_self_signed

import auth_token_tester as tester


TOKEN = b"test credential with spaces; $(not-a-command)"


def config(**overrides):
    return {
        "relay_url": "https://localhost:4433/moq?test=1",
        "transport": "quic", "draft_version": "16", "namespace": "live/event",
        "track_name": "", "auth_token": "base64:" + base64.b64encode(TOKEN).decode(),
        **overrides,
    }


class FakeClient:
    calls = []
    errors = {}
    def __init__(self, *args, **kwargs):
        self.negotiated_draft = kwargs["supported_drafts"]
        self.calls.append(("connect", args, kwargs))

    @contextlib.asynccontextmanager
    async def connect(self):
        yield self

    async def client_session_init(self, **kwargs):
        self.calls.append(("client_setup", kwargs))
        if "client_setup" in self.errors:
            raise self.errors["client_setup"]
        return True

    def __getattr__(self, method):
        async def send(*args, **kwargs):
            self.calls.append((method, args, kwargs))
            if method in self.errors:
                raise self.errors[method]
            return RequestOk(request_id=0)
        return send


class TesterTests(unittest.IsolatedAsyncioTestCase):
    def setUp(self):
        FakeClient.calls = []
        FakeClient.errors = {}

    async def run_quietly(self, cfg=None, factory=FakeClient):
        with contextlib.redirect_stdout(io.StringIO()) as output:
            report = await tester.run(cfg or config(), factory)
        self.assertNotIn(TOKEN.decode(), output.getvalue())
        self.assertNotIn(config()["auth_token"], output.getvalue())
        return report

    def test_decoding_and_validation(self):
        self.assertEqual(tester.decode_token(config()["auth_token"]), TOKEN)
        self.assertEqual(tester.decode_token("hex:" + TOKEN.hex()), TOKEN)
        self.assertEqual(tester.decode_token("plain token"), b"plain token")
        self.assertEqual(tester.decode_token("base64:-_8"), b"\xfb\xff")
        for value in ("", "base64:", "base64:!@#", "hex:nope"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                tester.decode_token(value)
        for overrides in (
            {"relay_url": "not a URL"}, {"relay_url": "https://user:pass@localhost"},
            {"relay_url": "https://localhost:bad"}, {"transport": "both"},
            {"draft_version": "999"}, {"namespace": "/"}, {"auth_token": ""},
            {"token_type": -1}, {"token_type": 1.5}, {"token_type": True},
            {"token_type": "16"}, {"token_type": 2**53},
        ):
            with self.subTest(overrides=overrides), self.assertRaises(ValueError):
                tester.validate_config(config(**overrides))

    async def test_exact_methods_token_placement_and_optional_track(self):
        report = await self.run_quietly()
        self.assertEqual(tuple(report["tests"]), tester.METHODS)
        self.assertTrue(all(result["allowed"] for result in report["tests"].values()))
        self.assertTrue(report["track"].startswith("auth-token-test-"))
        self.assertEqual(report["namespace"], "live/event")
        self.assertEqual(len([c for c in FakeClient.calls if c[0] == "connect"]), 7)
        for call in FakeClient.calls:
            if call[0] == "connect":
                self.assertEqual(call[2]["path"], "/moq?test=1")
                self.assertTrue(call[2]["use_quic"])
            elif call[0] == "client_setup":
                self.assertEqual(call[1]["parameters"][ParamType.AUTH_TOKEN], TOKEN)
                self.assertEqual(call[1]["parameters"][ParamType.AUTH_TOKEN].token_type, 0)
            else:
                self.assertEqual(call[2]["parameters"][ParamType.AUTH_TOKEN], TOKEN)
                self.assertEqual(call[2]["parameters"][ParamType.AUTH_TOKEN].token_type, 0)
                self.assertTrue(call[2]["wait_response"])
                if call[0] not in ("publish_namespace", "subscribe_namespace"):
                    self.assertEqual(call[1][1], report["track"])
        second = await self.run_quietly(config(track_name="video", transport="wt"))
        self.assertEqual(second["track"], "video")
        self.assertTrue(second["url"].startswith("https://"))

    async def test_configured_type_is_carried_on_setup_and_all_requests(self):
        report = await self.run_quietly(config(token_type=16))
        self.assertEqual(report["token_type"], 16)
        for call in FakeClient.calls:
            if call[0] != "connect":
                kwargs = call[1] if call[0] == "client_setup" else call[2]
                self.assertEqual(kwargs["parameters"][ParamType.AUTH_TOKEN].token_type, 16)

    async def test_errors_are_never_inferred_as_allowed_and_do_not_block_other_methods(self):
        FakeClient.errors = {
            "publish_namespace": MOQTRequestError(1, "Unauthorized " + TOKEN.decode()),
            "subscribe_namespace": MOQTRequestError(3, "Not supported"),
            "subscribe": MOQTRequestError(16, "Does not exist"),
            "fetch": TimeoutError(),
            "track_status": MOQTException(2, "Unauthorized"),
        }
        report = await self.run_quietly()
        for method in FakeClient.errors:
            self.assertFalse(report["tests"][method]["allowed"])
        self.assertTrue(report["tests"]["publish"]["allowed"])
        self.assertIn("[redacted]", report["tests"]["publish_namespace"]["detail"])
        self.assertEqual(report["tests"]["subscribe"]["error_code"], 16)

    async def test_setup_failure_marks_all_rows_blocked(self):
        FakeClient.errors = {"client_setup": ConnectionError("unreachable")}
        report = await self.run_quietly()
        self.assertEqual(len(report["tests"]), 7)
        self.assertTrue(all(not r["allowed"] for r in report["tests"].values()))
        self.assertEqual(len([c for c in FakeClient.calls if c[0] == "connect"]), 1)
        self.assertIn("Not attempted", report["tests"]["fetch"]["detail"])

    async def test_missing_track_authorization_is_separate_from_request_success(self):
        for draft in (14, 16, 18, 20, 22):
            for method in ("subscribe", "fetch", "track_status"):
                with self.subTest(draft=draft, method=method):
                    code = 4 if draft == 14 else 16
                    FakeClient.errors = {
                        method: MOQTRequestError(code, "Not found", response=object()),
                    }
                    report = await self.run_quietly(config(draft_version=str(draft)))
                    result = report["tests"][method]
                    self.assertFalse(result["allowed"])
                    self.assertEqual(result["authorization"], "allowed_inferred")
                    self.assertIn("does not prove token validity", result["detail"])

    def test_only_specific_wire_errors_support_authorization_inference(self):
        for draft in (14, 16, 18, 20, 22):
            for code in (0, 1, 2, 3, 4, 5, 16, 18):
                for method in tester.METHODS:
                    error = MOQTRequestError(code, "track not found", response=object())
                    actual = tester.request_authorization(method, draft, error)
                    auth_codes = (1, 16, 18) if draft == 14 else (1, 4, 5)
                    expected = (
                        "denied" if code in auth_codes else
                        "allowed_inferred" if method in ("subscribe", "fetch", "track_status")
                        and code == (4 if draft == 14 else 16) else "unknown"
                    )
                    self.assertEqual(actual, expected, (method, draft, code))
            self.assertEqual(tester.request_authorization(
                "subscribe", draft, MOQTRequestError(4 if draft == 14 else 16, "Not found"),
            ), "unknown")

    async def test_per_request_setup_failure_is_attributed_to_setup(self):
        class LaterFailure(FakeClient):
            async def client_session_init(self, **kwargs):
                if len([c for c in self.calls if c[0] == "connect"]) > 1:
                    raise MOQTException(2, "Setup refused")
                return True

        report = await self.run_quietly(factory=LaterFailure)
        self.assertTrue(report["tests"]["client_setup"]["allowed"])
        for method in tester.METHODS[1:]:
            self.assertFalse(report["tests"][method]["allowed"])
            self.assertEqual(report["tests"][method]["stage"], "client_setup")


class WireTests(unittest.IsolatedAsyncioTestCase):
    """Exercise real TLS, transports, token codecs and response routing."""

    async def test_all_drafts_and_transports(self):
        logging.disable(logging.CRITICAL)
        with tempfile.TemporaryDirectory() as directory:
            cert_path = str(Path(directory) / "cert.pem")
            key_path = str(Path(directory) / "key.pem")
            generate_self_signed(cert_path, key_path)
            for draft in (14, 16, 18, 20, 22):
                for transport in ("quic", "wt"):
                    with self.subTest(draft=draft, transport=transport):
                        seen = []
                        denied = False
                        missing = False

                        async def accept(session, message):
                            seen.append(message)
                            self.assertEqual(message.parameters[ParamType.AUTH_TOKEN], TOKEN)
                            self.assertEqual(message.parameters[ParamType.AUTH_TOKEN].token_type, 16)
                            if missing and message.type in (
                                MOQTMessageType.SUBSCRIBE, MOQTMessageType.FETCH,
                                MOQTMessageType.TRACK_STATUS,
                            ):
                                session._refuse_peer_request(
                                    message, RequestErrorCode.DOES_NOT_EXIST, "Track not found",
                                )
                                return
                            if denied:
                                session._refuse_peer_request(
                                    message, RequestErrorCode.UNAUTHORIZED,
                                    "Unauthorized " + TOKEN.decode(),
                                )
                                return
                            if message.type == MOQTMessageType.FETCH:
                                session.fetch_ok(message.request_id, largest_object_id=1)
                            elif message.type == MOQTMessageType.TRACK_STATUS:
                                if draft == 14:
                                    session._d14_track_status_ok(message.request_id, None)
                                else:
                                    session._send_reply(
                                        message.request_id, RequestOk(message.request_id), fin=True,
                                    )
                            else:
                                method = {
                                    MOQTMessageType.PUBLISH_NAMESPACE: "publish_namespace_ok",
                                    MOQTMessageType.SUBSCRIBE_NAMESPACE: "subscribe_namespace_ok",
                                    MOQTMessageType.SUBSCRIBE: "subscribe_ok",
                                    MOQTMessageType.PUBLISH: "publish_ok",
                                }[message.type]
                                getattr(session, method)(message)

                        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as reserved:
                            reserved.bind(("127.0.0.1", 0))
                            port = reserved.getsockname()[1]
                        server = MOQTServer(
                            host="127.0.0.1", port=port, certificate=cert_path,
                            private_key=key_path, path="/moq",
                            use_quic=transport == "quic", supported_drafts=draft,
                        )
                        for method in tester.METHODS[1:]:
                            server.register_handler(getattr(MOQTMessageType, method.upper()), accept)
                        listener = await server.serve()
                        try:
                            def client_factory(*args, **kwargs):
                                return MOQTClient(*args, ca_file=cert_path, **kwargs)

                            cfg = config(
                                relay_url=f"https://localhost:{port}/moq",
                                draft_version=str(draft), transport=transport, track_name="video",
                                token_type=16,
                            )
                            with contextlib.redirect_stdout(io.StringIO()):
                                report = await tester.run(cfg, client_factory)
                            self.assertEqual(
                                [m for m, r in report["tests"].items() if not r["allowed"]],
                                [], report,
                            )
                            self.assertEqual(len(seen), 6)
                            missing = True
                            with contextlib.redirect_stdout(io.StringIO()):
                                not_found = await tester.run(cfg, client_factory)
                            for method in ("subscribe", "fetch", "track_status"):
                                self.assertFalse(not_found["tests"][method]["allowed"])
                                self.assertEqual(
                                    not_found["tests"][method]["authorization"], "allowed_inferred",
                                    not_found["tests"][method],
                                )
                            missing = False
                            denied = True
                            with contextlib.redirect_stdout(io.StringIO()) as output:
                                refused = await tester.run(cfg, client_factory)
                            self.assertTrue(refused["tests"]["client_setup"]["allowed"])
                            for method in tester.METHODS[1:]:
                                self.assertFalse(refused["tests"][method]["allowed"], refused)
                                self.assertEqual(refused["tests"][method]["error_code"], 1)
                                self.assertEqual(refused["tests"][method]["authorization"], "denied")
                                self.assertIn("[redacted]", refused["tests"][method]["detail"])
                            self.assertNotIn(TOKEN.decode(), output.getvalue())
                        finally:
                            await server.close_sessions()
                            listener.close()


if __name__ == "__main__":
    unittest.main()
