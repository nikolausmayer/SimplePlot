/**
 * ===================================================================
 *
 * Author: Nikolaus Mayer, 2018 (mayern@cs.uni-freiburg.de)
 *
 * Sparkline++
 *
 * Plot pretty 1d data graphs
 *
 * ===================================================================
 *
 * Example output:
 *
 * ╭─────────Gaussian────────╮
 * │         ▂▅▇█▇▅▂         ├ max: 0.0997356
 * │       ▃▆███████▆▃       ├      0.0500025
 * │▁▁▁▂▃▅█████████████▅▃▂▁▁▁├ min: 0.00110796
 * ╰┬──────┬──────┬─────────┬╯
 *  0      7      14        25
 *
 * Usage example that produces this output:
 *
 * >
 * > #include <iostream>
 * > #include <string>
 * > #include <vector>
 * > #include "Sparkline.h"
 * >
 * > int main(int argv, char** argv) {
 * >   std::vector<float> data_vector;
 * >   /// (... fill vector with normal distribution curve ...)
 * >   
 * >   Sparkline::Configuration config;
 * >   config.setHeight(3);            // Plot height
 * >   config.setWidth(25);            // Plot width
 * >   config.setMin(0.0f);            // Min value
 * >   config.setMax(1.0f);            // Max value
 * >   config.setBox(true);            // Draw box outline
 * >   config.setTitle("Gaussian");    // Add caption
 * >   
 * >   std::cout << Sparkline::Sparkline( data_vector, config )
 * >             << std::endl;
 * >   
 * >   return 0;
 * > }
 * >
 *
 * ===================================================================
 */

#ifndef SPARKLINE_H__
#define SPARKLINE_H__

/// Set flag here or via -DWITH_TEXTDECORATOR in build command
//#define WITH_TEXTDECORATOR

/// If true, use unicode characters for better graphics (else ASCII)
#define USE_UNICODE_GRAPHICS

// System/STL
#include <cmath>          // log10
#include <iomanip>        // std::setw, std::setfill
#include <iostream>       // std::left
#include <limits>
#include <vector>
#include <sstream>        // std::ostringstream
#include <string>
#include <sys/ioctl.h>    // ioctl()
#include <unistd.h>       // STDOUT_FILENO
// Local files
#ifdef WITH_TEXTDECORATOR
  #include "TextDecorator.h"
  #define   RED(x) TD.red(x)
  #define  BLUE(x) TD.blue(x)
  #define GREEN(x) TD.green(x)
#else
  #define   RED(x) x
  #define  BLUE(x) x
  #define GREEN(x) x
#endif



/// ///////////////////////////////////////////////////////////////////
/// Helper functions
/// ///////////////////////////////////////////////////////////////////
namespace SparklineHelpers {

  /**
   * Get terminal width
   *
   * @returns The current terminal width (in characters)
   */
  unsigned short int TerminalWidth()
  {
    /// Defined in bits/ioctl-types.h
    struct winsize w;
    ioctl( STDOUT_FILENO, TIOCGWINSZ, &w );
    return w.ws_col;  // other is .ws_row
  }


  /**
   * Compute the character length of a number's string representation
   *
   * This works because the base-10 logarithm of a number corresponds
   * to the number's description length in base-10 math.
   *
   * @param n Input
   *
   * @returns The number of characters needed to pring "n"
   */
  size_t CharLength( size_t n )
  {
    return std::ceil(log10(n+1));
  }

}  // namespace SparklineHelpers





/// ///////////////////////////////////////////////////////////////////
/// Sparkline
/// ///////////////////////////////////////////////////////////////////
namespace Sparkline {


  /// /////////////////////////////////////////////////////////////////
  /// Some const values
  /// /////////////////////////////////////////////////////////////////

  /// │ .... ├ max: 1024.256
  ///               ^^^^^^^^
  const int PREC = 12;
  /// │ .... ├ max: 1024.256
  /// ^      ^^^^^^^
  const int ENCLOSURE_WIDTH = PREC+8;

