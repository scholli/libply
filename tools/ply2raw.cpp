#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

#include <functional>
#include <tuple>

#include <ply.hpp>

using namespace std::placeholders;
using namespace ply;

class ply_to_raw_converter
{
public:
  ply_to_raw_converter();
  bool convert(std::istream& istream, const std::string& istream_filename, std::ostream& ostream, const std::string& ostream_filename);
private:
  void info_callback(const std::string& filename, std::size_t line_number, const std::string& message);
  void warning_callback(const std::string& filename, std::size_t line_number, const std::string& message);
  void error_callback(const std::string& filename, std::size_t line_number, const std::string& message);
  std::tuple<std::function<void()>, std::function<void()> > element_definition_callback(const std::string& element_name, std::size_t count);
  template <typename ScalarType> std::function<void (ScalarType)> scalar_property_definition_callback(const std::string& element_name, const std::string& property_name);
  template <typename SizeType, typename ScalarType> std::tuple<std::function<void (SizeType)>, std::function<void (ScalarType)>, std::function<void ()> > list_property_definition_callback(const std::string& element_name, const std::string& property_name);
  void vertex_begin();
  void vertex_x(ply::float32 x);
  void vertex_y(ply::float32 y);
  void vertex_z(ply::float32 z);
  void vertex_end();
  void face_begin();
  void face_vertex_indices_begin(ply::uint8 size);
  void face_vertex_indices_element(ply::int32 vertex_index);
  void face_vertex_indices_end();
  void face_end();
  void ignore(std::string const& s) { std::cout << "Warning: ignoring element or property: " << s << std::endl; }
  std::ostream* ostream_;
  ply::float32 vertex_x_, vertex_y_, vertex_z_;
  ply::int32 face_vertex_indices_element_index_, face_vertex_indices_first_element_, face_vertex_indices_previous_element_;
  std::vector<std::tuple<ply::float32, ply::float32, ply::float32> > vertices_;
};

ply_to_raw_converter::ply_to_raw_converter()
{
}

void ply_to_raw_converter::info_callback(const std::string& filename, std::size_t line_number, const std::string& message)
{
  std::cerr << filename << ":" << line_number << ": " << "info: " << message << std::endl;
}

void ply_to_raw_converter::warning_callback(const std::string& filename, std::size_t line_number, const std::string& message)
{
  std::cerr << filename << ":" << line_number << ": " << "warning: " << message << std::endl;
}

void ply_to_raw_converter::error_callback(const std::string& filename, std::size_t line_number, const std::string& message)
{
  std::cerr << filename << ":" << line_number << ": " << "error: " << message << std::endl;
}

std::tuple<std::function<void()>, std::function<void()> > ply_to_raw_converter::element_definition_callback(const std::string& element_name, std::size_t count)
{
  if (element_name == "vertex") {
    return std::tuple<std::function<void()>, std::function<void()> >(
      std::bind(&ply_to_raw_converter::vertex_begin, this),
      std::bind(&ply_to_raw_converter::vertex_end, this)
    );
  }
  else if (element_name == "face") {
    return std::tuple<std::function<void()>, std::function<void()> >(
      std::bind(&ply_to_raw_converter::face_begin, this),
      std::bind(&ply_to_raw_converter::face_end, this)
    );
  }
  else {
    std::bind(&ply_to_raw_converter::ignore, this, element_name);
    //throw std::runtime_error("ply_to_raw_converter::element_definition_callback(): invalid element_name");
  }
}

template <>
std::function<void (ply::float32)> ply_to_raw_converter::scalar_property_definition_callback(const std::string& element_name, const std::string& property_name)
{
  if (element_name == "vertex") {
    if (property_name == "x") {
      return std::bind(&ply_to_raw_converter::vertex_x, this, _1);
    }
    else if (property_name == "y") {
      return std::bind(&ply_to_raw_converter::vertex_y, this, _1);
    }
    else if (property_name == "z") {
      return std::bind(&ply_to_raw_converter::vertex_z, this, _1);
    }
    else {
      throw std::runtime_error("ply_to_raw_converter::scalar_property_definition_callback(): invalid property_name");
    }
  }
  else {
    throw std::runtime_error("ply_to_raw_converter::scalar_property_definition_callback(): invalid element_name");
  }
}

