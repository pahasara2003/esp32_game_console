#include <Arduino.h>
#include <JPEGDEC.h>
#include <SD.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <driver/i2s.h>
#include <esp_timer.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/stream_buffer.h>

// Independent player for the MJPEG + PCM AVI supplied with this project.
constexpr char VIDEO_PATH[] = "/gameconsole-video-320x240-hq.avi";
constexpr int SD_CS = 9, BACKLIGHT = 0;
constexpr int I2S_BCLK = 6, I2S_LRCLK = 5, I2S_DIN = 7;
constexpr i2s_port_t AUDIO_PORT = I2S_NUM_0;
constexpr size_t AUDIO_BUFFER_BYTES = 32768;
constexpr unsigned FRAME_QUEUE_LENGTH = 4;
constexpr unsigned FRAME_POOL_COUNT = FRAME_QUEUE_LENGTH + 2;
constexpr uint32_t MAX_JPEG_BYTES = 32768;
constexpr uint32_t HEAP_HEADROOM = 48 * 1024;
constexpr size_t SD_CACHE_BYTES = 8192;
constexpr unsigned DMA_COUNT = 8, DMA_LENGTH = 256;
constexpr uint32_t DMA_FRAMES = DMA_COUNT * DMA_LENGTH;

TFT_eSPI screen;
JPEGDEC jpeg;
File avi;
SemaphoreHandle_t spiMutex = nullptr;
QueueHandle_t frameQueue = nullptr;
QueueHandle_t freeFrameQueue = nullptr;
StreamBufferHandle_t audioBuffer = nullptr;
TaskHandle_t audioTaskHandle = nullptr;
portMUX_TYPE clockLock = portMUX_INITIALIZER_UNLOCKED;
uint8_t *frameStorage[FRAME_POOL_COUNT] = {};
unsigned framePoolCount = 0;
uint32_t frameBufferBytes = 0;
uint8_t sdCache[SD_CACHE_BYTES];
uint32_t fileSize = 0, filePosition = 0, cacheStart = 0;
size_t cacheLength = 0;

struct AviInfo {
  uint32_t frameUs = 83333, totalFrames = 0;
  uint32_t suggestedFrameBytes = 0;
  uint32_t sampleRate = 0, moviStart = 0, moviEnd = 0;
  int width = 0, height = 0;
  uint16_t channels = 0, bits = 0;
  int videoStream = -1, audioStream = -1;
} info;

struct Frame {
  uint8_t *data;
  uint32_t size, index;
};

volatile bool demuxDone = false, audioDone = false, failed = false;
const char *failureReason = "Playback error";
uint64_t submittedSamples = 0;
int64_t drainStartedUs = 0;
uint32_t shownFrames = 0, skippedFrames = 0;
uint32_t averageRenderUs = 25000, maximumRenderUs = 0;
uint32_t frameBlitUs = 0, averageBlitUs = 0, averageSpiWaitUs = 0;
bool playing = false, finishShown = false;

static uint16_t le16(const uint8_t *p) {
  return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}

