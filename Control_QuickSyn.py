import serial
import time
import sys
# Open the serial connection
ser = serial.Serial('COM8', baudrate=115200, timeout=1)

# Function to set the frequency
def set_frequency(frequency):
    command = f":FREQ {frequency}Hz\n"
    ser.write(command.encode())
    time.sleep(0.1)
# Function to get the current frequency
def get_frequency():
    max_retries = 3
    retries = 0

    while retries < max_retries:
        ser.write(b":FREQ?\n")
        response = ser.readline().decode().strip()

        if response:
            try:
                return float(response)
            except ValueError:
                pass

        retries += 1
        time.sleep(0.1)

    raise ValueError("Failed to retrieve frequency")

# Example usage
frequency = 9.0e9  # Set the desired frequency to 10 MHz
set_frequency(frequency)
# Function to enable/disable RF output
def toggle_rf_output(enable):
    command = ":OUTP:STAT? "
    if enable:
        command += "1\n"
    else:
        command += "0\n"
    ser.write(command.encode())
    time.sleep(0.1)


current_frequency = get_frequency()
print(f"Current frequency: {current_frequency} Hz")


input("Press Enter to stop the program and close the serial connection...")
toggle_rf_output(False)
ser.close()

print("Serial connection closed. Program execution stopped.")
