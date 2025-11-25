// =========================================================
// ================= nodeC / RELAY ESP8266 =================
// =========================================================
#include "painlessMesh.h"

#define   MESH_PREFIX     "SSID0"
#define   MESH_PASSWORD   "nopassword"
#define   MESH_PORT       5555
#define   GATEWAY_ID      1239714569   // ID Gateway (root)

Scheduler     userScheduler;
painlessMesh  mesh;

// ---------------------------------------------------------
//                   IDENTITAS NODE
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
//                TASK KIRIM PESAN TIAP 2s
// ---------------------------------------------------------
void sendMessage() {
  String payload = String(random(14, 32));
  String packet;

  bool gatewayAvailable = mesh.isConnected(GATEWAY_ID);

  if (gatewayAvailable) {
    // Unicast ketika gateway reachable
    packet = "U|" + String(mesh.getNodeId()) + "|" + payload;
    mesh.sendSingle(GATEWAY_ID, packet);
    Serial.printf("[TX] Unicast to Gateway: %s\n", packet.c_str());
  } else {
    // Broadcast ketika gateway unreachable (biar node lain bisa forward)
    packet = "B|" + String(mesh.getNodeId()) + "|" + payload;
    mesh.sendSingle(109870372,packet);
    Serial.printf("[TX] Broadcast (gateway unreachable): %s\n", packet.c_str());
  }
}

Task taskSendMessage(TASK_SECOND * 2, TASK_FOREVER, &sendMessage);


// ---------------------------------------------------------
//                  CALLBACK: RECEIVE MESSAGE
// ---------------------------------------------------------
void receivedCallback(uint32_t from, String &msg) {
  String name = "Unknown";

  for (auto &n : nodes) {
    if (n.realID == from) name = n.alias;
  }

  Serial.printf("[RX from %s] %s\n", name.c_str(), msg.c_str());

  bool gwConnected = mesh.isConnected(GATEWAY_ID);

  if (gwConnected) {
    mesh.sendSingle(GATEWAY_ID, msg);
    Serial.println("[FORWARD] → Gateway");
  } else {
    Serial.println("[FORWARD] Gateway unreachable");
  }
}

// ---------------------------------------------------------
//             CALLBACK CHANGE CONNECTION / NEW NODE
// ---------------------------------------------------------
void newConnectionCallback(uint32_t nodeId) {
  String name = "Unknown";
  for (auto &n : nodes) if (n.realID == nodeId) name = n.alias;

  Serial.printf("[NEW CONNECTION] %s (ID=%u)\n", name.c_str(), nodeId);
}

void changedConnectionCallback() {
  Serial.println("[TOPOLOGY] Connection list changed");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("[TIME SYNC] Offset=%d\n", offset);
}


// ---------------------------------------------------------
//                           SETUP
// ---------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n=== Starting ESP8266 Client/Relay Node ===");

  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(
    MESH_PREFIX,
    MESH_PASSWORD,
    &userScheduler,
    MESH_PORT
);

  mesh.setRoot(false);
  mesh.setContainsRoot(true);

  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
}


// ---------------------------------------------------------
//                           LOOP
// ---------------------------------------------------------
void loop() {
  mesh.update();
}
