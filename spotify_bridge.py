import spotipy
from spotipy.oauth2 import SpotifyOAuth
import serial, time

COM_PORT = '/dev/ttyUSB0'
sp = spotipy.Spotify(auth_manager=SpotifyOAuth(
    client_id="YOUR KEYS HERE",
    client_secret="YOUR KEYS HERE",
    redirect_uri="http://127.0.0.1:8888/callback",
    scope="user-modify-playback-state user-read-currently-playing user-read-playback-state"
))

try:
    arduino = serial.Serial(COM_PORT, 115200, timeout=1)
    time.sleep(2)
    print("Bridge Online. Waiting for Spotify data...")
except Exception as e:
    print(f"Serial Error: {e}")
    exit()

while True:
    try:
        pb = sp.current_playback()
        if pb and pb.get('item'):
            track = pb['item'].get('name', 'Unknown')
            artist = pb['item']['artists'][0].get('name', 'Unknown')

            # THE FIX: Native, crash-proof character filter (No external libraries)
            raw_str = f"{track} - {artist}"
            clean_str = "".join([c for c in raw_str if 31 < ord(c) < 127])
            clean_str = clean_str.replace(",", "").strip()

            p_ms = pb.get('progress_ms', 0)
            d_ms = pb['item'].get('duration_ms', 1000) # Default to 1000 to prevent 0
            is_playing = 1 if pb.get('is_playing') else 0

            payload = f"D:{p_ms},{d_ms},50,{is_playing},{clean_str}\n"

            arduino.write(payload.encode('utf-8'))
            print(f"Sent: {payload.strip()}")

        if arduino.in_waiting > 0:
            cmd = arduino.readline().decode('utf-8', errors='ignore').strip()
            if cmd == "NEXT": sp.next_track()
            elif cmd == "PREV": sp.previous_track()
            elif cmd == "TOGGLE":
                curr = sp.current_playback()
                if curr and curr.get('is_playing'): sp.pause_playback()
                else: sp.start_playback()

    except Exception as e:
        print(f"Loop Error: {e}")

    time.sleep(0.5)
