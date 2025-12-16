#include "Connector.h"

Connector::Connector()
{
	this->mNetwork.type = CONNECTOR_TYPE_NONE;
	this->mNetwork.status = CONNECTOR_STATUS_NO_NETWORK;
	this->mNetwork.ssid[0] = '\0';
	this->mNetwork.password[0] = '\0';
	this->mNetwork.ip[0] = '\0';
	this->mNetwork.gateway[0] = '\0';
	this->mNetwork.mac[0] = '\0';

	this->mPsramEnabled = false;
	this->mLastMillis = 0;
	this->mEthernetClient = NULL;
	this->mWiFiClient = NULL;
	this->mMqttClient = NULL;
	this->mHttpClient = new HTTPClient();

	this->onConnect = NULL;
	this->onDisconnect = NULL;
	this->onMessage = NULL;
	this->onUnknownMessage = NULL;

	this->mConnection.host[0] = '\0';
	this->mConnection.defaultHost[0] = '\0';
	this->mConnection.port = CONNECTOR_PORT;
	this->mConnection.username[0] = '\0';
	this->mConnection.password[0] = '\0';
	this->mConnection.clientId[0] = '\0';
	this->mConnection.clientGroup[0] = '\0';

	this->mDescriptor.name[0] = '\0';
	this->mDescriptor.vendor[0] = '\0';
	this->mDescriptor.model[0] = '\0';
	this->mDescriptor.sn[0] = '\0';
}

Connector::~Connector()
{
	if (this->mEthernetClient)
		delete this->mEthernetClient;
	if (this->mWiFiClient)
		delete this->mWiFiClient;
	if (this->mMqttClient)
		delete this->mMqttClient;
	if (this->mHttpClient)
		delete this->mHttpClient;
	if (this->mMqttBuffer)
		free(this->mMqttBuffer);
}

void Connector::enablePsram()
{
	this->mPsramEnabled = true;
	this->mPublishMessage.enablePsram();
	this->mSubscribeMessage.enablePsram();
}

const char *Connector::getName()
{
	return this->mDescriptor.name;
}

const char *Connector::getVendor()
{
	return this->mDescriptor.vendor;
}

const char *Connector::getModel()
{
	return this->mDescriptor.model;
}

const char *Connector::getSerialNumber()
{
	return this->mDescriptor.sn;
}

const char *Connector::getAccessCode()
{
	return this->mDescriptor.accessCode;
}

void Connector::setDescriptor(const char *name, const char *vendor, const char *model, const char *sn, const char *accessCode)
{
	snprintf(this->mDescriptor.name, CONNECTOR_NAME_SIZE, name);
	snprintf(this->mDescriptor.vendor, CONNECTOR_VENDOR_SIZE, vendor);
	snprintf(this->mDescriptor.model, CONNECTOR_MODEL_SIZE, model);
	snprintf(this->mDescriptor.sn, CONNECTOR_SN_SIZE, sn);
	snprintf(this->mDescriptor.accessCode, CONNECTOR_ACCESS_CODE_SIZE, accessCode);
}

const char *Connector::getIPAddress()
{
	return this->mNetwork.ip;
}

const char *Connector::getGatewayAddress()
{
	return this->mNetwork.gateway;
}

const char *Connector::getMACAddress()
{
	return this->mNetwork.mac;
}

void Connector::setNetwork(uint8_t type)
{
	this->mNetwork.type = type;
}

void Connector::setNetwork(uint8_t type, const char *ssid, const char *password)
{
	this->mNetwork.type = type;
	strncpy(this->mNetwork.ssid, ssid, CONNECTOR_SSID_SIZE);
	strncpy(this->mNetwork.password, password, CONNECTOR_PASSWORD_SIZE);
}