  #ifdef USE_UNICODE_GRAPHICS
    /// Available ticks (▁▂▃▄▅▆▇█)
    /// From "The Unicode Standard, Version 7.0 - U2580" (Block elements)
    const std::string ticks[8] = { "\u2581",
                                   "\u2582",
                                   "\u2583",
                                   "\u2584",
                                   "\u2585",
                                   "\u2586",
                                   "\u2587",
                                   "\u2588" };
    const unsigned int TICKS = 8;
  #else
    const std::string ticks[3] = { ".", "o", "O" };
    const unsigned int TICKS = 3;
  #endif

  #ifdef USE_UNICODE_GRAPHICS
    /// Box outline chars (╭╮╰╯─┬│├) 
    /// From "The Unicode Standard, Version 7.0 - U2500" (Box drawing)
    ///              ╭────╮
    /// Example box: │test├
    ///              ╰┬──┬╯
    const std::string BOX_NW_CORNER     = "\u256d";  // ╭
    const std::string BOX_NE_CORNER     = "\u256e";  //   ╮
    const std::string BOX_SW_CORNER     = "\u2570";  // ╰
    const std::string BOX_SE_CORNER     = "\u256f";  //   ╯
    const std::string BOX_H_BORDER      = "\u2500";  // ─
    const std::string BOX_H_BORDER_TICK = "\u252c";  //   ─
    const std::string BOX_V_BORDER      = "\u2502";  // │
    const std::string BOX_V_BORDER_TICK = "\u251c";  //   ├
  #else
    /// Pure ASCII outline chars
    ///              +----+
    /// Example box: |test|
    ///              +,--,+
    const std::string BOX_NW_CORNER     = "+";
    const std::string BOX_NE_CORNER     = "+";
    const std::string BOX_SW_CORNER     = "+";
    const std::string BOX_SE_CORNER     = "+";
    const std::string BOX_H_BORDER      = "-";
    const std::string BOX_H_BORDER_TICK = ",";
    const std::string BOX_V_BORDER      = "|";
    const std::string BOX_V_BORDER_TICK = "|";  // (=BOX_V_BORDER)
  #endif



  /// /////////////////////////////////////////////////////////////////
  /// Configuration
  /// /////////////////////////////////////////////////////////////////

  /**
   * Store configurations for Sparkline() so that multiple Sparkline()
   * calls don't clutter the caller's code
   *
   * @param this_many_characters_wide Width of the sparkline
   * @param this_many_lines_high Line height of the plot (use higher
   *                             plots to see more details in the data)
   * @param enclose_in_box Iff TRUE, the sparkline plot will be 
   *                       surrounded by a box outline
   * @param title Optional caption for the plot
   * @param minv Optional minimum value for plot scaling; if used, also specify maxv!
   * @param maxv Optional maximum value for plot scaling
   */
  template <typename T>
  class Configuration {
    public:
      /// Constructor
      Configuration( size_t this_many_lines_high=1,
                     size_t this_many_characters_wide=0,
                     bool enclose_in_box=false,
                     bool print_colored=true,
                     const std::string& title="",
                     T minv = std::numeric_limits<T>::max(),
                     T maxv = std::numeric_limits<T>::min()
                   )
        : this_many_lines_high(this_many_lines_high),
          this_many_characters_wide(this_many_characters_wide),
          enclose_in_box(enclose_in_box),
          print_colored(print_colored),
          title(title),
          minv(minv),
          maxv(maxv)
      {};
      /// Destructor
      ~Configuration() {};

      /// Setters
      void setHeight( size_t v ) { this_many_lines_high=v; };
      void setWidth( size_t v ) { this_many_characters_wide=v; };
      void setBox( bool v ) { enclose_in_box=v; };
      void setColor( bool v ) { print_colored=v; };
      void setTitle( const std::string& v ) { title=v; };
      void setMin( T v ) { minv=v; };
      void setMax( T v ) { maxv=v; };

      /// Configuration parameters
      size_t this_many_lines_high;
      size_t this_many_characters_wide;
      bool enclose_in_box;
      bool print_colored;
      std::string title;
      T minv;
      T maxv;
  };





  /// /////////////////////////////////////////////////////////////////
  /// Sparklines
  /// /////////////////////////////////////////////////////////////////


