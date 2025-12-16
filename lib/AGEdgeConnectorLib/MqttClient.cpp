#include "MqttClient.h"

MqttClient::MqttClient()
{
    this->_state = MQTT_DISCONNECTED;
    this->_client = NULL;
    this->stream = NULL;
    setCallback(NULL);
    this->bufferSize = 0;
    setKeepAlive(MQTT_KEEPALIVE);
    setSocketTimeout(MQTT_SOCKET_TIMEOUT);
    this->mReadTimeoutEnabled = true;
}

MqttClient::MqttClient(Client &client)
{
    this->_state = MQTT_DISCONNECTED;
    setClient(client);
    this->stream = NULL;
    this->bufferSize = 0;
    setKeepAlive(MQTT_KEEPALIVE);
    setSocketTimeout(MQTT_SOCKET_TIMEOUT);
    this->mReadTimeoutEnabled = true;
}

MqttClient::~MqttClient()
{
    free(this->buffer);
}

boolean MqttClient::connect(const char *id)
{
    return connect(id, NULL, NULL, 0, 0, 0, 0, 1);
}

boolean MqttClient::connect(const char *id, const char *user, const char *pass)
{
    return connect(id, user, pass, 0, 0, 0, 0, 1);
}

boolean MqttClient::connect(const char *id, const char *willTopic, uint8_t willQos, boolean willRetain, const char *willMessage)
{
    return connect(id, NULL, NULL, willTopic, willQos, willRetain, willMessage, 1);
}

boolean MqttClient::connect(const char *id, const char *user, const char *pass, const char *willTopic, uint8_t willQos, boolean willRetain, const char *willMessage)
{
    return connect(id, user, pass, willTopic, willQos, willRetain, willMessage, 1);
}

boolean MqttClient::connect(const char *id, const char *user, const char *pass, const char *willTopic, uint8_t willQos, boolean willRetain, const char *willMessage, boolean cleanSession)
{
    if (!connected())
    {
        int result = 0;
        if (_client->connected())
        {
            result = 1;
        }
        else
        {
            if (domain != NULL)
            {
                result = _client->connect(this->domain, this->port);
            }
            else
            {
                result = _client->connect(this->ip, this->port);
            }
        }
        if (result == 1)
        {
            nextMsgId = 1;
            uint32_t length = MQTT_MAX_HEADER_SIZE;
            unsigned int j;

            uint8_t d[7] = {0x00, 0x04, 'M', 'Q', 'T', 'T', MQTT_VERSION};
            for (j = 0; j < MQTT_HEADER_VERSION_LENGTH; j++)
            {
                this->buffer[length++] = d[j];
            }

            uint8_t v;
            if (willTopic)
            {
                v = 0x04 | (willQos << 3) | (willRetain << 5);
            }
            else
            {
                v = 0x00;
            }
            if (cleanSession)
            {
                v = v | 0x02;
            }

            if (user != NULL)
            {
                v = v | 0x80;

                if (pass != NULL)
                {
                    v = v | (0x80 >> 1);
                }
            }
            this->buffer[length++] = v;

            this->buffer[length++] = ((this->keepAlive) >> 8);
            this->buffer[length++] = ((this->keepAlive) & 0xFF);

            CHECK_STRING_LENGTH(length, id)
            length = writeString(id, this->buffer, length);
            if (willTopic)
            {
                CHECK_STRING_LENGTH(length, willTopic)
                length = writeString(willTopic, this->buffer, length);
                CHECK_STRING_LENGTH(length, willMessage)
                length = writeString(willMessage, this->buffer, length);
            }

            if (user != NULL)
            {
                CHECK_STRING_LENGTH(length, user)
                length = writeString(user, this->buffer, length);
                if (pass != NULL)
                {
                    CHECK_STRING_LENGTH(length, pass)
                    length = writeString(pass, this->buffer, length);
                }
            }

            write(MQTTCONNECT, this->buffer, length - MQTT_MAX_HEADER_SIZE);

            lastInActivity = lastOutActivity = millis();

            while (!_client->available())
            {
                unsigned long t = millis();
                if (t - lastInActivity >= ((int32_t)this->socketTimeout * 1000UL))
                {
                    _state = MQTT_CONNECTION_TIMEOUT;
                    _client->stop();
                    return false;
                }
            }
            uint8_t llen;
            uint32_t len = readPacket(&llen);

            if (len == 4)
            {
                if (buffer[3] == 0)
                {
                    lastInActivity = millis();
                    pingOutstanding = false;
                    _state = MQTT_CONNECTED;
                    return true;
                }
                else
                {
                    _state = buffer[3];
                }
            }
            _client->stop();
        }
        else
        {
            _state = MQTT_CONNECT_FAILED;
        }
        return false;
    }
    return true;
}

