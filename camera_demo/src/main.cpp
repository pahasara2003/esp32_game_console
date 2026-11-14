#include <Arduino.h>
#include <WiFi.h>
#include <esp_camera.h>
#include <esp_http_server.h>
#include <esp_wifi.h>

static volatile uint32_t jpgRequests = 0;

// AI-Thinker ESP32-CAM pinout, matching Espressif's camera example.
static camera_config_t cameraConfig() {
  camera_config_t c = {};
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer = LEDC_TIMER_0;
  c.pin_d0 = 5; c.pin_d1 = 18; c.pin_d2 = 19; c.pin_d3 = 21;
  c.pin_d4 = 36; c.pin_d5 = 39; c.pin_d6 = 34; c.pin_d7 = 35;
  c.pin_xclk = 0; c.pin_pclk = 22; c.pin_vsync = 25; c.pin_href = 23;
  c.pin_sccb_sda = 26; c.pin_sccb_scl = 27;
  c.pin_pwdn = 32; c.pin_reset = -1;
  c.xclk_freq_hz = 20000000;
  c.pixel_format = PIXFORMAT_JPEG;
  c.frame_size = FRAMESIZE_240X240;
  c.jpeg_quality = 16;
  c.fb_count = psramFound() ? 2 : 1;
  c.fb_location = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  c.grab_mode = psramFound() ? CAMERA_GRAB_LATEST : CAMERA_GRAB_WHEN_EMPTY;
  return c;
}

static esp_err_t jpgHandler(httpd_req_t *req) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Capture failed");
  ++jpgRequests;
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  esp_err_t result = httpd_resp_send(req, (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  return result;
}

static esp_err_t streamHandler(httpd_req_t *req) {
  httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  while (true) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) return ESP_FAIL;
    char part[100];
    int n = snprintf(part, sizeof(part), "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
                     (unsigned)fb->len);
    esp_err_t result = httpd_resp_send_chunk(req, part, n);
    if (result == ESP_OK) result = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
    if (result == ESP_OK) result = httpd_resp_send_chunk(req, "\r\n", 2);
    esp_camera_fb_return(fb);
    if (result != ESP_OK) return result;
    delay(1);
  }
}

static esp_err_t rootHandler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_sendstr(req, "<html><body style='background:#111;color:white;font-family:sans-serif'>"
                               "<h2>ESP32-CAM test stream</h2><img src='/stream' width='480'>"
                               "<p><a href='/jpg'>Snapshot</a></p></body></html>");
}

void setup() {
  Serial.begin(115200);
  Serial.println("[CAM] Starting AI-Thinker camera test");
  camera_config_t config = cameraConfig();
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[CAM] init failed: 0x%x\n", err);
    return;
  }
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP("GameConsole-Cam", nullptr, 6, false, 2)) {
    Serial.println("[CAM] hotspot failed");
    return;
  }
  httpd_config_t serverConfig = HTTPD_DEFAULT_CONFIG();
  serverConfig.max_uri_handlers = 3;
  serverConfig.stack_size = 8192;
  httpd_handle_t server = nullptr;
  if (httpd_start(&server, &serverConfig) != ESP_OK) {
    Serial.println("[CAM] HTTP server failed");
    return;
  }
  httpd_uri_t root = {"/", HTTP_GET, rootHandler, nullptr};
  httpd_uri_t jpg = {"/jpg", HTTP_GET, jpgHandler, nullptr};
  httpd_uri_t stream = {"/stream", HTTP_GET, streamHandler, nullptr};
  httpd_register_uri_handler(server, &root);
  httpd_register_uri_handler(server, &jpg);
  httpd_register_uri_handler(server, &stream);
  Serial.printf("[CAM] ready: http://%s/stream (snapshot /jpg)\n",
                WiFi.softAPIP().toString().c_str());
}

void loop() {
  wifi_sta_list_t stations = {};
  esp_wifi_ap_get_sta_list(&stations);
  Serial.printf("[CAM] clients=%d jpg=%u heap=%u\n", stations.num,
                (unsigned)jpgRequests, (unsigned)ESP.getFreeHeap());
  delay(5000);
}
