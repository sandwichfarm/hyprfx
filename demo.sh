#!/bin/bash
# BMW Effects Demo — cycles through fire, tv, and pixelate effects
# Run inside nested Hyprland: Hyprland -c test.conf

DELAY_BEFORE=1.5   # pause before spawning
DELAY_SHOW=2.0     # how long to show the window
DELAY_AFTER=2.0    # pause after close effect finishes

effects=("fire" "tv" "pixelate")

sleep 2  # let the compositor settle

for effect in "${effects[@]}"; do
    # Set both open and close to the current effect
    hyprctl keyword plugin:bmw:open_effect "$effect"
    hyprctl keyword plugin:bmw:close_effect "$effect"

    sleep "$DELAY_BEFORE"

    # Spawn a kitty window (open effect plays)
    kitty --title "BMW Demo: $effect" -e sleep 999 &
    KPID=$!

    sleep "$DELAY_SHOW"

    # Kill it (close effect plays)
    kill $KPID 2>/dev/null
    wait $KPID 2>/dev/null

    sleep "$DELAY_AFTER"
done

# Show a final message
hyprctl keyword plugin:bmw:open_effect fire
kitty --title "Demo Complete" -e sh -c 'echo "All effects demonstrated!"; echo "Press Enter to exit."; read' &