  /**
   * Generate sparkline from data and return string representation
   *
   * @param data Input data as array
   * @param number_of_data_points The number of entries in "data"
   * @param this_many_lines_high Line height of the plot (use higher
   *        plots to see more details in the data)
   * @param this_many_characters_wide Character width of the plot; if 
   *        unspecified, the plot will be as many characters wide as
   *        there are data points
   * @param enclose_in_box Iff TRUE, the sparkline plot will be 
   *        surrounded by a box outline
   * @param title Optional caption for the plot
   * @param minv Optional minimum value for plot scaling; if used, also 
   *        specify maxv!
   * @param maxv Optional maximum value for plot scaling
   *
   * @returns A std::string containing the sparkline for "data"
   */
  template <typename T>  /*implicit parameter*/
  std::string Sparkline( const T* const data,
                         size_t number_of_data_points,
                         size_t this_many_lines_high=1,
                         size_t this_many_characters_wide=0,
                         bool enclose_in_box=false,
                         bool print_colored=true,
                         std::string title="",
                         T minv=std::numeric_limits<T>::max(),
                         T maxv=std::numeric_limits<T>::min()
                       ) 
  {
    /// Use provided min/max values or adapt to data range
    minv = std::min(minv, std::numeric_limits<T>::max());
    maxv = std::max(maxv, std::numeric_limits<T>::min());
    if ( minv == std::numeric_limits<T>::max() and
         maxv == std::numeric_limits<T>::min() ) {
      for ( size_t i = 0; i < number_of_data_points; ++i ) {
        minv = std::min(minv, data[i]);
        maxv = std::max(maxv, data[i]);
      }
    }

    /// If the plot could spill over the terminal boundaries,
    /// then limit its width
    float w_scale = 1.f;
    {
      size_t max_width = (size_t)SparklineHelpers::TerminalWidth();
      if ( enclose_in_box ) 
        max_width -= ENCLOSURE_WIDTH;

      if ( this_many_characters_wide == 0 )
        this_many_characters_wide = number_of_data_points;

      if ( max_width < this_many_characters_wide )
        this_many_characters_wide = max_width;

      w_scale = (float)this_many_characters_wide/number_of_data_points;
    }

    /// TODO 
    if ( this_many_characters_wide > number_of_data_points ) {
      throw std::runtime_error("Woops! Not implemented..");
    }

    /// Interpolate data points
    T bins[this_many_characters_wide];
    {
      float bin_slices_indices[this_many_characters_wide+1];
      for ( size_t i = 0; i <= this_many_characters_wide; ++i ) {
        bin_slices_indices[i] = i/w_scale;
      }

      const float mass_per_bin = 1/w_scale;

      for ( size_t i = 0; i < this_many_characters_wide; ++i ) {
        bins[i] = 0;

        const float lower = bin_slices_indices[i];
        const float upper = bin_slices_indices[i+1];
        bins[i] += (1.f-(lower-(size_t)lower)) * data[(size_t)lower];
        for (size_t j = (size_t)lower+1; j < (size_t)upper; ++j) {
          bins[i] += data[j];
        }
        if ((size_t)upper < number_of_data_points) {
          bins[i] += (upper-(size_t)upper) * data[(size_t)upper];
        }

        bins[i] /= mass_per_bin;
      }
    }
         

    #ifdef WITH_TEXTDECORATOR
      TextDecorator::TextDecorator TD(print_colored);
    #endif

    /// Assemble the output in a stringstream
    std::ostringstream oss;

    /// Begin box (upper border)
    if ( enclose_in_box ) {
      if ( title.compare("") == 0 ) {
        oss << GREEN(BOX_NW_CORNER);
        for ( size_t i = 0; i < this_many_characters_wide; ++i )
          oss << GREEN(BOX_H_BORDER);
        oss << GREEN(BOX_NE_CORNER) << '\n';
      } else if ( title.size() > this_many_characters_wide ) {
        oss << GREEN(title) << '\n';
      } else {
        size_t filler = this_many_characters_wide-title.size();
        oss << GREEN(BOX_NW_CORNER);
        size_t i = 0;
        /// Ticks left of the title
        while ( i < filler/2 ) {
          oss << GREEN(BOX_H_BORDER);
          ++i;
        }
        oss << GREEN(title);
        /// Skip ticks covered by title
        i += title.length();
        /// Ticks right of the title
        while ( i < this_many_characters_wide ) {
          oss << GREEN(BOX_H_BORDER);
          ++i;
        }
        oss << GREEN(BOX_NE_CORNER) << '\n';
      }
    }


    /// A higher line count means we can display the data more
    /// finely grained!
    size_t levels = this_many_lines_high*TICKS-1;
    /// Stretch plot over multiple lines if requested
    for ( int line = (int)this_many_lines_high-1; line >= 0; --line ) {
      int this_line_min_index = line*TICKS;
      int this_line_max_index = this_line_min_index+(TICKS-1);

      /// Left box border
      if ( enclose_in_box )
        oss << GREEN(BOX_V_BORDER);

      /// Go through all data points
      for ( size_t i = 0; i < this_many_characters_wide; ++i ) {
        const T _data = std::min(maxv, std::max(minv, bins[i]));
        float fraction = (float)(_data-minv)/(float)(maxv-minv);
        int index = std::floor(fraction*levels);
        if ( index < this_line_min_index ) {
          /// Current cell is above the data line -> empty
          oss << ' '; 
        } else if ( index > this_line_max_index ) {
          /// Current cell is below the data line -> solid
          oss << BLUE(ticks[TICKS-1]);
        } else {
          oss << BLUE(ticks[index-this_line_min_index]);
        }
      }

      /// Right box border and min/max value marks
      if ( enclose_in_box ) {
        if ( this_many_lines_high == 1 )
          oss << GREEN(BOX_V_BORDER_TICK) 
              << GREEN(" min: ")
              << std::left << std::setw(PREC) << GREEN(minv) 
              << GREEN(", max: ") 
              << std::left << std::setw(PREC) << GREEN(maxv);
        else if ( line == (int)this_many_lines_high-1 )
          oss << GREEN(BOX_V_BORDER_TICK) << GREEN(" max: ") << GREEN(maxv);
        else if ( line == 0 )
          oss << GREEN(BOX_V_BORDER_TICK) << GREEN(" min: ") << GREEN(minv);
        else
          /// Show "middle" level of this line
          oss << GREEN(BOX_V_BORDER_TICK) << GREEN("      ") 
              << GREEN((line*TICKS+4) * 
                  (maxv-minv)/(this_many_lines_high*TICKS) + minv);
          //oss << GREEN(BOX_V_BORDER);
      }

      if ( line > 0 )
        oss << '\n';
    }

    /// Finish box (lower border and sample index marks)
    if ( enclose_in_box ) {
      /// Compute tick marks positions
      const size_t sep = 2;
      const size_t x_ticks_separation = 2*sep + std::ceil(log10(number_of_data_points+1));
      const size_t x_ticks_number = this_many_characters_wide / x_ticks_separation + 1;
      size_t x_ticks[x_ticks_number];
      size_t x_ticks_values[x_ticks_number];
      for ( size_t i = 0; i < x_ticks_number-1; ++i ) {
        x_ticks[i]        = i*x_ticks_separation;
        x_ticks_values[i] = i*number_of_data_points/x_ticks_number;
      }
      x_ticks[x_ticks_number-1]        = this_many_characters_wide-1;
      x_ticks_values[x_ticks_number-1] = number_of_data_points;

      size_t next_tick = 0;
      oss << '\n';

      oss << GREEN(BOX_SW_CORNER);
      for ( size_t i = 0; i < this_many_characters_wide; ++i )
        if ( i == x_ticks[next_tick] ) {
          oss << GREEN(BOX_H_BORDER_TICK);
          ++next_tick;
        } else {
          oss << GREEN(BOX_H_BORDER);
        }
      oss << GREEN(BOX_SE_CORNER);


      /// X-ticks values
      {
        oss << "\n " << GREEN(x_ticks_values[0]);

        /// Keep track of the line length
        size_t current_col = 1;
        for ( size_t i = 1; i < x_ticks_number-1; ++i ) {
          /// We somehow have to distribute the string representations
          /// of the tick numbers over the length of one line.
          /// 
          ///           x_ticks_separation
          ///      vvvvvvv       >      vvvvvvv   
          ///                    > 
          ///       tofill       >       tofill
          ///        vvvv        >        vvv
          /// ...──┬──────┬──... > ...──┬──────┬──...
          ///     998    999     >     999   1000
          ///       ^    ^^      >       ^   ^^
          ///      lw    rw      >      lw   rw

          /// Width of the current tick's number
          size_t tmp = SparklineHelpers::CharLength(x_ticks_values[i]);
          current_col += tmp;
          /// This iteration takes tmp-tmp/2 characters of the total
          /// width of the next entry. The remaining tmp-2 characters
          /// will by covered by "lw" in the next iteration.
          const size_t rw = tmp-tmp/2;
          /// Width of the previous tick's number
          tmp = SparklineHelpers::CharLength(x_ticks_values[i-1]);
          const size_t lw = tmp/2;

          /// The remaining sepration space is filled with blanks
          size_t tofill = x_ticks_separation - rw - lw;
          oss << std::setw(tofill) << std::setfill(' ') << ' ';
          oss << GREEN(x_ticks_values[i]);

          current_col += tofill;
        }

        oss << std::setw(x_ticks[x_ticks_number-1]
               - current_col
               - SparklineHelpers::CharLength(number_of_data_points)+2)
            << ' ';
        oss << GREEN(x_ticks_values[x_ticks_number-1]) 
            << std::endl;
      }
    }

    return oss.str();
  };
  /// Yes C++, double CAN be used as float...
  std::string Sparkline( const float* const data,
                         size_t number_of_data_points,
                         size_t this_many_lines_high,
                         size_t this_many_characters_wide,
                         bool enclose_in_box,
                         bool print_colored,
                         std::string title,
                         double max,
                         double min
                       ) 
  {
    return Sparkline( data,
                      number_of_data_points,
                      this_many_lines_high,
                      this_many_characters_wide,
                      enclose_in_box,
                      print_colored,
                      title,
                      (float)max,
                      (float)min
                    );
  }
  