boolean MqttClient::readByte(uint8_t *result)
{

    if (this->mReadTimeoutEnabled)
    {
        uint32_t previousMillis = millis();
        while (!_client->available())
        {
            yield();
            uint32_t currentMillis = millis();
            if (currentMillis - previousMillis >= ((int32_t)this->socketTimeout * 1000))
            {
                return false;
            }
        }
    }
    else
    {
        if (_client->available() == 0)
        {
            return false;
        }
    }
    *result = _client->read();
    return true;
}

boolean MqttClient::readByte(uint8_t *result, uint32_t *index)
{
    uint32_t current_index = *index;
    uint8_t *write_address = &(result[current_index]);
    if (readByte(write_address))
    {
        *index = current_index + 1;
        return true;
    }
    return false;
}

uint32_t MqttClient::readPacket(uint8_t *lengthLength)
{
    uint32_t len = 0;
    if (!readByte(this->buffer, &len))
        return 0;

    bool isPublish = (this->buffer[0] & 0xF0) == MQTTPUBLISH;
    uint32_t multiplier = 1;
    uint32_t length = 0;
    uint8_t digit = 0;
    uint32_t skip = 0;
    uint32_t start = 0;

    do
    {
        if (len == 5)
        {
            _state = MQTT_DISCONNECTED;
            _client->stop();
            return 0;
        }
        if (!readByte(&digit))
            return 0;
        this->buffer[len++] = digit;
        length += (digit & 127) * multiplier;
        multiplier <<= 7;
    } while ((digit & 128) != 0);
    *lengthLength = len - 1;

    if (isPublish)
    {
        if (!readByte(this->buffer, &len))
            return 0;
        if (!readByte(this->buffer, &len))
            return 0;
        skip = (this->buffer[*lengthLength + 1] << 8) + this->buffer[*lengthLength + 2];
        start = 2;
        if (this->buffer[0] & MQTTQOS1)
        {
            // skip message id
            skip += 2;
        }
    }
    uint32_t idx = len;

    for (uint32_t i = start; i < length; i++)
    {
        if (!readByte(&digit))
            return 0;
        if (this->stream)
        {
            if (isPublish && idx - *lengthLength - 2 > skip)
            {
                this->stream->write(digit);
            }
        }

        if (len < this->bufferSize)
        {
            this->buffer[len] = digit;
            len++;
        }
        idx++;
    }

    if (!this->stream && idx > this->bufferSize)
    {
        len = 0;
    }
    return len;
}

