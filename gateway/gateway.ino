// =========================================================
// =============== GATEWAY / ROOT ESP32 ====================
// =========================================================

#include "painlessMesh.h"

#define   MESH_PREFIX     "SSID0"
#define   MESH_PASSWORD   "nopassword"
#define   MESH_PORT       5555

// ID Gateway (nodeId otomatis ditentukan hardware, ini hanya label map)
#define   GATEWAY_ID      1239714569

Scheduler     userScheduler;
painlessMesh  mesh;

// ---------------------------------------------------------
//               MAPPING NODE → NAMA
// ---------------------------------------------------------
struct NodeMap {
  uint32_t realID;
  String alias;
};

NodeMap nodes[] = {
  {1239714569, "Gateway"},
  {109870372,  "nodeB"},
  {3657532460, "nodeC"}
};

// ---------------------------------------------------------
//                     CALLBACK
// ---------------------------------------------------------
void receivedCallback(uint32_t from, String &msg) {
  String name = "Unknown";

  for (auto &n : nodes) {
    if (n.realID == from) name = n.alias;
  }

  Serial.printf("[RX FROM %s] %s\n", name.c_str(), msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  String name = "Unknown";
  for (auto &n : nodes) {
    if (n.realID == nodeId) name = n.alias;
  }

  Serial.printf("[NEW CONNECTION] %s (ID=%u)\n", name.c_str(), nodeId);
}

void changedConnectionCallback() {
  Serial.println("[TOPOLOGY] Connection list changed");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("[TIME SYNC] Offset=%d\n", offset);
}


// ---------------------------------------------------------
//                      SETUP
// ---------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n=== Starting ESP32 Mesh Gateway (ROOT) ===");

  // Reduce spam log
  mesh.setDebugMsgTypes(ERROR | STARTUP);

  // Init mesh
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);

  // Root mode
  mesh.setRoot(true);
  mesh.setContainsRoot(true);

  // Register callback
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  Serial.println("[SYSTEM] Gateway initialized.");
}


// ---------------------------------------------------------
//                      MAIN LOOP
// ---------------------------------------------------------
void loop() {
  mesh.update();
}