  /**
   * Generate sparkline from data using a Configuration object
   *
   * @param data Input data as array
   * @param number_of_data_points The number of entries in "data"
   * @param config Sparkline::Configuration object
   *
   * @returns A std::string containing the sparkline for "data"
   */
  template <typename T>  /*implicit parameter*/
  std::string Sparkline( const T* const data,
                         size_t number_of_data_points,
                         const Configuration<T>& config
                       )
  {
    return Sparkline( data,
                      number_of_data_points,
                      config.this_many_lines_high,
                      config.this_many_characters_wide,
                      config.enclose_in_box,
                      config.print_colored,
                      config.title,
                      config.minv,
                      config.maxv 
                    );
  };


  /**
   * Generate sparkline from data and return string representation
   *
   * @param data Input data as std::vector
   * @param this_many_lines_high Line height of the plot (use higher
   *        plots to see more details in the data)
   * @param enclose_in_box Iff TRUE, the sparkline plot will be 
   *        surrounded by a box outline
   *
   * @returns A std::string containing the sparkline for "data"
   */
  template <typename T>  /*implicit parameter*/
  std::string Sparkline( const std::vector<T>& data,
                         size_t this_many_lines_high=1,
                         size_t this_many_characters_wide=0,
                         bool enclose_in_box=false,
                         bool print_colored=true,
                         std::string title="",
                         T minv=std::numeric_limits<T>::max(),
                         T maxv=std::numeric_limits<T>::min()
                       )
  {
    /// TODO The standard guarantees that vector elements are
    /// stored contiguously, but does that hold for all compiler
    /// and language versions (VERY young issue; May 2014)?
    /// (www.open-std.org/jtc1/sc22/wg21/docs/lwg-defects.html#69)
    const T* const ptr = &data[0];
    return Sparkline( ptr,
                      (size_t)data.size(),
                      this_many_lines_high,
                      this_many_characters_wide,
                      enclose_in_box,
                      print_colored,
                      title,
                      minv,
                      maxv );
  };
  /// Yes C++, double CAN be used as float...
  std::string Sparkline( const std::vector<float>& data,
                         size_t this_many_lines_high,
                         size_t this_many_characters_wide,
                         bool enclose_in_box,
                         bool print_colored,
                         std::string title,
                         double max,
                         double min
                       ) 
  {
    return Sparkline( data,
                      this_many_lines_high,
                      this_many_characters_wide,
                      enclose_in_box,
                      print_colored,
                      title,
                      (float)max,
                      (float)min
                    );
  }


