#include <ESP8266WiFi.h>


// In preferences add the following as additional board manager URL
// https://arduino.esp8266.com/stable/package_esp8266com_index.json
//
// In board manager, search for and install esp8266
//
// Select Arduino->tools->Board
//
// NodeMCU 1.0 (ESP-12E Module)
//   80 Mhz
//   115200
//   4M (3M SPIFFS)
//

// Pin use:
//
// D5 GPIO14 R LEDs on when low
// D6 GPIO12 G LEDs on when low
// D7 GPIO13 B LEDs on when low
//

#define R_LED D5
#define G_LED D6
#define B_LED D7

#define USE_WEB

#ifdef USE_WEB
char ssid[] = "Walton";  //  your network SSID (name)
char password[] = "deadbeef00";       // your network password


// Set web server port number to 80
WiFiServer server(80);

String HTTP_req;          // stores the HTTP request
boolean LED_status = 0;   // state of LED2, off by default
boolean LED_status3 = 0;   // state of LED3, off by default
#endif

void setup() {
  Serial.begin(115200);

#ifdef USE_WEB
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
#endif

  analogWrite(R_LED, 255-r);
  analogWrite(G_LED, 255-g);
  analogWrite(B_LED, 255-b);

}

#ifdef USE_WEB
int hexAsciiToInt(char c)
{
  if (c>='0' && c<='9') return c-'0';
  else if (c>='a' && c<='f') return c-'a'+10;
}

char intToHexAscii(int hex)
{
  if (hex<10) return '0' + hex;
  else return 'a'+hex-10;
}

void ProcessInputColor(WiFiClient  cl)
{
  String favcolor="ff0000";
  int colorBegin;
  int a, r, g, b;
  
  colorBegin = HTTP_req.indexOf("/?favcolor=%");
  if (colorBegin > -1) {
    colorBegin += 12;
    a = hexAsciiToInt(HTTP_req[colorBegin+0])*16 + hexAsciiToInt(HTTP_req[colorBegin+1]);
    r = hexAsciiToInt(HTTP_req[colorBegin+2])*16 + hexAsciiToInt(HTTP_req[colorBegin+3]);
    g = hexAsciiToInt(HTTP_req[colorBegin+4])*16 + hexAsciiToInt(HTTP_req[colorBegin+5]);
    b = hexAsciiToInt(HTTP_req[colorBegin+6])*16 + hexAsciiToInt(HTTP_req[colorBegin+7]);
    favcolor = "";
    favcolor += intToHexAscii(r/16);
    favcolor += intToHexAscii(r%16);
    favcolor += intToHexAscii(g/16);
    favcolor += intToHexAscii(g%16);
    favcolor += intToHexAscii(b/16);
    favcolor += intToHexAscii(b%16);
  }

  Serial.println("favcolor="+favcolor);
  analogWrite(R_LED, 255-r);
  analogWrite(G_LED, 255-g);
  analogWrite(B_LED, 255-b);
  
  cl.println("<label for=\"favcolor\">Select color:</label>");
  cl.println("<input type=\"color\" id=\"favcolor\" name=\"favcolor\" value=\"#" + favcolor + "\">");
  cl.println("<input type=\"submit\" value=\"Submit\">");
}
#endif

void loop(){
#ifdef USE_WEB
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {  // got client?
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {   // client data available to read
	char c = client.read(); // read 1 byte (character) from client
	HTTP_req += c;  // save the HTTP request 1 char at a time
	// last line of client request is blank and ends with \n
	// respond to client only after last line received
	if (c == '\n' && currentLineIsBlank) {
	  // send a standard http response header
	  client.println("HTTP/1.1 200 OK");
	  client.println("Content-Type: text/html");
	  client.println("Connection: close");
	  client.println();
	  // send web page
	  client.println("<!DOCTYPE html>");
	  client.println("<html>");
	  client.println("<head>");
	  client.println("<title>Arduino LED Control</title>");
	  //adds styles
	  client.println("<style type=\"text/css\">body {font-size:1.7rem; font-family: Georgia; text-align:center; color: #333; background-color: #cdcdcd;} div{width:75%; background-color:#fff; padding:15px; text-align:left; border-top:5px solid #bb0000; margin:25px auto;}</style>");
	  client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
	  client.println("</head>");
	  client.println("<body>");
	  client.println("<div>");
	  client.println("<h1>LED color string control</h1>");
	  client.println("<form method=\"get\">");
	  ProcessInputColor(client);
	  client.println("</form>");
	  client.println("</div>");
	  client.println("</body>");
	  client.println("</html>");
	  Serial.print(HTTP_req);
	  HTTP_req = "";    // finished with request, empty string
	  break;
	}
	// every line of text received from the client ends with \r\n
	if (c == '\n') {
	  // last character on line of received text
	  // starting new line with next character read
	  currentLineIsBlank = true;
	} 
	else if (c != '\r') {
	  // a text character was received from client
	  currentLineIsBlank = false;
	}
      } // end if (client.available())
    } // end while (client.connected())
    delay(1);      // give the web browser time to receive the data
    client.stop(); // close the connection
  } // end if (client)
#else


  analogWrite(R_LED, 255-r);
  analogWrite(G_LED, 255-g);
  analogWrite(B_LED, 255-b);



#endif
}