template <>
std::tuple<std::function<void (ply::uint8)>, std::function<void (ply::int32)>, std::function<void ()> > ply_to_raw_converter::list_property_definition_callback(const std::string& element_name, const std::string& property_name)
{
  if ((element_name == "face") && (property_name == "vertex_indices")) {
    return std::tuple<std::function<void (ply::uint8)>, std::function<void (ply::int32)>, std::function<void ()> >(
      std::bind(&ply_to_raw_converter::face_vertex_indices_begin, this, _1),
      std::bind(&ply_to_raw_converter::face_vertex_indices_element, this, _1),
      std::bind(&ply_to_raw_converter::face_vertex_indices_end, this)
    );
  }
  else {
    throw std::runtime_error("ply_to_raw_converter::list_property_definition_callback(): invalid element_name or property_name");
  }
}

void ply_to_raw_converter::vertex_begin()
{
}

void ply_to_raw_converter::vertex_x(ply::float32 x)
{
  vertex_x_ = x;
}

void ply_to_raw_converter::vertex_y(ply::float32 y)
{
  vertex_y_ = y;
}

void ply_to_raw_converter::vertex_z(ply::float32 z)
{
  vertex_z_ = z;
}

void ply_to_raw_converter::vertex_end()
{
  vertices_.push_back(std::tuple<ply::float32, ply::float32, ply::float32 >(vertex_x_, vertex_y_, vertex_z_));
}

void ply_to_raw_converter::face_begin()
{
}

void ply_to_raw_converter::face_vertex_indices_begin(ply::uint8 size)
{
  face_vertex_indices_element_index_ = 0;
}

void ply_to_raw_converter::face_vertex_indices_element(ply::int32 vertex_index)
{
  if (face_vertex_indices_element_index_ == 0) {
    face_vertex_indices_first_element_ = vertex_index;
  }
  else if (face_vertex_indices_element_index_ == 1) {
    face_vertex_indices_previous_element_ = vertex_index;
  }
  else {
    (*ostream_) << std::get<0>(vertices_[   face_vertex_indices_first_element_])
         << " " << std::get<1>(vertices_[   face_vertex_indices_first_element_])
         << " " << std::get<2>(vertices_[   face_vertex_indices_first_element_])
         << " " << std::get<0>(vertices_[face_vertex_indices_previous_element_])
         << " " << std::get<1>(vertices_[face_vertex_indices_previous_element_])
         << " " << std::get<2>(vertices_[face_vertex_indices_previous_element_])
         << " " << std::get<0>(vertices_[                         vertex_index])
         << " " << std::get<1>(vertices_[                         vertex_index])
         << " " << std::get<2>(vertices_[                         vertex_index]) << "\n";
    face_vertex_indices_previous_element_ = vertex_index;
  }
  ++face_vertex_indices_element_index_;
}

void ply_to_raw_converter::face_vertex_indices_end()
{
}

void ply_to_raw_converter::face_end()
{
}

bool ply_to_raw_converter::convert(std::istream& istream, const std::string& istream_filename, std::ostream& ostream, const std::string& ostream_filename)
{
  ply::ply_parser::flags_type ply_parser_flags = 0;
  ply::ply_parser ply_parser(ply_parser_flags);

  ply_parser.info_callback(std::bind(&ply_to_raw_converter::info_callback, this, std::ref(istream_filename), _1, _2));
  ply_parser.warning_callback(std::bind(&ply_to_raw_converter::warning_callback, this, std::ref(istream_filename), _1, _2));
  ply_parser.error_callback(std::bind(&ply_to_raw_converter::error_callback, this, std::ref(istream_filename), _1, _2)); 

  ply_parser.element_definition_callback(std::bind(&ply_to_raw_converter::element_definition_callback, this, _1, _2));

  ply::ply_parser::scalar_property_definition_callbacks_type scalar_property_definition_callbacks;
  at<ply::float32>(scalar_property_definition_callbacks) = std::bind(&ply_to_raw_converter::scalar_property_definition_callback<ply::float32>, this, _1, _2);
  ply_parser.scalar_property_definition_callbacks(scalar_property_definition_callbacks);

  ply::ply_parser::list_property_definition_callbacks_type list_property_definition_callbacks;
  at<ply::uint8, ply::int32>(list_property_definition_callbacks) = std::bind(&ply_to_raw_converter::list_property_definition_callback<ply::uint8, ply::int32>, this, _1, _2);
  ply_parser.list_property_definition_callbacks(list_property_definition_callbacks);

  ostream_ = &ostream;

  return ply_parser.parse(istream);
}