void Connector::updateNetwork()
{
	if (this->mNetwork.type == CONNECTOR_TYPE_ETHERNET)
	{
		IPAddress ip = Ethernet.localIP();
		snprintf(this->mNetwork.ip, CONNECTOR_IP_ADDRESS_SIZE, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

		IPAddress gateway = Ethernet.gatewayIP();
		snprintf(this->mNetwork.gateway, CONNECTOR_IP_ADDRESS_SIZE, "%d.%d.%d.%d", gateway[0], gateway[1], gateway[2], gateway[3]);
		strncpy(this->mNetwork.mac, WiFi.macAddress().c_str(), CONNECTOR_MAC_SIZE);
		snprintf(this->mConnection.defaultHost, CONNECTOR_HOST_SIZE, "%d.%d.%d.%d", gateway[0], gateway[1], gateway[2], gateway[3] + 1);
	}
	else if (this->mNetwork.type == CONNECTOR_TYPE_WIFI)
	{
		IPAddress ip = WiFi.localIP();
		snprintf(this->mNetwork.ip, CONNECTOR_IP_ADDRESS_SIZE, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

		IPAddress gateway = WiFi.gatewayIP();
		snprintf(this->mNetwork.gateway, CONNECTOR_IP_ADDRESS_SIZE, "%d.%d.%d.%d", gateway[0], gateway[1], gateway[2], gateway[3]);
		strncpy(this->mNetwork.mac, WiFi.macAddress().c_str(), CONNECTOR_MAC_SIZE);
		snprintf(this->mConnection.defaultHost, CONNECTOR_HOST_SIZE, "%d.%d.%d.%d", gateway[0], gateway[1], gateway[2], gateway[3] + 1);
	}
	else
	{
		snprintf(this->mNetwork.ip, CONNECTOR_IP_ADDRESS_SIZE, "0.0.0.0");
		snprintf(this->mNetwork.gateway, CONNECTOR_IP_ADDRESS_SIZE, "0.0.0.0");
		snprintf(this->mNetwork.mac, CONNECTOR_MAC_SIZE, "00:00:00:00:00:00");
		snprintf(this->mConnection.defaultHost, CONNECTOR_HOST_SIZE, "0.0.0.0");
	}
}

const char *Connector::getClientGroup()
{
	return this->mConnection.clientGroup;
}

const char *Connector::getClientId()
{
	return this->mConnection.clientId;
}

void Connector::setConnection(const char *host, uint16_t port)
{
	snprintf(this->mConnection.host, CONNECTOR_HOST_SIZE, host);
	this->mConnection.port = port;
}

void Connector::setConnection(const char *host, uint16_t port, const char *username, const char *password)
{
	snprintf(this->mConnection.host, CONNECTOR_HOST_SIZE, host);
	this->mConnection.port = port;
	snprintf(this->mConnection.username, CONNECTOR_HOST_SIZE, username);
	snprintf(this->mConnection.password, CONNECTOR_HOST_SIZE, password);
}

bool Connector::waitForEthernetAvailable(uint8_t seconds)
{
	uint32_t t = 0;
	byte macAddress[6] = {
			0,
	};
	WiFi.macAddress(macAddress);
	while (Ethernet.begin(macAddress) == 0)
	{
		yield();
		if (t < (seconds * 10))
		{
			delay(100);
		}
		else
		{
			return false;
		}
		t++;
	}
	this->updateNetwork();
	return true;
}

bool Connector::waitForWiFiAvailable(uint8_t seconds)
{
	uint32_t t = 0;
	while (WiFi.status() != WL_CONNECTED)
	{
		yield();
		if (t < (seconds * 10))
		{
			delay(100);
		}
		else
		{
			return false;
		}
		t++;
	}
	this->updateNetwork();
	return true;
}

bool Connector::subscribe(const char *topic)
{
	return this->subscribe(topic, 0);
}

bool Connector::subscribe(const char *topic, uint8_t qos)
{
	if (this->mMqttClient && this->mNetwork.status == CONNECTOR_STATUS_CONNECTED)
	{
		return this->mMqttClient->subscribe(topic, qos);
	}
	return false;
}

bool Connector::unsubscribe(const char *topic)
{
	if (this->mMqttClient && this->mNetwork.status == CONNECTOR_STATUS_CONNECTED)
	{
		return this->mMqttClient->unsubscribe(topic);
	}
	return false;
}

bool Connector::publish(const char *topic, Message *msg, bool retain)
{
	if (this->mMqttClient && this->mNetwork.status == CONNECTOR_STATUS_CONNECTED)
	{
		uint32_t size = msg->toPayload(this->mMqttBuffer, sizeof(this->mMqttBuffer));
		return this->mMqttClient->publish(topic, this->mMqttBuffer, size, retain);
	}
	return false;
}

bool Connector::publish(const char *topic, const char *dataType, const char *format, ...)
{
	if (this->mMqttClient && this->mNetwork.status == CONNECTOR_STATUS_CONNECTED)
	{
		this->mPublishMessage.reset();
		this->mPublishMessage.version = MESSAGE_VERSION;
		this->mPublishMessage.type = MESSAGE_TYPE_VALUE;
		this->mPublishMessage.setLastModified();
		this->mPublishMessage.setDataType(dataType);

		va_list va;
		va_start(va, format);
		vsnprintf(this->mMessageBuffer, MESSAGE_BUFFER_SIZE, format, va);
		va_end(va);

		uint32_t length = strlen(this->mMessageBuffer);
		this->mPublishMessage.setData((uint8_t *)this->mMessageBuffer, length);

		uint32_t size = this->mPublishMessage.toPayload(this->mMqttBuffer, sizeof(this->mMqttBuffer));
		return this->mMqttClient->publish(topic, this->mMqttBuffer, size, false);
	}
	return false;
}

bool Connector::publish(const char *topic, const char *dataType, uint8_t *data, uint32_t dataSize)
{
	if (this->mMqttClient && this->mNetwork.status == CONNECTOR_STATUS_CONNECTED)
	{
		this->mPublishMessage.reset();
		this->mPublishMessage.version = MESSAGE_VERSION;
		this->mPublishMessage.type = MESSAGE_TYPE_VALUE;
		this->mPublishMessage.setLastModified();
		this->mPublishMessage.setDataType(dataType);
		this->mPublishMessage.setData(data, dataSize);

		uint32_t size = this->mPublishMessage.toPayload(this->mMqttBuffer, sizeof(this->mMqttBuffer));
		return this->mMqttClient->publish(topic, this->mMqttBuffer, size, false);
	}
	return false;
}

bool Connector::publishJSON(const char *topic, const char *format, ...)
{
	if (this->mMqttClient && this->mNetwork.status == CONNECTOR_STATUS_CONNECTED)
	{
		this->mPublishMessage.reset();
		this->mPublishMessage.version = MESSAGE_VERSION;
		this->mPublishMessage.type = MESSAGE_TYPE_VALUE;
		this->mPublishMessage.setLastModified();
		this->mPublishMessage.setDataType("application/json");

		va_list va;
		va_start(va, format);
		vsnprintf(this->mMessageBuffer, MESSAGE_BUFFER_SIZE, format, va);
		va_end(va);

		uint32_t length = strlen(this->mMessageBuffer);
		this->mPublishMessage.setData((uint8_t *)this->mMessageBuffer, length);

		uint32_t size = this->mPublishMessage.toPayload(this->mMqttBuffer, sizeof(this->mMqttBuffer));
		return this->mMqttClient->publish(topic, this->mMqttBuffer, size, false);
	}
	return false;
}

void Connector::close()
{
	if (this->mMqttClient != NULL)
	{
		this->mMqttClient->disconnect();
	}
}

void Connector::dispatchMessage(char *topic, uint8_t *payload, unsigned int size)
{
	if (this->mSubscribeMessage.fromPayload(payload, size))
	{
		if (this->onMessage != NULL)
		{
			this->onMessage(this, topic, &this->mSubscribeMessage);
		}
	}
	else
	{
		if (this->onUnknownMessage != NULL)
		{
			this->onUnknownMessage(this, topic, &this->mSubscribeMessage);
		}
	}
}

void Connector::setOnConnectCallback(CONNECTOR_CALLBACK_CONNECT)
{
	this->onConnect = onConnect;
}

void Connector::setOnDisconnectCallback(CONNECTOR_CALLBACK_DISCONNECT)
{
	this->onDisconnect = onDisconnect;
}

void Connector::setOnMessageCallback(CONNECTOR_CALLBACK_MESSAGE)
{
	this->onMessage = onMessage;
}

void Connector::setOnUnknownMessageCallback(CONNECTOR_CALLBACK_UNKNOWN_MESSAGE)
{
	this->onUnknownMessage = onUnknownMessage;
}

bool Connector::begin()
{

	this->mNetwork.status = CONNECTOR_STATUS_NO_NETWORK;

	if (this->mNetwork.type == CONNECTOR_TYPE_ETHERNET)
	{
		if (this->waitForEthernetAvailable(CONNECTOR_TIMEOUT))
		{
			this->mNetwork.status = CONNECTOR_STATUS_DISCONNECTED;
		}
		this->mEthernetClient = new EthernetClient();
		this->mMqttClient = new MqttClient(*this->mEthernetClient);
	}
	else if (this->mNetwork.type == CONNECTOR_TYPE_WIFI)
	{
		WiFi.mode(WIFI_STA);
		WiFi.setAutoReconnect(false);
		WiFi.begin(this->mNetwork.ssid, this->mNetwork.password);
		if (this->waitForWiFiAvailable(CONNECTOR_TIMEOUT))
		{
			this->mNetwork.status = CONNECTOR_STATUS_DISCONNECTED;
		}
		this->mWiFiClient = new WiFiClient();
		this->mMqttClient = new MqttClient(*this->mWiFiClient);
	}
	else
	{
		return true;
	}

	if (this->mPsramEnabled)
	{
		if (psramInit())
		{
			this->mMqttClient->setBufferSize(CONNECTOR_MQTT_PSRAM_BUFFER_SIZE, true);
			this->mMqttBuffer = (uint8_t *)ps_malloc(CONNECTOR_MQTT_PSRAM_BUFFER_SIZE);
		}
		else
		{
			this->mMqttClient->setBufferSize(CONNECTOR_MQTT_BUFFER_SIZE);
			this->mMqttBuffer = (uint8_t *)malloc(CONNECTOR_MQTT_BUFFER_SIZE);
		}
	}
	else
	{
		this->mMqttClient->setBufferSize(CONNECTOR_MQTT_BUFFER_SIZE);
		this->mMqttBuffer = (uint8_t *)malloc(CONNECTOR_MQTT_BUFFER_SIZE);
	}

	MQTT_CALLBACK_SIGNATURE = [=](char *topic, uint8_t *payload, unsigned int length)
	{
		this->dispatchMessage(topic, payload, length);
	};
	this->mMqttClient->setCallback(callback);
	return true;
}

bool Connector::loop()
{
	return this->loop(0);
}

bool Connector::loop(unsigned long intervals)
{

	if (this->mNetwork.type == CONNECTOR_TYPE_ETHERNET)
	{
		if (Ethernet.linkStatus() == LinkON)
		{
			if (this->mNetwork.status == CONNECTOR_STATUS_NO_NETWORK)
			{
				this->mNetwork.status = CONNECTOR_STATUS_DISCONNECTED;
			}
			Ethernet.maintain();
		}
		else
		{
			this->mNetwork.status = CONNECTOR_STATUS_NO_NETWORK;
			if (this->waitForEthernetAvailable(CONNECTOR_TIMEOUT))
			{
				this->mNetwork.status = CONNECTOR_STATUS_DISCONNECTED;
			}
		}
	}
	else if (this->mNetwork.type == CONNECTOR_TYPE_WIFI)
	{
		if (WiFi.status() == WL_CONNECTED)
		{
			if (this->mNetwork.status == CONNECTOR_STATUS_NO_NETWORK)
			{
				this->mNetwork.status = CONNECTOR_STATUS_DISCONNECTED;
			}
		}
		else
		{
			this->mNetwork.status = CONNECTOR_STATUS_NO_NETWORK;
			WiFi.reconnect();
			if (this->waitForWiFiAvailable(CONNECTOR_TIMEOUT))
			{
				this->mNetwork.status = CONNECTOR_STATUS_DISCONNECTED;
			}
		}
	}

	if (this->mMqttClient != NULL && this->mNetwork.status != CONNECTOR_STATUS_NO_NETWORK)
	{
		if (this->mMqttClient->connected())
		{
			this->mNetwork.status = CONNECTOR_STATUS_CONNECTED;
		}
		else
		{
			this->mNetwork.status = CONNECTOR_STATUS_DISCONNECTED;
			if (this->onDisconnect != NULL)
			{
				this->onDisconnect(this);
			}

			if (strlen(this->mConnection.host) > 0)
			{
				this->mMqttClient->setServer(this->mConnection.host, this->mConnection.port);
			}
			else
			{
				// Network Plug & Play
				this->mMqttClient->setServer(this->mConnection.defaultHost, this->mConnection.port);
			}

			bool success = false;

			if (strlen(this->mConnection.clientId) == 0)
			{
				// Use Descriptors
				snprintf(this->mConnection.clientGroup, CONNECTOR_CLIENT_GROUP_SIZE, "device/%s", this->mDescriptor.name);
				snprintf(this->mConnection.clientId, CONNECTOR_CLIENT_ID_SIZE, "device/%s/%s", this->mDescriptor.name, this->mDescriptor.sn);
				strncpy(this->mConnection.username, this->mDescriptor.model, CONNECTOR_MODEL_SIZE);
				strncpy(this->mConnection.password, this->mDescriptor.accessCode, CONNECTOR_ACCESS_CODE_SIZE);
			}

			if (strlen(this->mConnection.username) > 0 && strlen(this->mConnection.password) > 0)
			{
				success = this->mMqttClient->connect(this->mConnection.clientId, this->mConnection.username, this->mConnection.password);
			}
			else
			{
				success = this->mMqttClient->connect(this->mConnection.clientId);
			}

			if (success)
			{
				this->mNetwork.status = CONNECTOR_STATUS_CONNECTED;
				if (this->onConnect != NULL)
				{
					this->onConnect(this);
				}
			}
		}
		this->mMqttClient->loop();
	}

	if (intervals > 0)
	{
		unsigned long now = millis();
		if (this->mLastMillis + intervals < now)
		{
			this->mLastMillis = now;
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return true;
	}
}

bool Connector::OTA(const char *url)
{
	if (this->mNetwork.status != CONNECTOR_STATUS_NO_NETWORK)
	{
		this->mHttpClient->begin(url);
		int responseCode = this->mHttpClient->GET();
		if (responseCode == HTTP_CODE_OK)
		{
			int contentLength = this->mHttpClient->getSize();
			bool started = Update.begin(contentLength);
			if (started)
			{
				WiFiClient &client = this->mHttpClient->getStream();
				size_t written = Update.writeStream(client);
				if (written == contentLength && Update.end())
				{
					if (Update.isFinished())
					{
						ESP.restart();
					}
				}
			}
		}
		this->mHttpClient->end();
	}
	return false;
}