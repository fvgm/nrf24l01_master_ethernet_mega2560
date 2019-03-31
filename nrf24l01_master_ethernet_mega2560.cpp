#include "nrf24l01_master_ethernet_mega2560.h"

#include "libraries/ArduinoJson-6.x/ArduinoJson.h"
#define SD_CARD_CS_PIN 4
#define ETHERNET_CS_PIN 10
#define RF24_CE_PIN 7
#define RF24_CS_PIN 8

#define REQ_BUF_SZ   60     // tamanho do buffer para requisições HTTP
#define TXT_BUF_SZ   50     // tamanho do buffer do texto recebido

File root;

// variáveis Ethernet
EthernetClient incomingClient;
EthernetClient client;
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };     // MAC Ethernet
IPAddress ip(192, 168, 100, 177);                        // endereço IP do servidor WEB
EthernetServer server(80);                               // porta 80
File webFile;                                            // página da WEB no cartão SD
char HTTP_req[REQ_BUF_SZ] = {0};                         // buffer para requisões HTTP
short req_index = 0;                                     // índice para percorrer o buffer
char txt_buf[TXT_BUF_SZ] = {0};                          // buffer para salver o texto recebido do formulário

IPAddress remoteServer(107, 180, 40, 57);

struct webData {
	int a;
	int b;
	int c;
};

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

	// configura os pinos CS dos periférios SPI como saídas
	pinMode(SD_CARD_CS_PIN, OUTPUT);
	pinMode(ETHERNET_CS_PIN, OUTPUT);
	pinMode(RF24_CS_PIN, OUTPUT);
	pinMode(RF24_CE_PIN, OUTPUT);

	// desabilita todos os periférios SPI
	digitalWrite(SD_CARD_CS_PIN, HIGH);
	digitalWrite(ETHERNET_CS_PIN, HIGH);
	digitalWrite(RF24_CS_PIN, HIGH);
	digitalWrite(RF24_CE_PIN, HIGH);

    // inicialiação do RTC
	Serial.print("Inicializando RTC..............");
    rtc.Begin();
    rtc.Enable32kHzPin(false);
    rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
    now = rtc.GetDateTime();
    Serial.println("OK!");

	// inicialização do cartão SD
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


    // inicialização do chip Ethernet W5100
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
      Serial.print("Endereço do Servidor: ");
      Serial.print(Ethernet.localIP());
      Serial.println(":80");
    }
    digitalWrite(ETHERNET_CS_PIN, HIGH);

    // inicialização do rádio nRF24L01+
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

unsigned long previousMillis = 0;

void loop(){
	unsigned long currentMillis = millis();

	incomingClient = server.available();                                        // tenta obter um client

	if (incomingClient) {                                                       // há client conectado?
		Serial.println("novo incomingClient conectado..");                      // DEBUG
		boolean currentLineIsBlank = true;

		while (incomingClient.connected()) {

			if (incomingClient.available()) {
				char c = incomingClient.read();                                  // ler 1 byte do client
				if (req_index < (REQ_BUF_SZ - 1)) {
					HTTP_req[req_index] = c;
					req_index++;
				}

				if (c == '\n' && currentLineIsBlank) {                   // responde para o cliente somente se o último caracter da requisição for "\n"
					incomingClient.println("HTTP/1.1 200 OK");
					incomingClient.println("Content-Type: text/html");
					incomingClient.println("Connection: keep-alive");
					incomingClient.println();

					webFile = SD.open("index.htm");   // abre a página index.htm do cartão SD

					if (webFile) {
                        while(webFile.available()) {
                        	incomingClient.write(webFile.read()); // envia a página para o client
                        }
                        webFile.close();
                    }
  		       	    req_index = 0;                                           // reinicia o buffer e limpa todos os elementos
					StrClear(HTTP_req, REQ_BUF_SZ);
					break;
				}

				if (c == '\n') {
					currentLineIsBlank = true;
				} else if ( c != '\r') {
					currentLineIsBlank = false;
				}
			}
		}
		delay(1);
		incomingClient.stop();
	}



	if (currentMillis - previousMillis > 60000) {
		previousMillis = currentMillis;

		digitalWrite(RF24_CS_PIN, LOW);
		delay(100);
		receiveNodeData();
		delay(100);
		digitalWrite(RF24_CS_PIN, HIGH);

		data = "temp=";
		data += String(temp);

		if (client.connect(remoteServer, 80)) {
			client.println("POST /add.php HTTP/1.1");
			client.println("Host: www.magro.eng.br");
			client.println("Content-Type: application/x-www-form-urlencoded");
			client.print("Content-Length: ");
			client.println(data.length());
			client.println();
			client.print(data);
			Serial.println("REQ enviada.");
		}

		if (client.connected()) {
			client.stop();
			Serial.println("client.stop()");
		}
	}

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
			radio.read(&temp, sizeof(temp)); // ler a temperatura do nó
			Serial.print("Recebido ");
			Serial.print(temp);
			Serial.println(" com sucesso.");
		}
	} else { // falha na transmissão
		Serial.println("A transmissão ao NODE1 falhou.");
	}

}
