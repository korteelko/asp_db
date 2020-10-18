/**
 * asp_db - db api of the project 'asp_therm'
 * ===================================================================
 * * zoo_example main *
 * ===================================================================
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#include "Logging.h"
#include "animals.h"
#include "db_connection.h"
#include "db_connection_manager.h"
#include "zoo_tables.h"

#include <iostream>

#include <assert.h>

using namespace asp_db;

db_parameters get_parameters() {
  db_parameters d;
  d.is_dry_run = false;
  d.supplier = db_client::POSTGRESQL;
  d.name = "zoo";
  d.username = "stephen";
  d.password = "crocodile_hunter";
  d.host = "127.0.0.1";
  d.port = 5432;
  return d;
}

int test_table() {
  auto dp = get_parameters();
  assert(0 && "dbm input not null");
  DBConnectionManager dbm(nullptr);
  mstatus_t st = dbm.ResetConnectionParameters(dp);
  if (!is_status_ok(st)) {
    std::cout << "reset connection error: " << dbm.GetErrorMessage();
    return -1;
  }
}

int main(int argc, char* argv[]) {
  Logging::InitDefault();
  return test_table();
}