boolean MqttClient::loop()
{
    if (connected())
    {
        unsigned long t = millis();
        if ((t - lastInActivity > this->keepAlive * 1000UL) || (t - lastOutActivity > this->keepAlive * 1000UL))
        {
            if (pingOutstanding)
            {
                this->_state = MQTT_CONNECTION_TIMEOUT;
                _client->stop();
                return false;
            }
            else
            {
                this->buffer[0] = MQTTPINGREQ;
                this->buffer[1] = 0;
                _client->write(this->buffer, 2);
                lastOutActivity = t;
                lastInActivity = t;
                pingOutstanding = true;
            }
        }
        if (_client->available())
        {
            uint8_t llen;
            uint32_t len = readPacket(&llen);
            uint16_t msgId = 0;
            uint8_t *payload;
            if (len > 0)
            {
                lastInActivity = t;
                uint8_t type = this->buffer[0] & 0xF0;
                if (type == MQTTPUBLISH)
                {
                    if (callback)
                    {
                        uint32_t tl = (this->buffer[llen + 1] << 8) + this->buffer[llen + 2];
                        memmove(this->buffer + llen + 2, this->buffer + llen + 3, tl);
                        this->buffer[llen + 2 + tl] = 0;
                        char *topic = (char *)this->buffer + llen + 2;
                        if ((this->buffer[0] & 0x06) == MQTTQOS1)
                        {
                            msgId = (this->buffer[llen + 3 + tl] << 8) + this->buffer[llen + 3 + tl + 1];
                            payload = this->buffer + llen + 3 + tl + 2;
                            callback(topic, payload, len - llen - 3 - tl - 2);

                            this->buffer[0] = MQTTPUBACK;
                            this->buffer[1] = 2;
                            this->buffer[2] = (msgId >> 8);
                            this->buffer[3] = (msgId & 0xFF);
                            _client->write(this->buffer, 4);
                            lastOutActivity = t;
                        }
                        else
                        {
                            payload = this->buffer + llen + 3 + tl;
                            callback(topic, payload, len - llen - 3 - tl);
                        }
                    }
                }
                else if (type == MQTTPINGREQ)
                {
                    this->buffer[0] = MQTTPINGRESP;
                    this->buffer[1] = 0;
                    _client->write(this->buffer, 2);
                }
                else if (type == MQTTPINGRESP)
                {
                    pingOutstanding = false;
                }
            }
            else if (!connected())
            {
                return false;
            }
        }
        return true;
    }
    return false;
}

boolean MqttClient::publish(const char *topic, const char *payload)
{
    return publish(topic, (const uint8_t *)payload, payload ? strnlen(payload, this->bufferSize) : 0, false);
}

boolean MqttClient::publish(const char *topic, const char *payload, boolean retained)
{
    return publish(topic, (const uint8_t *)payload, payload ? strnlen(payload, this->bufferSize) : 0, retained);
}

boolean MqttClient::publish(const char *topic, const uint8_t *payload, unsigned int plength)
{
    return publish(topic, payload, plength, false);
}

boolean MqttClient::publish(const char *topic, const uint8_t *payload, unsigned int plength, boolean retained)
{
    if (connected())
    {
        if (this->bufferSize < MQTT_MAX_HEADER_SIZE + 2 + strnlen(topic, this->bufferSize) + plength)
        {
            return false;
        }
        uint32_t length = MQTT_MAX_HEADER_SIZE;
        length = writeString(topic, this->buffer, length);

        uint32_t i;
        for (i = 0; i < plength; i++)
        {
            this->buffer[length++] = payload[i];
        }

        uint32_t header = MQTTPUBLISH;
        if (retained)
        {
            header |= 1;
        }
        return write(header, this->buffer, length - MQTT_MAX_HEADER_SIZE);
    }
    return false;
}

boolean MqttClient::publish(const char *topic, uint8_t *head, unsigned int headlength, uint8_t *body, unsigned int bodylength, boolean retained)
{
    if (connected())
    {
        uint32_t plength = headlength + bodylength;
        if (this->bufferSize < MQTT_MAX_HEADER_SIZE + 2 + strnlen(topic, this->bufferSize) + plength)
        {
            return false;
        }
        uint32_t length = MQTT_MAX_HEADER_SIZE;
        length = writeString(topic, this->buffer, length);

        uint32_t i;
        for (i = 0; i < headlength; i++)
        {
            this->buffer[length++] = head[i];
        }
        for (i = 0; i < bodylength; i++)
        {
            yield();
            this->buffer[length++] = body[i];
        }

        uint8_t header = MQTTPUBLISH;
        if (retained)
        {
            header |= 1;
        }
        return write(header, this->buffer, length - MQTT_MAX_HEADER_SIZE);
    }
    return false;
}

boolean MqttClient::publish_P(const char *topic, const char *payload, boolean retained)
{
    return publish_P(topic, (const uint8_t *)payload, payload ? strnlen(payload, this->bufferSize) : 0, retained);
}

