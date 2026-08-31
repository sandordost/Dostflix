#!/bin/bash
set -euo pipefail

namespace=$1
session_id=$2
scope_name=$3
helper=$4
caller_uid=$(id -u)
cgroup_path=$(sed -n 's|^0::/||p' "/proc/$$/cgroup")
if [[ ! -d "/sys/fs/cgroup/$cgroup_path" ]]; then
  echo "test cgroup is not visible on host: $cgroup_path" >&2
  exit 1
fi
if ! sudo nsenter --net="/run/netns/$namespace" test -d "/sys/fs/cgroup/$cgroup_path"; then
  echo "test cgroup is not visible inside network namespace: $cgroup_path" >&2
  exit 1
fi

guard() {
  sudo nsenter --net="/run/netns/$namespace" env PKEXEC_UID="$caller_uid" \
    "$helper" "$@"
}

connect_from_scope() {
  local address=$1 port=$2
  sudo nsenter --net="/run/netns/$namespace" setpriv --reuid="$caller_uid" --regid="$caller_uid" \
    --clear-groups timeout 2 bash -c "exec 3<>/dev/tcp/${address}/${port}"
}

guard install "$$" "$session_id" "$scope_name" 5 198.51.100.1 tcp 18081 '' bootstrap
if connect_from_scope 192.0.2.1 18080; then
  echo 'clear interface leaked during bootstrap' >&2
  exit 1
fi
connect_from_scope 198.51.100.1 18081

guard install "$$" "$session_id" "$scope_name" 5 198.51.100.1 tcp 18081 dvpn protected
if connect_from_scope 192.0.2.1 18080; then
  echo 'clear interface leaked while protected' >&2
  exit 1
fi
connect_from_scope 198.51.100.1 18082

sudo nsenter --net="/run/netns/$namespace" ip link set dvpn down
if connect_from_scope 192.0.2.1 18080; then
  echo 'clear interface leaked after VPN interface loss' >&2
  exit 1
fi
sudo nsenter --net="/run/netns/$namespace" ip link set dvpn up

guard remove "$$" "$session_id" "$scope_name"
connect_from_scope 192.0.2.1 18080
