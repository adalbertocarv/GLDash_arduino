// =====================================================
// SKETCH COMPLETO -- VW Gol GL 1990 . RPM + Marcha
// Display: OLED 2.42" SSD1309 128x64 I2C
// MCU: Arduino Nano
//
// MEMORIA RAM: construtor _1_ (page buffer = 128 bytes de RAM).
// O construtor _F_ (full buffer) exige 1024 bytes -- o Nano
// tem 2048 bytes totais e trava silenciosamente com _F_.
// firstPage()/nextPage() redesenha a tela em paginas de 8px.
// TODO o codigo de desenho DEVE ficar dentro do do/while.
//
// Linhas ativas  --> trapezio preenchido com 2x drawTriangle()
// drawTriangle() do U8g2 EH preenchido (filled polygon).
// O trapezio e dividido em 2 triangulos sem lacuna.
// Linhas inativas --> linhas inativas nao sao desenhadas
// =====================================================

#include <U8g2lib.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// -- Display: construtor _1_ (page buffer, baixo consumo de RAM) --
U8G2_SSD1309_128X64_NONAME2_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
// Alternativa se nao inicializar: U8G2_SSD1306_128X64_NONAME_1_HW_I2C

// -- Sensor IMU MPU6050 e estado -----------------------
Adafruit_MPU6050 mpu;
bool mpuFound = false;

// -- Configuracao --------------------------------------
#define RPM_MIN   700
#define RPM_MAX   7000
#define FAN_LINES 30

// -- Arrays PROGMEM (gerados pelo RPM Fan Designer) ----
// pivo(64, 29) | arco 181 -> 360 graus
// comprLat=35 comprTopo=23 | espIn=0.00 espOut=2.30
const int8_t fanX1[FAN_LINES]    PROGMEM = {49, 50, 51, 53, 54, 56, 57, 58, 59, 60, 61, 62, 62, 63, 64, 64, 65, 66, 66, 67, 68, 69, 70, 71, 72, 74, 76, 77, 78, 79};
const int8_t fanY1[FAN_LINES]    PROGMEM = {29, 27, 26, 25, 24, 24, 24, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 25, 26, 27, 29};
const int8_t fanX2[FAN_LINES]    PROGMEM = {14, 16, 20, 24, 28, 32, 35, 39, 43, 46, 50, 53, 56, 60, 63, 66, 69, 72, 75, 79, 82, 86, 89, 93, 97, 101, 105, 109, 112, 114};
const int8_t fanY2[FAN_LINES]    PROGMEM = {28, 23, 19, 15, 11, 9, 7, 5, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 7, 9, 12, 15, 19, 24, 29};
const int8_t fanPerpX[FAN_LINES] PROGMEM = {2, 12, 23, 33, 43, 53, 62, 70, 77, 83, 89, 93, 97, 99, 100, 100, 99, 96, 93, 88, 82, 76, 68, 60, 51, 42, 32, 21, 11, 0};
const int8_t fanPerpY[FAN_LINES] PROGMEM = {-100, -99, -97, -94, -90, -85, -79, -72, -64, -55, -46, -36, -26, -15, -5, 6, 17, 27, 38, 47, 57, 65, 73, 80, 86, 91, 95, 98, 99, 100};

// Espessuras x10 (divide por 10.0 no codigo)
const uint8_t FAN_W_IN  = 0;  // -> 0.00 px (lado do pivo)
const uint8_t FAN_W_OUT = 23;  // -> 2.30 px (borda do leque)

// -- Posicoes dos elementos de texto (pixels) ----------
#define GEAR_X      64   // centro horizontal da marcha
#define GEAR_Y      56   // baseline vertical da marcha
#define RPM_LABEL_X 110   // X do label "RPM"
#define RPM_LABEL_Y 37   // Y do label "RPM"
#define RPM_VAL_X   113   // X direito do valor RPM (alinha pela direita)
#define RPM_VAL_Y   50   // Y do valor RPM
#define G_LABEL_X   5     // X do label "G"
#define G_LABEL_Y   37     // Y do label "G"
#define G_VAL_X     2     // X do valor G
#define G_VAL_Y     50     // Y do valor G

// -- Estado global -------------------------------------
int  currentRPM  = 0;
int  targetRPM   = 0;
char currentGear = 'N'; // Neutro eh mais seguro que '3'

