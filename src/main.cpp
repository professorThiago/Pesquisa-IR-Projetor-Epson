/*
 * ============================================================
 *  Leitor IR Epson para ESP32
 *  Biblioteca: crankyoldgit/IRremoteESP8266 @ ^2.9.0
 * ============================================================
 *  Protocolo Epson proprietário — 2 frames NEC por botão:
 *   Frame 1 (wake-up): 0x81C00FF0 — igual para todos os botões
 *   Frame 2 (comando): valor completo identifica o botão
 *
 *  Hardware: receptor IR OUT → GPIO 15 | VCC → 3.3V | GND → GND
 *  Monitor Serial: 115200 baud
 * ============================================================
 */

#include <Arduino.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <IRremoteESP8266.h>

#define IR_PIN      4
#define CAPTURE_BUF 150

#define EPSON_WAKEUP 0x81C00FF0UL   // frame 1, igual para todos

// ── Tabela de comandos (valor completo do frame 2) ───────────
struct IrCommand { uint32_t frame2; const char* name; const char* desc; };
const IrCommand KNOWN_COMMANDS[] = {
  { 0xC1AA09F6UL, "POWER",  "Liga / desliga o projetor" },
  { 0xC1AA49B6UL, "FREEZE", "Congela a imagem na tela"  },
  { 0xC1AAA15EUL, "Enter", "confirma a ação" },
  { 0xC1AA0DF2UL, "Cima", "Descrição" },
  { 0xC1AA4DB2UL, "Baixo", "Descrição" },
  { 0xC1AA8D72UL, "Direita", "Descrição" },
  { 0xC1AACD32UL, "Esquerda", "Descrição" },
  { 0xC1AA19E6UL, "Volume UP", "Descrição" },
  { 0xC1AA9966UL, "Volume Down", "Descrição" },
  { 0xC1AA59A6UL, "Menu", "Descrição" },
  { 0xC1AA21DEUL, "ESC", "Descrição" },
  { 0xC1AA11EEUL, "Zoom +", "Descrição" },
  { 0xC1AA916EUL, "Zoom -", "Descrição" },
  { 0xC1AAA956UL, "Home", "Descrição" },
  { 0xC1AAC936UL, "Mute", "Descrição" },
  { 0xC1AACE31UL, "HDMI", "Descrição" },
  { 0xC1AA29D6UL, "Computer", "Descrição" },
  { 0xC1AA6E91UL, "USB", "Descrição" },
  { 0xC1AA2ED1UL, "LAN", "Descrição" },
  { 0xC1AA31CEUL, "Source Search", "Descrição" },
  { 0xC1AA847BUL, "1", "Descrição" },
  { 0xC1AA4CB3UL, "2", "Descrição" },
{ 0xC1AA6C93UL, "3", "Descrição" },
{ 0xC1AA02FDUL, "4", "Descrição" },
{ 0xC1AA42BDUL, "5", "Descrição" },
{ 0xC1AA629DUL, "6", "Descrição" },
{ 0xC1AA12EDUL, "7", "Descrição" },
{ 0xC1AA52ADUL, "8", "Descrição" },
{ 0xC1AA32CDUL, "9", "Descrição" },
{ 0xC1AA7986UL, "0", "Descrição" },
{ 0xC1AA8C73UL, "ID", "Descrição" },
{ 0xC1AAF906UL, "User", "Descrição" },
{ 0xC1AA9C63UL, "Default", "Descrição" },
{ 0xC1AAF10EUL, "Color Mode", "Descrição" },
{ 0xC1AA51AEUL, "Aspect", "Descrição" },
{ 0xC1AA41BEUL, "Split", "Descrição" },




  // Adicione aqui conforme descobrir — copie o valor do "Frame inesperado":
  // { 0xXXXXXXXXUL, "NOME", "Descrição" },
};
const uint8_t NUM_COMMANDS = sizeof(KNOWN_COMMANDS) / sizeof(KNOWN_COMMANDS[0]);

IRrecv receiver(IR_PIN, CAPTURE_BUF, 15, true);
decode_results results;

bool waitingSecondFrame = false;

const char* lookupCommand(uint32_t frame2, const char** desc);

// =============================================================
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println();
  Serial.println("╔══════════════════════════════════════╗");
  Serial.println("║  Leitor IR Epson — ESP32             ║");
  Serial.println("╚══════════════════════════════════════╝");
  Serial.printf("  Pino receptor    : GPIO %d\n", IR_PIN);
  Serial.printf("  Comandos mapeados: %d\n", NUM_COMMANDS);
  Serial.println("  Frame 1 (wake-up): 0x81C00FF0 para todos");
  Serial.println("  Frame 2 (comando): identifica o botão\n");

  receiver.enableIRIn();
  Serial.println("Aguardando sinal IR...\n");
}

// =============================================================
void loop() {
  if (!receiver.decode(&results)) return;

  if (results.decode_type == NEC && results.value != 0xFFFFFFFF) {
    uint32_t val = (uint32_t)results.value;

    if (val == EPSON_WAKEUP) {
      waitingSecondFrame = true;
      // wake-up silencioso — não imprime nada

    } else if (waitingSecondFrame) {
      waitingSecondFrame = false;

      Serial.println("──────────────────────────────────────");

      const char* desc = nullptr;
      const char* name = lookupCommand(val, &desc);

      if (name) {
        Serial.printf("  ✓ %s\n", name);
        Serial.printf("    %s\n", desc);
        Serial.printf("    Frame 2: 0x%08X\n", val);
      } else {
        Serial.printf("  ❓ Botão desconhecido\n");
        Serial.printf("    Adicione em KNOWN_COMMANDS:\n");
        Serial.printf("    { 0x%08XUL, \"NOME\", \"Descrição\" },\n", val);
      }
      Serial.println();

    } else {
      // Frame chegou sem wake-up antes — ruído ou outro controle
      // Imprime para diagnóstico mas não confunde com botão Epson
      Serial.printf("  [?] Frame sem wake-up: 0x%08X\n\n", val);
    }
  }

  receiver.resume();
}

// =============================================================
const char* lookupCommand(uint32_t frame2, const char** desc) {
  for (uint8_t i = 0; i < NUM_COMMANDS; i++) {
    if (KNOWN_COMMANDS[i].frame2 == frame2) {
      if (desc) *desc = KNOWN_COMMANDS[i].desc;
      return KNOWN_COMMANDS[i].name;
    }
  }
  return nullptr;
}