# Auth Token Tester

Appears directly below Auth Token Generator in the Authentication tools.
Uses the same `ghcr.io/gmarzot/aiomoqt:0.14.1` image as Relay Probe.

## Inputs

- Relay URL, Transport (QUIC or WebTransport/H3), Draft Version (14/16/18/20/22).
- Namespace: slash-separated components, with `\/` for a literal slash.
- Optional Track Name: defaults to a unique `auth-token-test-<uuid>` name.
- Token: `base64:` output from Auth Token Generator, `hex:` bytes, or plain
  UTF-8. Unprefixed input is literal text, not implicitly base64-decoded.
- Token Type: must match the relay's `auth.token_type` (default 0, preserving
  the original behavior). For a relay configured with `token_type: 16`, enter
  **16**. This is separate from the protocol draft number and the `base64:`
  encoding prefix. The generator produces token value bytes, not this wire
  type identifier; aiomoqt wraps the value with the selected type for SETUP
  and every request.

## Checks and interpretation

The table contains `client_setup`, `publish_namespace`, `subscribe_namespace`,
`subscribe`, `publish`, `fetch`, and `track_status`. Each request gets an
independent connection, with the supplied token on both SETUP and the request.
No namespace announcement is required as a prerequisite to track requests:
this avoids confusing permission for one action with permission for another.
If the initial setup fails, all remaining rows say Not allowed and explain
that they were not attempted.

The table separates **Authorization** from **Request result**:

- Allowed / Accepted: the relay acknowledged the request.
- Allowed (inferred) / Namespace/track not found: subscribe, fetch, or track_status received a
  missing-resource error (draft 14 code `0x04`, drafts 16+ code `0x10`).
  moqx authorizes these operations before downstream lookup, so this indicates
  authorization passed but the requested resource is absent. This is an inference,
  not a protocol guarantee for other relays, proof of token validity, or delivery.
- Denied / Not accepted: an explicit request authorization error was received.
- Unknown / Not accepted: timeout, unsupported request, setup failure, or another
  error that cannot establish authorization. Reason text alone is never used
  to infer authorization.

The JSON `allowed` flag still means request success only; the separate
`authorization` field is `allowed`, `allowed_inferred`, `denied`, or `unknown`.
Allowed and Allowed (inferred) are shown in green. The summary counts both:
`N/7 requests authorizations allowed (or inferred)`.
Other methods do not infer authorization from missing resources.
Generated track names may fall outside a track-scoped token's grants.

With `require_setup_token: false`, a successful setup alone does not establish
token validity. Likewise, requests under an anonymous namespace grant may
succeed without proving the token is valid. Test a namespace outside anonymous
grants, matching the generated token's namespace/track and actions. Use the same
key ID, secret, and service configured on the relay when generating the token.

moqx ordering reference:
[AuthPublisherFilter subscribe/fetch/trackStatus](https://github.com/openmoq/moqx/blob/main/src/relay/AuthFilters.cpp).

These are real protocol requests: publish_namespace and publish temporarily
announce the supplied namespace/track. No objects are published. Connections
are closed after each check; relay-side cleanup follows the relay's session
handling. Use a test namespace/track when possible. TLS certificates are verified.
Each connection/check has a 12-second deadline.

For native QUIC, SETUP explicitly carries the URL's authority and path/query.
An omitted port stays omitted in AUTHORITY; an explicit port is retained.
This avoids aiomoqt's default `host:port` authority changing exact service
matches (for example `relay.example` versus `relay.example:443`). PATH keeps
its leading and trailing slashes. WebTransport routes through its HTTP CONNECT
request and must not carry these options in MoQT SETUP. The report displays the
requested authority and path; it cannot confirm the relay's selected service.

Results stream as JSON snapshots to the table and raw output. Exit code 0
means the diagnostic finished, not that every method was allowed.
Invalid input or an unexpected runner failure exits nonzero.
The token is passed in a silently mounted JSON file rather than CLI arguments,
library message logging is disabled, peer error details are redacted, and
history persistence is disabled. Parameters/results remain in the active
browser/server session until it ends.

## Research references

- [aiomoqt client/transport API](https://github.com/gmarzot/aiomoqt/blob/main/aiomoqt/client.py)
- [Probe connection and setup reference](https://github.com/gmarzot/aiomoqt/blob/main/aiomoqt/tools/relay_probe.py)
- [Request methods and response handling](https://github.com/gmarzot/aiomoqt/blob/main/aiomoqt/protocol.py)
- [Token wrapping and types](https://github.com/gmarzot/aiomoqt/blob/main/aiomoqt/messages/base.py)
- [Token authorization tests](https://github.com/gmarzot/aiomoqt/blob/main/aiomoqt/tests/test_auth_token_cache.py)

The APIs were also inspected in the pinned container, rather than assuming
upstream main and the deployed image were identical. The older experimental
`tools-inprogress/auth-test` informed token input handling, but is not reused:
it tests different scenarios and infers acceptance from some error strings.

## Tests

From `moq-test-ui`:

```sh
node --test tools/auth-token-tester/test-integration.js
docker run --rm --entrypoint python \
  -v "$PWD/tools/auth-token-tester:/tests:ro" \
  ghcr.io/gmarzot/aiomoqt:0.14.1 \
  -m unittest discover -s /tests -p 'test_*.py'
```
