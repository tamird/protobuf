// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstddef>
#include <iterator>
#include <string>

#include <benchmark/benchmark.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/json/json.h"
#include "google/protobuf/json/json_enumval_custom_string.pb.h"

#ifndef EXPECT_OK
#define EXPECT_OK(x) EXPECT_TRUE(x.ok())
#endif  // EXPECT_OK

namespace google {
namespace protobuf {
namespace {

using json_enumval_custom_string::Armor;
using json_enumval_custom_string::Knight;

// Gorget does not have a custom json enumval string set, so it defaults to
// the original enumval: ARMOR_GORGET.
TEST(JsonEnumvalCustomStringTest, GorgetDefaultSerialization) {
  Knight msg;
  msg.set_armor(Armor::ARMOR_GORGET);
  EXPECT_EQ(msg.armor(), Armor::ARMOR_GORGET);

  std::string json_res{};
  absl::Status status = json::MessageToJsonString(msg, &json_res);
  EXPECT_OK(status);
  EXPECT_EQ(json_res, R"json({"armor":"ARMOR_GORGET"})json");
}

// The great helm does have a custom json enumval, so let's confirm.
TEST(JsonEnumvalCustomStringTest, GreatHelmSerialization) {
  Knight msg;
  msg.set_armor(Armor::ARMOR_GREAT_HELM);
  EXPECT_EQ(msg.armor(), Armor::ARMOR_GREAT_HELM);

  std::string json_res{};
  absl::Status status = json::MessageToJsonString(msg, &json_res);
  EXPECT_OK(status);
  EXPECT_EQ(json_res, R"json({"armor":"gr8 helm"})json");
}

// Test that we accept the raw enum name as input even when a custom JSON
// enumval string option is set for Great Helm.
TEST(JsonEnumvalCustomStringTest, GreatHelmRawNameAccepted) {
  Knight msg;
  absl::Status status = json::JsonStringToMessage(
      R"json({"armor":"ARMOR_GREAT_HELM"})json", &msg);
  EXPECT_OK(status);
  EXPECT_EQ(msg.armor(), Armor::ARMOR_GREAT_HELM);
}

// The gauntlet has a double quote in the middle of its custom json enumval,
// let's make sure special characters are escaped properly.
TEST(JsonEnumvalCustomStringTest, GauntletSerialization) {
  Knight msg;
  msg.set_armor(Armor::ARMOR_GAUNTLET);
  EXPECT_EQ(msg.armor(), Armor::ARMOR_GAUNTLET);
  std::string json_res{};
  absl::Status status = json::MessageToJsonString(msg, &json_res);
  EXPECT_OK(status);
  EXPECT_EQ(json_res, "{\"armor\":\"a\\\"b\"}");
  // Roundtrip to make sure we can parse it back
  Knight msg2;
  absl::Status parse_status = json::JsonStringToMessage(json_res, &msg2);
  EXPECT_OK(parse_status);
  EXPECT_EQ(msg2.armor(), Armor::ARMOR_GAUNTLET);
}

// Test the intersection of case-insensitive and custom json enumval.
TEST(JsonEnumvalCustomStringTest, CaseInsensitiveGauntletParsing) {
  std::string json_res = "{\"armor\":\"A\\\"b\"}";
  json::ParseOptions parse_options;
  parse_options.case_insensitive_enum_parsing = true;

  Knight msg2;
  absl::Status parse_status =
      json::JsonStringToMessage(json_res, &msg2, parse_options);

  EXPECT_OK(parse_status);
  EXPECT_EQ(msg2.armor(), Armor::ARMOR_GAUNTLET);
}

// Test quotes surrounding the custom json enumval.
TEST(JsonEnumvalCustomStringTest, DoubleQuoteEnumSerialization) {
  Knight msg;
  msg.set_armor(Armor::ARMOR_PLATE);
  EXPECT_EQ(msg.armor(), Armor::ARMOR_PLATE);
  std::string json_res{};
  absl::Status status = json::MessageToJsonString(msg, &json_res);
  EXPECT_OK(status);
  EXPECT_EQ(json_res, "{\"armor\":\"\\\"plate\\\"\"}");
  // Roundtrip to make sure we can parse it back
  Knight msg2;
  absl::Status parse_status = json::JsonStringToMessage(json_res, &msg2);
  EXPECT_OK(parse_status);
  EXPECT_EQ(msg2.armor(), Armor::ARMOR_PLATE);
}

// Coif is an empty custom string, let's make sure that works.
TEST(JsonEnumvalCustomStringTest, CoifEmptySerialization) {
  Knight msg;
  msg.set_armor(Armor::ARMOR_COIF);
  EXPECT_EQ(msg.armor(), Armor::ARMOR_COIF);
  std::string json_res{};
  absl::Status status = json::MessageToJsonString(msg, &json_res);
  EXPECT_OK(status);
  EXPECT_EQ(json_res, R"json({"armor":""})json");
  // Roundtrip to make sure we can parse it back
  Knight msg2;
  absl::Status parse_status = json::JsonStringToMessage(json_res, &msg2);
  EXPECT_OK(parse_status);
  EXPECT_EQ(msg2.armor(), Armor::ARMOR_COIF);
}

// Ensure newlines and tabs are properly escaped.
TEST(JsonEnumvalCustomStringTest, PauldronEscapingSerialization) {
  Knight msg;
  msg.set_armor(Armor::ARMOR_PAULDRON);
  EXPECT_EQ(msg.armor(), Armor::ARMOR_PAULDRON);
  std::string json_res{};
  absl::Status status = json::MessageToJsonString(msg, &json_res);
  EXPECT_OK(status);

  EXPECT_EQ(json_res, R"json({"armor":"p\taul\ndron"})json");
  // Roundtrip to make sure we can parse it back
  Knight msg2;
  absl::Status parse_status = json::JsonStringToMessage(json_res, &msg2);
  EXPECT_OK(parse_status);
  EXPECT_EQ(msg2.armor(), Armor::ARMOR_PAULDRON);
}

// Test that aliased enum values both serialize to the correct custom string.
TEST(JsonEnumvalCustomStringTest, AliasedSerialization) {
  {
    Knight msg;
    msg.set_armor(Armor::ARMOR_SABATON);
    EXPECT_EQ(msg.armor(), Armor::ARMOR_SABATON);

    std::string json_res{};
    absl::Status status = json::MessageToJsonString(msg, &json_res);
    EXPECT_OK(status);
    EXPECT_EQ(json_res, R"json({"armor":"sabaton"})json");

    Knight msg2;
    absl::Status parse_status = json::JsonStringToMessage(json_res, &msg2);
    EXPECT_OK(parse_status);
    EXPECT_EQ(msg2.armor(), Armor::ARMOR_SABATON);
  }

  {
    Knight msg;
    msg.set_armor(Armor::ARMOR_SOLLERET);
    EXPECT_EQ(msg.armor(), Armor::ARMOR_SOLLERET);

    std::string json_res{};
    absl::Status status = json::MessageToJsonString(msg, &json_res);
    EXPECT_OK(status);
    EXPECT_EQ(json_res, R"json({"armor":"sabaton"})json");

    Knight msg2;
    absl::Status parse_status = json::JsonStringToMessage(json_res, &msg2);
    EXPECT_OK(parse_status);
    EXPECT_EQ(msg2.armor(), Armor::ARMOR_SOLLERET);
  }
}

// Test that we accept the raw enum names as input even when a custom JSON
// enumval string option is set for aliased values.
TEST(JsonEnumvalCustomStringTest, AliasedRawNamesAccepted) {
  {
    Knight msg;
    absl::Status status =
        json::JsonStringToMessage(R"json({"armor":"ARMOR_SABATON"})json", &msg);
    EXPECT_OK(status);
    EXPECT_EQ(msg.armor(), Armor::ARMOR_SABATON);
  }
  {
    Knight msg;
    absl::Status status = json::JsonStringToMessage(
        R"json({"armor":"ARMOR_SOLLERET"})json", &msg);
    EXPECT_OK(status);
    EXPECT_EQ(msg.armor(), Armor::ARMOR_SOLLERET);
  }
}

// Test that custom enumval option string containing numeric digits (e.g., "8")
// serializes properly and can be parsed back as a string, and is also parsed
// correctly as an integer input.
TEST(JsonEnumvalCustomStringTest, NumericStringCustomOption) {
  {
    Knight msg;
    msg.set_armor(Armor::ARMOR_HACHI_MAI_DO);
    EXPECT_EQ(msg.armor(), Armor::ARMOR_HACHI_MAI_DO);

    std::string json_res{};
    absl::Status status = json::MessageToJsonString(msg, &json_res);
    EXPECT_OK(status);
    EXPECT_EQ(json_res, R"json({"armor":"8"})json");

    Knight msg2;
    absl::Status parse_status = json::JsonStringToMessage(json_res, &msg2);
    EXPECT_OK(parse_status);
    EXPECT_EQ(msg2.armor(), Armor::ARMOR_HACHI_MAI_DO);
  }
  {
    // Verify that the integer input 8 also parses successfully.
    Knight msg;
    absl::Status status =
        json::JsonStringToMessage(R"json({"armor":8})json", &msg);
    EXPECT_OK(status);
    EXPECT_EQ(msg.armor(), Armor::ARMOR_HACHI_MAI_DO);
  }
}

// Int overrides always win.
TEST(JsonEnumvalCustomStringTest, GreatHelmIntOverride) {
  Knight msg;
  msg.set_armor(Armor::ARMOR_GREAT_HELM);
  std::string json_res;
  json::PrintOptions print_options;
  print_options.always_print_enums_as_ints = true;
  absl::Status status =
      json::MessageToJsonString(msg, &json_res, print_options);
  EXPECT_OK(status);
  EXPECT_EQ(json_res, R"json({"armor":1})json");
}

constexpr absl::string_view kDefaultPayloads[] = {
    R"json({"armor":"ARMOR_GREAT_HELM"})json",
    R"json({"armor":"ARMOR_GAUNTLET"})json",
    R"json({"armor":"ARMOR_PLATE"})json",
    R"json({"armor":"ARMOR_COIF"})json",
    R"json({"armor":"ARMOR_PAULDRON"})json",
    R"json({"armor":"ARMOR_SABATON"})json",
    R"json({"armor":"ARMOR_HACHI_MAI_DO"})json",
};

constexpr absl::string_view kDefaultCaselessPayloads[] = {
    R"json({"armor":"armor_great_helm"})json",
    R"json({"armor":"Armor_Gauntlet"})json",
    R"json({"armor":"ARMOR_plate"})json",
    R"json({"armor":"armor_Coif"})json",
    R"json({"armor":"ArmOr_PaulDrOn"})json",
    R"json({"armor":"armor_sabaton"})json",
    R"json({"armor":"ARMOR_hachi_mai_do"})json",
};

constexpr absl::string_view kCustomPayloads[] = {
    R"json({"armor":"gr8 helm"})json",
    R"json({"armor":"a\"b"})json",
    R"json({"armor":"\"plate\""})json",
    R"json({"armor":""})json",
    R"json({"armor":"p\taul\ndron"})json",
    R"json({"armor":"sabaton"})json",
    R"json({"armor":"8"})json",
};

