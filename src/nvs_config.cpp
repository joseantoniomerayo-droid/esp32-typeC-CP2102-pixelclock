#include <Arduino.h>
#include <Preferences.h>
#include "nvs_config.h"

static Preferences prefs;
static const char* NS = "reloj";

void nvsInit() { prefs.begin(NS, false); }
void nvsSaveStr(const char* key, const char* val) { prefs.putString(key, val); }
void nvsSaveInt(const char* key, int val) { prefs.putInt(key, val); }
String nvsLoadStr(const char* key, const char* def) { return prefs.getString(key, def); }
int nvsLoadInt(const char* key, int def) { return prefs.getInt(key, def); }

int getBrilloDia()    { return nvsLoadInt("brillo_dia",   DEF_BRILLO_DIA); }
int getBrilloNoche()  { return nvsLoadInt("brillo_noche", DEF_BRILLO_NOCHE); }
int getInicioNoche()  { return nvsLoadInt("inicio_noche", DEF_INICIO_NOCHE); }
int getFinNoche()     { return nvsLoadInt("fin_noche",    DEF_FIN_NOCHE); }
int getGradiente()    { return nvsLoadInt("gradiente",    DEF_GRADIENTE); }
int getGradienteNoche() { return nvsLoadInt("gradiente_noche", 2); }
int getClimaRefresh() { return nvsLoadInt("clima_refresh", DEF_CLIMA_REFRESH); }

float getLatitud() {
  String s = nvsLoadStr("clima_lat", DEF_LAT);
  return s.toFloat();
}
float getLongitud() {
  String s = nvsLoadStr("clima_lon", DEF_LON);
  return s.toFloat();
}

void setBrilloDia(int v)    { nvsSaveInt("brillo_dia",   v); }
void setBrilloNoche(int v)  { nvsSaveInt("brillo_noche", v); }
void setInicioNoche(int v)  { nvsSaveInt("inicio_noche", v); }
void setFinNoche(int v)     { nvsSaveInt("fin_noche",    v); }
void setGradiente(int v)    { nvsSaveInt("gradiente",    v); }
void setGradienteNoche(int v) { nvsSaveInt("gradiente_noche", v); }
void setClimaRefresh(int v) { nvsSaveInt("clima_refresh", v); }

void setLatitud(float v) {
  char buf[16]; snprintf(buf, sizeof(buf), "%.4f", v); nvsSaveStr("clima_lat", buf);
}
void setLongitud(float v) {
  char buf[16]; snprintf(buf, sizeof(buf), "%.4f", v); nvsSaveStr("clima_lon", buf);
}
