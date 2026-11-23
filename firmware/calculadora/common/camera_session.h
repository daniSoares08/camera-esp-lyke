#pragma once
// Guarantees cleanup on every return path. close() is called explicitly before
// starting HTTPS so the camera is never kept running while waiting for the AI.
template<class Camera> class CameraSession {
  Camera* camera;
public:
  explicit CameraSession(Camera& c):camera(&c){}
  CameraSession(const CameraSession&)=delete;
  CameraSession& operator=(const CameraSession&)=delete;
  void close(){if(camera){camera->shutdown();camera=nullptr;}}
  ~CameraSession(){close();}
};
