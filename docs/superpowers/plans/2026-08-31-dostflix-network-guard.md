# Dostflix Network Guard Plan

**Date:** 2026-08-31

**Goal:** Make it technically impossible for Dostflix-owned external sockets to use the clear interface before VPN validation, during tunnel loss, or during shutdown.

## Security boundary

Dostflix runs inside a dedicated transient systemd user scope. The firewall matches that cgroup-v2 scope, not the desktop user's UID, so browsers and other applications remain unaffected. A root-owned D-Bus service installs and removes only tables whose names and ownership records it created. Polkit authorizes every state-changing request from the desktop session.

The service never accepts arbitrary nftables text. Its narrow typed request contains:

- an unguessable session identifier;
- the verified caller PID and cgroup-v2 scope;
- one numeric VPN endpoint address, transport protocol, and port;
- the eventual tunnel interface name;
- the requested transition: bootstrap, protected, or remove.

Every string, number, process owner, cgroup path, and interface is validated before a fixed ruleset is generated.

## Rule states

### Bootstrap

For sockets in the Dostflix scope, permit only loopback and the exact numeric VPN endpoint/transport/port. Reject every other IPv4 and IPv6 destination. DNS resolution happens before this state only for the VPN hostname and its short-lived result becomes the endpoint allow-list.

### Protected

Permit loopback, the exact VPN transport endpoint, and traffic leaving through the verified VPN interface. Reject every other packet from the Dostflix scope. Provider and torrent networking remains disabled until this ruleset, NetworkManager state, routes, and DNS routing all pass verification.

### Removed

Delete only the session table recorded by the service. Removal is allowed after protected networking has stopped. Stale Dostflix-owned tables are audited at the next launch.

## Implementation order

1. Implement pure request validation and deterministic nftables ruleset generation with IPv4/IPv6 tests.
2. Add the root D-Bus service and Polkit action; resolve and verify the caller through the bus rather than trusting command-line identity fields.
3. Add the systemd user-scope launcher and validate its exact cgroup path.
4. Add endpoint parsing/resolution, guard transitions, route checks, and `networkReady` gating.
5. Add Linux network-namespace tests proving clear-interface packets fail in all lifecycle states.
6. Package the service, D-Bus policy, Polkit action, launcher, and tests.

## Fail-closed behavior

- Invalid or ambiguous input installs nothing and leaves remote features disabled.
- A helper crash leaves the kernel rules in place.
- VPN loss immediately clears `networkReady`; the protected rules remain installed.
- Failed rule removal or VPN shutdown is surfaced and recorded for startup audit.
- No provider, metadata, subtitle, tracker, DHT, peer, or seeding socket is enabled by this increment until isolation tests pass.
