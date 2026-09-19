# Raspad master

GoldSrc / CS 1.6 master stub. The client queries **UDP 27010**; the process
answers from an in-memory list loaded from `servers.json`. There is no HTTP
and no heartbeat.

Built on [lh](https://github.com/darwin128k/lh) (submodule `lib/lh`) and
vendored [cJSON](https://github.com/DaveGamble/cJSON) for the list file only.

## Layers

| Piece | Role |
|---|---|
| `query` | Binary `0x31` / `FF FF FF FF 66 0A` codec. No sockets. |
| `registry` | Vector of `IP:port`. No JSON. |
| `list_file` | Read `servers.json` into the registry. |
| `flood` | Per-IP hit window. |
| `udp` | One datagram → optional reply via `lh_io_dgram_t`. |
| `master` | Bind, `select`, reload the JSON when its mtime changes. |

lh supplies sockets, datagrams, clock, logger, vector. GoldSrc framing and
the list file stay in raspad.

## `servers.json`

Copied next to `raspad-master` at build time. Edit **that** copy (the one
beside the binary), save; the master reloads without a restart.

```json
{
  "servers": [
    { "ip": "37.230.210.218", "port": 27015 }
  ]
}
```

A top-level array of the same objects is also accepted. A parse error keeps
the last good list. The CS client never reads this file; it only speaks UDP
27010.

## Build

CMake 3.12, C11. After clone:

```sh
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target raspad-master
```

Tests (GoogleTest from lh):

```sh
cmake --build build --target raspad_master_test
```

## Run

```sh
./build/raspad-master
```

Listens on `0.0.0.0:27010` UDP. Host firewall must allow that port (on Debian,
`ufw allow 27010/udp`). Log line: `udp 27010 list servers.json`.

## Client

Raspad `steamclient` queries this master (baked `host:27010`). Internet tab
then A2S-probes each returned `IP:port`.
