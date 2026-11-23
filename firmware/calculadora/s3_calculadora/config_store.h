#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "calculator_app.h"
#include "secrets.local.h"

namespace deviceconfig {
struct Profile {String ssid,password;};
inline std::vector<Profile> profiles(){
  Preferences p;p.begin("calcnet",true);String raw=p.getString("networks","[]");p.end();JsonDocument doc;deserializeJson(doc,raw);std::vector<Profile> out;
  for(JsonObject v:doc.as<JsonArray>())if(out.size()<8)out.push_back({v["ssid"]|"",v["password"]|""});return out;
}
inline bool putProfile(const String& ssid,const String& password){
  if(ssid.length()<1||ssid.length()>32||password.length()>63||(!password.isEmpty()&&password.length()<8))return false;
  auto saved=profiles();int index=-1;for(size_t i=0;i<saved.size();i++)if(saved[i].ssid==ssid)index=i;
  if(index<0){if(saved.size()>=8)return false;saved.push_back({ssid,password});}else saved[index]={ssid,password};
  JsonDocument doc;JsonArray values=doc.to<JsonArray>();for(const auto& v:saved){JsonObject e=values.add<JsonObject>();e["ssid"]=v.ssid;e["password"]=v.password;}
  String raw;serializeJson(doc,raw);Preferences p;p.begin("calcnet",false);size_t written=p.putString("networks",raw);p.end();return written>0;
}
inline void seed(){Preferences p;p.begin("calcnet",false);bool exists=p.isKey("networks");p.end();if(!exists){putProfile(DEFAULT_WIFI_1_SSID,DEFAULT_WIFI_1_PASSWORD);putProfile(DEFAULT_WIFI_2_SSID,DEFAULT_WIFI_2_PASSWORD);}}
inline void migratePhone(){Preferences p;p.begin("calcnet",false);bool done=p.getBool("phone_v2",false);p.end();if(!done&&putProfile(DEFAULT_WIFI_2_SSID,DEFAULT_WIFI_2_PASSWORD)){p.begin("calcnet",false);p.putBool("phone_v2",true);p.end();}}
inline String apiKey(){Preferences p;p.begin("calcnet",true);String key=p.getString("api_key","");p.end();return key;}
inline bool putKey(String key){key.trim();if(key.length()<20||key.length()>240||!key.startsWith("sk-"))return false;for(unsigned i=0;i<key.length();i++)if(key[i]<=32||key[i]>=127)return false;Preferences p;p.begin("calcnet",false);bool ok=p.putString("api_key",key)>0;p.end();return ok;}
inline void loadSettings(calc::App& app){
  Preferences p;p.begin("calcprefs",true);app.engine.rpn=p.getBool("rpn",false);app.engine.degrees=p.getBool("degrees",true);app.economy=p.getBool("economy",false);app.mirror=p.getBool("mirror",false);app.flip=p.getBool("flip",false);
  for(int i=0;i<6;i++){String name="m"+String(i);app.engine.memory[i]=p.getDouble(name.c_str(),0);}
  String program=p.getString("program","");p.end();int first=0;for(unsigned i=0;i<=program.length();i++)if(i==program.length()||program[i]==' '){if(i>(unsigned)first&&app.program.size()<96)app.program.push_back(program.substring(first,i).c_str());first=i+1;}
}
inline void saveSettings(const calc::App& app){
  Preferences p;p.begin("calcprefs",false);p.putBool("rpn",app.engine.rpn);p.putBool("degrees",app.engine.degrees);p.putBool("economy",app.economy);p.putBool("mirror",app.mirror);p.putBool("flip",app.flip);
  for(int i=0;i<6;i++){String name="m"+String(i);p.putDouble(name.c_str(),app.engine.memory[i]);}
  String program;for(const auto& k:app.program){if(program.length())program+=' ';program+=k.c_str();}p.putString("program",program);p.end();
}
} // namespace deviceconfig
