// ESP32-S3 camera MJPEG stream for the vision-guided 4-DOF arm.
// Stream URL: http://<ESP32_IP>:81/stream  (IP is printed in Serial Monitor)
//
// Arduino IDE setup:
//   Board: your ESP32-S3 board (e.g. "ESP32S3 Dev Module" or "XIAO_ESP32S3")
//   Tools > PSRAM: "OPI PSRAM" (needed for the camera)

#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

// ---- Pick ONE board (uncomment the one you have) ----
#define BOARD_ESP32S3_EYE      // also Freenove ESP32-S3-WROOM CAM
// #define BOARD_XIAO_ESP32S3  // Seeed XIAO ESP32S3 Sense

const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASS = "YOUR_PASSWORD";

#if defined(BOARD_ESP32S3_EYE)
  #define PWDN_GPIO -1
  #define RESET_GPIO -1
  #define XCLK_GPIO 15
  #define SIOD_GPIO 4
  #define SIOC_GPIO 5
  #define Y9_GPIO 16
  #define Y8_GPIO 17
  #define Y7_GPIO 18
  #define Y6_GPIO 12
  #define Y5_GPIO 10
  #define Y4_GPIO 8
  #define Y3_GPIO 9
  #define Y2_GPIO 11
  #define VSYNC_GPIO 6
  #define HREF_GPIO 7
  #define PCLK_GPIO 13
#elif defined(BOARD_XIAO_ESP32S3)
  #define PWDN_GPIO -1
  #define RESET_GPIO -1
  #define XCLK_GPIO 10
  #define SIOD_GPIO 40
  #define SIOC_GPIO 39
  #define Y9_GPIO 48
  #define Y8_GPIO 11
  #define Y7_GPIO 12
  #define Y6_GPIO 14
  #define Y5_GPIO 16
  #define Y4_GPIO 18
  #define Y3_GPIO 17
  #define Y2_GPIO 15
  #define VSYNC_GPIO 38
  #define HREF_GPIO 47
  #define PCLK_GPIO 13
#endif

#define BOUNDARY "frame"
static const char* STREAM_TYPE = "multipart/x-mixed-replace;boundary=" BOUNDARY;
static const char* STREAM_BOUNDARY = "\r\n--" BOUNDARY "\r\n";
static const char* STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;

static esp_err_t stream_handler(httpd_req_t* req) {
  char part_buf[64];
  esp_err_t res = httpd_resp_set_type(req, STREAM_TYPE);
  if (res != ESP_OK) return res;

  while (true) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) { Serial.println("Frame capture failed"); res = ESP_FAIL; break; }

    size_t hlen = snprintf(part_buf, sizeof(part_buf), STREAM_PART, fb->len);
    res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, part_buf, hlen);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len);
    esp_camera_fb_return(fb);
    if (res != ESP_OK) break;  // client disconnected
  }
  return res;
}

void startStreamServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 81;
  config.ctrl_port = 32769;
  httpd_uri_t stream_uri = { .uri = "/stream", .method = HTTP_GET, .handler = stream_handler, .user_ctx = NULL };
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

void setup() {
  Serial.begin(115200);

  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO;  config.pin_d1 = Y3_GPIO;
  config.pin_d2 = Y4_GPIO;  config.pin_d3 = Y5_GPIO;
  config.pin_d4 = Y6_GPIO;  config.pin_d5 = Y7_GPIO;
  config.pin_d6 = Y8_GPIO;  config.pin_d7 = Y9_GPIO;
  config.pin_xclk = XCLK_GPIO;
  config.pin_pclk = PCLK_GPIO;
  config.pin_vsync = VSYNC_GPIO;
  config.pin_href = HREF_GPIO;
  config.pin_sccb_sda = SIOD_GPIO;
  config.pin_sccb_scl = SIOC_GPIO;
  config.pin_pwdn = PWDN_GPIO;
  config.pin_reset = RESET_GPIO;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;   // 320x240, fast enough for tracking
  config.jpeg_quality = 12;
  config.fb_count = 2;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed. Check the board selection and PSRAM setting.");
    return;
  }

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  WiFi.setSleep(false);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println();

  startStreamServer();
  Serial.print("Stream ready at: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":81/stream");
}

void loop() {
  delay(10000);
}
