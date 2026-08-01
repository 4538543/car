
from maix import (
    app,
    camera,
    comm,
    display,
    err,
    http,
    image,
    network,
    nn,
    pinmap,
    time,
    touchscreen,
    uart,
)
import json
import math
import os
import struct

report_on = True
APP_CMD_DETECT_RES = 0x02

# A small or stationary ball may receive a relatively low confidence score.
# Lower this value if detections are still missed; raise it if false positives
# become frequent.
# Steel-ball confidence often falls while stationary because the highlight and
# background dominate its appearance. The tight pipe ROI, size filter and
# nearest-target gate provide the false-positive protection, so inference and
# re-acquisition thresholds can be lower than before.
CONF_THRESHOLD = 0.18
ACQUIRE_CONF_THRESHOLD = 0.30
IOU_THRESHOLD = 0.30

MIN_ROI_SIZE = 20
TRACK_MAX_DISTANCE = 100
# After this many lost frames, search the complete ROI for a fresh target.
# Until re-acquired, the last position remains published with TRACK_HELD.
TRACK_RESET_FRAMES = 24
# Ignore only one isolated missed frame; holding starts on the 2nd miss.
TRACK_HOLD_DELAY_FRAMES = 1
TRACK_EMA_ALPHA = 0.30
# After five-point calibration, reject detections whose centre is too far
# from the four line segments joining the five sampled ball centres.
CALIBRATION_LINE_TOLERANCE_PX = 18
MIN_BOX_SIDE = 8
MAX_BOX_SIDE = 80
MIN_BOX_ASPECT = 0.50
MAX_BOX_ASPECT = 2.00
CALIBRATION_PATH = "/root/steelball_vision_calibration.json"
VISION_SETTINGS_PATH = "/root/steelball_vision_settings.json"
CALIBRATION_SAMPLE_COUNT = 15
CALIBRATION_VALUES = [0, 300, 500, 700, 1000]
CALIBRATION_LABELS = ["LEFT", "-5cm", "CENTER", "+5cm", "RIGHT"]
BUTTON_HEIGHT = 38
UART_DEVICE = "/dev/ttyS4"
UART_BAUD = 115200
PROTOCOL_VERSION = 2
FLAG_VALID = 0x01
FLAG_ROI_SET = 0x02
FLAG_CALIBRATED = 0x04
FLAG_TRACK_HELD = 0x08
FLAG_TARGET_SET = 0x10
FLAG_PIPE_AXIS_VALID = 0x20
INVALID_U16 = 0xFFFF
DYNAMIC_PIPE_EDGE_ENABLED = False

DEFAULT_VISION_SETTINGS = {
    "edge_enabled": False,
    "edge_low": 45,
    "edge_high": 135,
    "line_threshold": 700,
    "min_line_length": 90,
    "angle_tolerance": 12,
    "axis_alpha": 0.20,
    "axis_max_shift": 35,
    "edge_every_n": 3,
}

SETTING_ITEMS = [
    ("edge_low", "CANNY LOW", 0, 250, 5),
    ("edge_high", "CANNY HIGH", 5, 255, 5),
]

