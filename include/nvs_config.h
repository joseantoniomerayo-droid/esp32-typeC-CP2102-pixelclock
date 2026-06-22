#pragma once
#include <Arduino.h>

#define DEF_BRILLO_DIA    40
#define DEF_BRILLO_NOCHE  1
#define DEF_INICIO_NOCHE  23
#define DEF_FIN_NOCHE     7
#define DEF_GRADIENTE     0
#define DEF_LAT           "40.4168"
#define DEF_LON           "-3.7038"
#define DEF_CLIMA_REFRESH 30
#define DEF_USAR_NOCTURNO 1
#define DEF_CALENDAR_ACTIVO 1

void  nvsInit();
void  nvsSaveStr(const char* key, const char* val);
void  nvsSaveInt(const char* key, int val);
String nvsLoadStr(const char* key, const char* def);
int   nvsLoadInt(const char* key, int def);

int   getBrilloDia();
int   getBrilloNoche();
int   getInicioNoche();
int   getFinNoche();
int   getGradiente();
int   getGradienteNoche();
int   getFormatoFecha();
bool  getMostrarDia();
bool  getMarqueeActivo();
uint32_t getColorInicio(bool noche);
uint32_t getColorFin(bool noche);
float getLatitud();
float getLongitud();
int   getClimaRefresh();
bool  getUsarNocturno();
bool  getCalendarActivo();
void  setCalendarActivo(bool v);

void  setBrilloDia(int v);
void  setBrilloNoche(int v);
void  setInicioNoche(int v);
void  setFinNoche(int v);
void  setGradiente(int v);
void  setGradienteNoche(int v);
void  setFormatoFecha(int v);
void  setMostrarDia(bool v);
void  setMarqueeActivo(bool v);
void  setColorInicio(bool noche, uint32_t rgb);
void  setColorFin(bool noche, uint32_t rgb);
void  setLatitud(float v);
void  setLongitud(float v);
void  setClimaRefresh(int v);
void  setUsarNocturno(bool v);
