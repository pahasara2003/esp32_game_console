#ifdef CAMERA_VIEWER_ONLY
#include <Arduino.h>
#include <WiFi.h>
#include <esp_rom_tjpgd.h>
#include "game.h"

namespace cameraViewer {
constexpr char SSID[] = "GameConsole-Cam";
const uint8_t BSSID[6] = {0x80, 0xF3, 0xDA, 0x62, 0x81, 0x5D};
const IPAddress CAMERA(192, 168, 4, 1);
constexpr size_t MAX_JPEG = 65536;
static uint8_t *jpeg = nullptr;
static uint32_t frames = 0;
static unsigned long countStarted = 0;
static int cameraRssi = -999;
static int cameraChannel = -1;
static int scannedNetworks = -999;
static char scannedSsid[33] = "";
static uint16_t tile[16 * 16];
alignas(4) static uint8_t work[4096];
static esp_rom_tjpgd_dec_t decoder;

struct ImageData { const uint8_t *bytes; size_t length, offset; };

static uint32_t readJpeg(esp_rom_tjpgd_dec_t *dec, uint8_t *out, uint32_t count) {
  auto *input = static_cast<ImageData *>(dec->device);
  size_t available = input->length - input->offset;
  if (count > available) count = available;
  if (out) memcpy(out, input->bytes + input->offset, count);
  input->offset += count;
  return count;
}

static uint32_t drawTile(esp_rom_tjpgd_dec_t *, void *bitmap, esp_rom_tjpgd_rect_t *rect) {
  int width = rect->right - rect->left + 1;
  int height = rect->bottom - rect->top + 1;
  if (width <= 0 || height <= 0 || width > 16 || height > 16) return 0;
  auto *rgb = static_cast<uint8_t *>(bitmap);
  for (int i = 0; i < width * height; ++i) {
    uint8_t r = rgb[i * 3], g = rgb[i * 3 + 1], b = rgb[i * 3 + 2];
    tile[i] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }
  tft.pushImage(rect->left, rect->top + 40, width, height, tile);
  return 1;
}

static void status(const char *message) {
  tft.fillRect(0, 0, 240, 36, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(message, 120, 18);
}

static bool fetchFrame(size_t &length) {
  WiFiClient client;
  client.setTimeout(3000);
  if (!client.connect(CAMERA, 80, 2000)) return false;
  client.print("GET /jpg HTTP/1.1\r\nHost: 192.168.4.1\r\nConnection: close\r\n\r\n");
  String line = client.readStringUntil('\n');
  if (!line.startsWith("HTTP/1.1 200") && !line.startsWith("HTTP/1.0 200")) return false;
  length = 0;
  for (int i = 0; i < 32; ++i) {
    line = client.readStringUntil('\n');
    line.trim();
    if (!line.length()) break;
    if (line.startsWith("Content-Length:") || line.startsWith("content-length:"))
      length = line.substring(15).toInt();
  }
  if (!length || length > MAX_JPEG) return false;
  size_t received = client.readBytes(jpeg, length);
  client.stop();
  return received == length;
}

static bool showFrame(size_t length) {
  ImageData image{jpeg, length, 0};
  memset(&decoder, 0, sizeof(decoder));
  auto prepared = esp_rom_tjpgd_prepare(&decoder, readJpeg, work, sizeof(work), &image);
  if (prepared != JDR_OK || decoder.width != 240 || decoder.height != 240) {
    Serial.printf("[VIEW] JPEG prepare=%d size=%lux%lu\n", prepared,
                  (unsigned long)decoder.width, (unsigned long)decoder.height);
    return false;
  }
  tft.setSwapBytes(true);
  auto result = esp_rom_tjpgd_decomp(&decoder, drawTile, 0);
  if (result != JDR_OK) Serial.printf("[VIEW] JPEG decode=%d\n", result);
  return result == JDR_OK;
}
}

void cameraViewerSetup() {
  Serial.begin(115200);
  Serial.println("[VIEW] setup start");
  tft.init();
  setDisplayBrightness(displayBrightness, false);
  Serial.println("[VIEW] display ready");
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);
  cameraViewer::status("CONNECTING CAMERA");
  cameraViewer::jpeg = static_cast<uint8_t *>(malloc(cameraViewer::MAX_JPEG));
  if (!cameraViewer::jpeg) {
    cameraViewer::status("NO JPEG MEMORY");
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.onEvent([](arduino_event_id_t event, arduino_event_info_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED)
      Serial.printf("[VIEW] disconnect reason=%d\n", info.wifi_sta_disconnected.reason);
    if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP)
      Serial.println("[VIEW] got IP");
  });
  int networkCount = WiFi.scanNetworks();
  cameraViewer::scannedNetworks = networkCount;
  Serial.printf("[VIEW] scan found %d networks\n", networkCount);
  for (int i = 0; i < networkCount; ++i) {
    if (i == 0) strlcpy(cameraViewer::scannedSsid, WiFi.SSID(i).c_str(), sizeof(cameraViewer::scannedSsid));
    if (WiFi.SSID(i) == cameraViewer::SSID) {
      cameraViewer::cameraRssi = WiFi.RSSI(i);
      cameraViewer::cameraChannel = WiFi.channel(i);
    }
  }
  WiFi.begin(cameraViewer::SSID, nullptr, 6, cameraViewer::BSSID);
  Serial.printf("[VIEW] WiFi begin, status=%d\n", WiFi.status());
  cameraViewer::countStarted = millis();
}

void cameraViewerLoop() {
  using namespace cameraViewer;
  if (!jpeg) return;
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastReport = 0;
    if (millis() - lastReport > 3000) {
      Serial.printf("[VIEW] WiFi status=%d scan=%d first=%s camera RSSI=%d channel=%d\n",
                    WiFi.status(), scannedNetworks, scannedSsid, cameraRssi, cameraChannel);
      lastReport = millis();
    }
    status("CONNECTING CAMERA");
    if (millis() - countStarted > 10000) {
      WiFi.begin(SSID, nullptr, 6, BSSID);
      countStarted = millis();
    }
    delay(200);
    return;
  }
  size_t length;
  if (!fetchFrame(length)) {
    status("WAITING FOR FRAME");
    delay(150);
    return;
  }
  if (showFrame(length)) {
    ++frames;
    if (millis() - countStarted >= 1000) {
      float fps = frames * 1000.0f / (millis() - countStarted);
      Serial.printf("[VIEW] %.1f fps, %u bytes JPEG, free heap %u\n", fps,
                    (unsigned)length, (unsigned)ESP.getFreeHeap());
      char label[32];
      snprintf(label, sizeof(label), "CAMERA  %.1f FPS", fps);
      status(label);
      frames = 0;
      countStarted = millis();
    }
  }
}
#endif
