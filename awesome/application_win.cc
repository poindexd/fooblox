#include "application.h"
#include "view.h"
#include <Awesomium/WebCore.h>
#include <string>

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
using namespace Awesomium;

class ApplicationWin : public Application {
  bool is_running_;
 public:
  ApplicationWin() {
    is_running_ = true;
    listener_ = NULL;
    web_core_ = NULL;
  }

  virtual ~ApplicationWin() {
    if (listener())
      listener()->OnShutdown();

    if (web_core_)
      web_core_->Shutdown();
  }

  virtual void Run() {
    Load();

    // Main message loop:
    //MSG msg;
	char key;
    while (is_running_) {
      web_core_->Update();
      //TranslateMessage(&msg);
      //DispatchMessage(&msg);
      if (listener())
        listener()->OnUpdate();
	  key = cvWaitKey(100);
    }
  }
  
  virtual void Quit() {
    is_running_ = false;
  }

  virtual void Load() {
     web_core_ = WebCore::Initialize(WebConfig());

    if (listener())
      listener()->OnLoaded();
  }

  virtual View* CreateView(int width, int height) {
    return View::Create(width, height);
  }

  virtual void DestroyView(View* view) {
    delete view;
  }

  virtual void ShowMessage(const char* message) {
    std::wstring message_str(message, message + strlen(message));
    MessageBox(0, message_str.c_str(), message_str.c_str(), NULL);
  }
};

Application* Application::Create() {
  return new ApplicationWin();
}