#pragma once
#include <LittleFS.h>
#include <esp_heap_caps.h>

// Main-loop ownership: HTTP, camera and serial never mutate this concurrently.
class PhotoQueue {
public:
  static constexpr size_t ramBudget=1536*1024, flashBudget=600*1024, reserve=128*1024;
  struct Entry {uint32_t id=0;size_t bytes=0;uint8_t* ram=nullptr;};
  Entry entries[16];unsigned count=0;bool mounted=false;
  String error;
private:
  uint32_t sequence=0;size_t received=0,expected=0;uint8_t first[2]={},last[2]={};
  Entry pending;File file;bool importing=false;
  String path(uint32_t id){char name[32];snprintf(name,sizeof(name),"/photo-%08lu.jpg",(unsigned long)id);return name;}
public:
  size_t ramUsed()const {size_t n=0;for(unsigned i=0;i<count;i++)if(entries[i].ram)n+=entries[i].bytes;return n;}
  size_t flashUsed()const {size_t n=0;for(unsigned i=0;i<count;i++)if(!entries[i].ram)n+=entries[i].bytes;return n;}
  void recover(){
    File root=LittleFS.open("/");File f=root.openNextFile();
    while(f){String name=f.name();uint32_t id=0;unsigned long parsed=0;
      if(!f.isDirectory()&&sscanf(name.c_str(),"photo-%lu.jpg",&parsed)==1&&count<16){id=parsed;sequence=max(sequence,id);entries[count++]={id,f.size(),nullptr};}
      f=root.openNextFile();
    }
    for(unsigned i=0;i<count;i++)for(unsigned j=i+1;j<count;j++)if(entries[j].id<entries[i].id){Entry t=entries[i];entries[i]=entries[j];entries[j]=t;}
    LittleFS.remove("/photo.part");
  }
  void begin(){mounted=LittleFS.begin(false,"/littlefs",6,"spiffs");if(mounted)recover();}
  bool format(){if(importing)return false;clear();LittleFS.end();mounted=false;if(!LittleFS.format())return false;begin();return mounted;}
  void abort(){if(file)file.close();if(pending.ram)heap_caps_free(pending.ram);pending={};if(mounted)LittleFS.remove("/photo.part");importing=false;}
  bool start(size_t n,bool forceFlash=false){
    if(importing){error="Photo import busy";return false;}error="";
    if(count>=16||n<4||n>1500000){error="Photo size/count limit";return false;}
    pending={++sequence,n,nullptr};expected=n;received=0;memset(first,0,2);memset(last,0,2);
    if(!forceFlash&&ramUsed()+n<=ramBudget&&ESP.getFreePsram()>n+512*1024)
      pending.ram=(uint8_t*)heap_caps_malloc(n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(!pending.ram){
      if(!mounted||flashUsed()+n>flashBudget||LittleFS.totalBytes()<LittleFS.usedBytes()+n+reserve){pending={};error="Photo memory full / filesystem not formatted";return false;}
      file=LittleFS.open("/photo.part","w");if(!file){pending={};error="Photo file open failed";return false;}
    }
    importing=true;return true;
  }
  bool write(const uint8_t* data,size_t n){
    if(!importing||received+n>expected){error="Photo length mismatch";abort();return false;}
    for(size_t i=0;i<n;i++){if(received+i<2)first[received+i]=data[i];last[0]=last[1];last[1]=data[i];}
    if(pending.ram)memcpy(pending.ram+received,data,n);
    else if(file.write(data,n)!=n){error="Photo flash write failed";abort();return false;}
    received+=n;return true;
  }
  bool finish(){
    if(!importing||received!=expected||first[0]!=0xff||first[1]!=0xd8||last[0]!=0xff||last[1]!=0xd9){error="Incomplete/invalid JPEG";abort();return false;}
    if(file){file.flush();file.close();if(!LittleFS.rename("/photo.part",path(pending.id))){error="Photo commit failed";abort();return false;}}
    entries[count++]=pending;pending={};importing=false;return true;
  }
  bool save(const uint8_t* p,size_t n){return start(n)&&write(p,n)&&finish();}
  Entry* get(uint32_t id){for(unsigned i=0;i<count;i++)if(entries[i].id==id)return &entries[i];return nullptr;}
  File open(uint32_t id){return mounted?LittleFS.open(path(id),"r"):File();}
  bool clear(){abort();bool ok=true;for(unsigned i=0;i<count;i++){if(entries[i].ram)heap_caps_free(entries[i].ram);else if(mounted&&!LittleFS.remove(path(entries[i].id)))ok=false;entries[i]={};}count=0;return ok;}
};
