#!/bin/bash
#
# Because the load simular is not multithreaded, we need a way to start many. This script facilitates that.

binary="./MqttLoadSimulator"

processes=1
amount_active=1
amount_passive=1
target_host="localhost"
delay=0
burst_interval=3000
msg_per_burst=25

while [ -n "$*" ]; do
  flag=$1
  value=$2

  case "$flag" in
    "--processes")
      processes=$value
      shift
    ;;
    "--amount-active")
      amount_active=$value
      shift
    ;;
    "--amount-passive")
      amount_passive=$value
      shift
    ;;
    "--hostname")
      target_host=$value
      shift
    ;;
    "--delay")
      delay=$value
      shift
    ;;
    "--burst-interval")
      burst_interval=$value
      shift
    ;;
    "--msg-per-burst")
      msg_per_burst=$value
      shift
    ;;
    "--help")
      dohelp
    ;;
    "--")
      break
    ;;
    *)
      echo "unknown option $flag. Type --help"
      exit 66
    ;;
  esac

  shift
done

set -u

one_load_simulator()
{
  loadsimulatorpid=""

  being_killed()
  {
    [ -n "$loadsimulatorpid" ] && kill -SIGTERM "$loadsimulatorpid" 2> /dev/null
  }

  trap "being_killed" SIGINT SIGTERM

  "$binary" --burst-interval "$burst_interval" --msg-per-burst "$msg_per_burst" --hostname "$target_host" --amount-active "$amount_active" --amount-passive "$amount_passive" --delay "$delay" &> /dev/null & loadsimulatorpid=$!
  wait $loadsimulatorpid
  exit_status=$?
}

kill_all_children()
{
  for p in "${pids[@]}"; do
    kill -SIGTERM "$p" &> /dev/null
  done
}

declare -a pids

trap kill_all_children SIGINT SIGTERM

echo "Starting $processes load simulators, each with $amount_active active clients and $amount_passive passive clients, with $delay ms wait time between each connection."
echo "Total active clients: $(( processes * amount_active )). Total Passive clients: $(( processes * amount_passive ))."
echo "Each client bursting $msg_per_burst messages per $burst_interval ms (+/- random spread)."

for instancenr in $(seq 1 "$processes"); do
  one_load_simulator & pids+=($!)
done

wait