// -- Prototipos (obrigatorio no C++: declara antes de usar) ---
int  lerRPM();
char lerMarcha();
void desenhaLeque(int rpm);
void desenhaMarcha(char gear);
void desenhaRPMNumero(int rpm);
void drawFilledTrapezoid(int16_t ax, int16_t ay,
                          int16_t bx, int16_t by,
                          int16_t cx, int16_t cy,
                          int16_t dx, int16_t dy);
void atualizaGForce();


// -- Setup ---------------------------------------------
void setup() {
  u8g2.begin();
  u8g2.setContrast(255);

  // Tenta inicializar o MPU6050
  if (mpu.begin()) {
    mpuFound = true;
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }
}

// -- Loop ----------------------------------------------
void loop() {
  targetRPM   = lerRPM();
  currentGear = lerMarcha();
  // Suavizacao exponencial: ~4 ciclos para atingir o alvo
  currentRPM += (targetRPM - currentRPM) >> 2;

  atualizaGForce();

  // firstPage/nextPage: redesenha em paginas de 8 linhas.
  u8g2.firstPage();
  do {
    desenhaLeque(currentRPM);
    desenhaMarcha(currentGear);
    desenhaRPMNumero(currentRPM);
  } while (u8g2.nextPage());

  delay(40); // ~25 FPS
}

// ── Trapézio preenchido via 2× drawTriangle ────
void drawFilledTrapezoid(int16_t ax, int16_t ay,
                          int16_t bx, int16_t by,
                          int16_t cx, int16_t cy,
                          int16_t dx, int16_t dy) {
  u8g2.drawTriangle(ax, ay, bx, by, dx, dy); // triângulo 1
  u8g2.drawTriangle(bx, by, cx, cy, dx, dy); // triângulo 2
}


// ── Leque de RPM ──────────────────────────────
void desenhaLeque(int rpm) {
  int linhasAtivas = map(constrain(rpm, RPM_MIN, RPM_MAX),
                         RPM_MIN, RPM_MAX, 0, FAN_LINES);
  float wIn  = FAN_W_IN  / 10.0f;   // meia-largura interna em px
  float wOut = FAN_W_OUT / 10.0f;   // meia-largura externa em px

  for (int i = 0; i < FAN_LINES; i++) {
    float p1x = (int8_t)pgm_read_byte(&fanX1[i]);
    float p1y = (int8_t)pgm_read_byte(&fanY1[i]);
    float p2x = (int8_t)pgm_read_byte(&fanX2[i]);
    float p2y = (int8_t)pgm_read_byte(&fanY2[i]);
    float px  = (int8_t)pgm_read_byte(&fanPerpX[i]) / 100.0f;
    float py  = (int8_t)pgm_read_byte(&fanPerpY[i]) / 100.0f;

    if (i < linhasAtivas) {
      int16_t ax = (int16_t)(p1x + px * wIn);
      int16_t ay = (int16_t)(p1y + py * wIn);
      int16_t bx = (int16_t)(p1x - px * wIn);
      int16_t by = (int16_t)(p1y - py * wIn);
      int16_t cx = (int16_t)(p2x - px * wOut);
      int16_t cy = (int16_t)(p2y - py * wOut);
      int16_t dx = (int16_t)(p2x + px * wOut);
      int16_t dy = (int16_t)(p2y + py * wOut);
      drawFilledTrapezoid(ax, ay, bx, by, cx, cy, dx, dy);
    } else {
      // inativa -- invisivel
    }
  }
}

// ── Número da marcha ──────────────────────────
void desenhaMarcha(char gear) {
  char buf[2] = {gear, 0};
  if (gear >= '1' && gear <= '5')
    u8g2.setFont(u8g2_font_logisoso32_tn);
  else
    u8g2.setFont(u8g2_font_logisoso24_tr);
  int fw = u8g2.getStrWidth(buf);
  u8g2.setCursor(GEAR_X - (fw / 2), GEAR_Y);
  u8g2.print(gear);
}

// -- RPM e Forca G ------------------------------------
// Relacoes de transmissao do AP 1.6 (caixa 5-vel) x diferencial 4.27
// RPM_TO_KMH[i] = (60 * WHEEL_CIRC * 0.001) / (GEAR_RATIO[i] * FINAL_RATIO)
static const float RPM_TO_KMH[6] PROGMEM = {
  0.0f,       // [0] neutro/invalido
  0.01174f,   // [1] 1a  ratio 3.455
  0.02083f,   // [2] 2a  ratio 1.944
  0.03096f,   // [3] 3a  ratio 1.308
  0.04255f,   // [4] 4a  ratio 0.952
  0.05319f,   // [5] 5a  ratio 0.762
};

