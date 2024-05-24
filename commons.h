#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <iostream>

#define BUF_LEN 1600
#define EXIT "exit"

#include <iostream>

// Macro for catching exceptions
#define TRY_CATCH(expression) \
  try { \
    expression; \
  } catch (const std::exception& e) { \
    std::cerr << "Exception caught: " << e.what() << std::endl; \
  } catch (...) { \
    std::cerr << "Unknown exception caught." << std::endl; \
  }



#endif