#include "nrf24l01_master_ethernet_mega2560.h"

File root;

struct webData {
	char a;
	char b;
	char c;
};

// variáveis Ethernet
EthernetClient ethernetClient;
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };     // MAC Ethernet

IPAddress ip(192, 168, 100, 177);                        // endereço IP do servidor WEB
IPAddress remoteServerAddress(107, 180, 40, 57);

// variáveis do RTC
RtcDS3231<TwoWire> rtc(Wire);
RtcDateTime now;

// variáveis do nRF24L01+
RF24 radio(RF24_CE_PIN, RF24_CS_PIN);
const uint64_t pAddress = 0xB00B1E5000LL;
float temp;
String data;
int masterSendCount = 1; // armazenar a quantidade de transmissões bem-sucedidas

void setup() {
	Serial.begin(9600); // para DEBUG

	CSpinsInitialize();

    RTCInitialize();     // inicialiação do RTC

	SDCardInitialize(); // inicialização do cartão SD

    EthernetInitialize(); // inicialização do chip Ethernet W5100

    RadioInitialize(); // inicialização do rádio nRF24L01+
}

unsigned long previousMillis = 0;

void loop(){
	if(!ethernetClient.connect(remoteServerAddress, 80)){
		Serial.println("Conexao falhou..");
		return;
	}

	Serial.println("Conectado!");

	  // Send HTTP request
	  ethernetClient.println(F("GET /teste_json.php HTTP/1.0"));
	  ethernetClient.println(F("Host: magro.eng.br"));
	  ethernetClient.println(F("Connection: close"));
	  if (ethernetClient.println() == 0) {
	    Serial.println(F("Failed to send request"));
	    return;
	  }

	  // Check HTTP status
	  char status[32] = {0};
	  ethernetClient.readBytesUntil('\r', status, sizeof(status));
	  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
	    Serial.print(F("Unexpected response: "));
	    Serial.println(status);
	    return;
	  }

	  // Skip HTTP headers
	  char endOfHeaders[] = "\r\n\r\n";
	  if (!ethernetClient.find(endOfHeaders)) {
	    Serial.println(F("Invalid response"));
	    return;
	  }

	  // Allocate JsonBuffer
	  // Use arduinojson.org/assistant to compute the capacity.
	  const size_t capacity = JSON_OBJECT_SIZE(3) + 20;
	  DynamicJsonBuffer jsonBuffer(capacity);

	  // Parse JSON object
	  JsonObject& root = jsonBuffer.parseObject(ethernetClient);
	  if (!root.success()) {
	    Serial.println(F("Parsing failed!"));
	    return;
	  }

	  // Extract values
	  Serial.println(F("Response:"));
	  Serial.println(root["a"].as<char*>());
	  Serial.println(root["b"].as<char*>());
	  Serial.println(root["c"].as<char*>());

	  // Disconnect
	  ethernetClient.stop();
	  while(1);

} // loop



void CSpinsInitialize() {

	pinMode(SD_CARD_CS_PIN, OUTPUT);  // configura os pinos CS dos periférios SPI como saídas
	pinMode(ETHERNET_CS_PIN, OUTPUT);
	pinMode(RF24_CS_PIN, OUTPUT);
	pinMode(RF24_CE_PIN, OUTPUT);

	digitalWrite(SD_CARD_CS_PIN, HIGH);   	// desabilita todos os periférios SPI
	digitalWrite(ETHERNET_CS_PIN, HIGH);
	digitalWrite(RF24_CS_PIN, HIGH);
	digitalWrite(RF24_CE_PIN, HIGH);
}

void RTCInitialize() {
	Serial.print("Inicializando RTC..............");
    rtc.Begin();
    rtc.Enable32kHzPin(false);
    rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
    now = rtc.GetDateTime();
    Serial.println("OK!");
}

void SDCardInitialize() {
	   digitalWrite(SD_CARD_CS_PIN, LOW);
	    Serial.print("Inicializando o cartão SD......");
	    delay(1000);

	    if (!SD.begin(SD_CARD_CS_PIN)) {
	    	Serial.println("ERRO.");
	    	while(1);
	    }
	    Serial.println("OK!");

	    Serial.print("Verificando arquivos...........");
	    if (!SD.exists("index.htm")) {
	    	Serial.println("ERRO - Impossível encontrar o arquivo index.htm!");
			while(1);
	    }
	    Serial.println("OK!");
	    digitalWrite(SD_CARD_CS_PIN, HIGH);
}

void EthernetInitialize() {
    digitalWrite(ETHERNET_CS_PIN, LOW);
    Serial.print("Inicializando Ethernet.........");
    delay(1000);

    Ethernet.init(ETHERNET_CS_PIN);
    delay(200);
    Ethernet.begin(mac, ip);
    ethernetClient.setTimeout(10000);

    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("ERRO.");
      while(1);
    }
    else if (Ethernet.hardwareStatus() == EthernetW5100) {
      Serial.println("OK!");
      Serial.print("Endereço IP: ");
      Serial.print(Ethernet.localIP());
    }
    digitalWrite(ETHERNET_CS_PIN, HIGH);
}

void RadioInitialize() {
    digitalWrite(RF24_CS_PIN, LOW);
    Serial.print("Inicializando rádio...........");

    if (!radio.begin()) {
    	Serial.println("ERRO.");
    	while(1);
    }
    radio.setPALevel(RF24_PA_LOW);
    radio.setDataRate(RF24_250KBPS);
    radio.setChannel(0x76);
    radio.setRetries(4, 10);
    radio.setAutoAck(true);
    radio.enableAckPayload();
    radio.openWritingPipe(pAddress);
    radio.openReadingPipe(1, pAddress);
    radio.stopListening();
    Serial.println("OK!");
    digitalWrite(RF24_CS_PIN, HIGH);
}




