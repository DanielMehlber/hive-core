#include "MultipartFormdataTest.h"
#include "UrlParserTest.h"
#include "WebSocketConverterTest.h"
#include "WebSocketEndpointTest.h"
#include <gtest/gtest.h>

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}