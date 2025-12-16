#ifndef CONNECTOR_H_
#define CONNECTOR_H_

#include <Arduino.h>
#include <functional>
#include "time.h"
#include <sys/time.h>
#include <WiFi.h>
#include <UIPEthernet.h>
#include <HTTPClient.h>
#include <Update.h>
#include "MqttClient.h"
#include "Message.h"

#define CONNECTOR_NAME_SIZE 64
#define CONNECTOR_VENDOR_SIZE 64
#define CONNECTOR_MODEL_SIZE 64
#define CONNECTOR_SN_SIZE 64

#define CONNECTOR_TYPE_NONE 0
#define CONNECTOR_TYPE_ETHERNET 1
#define CONNECTOR_TYPE_WIFI 2

#define CONNECTOR_SSID_SIZE 64
#define CONNECTOR_PASSWORD_SIZE 64
#define CONNECTOR_IP_ADDRESS_SIZE 32
#define CONNECTOR_MAC_SIZE 32
#define CONNECTOR_TIMEOUT 3 // seconds

#define CONNECTOR_HOST_SIZE 64
#define CONNECTOR_PORT 16300
#define CONNECTOR_CLIENT_ID_SIZE 128
#define CONNECTOR_CLIENT_GROUP_SIZE 128
#define CONNECTOR_ACCESS_CODE_SIZE 64

#define CONNECTOR_STATUS_NO_NETWORK -1
#define CONNECTOR_STATUS_DISCONNECTED 0
#define CONNECTOR_STATUS_CONNECTED 1

#define CONNECTOR_MQTT_BUFFER_SIZE 4096
#define CONNECTOR_MQTT_PSRAM_BUFFER_SIZE 1048000

#define CONNECTOR_CALLBACK_CONNECT std::function<void(Connector *)> onConnect
#define CONNECTOR_CALLBACK_DISCONNECT std::function<void(Connector *)> onDisconnect
#define CONNECTOR_CALLBACK_MESSAGE std::function<void(Connector *, const char *, Message *)> onMessage
#define CONNECTOR_CALLBACK_UNKNOWN_MESSAGE std::function<void(Connector *, const char *, Message *)> onUnknownMessage

struct Descriptor
{
	char name[CONNECTOR_NAME_SIZE];
	char vendor[CONNECTOR_VENDOR_SIZE];
	char model[CONNECTOR_MODEL_SIZE];
	char sn[CONNECTOR_SN_SIZE];
	char accessCode[CONNECTOR_ACCESS_CODE_SIZE];
};

typedef struct
{
	uint8_t type;
	int8_t status;
	char ssid[CONNECTOR_SSID_SIZE];
	char password[CONNECTOR_PASSWORD_SIZE];
	char ip[CONNECTOR_IP_ADDRESS_SIZE];
	char gateway[CONNECTOR_IP_ADDRESS_SIZE];
	char mac[CONNECTOR_MAC_SIZE];
} AG_Network;

struct Connection
{
	char host[CONNECTOR_HOST_SIZE];
	char defaultHost[CONNECTOR_HOST_SIZE]; // Network Plug & Play
	uint16_t port;
	char username[CONNECTOR_MODEL_SIZE];			 // Model
	char password[CONNECTOR_ACCESS_CODE_SIZE]; // Access Code
	char clientGroup[CONNECTOR_CLIENT_GROUP_SIZE];
	char clientId[CONNECTOR_CLIENT_ID_SIZE];
};

class Connector
{
private:
	Descriptor mDescriptor;
	Connection mConnection;
	AG_Network mNetwork;
	bool mPsramEnabled;
	unsigned long mLastMillis;

	uint8_t *mMqttBuffer;
	char mMessageBuffer[MESSAGE_BUFFER_SIZE];

	EthernetClient *mEthernetClient;
	WiFiClient *mWiFiClient;
	MqttClient *mMqttClient;
	HTTPClient *mHttpClient;
	Message mPublishMessage;
	Message mSubscribeMessage;

	CONNECTOR_CALLBACK_CONNECT;
	CONNECTOR_CALLBACK_DISCONNECT;
	CONNECTOR_CALLBACK_MESSAGE;
	CONNECTOR_CALLBACK_UNKNOWN_MESSAGE;

public:
	Connector();
	~Connector();

	void enablePsram();

	const char *getName();
	const char *getVendor();
	const char *getModel();
	const char *getSerialNumber();
	const char *getAccessCode();

	void setDescriptor(const char *name, const char *vendor, const char *model, const char *sn, const char *accessCode);

	const char *getIPAddress();
	const char *getGatewayAddress();
	const char *getMACAddress();

	void setNetwork(uint8_t type);
	void setNetwork(uint8_t type, const char *ssid, const char *password);
	void updateNetwork();

	const char *getClientGroup();
	const char *getClientId();

	void setConnection(const char *host, uint16_t port);
	void setConnection(const char *host, uint16_t port, const char *username, const char *password);

	bool waitForEthernetAvailable(uint8_t seconds);
	bool waitForWiFiAvailable(uint8_t seconds);

	bool subscribe(const char *topic);
	bool subscribe(const char *topic, uint8_t qos);
	bool unsubscribe(const char *topic);
	bool publish(const char *topic, Message *msg, bool retain);
	bool publish(const char *topic, const char *dataType, const char *format, ...);
	bool publish(const char *topic, const char *dataType, uint8_t *data, uint32_t dataSize);
	bool publishJSON(const char *topic, const char *format, ...);
	void close();

	void notifyStatus();

	void dispatchMessage(char *topic, uint8_t *payload, unsigned int size);
	void setOnConnectCallback(CONNECTOR_CALLBACK_CONNECT);
	void setOnDisconnectCallback(CONNECTOR_CALLBACK_DISCONNECT);
	void setOnMessageCallback(CONNECTOR_CALLBACK_MESSAGE);
	void setOnUnknownMessageCallback(CONNECTOR_CALLBACK_UNKNOWN_MESSAGE);

	bool begin();
	bool loop();
	bool loop(unsigned long intervals);

	bool OTA(const char *url);
};

#endif