static uint32_t le32(const uint8_t *p) {
  return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
         (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

static bool fourcc(const uint8_t *p, const char *id) {
  return memcmp(p, id, 4) == 0;
}

static void fail(const char *reason) {
  failureReason = reason;
  failed = true;
  Serial.println(reason);
}

// Only the demux task uses the file once playback starts. Its SD transactions
// and the main task's TFT transactions share this lock because they use SPI2.
static bool readExact(void *buffer, size_t bytes) {
  if (filePosition > fileSize || bytes > fileSize - filePosition) return false;
  uint8_t *output = static_cast<uint8_t *>(buffer);
  while (bytes) {
    if (filePosition < cacheStart || filePosition - cacheStart >= cacheLength) {
      xSemaphoreTake(spiMutex, portMAX_DELAY);
      const bool positioned = avi.position() == filePosition || avi.seek(filePosition);
      cacheStart = filePosition;
      cacheLength = positioned ? avi.read(sdCache, std::min<size_t>(SD_CACHE_BYTES, fileSize - filePosition)) : 0;
      xSemaphoreGive(spiMutex);
      if (!cacheLength) return false;
    }
    const size_t offset = filePosition - cacheStart;
    const size_t count = std::min(bytes, cacheLength - offset);
    memcpy(output, sdCache + offset, count);
    output += count;
    filePosition += count;
    bytes -= count;
  }
  return true;
}

static bool seekFile(uint32_t position) {
  // Most AVI seeks only advance past padding or move within the read cache.
  // Defer physical seeks until an uncached read actually needs one.
  if (position > fileSize) return false;
  filePosition = position;
  return true;
}

static bool chunkHeader(uint8_t *header, uint32_t limit, uint32_t &end, uint32_t &next) {
  const uint32_t start = filePosition;
  if (start > limit || limit - start < 8 || !readExact(header, 8)) return false;
  const uint32_t size = le32(header + 4);
  const uint64_t wideEnd = uint64_t(start) + 8 + size;
  if (wideEnd > limit) return false;
  end = uint32_t(wideEnd);
  next = end + (size & 1);
  return next >= end && next <= limit;
}

static bool parseStream(uint32_t end, int streamNumber) {
  uint8_t type[4] = {}, format[40] = {};
  size_t formatBytes = 0;
  uint32_t scale = 0, rate = 0, suggestedBytes = 0;
  while (filePosition + 8 <= end) {
    uint8_t header[8], data[56] = {};
    uint32_t chunkEnd, next;
    if (!chunkHeader(header, end, chunkEnd, next)) return false;
    uint32_t size = le32(header + 4);
    if (fourcc(header, "strh")) {
      size_t count = std::min<size_t>(size, sizeof(data));
      if (count < 36 || !readExact(data, count)) return false;
      memcpy(type, data, 4);
      scale = le32(data + 20);
      rate = le32(data + 24);
      if (count >= 40) suggestedBytes = le32(data + 36);
    } else if (fourcc(header, "strf")) {
      formatBytes = std::min<size_t>(size, sizeof(format));
      if (!readExact(format, formatBytes)) return false;
    }
    if (!seekFile(next)) return false;
  }
  if (fourcc(type, "vids")) {
    if (formatBytes < 20 || !fourcc(format + 16, "MJPG")) return false;
    info.videoStream = streamNumber;
    info.width = int32_t(le32(format + 4));
    info.height = abs(int32_t(le32(format + 8)));
    info.suggestedFrameBytes = suggestedBytes;
    if (scale && rate) info.frameUs = uint64_t(scale) * 1000000 / rate;
  } else if (fourcc(type, "auds")) {
    if (formatBytes < 16 || le16(format) != 1) return false;
    info.audioStream = streamNumber;
    info.channels = le16(format + 2);
    info.sampleRate = le32(format + 4);
    info.bits = le16(format + 14);
  }
  return true;
}

static bool parseHeaderList(uint32_t end) {
  int streamNumber = 0;
  while (filePosition + 8 <= end) {
    uint8_t header[8], data[40] = {};
    uint32_t chunkEnd, next;
    if (!chunkHeader(header, end, chunkEnd, next)) return false;
    if (fourcc(header, "avih")) {
      size_t count = std::min<size_t>(le32(header + 4), sizeof(data));
      if (count < 40 || !readExact(data, count)) return false;
      info.frameUs = le32(data);
      info.totalFrames = le32(data + 16);
    } else if (fourcc(header, "LIST")) {
      if (le32(header + 4) < 4 || !readExact(data, 4)) return false;
      if (fourcc(data, "strl") && !parseStream(chunkEnd, streamNumber++)) return false;
    }
    if (!seekFile(next)) return false;
  }
  return true;
}

static bool openAvi() {
  avi = SD.open(VIDEO_PATH);
  if (!avi) { fail("Missing 320x240 HQ AVI on SD card"); return false; }
  fileSize = avi.size();
  filePosition = cacheStart = 0;
  cacheLength = 0;
  uint8_t riff[12];
  if (!readExact(riff, sizeof(riff)) || !fourcc(riff, "RIFF") || !fourcc(riff + 8, "AVI ")) {
    fail("Not a RIFF AVI file");
    return false;
  }
  const uint32_t end = std::min<uint64_t>(avi.size(), uint64_t(le32(riff + 4)) + 8);
  while (filePosition + 8 <= end) {
    uint8_t header[8], type[4];
    uint32_t chunkEnd, next;
    if (!chunkHeader(header, end, chunkEnd, next)) break;
    if (fourcc(header, "LIST")) {
      if (le32(header + 4) < 4 || !readExact(type, 4)) break;
      if (fourcc(type, "hdrl") && !parseHeaderList(chunkEnd)) {
        fail("Unsupported AVI stream header");
        return false;
      }
      if (fourcc(type, "movi")) {
        info.moviStart = filePosition;
        info.moviEnd = chunkEnd;
        break;
      }
    }
    if (!seekFile(next)) break;
  }
  if (!info.moviStart || info.videoStream < 0 || info.audioStream < 0 ||
      info.width != screen.width() || info.height != screen.height() ||
      !info.frameUs || info.bits != 16 ||
      (info.channels != 1 && info.channels != 2) ||
      info.sampleRate < 8000 || info.sampleRate > 48000) {
    fail("Need native 320x240 MJPEG + PCM");
    return false;
  }
  Serial.printf("AVI %dx%d %.2f fps; PCM %lu Hz %u channels; %lu frames\n",
                info.width, info.height, 1000000.0 / info.frameUs,
                (unsigned long)info.sampleRate, info.channels,
                (unsigned long)info.totalFrames);
  return seekFile(info.moviStart);
}

static int streamIndex(const uint8_t *id) {
  if (id[0] < '0' || id[0] > '9' || id[1] < '0' || id[1] > '9') return -1;
  return (id[0] - '0') * 10 + id[1] - '0';
}

static void demuxTask(void *) {
  uint32_t videoIndex = 0;
  uint32_t listEnds[8] = {info.moviEnd};
  unsigned depth = 1;
  uint8_t pcm[2048];
  while (!failed && depth) {
    if (filePosition >= listEnds[depth - 1]) {
      uint32_t next = listEnds[--depth];
      if (!seekFile(next + (next & 1))) { fail("AVI list seek failed"); break; }
      continue;
    }
    uint8_t header[8];
    uint32_t end, next;
    if (!chunkHeader(header, listEnds[depth - 1], end, next)) {
      fail("Invalid or truncated AVI chunk");
      break;
    }
    const uint32_t size = le32(header + 4);
    if (fourcc(header, "LIST")) {
      uint8_t type[4];
      if (size < 4 || !readExact(type, 4)) { fail("Invalid AVI list"); break; }
      if (fourcc(type, "rec ") || fourcc(type, "movi")) {
        if (depth >= 8) { fail("AVI lists nested too deeply"); break; }
        listEnds[depth++] = end;
        continue;
      }
    } else if (streamIndex(header) == info.videoStream &&
               header[2] == 'd' && (header[3] == 'c' || header[3] == 'b')) {
      const uint32_t index = videoIndex++;
      if (size) {
        if (size > frameBufferBytes) { fail("JPEG exceeds AVI frame buffer size"); break; }
        Frame frame{nullptr, size, index};
        while (!failed && xQueueReceive(freeFrameQueue, &frame.data, pdMS_TO_TICKS(20)) != pdTRUE) {}
        if (failed) break;
        if (!readExact(frame.data, size)) {
          xQueueSend(freeFrameQueue, &frame.data, 0);
          fail("SD video read failed");
          break;
        }
        bool queued = false;
        while (!failed && !(queued = xQueueSend(frameQueue, &frame, pdMS_TO_TICKS(20)) == pdTRUE)) {}
        if (!queued) xQueueSend(freeFrameQueue, &frame.data, 0);
      }
    } else if (streamIndex(header) == info.audioStream && header[2] == 'w' && header[3] == 'b') {
      if (size % (info.channels * 2)) { fail("Unaligned PCM audio chunk"); break; }
      uint32_t remaining = size;
      while (remaining && !failed) {
        size_t count = std::min<size_t>(remaining, sizeof(pcm));
        if (!readExact(pcm, count)) { fail("SD audio read failed"); break; }
        size_t sent = 0;
        while (sent < count && !failed)
          sent += xStreamBufferSend(audioBuffer, pcm + sent, count - sent, pdMS_TO_TICKS(20));
        remaining -= count;
      }
    }
    if (!seekFile(next)) { fail("AVI chunk seek failed"); break; }
  }
  xSemaphoreTake(spiMutex, portMAX_DELAY);
  avi.close();
  xSemaphoreGive(spiMutex);
  demuxDone = true;
  vTaskDelete(nullptr);
}

static bool beginAudio() {
  i2s_config_t config = {};
  config.mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX);
  config.sample_rate = info.sampleRate;
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  config.dma_buf_count = DMA_COUNT;
  config.dma_buf_len = DMA_LENGTH;
  config.use_apll = false;
  config.tx_desc_auto_clear = true;
  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = I2S_BCLK;
  pins.ws_io_num = I2S_LRCLK;
  pins.data_out_num = I2S_DIN;
  pins.data_in_num = I2S_PIN_NO_CHANGE;
  if (i2s_driver_install(AUDIO_PORT, &config, 0, nullptr) != ESP_OK) return false;
  if (i2s_set_pin(AUDIO_PORT, &pins) != ESP_OK) return false;
  i2s_zero_dma_buffer(AUDIO_PORT);
  return true;
}

static void audioTask(void *) {
  int16_t input[256], stereo[512];
  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  while (!failed) {
    const size_t bytes = xStreamBufferReceive(audioBuffer, input, sizeof(input), pdMS_TO_TICKS(20));
    if (!bytes) {
      if (demuxDone) break;
      continue;
    }
    const size_t frames = bytes / (info.channels * sizeof(int16_t));
    for (size_t i = 0; i < frames; ++i) {
      // Preserve headroom for the small speaker amplifier.
      stereo[i * 2] = int32_t(input[i * info.channels]) * 3 / 4;
      stereo[i * 2 + 1] = int32_t(input[i * info.channels + info.channels - 1]) * 3 / 4;
    }
    const uint8_t *data = reinterpret_cast<uint8_t *>(stereo);
    size_t remaining = frames * 4;
    while (remaining && !failed) {
      size_t written = 0;
      if (i2s_write(AUDIO_PORT, data, remaining, &written, pdMS_TO_TICKS(100)) != ESP_OK || !written) {
        fail("I2S audio write failed");
        break;
      }
      data += written;
      remaining -= written;
      portENTER_CRITICAL(&clockLock);
      submittedSamples += written / 4;
      portEXIT_CRITICAL(&clockLock);
    }
  }
  portENTER_CRITICAL(&clockLock);
  drainStartedUs = esp_timer_get_time();
  portEXIT_CRITICAL(&clockLock);
  // Let the final DMA buffers reach the speaker before stopping I2S.
  vTaskDelay(pdMS_TO_TICKS((DMA_FRAMES * 1000 / info.sampleRate) + 30));
  i2s_zero_dma_buffer(AUDIO_PORT);
  i2s_stop(AUDIO_PORT);
  audioDone = true;
  vTaskDelete(nullptr);
}

static uint64_t playbackUs() {
  portENTER_CRITICAL(&clockLock);
  const uint64_t samples = submittedSamples;
  const int64_t drain = drainStartedUs;
  portEXIT_CRITICAL(&clockLock);
  uint64_t audible = samples > DMA_FRAMES ? samples - DMA_FRAMES : 0;
  if (drain) {
    const uint64_t elapsedSamples = uint64_t(esp_timer_get_time() - drain) * info.sampleRate / 1000000;
    audible = std::min<uint64_t>(samples, audible + elapsedSamples);
  }
  return audible * 1000000 / info.sampleRate;
}

static int drawJpeg(JPEGDRAW *block) {
  // The file already matches the panel. Transfer the decoder's RGB565 block
  // directly, with no pixel scaling, crop math, or intermediate image copy.
  // The caller owns SPI for the entire frame, avoiding transaction changes
  // and SD-card interruptions between the JPEG decoder's small MCU blocks.
  const int64_t started = esp_timer_get_time();
  screen.pushImage(block->x, block->y, block->iWidth, block->iHeight, block->pPixels);
  frameBlitUs += esp_timer_get_time() - started;
  return 1;
}

static void message(const char *text, uint16_t color = TFT_WHITE) {
  xSemaphoreTake(spiMutex, portMAX_DELAY);
  screen.fillRect(0, screen.height() / 2 - 20, screen.width(), 40, TFT_BLACK);
  screen.setTextDatum(MC_DATUM);
  screen.setTextColor(color, TFT_BLACK);
  screen.drawString(text, screen.width() / 2, screen.height() / 2, 2);
  xSemaphoreGive(spiMutex);
}

static void logProgress() {
  static uint32_t previousSecond = UINT32_MAX;
  const uint64_t now = playbackUs();
  const uint32_t second = now / 1000000;
  if (second == previousSecond) return;
  previousSecond = second;
  char line[168];
  const int count = snprintf(line, sizeof(line),
      "Playback %lu s shown=%lu skipped=%lu render=%lu/%lu blit=%lu wait=%lu us audio=%u heap=%u\n",
      (unsigned long)second, (unsigned long)shownFrames, (unsigned long)skippedFrames,
      (unsigned long)averageRenderUs, (unsigned long)maximumRenderUs,
      (unsigned long)averageBlitUs, (unsigned long)averageSpiWaitUs,
      unsigned(xStreamBufferBytesAvailable(audioBuffer)), unsigned(ESP.getFreeHeap()));
  if (count > 0 && count < int(sizeof(line)) && Serial.availableForWrite() >= count)
    Serial.write(reinterpret_cast<const uint8_t *>(line), count);
}

void setup() {
  Serial.begin(115200);
  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, HIGH);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  spiMutex = xSemaphoreCreateMutex();
  if (!spiMutex) return;
  screen.init();
  screen.setRotation(1);
  screen.fillScreen(TFT_BLACK);
  Serial.println("Opening AVI...");
  SPI.end();
  SPI.begin(1, 21, 2, SD_CS);
  digitalWrite(TFT_CS, HIGH);
  if (!SD.begin(SD_CS, SPI, 40000000)) { fail("SD card mount failed"); return; }
  if (!openAvi()) return;
  audioBuffer = xStreamBufferCreate(AUDIO_BUFFER_BYTES, 1);
  if (info.suggestedFrameBytes > MAX_JPEG_BYTES) { fail("AVI needs JPEG frames above 32 KB"); return; }
  frameBufferBytes = info.suggestedFrameBytes ? info.suggestedFrameBytes : MAX_JPEG_BYTES;
  frameBufferBytes = (frameBufferBytes + 1023) & ~uint32_t(1023);
  const uint32_t heap = ESP.getFreeHeap();
  const uint32_t availableForFrames = heap > HEAP_HEADROOM ? heap - HEAP_HEADROOM : 0;
  framePoolCount = std::min<unsigned>(FRAME_POOL_COUNT, availableForFrames / (frameBufferBytes + 32));
  if (framePoolCount < 4) { fail("Not enough RAM for JPEG buffer pool"); return; }
  frameQueue = xQueueCreate(framePoolCount - 2, sizeof(Frame));
  freeFrameQueue = xQueueCreate(framePoolCount, sizeof(uint8_t *));
  if (!frameQueue || !freeFrameQueue || !audioBuffer) { fail("Cannot allocate playback buffers"); return; }
  for (unsigned i = 0; i < framePoolCount; ++i) {
    frameStorage[i] = static_cast<uint8_t *>(malloc(frameBufferBytes));
    if (!frameStorage[i]) { fail("Cannot allocate JPEG frame pool"); return; }
    xQueueSend(freeFrameQueue, &frameStorage[i], 0);
  }
  Serial.printf("Frame pool: %u buffers x %lu bytes, heap=%u\n", framePoolCount,
                (unsigned long)frameBufferBytes, unsigned(ESP.getFreeHeap()));
  if (!beginAudio()) { fail("I2S setup failed"); return; }
  Serial.println("Buffering...");
  if (xTaskCreate(audioTask, "avi-audio", 4096, nullptr, 3, &audioTaskHandle) != pdPASS) {
    fail("Cannot start audio task"); return;
  }
  // Refill PCM and compressed-frame queues as soon as a display transaction
  // releases SPI. Audio output remains the highest-priority task (3).
  if (xTaskCreate(demuxTask, "avi-reader", 6144, nullptr, 2, nullptr) != pdPASS) {
    fail("Cannot start SD reader"); return;
  }
  const uint32_t started = millis();
  while (!failed && !demuxDone &&
         (xStreamBufferBytesAvailable(audioBuffer) < 8192 || !uxQueueMessagesWaiting(frameQueue))) {
    // A smaller RAM-adapted frame queue can fill before 8 KB of interleaved
    // PCM appears. Start with its available audio instead of deadlocking the reader.
    if (uxQueueMessagesWaiting(frameQueue) >= framePoolCount - 2 &&
        xStreamBufferBytesAvailable(audioBuffer) > 0) break;
    if (millis() - started > 5000) { fail("AVI prebuffer timed out"); break; }
    delay(2);
  }
  if (failed) return;
  playing = true;
  Serial.println("Playing");
  xTaskNotifyGive(audioTaskHandle);
}

