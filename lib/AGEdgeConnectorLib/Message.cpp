#include "Message.h"

Message::Message()
{
	this->mPsramEnabled = false;
	this->reset();
}

Message::~Message()
{
	if (this->mData != NULL)
	{
		free(this->mData);
		this->mData = NULL;
	}
}

void Message::enablePsram()
{
	this->mPsramEnabled = true;
}

uint32_t Message::readInt32(uint8_t *src)
{
	return (uint32_t)((src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3]);
}

void Message::writeInt32(uint8_t *dst, uint32_t value)
{
	dst[0] = (uint8_t)((value & 0xff000000) >> 24);
	dst[1] = (uint8_t)((value & 0x00ff0000) >> 16);
	dst[2] = (uint8_t)((value & 0x0000ff00) >> 8);
	dst[3] = (uint8_t)(value & 0x000000ff);
}

bool Message::getOption(const char *name, char *buffer, uint32_t size)
{
	if (this->mOption != NULL)
	{
		char key[MESSAGE_KEY_SIZE] = {
				'\0',
		};
		char value[MESSAGE_VALUE_SIZE] = {
				'\0',
		};
		uint8_t t = 0;
		uint32_t length = strlen(this->mOption);
		for (uint32_t i = 0, p = 0; i < length; i++)
		{
			char c = this->mOption[i];
			if (c == '=')
			{
				t = 1;
				p = 0;
			}
			else if (c == '\r' || i + 1 == length)
			{
				if (strcmp(name, key) == 0)
				{
					if (i + 1 == length)
					{
						value[p] = c;
					}
					memset(buffer, 0, size);
					strncpy(buffer, value, size);
					return true;
				}
				t = 0;
				p = 0;
				memset(key, '\0', sizeof(key));
				memset(value, '\0', sizeof(value));
				i++;
			}
			else
			{
				if (t == 0)
				{
					key[p] = c;
					p++;
				}
				else if (t == 1)
				{
					value[p] = c;
					p++;
				}
			}
		}
	}
	return false;
}

size_t Message::getOptionLength()
{
	return (this->mOption == NULL) ? 0 : strlen(this->mOption);
}

void Message::setOption(const char *name, const char *value)
{
	if (strlen(this->mOption) > 0)
	{
		sprintf(this->mOption, "%s\r\n%s=%s", this->mOption, name, value);
	}
	else
	{
		sprintf(this->mOption, "%s=%s", name, value);
	}
}

void Message::setOption(const char *format, ...)
{
	va_list va;
	va_start(va, format);
	vsnprintf((char *)this->mOption, MESSAGE_OPTION_SIZE, format, va);
	va_end(va);
}

void Message::setLastModified()
{
	this->setLastModified(NULL);
}

void Message::setLastModified(const char *value)
{
	if (value == NULL)
	{
		char str[32] = {
				0,
		};
		time_t now = time(nullptr);
		struct tm *lt = localtime(&now);
		sprintf(str, "%04d-%02d-%02d %02d:%02d:%02d",
						lt->tm_year + 1900,
						lt->tm_mon + 1,
						lt->tm_mday,
						lt->tm_hour,
						lt->tm_min,
						lt->tm_sec);
		this->setOption("last-modified", str);
	}
	else
	{
		this->setOption("last-modified", value);
	}
}

void Message::setDataType(const char *value)
{
	this->setOption("data-type", value);
}

uint8_t *Message::getData()
{
	return this->mData;
}

bool Message::getString(char *buffer, uint32_t bufferSize)
{
	if (this->type == MESSAGE_TYPE_VALUE && this->mData != NULL)
	{
		for (uint32_t p = 0; p < this->mSize; p++)
		{
			if (this->mData[p] == 0)
			{
				if (p < bufferSize)
				{
					memset(buffer, 0, bufferSize);
					memcpy(buffer, this->mData, p);
				}
				else
				{
					memcpy(buffer, this->mData, bufferSize);
				}
				return true;
			}
		}
	}
	return false;
}

// JSON String Format : {"key":"string",..}
bool Message::getJSON(const char *key, char *buffer, uint32_t bufferSize)
{
	if (this->type == MESSAGE_TYPE_VALUE && this->mData != NULL)
	{
		const char *data = (char *)this->mData;
		uint32_t lastIndex = this->mSize - 1;
		if (data[0] == '{' && data[lastIndex] == '}')
		{
			char match[MESSAGE_KEY_SIZE] = {
					0,
			};
			snprintf(match, sizeof(match), "\"%s\":\"", key);
			char *start = strstr(data, match);
			if (start)
			{
				start += strlen(match);
				char *end = strchr(start, '"');
				if (end)
				{
					memset(buffer, 0, bufferSize);
					strncpy(buffer, start, end - start);
					buffer[end - start] = '\0';
					return true;
				}
			}
		}
	}
	return false;
}

// JSON String Format : {"key":1,..}
bool Message::getJSON(const char *key, int *buffer)
{
	if (this->type == MESSAGE_TYPE_VALUE && this->mData != NULL)
	{
		const char *data = (char *)this->mData;
		uint32_t lastIndex = this->mSize - 1;
		if (data[0] == '{' && data[lastIndex] == '}')
		{
			char match[MESSAGE_KEY_SIZE] = {
					0,
			};
			snprintf(match, sizeof(match), "\"%s\":", key);
			char *start = strstr(data, match);
			if (start)
			{
				start += strlen(match);
				*buffer = (int)strtol(start, NULL, 10);
				return true;
			}
		}
	}
	return false;
}

