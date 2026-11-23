#pragma once

// Receive into the inactive slot only. Buffer the header before erasing flash.
struct OtaReceiver {
  static constexpr size_t headerSize=sizeof(esp_image_header_t)+sizeof(esp_image_segment_header_t)+sizeof(esp_app_desc_t);
  uint8_t header[headerSize];size_t held=0,received=0,expected=0;
  esp_ota_handle_t handle=0;const esp_partition_t* target=nullptr;
  String error,expectedSha;bool receiving=false,complete=false,hashActive=false;
  mbedtls_sha256_context hash;
  OtaReceiver(){mbedtls_sha256_init(&hash);}
  void fail(const String& why){if(handle)esp_ota_abort(handle);handle=0;receiving=false;complete=false;error=why;if(hashActive){mbedtls_sha256_free(&hash);mbedtls_sha256_init(&hash);hashActive=false;}}
  void start(){
    if(handle)esp_ota_abort(handle);handle=0;held=received=expected=0;receiving=complete=false;error="";expectedSha="";
    if(!authorized()){error="Unauthorized";return;}if(!otaMode){error="Enter OTA mode first";return;}
    if(web.upload().filename!=String(projectName)+".ino.bin"){error="Wrong filename; application .ino.bin required";return;}
    String size=web.header("X-Firmware-Size");for(unsigned i=0;i<size.length();i++)if(!isDigit(size[i])){error="Invalid image length";return;}
    expected=strtoul(size.c_str(),nullptr,10);target=esp_ota_get_next_update_partition(nullptr);
    if(!target||target->address==esp_ota_get_running_partition()->address||expected<headerSize||expected>target->size){error="Image exceeds inactive OTA slot / invalid size";return;}
    expectedSha=web.header("X-Firmware-SHA256");expectedSha.toLowerCase();
    if(expectedSha.length()){if(expectedSha.length()!=64){error="Invalid SHA256";return;}for(unsigned i=0;i<64;i++)if(!isxdigit(expectedSha[i])){error="Invalid SHA256";return;}}
    mbedtls_sha256_starts(&hash,0);hashActive=true;receiving=true;
  }
  bool validateHeader(){
    esp_image_header_t image;esp_app_desc_t desc;memcpy(&image,header,sizeof(image));memcpy(&desc,header+sizeof(image)+sizeof(esp_image_segment_header_t),sizeof(desc));
    if(image.magic!=ESP_IMAGE_HEADER_MAGIC||image.chip_id!=0||desc.magic_word!=ESP_APP_DESC_MAGIC_WORD){fail("Wrong chip or invalid image header");return false;}
    if(strncmp(desc.project_name,projectName,sizeof(desc.project_name))!=0){fail("Wrong firmware project");return false;}
    esp_err_t rc=esp_ota_begin(target,expected,&handle);if(rc!=ESP_OK){fail(String("OTA begin: ")+esp_err_to_name(rc));return false;}
    rc=esp_ota_write(handle,header,held);if(rc!=ESP_OK){fail(String("OTA header write: ")+esp_err_to_name(rc));return false;}return true;
  }
  void write(const uint8_t* p,size_t n){
    if(!receiving)return;if(received+n>expected){fail("Image length exceeded");return;}
    mbedtls_sha256_update(&hash,p,n);received+=n;
    if(held<headerSize){size_t take=min(n,headerSize-held);memcpy(header+held,p,take);held+=take;p+=take;n-=take;if(held<headerSize)return;if(!validateHeader())return;}
    if(n){esp_err_t rc=esp_ota_write(handle,p,n);if(rc!=ESP_OK)fail(String("OTA write: ")+esp_err_to_name(rc));}
  }
  void finish(){
    if(!receiving)return;if(received!=expected||!handle){fail("Truncated image");return;}
    uint8_t digest[32];mbedtls_sha256_finish(&hash,digest);mbedtls_sha256_free(&hash);mbedtls_sha256_init(&hash);hashActive=false;
    if(expectedSha.length()){char hex[65];for(unsigned i=0;i<32;i++)sprintf(hex+2*i,"%02x",digest[i]);if(expectedSha!=hex){fail("SHA256 mismatch");return;}}
    // The ESP image checksum and appended SHA256 are independently checked here.
    esp_err_t rc=esp_ota_end(handle);handle=0;if(rc!=ESP_OK){fail(String("Image validation: ")+esp_err_to_name(rc));return;}
    rc=esp_ota_set_boot_partition(target);if(rc!=ESP_OK){fail(String("Boot selection: ")+esp_err_to_name(rc));return;}
    receiving=false;complete=true;
  }
} ota;