  /**
   * Generate sparkline from data using a Configuration object
   *
   * @param data Input data as std::vector
   * @param config Sparkline::Configuration object
   *
   * @returns A std::string containing the sparkline for "data"
   */
  template <typename T>  /*implicit parameter*/
  std::string Sparkline( const std::vector<T>& data,
                         const Configuration<T>& config
                       )
  {
    return Sparkline( data,
                      config.this_many_lines_high,
                      config.this_many_characters_wide,
                      config.enclose_in_box,
                      config.print_colored,
                      config.title,
                      config.minv,
                      config.maxv 
                    );
  };
  



  /**
   * Showcase: Display a normal distribution (Gaussian bell curve)
   */
  std::string ShowExampleGaussian()
  {
    std::vector<float> data_vec{
      0.000514092998764,
      0.00147728280398,
      0.00379866200793,
      0.0087406296979,
      0.0179969888377,
      0.0331590462642,
      0.054670024892,
      0.080656908173,
      0.106482668507,
      0.125794409231,
      0.132980760134,
      0.125794409231,
      0.106482668507,
      0.080656908173,
      0.054670024892,
      0.0331590462642,
      0.0179969888377,
      0.0087406296979,
      0.00379866200793,
      0.00147728280398,
      0.000514092998764
    };

    Configuration<float> config;
    config.setHeight(3);
    config.setMin(0);
    config.setMax(0.15);
    config.setBox(true);
    config.setTitle("Gaussian");
    config.setColor(false);
    return Sparkline(data_vec, config);
  }


