//client&relay program

#include <deque>
#include "painlessMesh.h"

std::deque<String> pendingQueue;
unsigned long lastRetry = 0;

#define   MESH_PREFIX     "SSID0"        
#define   MESH_PASSWORD   "nopassword"
#define   MESH_PORT       5555
#define   GATEWAY_ID      1239714569

Scheduler     userScheduler;
painlessMesh  mesh;

// --- Deklarasi fungsi ---
void sendMessage();
void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);

// --- Task untuk kirim pesan ---
Task taskSendMessage(TASK_SECOND * 3, TASK_FOREVER, &sendMessage);

// Fungsi kirim pesan broadcast
void sendMessage() {
  String payload = String(random(15,32));
  String packet;

  bool gatewayAvailable = mesh.isConnected(GATEWAY_ID);

  if (gatewayAvailable) {
    packet = "U|" + String(mesh.getNodeId()) + "|" + payload;
    mesh.sendSingle(GATEWAY_ID, packet);
    Serial.println("Unicast → Gateway");
  } else {
    packet = "B|" + String(mesh.getNodeId()) + "|" + payload;
    mesh.sendBroadcast(packet);
    Serial.println("Broadcast (Gateway unreachable)");
  }

  taskSendMessage.setInterval(2000);
}

struct NodeMap {
  uint32_t realID;
  String alias;
};

NodeMap nodes[] = {
  {1239714569, "Gateway"},
  {109870372, "nodeB"},
  {3657532460, "nodeC"}
};

void receivedCallback(uint32_t from, String &msg) {
  String name = "Unknown";
  for (auto &n : nodes) {
    if (n.realID == from) name = n.alias;
  }
  Serial.printf("[%s]: %s\n", name.c_str(), msg.c_str());

  bool gwConnected = mesh.isConnected(GATEWAY_ID);

  if (gwConnected) {
      mesh.sendSingle(GATEWAY_ID, msg);
      Serial.println("[Forward] → Gateway");
  } else {
      pendingQueue.push_back(msg);
      Serial.println("[Forward] Gateway unreachable, queued");
  }
}

void newConnectionCallback(uint32_t nodeId) {
  String name = "Unknown";
  for (auto &n : nodes) {
    if (n.realID == nodeId) name = n.alias;
  }
  Serial.printf("[CLIENT] Connected to %s (ID=%u)\n", name.c_str(), nodeId);
}

// Callback saat topologi berubah
void changedConnectionCallback() {
  Serial.println("[CLIENT] Connection list changed");
}

// Callback sinkronisasi waktu
void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("[CLIENT] Time adjusted. Offset = %d\n", offset);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\nStarting ESP8266 Mesh Client...");

  // Level debug minimal agar tidak banjir output
  mesh.setDebugMsgTypes(ERROR | STARTUP);

  // Inisialisasi mesh
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.setRoot(false);
  mesh.setContainsRoot(true);

  // Pasang callback
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  // Tambahkan dan aktifkan task pengirim pesan
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
}

void loop() {
  mesh.update();

  bool gwConnected = mesh.isConnected(GATEWAY_ID);

  if (gwConnected && !pendingQueue.empty()) {
    if (millis() - lastRetry > 500) {
      mesh.sendSingle(GATEWAY_ID, pendingQueue.front());
      pendingQueue.pop_front();
      lastRetry = millis();
      Serial.println("[Retry] Flushed a queued message");
    }
  }
}
