#!/bin/sh
# wasp — reference status-bar text generator.
#
# wasp's bar (dwl-patches' bar/barconfig) shows whatever's piped into its
# own stdin, one line per update, dwl-classic style -- it has no built-in
# status widgets of its own. Without something feeding it, the bar just
# shows its startup placeholder text ("dwl-<version>-dirty") forever. Run
# this piped into wasp to get it showing something real:
#
#   ./wasp <&- | : # (not this -- see below)
#   scripts/statusbar.sh | wasp
#
# or, once autostart lands in config.lua (see NOTES.md), spawn it from
# there instead of by hand.
#
# Widgets, left to right: CPU, RAM, volume, network, battery, date/time.
# Battery path is detected, not hardcoded -- BAT0 on some laptops, BAT1 on
# others (this one), so the first /sys/class/power_supply/BAT* entry found
# wins, same pattern used in Daniel's other WMs (pwm's
# battery_file_search(), spitfire's read_battery_status()). Network is the
# same idea: the interface is *found* (whatever the default route is
# actually using -- wlan0/wlp3s0/enp2s0/... all differ by machine and
# driver), never assumed by name.
#
# Feel free to treat this as a starting point rather than gospel -- swap
# widgets, reorder them, change the separator, whatever. It's just a shell
# loop writing lines to stdout.

battery_path() {
	for bat in /sys/class/power_supply/BAT*; do
		[ -d "$bat" ] && { printf '%s\n' "$bat"; return 0; }
	done
	return 1
}

BAT="$(battery_path)"

cpu() {
	# %idle from top's summary line, inverted -- avoids /proc/stat's usual
	# two-samples-a-second-apart dance for a "close enough, every few
	# seconds" bar reading. Forced to the C locale: top prints the decimal
	# point as "," under pt_PT etc., which awk won't parse as a number.
	idle=$(LC_ALL=C top -bn1 | awk '/^%?Cpu/ {for (i=1;i<=NF;i++) if ($i ~ /id,?$/) {print $(i-1); exit}}')
	[ -n "$idle" ] && printf 'CPU %d%%' "$(awk -v i="$idle" 'BEGIN{printf "%d", 100-i}')"
}

ram() {
	free -m | awk '/^Mem:/ {printf "RAM %d/%dM", $3, $2}'
}

volume() {
	pactl get-sink-volume @DEFAULT_SINK@ 2>/dev/null \
		| awk '{for (i=1;i<=NF;i++) if ($i ~ /%$/) {print $i; exit}}' \
		| awk '{printf "Vol %s", $1}'
}

network() {
	# Whichever interface the default route is actually on -- wired,
	# wireless, or a VPN tunnel, no name assumed. Needs `ip` (iproute2);
	# silently shows nothing if it's missing, same as the other widgets
	# do when their own tool/file isn't there.
	iface=$(ip route show default 2>/dev/null | awk '/^default/ {print $5; exit}')
	[ -n "$iface" ] || { printf 'Net down'; return 0; }

	# The kernel only creates this directory for actual wireless
	# interfaces -- distinguishes wired/wireless without parsing the
	# interface's own name (wlp0s20f3 vs enp2s0 vs eth0, ...).
	if [ -d "/sys/class/net/$iface/wireless" ]; then
		# Link quality is kernel-reported (no extra tool needed), out of
		# a conventional max of 70 -- some drivers don't report it, hence
		# the fallback below. SSID needs `iw` (optional -- degrades to
		# just the quality, or to a bare "Net wifi", if it's not
		# installed or the driver doesn't say).
		quality=$(awk -v i="$iface:" '$1==i {printf "%d", ($3+0)/70*100}' /proc/net/wireless)
		ssid=""
		command -v iw >/dev/null 2>&1 && ssid=$(iw dev "$iface" link 2>/dev/null \
			| awk -F': ' '/^\tSSID/ {print $2; exit}')
		if [ -n "$ssid" ] && [ -n "$quality" ]; then
			printf 'Net %s %s%%' "$ssid" "$quality"
		elif [ -n "$ssid" ]; then
			printf 'Net %s' "$ssid"
		elif [ -n "$quality" ]; then
			printf 'Net wifi %s%%' "$quality"
		else
			printf 'Net wifi'
		fi
	else
		printf 'Net wired'
	fi
}

battery() {
	[ -n "$BAT" ] || return 0
	cap=$(cat "$BAT/capacity" 2>/dev/null)
	status=$(cat "$BAT/status" 2>/dev/null)
	case "$status" in
		Charging) mark='+' ;;
		Discharging) mark='-' ;;
		*) mark='=' ;;
	esac
	[ -n "$cap" ] && printf 'Bat %s%d%%' "$mark" "$cap"
}

clock() {
	date '+%a %d %b %H:%M'
}

# Piped into wasp (directly, or via scripts/wasp-session): once wasp exits,
# this side of the pipe is still alive and blocked in `sleep`, so it won't
# notice for up to a whole sleep interval -- its next printf then fails
# loudly (SIGPIPE/EPIPE, "broken pipe" from the shell's printf builtin)
# before it finally exits, both delaying session teardown and spamming an
# alarming-looking (but harmless) error. Trapping TERM/INT/PIPE and
# backgrounding the sleep (so a signal interrupts `wait` immediately
# instead of only being noticed once the sleep completes on its own) fixes
# both: exits right away, silently, whichever of the two ways it finds out
# (a session-teardown signal, or the pipe actually breaking).
trap 'exit 0' TERM INT PIPE

while :; do
	printf '%s\n' "$(cpu) | $(ram) | $(volume) | $(network) | $(battery) | $(clock)" || exit 0
	sleep 5 &
	wait $! 2>/dev/null
done
