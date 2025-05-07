##########################################################
# Liliana Holguin
# NMSU
# SPRING 2025
# ENGR 402
# Capstone - Overhead Wireless EV Charging System
##########################################################


import serial
import keyboard
import time
import cv2
import numpy as np


def convert_pixel_to_steps(pixel_x, pixel_y):
    x_step_min, x_step_max = 2466, 6878
    y_step_min, y_step_max = 2895, 7241

    x_pixel_min, x_pixel_max = 0, 302
    y_pixel_min, y_pixel_max = 0, 440

    compression = 0.6  

    x_center = (x_step_min + x_step_max) / 2
    y_center = (y_step_min + y_step_max) / 2

    x_range = (x_step_max - x_step_min) * compression / 2
    y_range = (y_step_max - y_step_min) * compression / 2

    x_step_min_new = x_center - x_range
    x_step_max_new = x_center + x_range
    y_step_min_new = y_center - y_range
    y_step_max_new = y_center + y_range

    x_steps = x_step_min_new + (pixel_x - x_pixel_min) * (x_step_max_new - x_step_min_new) / (x_pixel_max - x_pixel_min)
    y_steps = y_step_min_new + (pixel_y - y_pixel_min) * (y_step_max_new - y_step_min_new) / (y_pixel_max - y_pixel_min)

    x_steps_corrected = x_steps - 600 
    y_steps_corrected = y_steps - 600 

    return int(x_steps_corrected), int(y_steps_corrected)





def read_img(ser, timeout=5):
    start = b'\xff\xd8'  
    end = b'\xff\xd9'    
    img_data = b''

    print("Waiting for start of image...")
    start_time = time.time()
    while True:
        if time.time() - start_time > timeout:
            print("Timeout: No start of image found.")
            return None
        byte = ser.read(1)
        if byte == b'':
            continue
        img_data += byte
        if start in img_data:
            print("Found start of image!")
            img_data = img_data[img_data.index(start):]
            break

    print("Receiving image data...")
    while True:
        byte = ser.read(1)
        if byte == b'':
            continue
        img_data += byte
        if end in img_data[-2:]:
            print("Found end of image!")
            break

    return img_data


def preprocess_image(image):
    lab = cv2.cvtColor(image, cv2.COLOR_BGR2LAB)
    l, a, b = cv2.split(lab)
    clahe = cv2.createCLAHE(clipLimit=3.0, tileGridSize=(8,8))
    cl = clahe.apply(l)
    lab = cv2.merge((cl,a,b))
    bright_image = cv2.cvtColor(lab, cv2.COLOR_LAB2BGR)
    blurred = cv2.GaussianBlur(bright_image, (7,7), 0)
    return blurred


def coordinate(filename):
    image = cv2.imread(filename)
    image = cv2.rotate(image, cv2.ROTATE_90_COUNTERCLOCKWISE)
    preprocessed = preprocess_image(image)
    if preprocessed is None:
        return None

    gray = cv2.cvtColor(preprocessed, cv2.COLOR_BGR2GRAY)
    _, mask = cv2.threshold(gray, 60, 255, cv2.THRESH_BINARY_INV)

    kernel = np.ones((5,5), np.uint8)
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)

    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    if not contours:
        print("No contours found!")
        return None, mask

    MIN_AREA = 500
    large_contours = [cnt for cnt in contours if cv2.contourArea(cnt) > MIN_AREA]

    if not large_contours:
        print("No large contours!")
        return None, mask

    largest = max(large_contours, key=cv2.contourArea)
    M = cv2.moments(largest)

    if M['m00'] == 0:
        print("Zero area contour!")
        return None, mask

    cx = int(M['m10'] / M['m00'])
    cy = int(M['m01'] / M['m00'])

    x_steps, y_steps = convert_pixel_to_steps(cx, cy)

    data = f"{x_steps},{y_steps}\n"
    ser.write(data.encode())
    
    print(f"Sending over serial: {data}")

    print(f"Black object center (pixel): ({cx}, {cy})")
    print(f"Target motor steps: X={x_steps}, Y={y_steps}")

    cv2.drawContours(image, [largest], -1, (0, 255, 0), 2)
    cv2.circle(image, (cx, cy), 5, (0, 0, 255), -1)
    cv2.putText(image, f"({cx},{cy})", (cx+10, cy), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255,0,0), 2)

    cv2.imshow('Black Object Detection', image)
    cv2.imshow('Binary Mask', mask)

    cv2.waitKey(0)
    cv2.destroyAllWindows()

    return (x_steps, y_steps), mask



ser = serial.Serial('COM14', 921600, timeout=1)
print(f"Serial port opened: {ser.name}")

ser.write(b'RESET\n')

print("Press SPACE to take Snapshot")


while True:
    if keyboard.is_pressed('space'):
        print("Taking picture!")
        ser.write(b'\x10') 

        ser.flush()
        time.sleep(1.5)

        img_data = read_img(ser)
        if img_data is None:
            print("Failed to read image. Exiting to prevent buffer issues.")
            ser.close()
            exit(1)

        filename = f"snapshot.jpg"
        with open(filename, 'wb') as f:
            f.write(img_data)

        print(f"Saved image as {filename}")
        coordinate(filename)

        time.sleep(1)

        ser.close()
        print("Serial port closed")
        break  