static int           _lastRPM     = 0;
static char          _lastGear    = 'N';
static float         _gForce      = 0.0f;
static unsigned long _lastMillis = 0;

void atualizaGForce() {
  if (mpuFound) {
    sensors_event_t a, g_event, temp;
    mpu.getEvent(&a, &g_event, &temp);
    // Aceleração no eixo Y (longitudinal), converte m/s^2 para G (divide por 9.80665)
    float g = a.acceleration.y / 9.80665f;
    _gForce = _gForce * 0.8f + g * 0.2f;
  } else {
    // Fallback: calcula a aceleração simulada com base na variação de RPM e marcha
    unsigned long now = millis();
    float dt = (now - _lastMillis) / 1000.0f;
    if (dt < 0.01f) return;

    int gearIdx = (currentGear >= '1' && currentGear <= '5') ? (currentGear - '0') : 0;
    float ratio = (gearIdx > 0) ? pgm_read_float(&RPM_TO_KMH[gearIdx]) : 0.0f;

    float kmhNow  = currentRPM   * ratio;
    float kmhLast = _lastRPM     * ratio;
    float accelMs2 = ((kmhNow - kmhLast) / 3.6f) / dt; // m/s^2
    float g = accelMs2 / 9.80665f;

    _gForce = _gForce * 0.6f + g * 0.4f;

    _lastRPM    = currentRPM;
    _lastGear   = currentGear;
    _lastMillis = now;
  }
}

void desenhaRPMNumero(int rpm) {
  char buf[8];

  // -- Lado direito: label "RPM" + valor numerico --
  u8g2.setFont(u8g2_font_micro_tr);
  u8g2.setCursor(RPM_LABEL_X, RPM_LABEL_Y);
  u8g2.print("RPM");

  sprintf(buf, "%d", rpm);
  u8g2.setFont(u8g2_font_6x10_mf);
  u8g2.setCursor(RPM_VAL_X - u8g2.getStrWidth(buf), RPM_VAL_Y);
  u8g2.print(buf);

  // -- Lado esquerdo: label "G" + valor da forca G --
  u8g2.setFont(u8g2_font_micro_tr);
  u8g2.setCursor(G_LABEL_X, G_LABEL_Y);
  u8g2.print("G");

  // Formata com sinal e 1 casa decimal: "+1.3" / "-0.8"
  char sign = (_gForce >= 0) ? '+' : '-';
  float absG = (_gForce >= 0) ? _gForce : -_gForce;
  int intPart  = (int)absG;
  int fracPart = (int)((absG - intPart) * 10);
  sprintf(buf, "%c%d.%d", sign, intPart, fracPart);

  u8g2.setFont(u8g2_font_6x10_mf);
  u8g2.setCursor(G_VAL_X, G_VAL_Y);
  u8g2.print(buf);
}

// ── Leitura RPM ───────────────────────────────
int lerRPM() {
  return map(analogRead(A6), 0, 1023, RPM_MIN, RPM_MAX);
}

// ── Leitura marcha (4 Sensores Hall AH3503 em quadrado) ──
char lerMarcha() {
  int hall_0_value = analogRead(A0); // Superior Esquerdo
  int hall_1_value = analogRead(A1); // Inferior Esquerdo
  int hall_2_value = analogRead(A2); // Inferior Direito
  int hall_3_value = analogRead(A3); // Superior Direito

  // Converte desvio em relação ao centro (512) para porcentagem (0-100)
  int pct0 = (abs(hall_0_value - 512) * 100) / 512;
  int pct1 = (abs(hall_1_value - 512) * 100) / 512;
  int pct2 = (abs(hall_2_value - 512) * 100) / 512;
  int pct3 = (abs(hall_3_value - 512) * 100) / 512;

  if (pct0 > 30 && pct3 > 30) {
    return '3'; // 3ª Marcha
  }
  else if (pct1 > 30 && pct2 > 30) {
    return '4'; // 4ª Marcha
  }
  else if (pct0 > 30) {
    return '1'; // 1ª Marcha
  }
  else if (pct3 > 30) {
    return '5'; // 5ª Marcha
  }
  else if (pct1 > 30) {
    return '2'; // 2ª Marcha
  }
  else if (pct2 > 30) {
    return 'R'; // Marcha Ré
  }
  else {
    return 'N'; // Neutro
  }
}
