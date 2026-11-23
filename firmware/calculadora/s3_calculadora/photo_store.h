#pragma once
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <memory>
#include <vector>

// A snapshot owns the exact JPEG bytes used by the API. HTTP readers retain
// their own reference so another capture cannot free an image being sent.
class PhotoStore {
public:
  struct Snapshot {
    std::shared_ptr<uint8_t> bytes;
    size_t length=0,width=0,height=0;
    uint32_t sequence=0,capturedAt=0;
    bool standby=false;
    bool apiAttempted=false;
    int apiHttpStatus=0;
  };
private:
  SemaphoreHandle_t mutex=nullptr;
  Snapshot latest;
  std::vector<Snapshot> batch;
public:
  void begin(){mutex=xSemaphoreCreateMutex();}
  Snapshot save(const camera_fb_t& frame){
    Snapshot next;
    auto* buffer=(uint8_t*)heap_caps_malloc(frame.len,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(!buffer)return next;
    next.bytes=std::shared_ptr<uint8_t>(buffer,[](uint8_t* p){heap_caps_free(p);});
    memcpy(buffer,frame.buf,frame.len);next.length=frame.len;next.width=frame.width;next.height=frame.height;next.capturedAt=millis();
    xSemaphoreTake(mutex,portMAX_DELAY);next.sequence=latest.sequence+1;latest=next;xSemaphoreGive(mutex);
    return next;
  }
  void setStandby(uint32_t sequence,bool value){xSemaphoreTake(mutex,portMAX_DELAY);if(latest.sequence==sequence)latest.standby=value;xSemaphoreGive(mutex);}
  void markApi(uint32_t sequence,int status=0){xSemaphoreTake(mutex,portMAX_DELAY);if(latest.sequence==sequence){latest.apiAttempted=true;latest.apiHttpStatus=status;}xSemaphoreGive(mutex);}
  unsigned count(){xSemaphoreTake(mutex,portMAX_DELAY);unsigned n=batch.size();xSemaphoreGive(mutex);return n;}
  bool append(const Snapshot& value){xSemaphoreTake(mutex,portMAX_DELAY);bool ok=batch.size()<3;if(ok)batch.push_back(value);xSemaphoreGive(mutex);return ok;}
  std::vector<Snapshot> selected(){xSemaphoreTake(mutex,portMAX_DELAY);auto out=batch;xSemaphoreGive(mutex);return out;}
  void clear(bool lastOnly=false){xSemaphoreTake(mutex,portMAX_DELAY);if(lastOnly){if(!batch.empty())batch.pop_back();}else batch.clear();xSemaphoreGive(mutex);}
  Snapshot snapshot(){xSemaphoreTake(mutex,portMAX_DELAY);auto value=latest;xSemaphoreGive(mutex);return value;}
};
