#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "driver/i2s.h"
#include "config.h"

String uploadURL = UploadURL;
String wavURL = WavURL;

#define BUTTON_PIN 4

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Speaker
#define SPK_BCLK 26
#define SPK_LRC  25
#define SPK_DIN  27

// Mic
#define MIC_SCK 14
#define MIC_WS  15
#define MIC_SD  32

#define SAMPLE_RATE 16000
#define WAV_HEADER_SIZE 44

#define MAX_RECORD_SECONDS 3
#define MAX_AUDIO_SIZE (SAMPLE_RATE * MAX_RECORD_SECONDS * 2)
#define MAX_WAV_SIZE (WAV_HEADER_SIZE + MAX_AUDIO_SIZE)

uint8_t* wavBuffer;
int actualWavSize = 0;

void showText(String title, String body = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  display.println(title);
  display.println();

  for (int i = 0; i < body.length(); i += 21) {
    display.println(body.substring(i, i + 21));
  }

  display.display();
}

void setupMic() {
  i2s_config_t mic_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t mic_pins = {
    .bck_io_num = MIC_SCK,
    .ws_io_num = MIC_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = MIC_SD
  };

  i2s_driver_install(I2S_NUM_1, &mic_config, 0, NULL);
  i2s_set_pin(I2S_NUM_1, &mic_pins);
  i2s_zero_dma_buffer(I2S_NUM_1);
}

void setupSpeaker() {
  i2s_config_t spk_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 512,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t spk_pins = {
    .bck_io_num = SPK_BCLK,
    .ws_io_num = SPK_LRC,
    .data_out_num = SPK_DIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &spk_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &spk_pins);
  i2s_zero_dma_buffer(I2S_NUM_0);
}

void writeWavHeader(uint8_t* header, int wavSize, int sampleRate) {
  memset(header, 0, 44);

  int fileSize = wavSize - 8;
  int dataSize = wavSize - 44;

  memcpy(header, "RIFF", 4);
  header[4] = fileSize & 0xff;
  header[5] = (fileSize >> 8) & 0xff;
  header[6] = (fileSize >> 16) & 0xff;
  header[7] = (fileSize >> 24) & 0xff;

  memcpy(header + 8, "WAVEfmt ", 8);
  header[16] = 16;
  header[20] = 1;
  header[22] = 1;

  header[24] = sampleRate & 0xff;
  header[25] = (sampleRate >> 8) & 0xff;
  header[26] = (sampleRate >> 16) & 0xff;
  header[27] = (sampleRate >> 24) & 0xff;

  int byteRate = sampleRate * 2;
  header[28] = byteRate & 0xff;
  header[29] = (byteRate >> 8) & 0xff;
  header[30] = (byteRate >> 16) & 0xff;
  header[31] = (byteRate >> 24) & 0xff;

  header[32] = 2;
  header[34] = 16;

  memcpy(header + 36, "data", 4);
  header[40] = dataSize & 0xff;
  header[41] = (dataSize >> 8) & 0xff;
  header[42] = (dataSize >> 16) & 0xff;
  header[43] = (dataSize >> 24) & 0xff;
}

void recordAudio() {
  showText("Listening...", "Hold button");
  Serial.println("Recording while button held...");

  writeWavHeader(wavBuffer, MAX_WAV_SIZE, SAMPLE_RATE);

  delay(300);

  int32_t samples[256];
  size_t bytesRead;
  int offset = WAV_HEADER_SIZE;

  while (digitalRead(BUTTON_PIN) == LOW && offset < MAX_WAV_SIZE) {
    i2s_read(I2S_NUM_1, samples, sizeof(samples), &bytesRead, portMAX_DELAY);

    int count = bytesRead / sizeof(int32_t);

    for (int i = 0; i < count && offset < MAX_WAV_SIZE; i++) {
      int16_t sample16 = samples[i] >> 14;

      wavBuffer[offset++] = sample16 & 0xff;
      wavBuffer[offset++] = (sample16 >> 8) & 0xff;
    }
  }

  actualWavSize = offset;
  writeWavHeader(wavBuffer, actualWavSize, SAMPLE_RATE);

  Serial.print("Recording done. WAV size: ");
  Serial.println(actualWavSize);
}

String uploadAudio() {
  showText( "Uploading audio...","Thinking...");
  Serial.println("Uploading audio...");

  HTTPClient http;
  http.setTimeout(120000);
  http.begin(uploadURL);
  http.addHeader("Content-Type", "audio/wav");

  int httpCode = http.POST(wavBuffer, actualWavSize);

  Serial.print("Upload HTTP code: ");
  Serial.println(httpCode);

  String response = "";

  if (httpCode > 0) {
    response = http.getString();
    Serial.println("Reply:");
    Serial.println(response);
  } else {
    Serial.print("Upload error: ");
    Serial.println(httpCode);
  }

  http.end();
  return response;
}

void playWavFromURL(String url) {
  HTTPClient http;
  http.setTimeout(60000);
  http.begin(url);

  int httpCode = http.GET();

  if (httpCode != 200) {
    Serial.print("WAV HTTP error: ");
    Serial.println(httpCode);
    http.end();
    return;
  }

  WiFiClient* stream = http.getStreamPtr();

  uint8_t header[44];
  stream->readBytes(header, 44);

  uint8_t buffer[512];
  size_t bytesWritten;

  i2s_zero_dma_buffer(I2S_NUM_0);

  Serial.println("Playing WAV...");

  while (http.connected()) {
    int availableBytes = stream->available();

    if (availableBytes > 0) {
      int bytesRead = stream->readBytes(buffer, min(availableBytes, 512));

      if (bytesRead > 0) {
        i2s_write(I2S_NUM_0, buffer, bytesRead, &bytesWritten, portMAX_DELAY);
      }
    } else {
      delay(1);
    }

    if (!stream->connected() && stream->available() == 0) break;
  }

  Serial.println("Done playing");
  http.end();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Wire.begin(21, 22);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  showText("Kiwi", "Starting...");

  wavBuffer = (uint8_t*)malloc(MAX_WAV_SIZE);
  if (!wavBuffer) {
    showText("Error", "No memory");
    while (true);
  }

  WiFi.begin(ssid, password);
  showText("WiFi", "Connecting...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  showText("Kiwi ready", "Hold button to talk");
  Serial.println("\nReady");
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(200);

    setupMic();
    recordAudio();
    i2s_driver_uninstall(I2S_NUM_1);

    showText("Thinking...", "Processing...");
    String reply = uploadAudio();

    if (reply.length() > 0) {
      showText("Kiwi says:", reply);

      setupSpeaker();
      playWavFromURL(wavURL);
      i2s_driver_uninstall(I2S_NUM_0);

      showText("Done", "Hold button again");
    } else {
      showText("Error", "No reply");
    }

    while (digitalRead(BUTTON_PIN) == LOW) {
      delay(10);
    }
  }
}