# Wireless preview.  The MaixCAM2 and iPad must be on the same LAN.
# A modest JPEG quality and frame-rate leave more CPU time for detection/UART.
STREAM_ENABLED = True
STREAM_PORT = 8000
STREAM_JPEG_QUALITY = 75
STREAM_MAX_FPS = 12
STREAM_FRAME_INTERVAL_MS = max(1, 1000 // STREAM_MAX_FPS)
STREAM_HTML = """<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport"
        content="width=device-width,initial-scale=1,maximum-scale=1">
  <title>MaixCAM2 无线画面</title>
  <style>
    html,body{margin:0;background:#111;color:#eee;font-family:sans-serif}
    main{min-height:100vh;display:flex;flex-direction:column;
         align-items:center;justify-content:center;gap:10px}
    img{display:block;max-width:100vw;max-height:calc(100vh - 48px);
        width:auto;height:auto;object-fit:contain}
    p{margin:0 8px 8px;font-size:14px;color:#aaa}
  </style>
</head>
<body><main>
  <img src="/stream" alt="MaixCAM2 stream">
  <p>MaixCAM2 实时检测画面</p>
</main></body>
</html>"""


def median(values):
    values = sorted(values)
    count = len(values)
    if count == 0:
        return None
    middle = count // 2
    if count % 2:
        return float(values[middle])
    return 0.5 * (values[middle - 1] + values[middle])


def calibration_valid(points):
    if not isinstance(points, list) or len(points) != 5:
        return False
    for index, point in enumerate(points):
        if (
            not isinstance(point, list)
            or len(point) != 3
            or point[0] != CALIBRATION_VALUES[index]
        ):
            return False
    return True


def load_calibration():
    try:
        with open(CALIBRATION_PATH, "r") as file:
            data = json.load(file)
        saved_roi = data.get("roi")
        saved_points = data.get("points")
        saved_target = data.get("target")
        if not saved_roi or len(saved_roi) != 4:
            saved_roi = None
        if not calibration_valid(saved_points):
            saved_points = []
        if not isinstance(saved_target, int) or not 0 <= saved_target <= 1000:
            saved_target = None
        print("Loaded calibration:", saved_roi, saved_points, saved_target)
        return saved_roi, saved_points, saved_target
    except Exception as exception:
        print("No saved calibration:", exception)
        return None, [], None


def save_calibration(roi, points, target):
    try:
        with open(CALIBRATION_PATH, "w") as file:
            json.dump(
                {"version": 3, "roi": roi, "points": points, "target": target},
                file,
            )
        print("Saved calibration:", roi, points, target)
    except Exception as exception:
        print("Failed to save calibration:", exception)


def load_vision_settings():
    settings = dict(DEFAULT_VISION_SETTINGS)
    try:
        with open(VISION_SETTINGS_PATH, "r") as file:
            saved = json.load(file)
        for key in settings:
            if key in saved:
                settings[key] = saved[key]
    except Exception as exception:
        print("Using default vision settings:", exception)
    return settings


def save_vision_settings(settings):
    try:
        with open(VISION_SETTINGS_PATH, "w") as file:
            json.dump(settings, file)
        print("Saved vision settings:", settings)
    except Exception as exception:
        print("Failed to save vision settings:", exception)


def point_projection(point, axis):
    """Return signed pixel coordinate along axis from its first endpoint."""
    if point is None or axis is None:
        return None
    left, right = axis
    dx = right[0] - left[0]
    dy = right[1] - left[1]
    length_squared = dx * dx + dy * dy
    if length_squared < 100:
        return None
    length = math.sqrt(length_squared)
    return (
        (point[0] - left[0]) * dx
        + (point[1] - left[1]) * dy
    ) / length


def static_axis_from_points(points):
    if not calibration_valid(points):
        return None
    return (
        (float(points[0][1]), float(points[0][2])),
        (float(points[-1][1]), float(points[-1][2])),
    )


def normalized_position(center, points, current_axis):
    """Map the ball center to 0..1000 with four piecewise intervals."""
    if center is None or not calibration_valid(points) or current_axis is None:
        return None

    static_axis = static_axis_from_points(points)
    current_length = point_projection(current_axis[1], current_axis)
    static_length = point_projection(static_axis[1], static_axis)
    current_s = point_projection(center, current_axis)
    if (
        current_s is None
        or current_length is None
        or static_length is None
        or current_length < 10
        or static_length < 10
    ):
        return None

    # Express the current measurement as a fraction of the dynamically tracked
    # pipe axis, then map it back onto the five-point calibration axis.
    static_s = current_s / current_length * static_length
    knot_s = [
        point_projection((point[1], point[2]), static_axis)
        for point in points
    ]
    if any(value is None for value in knot_s):
        return None

    if static_s <= knot_s[0]:
        return 0
    if static_s >= knot_s[-1]:
        return 1000

    for index in range(4):
        s0 = knot_s[index]
        s1 = knot_s[index + 1]
        if s0 <= static_s <= s1 and s1 - s0 >= 2:
            ratio = (static_s - s0) / (s1 - s0)
            value = (
                CALIBRATION_VALUES[index]
                + ratio
                * (CALIBRATION_VALUES[index + 1]
                   - CALIBRATION_VALUES[index])
            )
            return int(max(0, min(1000, round(value))))
    return None


def angle_difference_180(first, second):
    difference = abs(first - second) % 180.0
    return min(difference, 180.0 - difference)


def detect_pipe_axis(img, roi, static_axis, previous_axis, settings):
    """Track the unmodified pipe from its natural long edges."""
    if roi is None or static_axis is None or not settings["edge_enabled"]:
        return None
    try:
        gray = img.to_format(image.Format.FMT_GRAYSCALE)
        edges = gray.find_edges(
            image.EdgeDetector.EDGE_CANNY,
            roi=list(roi),
            threshold=[
                int(settings["edge_low"]),
                int(settings["edge_high"]),
            ],
        )
        lines = edges.find_lines(
            roi=list(roi),
            threshold=float(settings["line_threshold"]),
            theta_margin=8,
            rho_margin=12,
        )
    except Exception as exception:
        print("Pipe edge detection failed:", exception)
        return None

    sdx = static_axis[1][0] - static_axis[0][0]
    sdy = static_axis[1][1] - static_axis[0][1]
    static_angle = math.degrees(math.atan2(sdy, sdx)) % 180.0
    static_mid = (
        0.5 * (static_axis[0][0] + static_axis[1][0]),
        0.5 * (static_axis[0][1] + static_axis[1][1]),
    )
    static_length = math.sqrt(sdx * sdx + sdy * sdy)

    candidates = []
    for line in lines:
        length = float(line.length())
        if length < settings["min_line_length"]:
            continue
        dx = line.x2() - line.x1()
        dy = line.y2() - line.y1()
        line_angle = math.degrees(math.atan2(dy, dx)) % 180.0
        if (
            angle_difference_180(line_angle, static_angle)
            > settings["angle_tolerance"]
        ):
            continue
        midpoint = (
            0.5 * (line.x1() + line.x2()),
            0.5 * (line.y1() + line.y2()),
        )
        shift = math.sqrt(
            (midpoint[0] - static_mid[0]) ** 2
            + (midpoint[1] - static_mid[1]) ** 2
        )
        if shift > settings["axis_max_shift"]:
            continue
        candidates.append((length, line_angle, midpoint))

    if not candidates:
        return None

    # Use up to four strongest parallel natural edges. Averaging their
    # midpoints estimates the pipe centre instead of following one rim.
    candidates.sort(key=lambda item: item[0], reverse=True)
    candidates = candidates[:4]
    weight_sum = sum(item[0] for item in candidates)
    center_x = sum(item[0] * item[2][0] for item in candidates) / weight_sum
    center_y = sum(item[0] * item[2][1] for item in candidates) / weight_sum

    # Direction averaging is doubled because a line has no arrow.
    cosine = sum(
        item[0] * math.cos(math.radians(2.0 * item[1]))
        for item in candidates
    )
    sine = sum(
        item[0] * math.sin(math.radians(2.0 * item[1]))
        for item in candidates
    )
    angle = 0.5 * math.atan2(sine, cosine)
    ux = math.cos(angle)
    uy = math.sin(angle)
    if ux * sdx + uy * sdy < 0:
        ux = -ux
        uy = -uy

    half_length = 0.5 * static_length
    measured_axis = (
        (center_x - ux * half_length, center_y - uy * half_length),
        (center_x + ux * half_length, center_y + uy * half_length),
    )
    if previous_axis is None:
        return measured_axis

    alpha = float(settings["axis_alpha"])
    return (
        (
            previous_axis[0][0]
            + alpha * (measured_axis[0][0] - previous_axis[0][0]),
            previous_axis[0][1]
            + alpha * (measured_axis[0][1] - previous_axis[0][1]),
        ),
        (
            previous_axis[1][0]
            + alpha * (measured_axis[1][0] - previous_axis[1][0]),
            previous_axis[1][1]
            + alpha * (measured_axis[1][1] - previous_axis[1][1]),
        ),
    )


def crc16_ccitt(data):
    crc = 0xFFFF
    for value in data:
        crc ^= value << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def build_mcu_frame(frame_id, timestamp_ms, flags, position, obj, center,
                    candidate_count, target_position):
    if obj is None or center is None:
        center_x = INVALID_U16
        center_y = INVALID_U16
        box_w = 0
        box_h = 0
        confidence = 0
    else:
        center_x = int(max(0, min(65534, center[0])))
        center_y = int(max(0, min(65534, center[1])))
        box_w = int(max(0, min(65534, obj.w)))
        box_h = int(max(0, min(65534, obj.h)))
        confidence = int(max(0, min(100, round(obj.score * 100))))

    payload = struct.pack(
        "<BBHIHHHHHBBH",
        PROTOCOL_VERSION,
        flags,
        frame_id & 0xFFFF,
        timestamp_ms & 0xFFFFFFFF,
        INVALID_U16 if position is None else position,
        center_x,
        center_y,
        box_w,
        box_h,
        confidence,
        min(candidate_count, 255),
        INVALID_U16 if target_position is None else target_position,
    )
    return b"\xA5\x5A" + payload + struct.pack("<H", crc16_ccitt(payload))


def object_center_in_roi(obj, roi):
    """Return True when the center of a detection is inside the ROI."""
    if roi is None:
        return True
    roi_x, roi_y, roi_w, roi_h = roi
    center_x = obj.x + obj.w // 2
    center_y = obj.y + obj.h // 2
    return (
        roi_x <= center_x < roi_x + roi_w
        and roi_y <= center_y < roi_y + roi_h
    )


def object_size_valid(obj):
    """Reject obvious specks, large objects and very elongated boxes."""
    if (
        obj.w < MIN_BOX_SIDE
        or obj.h < MIN_BOX_SIDE
        or obj.w > MAX_BOX_SIDE
        or obj.h > MAX_BOX_SIDE
    ):
        return False
    aspect = obj.w / max(1, obj.h)
    return MIN_BOX_ASPECT <= aspect <= MAX_BOX_ASPECT


def make_roi(x1, y1, x2, y2, image_width, image_height):
    """Create a normalized, image-bounded ROI from two touch points."""
    left = max(0, min(x1, x2))
    top = max(0, min(y1, y2))
    right = min(image_width - 1, max(x1, x2))
    bottom = min(image_height - 1, max(y1, y2))
    return (left, top, right - left, bottom - top)


def object_center(obj):
    return (obj.x + obj.w // 2, obj.y + obj.h // 2)


def point_to_segment_distance(point, start, end):
    """Return the shortest pixel distance from point to a finite segment."""
    px, py = point
    x1, y1 = start
    x2, y2 = end
    dx = x2 - x1
    dy = y2 - y1
    length_squared = dx * dx + dy * dy
    if length_squared <= 1:
        return math.sqrt((px - x1) ** 2 + (py - y1) ** 2)
    ratio = ((px - x1) * dx + (py - y1) * dy) / length_squared
    ratio = max(0.0, min(1.0, ratio))
    nearest_x = x1 + ratio * dx
    nearest_y = y1 + ratio * dy
    return math.sqrt((px - nearest_x) ** 2 + (py - nearest_y) ** 2)


def center_near_calibration_polyline(center, points):
    """Accept a centre only near one of the four calibrated segments."""
    if not calibration_valid(points):
        # Before five-point calibration, preserve the original behaviour.
        return True
    sampled_centres = [
        (float(point[1]), float(point[2]))
        for point in points
    ]
    minimum_distance = min(
        point_to_segment_distance(
            center,
            sampled_centres[index],
            sampled_centres[index + 1],
        )
        for index in range(4)
    )
    return minimum_distance <= CALIBRATION_LINE_TOLERANCE_PX


def choose_primary(objs, last_center):
    """Acquire confidently; once locked, accept only a nearby candidate."""
    if not objs:
        return None

    if last_center is None:
        acquisition_candidates = [
            obj for obj in objs
            if obj.score >= ACQUIRE_CONF_THRESHOLD
        ]
        if not acquisition_candidates:
            return None
        return max(acquisition_candidates, key=lambda obj: obj.score)

    last_x, last_y = last_center
    nearest = min(
        objs,
        key=lambda obj: (
            object_center(obj)[0] - last_x
        ) ** 2 + (
            object_center(obj)[1] - last_y
        ) ** 2,
    )
    center_x, center_y = object_center(nearest)
    distance_sq = (center_x - last_x) ** 2 + (center_y - last_y) ** 2
    if distance_sq <= TRACK_MAX_DISTANCE ** 2:
        return nearest
    # Never jump to a distant high-confidence object while tracking.
    return None


def encode_objs(objs):
    '''
        encode objs info to bytes body for protocol
        2B x(LE) + 2B y(LE) + 2B w(LE) + 2B h(LE) + 2B idx + 4B score(float) ...
    '''
    body = b''
    for obj in objs:
        body += struct.pack("<hhHHHf", obj.x, obj.y, obj.w, obj.h, obj.class_id, obj.score)
    return body

# The model has been deployed to this directory on the device.
# Keep a script-directory fallback so this file can also run when packaged
# together with the model files.
device_model_path = "/root/models/model-314524/model_314524.mud"
local_model_path = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "model_314524.mud",
)

if os.path.exists(device_model_path):
    model_path = device_model_path
elif os.path.exists(local_model_path):
    model_path = local_model_path
else:
    raise FileNotFoundError(
        "Model not found. Expected: "
        f"{device_model_path} or {local_model_path}"
    )

print(f"Loading model: {model_path}")
detector = nn.YOLOv5(model=model_path)

input_width = detector.input_width()
input_height = detector.input_height()

cam = camera.Camera(input_width, input_height, detector.input_format())
dis = display.Display()
ts = touchscreen.TouchScreen()

p = comm.CommProtocol(buff_size = 1024)

# MaixCAM2 UART4: A21/TX -> MCU RX, A22/RX <- MCU TX.
err.check_raise(
    pinmap.set_pin_function("A21", "UART4_TX"),
    "Failed to map UART4 TX",
)
err.check_raise(
    pinmap.set_pin_function("A22", "UART4_RX"),
    "Failed to map UART4 RX",
)
serial = uart.UART(UART_DEVICE, UART_BAUD)
print(f"Vision UART ready: {UART_DEVICE}, {UART_BAUD}")

# Start an MJPEG-over-HTTP server.  Streaming is optional: a Wi-Fi or HTTP
# failure must never stop the detector or the MCU serial link.
jpeg_stream = None
stream_last_ms = 0
if STREAM_ENABLED:
    try:
        jpeg_stream = http.JpegStreamer("", STREAM_PORT)
        err.check_raise(
            jpeg_stream.set_html(STREAM_HTML),
            "Failed to set stream page",
        )
        err.check_raise(jpeg_stream.start(), "Failed to start JPEG stream")
        wifi_ip = network.wifi.Wifi().get_ip()
        if wifi_ip:
            print(f"iPad preview: http://{wifi_ip}:{STREAM_PORT}")
        else:
            print(
                f"JPEG stream started on port {STREAM_PORT}; "
                "connect Wi-Fi and check the device IP in Settings"
            )
    except Exception as exception:
        jpeg_stream = None
        print("Wireless preview disabled:", exception)

roi, calibration_points, target_position = load_calibration()
vision_settings = load_vision_settings()
drawing_roi = False
roi_mode = False
settings_mode = False
selected_setting = 0
tuning_show_edges = False
calibration_step = len(calibration_points)
roi_start = None
roi_preview = None
last_primary_center = None
last_primary_obj = None
track_missed_frames = TRACK_RESET_FRAMES
recent_centers = []
frame_id = 0
static_pipe_axis = static_axis_from_points(calibration_points)
current_pipe_axis = static_pipe_axis
pipe_axis_valid = False
edge_axis_locked = False
edge_frame_counter = 0

while not app.need_exit():
    # msg = p.get_msg()

    timestamp_ms = time.ticks_ms()
    img = cam.read()

    button_width = input_width // 4
    button_y = input_height - BUTTON_HEIGHT
    buttons = {
        "ROI": (0, button_y, button_width, BUTTON_HEIGHT),
        "CAL": (button_width, button_y, button_width, BUTTON_HEIGHT),
        "CAP": (2 * button_width, button_y, button_width, BUTTON_HEIGHT),
        "CLEAR": (
            3 * button_width,
            button_y,
            input_width - 3 * button_width,
            BUTTON_HEIGHT,
        ),
    }

    # CAL records L, -5 cm, centre, +5 cm and R from stable ball detections.
    while ts.available():
        touch_x, touch_y, pressed = ts.read0()
        image_x, image_y = image.resize_map_pos_reverse(
            input_width,
            input_height,
            dis.width(),
            dis.height(),
            image.Fit.FIT_CONTAIN,
            touch_x,
            touch_y,
        )
        image_x = max(0, min(input_width - 1, image_x))
        image_y = max(0, min(input_height - 1, image_y))

        touched_button = None
        for name, rect in buttons.items():
            x, y, w, h = rect
            if x <= image_x < x + w and y <= image_y < y + h:
                touched_button = name
                break

        if pressed and not drawing_roi:
            if touched_button == "ROI":
                roi_mode = True
            elif (
                touched_button == "CAL"
                and len(recent_centers) >= CALIBRATION_SAMPLE_COUNT
            ):
                if calibration_step >= len(CALIBRATION_VALUES):
                    calibration_points = []
                    calibration_step = 0
                    target_position = None
                captured_center = [
                    median([center[0] for center in recent_centers]),
                    median([center[1] for center in recent_centers]),
                ]
                calibration_points.append([
                    CALIBRATION_VALUES[calibration_step],
                    captured_center[0],
                    captured_center[1],
                ])
                calibration_step += 1
                recent_centers = []
                if calibration_step == len(CALIBRATION_VALUES):
                    static_pipe_axis = static_axis_from_points(
                        calibration_points
                    )
                    current_pipe_axis = static_pipe_axis
                    pipe_axis_valid = False
                    edge_axis_locked = False
                    save_calibration(
                        roi, calibration_points, target_position
                    )
            elif (
                touched_button == "CAP"
                and len(recent_centers) >= CALIBRATION_SAMPLE_COUNT
                and calibration_valid(calibration_points)
            ):
                # CAP stores and publishes the target; STM32 K4 starts task 3.
                capture_center = (
                    median([center[0] for center in recent_centers]),
                    median([center[1] for center in recent_centers]),
                )
                target_position = normalized_position(
                    capture_center,
                    calibration_points,
                    current_pipe_axis or static_pipe_axis,
                )
                save_calibration(
                    roi, calibration_points, target_position
                )
            elif touched_button == "CLEAR":
                roi = None
                calibration_points = []
                calibration_step = 0
                static_pipe_axis = None
                current_pipe_axis = None
                pipe_axis_valid = False
                edge_axis_locked = False
                roi_preview = None
                last_primary_center = None
                last_primary_obj = None
                track_missed_frames = TRACK_RESET_FRAMES
                recent_centers = []
                roi_mode = False
                target_position = None
                save_calibration(
                    roi, calibration_points, target_position
                )
            elif roi_mode and touched_button is None:
                drawing_roi = True
                roi_start = (image_x, image_y)
                roi_preview = (image_x, image_y, 0, 0)
        elif pressed and drawing_roi:
            roi_preview = make_roi(
                roi_start[0],
                roi_start[1],
                image_x,
                image_y,
                input_width,
                input_height,
            )
        elif not pressed and drawing_roi:
            new_roi = make_roi(
                roi_start[0],
                roi_start[1],
                image_x,
                image_y,
                input_width,
                input_height,
            )
            drawing_roi = False
            roi_preview = None
            if new_roi[2] >= MIN_ROI_SIZE and new_roi[3] >= MIN_ROI_SIZE:
                # Keep the bottom calibration buttons outside the ROI.
                roi = [
                    new_roi[0],
                    new_roi[1],
                    new_roi[2],
                    min(new_roi[3], button_y - new_roi[1]),
                ]
                calibration_points = []
                calibration_step = 0
                static_pipe_axis = None
                current_pipe_axis = None
                pipe_axis_valid = False
                edge_axis_locked = False
                last_primary_center = None
                last_primary_obj = None
                track_missed_frames = TRACK_RESET_FRAMES
                recent_centers = []
                roi_mode = False
                target_position = None
                save_calibration(
                    roi, calibration_points, target_position
                )

    detected_objs = detector.detect(
        img,
        conf_th=CONF_THRESHOLD,
        iou_th=IOU_THRESHOLD,
    )

    # Keep every target in the ROI and number them from left to right.
    objs = [
        obj for obj in detected_objs
        if (
            object_center_in_roi(obj, roi)
            and object_size_valid(obj)
            and center_near_calibration_polyline(
                object_center(obj),
                calibration_points,
            )
        )
    ]
    objs.sort(key=lambda obj: object_center(obj)[0])
    # After a long dropout allow a fresh acquisition anywhere inside the ROI.
    # The published position is still held at the last known centre until that
    # acquisition succeeds.
    tracking_reference = (
        None
        if track_missed_frames >= TRACK_RESET_FRAMES
        else last_primary_center
    )
    primary = choose_primary(objs, tracking_reference)
    track_held = False
    if primary is not None:
        measured_center = object_center(primary)
        if last_primary_center is None:
            last_primary_center = measured_center
        else:
            last_primary_center = (
                int(round(
                    last_primary_center[0]
                    + TRACK_EMA_ALPHA
                    * (measured_center[0] - last_primary_center[0])
                )),
                int(round(
                    last_primary_center[1]
                    + TRACK_EMA_ALPHA
                    * (measured_center[1] - last_primary_center[1])
                )),
            )
        last_primary_obj = primary
        track_missed_frames = 0
        recent_centers.append(last_primary_center)
        if len(recent_centers) > CALIBRATION_SAMPLE_COUNT:
            recent_centers.pop(0)
    else:
        track_missed_frames += 1
        if (
            track_missed_frames > TRACK_HOLD_DELAY_FRAMES
            and last_primary_obj is not None
            and last_primary_center is not None
        ):
            # Wait through several genuinely missing frames before assuming
            # the ball stayed still. Held frames keep the previous coordinate
            # and are marked with FLAG_TRACK_HELD (black overlay).
            primary = last_primary_obj
            track_held = True

    calibrated = calibration_valid(calibration_points)
    static_pipe_axis = static_axis_from_points(calibration_points)
    if calibrated and current_pipe_axis is None:
        current_pipe_axis = static_pipe_axis

    pipe_axis_valid = False
    if calibrated and DYNAMIC_PIPE_EDGE_ENABLED:
        edge_frame_counter += 1
        if edge_frame_counter >= int(vision_settings["edge_every_n"]):
            edge_frame_counter = 0
            detected_axis = detect_pipe_axis(
                img,
                roi,
                static_pipe_axis,
                current_pipe_axis,
                vision_settings,
            )
            if detected_axis is not None:
                current_pipe_axis = detected_axis
                pipe_axis_valid = True
                edge_axis_locked = True
            else:
                edge_axis_locked = False
        elif current_pipe_axis is not None:
            # A valid, smoothed axis is held between edge-detection frames.
            pipe_axis_valid = edge_axis_locked

    if not pipe_axis_valid:
        # Natural edges can disappear under glare or ball occlusion. Never use
        # a bad edge fit; fall back to the repeatable five-point static axis.
        current_pipe_axis = static_pipe_axis
        edge_axis_locked = False

    position = normalized_position(
        last_primary_center if primary is not None else None,
        calibration_points,
        current_pipe_axis,
    )

    flags = 0
    if primary is not None:
        flags |= FLAG_VALID
    if roi is not None:
        flags |= FLAG_ROI_SET
    if calibrated:
        flags |= FLAG_CALIBRATED
    if track_held:
        flags |= FLAG_TRACK_HELD
    if target_position is not None:
        flags |= FLAG_TARGET_SET
    if pipe_axis_valid:
        flags |= FLAG_PIPE_AXIS_VALID

    serial.write(build_mcu_frame(
        frame_id,
        timestamp_ms,
        flags,
        position,
        primary,
        last_primary_center if primary is not None else None,
        len(objs),
        target_position,
    ))
    frame_id = (frame_id + 1) & 0xFFFF

    if len(objs) > 0 and report_on:
        body = encode_objs(objs)
        p.report(APP_CMD_DETECT_RES, body)

    # The tuning page must show the actual grayscale source or the binary
    # Canny result. Building it before annotations prevents boxes/buttons from
    # becoming artificial edges.
    tuning_preview = None
    if settings_mode:
        try:
            tuning_preview = img.to_format(image.Format.FMT_GRAYSCALE)
            if tuning_show_edges:
                tuning_preview = tuning_preview.find_edges(
                    image.EdgeDetector.EDGE_CANNY,
                    roi=list(roi) if roi is not None else [],
                    threshold=[
                        int(vision_settings["edge_low"]),
                        int(vision_settings["edge_high"]),
                    ],
                )
        except Exception as exception:
            tuning_preview = None
            print("Tuning preview failed:", exception)

    # Calibration overlays.
    active_roi = roi_preview if drawing_roi else roi
    if active_roi is not None:
        roi_x, roi_y, roi_w, roi_h = active_roi
        img.draw_rect(
            roi_x,
            roi_y,
            roi_w,
            roi_h,
            color=image.COLOR_GREEN,
            thickness=2,
        )
        img.draw_string(
            roi_x,
            min(input_height - 20, roi_y + 2),
            "ROI",
            color=image.COLOR_GREEN,
        )
    elif roi_mode:
        img.draw_string(
            5,
            12,
            "Drag to draw ROI",
            color=image.COLOR_GREEN,
        )

    for index, point in enumerate(calibration_points):
        point_x, point_y = int(point[1]), int(point[2])
        img.draw_circle(
            point_x, point_y, 6, color=image.COLOR_BLUE, thickness=2
        )
        img.draw_string(
            point_x + 7,
            point_y,
            CALIBRATION_LABELS[index],
            color=image.COLOR_BLUE,
        )

    if calibrated:
        axis_color = (
            image.COLOR_GREEN if pipe_axis_valid else image.COLOR_BLUE
        )
        img.draw_line(
            int(current_pipe_axis[0][0]),
            int(current_pipe_axis[0][1]),
            int(current_pipe_axis[1][0]),
            int(current_pipe_axis[1][1]),
            color=axis_color,
            thickness=2,
        )
        if target_position is not None:
            target_ratio = target_position / 1000.0
            target_x = int(
                current_pipe_axis[0][0]
                + (current_pipe_axis[1][0] - current_pipe_axis[0][0])
                * target_ratio
            )
            target_y = int(
                current_pipe_axis[0][1]
                + (current_pipe_axis[1][1] - current_pipe_axis[0][1])
                * target_ratio
            )
            img.draw_circle(
                target_x, target_y, 10,
                color=image.COLOR_GREEN, thickness=3
            )
            img.draw_string(
                target_x + 12,
                target_y,
                f"T={target_position}",
                color=image.COLOR_GREEN,
            )

    if calibration_step < len(CALIBRATION_LABELS):
        img.draw_string(
            5,
            36,
            f"PLACE BALL: {CALIBRATION_LABELS[calibration_step]} / PRESS CAL",
            color=image.COLOR_GREEN,
        )

    if settings_mode:
        key, label, _, _, _ = SETTING_ITEMS[selected_setting]
        value = vision_settings[key]
        img.draw_rect(
            5, 64, input_width - 10, 54,
            color=image.COLOR_BLACK, thickness=-1
        )
        img.draw_string(
            10, 70, f"{label}: {value}", color=image.COLOR_YELLOW
        )
        img.draw_string(
            10,
            94,
            "VIEW=gray/edge  PARAM=low/high  BACK=save",
            color=image.COLOR_WHITE,
        )

    img.draw_string(
        5,
        input_height - BUTTON_HEIGHT - 24,
        f"COUNT: {len(objs)}",
        color=image.COLOR_GREEN,
    )
    if primary is None:
        img.draw_string(
            120,
            input_height - BUTTON_HEIGHT - 24,
            "TARGET: LOST",
            color=image.COLOR_RED,
        )
    else:
        primary_x, primary_y = last_primary_center
        position_text = "---" if position is None else str(position)
        held_text = " HOLD" if track_held else ""
        img.draw_string(
            120,
            input_height - BUTTON_HEIGHT - 24,
            f"TARGET:({primary_x},{primary_y}) P:{position_text}{held_text}",
            color=image.COLOR_YELLOW,
        )

    if track_held and last_primary_center is not None:
        held_x, held_y = last_primary_center
        held_radius = 10
        if last_primary_obj is not None:
            held_radius = max(
                8,
                int(max(last_primary_obj.w, last_primary_obj.h) / 2),
            )
        img.draw_circle(
            int(held_x),
            int(held_y),
            held_radius,
            color=image.COLOR_BLACK,
            thickness=3,
        )
        img.draw_string(
            int(held_x) + held_radius + 3,
            max(0, int(held_y) - 10),
            f"HOLD ({int(held_x)},{int(held_y)})",
            color=image.COLOR_BLACK,
        )

    for index, obj in enumerate(objs, start=1):
        is_primary = obj is primary
        color = image.COLOR_YELLOW if is_primary else image.COLOR_RED
        center_x, center_y = object_center(obj)
        img.draw_rect(obj.x, obj.y, obj.w, obj.h, color=color, thickness=2)
        img.draw_circle(center_x, center_y, 3, color=color, thickness=-1)
        marker = "*" if is_primary else ""
        msg = (
            f"{marker}{index} {detector.labels[obj.class_id]} "
            f"{obj.score:.2f} ({center_x},{center_y})"
        )
        img.draw_string(
            obj.x,
            max(0, obj.y - 20),
            msg,
            color=color,
        )

    for name, rect in buttons.items():
        button_x, button_y, button_w, button_h = rect
        img.draw_rect(
            button_x,
            button_y,
            button_w,
            button_h,
            color=image.COLOR_BLUE,
            thickness=-1,
        )
        img.draw_rect(
            button_x,
            button_y,
            button_w,
            button_h,
            color=image.COLOR_WHITE,
            thickness=1,
        )
        img.draw_string(
            button_x + 5,
            button_y + 8,
            name,
            color=image.COLOR_WHITE,
        )

    output_img = img
    if settings_mode and tuning_preview is not None:
        output_img = tuning_preview
        key, label, _, _, _ = SETTING_ITEMS[selected_setting]
        view_name = "EDGE" if tuning_show_edges else "GRAY"
        output_img.draw_rect(
            5, 5, input_width - 10, 54,
            color=image.COLOR_BLACK, thickness=-1
        )
        output_img.draw_string(
            10,
            10,
            f"VIEW:{view_name}  {label}:{vision_settings[key]}",
            color=image.COLOR_WHITE,
        )
        output_img.draw_string(
            10,
            34,
            f"LOW:{vision_settings['edge_low']} HIGH:{vision_settings['edge_high']}",
            color=image.COLOR_WHITE,
        )
        for name, rect in buttons.items():
            button_x, button_y, button_w, button_h = rect
            output_img.draw_rect(
                button_x,
                button_y,
                button_w,
                button_h,
                color=image.COLOR_BLACK,
                thickness=-1,
            )
            output_img.draw_rect(
                button_x,
                button_y,
                button_w,
                button_h,
                color=image.COLOR_WHITE,
                thickness=1,
            )
            output_img.draw_string(
                button_x + 5,
                button_y + 8,
                name,
                color=image.COLOR_WHITE,
            )

    # Publish the fully annotated frame.  Rate limiting reduces JPEG encoding
    # load and keeps model inference/UART timing responsive.
    if jpeg_stream is not None:
        stream_now_ms = time.ticks_ms()
        if stream_now_ms - stream_last_ms >= STREAM_FRAME_INTERVAL_MS:
            try:
                jpg = output_img.to_jpeg(STREAM_JPEG_QUALITY)
                jpeg_stream.write(jpg)
                stream_last_ms = stream_now_ms
            except Exception as exception:
                # Do not repeatedly throw in the real-time loop.
                print("Wireless preview stopped:", exception)
                try:
                    jpeg_stream.stop()
                except Exception:
                    pass
                jpeg_stream = None

    dis.show(output_img)
