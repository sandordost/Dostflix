#!/bin/bash
set -euo pipefail

project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
helper=${1:-"$project_root/build/dostflix-network-helper"}
tag=${BASHPID}
suffix=${tag: -5}
namespace="dfx-${tag}"
clear_host="dch${suffix}"
clear_ns="dcn${suffix}"
vpn_host="dvh${suffix}"
scope_id=$(printf '%032x' "$tag")
scope_name="dostflix-${scope_id}.scope"
server_pids=()

cleanup() {
  for server_pid in "${server_pids[@]:-}"; do
    kill "$server_pid" 2>/dev/null || true
  done
  sudo ip netns delete "$namespace" 2>/dev/null || true
  sudo ip link delete "$clear_host" 2>/dev/null || true
  sudo ip link delete "$vpn_host" 2>/dev/null || true
}
trap cleanup EXIT

sudo ip netns add "$namespace"
sudo ip link add "$clear_host" type veth peer name "$clear_ns"
sudo ip link set "$clear_ns" netns "$namespace"
sudo ip address add 192.0.2.1/24 dev "$clear_host"
sudo ip link set "$clear_host" up
sudo ip netns exec "$namespace" ip address add 192.0.2.2/24 dev "$clear_ns"
sudo ip netns exec "$namespace" ip link set "$clear_ns" up

sudo ip link add "$vpn_host" type veth peer name dvpn
sudo ip link set dvpn netns "$namespace"
sudo ip address add 198.51.100.1/24 dev "$vpn_host"
sudo ip link set "$vpn_host" up
sudo ip netns exec "$namespace" ip address add 198.51.100.2/24 dev dvpn
sudo ip netns exec "$namespace" ip link set dvpn up
sudo ip netns exec "$namespace" ip link set lo up

socat TCP-LISTEN:18080,bind=192.0.2.1,reuseaddr,fork EXEC:/bin/cat &
server_pids+=("$!")
socat TCP-LISTEN:18081,bind=198.51.100.1,reuseaddr,fork EXEC:/bin/cat &
server_pids+=("$!")
socat TCP-LISTEN:18082,bind=198.51.100.1,reuseaddr,fork EXEC:/bin/cat &
server_pids+=("$!")

servers_ready=false
for attempt in {1..20}; do
  if sudo nsenter --net="/run/netns/$namespace" timeout 1 \
       bash -c 'exec 3<>/dev/tcp/192.0.2.1/18080' 2>/dev/null \
     && sudo nsenter --net="/run/netns/$namespace" timeout 1 \
       bash -c 'exec 3<>/dev/tcp/198.51.100.1/18081' 2>/dev/null; then
    servers_ready=true
    break
  fi
  read -r -t 0.1 _ || true
done
if [[ "$servers_ready" != true ]]; then
  echo 'virtual network test servers did not become ready' >&2
  exit 1
fi

systemd-run --user --scope --quiet --unit="$scope_name" \
  "$project_root/tests/network/isolation_client.sh" \
  "$namespace" "$scope_id" "$scope_name" "$helper"

echo 'Dostflix network isolation test passed'