// JSON String Format : {"Key":[0, 1, 2 ...], ...}
bool Message::getJSON(const char *key, int *buffer, uint32_t size)
{
	if (this->type == MESSAGE_TYPE_VALUE && this->mData != NULL)
	{
		const char *data = (char *)this->mData;
		uint32_t lastIndex = this->mSize - 1;
		if (data[0] == '{' && data[lastIndex] == '}')
		{
			char match[MESSAGE_KEY_SIZE] = {
					0,
			};
			snprintf(match, sizeof(match), "\"%s\":[", key);
			char *start = strstr(data, match);
			if (start)
			{
				start += strlen(match);
				char *end = strchr(start, ']');
				if (end)
				{
					char parts[256];
					strncpy(parts, start, end - start);
					parts[end - start] = '\0';
					char *token = strtok(parts, ",");
					int currentIndex = 0;
					while (token != NULL)
					{
						if (currentIndex < size)
						{
							buffer[currentIndex++] = (int)strtol(token, NULL, 10);
							token = strtok(NULL, ",");
						}
					}
					return true;
				}
			}
		}
	}
	return false;
}

// JSON String Format : {"key":1.25,..}
bool Message::getJSON(const char *key, float *buffer)
{
	if (this->type == MESSAGE_TYPE_VALUE && this->mData != NULL)
	{
		const char *data = (char *)this->mData;
		uint32_t lastIndex = this->mSize - 1;
		if (data[0] == '{' && data[lastIndex] == '}')
		{
			char match[MESSAGE_KEY_SIZE] = {
					0,
			};
			snprintf(match, sizeof(match), "\"%s\":", key);
			char *start = strstr(data, match);
			if (start)
			{
				start += strlen(match);
				*buffer = (float)strtol(start, NULL, 10);
				return true;
			}
		}
	}
	return false;
}

void Message::setData(uint8_t *data, uint32_t dataSize)
{
	this->setSize(dataSize, true);
	memcpy(this->mData, data, dataSize);
}

bool Message::setValue(const char *format, ...)
{
	if (this->type == MESSAGE_TYPE_VALUE)
	{
		va_list va;
		va_start(va, format);
		vsnprintf((char *)this->mBuffer, MESSAGE_BUFFER_SIZE, format, va);
		va_end(va);

		uint32_t length = strlen((const char *)this->mBuffer);
		this->setSize(length + 1, true);
		memcpy(this->mData, this->mBuffer, length + 1);
		return true;
	}
	return false;
}

uint32_t Message::getSize()
{
	return this->mSize;
}
void Message::setSize(uint32_t size)
{
	if (this->mData == NULL)
	{
		this->setSize(size, true);
	}
	else
	{
		this->setSize(size, false);
	}
}

void Message::setSize(uint32_t size, bool reallocate)
{
	if (reallocate)
	{
		if (this->mData != NULL)
		{
			free(this->mData);
		}
		if (this->mPsramEnabled)
		{
			this->mData = (uint8_t *)ps_calloc(size, sizeof(uint8_t));
		}
		else
		{
			this->mData = (uint8_t *)calloc(size, sizeof(uint8_t));
		}
	}
	this->mSize = size;
}

bool Message::fromPayload(uint8_t *payload, uint32_t payloadSize)
{

	uint32_t length = 0;
	uint8_t *p = payload;

	this->reset();

	if (payload[0] == 0xFF && payload[1] == 0xA3)
	{
		this->version = payload[2];
		this->type = payload[3];
		p += 4;

		length = this->readInt32(p);
		p += 4;
		memcpy(this->mOption, p, length);
		p += length;

		if (this->type == MESSAGE_TYPE_VALUE)
		{
			length = this->readInt32(p);
			p += 4;
			this->setSize(length, true);
			memcpy(this->mData, p, length);
		}
		else
		{
			length = payloadSize - length;
			this->setSize(length, true);
			memcpy(this->mData, p, length);
		}
		return true;
	}
	else
	{
		this->version = 0;
		this->type = MESSAGE_TYPE_UNKNOWN;
		this->mOption[0] = '\0';
		this->setSize(payloadSize, true);
		memcpy(this->mData, payload, payloadSize);
		return false;
	}
}

uint32_t Message::toPayload(uint8_t *buffer, uint32_t bufferSize)
{

	uint32_t length = 0, p = 0;
	uint32_t size = this->mSize;

	memset(buffer, 0, bufferSize);

	buffer[0] = 0xFF;
	buffer[1] = 0xA3;
	buffer[2] = this->version;
	buffer[3] = this->type;
	p = 4;

	length = strlen(this->mOption);
	this->writeInt32(buffer + p, length);
	p += 4;
	memcpy(buffer + p, this->mOption, length);
	p += length;

	if (this->type == MESSAGE_TYPE_VALUE)
	{
		this->writeInt32(buffer + p, size);
		p += 4;
		if (size > 0)
		{
			memcpy(buffer + p, this->mData, size);
			p += size;
		}
	}
	else
	{
		if (size > 0)
		{
			memcpy(buffer + p, this->mData, size);
			p += size;
		}
	}
	return p;
}

void Message::reset()
{
	this->version = MESSAGE_VERSION;
	this->type = MESSAGE_TYPE_VALUE;
	this->mOption[0] = '\0';
	this->mBuffer[0] = '\0';
	if (this->mData != NULL)
	{
		free(this->mData);
		this->mData = NULL;
	}
	this->mSize = 0;
}