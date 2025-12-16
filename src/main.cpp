#include "Connector.h"

// #define CLIENT_ID		"device/ag.phri.sensor200/44JI19"
#define CLIENT_ID "device/com.daon.soriESP32/950409"
#define BROKER_SERVER "192.168.0.132"
#define BROKER_PORT 16300

#define TOPIC(y) CLIENT_ID y

Connector CON; // MQTT 연결 및 통신을 담당하는 커넥터 인스턴스

void onConnect(Connector *c);
void onMessage(Connector *c, const char *topic, Message *msg);

void onConnect(Connector *c) // 연결 성공 시 호출되는 콜백 함수
{
	c->subscribe(c->getClientGroup(), 0); // 클라이언트 그룹 구독
	c->subscribe(TOPIC("/notify/#"), 0);	// 알림 토픽 구독

	Serial.println("[Alert] Connected to MQTT broker.");
	Serial.print("Broker: ");
	Serial.println(BROKER_SERVER);
	Serial.print("Port: ");
	Serial.println(BROKER_PORT);
	Serial.println();
}

void onMessage(Connector *c, const char *topic, Message *msg) // 메시지 수신 시 호출되는 콜백 함수
{
	Serial.println("--------------------------------");
	Serial.println("[Alert] Message received.");
	Serial.println("Received message: ");
	Serial.print("Topic: ");
	Serial.println(topic);

	// 원시 데이터로 직접 확인
	uint8_t *data = msg->getData();
	uint32_t size = msg->getSize();

	Serial.print("Raw Payload: ");
	if (data != NULL && size > 0)
	{
		for (uint32_t i = 0; i < size; i++)
		{
			Serial.print((char)data[i]);
		}
		Serial.println();

		// 16진수로도 확인
		// Serial.print("[DEBUG] Hex: ");
		// for (uint32_t i = 0; i < size && i < 20; i++)
		// {
		// 	Serial.printf("%02X ", data[i]);
		// }
		// Serial.println();
	}
	else
	{
		Serial.println();
		Serial.println("[DEBUG] No Payload...");
	}

	Serial.print("Payload Size: ");
	Serial.println(size);

	if (strcmp(topic, c->getClientGroup()) == 0) // 클라이언트 그룹 토픽인 경우
	{
		// 디바이스 상태 정보 발행
		c->publishJSON(TOPIC("/status"),
									 "{\"name\":\"%s\",\"vendor\":\"%s\",\"model\":\"%s\",\"sn\":\"%s\",\"ip\":\"%s\"}",
									 c->getName(),
									 c->getVendor(),
									 c->getModel(),
									 c->getSerialNumber(),
									 c->getIPAddress());
	}
	else if (strcmp(topic, TOPIC("/notify/public")) == 0) // public 토픽인 경우
	{
		// 센서 정보 발행
		c->publishJSON(TOPIC("/status"),
									 "{\"DT\":%1.f,\"RH\":%1.f}",
									 25.4,
									 56.2);
	}
}

void setup()
{
	Serial.begin(115200);

	// CON.setDescriptor("ag.phri.sensor200", "PHRI", "Sensor200", "44JI19", "123456");
	CON.setDescriptor("com.daon.soriESP32", "DAON", "SoriESP32", "950409", "123456");

	// CON.setNetwork(CONNECTOR_TYPE_ETHERNET);

	// CON.setNetwork(CONNECTOR_TYPE_WIFI, "PHRI-WIFI", "123456");
	CON.setNetwork(CONNECTOR_TYPE_WIFI, "dinfo", "daon7521");

	CON.setConnection(BROKER_SERVER, BROKER_PORT);

	// MAC 주소 출력
	uint8_t mac_HW[6];
	esp_efuse_mac_get_default(mac_HW);
	Serial.printf("HW MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
								mac_HW[0], mac_HW[1], mac_HW[2], mac_HW[3], mac_HW[4], mac_HW[5]);

	CON.setOnConnectCallback(onConnect);
	CON.setOnMessageCallback(onMessage);
	CON.begin();
}

void loop()
{
	CON.loop();
}