boolean MqttClient::publish_P(const char *topic, const uint8_t *payload, unsigned int plength, boolean retained)
{
    uint8_t llen = 0;
    uint8_t digit;
    unsigned int rc = 0;
    uint32_t tlen;
    unsigned int pos = 0;
    unsigned int i;
    uint8_t header;
    unsigned int len;
    int expectedLength;

    if (!connected())
    {
        return false;
    }

    tlen = strnlen(topic, this->bufferSize);

    header = MQTTPUBLISH;
    if (retained)
    {
        header |= 1;
    }
    this->buffer[pos++] = header;
    len = plength + 2 + tlen;
    do
    {
        digit = len & 127;
        len >>= 7;
        if (len > 0)
        {
            digit |= 0x80;
        }
        this->buffer[pos++] = digit;
        llen++;
    } while (len > 0);

    pos = writeString(topic, this->buffer, pos);

    rc += _client->write(this->buffer, pos);

    for (i = 0; i < plength; i++)
    {
        rc += _client->write((char)pgm_read_byte_near(payload + i));
    }

    lastOutActivity = millis();

    expectedLength = 1 + llen + 2 + tlen + plength;

    return (rc == expectedLength);
}

boolean MqttClient::beginPublish(const char *topic, unsigned int plength, boolean retained)
{
    if (connected())
    {
        uint32_t length = MQTT_MAX_HEADER_SIZE;
        length = writeString(topic, this->buffer, length);
        uint8_t header = MQTTPUBLISH;
        if (retained)
        {
            header |= 1;
        }
        size_t hlen = buildHeader(header, this->buffer, plength + length - MQTT_MAX_HEADER_SIZE);
        uint32_t rc = _client->write(this->buffer + (MQTT_MAX_HEADER_SIZE - hlen), length - (MQTT_MAX_HEADER_SIZE - hlen));
        lastOutActivity = millis();
        return (rc == (length - (MQTT_MAX_HEADER_SIZE - hlen)));
    }
    return false;
}

int MqttClient::endPublish()
{
    return 1;
}

size_t MqttClient::write(uint8_t data)
{
    lastOutActivity = millis();
    return _client->write(data);
}

size_t MqttClient::write(const uint8_t *buffer, size_t size)
{
    lastOutActivity = millis();
    return _client->write(buffer, size);
}

size_t MqttClient::buildHeader(uint8_t header, uint8_t *buf, uint32_t length)
{
    uint8_t lenBuf[4];
    uint8_t llen = 0;
    uint8_t digit;
    uint8_t pos = 0;
    uint32_t len = length;
    do
    {
        digit = len & 127;
        len >>= 7;
        if (len > 0)
        {
            digit |= 0x80;
        }
        lenBuf[pos++] = digit;
        llen++;
    } while (len > 0);

    buf[4 - llen] = header;
    for (int i = 0; i < llen; i++)
    {
        buf[MQTT_MAX_HEADER_SIZE - llen + i] = lenBuf[i];
    }
    return llen + 1;
}

boolean MqttClient::write(uint8_t header, uint8_t *buf, uint32_t length)
{
    uint32_t rc;
    uint8_t hlen = buildHeader(header, buf, length);
    rc = _client->write(buf + (MQTT_MAX_HEADER_SIZE - hlen), length + hlen);
    lastOutActivity = millis();
    return (rc == hlen + length);
}

boolean MqttClient::subscribe(const char *topic)
{
    return subscribe(topic, 0);
}

boolean MqttClient::subscribe(const char *topic, uint8_t qos)
{
    uint32_t topicLength = strnlen(topic, this->bufferSize);
    if (topic == 0)
    {
        return false;
    }
    if (qos > 1)
    {
        return false;
    }
    if (this->bufferSize < 9 + topicLength)
    {
        return false;
    }
    if (connected())
    {
        uint32_t length = MQTT_MAX_HEADER_SIZE;
        nextMsgId++;
        if (nextMsgId == 0)
        {
            nextMsgId = 1;
        }
        this->buffer[length++] = (nextMsgId >> 8);
        this->buffer[length++] = (nextMsgId & 0xFF);
        length = writeString((char *)topic, this->buffer, length);
        this->buffer[length++] = qos;
        return write(MQTTSUBSCRIBE | MQTTQOS1, this->buffer, length - MQTT_MAX_HEADER_SIZE);
    }
    return false;
}