  /**
   * Showcase: Display a sine wave using various configurations
   */
  std::string ShowExamples()
  {
    std::ostringstream oss;
    oss << "\n"
        << "############################################################\n"
        << "#                Sparklines: Showcase examples             #\n"
        << "############################################################\n"
        << "\n\n";

    /// Generate two cycles of a sine wave
    std::vector<float> data_vec;
    for ( size_t i = 0; i <= 100; ++i ) {
      data_vec.push_back( sin((i*7.2)*3.14/180.) );
    }

    /// 40x10, box
    Configuration<float> config;
    config.setHeight(10);                    /// Make the plot 10 lines high
    config.setWidth(40);                     /// and 80 characters wide,
    config.setBox(true);                     /// surround it with a box
    config.setTitle("Showcase: With box, size 40x10");   /// and add a caption
    oss << Sparkline(data_vec, config) << '\n';

    /// 40x3, box
    config.setHeight(3);
    config.setTitle("Showcase: With box, size 40x3");
    oss << Sparkline(data_vec, config) << '\n';

    /// 40x1, no box
    config.setHeight(1);
    config.setBox(false);
    oss << "Showcase: Without box, size 40x1 (the original 'sparkline')\n"
        << Sparkline(data_vec, config) << "\n\n";

    /// 80x10, no box
    config.setHeight(10);
    config.setWidth(80);
    oss << "Showcase: Without box, size 80x10\n"
        << Sparkline(data_vec, config) << '\n';

    /// 80x10, box
    config.setHeight(10);
    config.setWidth(80);
    config.setBox(true);
    config.setTitle("Showcase: With box, size 80x10");
    oss << '\n' << Sparkline(data_vec, config) << '\n';

    /// 80x10, box, with artificial range limits
    config.setHeight(10);
    config.setWidth(80);
    config.setMin(-2.);
    config.setMax(4.);
    config.setBox(true);
    config.setTitle("Showcase: With box, size 80x10, y-range [-2,4]");
    oss << Sparkline(data_vec, config) << '\n';

    /// 80x10, box, with artificial range limits
    config.setHeight(10);
    config.setWidth(80);
    config.setMin(-.25);
    config.setMax(1.25);
    config.setBox(true);
    config.setTitle("Showcase: With box, size 80x10, y-range [-0.25,1.25]");
    oss << Sparkline(data_vec, config) << '\n';

    /// 80x10, box, no colors
    config.setColor(false);
    config.setTitle("Showcase: With box, size 80x10, no colors");
    oss << '\n' << Sparkline(data_vec, config) << '\n';


    return oss.str();
  }


}  // namespace Sparkline



/// Undefine RED, BLUE, GREEN
#ifdef WITH_TEXTDECORATOR
  #undef   RED
  #undef  BLUE
  #undef GREEN
#endif

#endif  // SPARKLINE_H__