void loop() {
  if (failed) {
    if (!finishShown && spiMutex) {
      message(failureReason, TFT_RED);
      finishShown = true;
    }
    delay(20);
    return;
  }
  if (!playing) { delay(20); return; }
  Frame frame{};
  if (xQueueReceive(frameQueue, &frame, pdMS_TO_TICKS(10)) == pdTRUE) {
    const uint64_t due = uint64_t(frame.index) * info.frameUs;
    const uint32_t leadUs = std::min(averageRenderUs, info.frameUs / 2);
    while (!failed && !audioDone && playbackUs() + leadUs < due) delay(1);
    const uint64_t now = playbackUs();
    if (!failed && (now <= due + info.frameUs || audioDone)) {
      const int64_t renderStarted = esp_timer_get_time();
      uint32_t spiWaitUs = 0;
      frameBlitUs = 0;
      if (jpeg.openRAM(frame.data, frame.size, drawJpeg)) {
        jpeg.setPixelType(RGB565_BIG_ENDIAN);
        if (jpeg.getWidth() == info.width && jpeg.getHeight() == info.height) {
          const int64_t waiting = esp_timer_get_time();
          xSemaphoreTake(spiMutex, portMAX_DELAY);
          spiWaitUs = esp_timer_get_time() - waiting;
          screen.startWrite();
          const bool decoded = jpeg.decode(0, 0, 0);
          screen.endWrite();
          xSemaphoreGive(spiMutex);
          if (decoded) ++shownFrames;
          else ++skippedFrames;
        } else ++skippedFrames;
        jpeg.close();
      } else ++skippedFrames;
      const uint32_t renderUs = esp_timer_get_time() - renderStarted;
      averageRenderUs = (averageRenderUs * 7 + renderUs) / 8;
      maximumRenderUs = std::max(maximumRenderUs, renderUs);
      averageBlitUs = (averageBlitUs * 7 + frameBlitUs) / 8;
      averageSpiWaitUs = (averageSpiWaitUs * 7 + spiWaitUs) / 8;
    } else ++skippedFrames;
    xQueueSend(freeFrameQueue, &frame.data, 0);
  }
  logProgress();
  if (demuxDone && audioDone && !uxQueueMessagesWaiting(frameQueue)) {
    playing = false;
    Serial.printf("Finished: shown=%lu skipped=%lu\n",
                  (unsigned long)shownFrames, (unsigned long)skippedFrames);
  }
}