boolean MqttClient::unsubscribe(const char *topic)
{
    uint32_t topicLength = strnlen(topic, this->bufferSize);
    if (topic == 0)
    {
        return false;
    }
    if (this->bufferSize < 9 + topicLength)
    {
        return false;
    }
    if (connected())
    {
        uint32_t length = MQTT_MAX_HEADER_SIZE;
        nextMsgId++;
        if (nextMsgId == 0)
        {
            nextMsgId = 1;
        }
        this->buffer[length++] = (nextMsgId >> 8);
        this->buffer[length++] = (nextMsgId & 0xFF);
        length = writeString(topic, this->buffer, length);
        return write(MQTTUNSUBSCRIBE | MQTTQOS1, this->buffer, length - MQTT_MAX_HEADER_SIZE);
    }
    return false;
}

void MqttClient::disconnect()
{
    this->buffer[0] = MQTTDISCONNECT;
    this->buffer[1] = 0;
    _client->write(this->buffer, 2);
    _state = MQTT_DISCONNECTED;
    _client->flush();
    _client->stop();
    lastInActivity = lastOutActivity = millis();
}

uint32_t MqttClient::writeString(const char *string, uint8_t *buf, uint32_t pos)
{
    const char *idp = string;
    uint32_t i = 0;
    pos += 2;
    while (*idp)
    {
        buf[pos++] = *idp++;
        i++;
    }
    buf[pos - i - 2] = (i >> 8);
    buf[pos - i - 1] = (i & 0xFF);
    return pos;
}

boolean MqttClient::connected()
{
    boolean rc;
    if (_client == NULL)
    {
        rc = false;
    }
    else
    {
        rc = (int)_client->connected();
        if (!rc)
        {
            if (this->_state == MQTT_CONNECTED)
            {
                this->_state = MQTT_CONNECTION_LOST;
                _client->flush();
                _client->stop();
            }
        }
        else
        {
            return this->_state == MQTT_CONNECTED;
        }
    }
    return rc;
}

MqttClient &MqttClient::setServer(uint8_t *ip, uint16_t port)
{
    IPAddress addr(ip[0], ip[1], ip[2], ip[3]);
    return setServer(addr, port);
}

MqttClient &MqttClient::setServer(IPAddress ip, uint16_t port)
{
    this->ip = ip;
    this->port = port;
    this->domain = NULL;
    return *this;
}

MqttClient &MqttClient::setServer(const char *domain, uint16_t port)
{
    this->domain = domain;
    this->port = port;
    return *this;
}

MqttClient &MqttClient::setCallback(MQTT_CALLBACK_SIGNATURE)
{
    this->callback = callback;
    return *this;
}

MqttClient &MqttClient::setClient(Client &client)
{
    this->_client = &client;
    return *this;
}

MqttClient &MqttClient::setStream(Stream &stream)
{
    this->stream = &stream;
    return *this;
}

int MqttClient::state()
{
    return this->_state;
}

boolean MqttClient::setBufferSize(size_t size)
{
    if (size == 0)
    {
        return false;
    }
    if (this->bufferSize == 0)
    {
        this->buffer = (uint8_t *)malloc(size);
    }
    else
    {
        uint8_t *newBuffer = (uint8_t *)realloc(this->buffer, size);
        if (newBuffer != NULL)
        {
            this->buffer = newBuffer;
        }
        else
        {
            return false;
        }
    }
    this->bufferSize = size;
    return (this->buffer != NULL);
}

boolean MqttClient::setBufferSize(size_t size, bool psram)
{
    if (psram)
    {
        this->buffer = (uint8_t *)ps_malloc(size);
        this->bufferSize = size;
        return (this->buffer != NULL);
    }
    else
    {
        return this->setBufferSize(size);
    }
}

uint32_t MqttClient::getBufferSize()
{
    return this->bufferSize;
}
MqttClient &MqttClient::setKeepAlive(uint16_t keepAlive)
{
    this->keepAlive = keepAlive;
    return *this;
}
MqttClient &MqttClient::setSocketTimeout(uint16_t timeout)
{
    this->socketTimeout = timeout;
    return *this;
}

void MqttClient::setReadTimeoutEnabled(bool enable)
{
    this->mReadTimeoutEnabled = enable;
}