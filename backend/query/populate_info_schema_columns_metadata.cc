//
// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <iostream>
#include <string>

#include "absl/flags/parse.h"
#include "zetasql/base/logging.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/substitute.h"
#include "riegeli/bytes/cfile_reader.h"
#include "riegeli/csv/csv_reader.h"
#include "riegeli/csv/csv_record.h"

using ::riegeli::CsvReader;
using ::riegeli::CsvReaderBase;
using ::riegeli::CsvRecord;
using ::riegeli::CFileReader;

static constexpr char kCsvSeparator = ',';

std::string PopulateInfoSchemaColumnsMetadata() {
  std::string metadata_code =
      R"(struct ColumnsMetaEntry {
  const char* table_name;
  const char* column_name;
  const char* is_nullable;
  const char* spanner_type;
};

inline const std::vector<ColumnsMetaEntry>& ColumnsMetadata() {
  // clang-format off
  static const zetasql_base::NoDestructor<std::vector<ColumnsMetaEntry>>
      kColumnsMetadata({
)";

  // clang-format off
  constexpr absl::string_view kInfoSchemaColumnsMetadata =
      "backend/query/info_schema_columns_metadata.csv"; // NOLINT
  // clang-format on
  CsvReaderBase::Options options;
  options.set_field_separator(kCsvSeparator);
  options.set_required_header(
      {"table_name", "column_name", "is_nullable", "spanner_type"});
  CFileReader file_reader = CFileReader(kInfoSchemaColumnsMetadata);
  CsvReader csv_reader(&file_reader, options);
  ZETASQL_CHECK(csv_reader.status().ok())
      << "Error reading csv file:" << csv_reader.status();
  absl::StrAppend(&metadata_code, "  // NOLINTBEGIN(whitespace/line_length)\n");
  for (CsvRecord record; csv_reader.ReadRecord(record);) {
    std::string table_name = absl::StrCat("\"", record["table_name"], "\"");
    std::string column_name = absl::StrCat("\"", record["column_name"], "\"");
    std::string is_nullable = absl::StrCat("\"", record["is_nullable"], "\"");
    std::string spanner_type = absl::StrCat("\"", record["spanner_type"], "\"");
    absl::StrAppend(&metadata_code, "    {", table_name, ", ", column_name,
                    ", ", is_nullable, ", ", spanner_type, "},\n");
  }
  absl::StrAppend(&metadata_code, R"(  });
  // NOLINTEND(whitespace/line_length)
  // clang-format on
    return *kColumnsMetadata;
}

)");
  ZETASQL_CHECK(csv_reader.Close()) << csv_reader.status();
  return metadata_code;
}

std::string PopulateInfoSchemaColumnsMetadataForIndex() {
  std::string metadata_for_index_code =
      R"(struct IndexColumnsMetaEntry {
  const char* table_name;
  const char* column_name;
  const char* is_nullable;
  const char* column_ordering;
  const char* spanner_type;
  int primary_key_ordinal = 0;
};

inline const std::vector<IndexColumnsMetaEntry>& IndexColumnsMetadata() {
  // clang-format off
  static const zetasql_base::NoDestructor<std::vector<IndexColumnsMetaEntry>>
      kColumnsMetadataForIndex({
)";

  // clang-format off
  constexpr absl::string_view kInfoSchemaColumnsMetadataForIndex =
      "backend/query/info_schema_columns_metadata_for_index.csv"; // NOLINT
  // clang-format on
  CsvReaderBase::Options options;
  options.set_field_separator(kCsvSeparator);
  options.set_required_header({"table_name", "column_name", "is_nullable",
                               "column_ordering", "spanner_type",
                               "ordinal_position"});
  CFileReader file_reader = CFileReader(kInfoSchemaColumnsMetadataForIndex);
  CsvReader csv_reader(&file_reader, options);
  ZETASQL_CHECK(csv_reader.status().ok())
      << "Error reading csv file:" << csv_reader.status();
  absl::StrAppend(&metadata_for_index_code,
                  "  // NOLINTBEGIN(whitespace/line_length)\n");

  for (CsvRecord record; csv_reader.ReadRecord(record);) {
    std::string table_name = absl::StrCat("\"", record["table_name"], "\"");
    std::string column_name = absl::StrCat("\"", record["column_name"], "\"");
    std::string is_nullable = absl::StrCat("\"", record["is_nullable"], "\"");
    std::string column_ordering =
        absl::StrCat("\"", record["column_ordering"], "\"");
    std::string spanner_type = absl::StrCat("\"", record["spanner_type"], "\"");
    std::string ordinal_position = record["ordinal_position"];

    absl::StrAppend(&metadata_for_index_code, "    {", table_name, ", ",
                    column_name, ", ", is_nullable, ", ", column_ordering, ", ",
                    spanner_type, ", ", ordinal_position, "},\n");
  }
  absl::StrAppend(&metadata_for_index_code, R"(  });
  // NOLINTEND(whitespace/line_length)
  // clang-format on
    return *kColumnsMetadataForIndex;
}

)");

  ZETASQL_CHECK(csv_reader.Close()) << csv_reader.status();
  return metadata_for_index_code;
}

int main(int argc, char* argv[]) {
  absl::ParseCommandLine(argc, argv);

  constexpr absl::string_view kTemplate =
      R"(#ifndef $0
#define $0

#include <string>
#include <vector>

#include "zetasql/base/no_destructor.h"

// WARNING -  DO NOT EDIT
// AUTOGENERATED FILE USING BUILD RULE:
// populate_info_schema_columns_metadata

namespace google::spanner::emulator::backend {

$1
$2
}  // namespace google::spanner::emulator::backend

#endif  // $0

)";

  std::cout << absl::Substitute(
      kTemplate,
      "THIRD_PARTY_CLOUD_SPANNER_EMULATOR_BACKEND_QUERY_INFO_SCHEMA_",
      PopulateInfoSchemaColumnsMetadata(),
      PopulateInfoSchemaColumnsMetadataForIndex());

  return 0;
}
