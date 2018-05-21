
/// System/STL
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
/// Local files
#include "Sparkline.h"



int main(int argc, char** argv) {

  float maxv = std::numeric_limits<float>::min();
  float minv = std::numeric_limits<float>::max();
  int height = 10;
  int width  = 0;
  std::string title = "SimplePlot";
  bool box   = true;
  bool color = true;

  #define INCREMENT_i_AND_CHECK ++i; if (i>=argc) break;
  for (int i = 1; i < argc; ++i) {
    if        (std::strcmp(argv[i], "-h"        ) == 0 or
               std::strcmp(argv[i], "--help"    ) == 0) {
      std::cout << "Usage: " << argv[0] << " [options]" << std::endl
                << std::endl
                << "Plot stuff like this:" << std::endl << std::endl
                << Sparkline::ShowExampleGaussian() << std::endl
                << "Values to be plotted are read from STDIN." << std::endl
                << std::endl
                << "Options:" << std::endl
                << "  --max      " << "Upper plot y-limit" << std::endl
                << "  --min      " << "Lower plot y-limit" << std::endl
                << "  --height   " << "Plot height in lines" << std::endl
                << "  --width    " << "Plot width in characters" << std::endl
                << "  --title    " << "Plot title" << std::endl
                << "  --no-box   " << "Disable enclosing box" << std::endl
                << "  --no-color " << "Disable color output" << std::endl
                << std::endl;
      return EXIT_FAILURE;
    } else if (std::strcmp(argv[i], "--max"     ) == 0) {
      INCREMENT_i_AND_CHECK;
      maxv = std::atof(argv[i]);
    } else if (std::strcmp(argv[i], "--min"     ) == 0) {
      INCREMENT_i_AND_CHECK;
      minv = std::atof(argv[i]);
    } else if (std::strcmp(argv[i], "--height"  ) == 0) {
      INCREMENT_i_AND_CHECK;
      height = std::atoi(argv[i]);
    } else if (std::strcmp(argv[i], "--width"   ) == 0) {
      INCREMENT_i_AND_CHECK;
      width = std::atoi(argv[i]);
    } else if (std::strcmp(argv[i], "--title"   ) == 0) {
      INCREMENT_i_AND_CHECK;
      title = argv[i];
    } else if (std::strcmp(argv[i], "--no-box"  ) == 0) {
      box = false;
    } else if (std::strcmp(argv[i], "--no-color") == 0) {
      color = false;
    } else {
      std::cerr << "Unrecognized option: \"" << argv[i] << "\"" << std::endl;
    }
  }
  #undef INCREMENT_i_AND_CHECK

  std::vector<float> data;
  {
    float dummy;
    while (!std::cin.eof())
    {
      std::cin >> dummy;
      if (std::cin.fail())
        break;
      data.push_back(dummy);
    }
  }

  std::cout << Sparkline::Sparkline<float>(data.data(),
                                           data.size(),
                                           height,
                                           width,
                                           box,
                                           color,
                                           title,
                                           minv,
                                           maxv) 
            << std::endl;

  /// Bye!
  return EXIT_SUCCESS;
}

