#include <Arduino.h>

int main(void)
{
  init();
  Serial.begin(57600);
  int a = 7;
  pinMode(LED_BUILTIN, OUTPUT);

  for (;;) {
    delay(1000);
    Serial.print("LED OFF ");
    Serial.print(a);
    Serial.println();
    a+=1;
  }
  return 0;
}
