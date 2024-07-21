#include <pgmspace.h>

#define SECRET

const char ssid[] = "";  //Exemplo: RDWIFI
const char pass[] = ""; //Exemplo: X3510rd08

#define THINGNAME "" //Exemplo: myespwork

int8_t TIME_ZONE = -3; //NYC(brl): -3 UTC

const char MQTT_HOST[] = "";  //Exemplo: xxxxxxxxxxo5p-ats.iot.sa-east-1.amazonaws.com
                          
// Copie o conteúdo do certificado CA 
static const char cacert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
-----END CERTIFICATE-----
)EOF";

// Copie o conteúdo do certificado xxxxxxx-certificate.pem.crt
static const char client_cert[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
-----END CERTIFICATE-----

)KEY";

// Copie o conteúdo do certificado xxxxxxxxx-private.pem.key
static const char privkey[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----

-----END RSA PRIVATE KEY-----

)KEY";