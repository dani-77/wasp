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
# Widgets, left to right: CPU, RAM, volume, battery, date/time. Battery
# path is detected, not hardcoded -- BAT0 on some laptops, BAT1 on others
# (this one), so the first /sys/class/power_supply/BAT* entry found wins,
# same pattern used in Daniel's other WMs (pwm's battery_file_search(),
# spitfire's read_battery_status()).
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

while :; do
	printf '%s\n' "$(cpu) | $(ram) | $(volume) | $(battery) | $(clock)"
	sleep 5
done
