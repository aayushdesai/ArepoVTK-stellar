#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

#include "stellar_camera_path_v055.h"

namespace {

const char *goodPath()
{
  return
      "# snapshot time camera look up extent center axis material disk outflow\n"
      "10 1 0 0 4 0 0 0 0 1 0 2 0 0 0 0 0 1 3 2 4\n"
      "11 2 1 0 4 1 0 0 0 1 0 2.1 1 0 0 0 0 1 3.1 2.1 4.1\n";
}

bool loadText(StellarCameraPathV055 *path, const std::string &text,
              std::string *error)
{
  std::istringstream input(text);
  return path->loadStream(input, error);
}

} // namespace

int main(int argc, char **argv)
{
  StellarCameraPathV055 path;
  std::string error;
  assert(loadText(&path, goodPath(), &error));
  assert(error.empty());
  assert(path.size() == 2);
  assert(path.find(10));
  assert(path.find(11));
  assert(!path.find(9));
  assert(!path.find(12));
  assert(path.find(11)->pose.screen_half_extent_cm == 2.1);

  const std::string duplicate =
      "10 1 0 0 4 0 0 0 0 1 0 2 0 0 0 0 0 1 3 2 4\n"
      "10 2 1 0 4 1 0 0 0 1 0 2 1 0 0 0 0 1 3 2 4\n";
  assert(!loadText(&path, duplicate, &error));
  assert(path.size() == 0);
  assert(error.find("must increase") != std::string::npos);

  const std::string bad_up =
      "10 1 0 0 4 0 0 0 0 2 0 2 0 0 0 0 0 1 3 2 4\n";
  assert(!loadText(&path, bad_up, &error));
  assert(error.find("Invalid stellar camera geometry") != std::string::npos);

  const std::string extra =
      "10 1 0 0 4 0 0 0 0 1 0 2 0 0 0 0 0 1 3 2 4 extra\n";
  assert(!loadText(&path, extra, &error));
  assert(error.find("Extra stellar camera path column") != std::string::npos);

  const std::string negative_snapshot =
      "-1 1 0 0 4 0 0 0 0 1 0 2 0 0 0 0 0 1 3 2 4\n";
  assert(!loadText(&path, negative_snapshot, &error));
  assert(error.find("Invalid stellar camera snapshot") != std::string::npos);

  if(argc == 2) {
    assert(path.loadFile(argv[1], &error));
    assert(path.size() > 0);
  } else {
    assert(argc == 1);
  }

  std::cout << "STELLAR_CAMERA_PATH_V055_OK\n";
  return 0;
}
