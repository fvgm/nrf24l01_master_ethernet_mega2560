#include "nrf24l01_master_ethernet_mega2560.h"

#include "libraries/ArduinoJson-6.x/ArduinoJson.h"
#define SD_CARD_CS_PIN 4
#define ETHERNET_CS_PIN 10
#define RF24_CE_PIN 7
#define RF24_CS_PIN 8

#define REQ_BUF_SZ   60     // tamanho do buffer para requisi��es HTTP
#define TXT_BUF_SZ   50     // tamanho do buffer do texto recebido

File root;

// vari�veis Ethernet
EthernetClient incomingClient;
EthernetClient client;
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };     // MAC Ethernet
IPAddress ip(192, 168, 100, 177);                        // endere�o IP do servidor WEB
EthernetServer server(80);                               // porta 80
File webFile;                                            // p�gina da WEB no cart�o SD
char HTTP_req[REQ_BUF_SZ] = {0};                         // buffer para requis�es HTTP
short req_index = 0;                                     // �ndice para percorrer o buffer
char txt_buf[TXT_BUF_SZ] = {0};                          // buffer para salver o texto recebido do formul�rio

IPAddress remoteServer(107, 180, 40, 57);

struct webData {
	int a;
	int b;
	int c;
};

// vari�veis do RTC
RtcDS3231<TwoWire> rtc(Wire);
RtcDateTime now;

// vari�veis do nRF24L01+
RF24 radio(RF24_CE_PIN, RF24_CS_PIN);
const uint64_t pAddress = 0xB00B1E5000LL;
float temp;
String data;
int masterSendCount = 1; // armazenar a quantidade de transmiss�es bem-sucedidas

void setup() {

	Serial.begin(9600); // para DEBUG

	// configura os pinos CS dos perif�rios SPI como sa�das
	pinMode(SD_CARD_CS_PIN, OUTPUT);
	pinMode(ETHERNET_CS_PIN, OUTPUT);
	pinMode(RF24_CS_PIN, OUTPUT);
	pinMode(RF24_CE_PIN, OUTPUT);

	// desabilita todos os perif�rios SPI
	digitalWrite(SD_CARD_CS_PIN, HIGH);
	digitalWrite(ETHERNET_CS_PIN, HIGH);
	digitalWrite(RF24_CS_PIN, HIGH);
	digitalWrite(RF24_CE_PIN, HIGH);

    // inicialia��o do RTC
	Serial.print("Inicializando RTC..............");
    rtc.Begin();
    rtc.Enable32kHzPin(false);
    rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
    now = rtc.GetDateTime();
    Serial.println("OK!");

	// inicializa��o do cart�o SD
    digitalWrite(SD_CARD_CS_PIN, LOW);
    Serial.print("Inicializando o cart�o SD......");
    delay(1000);

    if (!SD.begin(SD_CARD_CS_PIN)) {
    	Serial.println("ERRO.");
    	while(1);
    }
    Serial.println("OK!");

    Serial.print("Verificando arquivos...........");
    if (!SD.exists("index.htm")) {
    	Serial.println("ERRO - Imposs�vel encontrar o arquivo index.htm!");
		while(1);
    }
    Serial.println("OK!");
    digitalWrite(SD_CARD_CS_PIN, HIGH);


    // inicializa��o do chip Ethernet W5100
    digitalWrite(ETHERNET_CS_PIN, LOW);
    Serial.print("Inicializando Ethernet.........");
    delay(1000);

    Ethernet.init(ETHERNET_CS_PIN);
    delay(200);
    Ethernet.begin(mac, ip);
    server.begin();

    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("ERRO.");
      while(1);
    }
    else if (Ethernet.hardwareStatus() == EthernetW5100) {
      Serial.println("OK!");
      Serial.print("Endere�o do Servidor: ");
      Serial.print(Ethernet.localIP());
      Serial.println(":80");
    }
    digitalWrite(ETHERNET_CS_PIN, HIGH);

    // inicializa��o do r�dio nRF24L01+
    digitalWrite(RF24_CS_PIN, LOW);
    Serial.print("Inicializando r�dio...........");

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

unsigned long previousMillis = 0;

void loop(){


} // loop

void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

// formatTime(now, "h:m:s")
String formatTime(const RtcDateTime& dt, String format) {
  String h = dt.Hour() < 10 ? "0" + String(dt.Hour()) : String(dt.Hour()) ;
  String m = dt.Minute() < 10 ? "0" + String(dt.Minute()) : String(dt.Minute()) ;
  String s = dt.Second() < 10 ? "0" + String(dt.Second()) : String(dt.Second()) ;
  format.replace("h",h);
  format.replace("m",m);
  format.replace("s",s);
  return format;
}

void StrClear(char *str, char length) {
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

void receiveNodeData() {
	//radio.openWritingPipe(nodeAddresses[0]);

	Serial.println("Tentando transmitir ao NODE1..");

	bool tx_sent;
	tx_sent = radio.write(&masterSendCount, sizeof(masterSendCount));

	if (tx_sent) {
		if (radio.isAckPayloadAvailable()) {
			radio.read(&temp, sizeof(temp)); // ler a temperatura do n�
			Serial.print("Recebido ");
			Serial.print(temp);
			Serial.println(" com sucesso.");
		}
	} else { // falha na transmiss�o
		Serial.println("A transmiss�o ao NODE1 falhou.");
	}

}
