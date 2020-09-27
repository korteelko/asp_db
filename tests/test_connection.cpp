#include "gtest/gtest.h"

#include "db_connection.h"


using namespace asp_db;

TEST(db_parameters, Construct) {
  db_parameters p = db_parameters();
  p.host = "localhost";
  p.port = 12333;
  p.is_dry_run = false;
  p.name = "name";
  p.password = "pass";
  p.supplier = db_client::POSTGRESQL;
  p.username = "user";
  db_parameters pp(p);
  EXPECT_EQ(pp.host, p.host);
  EXPECT_EQ(pp.host, "localhost");
  EXPECT_EQ(pp.port, p.port);
  EXPECT_EQ(pp.is_dry_run, p.is_dry_run);
  EXPECT_EQ(pp.name, p.name);
  EXPECT_EQ(pp.password, p.password);
  EXPECT_EQ(pp.supplier, p.supplier);
  EXPECT_EQ(pp.username, p.username);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
