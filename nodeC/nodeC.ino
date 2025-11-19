//nodeC program
#include "painlessMesh.h"

#define   MESH_PREFIX     "SSID0"        
#define   MESH_PASSWORD   "nopassword"
#define   MESH_PORT       5555
#define   GATEWAY_ID      0000000

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

  taskSendMessage.setInterval(5000);
}

struct NodeMap {
  uint32_t realID;
  String alias;
};

NodeMap nodes[] = {
  {0000000000, "Gateway"},
  {109870372, "nodeB"},
  {3657532460, "nodeC"}
};

void receivedCallback(uint32_t from, String &msg) {
  String name = "Unknown";
  for (auto &n : nodes) {
    if (n.realID == from) name = n.alias;
  }
  Serial.printf("[%s]: %s\n",name.c_str(),msg.c_str());

  bool gwConnected = mesh.isConnected(GATEWAY_ID);

  if (gwConnected){
    mesh.sendSingle(GATEWAY_ID, msg);
    Serial.println("[CLIENT] Data forwarded!");
  } else {
    Serial.println("Gateway unreachable...");
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
}