constexpr absl::string_view kCustomCaselessPayloads[] = {
    R"json({"armor":"GR8 HELM"})json",
    R"json({"armor":"A\"B"})json",
    R"json({"armor":"\"PlAtE\""})json",
    R"json({"armor":""})json",
    R"json({"armor":"P\tAuL\nDrOn"})json",
    R"json({"armor":"SABATON"})json",
    R"json({"armor":"8"})json",
};

constexpr absl::string_view kUnknownPayloads[] = {
    R"json({"armor":"UNKNOWN_1"})json", R"json({"armor":"UNKNOWN_2"})json",
    R"json({"armor":"UNKNOWN_3"})json", R"json({"armor":"UNKNOWN_4"})json",
    R"json({"armor":"UNKNOWN_5"})json", R"json({"armor":"UNKNOWN_6"})json",
    R"json({"armor":"UNKNOWN_7"})json",
};

static_assert(
    std::size(kDefaultPayloads) == std::size(kDefaultCaselessPayloads),
    "benchmark payloads must have the same size to let the benchmark results "
    "be meaningfully comparable");
static_assert(
    std::size(kDefaultPayloads) == std::size(kCustomPayloads),
    "benchmark payloads must have the same size to let the benchmark results "
    "be meaningfully comparable");
static_assert(
    std::size(kDefaultPayloads) == std::size(kCustomCaselessPayloads),
    "benchmark payloads must have the same size to let the benchmark results "
    "be meaningfully comparable");
