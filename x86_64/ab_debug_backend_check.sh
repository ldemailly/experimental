#!/usr/bin/env bash
set -euo pipefail

# A/B check for remote-debug prerequisites of the Linux amd64 binary on Apple Silicon.
# It tests runtime and gdbserver startup behavior with Docker and Apple container.

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
IMAGE_DOCKER="experimental-x86_64-linux-build:latest"
IMAGE_APPLE="local/experimental-x86_64-linux-build:latest"

PASS_DOCKER_RUN="not-run"
PASS_DOCKER_GDB="not-run"
PASS_APPLE_RUN="not-run"
PASS_APPLE_GDB="not-run"
DETAIL_DOCKER_RUN=""
DETAIL_DOCKER_GDB=""
DETAIL_APPLE_RUN=""
DETAIL_APPLE_GDB=""

run_and_capture() {
  local outfile="$1"
  shift
  if "$@" >"$outfile" 2>&1; then
    return 0
  fi
  return $?
}

run_with_kill_timeout() {
  local timeout_s="$1"
  local outfile="$2"
  shift 2

  "$@" >"$outfile" 2>&1 &
  local pid=$!
  local deadline=$((SECONDS + timeout_s))

  while kill -0 "$pid" 2>/dev/null; do
    if (( SECONDS >= deadline )); then
      kill "$pid" 2>/dev/null || true
      wait "$pid" 2>/dev/null || true
      return 124
    fi
  done

  wait "$pid"
}

print_status() {
  local label="$1"
  local status="$2"
  local detail="$3"
  printf "%-34s %-8s %s\n" "$label" "$status" "$detail"
}

cd "$ROOT_DIR"

echo "== Building linux amd64 artifact =="
make hello_linux

echo ""
echo "== Backend availability =="
HAS_DOCKER=0
HAS_APPLE=0
if command -v docker >/dev/null 2>&1; then
  HAS_DOCKER=1
  echo "docker: found"
else
  echo "docker: missing"
fi
if command -v container >/dev/null 2>&1; then
  HAS_APPLE=1
  echo "container: found"
else
  echo "container: missing"
fi

echo ""
echo "== Running checks =="

if (( HAS_DOCKER )); then
  make linux_image >/dev/null

  if run_and_capture /tmp/docker_run.log \
    docker run --rm --platform linux/amd64 -v "$ROOT_DIR":/workspace -w /workspace "$IMAGE_DOCKER" \
      sh -lc './hello_linux'; then
    PASS_DOCKER_RUN="PASS"
    DETAIL_DOCKER_RUN="binary ran"
  else
    PASS_DOCKER_RUN="FAIL"
    DETAIL_DOCKER_RUN="$(tail -n 2 /tmp/docker_run.log | tr '\n' ' ' | sed 's/[[:space:]]\+/ /g')"
  fi

  # Probe against the real test binary; timeout==PASS means server stayed up waiting for attach.
  if run_with_kill_timeout 5 /tmp/docker_gdb.log \
    docker run --rm --platform linux/amd64 -p 127.0.0.1:2345:2345 -v "$ROOT_DIR":/workspace -w /workspace "$IMAGE_DOCKER" \
      sh -lc 'gdbserver 0.0.0.0:2345 ./hello_linux'; then
    PASS_DOCKER_GDB="PASS"
    DETAIL_DOCKER_GDB="gdbserver exited cleanly"
  else
    rc=$?
    if [[ $rc -eq 124 ]]; then
      PASS_DOCKER_GDB="PASS"
      DETAIL_DOCKER_GDB="gdbserver stayed alive for 5s waiting for attach"
    else
      PASS_DOCKER_GDB="FAIL"
      DETAIL_DOCKER_GDB="$(tail -n 3 /tmp/docker_gdb.log | tr '\n' ' ' | sed 's/[[:space:]]\+/ /g')"
    fi
  fi
else
  PASS_DOCKER_RUN="SKIP"
  PASS_DOCKER_GDB="SKIP"
  DETAIL_DOCKER_RUN="docker not installed"
  DETAIL_DOCKER_GDB="docker not installed"
fi

if (( HAS_APPLE )); then
  if ! container system status >/dev/null 2>&1; then
    PASS_APPLE_RUN="SKIP"
    PASS_APPLE_GDB="SKIP"
    DETAIL_APPLE_RUN="container system not running; run: container system start"
    DETAIL_APPLE_GDB="container system not running; run: container system start"
  else
    container build --platform linux/amd64 -t "$IMAGE_APPLE" -f Dockerfile.linux . >/tmp/apple_build.log 2>&1

    if run_and_capture /tmp/apple_run.log \
      container run --rm --arch amd64 --volume "$ROOT_DIR":/workspace --workdir /workspace "$IMAGE_APPLE" \
        sh -lc './hello_linux'; then
      PASS_APPLE_RUN="PASS"
      DETAIL_APPLE_RUN="binary ran"
    else
      PASS_APPLE_RUN="FAIL"
      DETAIL_APPLE_RUN="$(tail -n 2 /tmp/apple_run.log | tr '\n' ' ' | sed 's/[[:space:]]\+/ /g')"
    fi

    if run_with_kill_timeout 5 /tmp/apple_gdb.log \
      container run --rm --arch amd64 --publish 127.0.0.1:2346:2345 --volume "$ROOT_DIR":/workspace --workdir /workspace "$IMAGE_APPLE" \
        sh -lc 'gdbserver 0.0.0.0:2345 ./hello_linux'; then
      PASS_APPLE_GDB="PASS"
      DETAIL_APPLE_GDB="gdbserver exited cleanly"
    else
      rc=$?
      if [[ $rc -eq 124 ]]; then
        PASS_APPLE_GDB="PASS"
        DETAIL_APPLE_GDB="gdbserver stayed alive for 5s waiting for attach"
      else
        PASS_APPLE_GDB="FAIL"
        DETAIL_APPLE_GDB="$(tail -n 3 /tmp/apple_gdb.log | tr '\n' ' ' | sed 's/[[:space:]]\+/ /g')"
      fi
    fi
  fi
else
  PASS_APPLE_RUN="SKIP"
  PASS_APPLE_GDB="SKIP"
  DETAIL_APPLE_RUN="apple container CLI not installed"
  DETAIL_APPLE_GDB="apple container CLI not installed"
fi

echo ""
echo "== Result summary =="
printf "%-34s %-8s %s\n" "CHECK" "STATUS" "DETAIL"
print_status "docker: run hello_linux" "$PASS_DOCKER_RUN" "$DETAIL_DOCKER_RUN"
print_status "docker: start gdbserver probe" "$PASS_DOCKER_GDB" "$DETAIL_DOCKER_GDB"
print_status "apple container: run hello_linux" "$PASS_APPLE_RUN" "$DETAIL_APPLE_RUN"
print_status "apple container: start gdbserver" "$PASS_APPLE_GDB" "$DETAIL_APPLE_GDB"

echo ""
echo "Logs: /tmp/docker_run.log /tmp/docker_gdb.log /tmp/apple_build.log /tmp/apple_run.log /tmp/apple_gdb.log"