int main(int argc, char* argv[])
{
  int argi;
  for (argi = 1; argi < argc; ++argi) {

    if (argv[argi][0] != '-') {
      break;
    }
    if (argv[argi][1] == 0) {
      ++argi;
      break;
    }
    char short_opt, *long_opt, *opt_arg;
    if (argv[argi][1] != '-') {
      short_opt = argv[argi][1];
      opt_arg = &argv[argi][2];
      long_opt = &argv[argi][2];
      while (*long_opt != '\0') {
        ++long_opt;
      }
    }
    else {
      short_opt = 0;
      long_opt = &argv[argi][2];
      opt_arg = long_opt;
      while ((*opt_arg != '=') && (*opt_arg != '\0')) {
        ++opt_arg;
      }
      if (*opt_arg == '=') {
        *opt_arg++ = '\0';
      }
    }

    if ((short_opt == 'h') || (std::strcmp(long_opt, "help") == 0)) {
      std::cout << "Usage: ply2raw [OPTION] [[INFILE] OUTFILE]\n";
      std::cout << "Convert from PLY to POV-Ray RAW triangle format.\n";
      std::cout << "\n";
      std::cout << "  -h, --help       display this help and exit\n";
      std::cout << "  -v, --version    output version information and exit\n";
      std::cout << "\n";
      std::cout << "With no INFILE/OUTFILE, or when INFILE/OUTFILE is -, read standard input/output.\n";
      std::cout << "\n";
      std::cout << "The following PLY elements and properties are supported.\n";
      std::cout << "  element vertex\n";
      std::cout << "    property float32 x\n";
      std::cout << "    property float32 y\n";
      std::cout << "    property float32 z\n";
      std::cout << "  element face\n";
      std::cout << "    property list uint8 int32 vertex_indices.\n";
      std::cout << "\n";
      std::cout << "Report bugs to " << PLY_PACKAGE_BUGREPORT << "\n";
      return EXIT_SUCCESS;
    }

    else if ((short_opt == 'v') || (std::strcmp(long_opt, "version") == 0)) {
      std::cout << "ply2raw (" << PLY_PACKAGE_NAME << ") " << PLY_PACKAGE_VERSION << "\n";
      std::cout << "Copyright (C) 2007 " << PLY_PACKAGE_AUTHOR << "\n";
      std::cout << "\n";
      std::cout << "This program is free software; you can redistribute it and/or modify\n";
      std::cout << "it under the terms of the GNU General Public License as published by\n";
      std::cout << "the Free Software Foundation; either version 2 of the License, or\n";
      std::cout << "(at your option) any later version.\n";
      std::cout << "\n";
      std::cout << "This program is distributed in the hope that it will be useful,\n";
      std::cout << "but WITHOUT ANY WARRANTY; without even the implied warranty of\n";
      std::cout << "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n";
      std::cout << "GNU General Public License for more details.\n";
      std::cout << "\n";
      std::cout << "You should have received a copy of the GNU General Public License\n";
      std::cout << "along with this program; if not, write to the Free Software\n";
      std::cout << "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA\n";
      return EXIT_SUCCESS;
    }

    else {
      std::cerr << "ply2raw: " << "invalid option `" << argv[argi] << "'" << "\n";
      std::cerr << "Try `" << argv[0] << " --help' for more information.\n";
      return EXIT_FAILURE;
    }
  }

  int parc = argc - argi;
  char** parv = argv + argi;
  if (parc > 2) {
    std::cerr << "ply2raw: " << "too many parameters" << "\n";
    std::cerr << "Try `" << argv[0] << " --help' for more information.\n";
    return EXIT_FAILURE;
  }

  std::ifstream ifstream;
  const char* istream_filename = "";
  if (parc > 0) {
    istream_filename = parv[0];
    if (std::strcmp(istream_filename, "-") != 0) {
      ifstream.open(istream_filename);
      if (!ifstream.is_open()) {
        std::cerr << "ply2raw: " << istream_filename << ": " << "no such file or directory" << "\n";
        return EXIT_FAILURE;
      }
    }
  }

  std::ofstream ofstream;
  const char* ostream_filename = "";
  if (parc > 1) {
    ostream_filename = parv[1];
    if (std::strcmp(ostream_filename, "-") != 0) {
      ofstream.open(ostream_filename);
      if (!ofstream.is_open()) {
        std::cerr << "ply2raw: " << ostream_filename << ": " << "could not open file" << "\n";
        return EXIT_FAILURE;
      }
    }
  }

  std::istream& istream = ifstream.is_open() ? ifstream : std::cin;
  std::ostream& ostream = ofstream.is_open() ? ofstream : std::cout;

  class ply_to_raw_converter ply_to_raw_converter;
  return ply_to_raw_converter.convert(istream, istream_filename, ostream, ostream_filename);
}
