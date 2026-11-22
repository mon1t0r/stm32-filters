import serial
import socket
import struct
import time

SERIAL_PORT = "/dev/ttyACM0"
SERIAL_BAUD_RATE = 921600
SERIAL_TIMEOUT = 1.0

UDP_IP = "127.0.0.1"
UDP_PORT = 52856

CHANNELS = 2            # stereo
BYTES_PER_SAMPLE = 2    # int16_t
FRAMES_PER_PACKET = 120

# Calculated parameters
SAMPLES_PER_PACKET = FRAMES_PER_PACKET * CHANNELS
BYTES_PER_PACKET = SAMPLES_PER_PACKET * BYTES_PER_SAMPLE

def read_exact(port, size):
    """Read exactly size bytes from serial port."""

    data = bytearray()

    while len(data) < size:
        chunk = port.read(size - len(data))

        if chunk:
            data.extend(chunk)

    return bytes(data)


def main():

    print("Opening serial port...")

    ser = serial.Serial(
        port=SERIAL_PORT,
        baudrate=SERIAL_BAUD_RATE,
        timeout=SERIAL_TIMEOUT
    )

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    print(f"Serial:       {SERIAL_PORT}")
    print(f"Baud rate:    {SERIAL_BAUD_RATE}")
    print(f"UDP:          {UDP_IP}:{UDP_PORT}")
    print(f"Channels:     {CHANNELS}")
    print(f"Frames/packet:{FRAMES_PER_PACKET}")
    print(f"UDP payload:  {SAMPLES_PER_PACKET * 4} bytes")

    print("\nStreaming... Press Ctrl+C to stop.")

    # Discard data accumulated before starting
    ser.reset_input_buffer()

    try:
        while True:

            # Read samples
            raw = read_exact(ser, BYTES_PER_PACKET)

            # Unpack int16_t, Little Endian
            samples = struct.unpack(
                f"<{SAMPLES_PER_PACKET}h",
                raw
            )

            # Convert int16_t to normalized float32_t
            normalized = [sample / 32768.0 for sample in samples]

            # Pack float32_t, Little Endian
            payload = struct.pack(
                f"<{SAMPLES_PER_PACKET}f",
                *normalized
            )

            # Send UDP packet
            sock.sendto(payload, (UDP_IP, UDP_PORT))

    except KeyboardInterrupt:
        print("\nStopped.")

    finally:
        ser.close()
        sock.close()


if __name__ == "__main__":
    main()

