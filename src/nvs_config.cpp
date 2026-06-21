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
int getFormatoFecha()  { return nvsLoadInt("formato_fecha", 0); }
bool getMostrarDia()   { return nvsLoadInt("mostrar_dia", 1); }
bool getMarqueeActivo(){ return nvsLoadInt("marquee_activo", 1); }

// Color helpers: almacenados como "RRGGBB" en NVS
static uint32_t hexToRgb(const String& hex) {
  if (hex.length() < 6) return 0;
  return strtoul(hex.c_str(), NULL, 16);
}
static String rgbToHex(uint32_t rgb) {
  char buf[8]; snprintf(buf, sizeof(buf), "%06X", rgb); return String(buf);
}

uint32_t getColorInicio(bool noche) {
  const char* key = noche ? "grad_noc_ini" : "grad_ini";
  const char* def = noche ? "2850A0" : "00B4FF";
  return hexToRgb(nvsLoadStr(key, def));
}
uint32_t getColorFin(bool noche) {
  const char* key = noche ? "grad_noc_fin" : "grad_fin";
  const char* def = noche ? "102040" : "FF0000";
  return hexToRgb(nvsLoadStr(key, def));
}

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
void setFormatoFecha(int v)  { nvsSaveInt("formato_fecha", v); }
void setMostrarDia(bool v)   { nvsSaveInt("mostrar_dia", v); }
void setMarqueeActivo(bool v){ nvsSaveInt("marquee_activo", v); }
void setColorInicio(bool noche, uint32_t rgb) {
  nvsSaveStr(noche ? "grad_noc_ini" : "grad_ini", rgbToHex(rgb).c_str());
}
void setColorFin(bool noche, uint32_t rgb) {
  nvsSaveStr(noche ? "grad_noc_fin" : "grad_fin", rgbToHex(rgb).c_str());
}
void setClimaRefresh(int v) { nvsSaveInt("clima_refresh", v); }
int getTopRotacion()  { return nvsLoadInt("top_rotacion", DEF_TOP_ROTACION); }
void setTopRotacion(int v) { nvsSaveInt("top_rotacion", v); }

void setLatitud(float v) {
  char buf[16]; snprintf(buf, sizeof(buf), "%.4f", v); nvsSaveStr("clima_lat", buf);
}
void setLongitud(float v) {
  char buf[16]; snprintf(buf, sizeof(buf), "%.4f", v); nvsSaveStr("clima_lon", buf);
}
