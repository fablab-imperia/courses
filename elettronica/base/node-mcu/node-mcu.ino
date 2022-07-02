// Inclusione libreria DHT per lettura sensore temperatura DHT11
#include "DHT.h"

// Inclusione libreria Wifi
#include <WiFi.h>

#define LED_PIN 33
#define DHT_PIN 34

// Credenziali Wifi da utilizzare per il collegamento (access point esterno)
const char* ssid = "Esp32@Fablab";
const char* password = "123456";

// Creazione server web su porta 80 (HTTP)
WiFiServer server(80);

// Creo un sensore di temperatura
DHT dht(DHT_PIN, DHT11);

// Uso una stringa per salvare i dati ricevuti dal client
// in modo semplice
String header;

// Variabile che memorizza lo stato del LED
String ledState = "off";

// Valore attuale del tempo
unsigned long currentTime = millis();

// Ultimo valore di tempo salvato
unsigned long previousTime = 0; 

// Timeout per la connessione in millisecondi (example: 2000ms = 2s)
const long timeoutTime = 2000;

// La funzione setup viene automaticamente chiamata all'accensione della scheda
void setup() {

  // Preparo linea seriale per serial monitor
  Serial.begin(115200);
  dht.begin();

  // Configuro il pin per il LED come output
  pinMode(LED_PIN, OUTPUT);

  // 0V su LED all'avvio del programma
  digitalWrite(LED_PIN, LOW);

  // Configuro Wifi sulla scheda con SSID e Password
  Serial.print("Preparazione Wifi in corso con SSID ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // In attesa fino a che Wifi risulta connesso
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Dopo la connessione, ottengo un indirizzo IP e lo stampo su serial monitor 
  Serial.println("");
  Serial.println("WiFi pronto.");
  Serial.println("Indirizzo IP: ");
  Serial.println(WiFi.localIP());
  
  // Avvio il server web
  server.begin();
}


// Metodo Loop (chiamato all'infinito)
void loop(){

  // La variabile client viene riempita se riceviamo una chiamata via HTTP  
  // da qualche client (browser). In caso contrario rimane NULL
  WiFiClient client = server.available();   

  // Se la varibile è piena (ricevuta chiamata da client)
  if (client) { 
    // Salva ora attuale                            
    currentTime = millis();
    // Salva ora precedente
    previousTime = currentTime;

    //Stampo messaggio al primo collegamento
    Serial.println("Client connesso.");     

    // Salviamo le singole linee di testo ricevute dal client
    // in questa variabile    
    String currentLine = "";                


    // Se client ancora connesso e tempo limite non scaduto
    while (client.connected() && currentTime - previousTime <= timeoutTime) { 
      
      // Aggiorno tempo trascorso
      currentTime = millis();
      
      // Se ci sono dati in arrivo dal client
      if (client.available()) {             

        // Leggi un byte di dati dal client  
        char c = client.read();             
        // Stampa quanto letto su serial monitor
        Serial.write(c);     

        // Aggiungi byte letto a variabile                
        header += c;

        if (c == '\n') {                    

          // Le richieste HTTP terminano con una linea vuota.
          // Se abbiamo ricevuto un carattere di 'a capo' e la linea corrente
          // è vuota allora siamo alla fine della richiesta: occorre mandare una risposta

          if (currentLine.length() == 0) {

            float h = dht.readHumidity();
            // Read temperature as Celsius (the default)
            float t = dht.readTemperature();

            // Le risposte HTTP iniziano con codice HTTP e relativa traduzione (per esempio 200 OK)
            // che vuol dire "tutto bene dal server" o "nessun errore - eccoti la risposta"
            client.println("HTTP/1.1 200 OK");
            // Poi Content-type dice al client che tipo di dati stanno arrivando (pagina HTML) 
            client.println("Content-type:text/html");
            client.println();
            
            // Se nella request è presente "/on", accendiamo il LED
            if (header.indexOf("GET /on") >= 0) {
              Serial.println("LED on");
              ledState = "on";
              digitalWrite(LED_PIN, HIGH);
            // Se nella request è presente "/off", spegnamo il LED
            } else if (header.indexOf("GET /off") >= 0) {
              Serial.println("LED off");
              ledState = "off";
              digitalWrite(LED_PIN, LOW);
            }
            
            // Pagina HTML base
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            
            // CSS 

            // CSS base per pagina
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            // Stile per i bottoni
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            
            // Stile bottone spento
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Titolo pagina
            client.println("<body><h1>ESP32 Web Server</h1>");
            
            // Descrizione LED
            client.println("<p>LED - State " + ledState + "</p>");
            
            
            // Mostra il bottone OFF se LED acceso e viceversa     
            if (ledState == "off") {
              client.println("<p><a href=\"/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
            
            // Fine pagina HTML
            client.println("</body></html>");
            
            // Risposte HTTP terminano con linea vuota
            client.println();
            
            // Dopo aver inviato la risposta esci dal loop
            break;
          } else { 
            // In caso di newline, svuota linea corrente
            currentLine = "";
          }
        // Ogni carattere diverso da carriage return va accodato alla riga corrente (le linee finiscono in \r\n )
        } else if (c != '\r') {  
          currentLine += c;
        }
      }
    }
    // Svuota variabile con richiesta client
    header = "";

    // Termina connessione
    client.stop();

    Serial.println("Client non collegato");
    Serial.println("");
  }
}