static_assert(
    std::size(kDefaultPayloads) == std::size(kUnknownPayloads),
    "benchmark payloads must have the same size to let the benchmark results "
    "be meaningfully comparable");

void BM_ParseJsonDefault(benchmark::State& state) {
  constexpr int kNumParses = 100;
  constexpr size_t batch_size = std::size(kDefaultPayloads) * kNumParses;
  while (state.KeepRunningBatch(batch_size)) {
    for (absl::string_view payload : kDefaultPayloads) {
      for (int i = 0; i < kNumParses; ++i) {
        google::protobuf::Arena arena;
        Knight* msg = google::protobuf::Arena::Create<Knight>(&arena);
        absl::Status parse_status = json::JsonStringToMessage(payload, msg);
        ABSL_CHECK(parse_status.ok());
      }
    }
  }
}
BENCHMARK(BM_ParseJsonDefault);

void BM_ParseJsonDefaultCaseless(benchmark::State& state) {
  constexpr int kNumParses = 100;
  json::ParseOptions options;
  options.case_insensitive_enum_parsing = true;
  constexpr size_t batch_size =
      std::size(kDefaultCaselessPayloads) * kNumParses;
  while (state.KeepRunningBatch(batch_size)) {
    for (absl::string_view payload : kDefaultCaselessPayloads) {
      for (int i = 0; i < kNumParses; ++i) {
        google::protobuf::Arena arena;
        Knight* msg = google::protobuf::Arena::Create<Knight>(&arena);
        absl::Status parse_status =
            json::JsonStringToMessage(payload, msg, options);
        ABSL_CHECK(parse_status.ok());
      }
    }
  }
}
BENCHMARK(BM_ParseJsonDefaultCaseless);

