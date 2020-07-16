#include "gui/ui.h"

int main(int argc, char **argv) {
  App app;
  CHECK(app.Init()) << "Init app falied!";

  return app.MainLoop();
}