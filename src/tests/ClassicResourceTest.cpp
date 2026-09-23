#include <cstdint>
#include <initializer_list>
#include <stdexcept>
#include <string>

#include <phosg/Filesystem.hh>
#include <resource_file/IndexFormats/Formats.hh>
#include <resource_file/ResourceFile.hh>

static int16_t be16(const std::string& data, size_t offset) {
  return (static_cast<uint8_t>(data.at(offset)) << 8) |
      static_cast<uint8_t>(data.at(offset + 1));
}

int main(int argc, char** argv) {
  if (argc != 2) {
    return 2;
  }
  auto resources = ResourceDASM::parse_resource_fork(phosg::load_file(argv[1]));
  for (int id : {128, 129, 130, 131, 132}) {
    auto resource = resources.get_resource(0x57494E44, id);
    if (be16(resource->data, 0) < 0 || be16(resource->data, 2) < 0 ||
        be16(resource->data, 4) > 480 || be16(resource->data, 6) > 640) {
      throw std::runtime_error("7.1.2 window lies outside the classic canvas");
    }
  }
  for (int id : {137, 138, 144, 156, 166, 300}) {
    auto resource = resources.get_resource(0x444C4F47, id);
    if (be16(resource->data, 0) < 0 || be16(resource->data, 2) < 0 ||
        be16(resource->data, 4) > 480 || be16(resource->data, 6) > 640) {
      throw std::runtime_error("7.1.2 dialog lies outside the classic canvas");
    }
  }
  auto window = resources.get_resource(0x57494E44, 128);
  if (be16(window->data, 4) != 480 || be16(window->data, 6) != 640) {
    throw std::runtime_error("7.1.2 main window has the wrong bounds");
  }
  auto picture = resources.get_resource(0x50494354, 176);
  if (be16(picture->data, 8) != 640) {
    throw std::runtime_error("7.1.2 main panel has the wrong width");
  }
  if (!resources.resource_exists(0x64637462, 145)) {
    throw std::runtime_error("7.1.2 dialog color table 145 is missing");
  }
  return 0;
}