void BM_ParseJsonCustom(benchmark::State& state) {
  constexpr int kNumParses = 100;
  constexpr size_t batch_size = std::size(kCustomPayloads) * kNumParses;
  while (state.KeepRunningBatch(batch_size)) {
    for (absl::string_view payload : kCustomPayloads) {
      for (int i = 0; i < kNumParses; ++i) {
        google::protobuf::Arena arena;
        Knight* msg = google::protobuf::Arena::Create<Knight>(&arena);
        absl::Status parse_status = json::JsonStringToMessage(payload, msg);
        ABSL_CHECK(parse_status.ok());
      }
    }
  }
}
BENCHMARK(BM_ParseJsonCustom);

void BM_ParseJsonCustomCaseless(benchmark::State& state) {
  constexpr int kNumParses = 100;
  json::ParseOptions options;
  options.case_insensitive_enum_parsing = true;
  constexpr size_t batch_size = std::size(kCustomCaselessPayloads) * kNumParses;
  while (state.KeepRunningBatch(batch_size)) {
    for (absl::string_view payload : kCustomCaselessPayloads) {
      for (int i = 0; i < kNumParses; ++i) {
        google::protobuf::Arena arena;
        Knight* msg = google::protobuf::Arena::Create<Knight>(&arena);
        absl::Status parse_status =
            json::JsonStringToMessage(payload, msg, options);
        ABSL_CHECK(parse_status.ok());
      }
    }
  }
}
BENCHMARK(BM_ParseJsonCustomCaseless);

void BM_ParseJsonUnknown(benchmark::State& state) {
  constexpr int kNumParses = 100;
  constexpr size_t batch_size = std::size(kUnknownPayloads) * kNumParses;
  while (state.KeepRunningBatch(batch_size)) {
    for (absl::string_view payload : kUnknownPayloads) {
      for (int i = 0; i < kNumParses; ++i) {
        google::protobuf::Arena arena;
        Knight* msg = google::protobuf::Arena::Create<Knight>(&arena);
        absl::Status parse_status = json::JsonStringToMessage(payload, msg);
        ABSL_CHECK(!parse_status.ok());
      }
    }
  }
}
BENCHMARK(BM_ParseJsonUnknown);

}  // namespace
}  // namespace protobuf
}  // namespace google
